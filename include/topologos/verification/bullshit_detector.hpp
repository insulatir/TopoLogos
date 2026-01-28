#pragma once
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <memory>
#include <map>

// [Fix] node.hpp 직접 포함
#include "../ast/node.hpp"

namespace topologos::verification {

    struct Verdict {
        bool is_truth;
        double integrity_score;
        std::vector<std::string> violations;
    };

    struct RuntimeValue {
        enum Type { BOOL, NUMBER, STRING, LIST } type;
        bool bool_val = false;
        double num_val = 0.0;
        std::string str_val;
        std::vector<std::string> list_val;

        RuntimeValue() : type(BOOL) {}
        RuntimeValue(bool v) : type(BOOL), bool_val(v) {}
        RuntimeValue(double v) : type(NUMBER), num_val(v) {}
        RuntimeValue(std::string v) : type(STRING), str_val(v) {}
        RuntimeValue(std::vector<std::string> v) : type(LIST), list_val(v) {}

        bool as_bool() const {
            if (type == BOOL) return bool_val;
            if (type == NUMBER) return num_val != 0.0;
            return !str_val.empty();
        }
    };

    class BullshitDetector {
    public:
        explicit BullshitDetector(const std::string& rule_file_path);
        
        Verdict judge(const nlohmann::json& data);

    private:
        void load_rules(const std::string& filepath);
        void build_context(const nlohmann::json& data);
        RuntimeValue evaluate(const std::shared_ptr<topologos::ast::Expr>& expr);

        std::shared_ptr<topologos::ast::Program> laws_;
        std::map<std::string, RuntimeValue> context_;
    };
}