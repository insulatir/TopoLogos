#pragma once
#include <vector>
#include <memory>
#include <string>
#include "token.hpp"
// [Fix] 직접 참조
#include "../ast/node.hpp" 

namespace topologos::parser {

    class Parser {
    public:
        explicit Parser(std::vector<Token> tokens);
        std::shared_ptr<topologos::ast::Program> parse();

    private:
        std::vector<Token> tokens_;
        int current_ = 0;

        std::shared_ptr<topologos::ast::Domain> parse_domain();
        std::shared_ptr<topologos::ast::Axiom> parse_axiom();
        std::shared_ptr<topologos::ast::Rule> parse_rule();
        
        std::shared_ptr<topologos::ast::Expr> parse_expression();
        std::shared_ptr<topologos::ast::Expr> parse_logic_or();
        std::shared_ptr<topologos::ast::Expr> parse_logic_and();
        std::shared_ptr<topologos::ast::Expr> parse_equality();
        std::shared_ptr<topologos::ast::Expr> parse_comparison();
        std::shared_ptr<topologos::ast::Expr> parse_term();
        std::shared_ptr<topologos::ast::Expr> parse_factor();
        std::shared_ptr<topologos::ast::Expr> parse_unary();
        std::shared_ptr<topologos::ast::Expr> parse_primary();

        bool match(TokenType type);
        bool check(TokenType type) const;
        Token advance();
        bool is_at_end() const;
        Token peek() const;
        Token previous() const;
        Token consume(TokenType type, const std::string& message);
    };

}