#pragma once
#include <string>
#include <vector>
#include <memory>
#include <iostream>

namespace topologos::ast {

    struct Node {
        virtual ~Node() = default;
        virtual void print(int indent = 0) const = 0;
        void print_indent(int indent) const {
            for(int i=0; i<indent; ++i) std::cout << "  ";
        }
    };

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

    // [New] 함수 호출 (length(...) 등)
    struct CallExpr : Expr {
        std::string callee;
        std::vector<std::shared_ptr<Expr>> arguments;
        CallExpr(std::string c, std::vector<std::shared_ptr<Expr>> args)
            : callee(std::move(c)), arguments(std::move(args)) {}
        void print(int indent = 0) const override {
             std::cout << callee << "("; 
             for(size_t i=0; i<arguments.size(); ++i) {
                 arguments[i]->print();
                 if(i < arguments.size()-1) std::cout << ", ";
             }
             std::cout << ")";
        }
    };

    struct BinaryExpr : Expr {
        std::shared_ptr<Expr> left;
        std::string op;
        std::shared_ptr<Expr> right;
        BinaryExpr(std::shared_ptr<Expr> l, std::string o, std::shared_ptr<Expr> r)
            : left(std::move(l)), op(std::move(o)), right(std::move(r)) {}
        void print(int indent = 0) const override {
            std::cout << "("; left->print(); std::cout << " " << op << " "; right->print(); std::cout << ")";
        }
    };

    struct RuleNode : Node {
        std::string name;
        std::vector<std::pair<std::string, std::string>> params;
        std::vector<std::pair<std::string, std::shared_ptr<Expr>>> constants;
        std::shared_ptr<Expr> condition;
        std::string failure_msg;

        RuleNode(std::string n) : name(std::move(n)) {}
        void print(int indent = 0) const override { /* 생략 (기존 유지) */ }
    };

    struct AxiomNode : Node {
        std::string name;
        std::vector<std::shared_ptr<RuleNode>> rules;
        AxiomNode(std::string n) : name(std::move(n)) {}
        void print(int indent = 0) const override { /* 생략 */ }
    };

    struct DomainNode : Node {
        std::string name;
        std::vector<std::shared_ptr<AxiomNode>> axioms;
        DomainNode(std::string n) : name(std::move(n)) {}
        void print(int indent = 0) const override { /* 생략 */ }
    };

    struct ProgramNode : Node {
        std::vector<std::shared_ptr<DomainNode>> domains;
        void print(int indent = 0) const override { /* 생략 */ }
    };
}