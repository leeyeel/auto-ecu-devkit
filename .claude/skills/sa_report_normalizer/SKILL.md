---
name: sa_report_normalizer
description: Normalize static analysis reports (QAC, PC-lint/FlexeLint, Coverity, clang-tidy, cppcheck, GCC/Clang warnings) into a single JSON schema for Automotive ECU CI pipelines. Produces stable, machine-readable findings plus human-friendly summaries.
license: Proprietary (internal use)
---

# sa_report_normalizer — Static Analysis Report Normalizer

This skill converts heterogeneous static analysis outputs into a **single normalized JSON format** so downstream tooling (CI gates, dashboards, MISRA helpers, baselines) can operate consistently.

## What this skill does
- Parses raw logs from multiple tools into a common schema:
  - QAC / QAC++-style
  - PC-lint / FlexeLint
  - Coverity (common text formats)
  - clang-tidy
  - cppcheck
  - GCC/Clang compiler warnings/errors
- Emits:
  1) `findings.json` (machine-readable)
  2) Summary text (counts by tool/severity/rule/file)
  3) Optional CSV for spreadsheets

## Normalized JSON schema (v1)

```json
{
  "schema_version": "sa_normalized_v1",
  "generated_at_utc": "2026-01-06T00:00:00Z",
  "source": {"tool_hint": "auto", "input_name": "stdin"},
  "findings": [
    {
      "tool": "clang-tidy",
      "tool_rule": "modernize-use-nullptr",
      "misra_rule": null,
      "severity": "warning",
      "file": "src/foo.c",
      "line": 123,
      "column": 7,
      "function": "Foo_Init",
      "message": "use nullptr",
      "category": "style",
      "raw": "..."
    }
  ],
  "stats": {
    "total": 10,
    "by_tool": {"clang-tidy": 3},
    "by_severity": {"warning": 8, "error": 2},
    "by_file": {"src/foo.c": 4}
  }
}
```

### Field notes
- `tool`: canonical tool id (`qac`, `lint`, `coverity`, `clang-tidy`, `cppcheck`, `compiler`, `unknown`)
- `tool_rule`: rule/check id if available (e.g., `MISRA2012-10.1`, `modernize-use-nullptr`, `CID 12345`)
- `misra_rule`: extracted MISRA rule id if present (best-effort)
- `severity`: one of `error|warning|note|info` (normalized)
- `category`: best-effort category (e.g., `misra`, `security`, `style`, `bug`, `performance`, `portability`, `build`)
- `raw`: original line (preserved)

## Usage

### 1) From stdin
```bash
cat report.log | python3 scripts/sa_report_normalizer.py > findings.json
```

### 2) From a file
```bash
python3 scripts/sa_report_normalizer.py --input report.log --output findings.json
```

### 3) Emit CSV as well
```bash
python3 scripts/sa_report_normalizer.py --input report.log --output findings.json --csv findings.csv
```

### 4) Force a tool hint (optional)
```bash
python3 scripts/sa_report_normalizer.py --input report.log --tool-hint clang-tidy --output findings.json
```

## CI gate integration (example)
- Store `findings.json` as an artifact.
- Feed it to:
  - MISRA explain/fix helper
  - baseline comparison / warning budget tools
  - dashboards

## Extending parsers
Edit `scripts/sa_report_normalizer.py`:
- Add a new regex in `PARSERS`
- Map severities using `normalize_severity()`

## Limitations
- Coverity has many output formats. This parser supports common text patterns and is designed to be extended.
- MISRA rule extraction is best-effort and depends on the upstream tool output.
