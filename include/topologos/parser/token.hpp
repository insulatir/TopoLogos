#pragma once
#include <string>
#include <format> // C++20 formatting

namespace topologos::parser {

    enum class TokenType {
        // Keywords
        KW_COMMUNITY, KW_CONCEPT,

        // Symbols
        LBRACE, RBRACE, SEMICOLON,
        ARROW,          // ->
        WAVE,           // ~>
        COLON_COLON,    // ::

        // Literals
        IDENTIFIER,     // User defined names

        // Control
        END_OF_FILE,
        ERROR
    };

    struct Token {
        TokenType type;
        std::string lexeme; // 실제 텍스트 ("Community", "CPU" 등)
        int line;           // 에러 리포팅용 줄 번호

        // 디버깅을 위한 출력 함수
        std::string debug_string() const {
            return "Line " + std::to_string(line) + ":Type[" + std::to_string((int)type) + "] '" + lexeme + "'";
        }
    };
}