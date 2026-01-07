from __future__ import annotations
from pathlib import Path
from typing import List, Tuple
import numpy as np
import faiss

def l2_normalize(x: np.ndarray, eps: float = 1e-12) -> np.ndarray:
    norms = np.linalg.norm(x, axis=1, keepdims=True)
    return x / np.maximum(norms, eps)

class FaissIndex:
    def __init__(self, dim: int):
        self.dim = dim
        self.index = faiss.IndexFlatIP(dim)  # cosine via normalized vectors

    @property
    def ntotal(self) -> int:
        return self.index.ntotal

    def add(self, vecs: np.ndarray) -> List[int]:
        vecs = vecs.astype(np.float32, copy=False)
        vecs = l2_normalize(vecs)
        start_id = self.ntotal
        self.index.add(vecs)
        return list(range(start_id, start_id + vecs.shape[0]))

    def search(self, qvec: np.ndarray, top_k: int) -> Tuple[List[int], List[float]]:
        qvec = qvec.astype(np.float32, copy=False)
        qvec = l2_normalize(qvec)
        scores, ids = self.index.search(qvec, top_k)
        # qvec shape (1, dim) => first row
        return ids[0].tolist(), scores[0].tolist()

    def save(self, path: Path) -> None:
        path.parent.mkdir(parents=True, exist_ok=True)
        faiss.write_index(self.index, str(path))

    @staticmethod
    def load(path: Path) -> "FaissIndex":
        idx = faiss.read_index(str(path))
        fi = FaissIndex(idx.d)
        fi.index = idx
        fi.dim = idx.d
        return fi
