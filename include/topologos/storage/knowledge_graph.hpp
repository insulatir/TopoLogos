#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <sqlite3.h>
#include <nlohmann/json.hpp>

// [New] HTTP Client
#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "httplib.h"

namespace topologos::storage {

    using json = nlohmann::json;

    class KnowledgeGraph {
    public:
        explicit KnowledgeGraph(const std::string& db_path, const std::string& qdrant_host = "qdrant", int qdrant_port = 6333) 
            : db_path_(db_path), qdrant_client_(qdrant_host, qdrant_port) {
            open_sqlite();
            init_sqlite_schema();
            init_qdrant_collection();
        }

        ~KnowledgeGraph() {
            if (db_) sqlite3_close(db_);
        }

        void add_verified_node(const json& data, double score, const std::vector<float>& embedding) {
            std::string id = data["meta"]["source_id"];
            
            // 1. [SQLite] 그래프 관계 저장 (기존 로직 유지)
            save_graph_structure(data, id);

            // 2. [Qdrant] 벡터 및 메타데이터 저장
            if (!embedding.empty()) {
                save_vector(id, embedding, data, score);
            }
            
            std::cout << "[DB] Assimilated: " << id << " (SQLite Graph + Qdrant Vector)" << std::endl;
        }

    private:
        std::string db_path_;
        sqlite3* db_ = nullptr;
        httplib::Client qdrant_client_;
        const std::string COLLECTION_NAME = "topologos_knowledge";
        const int VECTOR_SIZE = 768; // BERT base size

        // --- SQLite Part ---
        void open_sqlite() { sqlite3_open(db_path_.c_str(), &db_); }
        
        void init_sqlite_schema() {
            // Edge 테이블만 남겨도 되지만, 호환성을 위해 Node 테이블도 유지 (Payload는 중복 저장 선택 사항)
            const char* sql = 
                "CREATE TABLE IF NOT EXISTS edges (id INTEGER PRIMARY KEY, source TEXT, target TEXT, relation TEXT);";
            char* err = 0;
            sqlite3_exec(db_, sql, 0, 0, &err);
        }

        void save_graph_structure(const json& data, const std::string& id) {
            // 기존 Edge 삭제 및 재생성
            execute_sql("DELETE FROM edges WHERE source = '" + id + "';");
            
            if (data["attributes"]["structure"].contains("dependency_sources")) {
                auto sources = data["attributes"]["structure"]["dependency_sources"];
                for (const auto& source : sources) {
                    std::string src_str = source.get<std::string>();
                    // SQL Injection 방지는 생략됨 (간단 예시), 실제론 Prepared Statement 사용 필수
                    std::string sql = "INSERT INTO edges (source, target, relation) VALUES ('" + id + "', '" + src_str + "', 'DEPENDS_ON');";
                    execute_sql(sql);
                }
            }
        }

        void execute_sql(const std::string& sql) {
            char* err = 0;
            sqlite3_exec(db_, sql.c_str(), 0, 0, &err);
            if(err) sqlite3_free(err);
        }

        // --- Qdrant Part ---
        void init_qdrant_collection() {
            // 컬렉션 확인 및 생성 (Upsert 방식)
            // Qdrant REST API: PUT /collections/{name}
            json payload = {
                {"vectors", {
                    {"size", VECTOR_SIZE},
                    {"distance", "Cosine"}
                }}
            };
            qdrant_client_.Put("/collections/" + COLLECTION_NAME, payload.dump(), "application/json");
        }

        void save_vector(const std::string& id, const std::vector<float>& embedding, const json& data, double score) {
            // Qdrant Point 구조 생성
            // ID는 정수나 UUID여야 함. 여기서는 문자열 ID를 해싱하여 사용하거나 UUID 생성 필요.
            // 편의상 문자열 ID의 Hash를 사용하거나 Qdrant의 UUID 지원 기능 활용.
            
            // 해시 생성 (임시)
            std::hash<std::string> hasher;
            uint64_t point_id = hasher(id); 

            json point = {
                {"id", point_id},
                {"vector", embedding},
                {"payload", {
                    {"original_id", id},
                    {"summary", data["attributes"]["summary"]},
                    {"score", score},
                    {"timestamp", data["meta"]["timestamp"]}
                }}
            };

            json batch = {
                {"points", {point}}
            };

            // Upsert Points: PUT /collections/{name}/points
            auto res = qdrant_client_.Put("/collections/" + COLLECTION_NAME + "/points?wait=true", batch.dump(), "application/json");
            
            if (!res || res->status != 200) {
                std::cerr << "[Qdrant Error] Failed to upsert vector: " << (res ? res->body : "Connection failed") << std::endl;
            }
        }
    };
}