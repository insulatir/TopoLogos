#pragma once
#include <string>
#include <vector>
#include <memory>

namespace topologos::parser {

    // 기본 노드 타입
    struct Node {
        virtual ~Node() = default;
    };

    // "Concept Coding;" 같은 개념 정의
    struct ConceptNode : Node {
        std::string name;
        explicit ConceptNode(std::string n) : name(std::move(n)) {}
    };

    // "Coding -> Bug;" 같은 관계 정의
    struct RelationNode : Node {
        std::string source;
        std::string target;
        explicit RelationNode(std::string s, std::string t) : source(std::move(s)), target(std::move(t)) {}
    };

    // 전체 프로그램 (커뮤니티들의 집합)
    struct ProgramNode : Node {
        std::vector<std::unique_ptr<Node>> statements;
        // 관계들만 순회하기 위한 헬퍼
        std::vector<const RelationNode*> relations;
    };
}