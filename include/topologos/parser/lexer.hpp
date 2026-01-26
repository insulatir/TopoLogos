#pragma once
#include <string>
#include <vector>
#include <unordered_map>

namespace topologos::parser {

    enum class TokenType {
        // 키워드
        COMMUNITY, CONCEPT,
        // 기호
        LBRACE, RBRACE, SEMICOLON, ARROW,
        // 데이터
        IDENTIFIER,
        // 끝
        END_OF_FILE
    };

    struct Token {
        TokenType type;
        std::string value;
        size_t line;
    };

    class Lexer {
    public:
        explicit Lexer(std::string source);
        
        // 외부에서 호출하는 메인 함수
        std::vector<Token> tokenize();

    private:
        const std::string source_;
        std::vector<Token> tokens_;
        
        // 파싱 위치를 추적하는 변수들
        size_t start_ = 0;
        size_t current_ = 0;
        size_t line_ = 1;

        // 내부 헬퍼 함수들
        bool is_at_end() const;
        char advance();
        char peek() const;
        char peek_next() const;
        bool match(char expected);
        
        void scan_token();
        void add_token(TokenType type);
        void add_token(TokenType type, std::string value);
        
        void identifier();
        void skip_whitespace();
    };
}