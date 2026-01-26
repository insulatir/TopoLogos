#include "topologos/parser/parser.hpp"
#include <stdexcept>
#include <iostream>

namespace topologos::parser {

    Parser::Parser(std::vector<Token> tokens) : tokens_(std::move(tokens)) {}

    // 메인 파싱 루프
    std::unique_ptr<ProgramNode> Parser::parse() {
        auto program = std::make_unique<ProgramNode>();
        
        try {
            while (!is_at_end()) {
                // Community 키워드로 시작하는 블록 파싱
                parse_community(program.get());
            }
        } catch (const std::exception& e) {
            std::cerr << "[Parser Error] " << e.what() << std::endl;
            return nullptr; 
        }

        return program;
    }

    // "Community 이름 { ... }" 파싱
    void Parser::parse_community(ProgramNode* program) {
        consume(TokenType::COMMUNITY, "Expect 'Community' keyword.");
        consume(TokenType::IDENTIFIER, "Expect community name.");
        consume(TokenType::LBRACE, "Expect '{' before community body.");

        while (!check(TokenType::RBRACE) && !is_at_end()) {
            parse_statement(program);
        }

        consume(TokenType::RBRACE, "Expect '}' after community body.");
    }

    // 내부 문장 파싱 (Concept 정의 또는 관계 정의)
    void Parser::parse_statement(ProgramNode* program) {
        if (match(TokenType::CONCEPT)) {
            // "Concept 이름;"
            Token name = consume(TokenType::IDENTIFIER, "Expect concept name.");
            consume(TokenType::SEMICOLON, "Expect ';' after concept declaration.");
            
            // AST에 추가 (필요하다면)
            // program->statements.push_back(std::make_unique<ConceptNode>(name.value));
        } 
        else if (check(TokenType::IDENTIFIER)) {
            // "Source -> Target;"
            Token source = advance(); // Source
            
            if (match(TokenType::ARROW)) {
                Token target = consume(TokenType::IDENTIFIER, "Expect target name.");
                consume(TokenType::SEMICOLON, "Expect ';' after relation.");
                
                // AST에 관계 추가 (Generator가 사용할 데이터)
                auto relation = std::make_unique<RelationNode>(source.value, target.value);
                
                // *중요* RelationNode 포인터를 relations 벡터에도 저장 (Generator 순회용)
                program->relations.push_back(relation.get());
                
                // 소유권은 statements 벡터가 가짐
                program->statements.push_back(std::move(relation));
            } else {
                // 화살표가 없으면 그냥 식별자일 수 있음 (에러 처리 혹은 무시)
                // 여기선 세미콜론 기대
                consume(TokenType::SEMICOLON, "Expect ';' or '->'.");
            }
        } else {
            // 알 수 없는 토큰 스킵
            advance();
        }
    }

    // --- 헬퍼 함수 구현 ---

    bool Parser::is_at_end() const {
        return peek().type == TokenType::END_OF_FILE;
    }

    const Token& Parser::peek() const {
        return tokens_[current_];
    }

    const Token& Parser::previous() const {
        return tokens_[current_ - 1];
    }

    const Token& Parser::advance() {
        if (!is_at_end()) current_++;
        return previous();
    }

    bool Parser::check(TokenType type) const {
        if (is_at_end()) return false;
        return peek().type == type;
    }

    bool Parser::match(TokenType type) {
        if (check(type)) {
            advance();
            return true;
        }
        return false;
    }

    const Token& Parser::consume(TokenType type, const std::string& message) {
        if (check(type)) return advance();
        
        // 에러 발생 시 예외 던짐 (간단한 처리를 위해)
        throw std::runtime_error(message + " at line " + std::to_string(peek().line));
    }
}