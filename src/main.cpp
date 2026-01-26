#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include "../include/topologos/parser/lexer.hpp"
#include "../include/topologos/parser/parser.hpp"
#include "../include/topologos/codegen/generator.hpp"
#include "../include/topologos/ai/engine.hpp"

// [수정] main 함수 시그니처 변경 (인자 받기 위함)
int main(int argc, char* argv[]) {
    std::cout << "[TopoLogos] Initializing Research Core...\n" << std::endl;

    // 1. AI 엔진 초기화
    topologos::ai::NLIEngine* engine_ptr = nullptr;
    try {
        topologos::ai::EngineConfig config;
        config.model_path = "bert_nli.onnx";
        config.vocab_path = "vocab.txt";
        config.threshold = 2.5f; 

        static topologos::ai::NLIEngine engine(config);
        engine_ptr = &engine;
        std::cout << "[System] AI Logical Core Online." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[Warning] AI Offline: " << e.what() << std::endl;
    }

    std::cout << "\n=== [Phase 1] Loading Source Code ===\n" << std::endl;

    // [수정] 파일에서 읽어오기 로직
    std::string filename = "life.topo"; // 기본값
    if (argc > 1) filename = argv[1];

    std::ifstream file(filename);
    std::stringstream buffer;
    if (file.is_open()) {
        buffer << file.rdbuf();
        std::cout << "✅ Loaded file: " << filename << std::endl;
    } else {
        // 파일이 없으면 기본 테스트 코드 사용 (에러 방지용)
        std::cerr << "⚠️ File not found. Using default script.\n";
        buffer << "Community DevLife { Concept Coding; Concept Bug; Coding -> Bug; }";
    }

    // [수정] 중복 선언 방지 (여기서 딱 한 번만 선언)
    std::string source = buffer.str();

    // 2. 파싱
    topologos::parser::Lexer lexer(source);
    auto tokens = lexer.tokenize();
    topologos::parser::Parser parser(tokens);
    auto program = parser.parse();

    if (program) {
        std::cout << "\n=== [Phase 3] Generating Knowledge & Visualization ===\n" << std::endl;
        
        topologos::codegen::CppGenerator generator(engine_ptr);
        
        // [수정] program.get()을 사용하여 포인터만 전달
        std::string cpp_code = generator.generate(program.get());
        
        // C++ 헤더 저장
        std::ofstream out_file("knowledge.hpp");
        if (out_file.is_open()) {
            out_file << "#pragma once\n";
            out_file << cpp_code;
            out_file.close();
            std::cout << "✅ Generated 'knowledge.hpp'" << std::endl;
        }

        // [New] Mermaid 그래프 출력
        std::cout << "\n=== 📊 Knowledge Graph (Mermaid) ===\n";
        std::cout << "graph TD;\n";
        std::cout << generator.get_mermaid(); // 여기가 핵심!
        std::cout << "====================================\n" << std::endl;

    } else {
        std::cerr << "Parsing Failed." << std::endl;
    }

    return 0;
}