#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <sqlite3.h>
#include <nlohmann/json.hpp>

#ifndef CPPHTTPLIB_OPENSSL_SUPPORT
#define CPPHTTPLIB_OPENSSL_SUPPORT
#endif
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

        void add_verified_node(const json& data, double score, const std::vector<float>& embedding = {}) {
            std::string id = data["meta"]["source_id"];
            
            // 1. [Fix] 대시보드용 Node 메타데이터 저장 (SQLite)
            save_node_sqlite(data, score);

            // 2. 그래프 관계 저장 (SQLite)
            save_graph_structure(data, id);
            
            // 3. 의미론적 벡터 저장 (Qdrant)
            if (!embedding.empty()) {
                save_vector(id, embedding, data, score);
            }
            
            std::cout << "[DB] Assimilated: " << id << std::endl;
        }

    private:
        std::string db_path_;
        sqlite3* db_ = nullptr;
        httplib::Client qdrant_client_;
        const std::string COLLECTION_NAME = "topologos_knowledge";
        const int VECTOR_SIZE = 768;

        void open_sqlite() { sqlite3_open(db_path_.c_str(), &db_); }
        
        void init_sqlite_schema() {
            // [Fix] nodes 테이블 다시 생성 (dashboard.py 호환성)
            const char* sql = 
                "CREATE TABLE IF NOT EXISTS nodes (id TEXT PRIMARY KEY, type TEXT, score REAL, payload TEXT);"
                "CREATE TABLE IF NOT EXISTS edges (id INTEGER PRIMARY KEY, source TEXT, target TEXT, relation TEXT);";
            char* err = 0;
            sqlite3_exec(db_, sql, 0, 0, &err);
            if(err) sqlite3_free(err);
        }

        // [New] Node 저장 헬퍼 함수
        void save_node_sqlite(const json& data, double score) {
            std::string id = data["meta"]["source_id"];
            std::string type = "VerifiedFact";
            std::string payload = data.dump();
            
            const char* sql = "INSERT OR REPLACE INTO nodes (id, type, score, payload) VALUES (?, ?, ?, ?);";
            sqlite3_stmt* stmt;
            if (sqlite3_prepare_v2(db_, sql, -1, &stmt, 0) == SQLITE_OK) {
                sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_text(stmt, 2, type.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_double(stmt, 3, score);
                sqlite3_bind_text(stmt, 4, payload.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_step(stmt);
                sqlite3_finalize(stmt);
            }
        }

        void save_graph_structure(const json& data, const std::string& id) {
            char* err = 0;
            std::string del_sql = "DELETE FROM edges WHERE source = '" + id + "';";
            sqlite3_exec(db_, del_sql.c_str(), 0, 0, &err);
            if(err) sqlite3_free(err);

            if (data["attributes"]["structure"].contains("dependency_sources")) {
                for (const auto& source : data["attributes"]["structure"]["dependency_sources"]) {
                    std::string src_str = source.get<std::string>();
                    std::string sql = "INSERT INTO edges (source, target, relation) VALUES ('" + id + "', '" + src_str + "', 'DEPENDS_ON');";
                    sqlite3_exec(db_, sql.c_str(), 0, 0, &err);
                    if(err) sqlite3_free(err);
                }
            }
        }

        void init_qdrant_collection() {
            json payload = {{"vectors", {{"size", VECTOR_SIZE}, {"distance", "Cosine"}}}};
            qdrant_client_.Put("/collections/" + COLLECTION_NAME, payload.dump(), "application/json");
        }

        void save_vector(const std::string& id, const std::vector<float>& embedding, const json& data, double score) {
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
            json batch = {{"points", {point}}};
            qdrant_client_.Put("/collections/" + COLLECTION_NAME + "/points?wait=true", batch.dump(), "application/json");
        }
    };
}