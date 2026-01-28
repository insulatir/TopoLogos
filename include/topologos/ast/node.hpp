#pragma once
#include <string>
#include <vector>
#include <memory>
#include <iostream>

namespace topologos::ast {

    // 기본 노드
    struct Node {
        virtual ~Node() = default;
        virtual void print(int indent = 0) const = 0;
    };

    // --- Expressions ---
    struct Expr : Node {};

    struct Literal : Expr {
        std::string value;
        Literal(std::string v) : value(std::move(v)) {}
        void print(int indent = 0) const override { std::cout << value; }
    };

    struct Variable : Expr {
        std::string name;
        Variable(std::string n) : name(std::move(n)) {}
        void print(int indent = 0) const override { std::cout << name; }
    };

    struct CallExpr : Expr {
        std::string callee;
        std::vector<std::shared_ptr<Expr>> arguments;
        CallExpr(std::string c, std::vector<std::shared_ptr<Expr>> args)
            : callee(std::move(c)), arguments(std::move(args)) {}
        void print(int indent = 0) const override { std::cout << "call:" << callee; }
    };

    struct BinaryExpr : Expr {
        std::shared_ptr<Expr> left;
        std::string op;
        std::shared_ptr<Expr> right;
        BinaryExpr(std::shared_ptr<Expr> l, std::string o, std::shared_ptr<Expr> r)
            : left(std::move(l)), op(std::move(o)), right(std::move(r)) {}
        void print(int indent = 0) const override { std::cout << "binary:" << op; }
    };

    // --- Structure Nodes ---
    // [호환성] BullshitDetector가 찾는 구조체들
    struct Rule : Node {
        std::string name;
        std::vector<std::pair<std::string, std::string>> params;
        std::vector<std::pair<std::string, std::shared_ptr<Expr>>> constants;
        std::shared_ptr<Expr> condition;
        std::string failure_msg;

        Rule(std::string n) : name(std::move(n)) {}
        void print(int indent = 0) const override { std::cout << "Rule " << name << "\n"; }
    };
    using RuleNode = Rule; 

    struct Axiom : Node {
        std::string name;
        std::vector<std::shared_ptr<Rule>> rules;
        Axiom(std::string n) : name(std::move(n)) {}
        void print(int indent = 0) const override { std::cout << "Axiom " << name << "\n"; }
    };
    using AxiomNode = Axiom;

    struct Domain : Node {
        std::string name;
        std::vector<std::shared_ptr<Axiom>> axioms;
        Domain(std::string n) : name(std::move(n)) {}
        void print(int indent = 0) const override { std::cout << "Domain " << name << "\n"; }
    };
    using DomainNode = Domain;

    struct Program : Node {
        std::vector<std::shared_ptr<Domain>> domains;
        void print(int indent = 0) const override { std::cout << "Program\n"; }
    };
    using ProgramNode = Program;

} // namespace topologos::ast