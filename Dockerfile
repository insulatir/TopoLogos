# 1. Base 스테이지: 공통 환경 (Python, 한글 폰트 등)
FROM ubuntu:22.04 AS base
ENV DEBIAN_FRONTEND=noninteractive
RUN sed -i 's/archive.ubuntu.com/mirror.kakao.com/g' /etc/apt/sources.list && \
    sed -i 's/security.ubuntu.com/mirror.kakao.com/g' /etc/apt/sources.list
RUN apt-get update && apt-get install -y \
    python3 python3-pip fonts-nanum libsqlite3-0 graphviz \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /app
COPY requirements.txt .
RUN pip3 install --no-cache-dir -r requirements.txt

# 2. Builder 스테이지: C++ 빌드 전용 (이미지 크기 최적화를 위해 나중에 버려짐)
FROM base AS builder
RUN apt-get update && apt-get install -y build-essential cmake git libsqlite3-dev
COPY . .
RUN mkdir -p build && cd build && cmake .. && make

# ---------------------------------------------------------
# 3. Engine 타겟: 실제 실행용 엔진 이미지
# ---------------------------------------------------------
FROM base AS engine
# 빌드된 결과물만 쏙 빼옵니다 (용량 다이어트)
COPY --from=builder /app/build/ /app/build/
COPY --from=builder /app/config/ /app/config/
COPY --from=builder /app/external/ /app/external/
ENV LD_LIBRARY_PATH="/app/external/onnxruntime/lib:${LD_LIBRARY_PATH}"
CMD ["./build/TopoLogos", "config/life.topo"]

# ---------------------------------------------------------
# 4. Miner 타겟: 뉴스 수집용 이미지
# ---------------------------------------------------------
FROM base AS miner
COPY tools/miner/ /app/tools/miner/
CMD ["python3", "tools/miner/miner.py", "--daemon", "--rss", "https://techcrunch.com/feed/"]

# ---------------------------------------------------------
# 5. Dashboard 타겟: 시각화 웹 이미지
# ---------------------------------------------------------
FROM base AS dashboard
COPY tools/dashboard/ /app/tools/dashboard/
COPY data/ /app/data/ 
EXPOSE 5000
CMD ["python3", "tools/dashboard/app.py"]