from __future__ import annotations
from pathlib import Path
from typing import Any, Dict
from .rag03_embeddings import OpenAIEmbeddingProvider, EmbeddingProvider
from .rag03_index_faiss import FaissIndex
from .rag03_meta_sqlite import MetaStore

def pick_embedder(kind: str, model: str) -> EmbeddingProvider:
    kind = kind.lower()
    if kind == "openai":
        return OpenAIEmbeddingProvider(model=model)
    raise ValueError(f"Unknown embedder kind: {kind}")

def rag_query(index_dir: Path, query: str, *, top_k: int = 8, embedder_kind: str = "openai", 
              embedder_model: str = "text-embedding-3-large", ) -> Dict[str, Any]:
    faiss_path = index_dir / "faiss.index"
    sqlite_path = index_dir / "meta.sqlite"

    if not faiss_path.exists() or not sqlite_path.exists():
        raise FileNotFoundError(f"Index not found in {index_dir}: need faiss.index and meta.sqlite")

    embedder = pick_embedder(embedder_kind, embedder_model)
    qvec = embedder.embed_texts([query])  # (1, dim)

    fi = FaissIndex.load(faiss_path)
    ids, scores = fi.search(qvec, top_k=top_k)

    # FAISS can return -1 for empty results
    pairs = [(i, s) for i, s in zip(ids, scores) if i is not None and int(i) >= 0]
    ids2 = [int(i) for i, _ in pairs]
    scores2 = [float(s) for _, s in pairs]

    meta = MetaStore(sqlite_path)
    chunks = meta.get_chunks_by_faiss_ids(ids2)
    meta.close()

    # attach scores in the same order
    for c, s in zip(chunks, scores2):
        c["score"] = s

    return {"query": query, "top_k": top_k, "hits": chunks, "index_dir": str(index_dir.resolve()),}

