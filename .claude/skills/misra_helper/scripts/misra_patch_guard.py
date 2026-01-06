#!/usr/bin/env python3
"""
misra_patch_guard.py

Fail fast if a patch introduces forbidden APIs or patterns.

Usage:
  git diff | python3 scripts/misra_patch_guard.py
"""

from __future__ import annotations
import re
import sys
from typing import List
import yaml  # type: ignore

DEFAULT_BANNED_FUNCS = [
  "malloc","free","calloc","realloc",
  "printf","sprintf","vsprintf","scanf",
  "strcpy","strcat","gets",
]
DEFAULT_BANNED_KEYWORDS = ["goto"]

def load_bans(path: str):
    try:
        with open(path, "r", encoding="utf-8") as f:
            y = yaml.safe_load(f) or {}
        return list(y.get("banned_functions", []) or DEFAULT_BANNED_FUNCS), list(y.get("banned_keywords", []) or DEFAULT_BANNED_KEYWORDS)
    except Exception:
        return DEFAULT_BANNED_FUNCS, DEFAULT_BANNED_KEYWORDS

def main() -> int:
    banned_funcs, banned_keywords = load_bans("data/banned_apis.yml")
    diff = sys.stdin.read()

    added = []
    for line in diff.splitlines():
        if line.startswith("+++"):
            continue
        if line.startswith("+") and not line.startswith("++"):
            added.append(line[1:])
    text = "\\n".join(added)

    hits: List[str] = []
    for fn in banned_funcs:
        if re.search(rf"\\b{re.escape(fn)}\\s*\\(", text):
            hits.append(f"banned_function:{fn}")
    for kw in banned_keywords:
        if re.search(rf"\\b{re.escape(kw)}\\b", text):
            hits.append(f"banned_keyword:{kw}")

    if hits:
        sys.stderr.write("misra_patch_guard: forbidden patterns introduced:\\n")
        for h in sorted(set(hits)):
            sys.stderr.write(f"  - {h}\\n")
        return 2

    sys.stderr.write("misra_patch_guard: OK\\n")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
