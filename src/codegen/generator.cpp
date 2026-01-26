#include "topologos/codegen/generator.hpp"
#include <iostream>
#include <format>
#include <set>

namespace topologos::codegen {

    std::string CppGenerator::generate(const parser::ProgramNode* program) {
        ss_.str("");
        mermaid_ss_.str("");
        registration_lines_.clear(); // 초기화
        std::set<std::string> defined_concepts;

        emit_line("#pragma once");
        emit_line("#include <expected>");
        emit_line("#include <string>");
        emit_line("#include <vector>");
        emit_line("#include <map>");
        emit_line("");

        emit_line("namespace TopoLogosKnowledge {");
        indent_level_++;

        if (program) {
            // 1. 구조체 정의
            for (const auto* relation : program->relations) {
                if (defined_concepts.find(relation->source) == defined_concepts.end()) {
                    emit_line(std::format("struct {} {{}};", relation->source));
                    defined_concepts.insert(relation->source);
                }
                if (defined_concepts.find(relation->target) == defined_concepts.end()) {
                    emit_line(std::format("struct {} {{}};", relation->target));
                    defined_concepts.insert(relation->target);
                }
            }
            emit_line("");

            // 2. 함수 정의 및 등록 정보 수집
            for (const auto* relation : program->relations) {
                visit_relation(relation);
            }

            // 3. 런타임용 그래프 데이터 생성 (Dynamic Graph)
            emit_line("");
            emit_line("// Adjacency List for Dynamic Search");
            emit_line("inline std::map<std::string, std::vector<std::string>> get_graph() {");
            emit_line("    std::map<std::string, std::vector<std::string>> g;");
            
            // 수집된 등록 코드(push_back) 출력
            for (const auto& line : registration_lines_) {
                emit_line(line);
            }
            
            emit_line("    return g;");
            emit_line("}");
        }

        indent_level_--;
        emit_line("} // namespace");
        
        return ss_.str();
    }

    void CppGenerator::visit_relation(const parser::RelationNode* relation) {
        bool is_strong = false;
        
        if (engine_) {
            std::string premise = std::format("This is a {}", relation->source);
            std::string hypothesis = std::format("This is a {}", relation->target);
            std::cout << "[Debug] AI Checking: " << premise << " -> " << hypothesis << std::endl;
            if (engine_->predict(premise, hypothesis) == ai::LogicResult::ENTAILMENT) {
                is_strong = true;
            }
        }

        // 함수 생성 (기존 로직)
        std::string func_name = std::format("relation_{}_to_{}", relation->source, relation->target);
        if (is_strong) {
            emit_line(std::format("inline auto {}({} input) -> {} {{ return {}(); }}", 
                func_name, relation->source, relation->target, relation->target));
        } else {
            emit_line(std::format("inline auto {}({} input) -> std::expected<{}, std::string> {{ return {}(); }}", 
                func_name, relation->source, relation->target, relation->target));
        }

        // [New] 등록 코드 수집 (Runtime Map 생성용)
        // g["Coding"].push_back("Bug"); 형태
        registration_lines_.push_back(std::format("    g[\"{}\"].push_back(\"{}\");", relation->source, relation->target));

        // Mermaid
        std::string arrow = is_strong ? "==>" : "-.->";
        std::string label = is_strong ? "Strong" : "Weak";
        mermaid_ss_ << "    " << relation->source << " " << arrow << "|" << label << "| " << relation->target << "\n";
        if (is_strong) mermaid_ss_ << "    style " << relation->target << " stroke:#f00,stroke-width:2px\n";
    }

    void CppGenerator::emit_indent() { for (int i = 0; i < indent_level_; ++i) ss_ << "    "; }
    void CppGenerator::emit_line(const std::string& line) { emit_indent(); ss_ << line << "\n"; }
}