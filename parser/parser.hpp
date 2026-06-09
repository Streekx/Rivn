#ifndef RIVEN_PARSER_HPP
#define RIVEN_PARSER_HPP

#include "../lexer/lexer.hpp"
#include "../ast/ast.hpp"
#include <memory>
#include <vector>

namespace Riven {

class Parser {
private:
    std::vector<Token> tokens;
    size_t current;

    Token peek(size_t offset = 0) const;
    Token advance();
    bool isAtEnd() const;
    bool check(TokenType type) const;
    bool match(TokenType type);
    Token consume(TokenType type, std::string message);

    // Parsing methods
    std::shared_ptr<ASTNode> parseTopLevel();
    std::shared_ptr<StatementNode> parseStatement();
    std::shared_ptr<VarDeclNode> parseVarDecl();
    std::shared_ptr<StatementNode> parseAssignmentOrSuffix();
    std::shared_ptr<BlockNode> parseBlock();
    std::shared_ptr<IfNode> parseIf();
    std::shared_ptr<ReturnNode> parseReturn();
    std::shared_ptr<RecNode> parseRec();
    std::shared_ptr<CraftNode> parseCraft();
    std::shared_ptr<RescAttackNode> parseRescAttack();
    std::shared_ptr<AttackNode> parseAttack();
    std::shared_ptr<RawBlockNode> parseRaw();
    std::shared_ptr<CoreNode> parseCore();
    std::shared_ptr<ImprintNode> parseImprint();

    // Expressions
    std::shared_ptr<ExpressionNode> parseExpression();
    std::shared_ptr<ExpressionNode> parseEquality();
    std::shared_ptr<ExpressionNode> parseComparison();
    std::shared_ptr<ExpressionNode> parseTerm();
    std::shared_ptr<ExpressionNode> parseFactor();
    std::shared_ptr<ExpressionNode> parseUnary();
    std::shared_ptr<ExpressionNode> parsePrimary();

public:
    Parser(std::vector<Token> tokens);
    std::shared_ptr<ProgramNode> parse();
};

} // namespace Riven

#endif // RIVEN_PARSER_HPP
