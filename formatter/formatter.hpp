#ifndef RIVEN_FORMATTER_HPP
#define RIVEN_FORMATTER_HPP

#include "../lexer/lexer.hpp"
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>

namespace Riven {

class Formatter {
public:
    static std::string format(const std::vector<Token>& tokens) {
        std::stringstream ss;
        int indentLevel = 0;
        bool needSpace = false;
        bool newlineAfterClose = false;

        for (size_t i = 0; i < tokens.size(); ++i) {
            const auto& tok = tokens[i];
            if (tok.type == TokenType::END_OF_FILE) break;

            if (tok.type == TokenType::RBRACE) {
                indentLevel = std::max(0, indentLevel - 1);
                ss << "\n" << std::string(indentLevel * 4, ' ') << "}";
                newlineAfterClose = true;
                needSpace = false;
                continue;
            }

            if (newlineAfterClose) {
                ss << "\n" << std::string(indentLevel * 4, ' ');
                newlineAfterClose = false;
            }

            if (tok.type == TokenType::LBRACE) {
                ss << " {\n";
                indentLevel++;
                ss << std::string(indentLevel * 4, ' ');
                needSpace = false;
                continue;
            }

            if (tok.type == TokenType::SEMICOLON) {
                ss << ";\n" << std::string(indentLevel * 4, ' ');
                needSpace = false;
                continue;
            }

            if (tok.type == TokenType::COMMA) {
                ss << ", ";
                needSpace = false;
                continue;
            }

            // Keyword spacing heuristics
            bool isKeyword = (tok.type == TokenType::CRAFT || tok.type == TokenType::REC || 
                              tok.type == TokenType::IF || tok.type == TokenType::ELSE || 
                              tok.type == TokenType::ALTIF || tok.type == TokenType::RAW || 
                              tok.type == TokenType::RESC || tok.type == TokenType::FIRM || 
                              tok.type == TokenType::CONSISTOF || tok.type == TokenType::RETURN);

            bool isOperator = (tok.type == TokenType::OP_ASSIGN || tok.type == TokenType::OP_EQ || 
                               tok.type == TokenType::OP_NE || tok.type == TokenType::OP_LT || 
                               tok.type == TokenType::OP_GT || tok.type == TokenType::OP_LE || 
                               tok.type == TokenType::OP_GE || tok.type == TokenType::OP_PLUS || 
                               tok.type == TokenType::OP_MINUS || tok.type == TokenType::OP_MUL || 
                               tok.type == TokenType::OP_DIV);

            if (isOperator) {
                ss << " " << tok.value << " ";
                needSpace = false;
                continue;
            }

            if (needSpace && tok.type != TokenType::RPAREN && tok.type != TokenType::LBRACKET && tok.type != TokenType::RBRACKET) {
                ss << " ";
            }

            ss << tok.value;
            needSpace = isKeyword || (tok.type == TokenType::IDENTIFIER && i + 1 < tokens.size() && tokens[i+1].type == TokenType::IDENTIFIER);
        }
        return ss.str();
    }
};

} // namespace Riven

#endif // RIVEN_FORMATTER_HPP
