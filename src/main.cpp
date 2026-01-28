#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <filesystem>

// Act 1 Headers
#include "topologos/parser/lexer.hpp"
#include "topologos/parser/parser.hpp"
#include "topologos/codegen/generator.hpp"
#include "topologos/ai/engine.hpp"

// Act 2 Headers
#include "topologos/verification/bullshit_detector.hpp"
#include "topologos/storage/knowledge_graph.hpp"

namespace fs = std::filesystem;

std::string resolve_path(const std::string& path) {
    if (fs::exists(path)) return path;
    std::string up_path = "../" + path;
    if (fs::exists(up_path)) return up_path;
    return path;
}

int main(int argc, char* argv[]) {
    std::cout << "[TopoLogos] Initializing Research Core..." << std::endl;

    std::string rule_file = (argc > 1) ? argv[1] : resolve_path("config/life.topo");
    std::string inbox_path = resolve_path("data/inbox");
    std::string db_path = "data/topo_db.sqlite"; 
    if (fs::exists("../data")) db_path = "../data/topo_db.sqlite";

    topologos::ai::NLIEngine* engine_ptr = nullptr;
    
    // ---------------------------------------------------------
    // [Phase 1] Verification & Persistence
    // ---------------------------------------------------------
    std::cout << "\n=== [Phase 1] Structural Verification & Assimilation ===\n" << std::endl;
    
    topologos::verification::BullshitDetector detector(rule_file);
    topologos::storage::KnowledgeGraph db(db_path);

    if (fs::exists(inbox_path)) {
        for (const auto& entry : fs::directory_iterator(inbox_path)) {
            if (entry.path().extension() == ".json") {
                std::cout << ">> Inspecting: " << entry.path().filename() << "... ";
                try {
                    std::ifstream f(entry.path());
                    nlohmann::json data = nlohmann::json::parse(f);
                    
                    // 판별 수행
                    auto verdict = detector.judge(data);

                    if (verdict.is_truth) {
                        std::cout << "[TRUTH] (Score: " << verdict.integrity_score << ") -> Saving.\n";
                        db.add_verified_node(data, verdict.integrity_score);
                    } else {
                        std::cout << "[SCAM] Discarded (Score: " << verdict.integrity_score << ")\n";
                        for (const auto& msg : verdict.violations) {
                            std::cout << "    - " << msg << "\n";
                        }
                    }
                } catch (const std::exception& e) {
                    std::cerr << "Error processing file: " << e.what() << "\n";
                }
            }
        }
    } else {
        std::cout << "[Info] Inbox directory not found (" << inbox_path << "). Waiting for Miner...\n";
    }

    // ---------------------------------------------------------
    // [Phase 2] Generating Knowledge Graph (Visualization)
    // ---------------------------------------------------------
    std::cout << "\n=== [Phase 2] Generating Knowledge Graph (Visualization) ===\n";
    
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
        // AI 엔진 없이 코드 생성기 초기화
        topologos::codegen::CppGenerator generator(nullptr); 
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