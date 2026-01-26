#include "topologos/parser/lexer.hpp"
#include <cctype>

namespace topologos::parser {

    Lexer::Lexer(std::string source) : source_(std::move(source)) {}

    std::vector<Token> Lexer::tokenize() {
        tokens_.clear();
        while (!is_at_end()) {
            start_ = current_;
            scan_token();
        }
        tokens_.push_back({TokenType::END_OF_FILE, "", line_});
        return tokens_;
    }

    void Lexer::scan_token() {
        char c = advance();
        switch (c) {
            case '{': add_token(TokenType::LBRACE); break;
            case '}': add_token(TokenType::RBRACE); break;
            case ';': add_token(TokenType::SEMICOLON); break;
            
            case '-':
                if (match('>')) {
                    add_token(TokenType::ARROW);
                } else {
                    // 그냥 하이픈인 경우 처리 (현재 문법엔 없지만 식별자로 넘길 수도 있음)
                }
                break;

            case ' ':
            case '\r':
            case '\t':
                // 공백 무시
                break;
            case '\n':
                line_++;
                break;

            case '/':
                if (match('/')) {
                    // 주석 처리: 줄바꿈까지 스킵
                    while (peek() != '\n' && !is_at_end()) advance();
                }
                break;

            default:
                if (std::isalpha(c)) {
                    identifier();
                } else {
                    // 알 수 없는 문자 무시
                }
                break;
        }
    }

    void Lexer::identifier() {
        while (std::isalnum(peek()) || peek() == '_') advance();

        std::string text = source_.substr(start_, current_ - start_);
        TokenType type = TokenType::IDENTIFIER;
        
        if (text == "Community") type = TokenType::COMMUNITY;
        if (text == "Concept") type = TokenType::CONCEPT;

        add_token(type, text);
    }

    void Lexer::add_token(TokenType type) {
        add_token(type, "");
    }

    void Lexer::add_token(TokenType type, std::string value) {
        if (value.empty()) {
            // 값이 없으면 소스 코드에서 잘라옴
            value = source_.substr(start_, current_ - start_);
        }
        tokens_.push_back({type, value, line_});
    }

    bool Lexer::is_at_end() const {
        return current_ >= source_.length();
    }

    char Lexer::advance() {
        return source_[current_++];
    }

    char Lexer::peek() const {
        if (is_at_end()) return '\0';
        return source_[current_];
    }

    char Lexer::peek_next() const {
        if (current_ + 1 >= source_.length()) return '\0';
        return source_[current_ + 1];
    }

    bool Lexer::match(char expected) {
        if (is_at_end()) return false;
        if (source_[current_] != expected) return false;
        current_++;
        return true;
    }
    
    void Lexer::skip_whitespace() {
        // scan_token 내부에서 처리하므로 비워둠, 혹은 로직 이동 가능
    }
}