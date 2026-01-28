// [Fix] 표준 Include 경로 사용
#include "topologos/verification/bullshit_detector.hpp"
#include "topologos/parser/lexer.hpp"
#include "topologos/parser/parser.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cmath>

using namespace std;
using namespace topologos::ast;
using json = nlohmann::json;

namespace topologos::verification {

    BullshitDetector::BullshitDetector(const string& rule_file_path) {
        load_rules(rule_file_path);
    }

    void BullshitDetector::load_rules(const string& filepath) {
        ifstream f(filepath);
        if (!f.is_open()) {
            cerr << "[Detector] Warning: Rule file not found (" << filepath << ")\n";
            return;
        }
        stringstream buffer;
        buffer << f.rdbuf();

        cout << "[Detector] Compiling Laws from: " << filepath << " ... " << flush; // [Fix] flush 추가
        
        try {
            parser::Lexer lexer(buffer.str());
            auto tokens = lexer.tokenize();
            parser::Parser parser(tokens);
            laws_ = parser.parse();

            if (laws_) cout << "Success." << endl;
            else cerr << "Failed (Parser returned null)." << endl;
        } catch (const exception& e) {
            cerr << "\n[Detector] Compile Error: " << e.what() << endl;
        }
    }

    void BullshitDetector::build_context(const json& data) {
        context_.clear();
        if (data.contains("attributes")) {
            for (auto& [domain_key, domain_val] : data["attributes"].items()) {
                if (domain_val.is_object()) {
                    for (auto& [key, val] : domain_val.items()) {
                        if (val.is_number()) context_[key] = RuntimeValue(val.get<double>());
                        else if (val.is_boolean()) context_[key] = RuntimeValue(val.get<bool>());
                        else if (val.is_string()) context_[key] = RuntimeValue(val.get<string>());
                        else if (val.is_array()) {
                            std::vector<std::string> vec;
                            for(const auto& elem : val) {
                                if(elem.is_string()) vec.push_back(elem.get<string>());
                            }
                            context_[key] = RuntimeValue(vec);
                        }
                    }
                }
            }
        }
    }

    Verdict BullshitDetector::judge(const json& data) {
        Verdict v;
        double score = 100.0;
        v.is_truth = false; // 기본값

        if (!laws_) {
            v.violations.push_back("System Error: No laws loaded.");
            v.integrity_score = 0.0;
            return v;
        }

        build_context(data);

        for (const auto& domain : laws_->domains) {
            for (const auto& axiom : domain->axioms) {
                for (const auto& rule : axiom->rules) {
                    // 상수 처리
                    for (const auto& constant : rule->constants) {
                        context_[constant.first] = evaluate(constant.second);
                    }
                    // 조건 평가
                    if (rule->condition) {
                        RuntimeValue result = evaluate(rule->condition);
                        if (!result.as_bool()) {
                            string msg = rule->failure_msg.empty() ? ("Rule: " + rule->name) : rule->failure_msg;
                            v.violations.push_back("[" + domain->name + "] " + msg);
                            score -= 20.0; 
                        }
                    }
                }
            }
        }

        v.integrity_score = max(0.0, score);
        v.is_truth = (v.integrity_score >= 50.0); // [Tuning] 50점 이상이면 일단 통과 (조정 가능)
        return v;
    }

    RuntimeValue BullshitDetector::evaluate(const shared_ptr<Expr>& expr) {
        if (!expr) return RuntimeValue(false);

        if (auto lit = dynamic_pointer_cast<Literal>(expr)) {
             if (lit->value == "true") return RuntimeValue(true);
             if (lit->value == "false") return RuntimeValue(false);
             try { return RuntimeValue(stod(lit->value)); } catch(...) {}
             return RuntimeValue(lit->value);
        }

        if (auto var = dynamic_pointer_cast<Variable>(expr)) {
             if (context_.find(var->name) != context_.end()) return context_[var->name];
             return RuntimeValue(0.0);
        }

        if (auto call = dynamic_pointer_cast<CallExpr>(expr)) {
            if (call->callee == "length" && !call->arguments.empty()) {
                RuntimeValue arg = evaluate(call->arguments[0]);
                if (arg.type == RuntimeValue::LIST) return RuntimeValue((double)arg.list_val.size());
                if (arg.type == RuntimeValue::STRING) return RuntimeValue((double)arg.str_val.length());
                return RuntimeValue(0.0);
            }
        }

        if (auto bin = dynamic_pointer_cast<BinaryExpr>(expr)) {
             RuntimeValue left = evaluate(bin->left);
             RuntimeValue right = evaluate(bin->right);
             
             if (bin->op == "+") return RuntimeValue(left.num_val + right.num_val);
             if (bin->op == "-") return RuntimeValue(left.num_val - right.num_val);
             if (bin->op == "*") return RuntimeValue(left.num_val * right.num_val);
             if (bin->op == "/") return RuntimeValue(right.num_val == 0 ? 0 : left.num_val / right.num_val);
             if (bin->op == "<") return RuntimeValue(left.num_val < right.num_val);
             if (bin->op == "<=") return RuntimeValue(left.num_val <= right.num_val);
             if (bin->op == ">") return RuntimeValue(left.num_val > right.num_val);
             if (bin->op == ">=") return RuntimeValue(left.num_val >= right.num_val);
             if (bin->op == "==") return RuntimeValue(abs(left.num_val - right.num_val) < 1e-9);
             if (bin->op == "&&") return RuntimeValue(left.as_bool() && right.as_bool());
             if (bin->op == "||") return RuntimeValue(left.as_bool() || right.as_bool());
        }

        return RuntimeValue(false);
    }
}