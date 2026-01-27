#pragma once
#include <string>

namespace topologos::parser {

    enum class TokenType {
        // [기존] 구조 정의용
        COMMUNITY, CONCEPT, ARROW,
        
        // [New] 법칙 정의용 (life.topo)
        DOMAIN, AXIOM, RULE, 
        THRESHOLD, CONDITION, FAILURE_MSG,
        
        // [New] 자료형 및 값
        TYPE_FLOAT, TYPE_INT, TYPE_BOOL, TYPE_STRING, TYPE_LIST,
        TRUE, FALSE, NUMBER, STRING_LITERAL,
        
        // [New] 연산자 및 구두점
        PLUS, MINUS, STAR, SLASH,
        EQUAL, EQUAL_EQUAL, BANG_EQUAL, // = , == , !=
        LESS, LESS_EQUAL, GREATER, GREATER_EQUAL, // <, <=, >, >=
        AND, OR, NOT, // &&, ||, !
        
        LPAREN, RPAREN, LBRACE, RBRACE, LBRACKET, RBRACKET, // (), {}, []
        COMMA, COLON, SEMICOLON, DOT,
        
        IDENTIFIER, END_OF_FILE
    };

    struct Token {
        TokenType type;
        std::string value;
        int line;
    };
}