# tools/ask_brain.py
import sys
from qdrant_client import QdrantClient
from sentence_transformers import SentenceTransformer # 로컬 임베딩용 (또는 API 사용)

# 1. 질문을 벡터로 변환 (엔진과 같은 모델 사용해야 함)
# 테스트용으로는 fast-embed나 sentence-transformers 사용
model = SentenceTransformer('all-MiniLM-L6-v2') 
client = QdrantClient("localhost", port=6333)

query = sys.argv[1] if len(sys.argv) > 1 else "What happened with Google?"
vector = model.encode(query).tolist()

# 2. Qdrant 검색
results = client.search(
    collection_name="topologos_knowledge",
    query_vector=vector,
    limit=3
)

print(f"🔎 Searching for: {query}")
for hit in results:
    print(f"- [Score: {hit.score:.4f}] {hit.payload['summary']} (ID: {hit.payload['original_id']})")