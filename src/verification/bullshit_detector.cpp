#include "../../include/topologos/verification/bullshit_detector.hpp"
#include "../../include/topologos/parser/lexer.hpp"
#include "../../include/topologos/parser/parser.hpp"
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
            cerr << "[Detector] Warning: Rule file not found (" << filepath << "). Engine is empty." << endl;
            return;
        }

        stringstream buffer;
        buffer << f.rdbuf();

        cout << "[Detector] Compiling Laws from: " << filepath << " ... ";
        
        try {
            // 1. Lexing
            parser::Lexer lexer(buffer.str());
            auto tokens = lexer.tokenize();

            // 2. Parsing
            parser::Parser parser(tokens);
            laws_ = parser.parse();

            if (laws_) {
                cout << "Success." << endl;
            } else {
                cerr << "Failed (Parser returned null)." << endl;
            }
        } catch (const exception& e) {
            cerr << "\n[Detector] Compile Error: " << e.what() << endl;
        }
    }

    // [Update] build_context: JSON Array를 vector<string>으로 변환
    void BullshitDetector::build_context(const json& data) {
        context_.clear();
        if (data.contains("attributes")) {
            for (auto& [domain_key, domain_val] : data["attributes"].items()) {
                if (domain_val.is_object()) {
                    for (auto& [key, val] : domain_val.items()) {
                        if (val.is_number()) context_[key] = RuntimeValue(val.get<double>());
                        else if (val.is_boolean()) context_[key] = RuntimeValue(val.get<bool>());
                        else if (val.is_string()) context_[key] = RuntimeValue(val.get<string>());
                        
                        // [New] 리스트 지원
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
        v.violations.clear();
        double score = 100.0;

        if (!laws_) {
            v.violations.push_back("System Error: No laws loaded.");
            v.is_truth = false;
            v.integrity_score = 0.0;
            return v;
        }

        // 1. 현재 증거 데이터로 컨텍스트 구축
        build_context(data);

        // 2. AST 순회하며 규칙 검사
        for (const auto& domain : laws_->domains) {
            for (const auto& axiom : domain->axioms) {
                for (const auto& rule : axiom->rules) {
                    
                    // (1) 로컬 상수/변수 처리 (threshold 등)
                    // 기존 컨텍스트를 백업하고 복원하는 스코프 처리가 정석이지만,
                    // 여기선 간단히 덮어쓰고 나중에 복구하지 않음 (전역 유니크 가정)
                    for (const auto& constant : rule->constants) {
                        string name = constant.first;
                        RuntimeValue val = evaluate(constant.second);
                        context_[name] = val;
                    }

                    // (2) 조건(Condition) 평가
                    if (rule->condition) {
                        RuntimeValue result = evaluate(rule->condition);
                        
                        // 조건이 '거짓(False)'이면 위반!
                        // life.topo에서는 "지켜야 할 조건"을 명시하므로, false가 나오면 위반임.
                        if (!result.as_bool()) {
                            string msg = rule->failure_msg.empty() 
                                ? ("Rule Violated: " + rule->name) 
                                : rule->failure_msg;
                            
                            // 도메인 태그 추가
                            msg = "[" + domain->name + "] " + msg; // 이미 대문자일수 있음
                            
                            v.violations.push_back(msg);
                            
                            // 점수 차감 (단순화: 규칙 하나당 20점 감점)
                            // 나중에 life.topo에 weight 속성을 추가하여 동적으로 할 수도 있음
                            score -= 20.0; 
                        }
                    }
                }
            }
        }

        v.integrity_score = max(0.0, score);
        v.is_truth = (v.integrity_score >= 80.0);
        return v;
    }

    // [Update] evaluate: CallExpr(함수 호출) 지원
    RuntimeValue BullshitDetector::evaluate(const shared_ptr<Expr>& expr) {
        if (!expr) return RuntimeValue(false);

        // 1. 리터럴
        if (auto lit = dynamic_pointer_cast<Literal>(expr)) {
             // ... 기존과 동일 ...
             if (lit->value == "true") return RuntimeValue(true);
             if (lit->value == "false") return RuntimeValue(false);
             try { return RuntimeValue(stod(lit->value)); } catch(...) {}
             return RuntimeValue(lit->value);
        }

        // 2. 변수
        if (auto var = dynamic_pointer_cast<Variable>(expr)) {
             if (context_.find(var->name) != context_.end()) return context_[var->name];
             return RuntimeValue(0.0);
        }

        // 3. [New] 함수 호출 (length)
        if (auto call = dynamic_pointer_cast<CallExpr>(expr)) {
            if (call->callee == "length" && !call->arguments.empty()) {
                RuntimeValue arg = evaluate(call->arguments[0]);
                if (arg.type == RuntimeValue::LIST) return RuntimeValue((double)arg.list_val.size());
                if (arg.type == RuntimeValue::STRING) return RuntimeValue((double)arg.str_val.length());
                return RuntimeValue(0.0);
            }
            // 다른 함수 추가 가능
        }

        // 4. 이항 연산
        if (auto bin = dynamic_pointer_cast<BinaryExpr>(expr)) {
             // ... 기존과 동일 ...
             RuntimeValue left = evaluate(bin->left);
             RuntimeValue right = evaluate(bin->right);
             // (이항 연산 로직은 이전 코드 복사해서 그대로 유지해주세요)
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

} // namespace topologos::verification