#pragma once
#include <string>
#include <sstream>
#include "../ast/node.hpp" // [중요] 새 AST 헤더
#include "../ai/engine.hpp"

namespace topologos::codegen {

    class CppGenerator {
    public:
        explicit CppGenerator(ai::NLIEngine* engine = nullptr) : engine_(engine) {}

        // [수정] 인자 타입을 ast::ProgramNode* 로 변경
        std::string generate(const topologos::ast::ProgramNode* program);
        
        std::string get_mermaid() const { return mermaid_buffer_.str(); }

    private:
        ai::NLIEngine* engine_;
        std::stringstream output_;
        std::stringstream mermaid_buffer_;

        // [수정] 새 노드 타입에 맞는 방문 함수들
        void visit_domain(const topologos::ast::DomainNode* node);
        void visit_axiom(const topologos::ast::AxiomNode* node);
        void visit_rule(const topologos::ast::RuleNode* node);
    };

} // namespace topologos::codegen