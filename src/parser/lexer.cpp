#include "../../include/topologos/parser/lexer.hpp" // 경로 주의
#include <cctype>
#include <unordered_map>

namespace topologos::parser {

    Lexer::Lexer(std::string source) : source_(std::move(source)) {}

    std::vector<Token> Lexer::tokenize() {
        tokens_.clear();
        line_ = 1;
        current_ = 0;
        
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
            case '(': add_token(TokenType::LPAREN); break;
            case ')': add_token(TokenType::RPAREN); break;
            case '{': add_token(TokenType::LBRACE); break;
            case '}': add_token(TokenType::RBRACE); break;
            case '[': add_token(TokenType::LBRACKET); break;
            case ']': add_token(TokenType::RBRACKET); break;
            case ',': add_token(TokenType::COMMA); break;
            case '.': add_token(TokenType::DOT); break;
            case ';': add_token(TokenType::SEMICOLON); break;
            case ':': add_token(TokenType::COLON); break;
            
            case '*': add_token(TokenType::STAR); break;
            case '+': add_token(TokenType::PLUS); break;
            
            case '-':
                if (match('>')) add_token(TokenType::ARROW);
                else add_token(TokenType::MINUS);
                break;
            
            case '!':
                add_token(match('=') ? TokenType::BANG_EQUAL : TokenType::NOT);
                break;
            case '=':
                add_token(match('=') ? TokenType::EQUAL_EQUAL : TokenType::EQUAL);
                break;
            case '<':
                add_token(match('=') ? TokenType::LESS_EQUAL : TokenType::LESS);
                break;
            case '>':
                add_token(match('=') ? TokenType::GREATER_EQUAL : TokenType::GREATER);
                break;
            case '&':
                if (match('&')) add_token(TokenType::AND);
                break;
            case '|':
                if (match('|')) add_token(TokenType::OR);
                break;

            case '/':
                if (match('/')) {
                    // 주석: 줄바꿈까지 스킵
                    while (peek() != '\n' && !is_at_end()) advance();
                } else {
                    add_token(TokenType::SLASH);
                }
                break;

            case ' ':
            case '\r':
            case '\t':
                break;
            case '\n':
                line_++;
                break;

            case '"': string_literal(); break;

            default:
                if (std::isdigit(c)) {
                    number();
                } else if (std::isalpha(c) || c == '_') {
                    identifier();
                } else {
                    // Unknown char ignored
                }
                break;
        }
    }

    void Lexer::identifier() {
        while (std::isalnum(peek()) || peek() == '_') advance();

        std::string text = source_.substr(start_, current_ - start_);
        TokenType type = TokenType::IDENTIFIER;
        
        // 키워드 매핑
        static const std::unordered_map<std::string, TokenType> keywords = {
            {"domain", TokenType::DOMAIN},
            {"axiom", TokenType::AXIOM},
            {"rule", TokenType::RULE},
            {"threshold", TokenType::THRESHOLD},
            {"condition", TokenType::CONDITION},
            {"failure_msg", TokenType::FAILURE_MSG},
            {"float", TokenType::TYPE_FLOAT},
            {"int", TokenType::TYPE_INT},
            {"bool", TokenType::TYPE_BOOL},
            {"string", TokenType::TYPE_STRING},
            {"list", TokenType::TYPE_LIST},
            {"true", TokenType::TRUE},
            {"false", TokenType::FALSE},
            {"Community", TokenType::COMMUNITY}, // 하위 호환
            {"Concept", TokenType::CONCEPT}      // 하위 호환
        };

        if (keywords.find(text) != keywords.end()) {
            type = keywords.at(text);
        }

        add_token(type, text);
    }

    void Lexer::number() {
        while (std::isdigit(peek())) advance();

        // 소수점 처리
        if (peek() == '.' && std::isdigit(peek_next())) {
            advance(); // Consume '.'
            while (std::isdigit(peek())) advance();
        }

        add_token(TokenType::NUMBER);
    }

    void Lexer::string_literal() {
        while (peek() != '"' && !is_at_end()) {
            if (peek() == '\n') line_++;
            advance();
        }

        if (is_at_end()) return; // Unterminated string

        advance(); // Closing "
        
        // 따옴표 제외한 값 저장
        std::string value = source_.substr(start_ + 1, current_ - start_ - 2);
        tokens_.push_back({TokenType::STRING_LITERAL, value, line_});
    }

    // Helper functions (기존과 동일하지만 명시)
    void Lexer::add_token(TokenType type) { add_token(type, ""); }
    void Lexer::add_token(TokenType type, std::string val) {
        if (type != TokenType::STRING_LITERAL && val.empty()) {
             val = source_.substr(start_, current_ - start_);
        }
        tokens_.push_back({type, val, line_});
    }
    
    char Lexer::advance() { return source_[current_++]; }
    bool Lexer::match(char expected) {
        if (is_at_end() || source_[current_] != expected) return false;
        current_++; return true;
    }
    char Lexer::peek() const { return is_at_end() ? '\0' : source_[current_]; }
    char Lexer::peek_next() const { return (current_ + 1 >= source_.length()) ? '\0' : source_[current_ + 1]; }
    bool Lexer::is_at_end() const { return current_ >= source_.length(); }
}