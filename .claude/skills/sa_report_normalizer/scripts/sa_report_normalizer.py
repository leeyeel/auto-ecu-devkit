#!/usr/bin/env python3
"""
sa_report_normalizer.py

Normalize static analysis output into a single JSON schema for ECU CI pipelines.

Supported (best-effort):
- QAC / QAC++ style: file(line): severity rule: message
- PC-lint / FlexeLint: file line col severity rule: message
- Coverity: several common text formats including "CID <id>" lines
- clang-tidy: file:line:col: <severity>: <message> [check]
- cppcheck: file:line:col: <severity>: <message> [id]
- GCC/Clang warnings/errors: file:line:col: warning|error: message

Usage:
  cat report.log | python3 sa_report_normalizer.py > findings.json
  python3 sa_report_normalizer.py --input report.log --output findings.json --csv findings.csv
"""

from __future__ import annotations

import argparse
import dataclasses
import datetime as _dt
import json
import re
import sys
from collections import Counter 
from typing import Dict, Iterable, List, Optional, Tuple

SCHEMA_VERSION = "sa_normalized_v1"

@dataclasses.dataclass
class Finding:
    tool: str
    tool_rule: Optional[str]
    misra_rule: Optional[str]
    severity: str
    file: Optional[str]
    line: Optional[int]
    column: Optional[int]
    function: Optional[str]
    message: str
    category: str
    raw: str

# --- Severity normalization -------------------------------------------------
_SEV_MAP = {
    "fatal": "error",
    "error": "error",
    "err": "error",
    "warning": "warning",
    "warn": "warning",
    "w": "warning",
    "note": "note",
    "info": "info",
    "information": "info",
    "advice": "warning",
    "advisory": "warning",
}

def normalize_severity(sev: Optional[str]) -> str:
    if not sev:
        return "info"
    s = sev.strip().lower()
    if s in ("high", "medium", "low"):
        return "warning"
    return _SEV_MAP.get(s, "info")

# --- MISRA rule extraction --------------------------------------------------

# Matches patterns like:
# - MISRA C:2012 Rule 10.1
# - MISRA2012-10.1
# - MISRA-C:2012 10.1
_MISRA_RE = re.compile(
    r"(?:(?:MISRA)\s*(?:C)?\s*[:\-]?\s*(?:2012)?\s*(?:Rule)?\s*)"
    r"(?P<rule>\d{1,2}\.\d{1,2})",
    re.IGNORECASE,
)

_MISRA_DASH_RE = re.compile(r"\bMISRA(?:2012)?[-_](?P<rule>\d{1,2}\.\d{1,2})\b", re.IGNORECASE)

def extract_misra_rule(text: str) -> Optional[str]:
    m = _MISRA_DASH_RE.search(text)
    if m:
        return m.group("rule")
    m = _MISRA_RE.search(text)
    if m:
        return m.group("rule")
    return None

# --- Categories -------------------------------------------------------------
def guess_category(tool: str, tool_rule: Optional[str], message: str) -> str:
    # Keep categories coarse; downstream can re-classify.
    msg = message.lower()
    rule = (tool_rule or "").lower()

    if "misra" in msg or "misra" in rule:
        return "misra"
    if any(k in msg for k in ("overflow", "out of bounds", "null deref", "use after free", "buffer")):
        return "bug"
    if any(k in msg for k in ("race", "deadlock", "atomic")):
        return "concurrency"
    if any(k in msg for k in ("security", "taint", "injection", "cwe-")):
        return "security"
    if tool == "compiler":
        return "build"
    if any(k in rule for k in ("performance", "readability", "modernize", "clang-analyzer")):
        return "style" if "readability" in rule or "modernize" in rule else "performance"
    if any(k in msg for k in ("portability", "endianness", "alignment")):
        return "portability"
    return "general"

# --- Parsers ----------------------------------------------------------------
# QAC: path/file.c(123): Warning 1234: message
QAC_RE = re.compile(
    r"^(?P<file>.+?)\((?P<line>\d+)\)\s*:\s*(?P<severity>[A-Za-z]+)\s+(?P<rule>[^:]+?)\s*:\s*(?P<msg>.+)$"
)

# PC-lint / FlexeLint (common): file.c 123 7 Warning 567: message
LINT_RE = re.compile(
    r"^(?P<file>.+?)\s+(?P<line>\d+)\s+(?P<col>\d+)\s+(?P<severity>[A-Za-z]+)\s+(?P<rule>\d+)\s*:\s*(?P<msg>.+)$"
)

# clang-tidy: file:line:col: warning: message [check-name]
CLANG_TIDY_RE = re.compile(
    r"^(?P<file>.+?):(?P<line>\d+):(?P<col>\d+):\s*(?P<severity>warning|error|note):\s*(?P<msg>.+?)\s*\[(?P<rule>[^\]]+)\]\s*$",
    re.IGNORECASE,
)

# cppcheck: file:line:col: (severity): message [id]
CPPCHECK_RE = CLANG_TIDY_RE  # same surface format usually

# GCC/Clang compiler: file:line:col: warning: message
COMPILER_RE = re.compile(
    r"^(?P<file>.+?):(?P<line>\d+):(?P<col>\d+):\s*(?P<severity>warning|error|note):\s*(?P<msg>.+)$",
    re.IGNORECASE,
)

# Coverity common patterns (best-effort):
# - "CID 12345: <type> (<desc>)"
# - "path/file.c:123: <desc> (CID 12345)"
COVERITY_CID_INLINE_RE = re.compile(r"\(CID\s+(?P<cid>\d+)\)", re.IGNORECASE)
COVERITY_CID_PREFIX_RE = re.compile(r"^CID\s+(?P<cid>\d+)\s*:\s*(?P<msg>.+)$", re.IGNORECASE)
COVERITY_FILELINE_RE = re.compile(r"^(?P<file>.+?):(?P<line>\d+):\s*(?P<msg>.+?)\s*\(CID\s+(?P<cid>\d+)\)\s*$", re.IGNORECASE)

def _mk(tool: str, raw: str, *,
        file: Optional[str] = None,
        line: Optional[int] = None,
        col: Optional[int] = None,
        severity: Optional[str] = None,
        tool_rule: Optional[str] = None,
        message: str = "",
        function: Optional[str] = None) -> Finding:
    sev = normalize_severity(severity)
    misra = extract_misra_rule(raw) or extract_misra_rule(message) or (extract_misra_rule(tool_rule or "") if tool_rule else None)
    cat = guess_category(tool, tool_rule, message)
    return Finding(
        tool=tool,
        tool_rule=tool_rule,
        misra_rule=misra,
        severity=sev,
        file=file,
        line=line,
        column=col,
        function=function,
        message=message.strip(),
        category=cat,
        raw=raw.rstrip("\n"),
    )

def parse_line(line: str, tool_hint: str = "auto") -> Optional[Finding]:
    s = line.rstrip("\n")
    if not s.strip():
        return None

    # If tool_hint is specified, try that parser first.
    parsers: List[Tuple[str, callable]] = []
    if tool_hint != "auto":
        parsers.append((tool_hint, None))  # placeholder, handled below

    # Ordered by specificity to reduce misclassification
    ordered = [
        ("qac", _parse_qac),
        ("lint", _parse_lint),
        ("clang-tidy", _parse_clang_tidy),
        ("cppcheck", _parse_cppcheck),
        ("coverity", _parse_coverity),
        ("compiler", _parse_compiler),
    ]

    if tool_hint != "auto":
        # Put hinted parser first, if it exists in ordered.
        hinted = [p for p in ordered if p[0] == tool_hint]
        others = [p for p in ordered if p[0] != tool_hint]
        ordered = hinted + others

    for tool, fn in ordered:
        f = fn(s)
        if f is not None:
            return f

    # Unknown line: keep as info so pipelines can inspect and improve parsers.
    return _mk("unknown", s, message=s, severity="info", tool_rule=None)


def _parse_qac(s: str) -> Optional[Finding]:
    m = QAC_RE.match(s)
    if not m:
        return None
    return _mk(
        "qac", s,
        file=m.group("file"),
        line=int(m.group("line")),
        severity=m.group("severity"),
        tool_rule=m.group("rule").strip(),
        message=m.group("msg"),
    )


def _parse_lint(s: str) -> Optional[Finding]:
    m = LINT_RE.match(s)
    if not m:
        return None
    return _mk(
        "lint", s,
        file=m.group("file"),
        line=int(m.group("line")),
        col=int(m.group("col")),
        severity=m.group("severity"),
        tool_rule=m.group("rule").strip(),
        message=m.group("msg"),
    )


def _parse_clang_tidy(s: str) -> Optional[Finding]:
    m = CLANG_TIDY_RE.match(s)
    if not m:
        return None
    return _mk(
        "clang-tidy", s,
        file=m.group("file"),
        line=int(m.group("line")),
        col=int(m.group("col")),
        severity=m.group("severity"),
        tool_rule=m.group("rule").strip(),
        message=m.group("msg"),
    )


def _parse_cppcheck(s: str) -> Optional[Finding]:
    # Distinguish from clang-tidy via rule id patterns commonly used by cppcheck
    m = CPPCHECK_RE.match(s)
    if not m:
        return None
    rule = m.group("rule").strip()
    # Heuristic: cppcheck ids often contain "misra" or "cppcheck" style ids
    if "cppcheck" in rule.lower() or "misra" in rule.lower() or rule.lower().startswith("style") or rule.lower().startswith("warning"):
        tool = "cppcheck"
    else:
        # could still be clang-tidy; let clang-tidy parser handle earlier in order
        return None
    return _mk(
        tool, s,
        file=m.group("file"),
        line=int(m.group("line")),
        col=int(m.group("col")),
        severity=m.group("severity"),
        tool_rule=rule,
        message=m.group("msg"),
    )


def _parse_compiler(s: str) -> Optional[Finding]:
    m = COMPILER_RE.match(s)
    if not m:
        return None
    return _mk(
        "compiler", s,
        file=m.group("file"),
        line=int(m.group("line")),
        col=int(m.group("col")),
        severity=m.group("severity"),
        tool_rule=None,
        message=m.group("msg"),
    )


def _parse_coverity(s: str) -> Optional[Finding]:
    m = COVERITY_FILELINE_RE.match(s)
    if m:
        cid = m.group("cid")
        return _mk(
            "coverity", s,
            file=m.group("file"),
            line=int(m.group("line")),
            severity="warning",
            tool_rule=f"CID {cid}",
            message=m.group("msg"),
        )

    m = COVERITY_CID_PREFIX_RE.match(s)
    if m:
        cid = m.group("cid")
        msg = m.group("msg")
        # Try to infer severity from msg keywords
        sev = "error" if "error" in msg.lower() else "warning"
        return _mk(
            "coverity", s,
            file=None,
            line=None,
            severity=sev,
            tool_rule=f"CID {cid}",
            message=msg,
        )

    # Inline CID somewhere in the line; treat as coverity if present.
    m = COVERITY_CID_INLINE_RE.search(s)
    if m:
        cid = m.group("cid")
        return _mk(
            "coverity", s,
            file=None,
            line=None,
            severity="warning",
            tool_rule=f"CID {cid}",
            message=s,
        )

    return None


# --- Normalization pipeline -------------------------------------------------

def parse_stream(lines: Iterable[str], tool_hint: str = "auto") -> List[Finding]:
    findings: List[Finding] = []
    for line in lines:
        f = parse_line(line, tool_hint=tool_hint)
        if f is not None:
            # Drop unknown blank lines
            findings.append(f)
    return findings

def compute_stats(findings: List[Finding]) -> Dict[str, object]:
    by_tool = Counter(f.tool for f in findings)
    by_sev = Counter(f.severity for f in findings)
    by_file = Counter(f.file or "<unknown>" for f in findings)
    by_rule = Counter((f.tool, f.tool_rule or "<none>") for f in findings)

    top_rules = []
    for (tool, rule), cnt in by_rule.most_common(20):
        top_rules.append({"tool": tool, "tool_rule": rule, "count": cnt})

    return {
        "total": len(findings),
        "by_tool": dict(by_tool),
        "by_severity": dict(by_sev),
        "by_file": dict(by_file),
        "top_rules": top_rules,
    }

def to_payload(findings: List[Finding], tool_hint: str, input_name: str) -> Dict[str, object]:
    now = _dt.datetime.utcnow().replace(microsecond=0).isoformat() + "Z"
    return {
        "schema_version": SCHEMA_VERSION,
        "generated_at_utc": now,
        "source": {"tool_hint": tool_hint, "input_name": input_name},
        "findings": [dataclasses.asdict(f) for f in findings],
        "stats": compute_stats(findings),
    }


def write_csv(findings: List[Finding], path: str) -> None:
    import csv
    with open(path, "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(["tool", "tool_rule", "misra_rule", "severity", "file", "line", "column", "function", "category", "message"])
        for x in findings:
            w.writerow([x.tool, x.tool_rule or "", x.misra_rule or "", x.severity, x.file or "", x.line or "", x.column or "", x.function or "", x.category, x.message])

def main() -> int:
    ap = argparse.ArgumentParser(description="Normalize static analysis reports into a common JSON format.")
    ap.add_argument("--input", "-i", default="-", help="Input file (default: stdin)")
    ap.add_argument("--output", "-o", default="-", help="Output JSON file (default: stdout)")
    ap.add_argument("--csv", default=None, help="Optional CSV output path")
    ap.add_argument("--tool-hint", default="auto", choices=["auto", "qac", "lint", "coverity", "clang-tidy", "cppcheck", "compiler"], help="Prefer a specific parser first")
    args = ap.parse_args()

    if args.input == "-":
        text = sys.stdin.read().splitlines(True)
        input_name = "stdin"
    else:
        with open(args.input, "r", encoding="utf-8", errors="replace") as f:
            text = f.readlines()
        input_name = args.input

    findings = parse_stream(text, tool_hint=args.tool_hint)
    payload = to_payload(findings, tool_hint=args.tool_hint, input_name=input_name)

    out_json = json.dumps(payload, ensure_ascii=False, indent=2)
    if args.output == "-":
        sys.stdout.write(out_json + "\n")
    else:
        with open(args.output, "w", encoding="utf-8") as f:
            f.write(out_json + "\n")

    if args.csv:
        write_csv(findings, args.csv)

    # Also print a brief summary to stderr for CI logs.
    st = payload["stats"]
    sys.stderr.write(f"[sa_report_normalizer] total={st['total']} tools={st['by_tool']} severity={st['by_severity']}\n")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
