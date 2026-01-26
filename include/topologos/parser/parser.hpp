#pragma once
#include <vector>
#include <memory>
#include <string>
#include "lexer.hpp"
#include "ast.hpp"

namespace topologos::parser {

    class Parser {
    public:
        // 생성자: 토큰 목록을 받음
        explicit Parser(std::vector<Token> tokens);

        // 메인 함수: 파싱 결과를 AST(ProgramNode)로 반환
        std::unique_ptr<ProgramNode> parse();

    private:
        std::vector<Token> tokens_;
        size_t current_ = 0;

        // --- 토큰 탐색 헬퍼 함수들 ---
        bool is_at_end() const;
        const Token& peek() const;
        const Token& previous() const;
        const Token& advance();
        bool check(TokenType type) const;
        bool match(TokenType type);
        const Token& consume(TokenType type, const std::string& message);

        // --- 파싱 로직 함수들 ---
        void parse_community(ProgramNode* program);
        void parse_statement(ProgramNode* program);
    };
}