#include "semantic.hpp"
#include <algorithm>
#include <sstream>

namespace Riven {

SymbolTable::SymbolTable() {
    pushScope(); // Global scope
}

void SymbolTable::pushScope() {
    scopes.push_back(std::map<std::string, Symbol>());
}

void SymbolTable::popScope() {
    if (scopes.size() > 1) {
        scopes.pop_back();
    }
}

bool SymbolTable::insert(const std::string& name, const Symbol& sym) {
    if (scopes.back().find(name) != scopes.back().end()) {
        return false; 
    }
    scopes.back()[name] = sym;
    return true;
}

bool SymbolTable::lookup(const std::string& name, Symbol& foundSym) {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        if (it->find(name) != it->end()) {
            foundSym = (*it)[name];
            // Mark symbol as used when looked up
            (*it)[name].isUnused = false;
            return true;
        }
    }
    return false;
}

bool SymbolTable::update(const std::string& name, const Symbol& sym) {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        if (it->find(name) != it->end()) {
            (*it)[name] = sym;
            return true;
        }
    }
    return false;
}

std::map<std::string, Symbol> SymbolTable::getCurrentScope() const {
    return scopes.empty() ? std::map<std::string, Symbol>() : scopes.back();
}


SemanticAnalyzer::SemanticAnalyzer() : inRawBlock(false), currentReturnType("void"), hasReturn(false) {}

void SemanticAnalyzer::analyze(std::shared_ptr<ProgramNode> root) {
    if (root) {
        root->accept(this);
    }
}

void SemanticAnalyzer::checkBorrowIntegrity(const std::string& varName, bool reqMutable, SourceLocation loc) {
    for (const auto& lease : activeBorrows) {
        if (lease.variableName == varName) {
            if (reqMutable) {
                // If we want to write/mutably borrow, we CANNOT have any existing borrows at all (either mutable or immutable)
                semanticErrors.push_back("Borrow Check Error at line " + std::to_string(loc.line) + 
                                         ": Cannot modify or mutably borrow '" + varName + 
                                         "' which has an active compile-time lease (borrowed as '" + 
                                         (lease.isMutable ? "mutable" : "immutable") + "').");
            } else {
                // If we want a read-only borrow, we cannot have an active MUTABLE borrow
                if (lease.isMutable) {
                    semanticErrors.push_back("Borrow Check Error at line " + std::to_string(loc.line) + 
                                             ": Cannot borrow '" + varName + "' as immutable because it has been leased as mutable.");
                }
            }
        }
    }
}

void SemanticAnalyzer::endBorrowLease(const std::string& lifetime) {
    auto it = std::remove_if(activeBorrows.begin(), activeBorrows.end(), [&](const BorrowLease& lease) {
        return lease.lifetimeScope == lifetime;
    });
    activeBorrows.erase(it, activeBorrows.end());
}

void SemanticAnalyzer::visit(LiteralIntNode* node) {}
void SemanticAnalyzer::visit(LiteralFloatNode* node) {}
void SemanticAnalyzer::visit(LiteralStringNode* node) {}

void SemanticAnalyzer::visit(IdentifierNode* node) {
    Symbol sym;
    if (!symTable.lookup(node->name, sym)) {
        semanticErrors.push_back("Semantic Error at line " + std::to_string(node->location.line) + 
                                 ": Undeclared variable '" + node->name + "'");
    } else {
        // Enforce basic read access check
        checkBorrowIntegrity(node->name, false, node->location);
    }
}

void SemanticAnalyzer::visit(BinaryOpNode* node) {
    node->left->accept(this);
    node->right->accept(this);
}

void SemanticAnalyzer::visit(UnaryOpNode* node) {
    node->expr->accept(this);
}

void SemanticAnalyzer::visit(ReferenceNode* node) {
    node->expr->accept(this);
    
    std::string referencedVar = "";
    if (auto idNode = std::dynamic_pointer_cast<IdentifierNode>(node->expr)) {
        referencedVar = idNode->name;
    }
    
    if (!referencedVar.empty()) {
        Symbol sym;
        if (symTable.lookup(referencedVar, sym)) {
            // Apply single-writer or multiple-reader safety loan checks
            checkBorrowIntegrity(referencedVar, true, node->location);
            
            // Register a borrow lease with local/explicit lifetime
            std::string lt = node->lifetimeName.empty() ? "'local_lease" : node->lifetimeName;
            activeBorrows.push_back(BorrowLease{referencedVar, true, lt});
        }
    }

    if (!inRawBlock) {
        linterWarnings.push_back("Linter Warning at line " + std::to_string(node->location.line) +
                                 ": Reference operator used outside unsafe raw block. Standard variables should be stack-allocated by default.");
    }
}

void SemanticAnalyzer::visit(PointerNode* node) {
    node->expr->accept(this);
    if (!inRawBlock) {
        semanticErrors.push_back("Memory Safety Error at line " + std::to_string(node->location.line) +
                                 ": Direct raw pointer dereferences are forbidden. Re-route dereferences inside local 'raw { ... }' containers.");
    }
}

void SemanticAnalyzer::visit(ScanqNode* node) {}

void SemanticAnalyzer::visit(ImportNode* node) {
    if (node->filepath.empty()) {
        semanticErrors.push_back("Semantic Error at line " + std::to_string(node->location.line) +
                                 ": Missing file path in consistof directive.");
    }
}

void SemanticAnalyzer::visit(VarDeclNode* node) {
    if (node->initializer) {
        node->initializer->accept(this);
    }

    Symbol sym;
    sym.name = node->name;
    sym.type = node->type;
    sym.isFirm = node->isFirm;
    sym.isBorrowMutable = node->isBorrowMutable;
    sym.lifetime = node->explicitLifetime;
    sym.declaredLoc = node->location;
    sym.isUnused = true;

    if (!symTable.insert(node->name, sym)) {
        semanticErrors.push_back("Semantic Error at line " + std::to_string(node->location.line) +
                                 ": Re-declaration of variable '" + node->name + "' is forbidden in local lexical scope.");
    }
}

void SemanticAnalyzer::visit(AssignNode* node) {
    node->target->accept(this);
    node->value->accept(this);

    if (auto idNode = std::dynamic_pointer_cast<IdentifierNode>(node->target)) {
        Symbol sym;
        if (symTable.lookup(idNode->name, sym)) {
            if (sym.isFirm) {
                semanticErrors.push_back("Semantic Error at line " + std::to_string(node->location.line) +
                                         ": Re-assignment to secure 'firm' constant '" + idNode->name + "' is prohibited.");
            }
            // Ensure no active borrow is violating mutation rules
            checkBorrowIntegrity(idNode->name, true, node->location);
        }
    }
}

void SemanticAnalyzer::visit(SuffixOpNode* node) {
    Symbol sym;
    if (!symTable.lookup(node->name, sym)) {
        semanticErrors.push_back("Semantic Error at line " + std::to_string(node->location.line) +
                                 ": Undeclared variable '" + node->name + "' cannot be mutated with suffix operators.");
    } else if (sym.isFirm) {
        semanticErrors.push_back("Semantic Error at line " + std::to_string(node->location.line) +
                                 ": Side-effect modification of immutable secure 'firm' variable '" + node->name + "' is prohibited.");
    } else {
        // Safe check for active borrows
        checkBorrowIntegrity(node->name, true, node->location);
    }
}

void SemanticAnalyzer::visit(ImprintNode* node) {
    node->expr->accept(this);
}

void SemanticAnalyzer::visit(BlockNode* node) {
    symTable.pushScope();
    for (auto& stmt : node->statements) {
        stmt->accept(this);
    }
    symTable.popScope();
}

void SemanticAnalyzer::visit(IfNode* node) {
    node->condition->accept(this);
    node->thenBranch->accept(this);
    for (auto& altifBr : node->altifBranches) {
        altifBr.first->accept(this);
        altifBr.second->accept(this);
    }
    if (node->elseBranch) {
        node->elseBranch->accept(this);
    }
}

void SemanticAnalyzer::visit(ReturnNode* node) {
    hasReturn = true;
    if (node->isFin) {
        if (currentReturnType != "int" && currentReturnType != "core" && currentReturnType != "void") {
            semanticErrors.push_back("Semantic Error at line " + std::to_string(node->location.line) +
                                     ": 'return fin' is only valid in craft contexts returning int, core, or void.");
        }
    } else if (node->expr) {
        node->expr->accept(this);
    }
}

void SemanticAnalyzer::visit(RecNode* node) {
    if (declaredRecords.find(node->name) != declaredRecords.end()) {
        semanticErrors.push_back("Semantic Error at line " + std::to_string(node->location.line) +
                                 ": Systems Record '" + node->name + "' already defined in compilation scope.");
    } else {
        declaredRecords.insert(node->name);
        recordNodes[node->name] = std::shared_ptr<RecNode>(node, [](RecNode*){}); // Ref-only pointer wrapper
    }
}

void SemanticAnalyzer::visit(CallNode* node) {
    for (auto& arg : node->args) {
        arg->accept(this);
    }

    // Handle generic monomorphization checks on-the-fly
    if (genericCrafts.find(node->callee) != genericCrafts.end()) {
        auto craftTemplate = genericCrafts[node->callee];
        if (!node->genericArgs.empty()) {
            std::string signatureKey = node->callee + "<";
            for (size_t i = 0; i < node->genericArgs.size(); ++i) {
                signatureKey += node->genericArgs[i] + (i + 1 < node->genericArgs.size() ? "," : "");
            }
            signatureKey += ">";
            if (monomorphizedInstances.find(signatureKey) == monomorphizedInstances.end()) {
                monomorphizedInstances.insert(signatureKey);
                // In a production-level monomorphize pass, we copy AST template, substitute T -> concrete types, and analyze!
            }
        }
    }
}

void SemanticAnalyzer::visit(CallStmtNode* node) {
    node->call->accept(this);
}

void SemanticAnalyzer::visit(CraftNode* node) {
    Symbol sym{node->name, "craft", false, false, false, "", node->location};
    if (!symTable.insert(node->name, sym)) {
        semanticErrors.push_back("Semantic Error at line " + std::to_string(node->location.line) +
                                 ": craft identifier '" + node->name + "' clashes with a symbol in current global registry.");
    }

    // Register generic template for monomorphization if it has type parameters
    if (!node->typeParams.empty()) {
        genericCrafts[node->name] = std::shared_ptr<CraftNode>(node, [](CraftNode*){});
    }

    symTable.pushScope();
    currentReturnType = node->returnType;
    hasReturn = false;

    for (auto& param : node->params) {
        Symbol paramSym{param.second, param.first, false, false, false, "", node->location};
        symTable.insert(param.second, paramSym);
    }

    node->body->accept(this);

    if (node->returnType != "void" && !hasReturn) {
        linterWarnings.push_back("Linter Warning at line " + std::to_string(node->location.line) +
                                 ": craft block '" + node->name + "' has no positive return paths, yet specifies return type '" + node->returnType + "'.");
    }

    symTable.popScope();
    currentReturnType = "void";
}

void SemanticAnalyzer::visit(RescAttackNode* node) {
    node->tryBlock->accept(this);
    
    symTable.pushScope();
    Symbol errorSym{node->errorVar, "string", true, false, false, "", node->location};
    symTable.insert(node->errorVar, errorSym);
    node->rescueBlock->accept(this);
    symTable.popScope();
}

void SemanticAnalyzer::visit(AttackNode* node) {
    node->errorExpr->accept(this);
}

void SemanticAnalyzer::visit(RawBlockNode* node) {
    bool oldRaw = inRawBlock;
    inRawBlock = true;
    node->body->accept(this);
    inRawBlock = oldRaw;
}

void SemanticAnalyzer::visit(CoreNode* node) {
    std::string oldReturn = currentReturnType;
    currentReturnType = "core";
    node->body->accept(this);
    currentReturnType = oldReturn;
}

void SemanticAnalyzer::visit(TraitNode* node) {
    if (declaredTraits.find(node->name) != declaredTraits.end()) {
        semanticErrors.push_back("Semantic Error at line " + std::to_string(node->location.line) +
                                 ": Trait definition '" + node->name + "' already registered in module hierarchy.");
    } else {
        declaredTraits[node->name] = std::shared_ptr<TraitNode>(node, [](TraitNode*){});
    }
}

void SemanticAnalyzer::visit(MethodCallNode* node) {
    node->instance->accept(this);
    for (auto& arg : node->args) {
        arg->accept(this);
    }
    
    // Check receiver record metadata or dynamic trait mappings
    std::string receiverRecord = "unknown";
    if (auto idInst = std::dynamic_pointer_cast<IdentifierNode>(node->instance)) {
        Symbol sym;
        if (symTable.lookup(idInst->name, sym)) {
            receiverRecord = sym.type;
        }
    }

    if (receiverRecord != "unknown" && declaredRecords.find(receiverRecord) == declaredRecords.end()) {
        linterWarnings.push_back("Method Resolution Warning at line " + std::to_string(node->location.line) +
                                 ": Resolving method '" + node->methodName + "' on primitive or unresolved type '" + receiverRecord + "'.");
    }
}

void SemanticAnalyzer::visit(ProgramNode* node) {
    for (auto& n : node->nodes) {
        n->accept(this);
    }
}

} // namespace Riven
