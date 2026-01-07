from __future__ import annotations
import argparse
import json
import re
from collections import Counter
from dataclasses import dataclass
from pathlib import Path
from typing import List, Tuple, Dict, Any, Optional
import pdfplumber

_whitespace_re = re.compile(r"[ \t]+")
_multi_newline_re = re.compile(r"\n{3,}")
_hyphen_linebreak_re = re.compile(r"(\w)-\n(\w)") 

def normalize_text(text: str) -> str:
    """Normalize extracted text to be more embedding-/search-friendly."""
    if not text:
        return ""

    # Normalize Windows line endings
    text = text.replace("\r\n", "\n").replace("\r", "\n")

    # Fix hyphen line breaks: regis-\nter -> register
    text = _hyphen_linebreak_re.sub(r"\1\2", text)

    # Remove trailing spaces on each line
    text = "\n".join(line.rstrip() for line in text.splitlines())

    # Collapse multiple spaces/tabs
    text = _whitespace_re.sub(" ", text)

    # Collapse excessive blank lines
    text = _multi_newline_re.sub("\n\n", text)

    return text.strip()


def merge_wrapped_lines(lines: List[str]) -> str:
    """
    Merge lines that are likely "soft-wrapped" by PDF extraction.
    Keeps paragraph breaks on empty lines.
    Heuristics:
      - If a line doesn't end with punctuation and next line starts with lowercase, merge.
      - If a line ends with comma/colon/semicolon and next continues, merge.
    """
    merged: List[str] = []
    i = 0

    def is_para_break(s: str) -> bool:
        return len(s.strip()) == 0

    while i < len(lines):
        line = lines[i].rstrip()
        if is_para_break(line):
            merged.append("")
            i += 1
            continue

        cur = line
        while i + 1 < len(lines):
            nxt = lines[i + 1].rstrip()

            if is_para_break(nxt):
                break

            cur_strip = cur.rstrip()
            nxt_strip = nxt.lstrip()

            # If current ends with hyphen, merge directly (already handled in normalize_text too)
            if cur_strip.endswith("-"):
                cur = cur_strip[:-1] + nxt_strip
                i += 1
                continue

            # Heuristic for soft wrap
            end_punct = cur_strip[-1:] in {".", "!", "?", ")", "]", "}", ":", ";", ","}
            nxt_starts_lower = nxt_strip[:1].islower()

            if (not end_punct and nxt_starts_lower) or (cur_strip[-1:] in {",", ":", ";"}):
                cur = cur_strip + " " + nxt_strip
                i += 1
            else:
                break

        merged.append(cur)
        i += 1

    # Re-join; preserve paragraph breaks
    return "\n".join(merged)


# -----------------------------
# Header/footer detection
# -----------------------------

@dataclass
class HeaderFooterRules:
    header_lines: List[str]
    footer_lines: List[str]


def _top_bottom_lines(page_text: str, top_n: int, bottom_n: int) -> Tuple[List[str], List[str]]:
    lines = [ln.strip() for ln in page_text.splitlines()]
    lines = [ln for ln in lines if ln]  # ignore empty lines for header/footer counting
    if not lines:
        return [], []
    top = lines[:top_n]
    bottom = lines[-bottom_n:] if bottom_n > 0 else []
    return top, bottom


def build_header_footer_rules(
    page_texts: List[str],
    top_n: int = 3,
    bottom_n: int = 3,
    min_ratio: float = 0.35,
) -> HeaderFooterRules:
    """
    Identify repeated header/footer lines across pages.
    A line is considered header/footer if it appears in >= min_ratio of pages.
    """
    header_counter: Counter[str] = Counter()
    footer_counter: Counter[str] = Counter()

    total_pages = max(1, len(page_texts))

    for txt in page_texts:
        top, bottom = _top_bottom_lines(txt, top_n=top_n, bottom_n=bottom_n)
        header_counter.update(top)
        footer_counter.update(bottom)

    threshold = max(2, int(total_pages * min_ratio))

    header_lines = [line for line, cnt in header_counter.items() if cnt >= threshold and len(line) >= 3]
    footer_lines = [line for line, cnt in footer_counter.items() if cnt >= threshold and len(line) >= 3]

    # Sort by frequency desc for determinism
    header_lines.sort(key=lambda x: (-header_counter[x], x))
    footer_lines.sort(key=lambda x: (-footer_counter[x], x))

    return HeaderFooterRules(header_lines=header_lines, footer_lines=footer_lines)


def strip_header_footer(page_text: str, rules: HeaderFooterRules) -> Tuple[str, Dict[str, List[str]]]:
    """
    Remove detected header/footer lines from a page.
    Returns cleaned text + dict of removed lines for debugging.
    """
    removed = {"header": [], "footer": []}
    lines = [ln.rstrip() for ln in page_text.splitlines()]

    # Remove exact matches (striped)
    def is_header(line: str) -> bool:
        s = line.strip()
        return s in rules.header_lines

    def is_footer(line: str) -> bool:
        s = line.strip()
        return s in rules.footer_lines

    cleaned: List[str] = []
    for ln in lines:
        if is_header(ln):
            removed["header"].append(ln.strip())
            continue
        if is_footer(ln):
            removed["footer"].append(ln.strip())
            continue
        cleaned.append(ln)

    return "\n".join(cleaned), removed


# -----------------------------
# Extraction core
# -----------------------------

def extract_pages_text(pdf_path: Path, start_page: int = 1, end_page: Optional[int] = None) -> List[str]:
    """
    Extract raw text for each page using pdfplumber.
    Pages are 1-indexed in parameters.
    """
    texts: List[str] = []
    with pdfplumber.open(str(pdf_path)) as pdf:
        total = len(pdf.pages)
        sp = max(1, start_page)
        ep = min(total, end_page) if end_page else total

        for idx in range(sp - 1, ep):
            page = pdf.pages[idx]
            # layout=True keeps line breaks more stable on complex PDFs
            raw = page.extract_text(layout=True) or ""
            texts.append(raw)

    return texts


def pdf_to_jsonl(
    pdf_path: Path,
    out_jsonl: Path,
    *,
    doc_name: Optional[str] = None,
    start_page: int = 1,
    end_page: Optional[int] = None,
    top_n: int = 3,
    bottom_n: int = 3,
    min_ratio: float = 0.35,
) -> Dict[str, Any]:
    """
    Extract PDF into page-level JSONL with page number + cleaned text.
    Returns a small stats dict.
    """
    pdf_path = pdf_path.resolve()
    out_jsonl = out_jsonl.resolve()
    out_jsonl.parent.mkdir(parents=True, exist_ok=True)

    doc = doc_name or pdf_path.stem

    raw_texts = extract_pages_text(pdf_path, start_page=start_page, end_page=end_page)
    rules = build_header_footer_rules(raw_texts, top_n=top_n, bottom_n=bottom_n, min_ratio=min_ratio)

    pages_written = 0
    empty_pages = 0

    with out_jsonl.open("w", encoding="utf-8") as f:
        for i, raw in enumerate(raw_texts, start=start_page):
            stripped, removed = strip_header_footer(raw, rules)

            # Clean + merge wrapped lines
            lines = stripped.splitlines()
            merged = merge_wrapped_lines(lines)
            cleaned = normalize_text(merged)

            if not cleaned:
                empty_pages += 1

            rec = {
                "doc": doc,
                "source_pdf": str(pdf_path),
                "page": i,               # 1-indexed page number
                "text": cleaned,
                "removed": removed,      # for debugging header/footer
            }
            f.write(json.dumps(rec, ensure_ascii=False) + "\n")
            pages_written += 1

    return {
        "doc": doc,
        "pdf": str(pdf_path),
        "out": str(out_jsonl),
        "pages": pages_written,
        "empty_pages": empty_pages,
        "header_lines": rules.header_lines,
        "footer_lines": rules.footer_lines,
        "start_page": start_page,
        "end_page": end_page or (start_page + pages_written - 1),
    }


# -----------------------------
# CLI
# -----------------------------

def main():
    ap = argparse.ArgumentParser(description="Extract a PDF manual into page-level JSONL for RAG.")
    ap.add_argument("--pdf", required=True, help="Path to PDF")
    ap.add_argument("--out", required=True, help="Output JSONL path")
    ap.add_argument("--doc-name", default=None, help="Document name override")
    ap.add_argument("--start-page", type=int, default=1, help="1-indexed start page")
    ap.add_argument("--end-page", type=int, default=None, help="1-indexed end page (inclusive)")
    ap.add_argument("--top-n", type=int, default=3, help="How many top lines to consider as header candidates")
    ap.add_argument("--bottom-n", type=int, default=3, help="How many bottom lines to consider as footer candidates")
    ap.add_argument("--min-ratio", type=float, default=0.35, help="Line must appear on this ratio of pages to be removed")
    args = ap.parse_args()

    stats = pdf_to_jsonl(
        Path(args.pdf),
        Path(args.out),
        doc_name=args.doc_name,
        start_page=args.start_page,
        end_page=args.end_page,
        top_n=args.top_n,
        bottom_n=args.bottom_n,
        min_ratio=args.min_ratio,
    )
    print(json.dumps(stats, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()

