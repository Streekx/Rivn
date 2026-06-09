#ifndef RIVEN_SEMANTIC_HPP
#define RIVEN_SEMANTIC_HPP

#include "../ast/ast.hpp"
#include <map>
#include <string>
#include <vector>
#include <set>
#include <unordered_set>
#include <unordered_map>

namespace Riven {

struct Symbol {
    std::string name;
    std::string type;
    bool isFirm = false;       // constant
    bool isBorrowMutable = false; // &mut tracking
    bool isUnused = true;
    std::string lifetime = ""; // Associated lifetime 'a
    SourceLocation declaredLoc;
};

// Represents a compile-time active loan or lease of memory (Riven borrowing model)
struct BorrowLease {
    std::string variableName;
    bool isMutable;
    std::string lifetimeScope;
};

class SymbolTable {
private:
    std::vector<std::map<std::string, Symbol>> scopes;

public:
    SymbolTable();
    void pushScope();
    void popScope();
    bool insert(const std::string& name, const Symbol& sym);
    bool lookup(const std::string& name, Symbol& foundSym);
    bool update(const std::string& name, const Symbol& sym);
    std::map<std::string, Symbol> getCurrentScope() const;
    size_t getDepth() const { return scopes.size(); }
};

class SemanticAnalyzer : public ASTVisitor {
private:
    SymbolTable symTable;
    std::vector<std::string> semanticErrors;
    std::vector<std::string> linterWarnings;
    bool inRawBlock;
    std::string currentReturnType;
    bool hasReturn;

    // Trackers for multi-paradigm typing safety
    std::unordered_set<std::string> declaredRecords;
    std::unordered_map<std::string, std::shared_ptr<RecNode>> recordNodes;
    std::unordered_map<std::string, std::shared_ptr<TraitNode>> declaredTraits;
    
    // Generics and monomorphization registries
    std::unordered_map<std::string, std::shared_ptr<CraftNode>> genericCrafts; // Generic function templates
    std::unordered_set<std::string> monomorphizedInstances;

    // Borrow checker active tables
    std::vector<BorrowLease> activeBorrows;

    // Helper functions for advanced type validating
    void checkBorrowIntegrity(const std::string& varName, bool reqMutable, SourceLocation loc);
    void endBorrowLease(const std::string& lifetime);

public:
    SemanticAnalyzer();
    void analyze(std::shared_ptr<ProgramNode> root);

    const std::vector<std::string>& getErrors() const { return semanticErrors; }
    const std::vector<std::string>& getWarnings() const { return linterWarnings; }

    void visit(LiteralIntNode* node) override;
    void visit(LiteralFloatNode* node) override;
    void visit(LiteralStringNode* node) override;
    void visit(IdentifierNode* node) override;
    void visit(BinaryOpNode* node) override;
    void visit(UnaryOpNode* node) override;
    void visit(ReferenceNode* node) override;
    void visit(PointerNode* node) override;
    void visit(ScanqNode* node) override;
    void visit(ImportNode* node) override;
    void visit(VarDeclNode* node) override;
    void visit(AssignNode* node) override;
    void visit(SuffixOpNode* node) override;
    void visit(ImprintNode* node) override;
    void visit(BlockNode* node) override;
    void visit(IfNode* node) override;
    void visit(ReturnNode* node) override;
    void visit(RecNode* node) override;
    void visit(CallNode* node) override;
    void visit(CallStmtNode* node) override;
    void visit(CraftNode* node) override;
    void visit(RescAttackNode* node) override;
    void visit(AttackNode* node) override;
    void visit(RawBlockNode* node) override;
    void visit(CoreNode* node) override;
    void visit(TraitNode* node) override;
    void visit(MethodCallNode* node) override;
    void visit(ProgramNode* node) override;
};

} // namespace Riven

#endif // RIVEN_SEMANTIC_HPP
