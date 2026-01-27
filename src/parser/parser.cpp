#include "../../include/topologos/parser/parser.hpp"
#include <iostream>
#include <vector>
#include <memory>
#include <stdexcept>

namespace topologos::parser {
    
    using namespace ast;

    Parser::Parser(std::vector<Token> tokens) : tokens_(std::move(tokens)) {}

    std::shared_ptr<ProgramNode> Parser::parse() {
        auto program = std::make_shared<ProgramNode>();
        try {
            while (!is_at_end()) {
                if (match(TokenType::DOMAIN)) {
                    program->domains.push_back(parse_domain());
                } else {
                    // 알 수 없는 토큰은 스킵 (또는 에러 처리)
                    advance(); 
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "[Parser Error] " << e.what() << std::endl;
            return nullptr;
        }
        return program;
    }

    // --- 구조 파싱 (Structure Parsing) ---

    std::shared_ptr<DomainNode> Parser::parse_domain() {
        consume(TokenType::IDENTIFIER, "Expect domain name.");
        std::string name = previous().value;
        auto domain = std::make_shared<DomainNode>(name);

        consume(TokenType::LBRACE, "Expect '{' before domain body.");
        
        while (!check(TokenType::RBRACE) && !is_at_end()) {
            if (match(TokenType::AXIOM)) {
                domain->axioms.push_back(parse_axiom());
            } else {
                advance(); // 에러 복구
            }
        }
        
        consume(TokenType::RBRACE, "Expect '}' after domain body.");
        return domain;
    }

    std::shared_ptr<AxiomNode> Parser::parse_axiom() {
        consume(TokenType::IDENTIFIER, "Expect axiom name.");
        std::string name = previous().value;
        auto axiom = std::make_shared<AxiomNode>(name);

        consume(TokenType::LBRACE, "Expect '{' before axiom body.");
        
        while (!check(TokenType::RBRACE) && !is_at_end()) {
            if (match(TokenType::RULE)) {
                axiom->rules.push_back(parse_rule());
            } else {
                advance();
            }
        }

        consume(TokenType::RBRACE, "Expect '}' after axiom body.");
        return axiom;
    }

    // [Update] parse_rule: 제네릭 타입(list<string>) 처리 추가
    std::shared_ptr<RuleNode> Parser::parse_rule() {
        consume(TokenType::IDENTIFIER, "Expect rule name.");
        std::string name = previous().value;
        auto rule = std::make_shared<RuleNode>(name);

        consume(TokenType::LPAREN, "Expect '(' after rule name.");
        if (!check(TokenType::RPAREN)) {
            do {
                consume(TokenType::IDENTIFIER, "Expect parameter name.");
                std::string param_name = previous().value;
                consume(TokenType::COLON, "Expect ':' after parameter name.");
                
                // --- [FIX Start] 제네릭 타입 파싱 ---
                std::string param_type;
                if (match(TokenType::TYPE_LIST)) {
                    param_type = "list";
                    if (match(TokenType::LESS)) {
                        param_type += "<";
                        advance(); // 내부 타입 (예: string)
                        param_type += previous().value;
                        consume(TokenType::GREATER, "Expect '>' after generic type.");
                        param_type += ">";
                    }
                } else {
                    advance(); // 일반 타입
                    param_type = previous().value;
                }
                // --- [FIX End] ---

                rule->params.push_back({param_name, param_type});
            } while (match(TokenType::COMMA));
        }
        consume(TokenType::RPAREN, "Expect ')' after parameters.");

        // ... 기존 본문 파싱 코드 그대로 유지 ...
        consume(TokenType::LBRACE, "Expect '{' before rule body.");
        while (!check(TokenType::RBRACE) && !is_at_end()) {
            if (match(TokenType::CONDITION)) {
                consume(TokenType::COLON, "Expect ':' after 'condition'.");
                rule->condition = parse_expression();
                consume(TokenType::SEMICOLON, "Expect ';' after condition.");
            } else if (match(TokenType::FAILURE_MSG)) {
                consume(TokenType::COLON, "Expect ':' after 'failure_msg'.");
                consume(TokenType::STRING_LITERAL, "Expect string for failure message.");
                rule->failure_msg = previous().value;
                consume(TokenType::SEMICOLON, "Expect ';' after message.");
            } else if (match(TokenType::IDENTIFIER) || match(TokenType::THRESHOLD)) {
                std::string key = previous().value;
                consume(TokenType::COLON, "Expect ':' after property name.");
                rule->constants.push_back({key, parse_expression()});
                consume(TokenType::SEMICOLON, "Expect ';' after property.");
            } else {
                advance();
            }
        }
        consume(TokenType::RBRACE, "Expect '}' after rule body.");
        return rule;
    }

    // --- 표현식 파싱 (Expression Parsing) ---
    // 우선순위: OR -> AND -> Equality -> Comparison -> Term -> Factor -> Unary -> Primary

    std::shared_ptr<Expr> Parser::parse_expression() {
        return parse_logic_or();
    }

    std::shared_ptr<Expr> Parser::parse_logic_or() {
        auto expr = parse_logic_and();
        while (match(TokenType::OR)) {
            std::string op = "||";
            auto right = parse_logic_and();
            expr = std::make_shared<BinaryExpr>(expr, op, right);
        }
        return expr;
    }

    std::shared_ptr<Expr> Parser::parse_logic_and() {
        auto expr = parse_equality();
        while (match(TokenType::AND)) {
            std::string op = "&&";
            auto right = parse_equality();
            expr = std::make_shared<BinaryExpr>(expr, op, right);
        }
        return expr;
    }

    std::shared_ptr<Expr> Parser::parse_equality() {
        auto expr = parse_comparison();
        while (match(TokenType::EQUAL_EQUAL) || match(TokenType::BANG_EQUAL)) {
            std::string op = previous().type == TokenType::EQUAL_EQUAL ? "==" : "!=";
            auto right = parse_comparison();
            expr = std::make_shared<BinaryExpr>(expr, op, right);
        }
        return expr;
    }

    std::shared_ptr<Expr> Parser::parse_comparison() {
        auto expr = parse_term();
        while (match(TokenType::GREATER) || match(TokenType::GREATER_EQUAL) ||
               match(TokenType::LESS) || match(TokenType::LESS_EQUAL)) {
            std::string op;
            if (previous().type == TokenType::GREATER) op = ">";
            else if (previous().type == TokenType::GREATER_EQUAL) op = ">=";
            else if (previous().type == TokenType::LESS) op = "<";
            else op = "<=";
            
            auto right = parse_term();
            expr = std::make_shared<BinaryExpr>(expr, op, right);
        }
        return expr;
    }

    std::shared_ptr<Expr> Parser::parse_term() {
        auto expr = parse_factor();
        while (match(TokenType::MINUS) || match(TokenType::PLUS)) {
            std::string op = previous().type == TokenType::PLUS ? "+" : "-";
            auto right = parse_factor();
            expr = std::make_shared<BinaryExpr>(expr, op, right);
        }
        return expr;
    }

    std::shared_ptr<Expr> Parser::parse_factor() {
        auto expr = parse_unary();
        while (match(TokenType::SLASH) || match(TokenType::STAR)) {
            std::string op = previous().type == TokenType::STAR ? "*" : "/";
            auto right = parse_unary();
            expr = std::make_shared<BinaryExpr>(expr, op, right);
        }
        return expr;
    }

    std::shared_ptr<Expr> Parser::parse_unary() {
        if (match(TokenType::NOT) || match(TokenType::MINUS)) {
            std::string op = previous().type == TokenType::NOT ? "!" : "-";
            auto right = parse_unary();
            // 단항 연산자는 편의상 BinaryExpr의 left를 nullptr로 처리하거나 별도 노드 필요
            // 여기서는 0 - right 형태로 변환하거나 단순화 처리
            return std::make_shared<BinaryExpr>(std::make_shared<Literal>("0"), op, right);
        }
        return parse_primary();
    }

    // [Update] parse_primary: 함수 호출 파싱 추가
    std::shared_ptr<Expr> Parser::parse_primary() {
        if (match(TokenType::FALSE)) return std::make_shared<Literal>("false");
        if (match(TokenType::TRUE)) return std::make_shared<Literal>("true");
        if (match(TokenType::NUMBER)) return std::make_shared<Literal>(previous().value);
        if (match(TokenType::STRING_LITERAL)) return std::make_shared<Literal>(previous().value);

        if (match(TokenType::IDENTIFIER) || match(TokenType::THRESHOLD)) {
            std::string name = previous().value;
            
            // [FIX] Identifier 뒤에 '('가 오면 함수 호출로 파싱
            if (match(TokenType::LPAREN)) {
                std::vector<std::shared_ptr<Expr>> args;
                if (!check(TokenType::RPAREN)) {
                    do {
                        args.push_back(parse_expression());
                    } while (match(TokenType::COMMA));
                }
                consume(TokenType::RPAREN, "Expect ')' after arguments.");
                return std::make_shared<CallExpr>(name, args);
            }
            return std::make_shared<Variable>(name);
        }

        if (match(TokenType::LPAREN)) {
            auto expr = parse_expression();
            consume(TokenType::RPAREN, "Expect ')' after expression.");
            return expr;
        }
        throw std::runtime_error("Expect expression. Found: " + peek().value);
    }

    // --- Helpers ---

    bool Parser::match(TokenType type) {
        if (check(type)) {
            advance();
            return true;
        }
        return false;
    }

    bool Parser::check(TokenType type) const {
        if (is_at_end()) return false;
        return peek().type == type;
    }

    Token Parser::advance() {
        if (!is_at_end()) current_++;
        return previous();
    }

    bool Parser::is_at_end() const {
        return peek().type == TokenType::END_OF_FILE;
    }

    Token Parser::peek() const {
        return tokens_[current_];
    }

    Token Parser::previous() const {
        return tokens_[current_ - 1];
    }

    Token Parser::consume(TokenType type, const std::string& message) {
        if (check(type)) return advance();
        throw std::runtime_error(message + " at line " + std::to_string(peek().line));
    }

} // namespace topologos::parser