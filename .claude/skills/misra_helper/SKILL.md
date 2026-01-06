---
name: misra_helper
description: Explain MISRA C rule violations and produce minimal, behavior-preserving fixes for Automotive ECU C code. Parses common static analysis outputs, classifies findings into MISRA-oriented buckets, suggests fix patterns, and can optionally cite official MISRA PDF locations if an index is provided.
license: Proprietary (customer owns MISRA/ISO26262 PDFs)
---

# misra_helper — MISRA 规则解释与最小修复助手

该 skill 面向 **汽车 ECU** 工程，目标是把静态分析/检查工具的输出，转成**可读、可审计、可自动化**的解释与修复建议。

支持两类核心工作流：

1) **解释（Explain）**：给定告警与/或代码，解释问题本质与 ECU 场景风险。  
2) **最小修复（Minimal Fix）**：输出 **最小 diff、保持行为不变** 的修复方案（必要时给 unified diff）。

> 重要说明  
> - 本 skill 不替代你们的 MISRA 工具链；CI/静态分析仍是最终裁判。  
> - 默认立场保守：不大重构、不改外部接口、不引入动态内存、不引入禁用 API。  

---

## 1. 能做什么

- 解析并归一化常见工具输出：
  - QAC / QAC++（典型 file(line): severity rule: message）
  - PC-lint / FlexeLint
  - cppcheck MISRA addon
  - clang-tidy（部分检查与 MISRA 风格问题重叠）
  - 编译器 warning/error（很多会与未定义行为/初始化相关）
  - Coverity（常见文本格式的 best-effort 支持）
- 输出：
  1) `findings.json`（机器可读）
  2) 统计汇总（stderr，便于 CI 日志）
- 可选：查询本地 **MISRA PDF 索引**（若你们提供），返回规则页码/章节/短摘录

---

## 2. 输入

### 2.1 必需（满足其一即可）
- 静态分析报告文本（log），或
- 用户提供的代码片段 + 问题描述

### 2.2 强烈建议
- 报告 + 相关代码（可定位行号）
- 约束条件：
  - 保持 API 不变
  - ISR/Task 上下文
  - 禁止动态内存
  - 时序/栈/内存限制

---

## 3. 输出结构（Claude/Agent 侧建议固定为 4 段）

1) **Findings 表格**  
`{file, line, tool, tool_rule, misra_rule(若可提取), severity, bucket, short risk}`

2) **规则解释（工程视角）**  
- 为什么要限制
- 在 ECU/实时/安全场景下的具体危害

3) **最小修复计划**  
- 明确最小修改策略（拆表达式/显式转换/补 default/补 NULL 检查等）
- 尽量给 unified diff

4) **验证清单**  
- 需要重新跑：build、单测、MISRA、静态分析
- 若仍需偏离（deviation），给出需要补的审计材料要点

可选第 5 段（如果索引存在）：

5) **MISRA PDF 引用（可选）**  
- 规则号/指令号
- 章节/页码
- 短摘录（你们自行控制长度策略）

---

## 4. JSON Schema（v1）

`misra_parse.py` 输出：

```json
{
  "schema_version": "misra_helper_v1",
  "generated_at_utc": "2026-01-06T00:00:00Z",
  "source": {"tool_hint": "auto", "input_name": "stdin"},
  "findings": [
    {
      "tool": "qac",
      "tool_rule": "1234",
      "misra_rule": "10.1",
      "severity": "warning",
      "file": "src/foo.c",
      "line": 12,
      "column": null,
      "message": "MISRA C:2012 Rule 10.1 ...",
      "bucket": "essential_type",
      "fix_patterns": ["use explicit cast with justification"],
      "raw": "..."
    }
  ],
  "stats": {...}
}
```

---

## 5. 使用方式

### 5.1 解析报告 -> JSON
```bash
cat report.log | python3 scripts/misra_parse.py > findings.json
```

### 5.2 Patch Guard（禁止项“先挡住”）
```bash
git diff | python3 scripts/misra_patch_guard.py
```

### 5.3 可选：查询 MISRA PDF 索引
```bash
python3 scripts/misra_pdf_query.py --rule 10.1 --index data/misra_pdf_index.jsonl
python3 scripts/misra_pdf_query.py --query "side effects expression" --index data/misra_pdf_index.jsonl
```

---

## 6. 扩展方式

- `scripts/misra_parse.py`：补充更多工具输出 regex
- `data/rule_map.yml`：补充关键字与 bucket 归类、修复套路
- `data/misra_pdf_index.jsonl`：放你们内部生成的 MISRA PDF 索引（JSONL，一行一条规则/指令）

