#pragma once
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <nlohmann/json.hpp>
#include "../ast/node.hpp"

namespace topologos::verification {

    // [Update] LIST 타입 지원 추가
    struct RuntimeValue {
        enum Type { NUMBER, STRING, BOOLEAN, LIST } type;
        double num_val = 0.0;
        std::string str_val;
        std::vector<std::string> list_val; // 리스트 저장용

        RuntimeValue() : type(NUMBER), num_val(0.0) {}
        RuntimeValue(double v) : type(NUMBER), num_val(v) {}
        RuntimeValue(bool v) : type(BOOLEAN), num_val(v ? 1.0 : 0.0) {}
        RuntimeValue(std::string v) : type(STRING), str_val(std::move(v)) {}
        // [New] 리스트 생성자
        RuntimeValue(std::vector<std::string> v) : type(LIST), list_val(std::move(v)) {}

        bool as_bool() const {
            if (type == NUMBER || type == BOOLEAN) return num_val != 0.0;
            if (type == LIST) return !list_val.empty();
            return !str_val.empty();
        }
    };

    struct Verdict {
        bool is_truth;
        double integrity_score;
        std::vector<std::string> violations;
    };

    class BullshitDetector {
    public:
        explicit BullshitDetector(const std::string& rule_file_path);
        Verdict judge(const nlohmann::json& evidence_json);

    private:
        std::shared_ptr<topologos::ast::ProgramNode> laws_;
        void load_rules(const std::string& filepath);
        std::map<std::string, RuntimeValue> context_;
        
        RuntimeValue evaluate(const std::shared_ptr<topologos::ast::Expr>& expr);
        void build_context(const nlohmann::json& data);
    };

}