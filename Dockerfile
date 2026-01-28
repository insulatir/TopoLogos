# 1. Base 스테이지
FROM ubuntu:22.04 AS base
ENV DEBIAN_FRONTEND=noninteractive
# 카카오 미러 사용 (속도 향상)
RUN sed -i 's/archive.ubuntu.com/mirror.kakao.com/g' /etc/apt/sources.list && \
    sed -i 's/security.ubuntu.com/mirror.kakao.com/g' /etc/apt/sources.list
RUN apt-get update && apt-get install -y \
    python3 python3-pip fonts-nanum libsqlite3-0 graphviz \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /app
COPY requirements.txt .
RUN pip3 install --no-cache-dir -r requirements.txt

# 2. Builder 스테이지
FROM base AS builder
RUN apt-get update && apt-get install -y \
    build-essential cmake git libsqlite3-dev libssl-dev wget

WORKDIR /app

# [Network] 1. ONNX Runtime 라이브러리 다운로드
RUN mkdir -p external && \
    wget -q https://github.com/microsoft/onnxruntime/releases/download/v1.17.1/onnxruntime-linux-x64-1.17.1.tgz && \
    tar -xf onnxruntime-linux-x64-1.17.1.tgz && \
    mv onnxruntime-linux-x64-1.17.1 external/onnxruntime && \
    rm onnxruntime-linux-x64-1.17.1.tgz

# 2. [변경] 모델 파일은 로컬에서 복사 (인증 에러 방지)
COPY external/onnxruntime/bert_nli.onnx external/onnxruntime/bert_nli.onnx
COPY . .

# CMake 빌드
RUN mkdir -p build && cd build && cmake .. && make

# 3. Engine 타겟
FROM base AS engine
COPY --from=builder /app/build/ /app/build/
COPY --from=builder /app/config/ /app/config/
COPY --from=builder /app/external/ /app/external/
# vocab.txt는 소스 루트에 있으므로 복사
COPY --from=builder /app/vocab.txt /app/vocab.txt

ENV LD_LIBRARY_PATH="/app/external/onnxruntime/lib:${LD_LIBRARY_PATH}"
CMD ["./build/TopoLogos", "config/life.topo"]

# 4. Miner 타겟
FROM base AS miner
WORKDIR /app
COPY tools/miner /app/tools/miner
RUN mkdir -p /app/data/inbox
CMD ["python3", "tools/miner/main.py", "--daemon", "--rss", "https://techcrunch.com/feed/"]

# 5. Dashboard 타겟
FROM base AS dashboard
WORKDIR /app
COPY tools/dashboard /app/tools/dashboard
RUN mkdir -p /app/data/db /app/data/inbox
EXPOSE 5000
CMD ["python3", "tools/dashboard/app.py"]