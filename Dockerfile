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

# 2. Builder 스테이지: C++ 빌드 전용
FROM base AS builder
# [변경] wget 추가 (ONNX Runtime 다운로드용)
RUN apt-get update && apt-get install -y \
    build-essential cmake git libsqlite3-dev libssl-dev wget

WORKDIR /app

# [New] ONNX Runtime 다운로드 및 설치 (Git에 올리지 않고 빌드 때 받음)
# v1.17.1 Linux x64 버전 다운로드 -> 압축 해제 -> external/onnxruntime 폴더로 배치
RUN mkdir -p external && \
    wget -q https://github.com/microsoft/onnxruntime/releases/download/v1.17.1/onnxruntime-linux-x64-1.17.1.tgz && \
    tar -xf onnxruntime-linux-x64-1.17.1.tgz && \
    mv onnxruntime-linux-x64-1.17.1 external/onnxruntime && \
    rm onnxruntime-linux-x64-1.17.1.tgz

# 소스 복사 (이때 로컬의 빈 external 폴더가 덮어씌워지지 않도록 주의, Docker는 병합함)
COPY . .

# CMake 빌드
RUN mkdir -p build && cd build && cmake .. && make

# ---------------------------------------------------------
# 3. Engine 타겟: 실제 실행용 엔진 이미지
# ---------------------------------------------------------
FROM base AS engine
# 빌드된 결과물만 쏙 빼옵니다
COPY --from=builder /app/build/ /app/build/
COPY --from=builder /app/config/ /app/config/
# [중요] Builder에서 다운로드 받은 external 폴더를 가져옵니다
COPY --from=builder /app/external/ /app/external/

ENV LD_LIBRARY_PATH="/app/external/onnxruntime/lib:${LD_LIBRARY_PATH}"
CMD ["./build/TopoLogos", "config/life.topo"]

# Miner 부분 수정
FROM base AS miner
WORKDIR /app
# [변경] tools/miner 폴더 통째로 복사
COPY tools/miner /app/tools/miner 
CMD ["python3", "tools/miner/main.py", "--daemon", "--rss", "https://techcrunch.com/feed/"]

# Dashboard 부분 수정
FROM base AS dashboard
WORKDIR /app
# [변경] tools/dashboard 폴더 통째로 복사
COPY tools/dashboard /app/tools/dashboard
EXPOSE 5000
CMD ["python3", "tools/dashboard/app.py"]