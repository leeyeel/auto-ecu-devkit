from __future__ import annotations
import os
from pathlib import Path
from typing import Any, Dict, List, Optional
from rag03_query import rag_query

def format_hits_for_prompt(hits: List[Dict[str, Any]], *, max_chars_each: int = 1200) -> str:
    lines: List[str] = []
    for i, h in enumerate(hits, start=1):
        src = Path(h.get("source_pdf", "")).name or h.get("doc", "manual")
        pages = f"p.{h.get('page_start')}–{h.get('page_end')}"
        section = h.get("section_title") or "(no section title)"
        score = h.get("score")
        text = (h.get("text") or "").strip()
        if len(text) > max_chars_each:
            text = text[:max_chars_each].rstrip() + " ..."

        lines.append(f"[{i}] SOURCE: {src} | {pages} | {section} | score={score:.4f}")
        lines.append(text)
        lines.append("")  # blank line between hits
    return "\n".join(lines).strip()

def looks_like_chip_question(q: str) -> bool:
    ql = q.lower()
    keywords = [
        "s32k", "nxp", "flexcan", "can", "lin", "spi", "i2c", "uart",
        "register", "bit", "clock", "pll", "interrupt", "dma", "gpio",
        "freertos", "adc", "pwm", "timer", "wdog"
    ]
    return any(k in ql for k in keywords)

def rag_augment(user_query: str) -> Optional[str]:
    """
    Returns a formatted evidence block, or None if RAG is not configured / no hits.
    Controlled by env vars to keep MVP simple.
    """
    if not looks_like_chip_question(user_query):
        return None

    index_dir = os.getenv("RAG_INDEX_DIR")
    if not index_dir:
        return None

    top_k = int(os.getenv("RAG_TOP_K", "8"))
    embed_model = os.getenv("RAG_EMBED_MODEL", "Qwen/Qwen3-Embedding-8B")
    embedder_kind = "openai"

    try:
        res = rag_query(
            Path(index_dir).expanduser(),
            user_query,
            top_k=top_k,
            embedder_kind=embedder_kind,
            embedder_model=embed_model,
        )
    except Exception:
        return None

    hits = res.get("hits") or []
    if not hits:
        return None

    evidence = format_hits_for_prompt(hits)
    return (
        "You MUST base your answer and any code on the following cited manual excerpts. "
        "If something is not supported by the excerpts, explicitly say it is an assumption.\n\n"
        "=== MANUAL EVIDENCE (RAG) ===\n"
        f"{evidence}\n"
        "=== END EVIDENCE ==="
    )

