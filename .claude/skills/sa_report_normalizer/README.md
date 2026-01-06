# sa_report_normalizer (Skill Package)

This directory is intended to be copied into your project's skills folder.

Quick test:

```bash
cat <<'EOF' > /tmp/sa.log
src/a.c(10): Warning 1234: MISRA C:2012 Rule 10.1 signed/unsigned conversion
src/b.c 20 5 Warning 567: something
src/c.c:30:7: warning: message [modernize-use-nullptr]
src/d.c:40:2: warning: msg [cppcheck-misraRule]
src/e.c:50: note: -Wsomething
src/f.c:60: Null dereference (CID 12345)
EOF

python3 scripts/sa_report_normalizer.py --input /tmp/sa.log --output /tmp/findings.json --csv /tmp/findings.csv
cat /tmp/findings.json | head
```
