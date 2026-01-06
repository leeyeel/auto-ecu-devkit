#!/usr/bin/env python3
"""
misra_pdf_query.py

Query a local MISRA PDF index (JSONL) by rule id or free text.

Index JSONL example:
{"kind":"rule","id":"10.1","title":"...","section":"...","page_start":123,"page_end":124,"text":"...","source_pdf":"MISRA_C_2012.pdf"}

Usage:
  python3 scripts/misra_pdf_query.py --rule 10.1 --index data/misra_pdf_index.jsonl
  python3 scripts/misra_pdf_query.py --query "side effects expression" --index data/misra_pdf_index.jsonl
"""

from __future__ import annotations
import argparse, json, re, sys
from typing import Dict, List

def load_jsonl(path: str) -> List[Dict]:
    rows = []
    with open(path, "r", encoding="utf-8", errors="replace") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            rows.append(json.loads(line))
    return rows

def score_query(text: str, q: str) -> int:
    t = text.lower()
    tokens = [x for x in re.split(r"\\W+", q.lower()) if x]
    return sum(1 for tok in tokens if tok in t)

def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--index", required=True)
    ap.add_argument("--rule", default=None)
    ap.add_argument("--query", default=None)
    ap.add_argument("--topk", type=int, default=5)
    ap.add_argument("--max-excerpt-chars", type=int, default=600)
    args = ap.parse_args()

    rows = load_jsonl(args.index)

    if args.rule:
        rid = args.rule.strip()
        hits = [r for r in rows if str(r.get("id")) == rid]
        if not hits:
            sys.stderr.write(f"No hits for rule {rid}\\n")
            return 1
        for r in hits[:args.topk]:
            text = (r.get("text") or "")
            excerpt = text[:args.max_excerpt_chars]
            out = {k: r.get(k) for k in ("id","kind","title","section","page_start","page_end","source_pdf")}
            out["excerpt"] = excerpt
            sys.stdout.write(json.dumps(out, ensure_ascii=False, indent=2) + "\\n")
        return 0

    if args.query:
        q = args.query.strip()
        scored = []
        for r in rows:
            hay = f"{r.get('id','')} {r.get('title','')} {r.get('text','')}"
            s = score_query(hay, q)
            if s > 0:
                scored.append((s, r))
        scored.sort(key=lambda x: (-x[0], str(x[1].get("id",""))))
        for s, r in scored[:args.topk]:
            text = (r.get("text") or "")
            excerpt = text[:args.max_excerpt_chars]
            out = {k: r.get(k) for k in ("id","kind","title","section","page_start","page_end","source_pdf")}
            out["score"] = s
            out["excerpt"] = excerpt
            sys.stdout.write(json.dumps(out, ensure_ascii=False, indent=2) + "\\n")
        return 0

    ap.error("Provide --rule or --query")
    return 2

if __name__ == "__main__":
    raise SystemExit(main())
