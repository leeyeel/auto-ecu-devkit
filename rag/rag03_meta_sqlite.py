from __future__ import annotations
import uuid
import hashlib
import sqlite3
from pathlib import Path
from dataclasses import dataclass
from typing import List, Optional, Dict, Any

@dataclass
class ChunkRow:
    chunk_id: str
    doc: str
    source_pdf: str
    page_start: int
    page_end: int
    section_title: Optional[str]
    text: str
    sha1: str

def sha1_text(s: str) -> str:
    return hashlib.sha1(s.encode("utf-8", errors="ignore")).hexdigest()

class MetaStore:
    def __init__(self, sqlite_path: Path):
        self.sqlite_path = sqlite_path
        self.sqlite_path.parent.mkdir(parents=True, exist_ok=True)
        self.conn = sqlite3.connect(str(self.sqlite_path))
        self.conn.execute("PRAGMA journal_mode=WAL;")
        self._init_schema()

    def _init_schema(self) -> None:
        self.conn.executescript(
            """
            CREATE TABLE IF NOT EXISTS chunks (
              chunk_id TEXT PRIMARY KEY,
              doc TEXT NOT NULL,
              source_pdf TEXT NOT NULL,
              page_start INTEGER NOT NULL,
              page_end INTEGER NOT NULL,
              section_title TEXT,
              text TEXT NOT NULL,
              sha1 TEXT NOT NULL UNIQUE
            );

            CREATE TABLE IF NOT EXISTS faiss_map (
              faiss_id INTEGER PRIMARY KEY,
              chunk_id TEXT NOT NULL,
              FOREIGN KEY(chunk_id) REFERENCES chunks(chunk_id)
            );

            CREATE INDEX IF NOT EXISTS idx_chunks_doc ON chunks(doc);
            CREATE INDEX IF NOT EXISTS idx_chunks_pages ON chunks(page_start, page_end);
            """
        )
        self.conn.commit()

    def close(self) -> None:
        self.conn.close()

    def has_sha1(self, sha1: str) -> bool:
        cur = self.conn.execute("SELECT 1 FROM chunks WHERE sha1 = ? LIMIT 1;", (sha1,))
        return cur.fetchone() is not None

    def insert_chunk(self, doc: str, source_pdf: str, page_start: int, page_end: int,
                     section_title: Optional[str], text: str) -> ChunkRow:
        s = sha1_text(text)
        chunk_id = str(uuid.uuid4())
        row = ChunkRow(
            chunk_id=chunk_id,
            doc=doc,
            source_pdf=source_pdf,
            page_start=page_start,
            page_end=page_end,
            section_title=section_title,
            text=text,
            sha1=s,
        )
        self.conn.execute(
            """
            INSERT INTO chunks (chunk_id, doc, source_pdf, page_start, page_end, section_title, text, sha1)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?);
            """,
            (row.chunk_id, row.doc, row.source_pdf, row.page_start, row.page_end, row.section_title, row.text, row.sha1),
        )
        return row

    def insert_faiss_map(self, faiss_ids: List[int], chunk_ids: List[str]) -> None:
        self.conn.executemany(
            "INSERT INTO faiss_map (faiss_id, chunk_id) VALUES (?, ?);",
            list(zip(faiss_ids, chunk_ids)),
        )

    def commit(self) -> None:
        self.conn.commit()

    def get_chunks_by_faiss_ids(self, faiss_ids: List[int]) -> List[Dict[str, Any]]:
        if not faiss_ids:
            return []

        qmarks = ",".join("?" for _ in faiss_ids)
        cur = self.conn.execute(
            f"""
            SELECT m.faiss_id, c.doc, c.source_pdf, c.page_start, c.page_end, c.section_title, c.text
            FROM faiss_map m
            JOIN chunks c ON c.chunk_id = m.chunk_id
            WHERE m.faiss_id IN ({qmarks});
            """,
            faiss_ids,
        )
        rows = cur.fetchall()
        by_id = {
            int(r[0]): {
                "faiss_id": int(r[0]),
                "doc": r[1],
                "source_pdf": r[2],
                "page_start": int(r[3]),
                "page_end": int(r[4]),
                "section_title": r[5],
                "text": r[6],
            }
            for r in rows
        }
        return [by_id[i] for i in faiss_ids if i in by_id]
