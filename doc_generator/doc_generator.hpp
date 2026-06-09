#ifndef RIVEN_DOC_GENERATOR_HPP
#define RIVEN_DOC_GENERATOR_HPP

#include "../ast/ast.hpp"
#include <string>
#include <vector>
#include <sstream>

namespace Riven {

class DocGenerator : public ASTVisitor {
private:
    std::stringstream ss;

public:
    std::string generateDocs(std::shared_ptr<ProgramNode> root, const std::string& moduleName = "Main module") {
        ss.str("");
        ss.clear();
        ss << "# API Documentation: " << moduleName << "\n\n";
        ss << "This document was automatically compiled from Riven codebase metadata annotations.\n\n";
        
        if (root) root->accept(this);

        ss << "\n---\n*Riven Language Toolchain v0.9.3 - Auto Generator*\n";
        return ss.str();
    }

    void visit(LiteralIntNode* node) override {}
    void visit(LiteralFloatNode* node) override {}
    void visit(LiteralStringNode* node) override {}
    void visit(IdentifierNode* node) override {}
    void visit(BinaryOpNode* node) override {}
    void visit(UnaryOpNode* node) override {}
    void visit(ReferenceNode* node) override {}
    void visit(PointerNode* node) override {}
    void visit(ScanqNode* node) override {}
    void visit(ImportNode* node) override {}
    void visit(VarDeclNode* node) override {
        ss << "- **Variable** `" << node->name << "` of type `" << node->type << "` (" << (node->isFirm ? "firm / constant" : "mutable") << ")\n";
    }
    void visit(AssignNode* node) override {}
    void visit(SuffixOpNode* node) override {}
    void visit(ImprintNode* node) override {}
    void visit(BlockNode* node) override {
        for (auto& st : node->statements) st->accept(this);
    }
    void visit(IfNode* node) override {}
    void visit(ReturnNode* node) override {}
    
    void visit(RecNode* node) override {
        ss << "### Record `" << node->name << "`\n\n";
        ss << "Systems record grouping multiple values. Layout details:\n\n";
        ss << "| Field Type | Field Name |\n";
        ss << "|---|---|\n";
        for (auto const& fld : node->fields) {
            ss << "| `" << fld.first << "` | `" << fld.second << "` |\n";
        }
        ss << "\n";
    }

    void visit(CallNode* node) override {}
    void visit(CallStmtNode* node) override {}
    
    void visit(CraftNode* node) override {
        ss << "### Craft Function `" << node->name << "`\n\n";
        ss << "**Signature:** `craft " << node->returnType << " " << node->name << "(";
        for (size_t i = 0; i < node->params.size(); ++i) {
            ss << node->params[i].first << " " << node->params[i].second;
            if (i + 1 < node->params.size()) ss << ", ";
        }
        ss << ")`\n\n";
        ss << "- **Returns:** `" << node->returnType << "`\n";
        if (!node->params.empty()) {
            ss << "- **Parameters:**\n";
            for (auto const& p : node->params) {
                ss << "  - `" << p.second << "` - type `" << p.first << "`\n";
            }
        }
        ss << "\n";
    }

    void visit(RescAttackNode* node) override {
        node->tryBlock->accept(this);
        node->rescueBlock->accept(this);
    }

    void visit(AttackNode* node) override {}
    void visit(RawBlockNode* node) override {
        node->body->accept(this);
    }
    void visit(CoreNode* node) override {
        node->body->accept(this);
    }
    
    void visit(TraitNode* node) override {
        ss << "### Trait Interface `" << node->name << "`\n\n";
        ss << "Defines a compile-time contractual trait interface containing methods:\n\n";
        for (auto const& method : node->interfaceMethods) {
            ss << "- `" << method->name << "` returning `" << method->returnType << "`\n";
        }
        ss << "\n";
    }
    
    void visit(MethodCallNode* node) override {}

    void visit(ProgramNode* node) override {
        for (auto& n : node->nodes) n->accept(this);
    }
};

} // namespace Riven

#endif // RIVEN_DOC_GENERATOR_HPP
