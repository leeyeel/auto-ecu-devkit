from __future__ import annotations
import os
import numpy as np
from typing import List, Protocol 

class EmbeddingProvider(Protocol):
    def embed_texts(self, texts: List[str]) -> np.ndarray:
        """Return shape (n, dim) float32 ndarray."""

class OpenAIEmbeddingProvider:
    """
    Uses OpenAI embeddings API via `openai` python package.
    Env:
      - OPENAI_API_KEY required
      - OPENAI_BASE_URL optional (for compatible gateways)
    """
    def __init__(self, model: str = "text-embedding-3-large", batch_size: int = 64):
        self.model = model
        self.batch_size = batch_size

        from openai import OpenAI  # lazy import
        base_url = os.getenv("OPENAI_BASE_URL")
        self.client = OpenAI(api_key=os.getenv("OPENAI_API_KEY"), base_url=base_url)

    def embed_texts(self, texts: List[str]) -> np.ndarray:
        import numpy as np

        all_vecs: list[list[float]] = []
        for i in range(0, len(texts), self.batch_size):
            batch = texts[i : i + self.batch_size]
            resp = self.client.embeddings.create(model=self.model, input=batch)
            # resp.data is in the same order
            all_vecs.extend([d.embedding for d in resp.data])

        arr = np.array(all_vecs, dtype=np.float32)
        return arr
