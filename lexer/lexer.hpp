#ifndef RIVEN_LEXER_HPP
#define RIVEN_LEXER_HPP

#include <string>
#include <vector>
#include <iostream>

namespace Riven {

enum class TokenType {
    // Keywords
    CONSISTOF,
    IMPRINT,
    FIRM,
    SCANQ,
    CORE,
    RIVEN,
    IF,
    ELSE,
    ALTIF,
    RETURN,
    FIN,
    REC,
    CRAFT,
    RESC,
    ATTACK,
    RAW,
    REF,
    PTR,

    // Identifiers & Literals
    IDENTIFIER,
    LITERAL_INT,
    LITERAL_FLOAT,
    LITERAL_STRING,

    // Operators
    OP_ASSIGN,       // =
    OP_INCREMENT,    // +>
    OP_DECREMENT,    // -<
    OP_PLUS,         // +
    OP_MINUS,        // -
    OP_MUL,          // *
    OP_DIV,          // /
    OP_EQ,           // ==
    OP_NE,           // !=
    OP_LT,           // <
    OP_GT,           // >
    OP_LE,           // <=
    OP_GE,           // >=
    OP_AMP,          // &

    // Delimiters
    LPAREN,          // (
    RPAREN,          // )
    LBRACE,          // {
    RBRACE,          // }
    LBRACKET,        // [
    RBRACKET,        // ]
    COMMA,           // ,
    SEMICOLON,       // ;
    COLON,           // :

    // Special
    END_OF_FILE,
    UNKNOWN
};

struct SourceLocation {
    size_t line;
    size_t column;
    std::string filename;
};

struct Token {
    TokenType type;
    std::string value;
    SourceLocation location;

    std::string toString() const;
    static std::string typeToString(TokenType type);
};

class Lexer {
private:
    std::string source;
    std::string filename;
    size_t position;
    size_t line;
    size_t column;

    char peek(size_t offset = 0) const;
    char advance();
    void skipWhitespaceAndComments();
    Token readIdentifierOrKeyword();
    Token readNumber();
    Token readString();

public:
    Lexer(std::string source, std::string filename = "main.rn");
    std::vector<Token> tokenize();
};

} // namespace Riven

#endif // RIVEN_LEXER_HPP
