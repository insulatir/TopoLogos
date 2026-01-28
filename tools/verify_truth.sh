#!/bin/bash
# tools/verify_truth.sh

echo "🔍 Verifying Reality Consistency..."

# C++23 표준으로 컴파일 시도
g++ -std=c++23 examples/robot_brain.cpp -o robot_brain

if [ $? -eq 0 ]; then
    echo "✅ [SUCCESS] Reality is logical. Robot is advancing."
    ./robot_brain
else
    echo "❌ [FAILURE] Reality is broken or empty. Robot cannot act."
fi