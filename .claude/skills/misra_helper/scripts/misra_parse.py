#!/usr/bin/env python3
"""
misra_parse.py

Parse static analysis output and classify into MISRA-oriented buckets.
Outputs normalized JSON schema: misra_helper_v1.

Supported (best-effort):
- QAC/QAC++: file(line): Severity rule: message
- PC-lint/FlexeLint: file line col Severity rule: message
- clang-tidy / cppcheck / compiler: file:line:col: severity: message [rule]
- Coverity: file:line: msg (CID 12345) OR "CID 12345: msg"

Requires: pyyaml (for rule_map.yml)

Usage:
  cat report.log | python3 scripts/misra_parse.py > findings.json
  python3 scripts/misra_parse.py --input report.log --output findings.json --rule-map data/rule_map.yml
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

import yaml  # type: ignore

SCHEMA_VERSION = "misra_helper_v1"

@dataclasses.dataclass
class Finding:
    tool: str
    tool_rule: Optional[str]
    misra_rule: Optional[str]
    severity: str
    file: Optional[str]
    line: Optional[int]
    column: Optional[int]
    message: str
    bucket: str
    fix_patterns: List[str]
    raw: str

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
    "required": "warning",
    "mandatory": "error",
}

def normalize_severity(sev: Optional[str]) -> str:
    if not sev:
        return "info"
    s = sev.strip().lower()
    if s in ("high", "medium", "low"):
        return "warning"
    return _SEV_MAP.get(s, "info")

_MISRA_RULE_RE = re.compile(
    r"(?:(?:MISRA)\s*(?:C)?\s*[:\-]?\s*(?:2012)?\s*(?:Rule|Directive)?\s*)"
    r"(?P<rule>\d{1,2}\.\d{1,2})",
    re.IGNORECASE,
)
_MISRA_DASH_RE = re.compile(r"\bMISRA(?:2012)?[-_](?P<rule>\d{1,2}\.\d{1,2})\b", re.IGNORECASE)
_MISRA_DIR_RE = re.compile(r"\bDirective\s+(?P<dir>\d{1,2}\.\d{1,2})\b", re.IGNORECASE)

def extract_misra_rule(text: str) -> Optional[str]:
    m = _MISRA_DIR_RE.search(text)
    if m:
        return f"D{m.group('dir')}"
    m = _MISRA_DASH_RE.search(text)
    if m:
        return m.group("rule")
    m = _MISRA_RULE_RE.search(text)
    if m:
        return m.group("rule")
    return None

def load_rule_map(path: str) -> Dict[str, object]:
    with open(path, "r", encoding="utf-8") as f:
        return yaml.safe_load(f) or {}

def classify(rule_map: Dict[str, object], text: str) -> Tuple[str, List[str]]:
    buckets = rule_map.get("buckets", {}) or {}
    default_bucket = rule_map.get("default_bucket", "general")
    hay = text.lower()
    for bucket, spec in buckets.items():
        kws = [k.lower() for k in (spec.get("keywords", []) or [])]
        if any(k in hay for k in kws):
            return bucket, list(spec.get("fix_patterns", []) or [])
    return default_bucket, []

# Parsers
QAC_RE = re.compile(r"^(?P<file>.+?)\((?P<line>\d+)\)\s*:\s*(?P<severity>[A-Za-z]+)\s+(?P<rule>[^:]+?)\s*:\s*(?P<msg>.+)$")
LINT_RE = re.compile(r"^(?P<file>.+?)\s+(?P<line>\d+)\s+(?P<col>\d+)\s+(?P<severity>[A-Za-z]+)\s+(?P<rule>\d+)\s*:\s*(?P<msg>.+)$")
CLANGSTYLE_RE = re.compile(r"^(?P<file>.+?):(?P<line>\d+):(?P<col>\d+):\s*(?P<severity>warning|error|note):\s*(?P<msg>.+?)\s*(?:\[(?P<rule>[^\]]+)\])?\s*$", re.IGNORECASE)
COVERITY_FILELINE_RE = re.compile(r"^(?P<file>.+?):(?P<line>\d+):\s*(?P<msg>.+?)\s*\(CID\s+(?P<cid>\d+)\)\s*$", re.IGNORECASE)
COVERITY_CID_PREFIX_RE = re.compile(r"^CID\s+(?P<cid>\d+)\s*:\s*(?P<msg>.+)$", re.IGNORECASE)

def parse_one_line(s: str) -> Tuple[str, Dict[str, object]]:
    m = QAC_RE.match(s)
    if m:
        return "qac", dict(file=m.group("file"), line=int(m.group("line")), col=None,
                           severity=m.group("severity"), tool_rule=m.group("rule").strip(), msg=m.group("msg"))
    m = LINT_RE.match(s)
    if m:
        return "lint", dict(file=m.group("file"), line=int(m.group("line")), col=int(m.group("col")),
                            severity=m.group("severity"), tool_rule=m.group("rule").strip(), msg=m.group("msg"))
    m = COVERITY_FILELINE_RE.match(s)
    if m:
        return "coverity", dict(file=m.group("file"), line=int(m.group("line")), col=None,
                                severity="warning", tool_rule=f"CID {m.group('cid')}", msg=m.group("msg"))
    m = COVERITY_CID_PREFIX_RE.match(s)
    if m:
        return "coverity", dict(file=None, line=None, col=None,
                                severity="warning", tool_rule=f"CID {m.group('cid')}", msg=m.group("msg"))
    m = CLANGSTYLE_RE.match(s)
    if m:
        rule = (m.group("rule") or "").strip() or None
        tool = "compiler"
        if rule:
            rl = rule.lower()
            if "cppcheck" in rl or "misra" in rl:
                tool = "cppcheck"
            else:
                tool = "clang-tidy"
        return tool, dict(file=m.group("file"), line=int(m.group("line")), col=int(m.group("col")),
                          severity=m.group("severity"), tool_rule=rule, msg=m.group("msg"))
    return "unknown", dict(file=None, line=None, col=None, severity=None, tool_rule=None, msg=s)

def mk_finding(tool: str, raw: str, fields: Dict[str, object], rule_map: Dict[str, object]) -> Finding:
    sev = normalize_severity(fields.get("severity"))
    msg = str(fields.get("msg") or "").strip()
    tool_rule = fields.get("tool_rule")
    misra = extract_misra_rule(raw) or extract_misra_rule(msg) or (extract_misra_rule(str(tool_rule)) if tool_rule else None)
    bucket, fix_patterns = classify(rule_map, f"{tool_rule or ''} {msg} {raw}")
    return Finding(
        tool=tool,
        tool_rule=str(tool_rule) if tool_rule is not None else None,
        misra_rule=misra,
        severity=sev,
        file=fields.get("file"),
        line=fields.get("line"),
        column=fields.get("col"),
        message=msg,
        bucket=bucket,
        fix_patterns=fix_patterns,
        raw=raw.rstrip("\\n"),
    )

def parse_stream(lines: Iterable[str], rule_map: Dict[str, object]) -> List[Finding]:
    out: List[Finding] = []
    for line in lines:
        s = line.rstrip("\\n")
        if not s.strip():
            continue
        tool, fields = parse_one_line(s)
        out.append(mk_finding(tool, s, fields, rule_map))
    return out

def compute_stats(findings: List[Finding]) -> Dict[str, object]:
    return {
        "total": len(findings),
        "by_tool": dict(Counter(f.tool for f in findings)),
        "by_severity": dict(Counter(f.severity for f in findings)),
        "by_bucket": dict(Counter(f.bucket for f in findings)),
        "by_file": dict(Counter(f.file or "<unknown>" for f in findings)),
    }

def main() -> int:
    ap = argparse.ArgumentParser(description="Normalize and classify MISRA-oriented findings.")
    ap.add_argument("--input", "-i", default="-")
    ap.add_argument("--output", "-o", default="-")
    ap.add_argument("--rule-map", default="data/rule_map.yml")
    args = ap.parse_args()

    rule_map = load_rule_map(args.rule_map)

    if args.input == "-":
        lines = sys.stdin.read().splitlines(True)
        input_name = "stdin"
    else:
        with open(args.input, "r", encoding="utf-8", errors="replace") as f:
            lines = f.readlines()
        input_name = args.input

    findings = parse_stream(lines, rule_map)
    payload = {
        "schema_version": SCHEMA_VERSION,
        "generated_at_utc": _dt.datetime.utcnow().replace(microsecond=0).isoformat() + "Z",
        "source": {"tool_hint": "auto", "input_name": input_name},
        "findings": [dataclasses.asdict(f) for f in findings],
        "stats": compute_stats(findings),
    }

    out = json.dumps(payload, ensure_ascii=False, indent=2)
    if args.output == "-":
        sys.stdout.write(out + "\\n")
    else:
        with open(args.output, "w", encoding="utf-8") as f:
            f.write(out + "\\n")

    sys.stderr.write(f"[misra_parse] total={payload['stats']['total']} bucket={payload['stats']['by_bucket']}\\n")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
