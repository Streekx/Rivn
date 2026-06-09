#ifndef RIVEN_LINTER_HPP
#define RIVEN_LINTER_HPP

#include "../ast/ast.hpp"
#include <string>
#include <vector>

namespace Riven {

struct LinterIssue {
    enum class Severity { WARNING, CODE_SMELL };
    Severity severity;
    std::string message;
    size_t line;
    size_t col;
};

class Linter : public ASTVisitor {
private:
    std::vector<LinterIssue> issues;
    bool inRawBlock = false;

public:
    std::vector<LinterIssue> lint(std::shared_ptr<ProgramNode> root) {
        issues.clear();
        if (root) root->accept(this);
        return issues;
    }

    void visit(LiteralIntNode* node) override {}
    void visit(LiteralFloatNode* node) override {}
    void visit(LiteralStringNode* node) override {}
    void visit(IdentifierNode* node) override {}
    
    void visit(BinaryOpNode* node) override {
        node->left->accept(this);
        node->right->accept(this);
    }
    
    void visit(UnaryOpNode* node) override { 
        node->expr->accept(this); 
    }
    
    void visit(ReferenceNode* node) override {
        node->expr->accept(this);
        if (!inRawBlock) {
            issues.push_back({LinterIssue::Severity::CODE_SMELL, 
                "Taking reference '&' outside raw scope. High possibility of pointer dangling.", 
                node->location.line, node->location.column});
        }
    }

    void visit(PointerNode* node) override {
        node->expr->accept(this);
        if (!inRawBlock) {
            issues.push_back({LinterIssue::Severity::WARNING, 
                "Unsafe pointer manipulation outside raw scope. Encase pointers in 'raw { ... }' for compilation safety.", 
                node->location.line, node->location.column});
        }
    }

    void visit(ScanqNode* node) override {}
    void visit(ImportNode* node) override {}
    
    void visit(VarDeclNode* node) override {
        if (node->initializer) node->initializer->accept(this);
        if (!node->isFirm && node->type == "auto") {
            issues.push_back({LinterIssue::Severity::CODE_SMELL, 
                "Mutable auto variable declaration '" + node->name + "'. Standard Riven rules advise marking variables as solid 'firm' if value is read-only.", 
                node->location.line, node->location.column});
        }
    }

    void visit(AssignNode* node) override {
        node->target->accept(this);
        node->value->accept(this);
    }

    void visit(SuffixOpNode* node) override {}
    void visit(ImprintNode* node) override { node->expr->accept(this); }
    
    void visit(BlockNode* node) override {
        for (auto& st : node->statements) st->accept(this);
    }
    
    void visit(IfNode* node) override {
        node->condition->accept(this);
        node->thenBranch->accept(this);
        for (auto& a : node->altifBranches) {
            a.first->accept(this);
            a.second->accept(this);
        }
        if (node->elseBranch) node->elseBranch->accept(this);
    }
    
    void visit(ReturnNode* node) override {
        if (node->expr) node->expr->accept(this);
    }
    
    void visit(RecNode* node) override {}
    
    void visit(CallNode* node) override {
        for (auto& a : node->args) a->accept(this);
    }
    
    void visit(CallStmtNode* node) override { node->call->accept(this); }
    void visit(CraftNode* node) override { node->body->accept(this); }
    
    void visit(RescAttackNode* node) override {
        node->tryBlock->accept(this);
        node->rescueBlock->accept(this);
    }

    void visit(AttackNode* node) override {
        node->errorExpr->accept(this);
        if (inRawBlock) {
            issues.push_back({LinterIssue::Severity::WARNING, 
                "Raising exceptions using 'attack' within 'raw' block is highly volatile. Raw blocks should contain minimal system interactions.", 
                node->location.line, node->location.column});
        }
    }

    void visit(RawBlockNode* node) override {
        bool prev = inRawBlock;
        inRawBlock = true;
        node->body->accept(this);
        inRawBlock = prev;
    }

    void visit(CoreNode* node) override { node->body->accept(this); }
    
    void visit(TraitNode* node) override {
        if (node->interfaceMethods.empty()) {
            issues.push_back({LinterIssue::Severity::CODE_SMELL,
                "Defining empty trait interface '" + node->name + "'. This adds zero polymorphism capabilities.",
                node->location.line, node->location.column});
        }
    }
    
    void visit(MethodCallNode* node) override {
        node->instance->accept(this);
        for (auto& a : node->args) {
            a->accept(this);
        }
    }

    void visit(ProgramNode* node) override {
        for (auto& n : node->nodes) n->accept(this);
    }
};

} // namespace Riven

#endif // RIVEN_LINTER_HPP
