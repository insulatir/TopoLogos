#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <filesystem>

// Act 1 Headers
#include "../include/topologos/parser/lexer.hpp"
#include "../include/topologos/parser/parser.hpp"
#include "../include/topologos/codegen/generator.hpp"
#include "../include/topologos/ai/engine.hpp"

// Act 2 Headers
#include "../include/topologos/verification/bullshit_detector.hpp"
#include "../include/topologos/storage/knowledge_graph.hpp" // [New] DB 기능

namespace fs = std::filesystem;

std::string resolve_path(const std::string& path) {
    if (fs::exists(path)) return path;
    std::string up_path = "../" + path;
    if (fs::exists(up_path)) return up_path;
    return path;
}

int main(int argc, char* argv[]) {
    std::cout << "[TopoLogos] Initializing Research Core...\n" << std::endl;

    // 0. 경로 설정 (수정됨)
    // 규칙, 모델 등은 기존 방식(resolve_path) 유지
    std::string rule_file = (argc > 1) ? argv[1] : resolve_path("config/life.topo");
    std::string model_path = resolve_path("bert_nli.onnx");
    std::string inbox_path = resolve_path("data/inbox");
    
    // [FIX] DB 경로는 '폴더'를 기준으로 잡아야 함
    std::string db_path = "data/topo_db.sqlite"; 
    if (fs::exists("../data")) { 
        db_path = "../data/topo_db.sqlite"; 
    }
    // [Act 1] AI Engine (Optional)
    topologos::ai::NLIEngine* engine_ptr = nullptr;
    // ... (AI 로딩 코드는 동일하므로 생략, 이전 코드 유지 가능하나 깔끔하게 재작성)
    try {
        if (fs::exists(model_path)) {
            topologos::ai::EngineConfig config;
            config.model_path = model_path;
            config.vocab_path = resolve_path("vocab.txt");
            config.threshold = 2.5f; 
            static topologos::ai::NLIEngine engine(config);
            engine_ptr = &engine;
            std::cout << "[System] AI Logical Core Online." << std::endl;
        }
    } catch (...) {}

    // ---------------------------------------------------------
    // [Phase 1] Verification & Persistence
    // ---------------------------------------------------------
    std::cout << "\n=== [Phase 1] Structural Verification & Assimilation ===\n";
    
    topologos::verification::BullshitDetector detector(rule_file);
    topologos::storage::KnowledgeGraph db(db_path); // [New] DB 로드

    if (fs::exists(inbox_path)) {
        for (const auto& entry : fs::directory_iterator(inbox_path)) {
            if (entry.path().extension() == ".json") {
                std::cout << ">> Inspecting: " << entry.path().filename() << "... ";
                try {
                    std::ifstream f(entry.path());
                    nlohmann::json data = nlohmann::json::parse(f);
                    auto verdict = detector.judge(data);

                    if (verdict.is_truth) {
                        std::cout << "[TRUTH] (Score: " << verdict.integrity_score << ") -> Saving.\n";
                        db.add_verified_node(data, verdict.integrity_score);
                    } else {
                        // [수정] 단순히 Discarded만 출력하지 말고 이유를 보여줌
                        std::cout << "[SCAM] Discarded (Score: " << verdict.integrity_score << ")\n";
                        for (const auto& msg : verdict.violations) {
                            std::cout << "    - " << msg << "\n"; // 위반 사유 출력
                        }
                    }
                } catch (const std::exception& e) {
                    std::cerr << "Error: " << e.what() << "\n";
                }
            }
        }
    } else {
        std::cout << "[Info] Inbox directory not found.\n";
    }

    // ---------------------------------------------------------
    // [Phase 2] Generating Knowledge Graph (Visualization)
    // ---------------------------------------------------------
    std::cout << "\n=== [Phase 2] Generating Knowledge Graph (Visualization) ===\n";
    
    // [FIX] 문법을 Act 2(Domain/Axiom) 형식으로 변경해야 파서가 이해합니다.
    std::string source = 
        "domain TopoLogos { \n"
        "  axiom FilterSystem { \n"
        "    rule TruthCheck(score: float) { condition: score > 80; } \n"
        "    rule ScamCheck(score: float)  { condition: score < 50; } \n"
        "  } \n"
        "}";

    topologos::parser::Lexer lexer(source);
    auto tokens = lexer.tokenize();
    topologos::parser::Parser parser(tokens);
    auto program = parser.parse();

    if (program) {
        topologos::codegen::CppGenerator generator(engine_ptr);
        
        // [FIX] 이 줄이 빠져 있었습니다! AST를 순회하며 그래프를 생성합니다.
        generator.generate(program.get()); 

        std::cout << "\n=== 📊 Knowledge Graph (Mermaid) ===\n";
        std::cout << "graph TD;\n";
        std::cout << generator.get_mermaid(); 
        std::cout << "====================================\n" << std::endl;
    } else {
        std::cerr << "Visualization Failed (Check Syntax)." << std::endl;
    }

    return 0;
}
