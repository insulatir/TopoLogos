# Dockerfile
FROM ubuntu:22.04

# 상호작용 없는 설치 모드 설정
ENV DEBIAN_FRONTEND=noninteractive

# [NEW] 🚀 속도 패치: 기본 서버를 'mirror.kakao.com'으로 변경 (필수)
RUN sed -i 's/archive.ubuntu.com/mirror.kakao.com/g' /etc/apt/sources.list && \
    sed -i 's/security.ubuntu.com/mirror.kakao.com/g' /etc/apt/sources.list

# 1. 시스템 패키지 설치 (C++, Python, SQLite, Graphviz, 한글 폰트)
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    python3 \
    python3-pip \
    libsqlite3-dev \
    graphviz \
    fonts-nanum \
    && rm -rf /var/lib/apt/lists/*

# 작업 디렉토리 설정
WORKDIR /app

# 2. Python 의존성 설치
COPY requirements.txt .
RUN pip3 install --no-cache-dir -r requirements.txt

# 3. 소스 코드 복사
COPY . .

# 4. C++ 엔진 빌드
RUN mkdir -p build && cd build && \
    cmake .. && \
    make

# 5. ONNX 런타임 라이브러리 경로 설정 (매우 중요)
ENV LD_LIBRARY_PATH="/app/external/onnxruntime/lib:${LD_LIBRARY_PATH}"

# 기본 실행 명령 (컨테이너 실행 시 오버라이딩 가능)
CMD ["./build/TopoLogos", "config/life.topo"]