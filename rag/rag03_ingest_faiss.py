from __future__ import annotations
import argparse
import json
from pathlib import Path
from typing import Any, Dict, List, Optional
from .rag03_embeddings import OpenAIEmbeddingProvider, EmbeddingProvider
from .rag03_index_faiss import FaissIndex
from .rag03_meta_sqlite import MetaStore, sha1_text

def iter_chunks_jsonl(path: Path):
    with path.open("r", encoding="utf-8") as f:
        for line in f:
            if not line.strip():
                continue
            obj = json.loads(line)
            yield obj

def pick_embedder(kind: str, model: str) -> EmbeddingProvider:
    kind = kind.lower()
    if kind == "openai":
        return OpenAIEmbeddingProvider(model=model)
    raise ValueError(f"Unknown embedder kind: {kind}")

def ingest(
    chunks_jsonl: Path,
    index_dir: Path,
    *,
    embedder_kind: str = "openai",
    embedder_model: str = "text-embedding-3-large",
    batch_size: int = 64,
) -> Dict[str, Any]:
    index_dir.mkdir(parents=True, exist_ok=True)
    faiss_path = index_dir / "faiss.index"
    sqlite_path = index_dir / "meta.sqlite"

    meta = MetaStore(sqlite_path)

    # Load / init FAISS
    faiss_index: Optional[FaissIndex] = None
    if faiss_path.exists():
        faiss_index = FaissIndex.load(faiss_path)

    embedder = pick_embedder(embedder_kind, embedder_model)

    # Collect new chunks
    to_add: List[Dict[str, Any]] = []
    skipped = 0
    total = 0
    for ch in iter_chunks_jsonl(chunks_jsonl):
        total += 1
        text = (ch.get("text") or "").strip()
        if not text:
            continue
        s = sha1_text(text)
        if meta.has_sha1(s):
            skipped += 1
            continue
        to_add.append(ch)

    if not to_add:
        meta.close()
        return {
            "status": "ok",
            "message": "No new chunks to ingest (all existing).",
            "total_chunks_in_file": total,
            "skipped_existing": skipped,
            "added": 0,
            "index_dir": str(index_dir.resolve()),
        }

    # Embed in batches
    added = 0
    inserted_rows = 0

    # Create FAISS if not exists (need dim)
    def ensure_faiss(dim: int):
        nonlocal faiss_index
        if faiss_index is None:
            faiss_index = FaissIndex(dim)

    chunk_ids: List[str] = []
    faiss_ids: List[int] = []

    for i in range(0, len(to_add), batch_size):
        batch = to_add[i : i + batch_size]
        texts = [(b.get("text") or "") for b in batch]
        vecs = embedder.embed_texts(texts)
        if vecs.ndim != 2 or vecs.shape[0] != len(texts):
            raise RuntimeError(f"Bad embeddings shape: {vecs.shape}")

        ensure_faiss(vecs.shape[1])

        # Insert chunks into sqlite first
        new_rows = []
        for b in batch:
            row = meta.insert_chunk(
                doc=b.get("doc") or "doc",
                source_pdf=b.get("source_pdf") or "",
                page_start=int(b.get("page_start") or 0),
                page_end=int(b.get("page_end") or 0),
                section_title=b.get("section_title"),
                text=(b.get("text") or ""),
            )
            new_rows.append(row)

        # Add vectors to FAISS
        ids = faiss_index.add(vecs)
        # Map
        meta.insert_faiss_map(ids, [r.chunk_id for r in new_rows])

        inserted_rows += len(new_rows)
        added += len(ids)

        meta.commit()

    # Save index
    faiss_index.save(faiss_path)
    meta.close()

    return {
        "status": "ok",
        "total_chunks_in_file": total,
        "skipped_existing": skipped,
        "added": added,
        "inserted_rows": inserted_rows,
        "faiss_path": str(faiss_path.resolve()),
        "sqlite_path": str(sqlite_path.resolve()),
        "index_dir": str(index_dir.resolve()),
        "embedder_kind": embedder_kind,
        "embedder_model": embedder_model,
    }

def main():
    ap = argparse.ArgumentParser(description="Step3: ingest chunk JSONL into FAISS+SQLite index.")
    ap.add_argument("--chunks", required=True, help="Chunk JSONL from Step2")
    ap.add_argument("--index-dir", required=True, help="Directory to store faiss.index + meta.sqlite")
    ap.add_argument("--embedder", default="openai", choices=["openai", "local", "sentence-transformers", "st"])
    ap.add_argument("--model", default="text-embedding-3-large", help="Embedding model name")
    ap.add_argument("--batch-size", type=int, default=64)
    args = ap.parse_args()

    stats = ingest(
        chunks_jsonl=Path(args.chunks),
        index_dir=Path(args.index_dir),
        embedder_kind=args.embedder,
        embedder_model=args.model,
        batch_size=args.batch_size,
    )
    print(json.dumps(stats, ensure_ascii=False, indent=2))

if __name__ == "__main__":
    main()
