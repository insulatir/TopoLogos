#pragma once
#include <string>
#include <vector>
#include <sstream>
// node.hpp 안에 이미 ProgramNode 등의 using 선언이 포함되어 있으므로 이것만 있으면 됩니다.
#include "topologos/ast/node.hpp" 
#include "topologos/ai/engine.hpp"
#include <nlohmann/json.hpp> 

namespace topologos::codegen {

    class CppGenerator {
    public:
        explicit CppGenerator(topologos::ai::NLIEngine* engine = nullptr) : engine_(engine) {}

        // [Public API]
        // 1. AST를 순회하며 Mermaid 그래프 생성 (내부 버퍼에 저장)
        void generate(const topologos::ast::Node* root);
        
        // 2. 생성된 Mermaid 문자열 반환
        std::string get_mermaid() const { return mermaid_ss_.str(); }

        // 3. 검증된 지식을 C++ 헤더로 변환
        std::string transpile_knowledge(const std::vector<nlohmann::json>& verified_facts);

    private:
        topologos::ai::NLIEngine* engine_;
        std::stringstream mermaid_ss_;

        // [Internal Visitors] 
        // node.hpp를 포함했으므로 ProgramNode 등의 타입을 바로 사용할 수 있습니다.
        void visit_program(const topologos::ast::ProgramNode* node);
        void visit_domain(const topologos::ast::DomainNode* node);
        void visit_axiom(const topologos::ast::AxiomNode* node);
        void visit_rule(const topologos::ast::RuleNode* node);
        
        // Helper
        std::string sanitize_name(const std::string& raw);
    };

}