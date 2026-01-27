#pragma once
#include <vector>
#include <memory>
#include <string>
#include "token.hpp"
#include "../ast/node.hpp" // [중요] AST 정의 포함

namespace topologos::parser {

    class Parser {
    public:
        explicit Parser(std::vector<Token> tokens);

        // 반환 타입을 shared_ptr<ast::ProgramNode>로 통일
        std::shared_ptr<topologos::ast::ProgramNode> parse();

    private:
        std::vector<Token> tokens_;
        int current_ = 0;

        // --- Parsing Methods ---
        std::shared_ptr<topologos::ast::DomainNode> parse_domain();
        std::shared_ptr<topologos::ast::AxiomNode> parse_axiom();
        std::shared_ptr<topologos::ast::RuleNode> parse_rule();
        
        // Expression Parsing
        std::shared_ptr<topologos::ast::Expr> parse_expression();
        std::shared_ptr<topologos::ast::Expr> parse_logic_or();
        std::shared_ptr<topologos::ast::Expr> parse_logic_and();
        std::shared_ptr<topologos::ast::Expr> parse_equality();
        std::shared_ptr<topologos::ast::Expr> parse_comparison();
        std::shared_ptr<topologos::ast::Expr> parse_term();
        std::shared_ptr<topologos::ast::Expr> parse_factor();
        std::shared_ptr<topologos::ast::Expr> parse_unary();
        std::shared_ptr<topologos::ast::Expr> parse_primary();

        // --- Helpers ---
        bool match(TokenType type);
        bool check(TokenType type) const;
        Token advance();
        bool is_at_end() const;
        Token peek() const;
        Token previous() const;
        Token consume(TokenType type, const std::string& message);
    };

} // namespace topologos::parser