#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>

// Act 1 Headers
#include "topologos/parser/lexer.hpp"
#include "topologos/parser/parser.hpp"
#include "topologos/codegen/generator.hpp"
#include "topologos/ai/engine.hpp" // [Active]

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

    // ---------------------------------------------------------
    // [Active] AI Engine Initialization
    // ---------------------------------------------------------
    // 이제 Dockerfile이 파일을 보장하므로 바로 초기화합니다.
    std::cout << "[Engine] Booting Neural Core..." << std::endl;
    
    // 엔진 설정 및 생성
    topologos::ai::EngineConfig ai_config;
    ai_config.model_path = resolve_path("external/onnxruntime/bert_nli.onnx");
    ai_config.vocab_path = resolve_path("vocab.txt");

    static topologos::ai::NLIEngine engine(ai_config);
    topologos::ai::NLIEngine* engine_ptr = &engine;

    std::cout << "[Engine] Neural Core Online." << std::endl;

    // ---------------------------------------------------------
    // [Phase 1] Verification & Assimilation
    // ---------------------------------------------------------
    std::cout << "\n=== [Phase 1] Structural Verification & Assimilation ===\n" << std::endl;
    
    topologos::verification::BullshitDetector detector(rule_file);
    topologos::storage::KnowledgeGraph db(db_path);

    // [Data Collection for Compilation]
    std::vector<nlohmann::json> verified_facts_archive;

    if (fs::exists(inbox_path)) {
        for (const auto& entry : fs::directory_iterator(inbox_path)) {
            if (entry.path().extension() == ".json") {
                std::cout << ">> Inspecting: " << entry.path().filename() << "... ";
                try {
                    std::ifstream f(entry.path());
                    nlohmann::json data = nlohmann::json::parse(f);
                    
                    auto verdict = detector.judge(data);

                    if (verdict.is_truth) {
                        // [Vectorize] AI 엔진으로 임베딩 생성
                        std::vector<float> embedding;
                        std::string summary = data["attributes"].value("summary", "");
                        
                        if (!summary.empty()) {
                            // 텍스트를 벡터로 변환 (AI 수행)
                            embedding = engine_ptr->get_embedding(summary);
                        }

                        std::cout << "[TRUTH] (Score: " << verdict.integrity_score << ") -> Saving.\n";
                    
                        // [New] 컴파일을 위해 메모리에 보관
                        // 점수 정보 추가
                        data["score"] = verdict.integrity_score;
                        verified_facts_archive.push_back(data);

                        db.add_verified_node(data, verdict.integrity_score, embedding);
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
    // [Phase 3] The Graph-to-Logic Compilation
    // ---------------------------------------------------------
    std::cout << "\n=== [Phase 3] Compiling Knowledge to C++ Middleware ===\n";
    
    topologos::codegen::CppGenerator compiler(engine_ptr);
    
    // 1. 지식 컴파일 (JSON -> C++ Header)
    std::string cpp_code = compiler.transpile_knowledge(verified_facts_archive);
    
    // 2. 파일로 저장 (reality.hpp)
    std::string output_path = resolve_path("data/reality.hpp");
    std::ofstream out_file(output_path);
    if (out_file.is_open()) {
        out_file << cpp_code;
        out_file.close();
        std::cout << ">> [SUCCESS] Generated Immutable Truth: " << output_path << "\n";
        std::cout << ">> You can now include \"data/reality.hpp\" in your C++ projects.\n";
    } else {
        std::cerr << ">> [ERROR] Failed to write reality.hpp\n";
    }

    // ---------------------------------------------------------
    // [Phase 2] Visualization (기존 동일)
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
        topologos::codegen::CppGenerator generator(engine_ptr); // AI 엔진 주입
        generator.generate(program.get()); 

        std::cout << "\n=== 📊 Knowledge Graph (Mermaid) ===\n";
        std::cout << "graph TD;\n";
        std::cout << generator.get_mermaid(); 
        std::cout << "====================================\n" << std::endl;
    }

    return 0;
}