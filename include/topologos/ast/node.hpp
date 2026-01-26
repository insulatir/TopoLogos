#pragma once
#include <string>
#include <vector>
#include <memory>
#include <iostream>

namespace topologos::ast {

    // 모든 노드의 부모
    struct Node {
        virtual ~Node() = default;
        // 디버깅용: 자신의 정보를 출력하는 함수
        virtual void print(int indent = 0) const = 0;
        
        void print_indent(int indent) const {
            for(int i=0; i<indent; ++i) std::cout << "  ";
        }
    };

    // 1. 개념 (Concept): "CPU", "Salary" 등
    struct Concept : Node {
        std::string name;

        Concept(std::string n) : name(std::move(n)) {}

        void print(int indent = 0) const override {
            print_indent(indent);
            std::cout << "[Concept] " << name << "\n";
        }
    };

    // 2. 관계 (Relation): A -> B 또는 A ~> B
    enum class Strength { STRONG, WEAK };

    struct Relation : Node {
        std::string source;
        std::string target;
        Strength strength;

        Relation(std::string s, std::string t, Strength str)
            : source(std::move(s)), target(std::move(t)), strength(str) {}

        void print(int indent = 0) const override {
            print_indent(indent);
            std::cout << "[Relation] " << source 
                      << (strength == Strength::STRONG ? " -> " : " ~> ")
                      << target << "\n";
        }
    };

    // 3. 커뮤니티 (Community): 개념과 관계의 집합
    struct Community : Node {
        std::string name;
        std::vector<std::shared_ptr<Concept>> concepts;
        std::vector<std::shared_ptr<Relation>> relations;

        Community(std::string n) : name(std::move(n)) {}

        void print(int indent = 0) const override {
            print_indent(indent);
            std::cout << "[Community] " << name << " {\n";
            for (const auto& c : concepts) c->print(indent + 1);
            for (const auto& r : relations) r->print(indent + 1);
            print_indent(indent);
            std::cout << "}\n";
        }
    };

    // 4. 전체 프로그램 (루트 노드)
    struct Program : Node {
        std::vector<std::shared_ptr<Node>> statements; // Community 또는 Relation

        void print(int indent = 0) const override {
            std::cout << "=== AST Structure ===\n";
            for (const auto& stmt : statements) stmt->print(indent);
        }
    };
}