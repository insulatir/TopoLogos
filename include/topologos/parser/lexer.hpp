#pragma once
#include <string>
#include <vector>
#include "token.hpp" // [중요] 정의는 여기서 가져옴

namespace topologos::parser {

    class Lexer {
    public:
        explicit Lexer(std::string source);
        std::vector<Token> tokenize();

    private:
        std::string source_;
        std::vector<Token> tokens_;
        int start_ = 0;
        int current_ = 0;
        int line_ = 1;

        void scan_token();
        void identifier();
        void number();
        void string_literal();
        
        void add_token(TokenType type);
        void add_token(TokenType type, std::string literal);
        char advance();
        bool match(char expected);
        char peek() const;
        char peek_next() const;
        bool is_at_end() const;
    };

} // namespace topologos::parser