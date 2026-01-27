// include/topologos/storage/knowledge_graph.hpp
#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

namespace topologos::storage {

    using json = nlohmann::json;

    class KnowledgeGraph {
    public:
        explicit KnowledgeGraph(const std::string& db_path) : db_path_(db_path) {
            load();
        }

        // include/topologos/storage/knowledge_graph.hpp

        void add_verified_node(const json& data, double score) {
            std::string id = data["meta"]["source_id"];
            
            // 1. 노드 갱신 (덮어쓰기)
            json node = {
                {"id", id},
                {"score", score},
                {"timestamp", data["meta"]["timestamp"]},
                {"type", "VerifiedFact"},
                {"attributes", data["attributes"]}
            };
            db_["nodes"][id] = node;

            // 2. [FIX] 엣지 중복 방지 로직
            // 기존 엣지 목록에서 "현재 처리 중인 ID"가 출발지(from)인 엣지를 모두 제거한 새 목록을 만듭니다.
            json new_edges = json::array();
            if (db_.contains("edges") && db_["edges"].is_array()) {
                for (const auto& edge : db_["edges"]) {
                    // 'from'이 현재 id와 다른 것들만 유지 (즉, 내 엣지는 다 지움)
                    if (edge["from"] != id) {
                        new_edges.push_back(edge);
                    }
                }
            }

            // 3. 새로운 엣지 추가
            auto sources = data["attributes"]["structure"]["dependency_sources"];
            for (const auto& source : sources) {
                json edge = {
                    {"from", id},
                    {"to", source},
                    {"relation", "DEPENDS_ON"}
                };
                new_edges.push_back(edge);
            }
            
            // 엣지 목록 교체
            db_["edges"] = new_edges;
            
            std::cout << "[DB] Assimilated: " << id << " (Edges Updated)" << std::endl;
        }

        // 디스크에 저장
        void save() {
            std::ofstream f(db_path_);
            if (f.is_open()) {
                f << db_.dump(4); // 4칸 들여쓰기 (Pretty Print)
                std::cout << "[DB] Knowledge persisted to '" << db_path_ << "'" << std::endl;
            }
        }

    private:
        std::string db_path_;
        json db_;

        void load() {
            std::ifstream f(db_path_);
            if (f.good()) {
                try {
                    db_ = json::parse(f);
                } catch (...) {
                    db_ = json::object(); // 파일이 깨졌거나 비었으면 초기화
                    db_["nodes"] = json::object();
                    db_["edges"] = json::array();
                }
            } else {
                // 초기 DB 구조
                db_["nodes"] = json::object();
                db_["edges"] = json::array();
            }
        }
    };

} // namespace topologos::storage