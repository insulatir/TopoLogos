#pragma once
#include <string>
#include <sstream>
#include "../parser/ast.hpp" // AST 정의
#include "../ai/engine.hpp"  // AI 엔진

namespace topologos::codegen {

    class CppGenerator {
    public:
        // AI 엔진 주입
        explicit CppGenerator(ai::NLIEngine* engine = nullptr) : engine_(engine) {}

        // 핵심 함수: ProgramNode를 받아서 C++ 코드 문자열 반환
        std::string generate(const parser::ProgramNode* program);

        // Mermaid 시각화 코드 반환
        std::string get_mermaid() const { return mermaid_ss_.str(); }

    private:
        ai::NLIEngine* engine_;
        
        std::stringstream ss_;          // C++ 코드 저장용
        std::stringstream mermaid_ss_;  // 그래프 코드 저장용
        std::vector<std::string> registration_lines_;

        int indent_level_ = 0;

        // 헬퍼 함수들
        void emit_line(const std::string& line);
        void emit_indent();
        
        // 관계 하나를 처리하는 함수 (가장 중요)
        void visit_relation(const parser::RelationNode* relation);
    };
}