from __future__ import annotations

import argparse
import json
import re
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Dict, Iterable, Iterator, List, Optional, Tuple


# -----------------------------
# Title / section detection heuristics
# -----------------------------

# e.g. "12.3.4", "3.2", "1.2.3.4"
_re_numbered_heading = re.compile(r"^\s*(\d+(?:\.\d+){1,6})\s+(.+?)\s*$")

# e.g. "CHAPTER 12", "Chapter 5", "SECTION 3"
_re_chapter_section = re.compile(r"^\s*(CHAPTER|Chapter|SECTION|Section|APPENDIX|Appendix)\s+([A-Z0-9]+)\b.*$")

# All caps-ish line: "FLEXCAN MODULE" or "REGISTER DESCRIPTION"
# Keep it conservative to avoid false positives on normal lines.
_re_all_caps = re.compile(r"^[A-Z0-9][A-Z0-9 \-_/().,:]{8,}$")

# Figure / Table labels often signal boundaries
_re_figure_table = re.compile(r"^\s*(Figure|FIGURE|Table|TABLE)\s+\d+([.-]\d+)?\b.*$")

# Register style: "CTRL1 Register (CAN_CTRL1)"
_re_register_line = re.compile(r"^\s*([A-Z0-9_]+)\s+Register\b.*$|^\s*Register\s+\d+\b.*$", re.IGNORECASE)


def is_heading_line(line: str) -> bool:
    s = line.strip()
    if not s:
        return False
    if _re_numbered_heading.match(s):
        return True
    if _re_chapter_section.match(s):
        return True
    if _re_figure_table.match(s):
        return True
    # all caps headings
    if _re_all_caps.match(s) and not s.endswith("."):
        return True
    return False


def extract_heading_title(line: str) -> str:
    """Return a normalized heading title for metadata."""
    s = line.strip()
    m = _re_numbered_heading.match(s)
    if m:
        return f"{m.group(1)} {m.group(2).strip()}"
    m = _re_chapter_section.match(s)
    if m:
        return s
    # For figures/tables, keep label (often helpful in manual search)
    if _re_figure_table.match(s):
        return s
    if _re_all_caps.match(s):
        return s
    return s[:120]


# -----------------------------
# Chunk model
# -----------------------------

@dataclass
class PageRecord:
    doc: str
    source_pdf: str
    page: int
    text: str


@dataclass
class Chunk:
    doc: str
    source_pdf: str
    page_start: int
    page_end: int
    section_title: Optional[str]
    text: str

    def to_json(self) -> Dict[str, Any]:
        return {
            "doc": self.doc,
            "source_pdf": self.source_pdf,
            "page_start": self.page_start,
            "page_end": self.page_end,
            "section_title": self.section_title,
            "text": self.text,
            "char_len": len(self.text),
        }


# -----------------------------
# IO
# -----------------------------

def iter_pages(jsonl_path: Path) -> Iterator[PageRecord]:
    with jsonl_path.open("r", encoding="utf-8") as f:
        for line in f:
            if not line.strip():
                continue
            obj = json.loads(line)
            yield PageRecord(
                doc=obj.get("doc") or Path(obj.get("source_pdf", "")).stem or "doc",
                source_pdf=obj["source_pdf"],
                page=int(obj["page"]),
                text=obj.get("text", "") or "",
            )


# -----------------------------
# Paragraph splitting
# -----------------------------

def split_into_blocks(page_text: str) -> List[str]:
    """
    Split page text into blocks by blank lines, while preserving headings.
    """
    # Normalize newlines
    t = page_text.replace("\r\n", "\n").replace("\r", "\n").strip()
    if not t:
        return []

    raw_blocks = re.split(r"\n\s*\n+", t)
    blocks: List[str] = []
    for b in raw_blocks:
        b = b.strip()
        if not b:
            continue
        blocks.append(b)
    return blocks


def block_starts_with_heading(block: str) -> Optional[str]:
    """
    If a block's first line is a heading, return that heading title.
    """
    first_line = block.splitlines()[0].strip()
    if is_heading_line(first_line):
        return extract_heading_title(first_line)
    return None


# -----------------------------
# Chunking logic
# -----------------------------

def chunk_pages(
    pages: Iterable[PageRecord],
    *,
    target_chars: int = 2800,
    max_chars: int = 3600,
    min_chars: int = 900,
    overlap_chars: int = 200,
) -> List[Chunk]:
    """
    Create chunks by aggregating paragraph-like blocks across pages.
    Prefer splitting at headings.
    """

    chunks: List[Chunk] = []

    cur_doc: Optional[str] = None
    cur_pdf: Optional[str] = None
    cur_section: Optional[str] = None
    cur_text_parts: List[str] = []
    cur_page_start: Optional[int] = None
    cur_page_end: Optional[int] = None

    def flush(force: bool = False):
        nonlocal cur_text_parts, cur_page_start, cur_page_end, cur_section
        if not cur_text_parts or cur_page_start is None or cur_page_end is None:
            return
        text = "\n\n".join(cur_text_parts).strip()
        if not text:
            return

        # If too small and not forced, keep accumulating
        if (len(text) < min_chars) and (not force):
            return

        chunks.append(
            Chunk(
                doc=cur_doc or "doc",
                source_pdf=cur_pdf or "",
                page_start=cur_page_start,
                page_end=cur_page_end,
                section_title=cur_section,
                text=text,
            )
        )

        # overlap: keep tail
        if overlap_chars > 0 and len(text) > overlap_chars:
            tail = text[-overlap_chars:]
            # Start next chunk with the tail as context
            cur_text_parts = [tail]
            # Note: page range continues; we keep page_end as start of overlap logically
            # For traceability, keep page_start as current end page.
            cur_page_start = cur_page_end
        else:
            cur_text_parts = []
            cur_page_start = None

    for pr in pages:
        cur_doc = pr.doc
        cur_pdf = pr.source_pdf

        if not pr.text.strip():
            continue

        blocks = split_into_blocks(pr.text)

        for block in blocks:
            heading = block_starts_with_heading(block)

            # If encountering a heading, prefer flushing current chunk first
            if heading is not None:
                # Flush even if small: headings are good natural boundaries
                flush(force=True)
                cur_section = heading

            # Init page range
            if cur_page_start is None:
                cur_page_start = pr.page
            cur_page_end = pr.page

            # If adding this block will exceed max_chars, flush first
            prospective_len = len("\n\n".join(cur_text_parts)) + (2 if cur_text_parts else 0) + len(block)
            if prospective_len > max_chars:
                flush(force=True)

                # After flush, re-init page range if needed
                if cur_page_start is None:
                    cur_page_start = pr.page
                cur_page_end = pr.page

            cur_text_parts.append(block)

            # If we've reached target size, flush (not forced)
            current_len = len("\n\n".join(cur_text_parts))
            if current_len >= target_chars:
                flush(force=False)

    # Flush remaining
    flush(force=True)
    return chunks


# -----------------------------
# CLI
# -----------------------------

def write_chunks_jsonl(chunks: List[Chunk], out_path: Path) -> None:
    out_path.parent.mkdir(parents=True, exist_ok=True)
    with out_path.open("w", encoding="utf-8") as f:
        for ch in chunks:
            f.write(json.dumps(ch.to_json(), ensure_ascii=False) + "\n")


def main():
    ap = argparse.ArgumentParser(description="Chunk page-level JSONL into chunk-level JSONL for RAG.")
    ap.add_argument("--in-pages", required=True, help="Input pages JSONL (from Step1)")
    ap.add_argument("--out-chunks", required=True, help="Output chunks JSONL")
    ap.add_argument("--target-chars", type=int, default=2800)
    ap.add_argument("--max-chars", type=int, default=3600)
    ap.add_argument("--min-chars", type=int, default=900)
    ap.add_argument("--overlap-chars", type=int, default=200)
    args = ap.parse_args()

    pages = list(iter_pages(Path(args.in_pages)))
    chunks = chunk_pages(
        pages,
        target_chars=args.target_chars,
        max_chars=args.max_chars,
        min_chars=args.min_chars,
        overlap_chars=args.overlap_chars,
    )
    write_chunks_jsonl(chunks, Path(args.out_chunks))

    stats = {
        "in_pages": len(pages),
        "out_chunks": len(chunks),
        "avg_chars": int(sum(len(c.text) for c in chunks) / max(1, len(chunks))),
        "min_chars": min((len(c.text) for c in chunks), default=0),
        "max_chars": max((len(c.text) for c in chunks), default=0),
        "out_path": str(Path(args.out_chunks).resolve()),
    }
    print(json.dumps(stats, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()

