#ifndef RIVEN_AST_HPP
#define RIVEN_AST_HPP

#include "../lexer/lexer.hpp"
#include <memory>
#include <string>
#include <vector>

namespace Riven {

struct ASTVisitor;

struct ASTNode {
    SourceLocation location;
    virtual ~ASTNode() = default;
    virtual void accept(ASTVisitor* visitor) = 0;
};

struct ExpressionNode : public ASTNode {};
struct StatementNode : public ASTNode {};

struct LiteralIntNode : public ExpressionNode {
    int value;
    LiteralIntNode(int val, SourceLocation loc) : value(val) { location = loc; }
    void accept(ASTVisitor* visitor) override;
};

struct LiteralFloatNode : public ExpressionNode {
    double value;
    LiteralFloatNode(double val, SourceLocation loc) : value(val) { location = loc; }
    void accept(ASTVisitor* visitor) override;
};

struct LiteralStringNode : public ExpressionNode {
    std::string value;
    LiteralStringNode(std::string val, SourceLocation loc) : value(val) { location = loc; }
    void accept(ASTVisitor* visitor) override;
};

struct IdentifierNode : public ExpressionNode {
    std::string name;
    IdentifierNode(std::string nm, SourceLocation loc) : name(nm) { location = loc; }
    void accept(ASTVisitor* visitor) override;
};

struct BinaryOpNode : public ExpressionNode {
    std::string op;
    std::shared_ptr<ExpressionNode> left;
    std::shared_ptr<ExpressionNode> right;
    BinaryOpNode(std::string op, std::shared_ptr<ExpressionNode> l, std::shared_ptr<ExpressionNode> r, SourceLocation loc)
        : op(op), left(l), right(r) { location = loc; }
    void accept(ASTVisitor* visitor) override;
};

struct UnaryOpNode : public ExpressionNode {
    std::string op;
    std::shared_ptr<ExpressionNode> expr;
    UnaryOpNode(std::string op, std::shared_ptr<ExpressionNode> e, SourceLocation loc)
        : op(op), expr(e) { location = loc; }
    void accept(ASTVisitor* visitor) override;
};

struct ReferenceNode : public ExpressionNode {
    std::shared_ptr<ExpressionNode> expr; // ref data
    std::string lifetimeName; // Compile-time static lifetime tracker 'a
    ReferenceNode(std::shared_ptr<ExpressionNode> e, SourceLocation loc, std::string lt = "")
        : expr(e), lifetimeName(lt) { location = loc; }
    void accept(ASTVisitor* visitor) override;
};

struct PointerNode : public ExpressionNode {
    std::shared_ptr<ExpressionNode> expr; // ptr memory
    PointerNode(std::shared_ptr<ExpressionNode> e, SourceLocation loc)
        : expr(e) { location = loc; }
    void accept(ASTVisitor* visitor) override;
};

struct ScanqNode : public ExpressionNode {
    ScanqNode(SourceLocation loc) { location = loc; }
    void accept(ASTVisitor* visitor) override;
};

struct ImportNode : public StatementNode {
    std::string filepath;
    ImportNode(std::string path, SourceLocation loc) : filepath(path) { location = loc; }
    void accept(ASTVisitor* visitor) override;
};

struct VarDeclNode : public StatementNode {
    bool isFirm; // constant or borrow lease rules
    std::string type;
    std::string name;
    std::shared_ptr<ExpressionNode> initializer;
    bool isBorrowMutable = false; // &mut tracking
    std::string explicitLifetime = ""; // 'a
    VarDeclNode(bool firm, std::string t, std::string n, std::shared_ptr<ExpressionNode> init, SourceLocation loc)
        : isFirm(firm), type(t), name(n), initializer(init) { location = loc; }
    void accept(ASTVisitor* visitor) override;
};

struct AssignNode : public StatementNode {
    std::shared_ptr<ExpressionNode> target;
    std::shared_ptr<ExpressionNode> value;
    AssignNode(std::shared_ptr<ExpressionNode> tgt, std::shared_ptr<ExpressionNode> val, SourceLocation loc)
        : target(tgt), value(val) { location = loc; }
    void accept(ASTVisitor* visitor) override;
};

struct SuffixOpNode : public StatementNode {
    std::string name;
    std::string op; // "+>" or "-<"
    SuffixOpNode(std::string n, std::string o, SourceLocation loc) : name(n), op(o) { location = loc; }
    void accept(ASTVisitor* visitor) override;
};

struct ImprintNode : public StatementNode {
    std::shared_ptr<ExpressionNode> expr;
    ImprintNode(std::shared_ptr<ExpressionNode> e, SourceLocation loc) : expr(e) { location = loc; }
    void accept(ASTVisitor* visitor) override;
};

struct BlockNode : public StatementNode {
    std::vector<std::shared_ptr<StatementNode>> statements;
    BlockNode(std::vector<std::shared_ptr<StatementNode>> stmts, SourceLocation loc) : statements(stmts) { location = loc; }
    void accept(ASTVisitor* visitor) override;
};

struct IfNode : public StatementNode {
    std::shared_ptr<ExpressionNode> condition;
    std::shared_ptr<StatementNode> thenBranch;
    std::vector<std::pair<std::shared_ptr<ExpressionNode>, std::shared_ptr<StatementNode>>> altifBranches;
    std::shared_ptr<StatementNode> elseBranch;
    IfNode(std::shared_ptr<ExpressionNode> cond, std::shared_ptr<StatementNode> thenBr,
           std::vector<std::pair<std::shared_ptr<ExpressionNode>, std::shared_ptr<StatementNode>>> altifs,
           std::shared_ptr<StatementNode> elseBr, SourceLocation loc)
        : condition(cond), thenBranch(thenBr), altifBranches(altifs), elseBranch(elseBr) { location = loc; }
    void accept(ASTVisitor* visitor) override;
};

struct ReturnNode : public StatementNode {
    bool isFin; // return fin
    std::shared_ptr<ExpressionNode> expr;
    ReturnNode(bool fin, std::shared_ptr<ExpressionNode> e, SourceLocation loc) : isFin(fin), expr(e) { location = loc; }
    void accept(ASTVisitor* visitor) override;
};

struct RecNode : public StatementNode {
    std::string name;
    std::vector<std::pair<std::string, std::string>> fields; // {Type, Name}
    std::vector<std::string> typeParams; // Generic parameters like T, U (Generics!)
    RecNode(std::string nm, std::vector<std::pair<std::string, std::string>> flds, SourceLocation loc)
        : name(nm), fields(flds) { location = loc; }
    void accept(ASTVisitor* visitor) override;
};

struct CallNode : public ExpressionNode {
    std::string callee;
    std::vector<std::shared_ptr<ExpressionNode>> args;
    std::vector<std::string> genericArgs; // Monomorphized concrete type arguments like <int>
    CallNode(std::string cl, std::vector<std::shared_ptr<ExpressionNode>> a, SourceLocation loc)
        : callee(cl), args(a) { location = loc; }
    void accept(ASTVisitor* visitor) override;
};

struct CallStmtNode : public StatementNode {
    std::shared_ptr<CallNode> call;
    CallStmtNode(std::shared_ptr<CallNode> c, SourceLocation loc) : call(c) { location = loc; }
    void accept(ASTVisitor* visitor) override;
};

struct CraftNode : public StatementNode {
    std::string name;
    std::string returnType;
    std::vector<std::pair<std::string, std::string>> params; // {Type, Name}
    std::shared_ptr<BlockNode> body;
    std::vector<std::string> typeParams; // Generic function support
    CraftNode(std::string n, std::string rt, std::vector<std::pair<std::string, std::string>> p, std::shared_ptr<BlockNode> b, SourceLocation loc)
        : name(n), returnType(rt), params(p), body(b) { location = loc; }
    void accept(ASTVisitor* visitor) override;
};

struct RescAttackNode : public StatementNode {
    std::shared_ptr<BlockNode> tryBlock;
    std::string errorVar;
    std::shared_ptr<BlockNode> rescueBlock;
    RescAttackNode(std::shared_ptr<BlockNode> tryBl, std::string errVar, std::shared_ptr<BlockNode> rescBl, SourceLocation loc)
        : tryBlock(tryBl), errorVar(errVar), rescueBlock(rescBl) { location = loc; }
    void accept(ASTVisitor* visitor) override;
};

struct AttackNode : public StatementNode {
    std::shared_ptr<ExpressionNode> errorExpr;
    AttackNode(std::shared_ptr<ExpressionNode> err, SourceLocation loc) : errorExpr(err) { location = loc; }
    void accept(ASTVisitor* visitor) override;
};

struct RawBlockNode : public StatementNode {
    std::shared_ptr<BlockNode> body;
    RawBlockNode(std::shared_ptr<BlockNode> b, SourceLocation loc) : body(b) { location = loc; }
    void accept(ASTVisitor* visitor) override;
};

struct CoreNode : public StatementNode {
    std::shared_ptr<BlockNode> body;
    CoreNode(std::shared_ptr<BlockNode> b, SourceLocation loc) : body(b) { location = loc; }
    void accept(ASTVisitor* visitor) override;
};

struct TraitNode : public StatementNode {
    std::string name;
    std::vector<std::shared_ptr<CraftNode>> interfaceMethods;
    TraitNode(std::string nm, std::vector<std::shared_ptr<CraftNode>> m, SourceLocation loc)
        : name(nm), interfaceMethods(m) { location = loc; }
    void accept(ASTVisitor* visitor) override;
};

struct MethodCallNode : public ExpressionNode {
    std::shared_ptr<ExpressionNode> instance;
    std::string methodName;
    std::vector<std::shared_ptr<ExpressionNode>> args;
    MethodCallNode(std::shared_ptr<ExpressionNode> inst, std::string mName, std::vector<std::shared_ptr<ExpressionNode>> a, SourceLocation loc)
        : instance(inst), methodName(mName), args(a) { location = loc; }
    void accept(ASTVisitor* visitor) override;
};

struct ProgramNode : public ASTNode {
    std::vector<std::shared_ptr<ASTNode>> nodes;
    ProgramNode(std::vector<std::shared_ptr<ASTNode>> nds, SourceLocation loc) : nodes(nds) { location = loc; }
    void accept(ASTVisitor* visitor) override;
};

struct ASTVisitor {
    virtual void visit(LiteralIntNode* node) = 0;
    virtual void visit(LiteralFloatNode* node) = 0;
    virtual void visit(LiteralStringNode* node) = 0;
    virtual void visit(IdentifierNode* node) = 0;
    virtual void visit(BinaryOpNode* node) = 0;
    virtual void visit(UnaryOpNode* node) = 0;
    virtual void visit(ReferenceNode* node) = 0;
    virtual void visit(PointerNode* node) = 0;
    virtual void visit(ScanqNode* node) = 0;
    virtual void visit(ImportNode* node) = 0;
    virtual void visit(VarDeclNode* node) = 0;
    virtual void visit(AssignNode* node) = 0;
    virtual void visit(SuffixOpNode* node) = 0;
    virtual void visit(ImprintNode* node) = 0;
    virtual void visit(BlockNode* node) = 0;
    virtual void visit(IfNode* node) = 0;
    virtual void visit(ReturnNode* node) = 0;
    virtual void visit(RecNode* node) = 0;
    virtual void visit(CallNode* node) = 0;
    virtual void visit(CallStmtNode* node) = 0;
    virtual void visit(CraftNode* node) = 0;
    virtual void visit(RescAttackNode* node) = 0;
    virtual void visit(AttackNode* node) = 0;
    virtual void visit(RawBlockNode* node) = 0;
    virtual void visit(CoreNode* node) = 0;
    virtual void visit(TraitNode* node) = 0;
    virtual void visit(MethodCallNode* node) = 0;
    virtual void visit(ProgramNode* node) = 0;
};

inline void LiteralIntNode::accept(ASTVisitor* v) { v->visit(this); }
inline void LiteralFloatNode::accept(ASTVisitor* v) { v->visit(this); }
inline void LiteralStringNode::accept(ASTVisitor* v) { v->visit(this); }
inline void IdentifierNode::accept(ASTVisitor* v) { v->visit(this); }
inline void BinaryOpNode::accept(ASTVisitor* v) { v->visit(this); }
inline void UnaryOpNode::accept(ASTVisitor* v) { v->visit(this); }
inline void ReferenceNode::accept(ASTVisitor* v) { v->visit(this); }
inline void PointerNode::accept(ASTVisitor* v) { v->visit(this); }
inline void ScanqNode::accept(ASTVisitor* v) { v->visit(this); }
inline void ImportNode::accept(ASTVisitor* v) { v->visit(this); }
inline void VarDeclNode::accept(ASTVisitor* v) { v->visit(this); }
inline void AssignNode::accept(ASTVisitor* v) { v->visit(this); }
inline void SuffixOpNode::accept(ASTVisitor* v) { v->visit(this); }
inline void ImprintNode::accept(ASTVisitor* v) { v->visit(this); }
inline void BlockNode::accept(ASTVisitor* v) { v->visit(this); }
inline void IfNode::accept(ASTVisitor* v) { v->visit(this); }
inline void ReturnNode::accept(ASTVisitor* v) { v->visit(this); }
inline void RecNode::accept(ASTVisitor* v) { v->visit(this); }
inline void CallNode::accept(ASTVisitor* v) { v->visit(this); }
inline void CallStmtNode::accept(ASTVisitor* v) { v->visit(this); }
inline void CraftNode::accept(ASTVisitor* v) { v->visit(this); }
inline void RescAttackNode::accept(ASTVisitor* v) { v->visit(this); }
inline void AttackNode::accept(ASTVisitor* v) { v->visit(this); }
inline void RawBlockNode::accept(ASTVisitor* v) { v->visit(this); }
inline void CoreNode::accept(ASTVisitor* v) { v->visit(this); }
inline void TraitNode::accept(ASTVisitor* v) { v->visit(this); }
inline void MethodCallNode::accept(ASTVisitor* v) { v->visit(this); }
inline void ProgramNode::accept(ASTVisitor* v) { v->visit(this); }

} // namespace Riven

#endif // RIVEN_AST_HPP
