# misra_helper (Skill Package)

## Quick test

```bash
cat <<'EOF' > /tmp/misra.log
src/a.c(10): Warning 1234: MISRA C:2012 Rule 10.1 signed/unsigned conversion
src/b.c 20 5 Warning 567: expression has side effect and no sequence point
src/c.c:30:7: warning: do not use strcpy [cppcheck-misraRule]
src/e.c:50: note: -Wsomething
src/f.c:60: Null dereference (CID 12345)
EOF

python3 scripts/misra_parse.py --input /tmp/misra.log --output /tmp/misra_findings.json
cat /tmp/misra_findings.json | head -n 60

# Patch guard
git diff | python3 scripts/misra_patch_guard.py
```

## PDF index (optional)
- Put your internal index at `data/misra_pdf_index.jsonl` (JSONL, one record per rule/directive).
- Use `scripts/misra_pdf_query.py` to fetch page/section and short excerpt.

