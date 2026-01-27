cmake_minimum_required(VERSION 3.20)

# 프로젝트 이름 및 버전
project(TopoLogos VERSION 0.2.0 LANGUAGES CXX)

# C++23 표준 강제
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SQLite3 REQUIRED)

# [의존성 추가] nlohmann_json (JSON 처리를 위해 필수)
include(FetchContent)
FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz
)
FetchContent_MakeAvailable(json)

# 헤더 경로 포함
include_directories(include)

# 소스 파일 목록 정의
# (기존 소스 + 새로 만든 bullshit_detector.cpp 추가)
set(SOURCES
    src/main.cpp
    src/verification/bullshit_detector.cpp
    src/ai/engine.cpp
    src/parser/parser.cpp
    src/parser/lexer.cpp
    src/codegen/generator.cpp
    # src/rag_app.cpp 는 삭제하거나 제외
)

# 실행 파일 생성
add_executable(TopoLogos ${SOURCES})

# 2. [핵심 수정] 헤더 파일 경로 추가
# ${CMAKE_SOURCE_DIR}는 프로젝트 루트 경로를 의미합니다.
target_include_directories(TopoLogos PRIVATE
    "${CMAKE_SOURCE_DIR}/include"                        # 내 프로젝트 헤더
    "${CMAKE_SOURCE_DIR}/external/onnxruntime/include"   # ONNX Runtime 헤더 (이게 없어서 에러 발생)
)

# 라이브러리 링크 (JSON 및 Thread 등)
target_link_libraries(TopoLogos PRIVATE
    "${CMAKE_SOURCE_DIR}/external/onnxruntime/lib/libonnxruntime.so.1.17.1"
    "nlohmann_json::nlohmann_json"
    "SQLite::SQLite3")

if(UNIX)
    target_link_libraries(TopoLogos PRIVATE pthread dl)
endif()

message(STATUS "TopoLogos Configured with Structural Existentialism Modules.")