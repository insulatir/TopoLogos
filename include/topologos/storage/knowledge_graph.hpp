#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <sqlite3.h> // [변경] JSON 대신 SQLite
#include <nlohmann/json.hpp>

namespace topologos::storage {

    using json = nlohmann::json;

    class KnowledgeGraph {
    public:
        explicit KnowledgeGraph(const std::string& db_path) : db_path_(db_path) {
            open_db();
            init_schema();
        }

        ~KnowledgeGraph() {
            if (db_) sqlite3_close(db_);
        }

        void add_verified_node(const json& data, double score) {
            std::string id = data["meta"]["source_id"];
            std::string type = "VerifiedFact";
            std::string payload = data.dump(); 

            // 1. Node 저장 (Upsert)
            std::string sql_node = "INSERT OR REPLACE INTO nodes (id, type, score, payload) VALUES (?, ?, ?, ?);";
            execute_prepared(sql_node, {id, type, std::to_string(score), payload});

            // 2. 기존 Edge 삭제 (갱신용)
            std::string sql_del = "DELETE FROM edges WHERE source = ?;";
            execute_prepared(sql_del, {id});

            // 3. Edge 저장
            if (data["attributes"]["structure"].contains("dependency_sources")) {
                auto sources = data["attributes"]["structure"]["dependency_sources"];
                std::string sql_edge = "INSERT INTO edges (source, target, relation) VALUES (?, ?, ?);";
                for (const auto& source : sources) {
                    execute_prepared(sql_edge, {id, source.get<std::string>(), "DEPENDS_ON"});
                }
            }
            std::cout << "[DB] Assimilated: " << id << " (Stored in SQLite)" << std::endl;
        }

        // save() 함수는 삭제됨 (SQLite는 자동 저장)

    private:
        std::string db_path_;
        sqlite3* db_ = nullptr;

        void open_db() {
            sqlite3_open(db_path_.c_str(), &db_);
        }

        void init_schema() {
            const char* sql = 
                "CREATE TABLE IF NOT EXISTS nodes (id TEXT PRIMARY KEY, type TEXT, score REAL, payload TEXT, created_at DATETIME DEFAULT CURRENT_TIMESTAMP);"
                "CREATE TABLE IF NOT EXISTS edges (id INTEGER PRIMARY KEY, source TEXT, target TEXT, relation TEXT, created_at DATETIME DEFAULT CURRENT_TIMESTAMP);";
            char* err = 0;
            sqlite3_exec(db_, sql, 0, 0, &err);
        }

        void execute_prepared(const std::string& sql, const std::vector<std::string>& params) {
            sqlite3_stmt* stmt;
            sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, 0);
            for (size_t i = 0; i < params.size(); i++) {
                sqlite3_bind_text(stmt, i + 1, params[i].c_str(), -1, SQLITE_TRANSIENT);
            }
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
    };
}