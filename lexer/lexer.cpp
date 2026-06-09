#include "lexer.hpp"
#include <cctype>

namespace Riven {

std::string Token::toString() const {
    return "[" + typeToString(type) + " '" + value + "' " + 
           location.filename + ":" + std::to_string(location.line) + ":" + 
           std::to_string(location.column) + "]";
}

std::string Token::typeToString(TokenType type) {
    switch (type) {
        case TokenType::CONSISTOF: return "CONSISTOF";
        case TokenType::IMPRINT: return "IMPRINT";
        case TokenType::FIRM: return "FIRM";
        case TokenType::SCANQ: return "SCANQ";
        case TokenType::CORE: return "CORE";
        case TokenType::RIVEN: return "RIVEN";
        case TokenType::IF: return "IF";
        case TokenType::ELSE: return "ELSE";
        case TokenType::ALTIF: return "ALTIF";
        case TokenType::RETURN: return "RETURN";
        case TokenType::FIN: return "FIN";
        case TokenType::REC: return "REC";
        case TokenType::CRAFT: return "CRAFT";
        case TokenType::RESC: return "RESC";
        case TokenType::ATTACK: return "ATTACK";
        case TokenType::RAW: return "RAW";
        case TokenType::REF: return "REF";
        case TokenType::PTR: return "PTR";
        case TokenType::IDENTIFIER: return "IDENTIFIER";
        case TokenType::LITERAL_INT: return "LITERAL_INT";
        case TokenType::LITERAL_FLOAT: return "LITERAL_FLOAT";
        case TokenType::LITERAL_STRING: return "LITERAL_STRING";
        case TokenType::OP_ASSIGN: return "OP_ASSIGN";
        case TokenType::OP_INCREMENT: return "OP_INCREMENT";
        case TokenType::OP_DECREMENT: return "OP_DECREMENT";
        case TokenType::OP_PLUS: return "OP_PLUS";
        case TokenType::OP_MINUS: return "OP_MINUS";
        case TokenType::OP_MUL: return "OP_MUL";
        case TokenType::OP_DIV: return "OP_DIV";
        case TokenType::OP_EQ: return "OP_EQ";
        case TokenType::OP_NE: return "OP_NE";
        case TokenType::OP_LT: return "OP_LT";
        case TokenType::OP_GT: return "OP_GT";
        case TokenType::OP_LE: return "OP_LE";
        case TokenType::OP_GE: return "OP_GE";
        case TokenType::OP_AMP: return "OP_AMP";
        case TokenType::LPAREN: return "LPAREN";
        case TokenType::RPAREN: return "RPAREN";
        case TokenType::LBRACE: return "LBRACE";
        case TokenType::RBRACE: return "RBRACE";
        case TokenType::LBRACKET: return "LBRACKET";
        case TokenType::RBRACKET: return "RBRACKET";
        case TokenType::COMMA: return "COMMA";
        case TokenType::SEMICOLON: return "SEMICOLON";
        case TokenType::COLON: return "COLON";
        case TokenType::END_OF_FILE: return "EOF";
        case TokenType::UNKNOWN: return "UNKNOWN";
    }
    return "UNKNOWN";
}

Lexer::Lexer(std::string source, std::string filename)
    : source(source), filename(filename), position(0), line(1), column(1) {}

char Lexer::peek(size_t offset) const {
    if (position + offset >= source.length()) {
        return '\0';
    }
    return source[position + offset];
}

char Lexer::advance() {
    if (position >= source.length()) {
        return '\0';
    }
    char current = source[position++];
    if (current == '\n') {
        line++;
        column = 1;
    } else {
        column++;
    }
    return current;
}

void Lexer::skipWhitespaceAndComments() {
    while (position < source.length()) {
        char current = peek();
        if (std::isspace(current)) {
            advance();
        } else if (current == '/' && peek(1) == '/') {
            // Single-line comment
            while (peek() != '\n' && peek() != '\0') {
                advance();
            }
        } else if (current == '/' && peek(1) == '*') {
            // Multi-line comment
            advance(); // '/'
            advance(); // '*'
            while (!(peek() == '*' && peek(1) == '/') && peek() != '\0') {
                advance();
            }
            if (peek() != '\0') {
                advance(); // '*'
                advance(); // '/'
            }
        } else {
            break;
        }
    }
}

Token Lexer::readIdentifierOrKeyword() {
    SourceLocation loc{line, column, filename};
    std::string value;

    while (std::isalnum(peek()) || peek() == '_') {
        value += advance();
    }

    TokenType type = TokenType::IDENTIFIER;

    if (value == "consistof")       type = TokenType::CONSISTOF;
    else if (value == "Imprint")    type = TokenType::IMPRINT;
    else if (value == "firm")       type = TokenType::FIRM;
    else if (value == "scanq")      type = TokenType::SCANQ;
    else if (value == "core")       type = TokenType::CORE;
    else if (value == "riven")      type = TokenType::RIVEN;
    else if (value == "if")         type = TokenType::IF;
    else if (value == "else")       type = TokenType::ELSE;
    else if (value == "altif")      type = TokenType::ALTIF;
    else if (value == "return")     type = TokenType::RETURN;
    else if (value == "fin")        type = TokenType::FIN;
    else if (value == "rec")        type = TokenType::REC;
    else if (value == "craft")      type = TokenType::CRAFT;
    else if (value == "resc")       type = TokenType::RESC;
    else if (value == "attack")     type = TokenType::ATTACK;
    else if (value == "raw")        type = TokenType::RAW;
    else if (value == "ref")        type = TokenType::REF;
    else if (value == "ptr")        type = TokenType::PTR;

    return Token{type, value, loc};
}

Token Lexer::readNumber() {
    SourceLocation loc{line, column, filename};
    std::string value;
    bool isFloat = false;

    while (std::isdigit(peek())) {
        value += advance();
    }

    if (peek() == '.' && std::isdigit(peek(1))) {
        isFloat = true;
        value += advance(); // '.'
        while (std::isdigit(peek())) {
            value += advance();
        }
    }

    TokenType type = isFloat ? TokenType::LITERAL_FLOAT : TokenType::LITERAL_INT;
    return Token{type, value, loc};
}

Token Lexer::readString() {
    SourceLocation loc{line, column, filename};
    advance(); // Skip starting double quote
    std::string value;

    while (peek() != '"' && peek() != '\0') {
        if (peek() == '\\' && peek(1) == 'n') {
            value += '\n';
            advance();
            advance();
        } else {
            value += advance();
        }
    }

    if (peek() == '"') {
        advance(); // Skip closing double quote
    }

    return Token{TokenType::LITERAL_STRING, value, loc};
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;

    while (position < source.length()) {
        skipWhitespaceAndComments();
        if (position >= source.length()) break;

        char current = peek();
        SourceLocation loc{line, column, filename};

        if (std::isalpha(current) || current == '_') {
            tokens.push_back(readIdentifierOrKeyword());
        } else if (std::isdigit(current)) {
            tokens.push_back(readNumber());
        } else if (current == '"') {
            tokens.push_back(readString());
        } else if (current == '+' && peek(1) == '>') {
            advance(); advance();
            tokens.push_back(Token{TokenType::OP_INCREMENT, "+>", loc});
        } else if (current == '-' && peek(1) == '<') {
            advance(); advance();
            tokens.push_back(Token{TokenType::OP_DECREMENT, "-<", loc});
        } else if (current == '=' && peek(1) == '=') {
            advance(); advance();
            tokens.push_back(Token{TokenType::OP_EQ, "==", loc});
        } else if (current == '!' && peek(1) == '=') {
            advance(); advance();
            tokens.push_back(Token{TokenType::OP_NE, "!=", loc});
        } else if (current == '<' && peek(1) == '=') {
            advance(); advance();
            tokens.push_back(Token{TokenType::OP_LE, "<=", loc});
        } else if (current == '>' && peek(1) == '=') {
            advance(); advance();
            tokens.push_back(Token{TokenType::OP_GE, ">=", loc});
        } else if (current == '=') {
            advance();
            tokens.push_back(Token{TokenType::OP_ASSIGN, "=", loc});
        } else if (current == '+') {
            advance();
            tokens.push_back(Token{TokenType::OP_PLUS, "+", loc});
        } else if (current == '-') {
            advance();
            tokens.push_back(Token{TokenType::OP_MINUS, "-", loc});
        } else if (current == '*') {
            advance();
            tokens.push_back(Token{TokenType::OP_MUL, "*", loc});
        } else if (current == '/') {
            advance();
            tokens.push_back(Token{TokenType::OP_DIV, "/", loc});
        } else if (current == '<') {
            advance();
            tokens.push_back(Token{TokenType::OP_LT, "<", loc});
        } else if (current == '>') {
            advance();
            tokens.push_back(Token{TokenType::OP_GT, ">", loc});
        } else if (current == '&') {
            advance();
            tokens.push_back(Token{TokenType::OP_AMP, "&", loc});
        } else if (current == '(') {
            advance();
            tokens.push_back(Token{TokenType::LPAREN, "(", loc});
        } else if (current == ')') {
            advance();
            tokens.push_back(Token{TokenType::RPAREN, ")", loc});
        } else if (current == '{') {
            advance();
            tokens.push_back(Token{TokenType::LBRACE, "{", loc});
        } else if (current == '}') {
            advance();
            tokens.push_back(Token{TokenType::RBRACE, "}", loc});
        } else if (current == '[') {
            advance();
            tokens.push_back(Token{TokenType::LBRACKET, "[", loc});
        } else if (current == ']') {
            advance();
            tokens.push_back(Token{TokenType::RBRACKET, "]", loc});
        } else if (current == ',') {
            advance();
            tokens.push_back(Token{TokenType::COMMA, ",", loc});
        } else if (current == ';') {
            advance();
            tokens.push_back(Token{TokenType::SEMICOLON, ";", loc});
        } else if (current == ':') {
            advance();
            tokens.push_back(Token{TokenType::COLON, ":", loc});
        } else {
            advance();
            tokens.push_back(Token{TokenType::UNKNOWN, std::string(1, current), loc});
        }
    }

    tokens.push_back(Token{TokenType::END_OF_FILE, "", SourceLocation{line, column, filename}});
    return tokens;
}

} // namespace Riven
