#include "parser.hpp"
#include <stdexcept>

namespace Riven {

Parser::Parser(std::vector<Token> tokens) : tokens(tokens), current(0) {}

Token Parser::peek(size_t offset) const {
    if (current + offset >= tokens.size()) {
        return tokens.back();
    }
    return tokens[current + offset];
}

Token Parser::advance() {
    if (!isAtEnd()) {
        current++;
    }
    return tokens[current - 1];
}

bool Parser::isAtEnd() const {
    return peek().type == TokenType::END_OF_FILE;
}

bool Parser::check(TokenType type) const {
    if (isAtEnd()) return false;
    return peek().type == type;
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

Token Parser::consume(TokenType type, std::string message) {
    if (check(type)) {
        return advance();
    }
    throw std::runtime_error("Parser Error at line " + std::to_string(peek().location.line) + 
                            ", col " + std::to_string(peek().location.column) + ": " + message);
}

std::shared_ptr<ProgramNode> Parser::parse() {
    SourceLocation startLoc = peek().location;
    std::vector<std::shared_ptr<ASTNode>> nodes;

    while (!isAtEnd()) {
        try {
            nodes.push_back(parseTopLevel());
        } catch (const std::exception& e) {
            std::cerr << "Compilation error: " << e.what() << std::endl;
            // Panic recovery: skip until structural tokens
            advance();
            while (!isAtEnd() && peek().type != TokenType::SEMICOLON && 
                   peek().type != TokenType::RBRACE && peek().type != TokenType::CRAFT) {
                advance();
            }
            if (peek().type == TokenType::SEMICOLON) advance();
        }
    }

    return std::make_shared<ProgramNode>(nodes, startLoc);
}

std::shared_ptr<ASTNode> Parser::parseTopLevel() {
    if (match(TokenType::CONSISTOF)) {
        SourceLocation loc = peek(-1).location;
        std::string filename;
        if (check(TokenType::LITERAL_STRING)) {
            filename = advance().value;
        } else {
            // Raw path representation
            filename = consume(TokenType::IDENTIFIER, "Expected filename or path after consistof").value;
            while (match(TokenType::OP_DIV) || match(TokenType::IDENTIFIER)) {
                filename += peek(-1).value;
            }
        }
        return std::make_shared<ImportNode>(filename, loc);
    }

    if (peek().type == TokenType::REC) {
        return parseRec();
    }

    if (peek().type == TokenType::CRAFT) {
        return parseCraft();
    }

    if (peek().type == TokenType::RIVEN) {
        advance(); // Match 'riven'
        consume(TokenType::CORE, "Expected 'core' after 'riven'");
        return parseCore();
    }

    // Otherwise treat as a global statement
    return parseStatement();
}

std::shared_ptr<StatementNode> Parser::parseStatement() {
    if (peek().type == TokenType::FIRM) {
        return parseVarDecl();
    }

    // Let's check typing declarations like `int x = 5;` or primitive variables
    if (peek().type == TokenType::IDENTIFIER && peek(1).type == TokenType::IDENTIFIER) {
        return parseVarDecl();
    }

    if (match(TokenType::IMPRINT)) {
        return parseImprint();
    }

    if (peek().type == TokenType::IF) {
        return parseIf();
    }

    if (peek().type == TokenType::RETURN) {
        return parseReturn();
    }

    if (peek().type == TokenType::RESC) {
        return parseRescAttack();
    }

    if (match(TokenType::ATTACK)) {
        return parseAttack();
    }

    if (peek().type == TokenType::RAW) {
        return parseRaw();
    }

    if (peek().type == TokenType::LBRACE) {
        return parseBlock();
    }

    return parseAssignmentOrSuffix();
}

std::shared_ptr<VarDeclNode> Parser::parseVarDecl() {
    bool isFirm = false;
    SourceLocation loc = peek().location;
    if (match(TokenType::FIRM)) {
        isFirm = true;
    }

    std::string type = "auto";
    if (peek().type == TokenType::IDENTIFIER && peek(1).type == TokenType::IDENTIFIER) {
        type = advance().value;
    }

    std::string name = consume(TokenType::IDENTIFIER, "Expected variable name").value;
    std::shared_ptr<ExpressionNode> initializer = nullptr;

    if (match(TokenType::OP_ASSIGN)) {
        initializer = parseExpression();
    }

    match(TokenType::SEMICOLON); // Semicolon is optional but recommended
    return std::make_shared<VarDeclNode>(isFirm, type, name, initializer, loc);
}

std::shared_ptr<StatementNode> Parser::parseAssignmentOrSuffix() {
    SourceLocation loc = peek().location;
    
    // Check if it's a function call directly as statement (e.g. print_all();)
    if (peek().type == TokenType::IDENTIFIER && peek(1).type == TokenType::LPAREN) {
        std::string callee = advance().value;
        consume(TokenType::LPAREN, "Expected '('");
        std::vector<std::shared_ptr<ExpressionNode>> args;
        if (!check(TokenType::RPAREN)) {
            do {
                args.push_back(parseExpression());
            } while (match(TokenType::COMMA));
        }
        consume(TokenType::RPAREN, "Expected ')'");
        match(TokenType::SEMICOLON); // Option separator
        auto callNode = std::make_shared<CallNode>(callee, args, loc);
        return std::make_shared<CallStmtNode>(callNode, loc);
    }

    std::shared_ptr<ExpressionNode> left = parseExpression();

    // Suffix checks on identifiers
    if (auto idLeft = std::dynamic_pointer_cast<IdentifierNode>(left)) {
        if (match(TokenType::OP_INCREMENT)) {
            match(TokenType::SEMICOLON);
            return std::make_shared<SuffixOpNode>(idLeft->name, "+>", loc);
        } else if (match(TokenType::OP_DECREMENT)) {
            match(TokenType::SEMICOLON);
            return std::make_shared<SuffixOpNode>(idLeft->name, "-<", loc);
        }
    }

    if (match(TokenType::OP_ASSIGN)) {
        std::shared_ptr<ExpressionNode> right = parseExpression();
        match(TokenType::SEMICOLON);
        return std::make_shared<AssignNode>(left, right, loc);
    }

    throw std::runtime_error("Unknown expression statement beginning at token " + peek().value);
}

std::shared_ptr<BlockNode> Parser::parseBlock() {
    SourceLocation loc = consume(TokenType::LBRACE, "Expected '{'").location;
    std::vector<std::shared_ptr<StatementNode>> statements;

    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        statements.push_back(parseStatement());
    }

    consume(TokenType::RBRACE, "Expected '}'");
    return std::make_shared<BlockNode>(statements, loc);
}

std::shared_ptr<IfNode> Parser::parseIf() {
    SourceLocation loc = consume(TokenType::IF, "Expected 'if'").location;
    consume(TokenType::LPAREN, "Expected '(' after if");
    std::shared_ptr<ExpressionNode> condition = parseExpression();
    consume(TokenType::RPAREN, "Expected ')' after condition");

    std::shared_ptr<StatementNode> thenBr = parseStatement();
    std::vector<std::pair<std::shared_ptr<ExpressionNode>, std::shared_ptr<StatementNode>>> altifs;
    std::shared_ptr<StatementNode> elseBr = nullptr;

    while (match(TokenType::ALTIF)) {
        consume(TokenType::LPAREN, "Expected '(' after altif");
        auto altifCond = parseExpression();
        consume(TokenType::RPAREN, "Expected ')'");
        auto altifStmt = parseStatement();
        altifs.push_back({altifCond, altifStmt});
    }

    if (match(TokenType::ELSE)) {
        elseBr = parseStatement();
    }

    return std::make_shared<IfNode>(condition, thenBr, altifs, elseBr, loc);
}

std::shared_ptr<ReturnNode> Parser::parseReturn() {
    SourceLocation loc = consume(TokenType::RETURN, "Expected 'return'").location;
    bool isFin = false;
    std::shared_ptr<ExpressionNode> expr = nullptr;

    if (match(TokenType::FIN)) {
        isFin = true;
    } else if (!check(TokenType::SEMICOLON) && !check(TokenType::RBRACE)) {
        expr = parseExpression();
    }

    match(TokenType::SEMICOLON);
    return std::make_shared<ReturnNode>(isFin, expr, loc);
}

std::shared_ptr<RecNode> Parser::parseRec() {
    SourceLocation loc = consume(TokenType::REC, "Expected 'rec'").location;
    std::string name = consume(TokenType::IDENTIFIER, "Expected struct name").value;
    consume(TokenType::LBRACE, "Expected '{'");

    std::vector<std::pair<std::string, std::string>> fields;
    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        std::string fldType = consume(TokenType::IDENTIFIER, "Expected field type").value;
        std::string fldName = consume(TokenType::IDENTIFIER, "Expected field name").value;
        fields.push_back({fldType, fldName});
        match(TokenType::SEMICOLON);
    }

    consume(TokenType::RBRACE, "Expected '}'");
    return std::make_shared<RecNode>(name, fields, loc);
}

std::shared_ptr<CraftNode> Parser::parseCraft() {
    SourceLocation loc = consume(TokenType::CRAFT, "Expected 'craft'").location;
    std::string returnType = "void";

    // Support 'craft returnType funcName(...)'
    if (peek().type == TokenType::IDENTIFIER && peek(1).type == TokenType::IDENTIFIER) {
        returnType = advance().value;
    }

    std::string name = consume(TokenType::IDENTIFIER, "Expected function name").value;
    consume(TokenType::LPAREN, "Expected '('");

    std::vector<std::pair<std::string, std::string>> params;
    if (!check(TokenType::RPAREN)) {
        do {
            std::string paramType = consume(TokenType::IDENTIFIER, "Expected parameter type").value;
            std::string paramName = consume(TokenType::IDENTIFIER, "Expected parameter name").value;
            params.push_back({paramType, paramName});
        } while (match(TokenType::COMMA));
    }

    consume(TokenType::RPAREN, "Expected ')'");
    auto body = parseBlock();

    return std::make_shared<CraftNode>(name, returnType, params, body, loc);
}

std::shared_ptr<RescAttackNode> Parser::parseRescAttack() {
    SourceLocation loc = consume(TokenType::RESC, "Expected 'resc'").location;
    auto rescueBody = parseBlock();

    std::string errVar = "error";
    if (match(TokenType::COLON)) {
        errVar = consume(TokenType::IDENTIFIER, "Expected rescue variable name").value;
    }

    consume(TokenType::ATTACK, "Expected rescue block handler keyword 'attack'");
    auto handlerBody = parseBlock();

    return std::make_shared<RescAttackNode>(rescueBody, errVar, handlerBody, loc);
}

std::shared_ptr<AttackNode> Parser::parseAttack() {
    SourceLocation loc = peek(-1).location;
    std::shared_ptr<ExpressionNode> errExpr = parseExpression();
    match(TokenType::SEMICOLON);
    return std::make_shared<AttackNode>(errExpr, loc);
}

std::shared_ptr<RawBlockNode> Parser::parseRaw() {
    SourceLocation loc = consume(TokenType::RAW, "Expected 'raw'").location;
    auto block = parseBlock();
    return std::make_shared<RawBlockNode>(block, loc);
}

std::shared_ptr<CoreNode> Parser::parseCore() {
    SourceLocation loc = peek(-1).location;
    auto block = parseBlock();
    return std::make_shared<CoreNode>(block, loc);
}

std::shared_ptr<ImprintNode> Parser::parseImprint() {
    SourceLocation loc = peek(-1).location;
    consume(TokenType::LPAREN, "Expected '(' after Imprint");
    std::shared_ptr<ExpressionNode> expr = parseExpression();
    consume(TokenType::RPAREN, "Expected ')' after parameters");
    match(TokenType::SEMICOLON);
    return std::make_shared<ImprintNode>(expr, loc);
}

// Expr precedence
std::shared_ptr<ExpressionNode> Parser::parseExpression() {
    return parseEquality();
}

std::shared_ptr<ExpressionNode> Parser::parseEquality() {
    auto left = parseComparison();
    while (match(TokenType::OP_EQ) || match(TokenType::OP_NE)) {
        std::string op = peek(-1).value;
        auto right = parseComparison();
        left = std::make_shared<BinaryOpNode>(op, left, right, left->location);
    }
    return left;
}

std::shared_ptr<ExpressionNode> Parser::parseComparison() {
    auto left = parseTerm();
    while (match(TokenType::OP_LT) || match(TokenType::OP_GT) || 
           match(TokenType::OP_LE) || match(TokenType::OP_GE)) {
        std::string op = peek(-1).value;
        auto right = parseTerm();
        left = std::make_shared<BinaryOpNode>(op, left, right, left->location);
    }
    return left;
}

std::shared_ptr<ExpressionNode> Parser::parseTerm() {
    auto left = parseFactor();
    while (match(TokenType::OP_PLUS) || match(TokenType::OP_MINUS)) {
        std::string op = peek(-1).value;
        auto right = parseFactor();
        left = std::make_shared<BinaryOpNode>(op, left, right, left->location);
    }
    return left;
}

std::shared_ptr<ExpressionNode> Parser::parseFactor() {
    auto left = parseUnary();
    while (match(TokenType::OP_MUL) || match(TokenType::OP_DIV)) {
        std::string op = peek(-1).value;
        auto right = parseUnary();
        left = std::make_shared<BinaryOpNode>(op, left, right, left->location);
    }
    return left;
}

std::shared_ptr<ExpressionNode> Parser::parseUnary() {
    if (match(TokenType::OP_MINUS)) {
        SourceLocation loc = peek(-1).location;
        auto expr = parseUnary();
        return std::make_shared<UnaryOpNode>("-", expr, loc);
    }

    if (match(TokenType::REF)) {
        SourceLocation loc = peek(-1).location;
        consume(TokenType::LPAREN, "Expected '(' after ref");
        auto expr = parseExpression();
        consume(TokenType::RPAREN, "Expected ')'");
        return std::make_shared<ReferenceNode>(expr, loc);
    }

    if (match(TokenType::PTR)) {
        SourceLocation loc = peek(-1).location;
        consume(TokenType::LPAREN, "Expected '(' after ptr");
        auto expr = parseExpression();
        consume(TokenType::RPAREN, "Expected ')'");
        return std::make_shared<PointerNode>(expr, loc);
    }

    return parsePrimary();
}

std::shared_ptr<ExpressionNode> Parser::parsePrimary() {
    SourceLocation loc = peek().location;

    if (match(TokenType::LITERAL_INT)) {
        return std::make_shared<LiteralIntNode>(std::stoi(peek(-1).value), loc);
    }

    if (match(TokenType::LITERAL_FLOAT)) {
        return std::make_shared<LiteralFloatNode>(std::stod(peek(-1).value), loc);
    }

    if (match(TokenType::LITERAL_STRING)) {
        return std::make_shared<LiteralStringNode>(peek(-1).value, loc);
    }

    if (match(TokenType::SCANQ)) {
        consume(TokenType::LPAREN, "Expected '(' after scanq");
        consume(TokenType::RPAREN, "Expected ')'");
        return std::make_shared<ScanqNode>(loc);
    }

    if (match(TokenType::IDENTIFIER)) {
        std::string name = peek(-1).value;
        
        // Check array index indexing x[5]
        if (match(TokenType::LBRACKET)) {
            auto idx = parseExpression();
            consume(TokenType::RBRACKET, "Expected ']'");
            // Treat as identifier + indexing operation
            auto baseNode = std::make_shared<IdentifierNode>(name, loc);
            return std::make_shared<BinaryOpNode>("[]", baseNode, idx, loc);
        }

        // Inline function calls "myFunc()"
        if (match(TokenType::LPAREN)) {
            std::vector<std::shared_ptr<ExpressionNode>> args;
            if (!check(TokenType::RPAREN)) {
                do {
                    args.push_back(parseExpression());
                } while (match(TokenType::COMMA));
            }
            consume(TokenType::RPAREN, "Expected ')'");
            return std::make_shared<CallNode>(name, args, loc);
        }

        return std::make_shared<IdentifierNode>(name, loc);
    }

    if (match(TokenType::LPAREN)) {
        auto expr = parseExpression();
        consume(TokenType::RPAREN, "Expected ')' to close parenthesis");
        return expr;
    }

    throw std::runtime_error("Expected primary expression (literal, variable, or parenthesis) at token '" + peek().value + "'");
}

} // namespace Riven
