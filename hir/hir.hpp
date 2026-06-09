#ifndef RIVEN_HIR_HPP
#define RIVEN_HIR_HPP

#include "../ast/ast.hpp"
#include <string>
#include <vector>
#include <memory>
#include <sstream>

namespace Riven {

struct HIROp {
    enum class OpCode {
        ALLOC,
        LOAD,
        STORE,
        ADD,
        SUB,
        MUL,
        DIV,
        COMPARE_EQ,
        COMPARE_LT,
        COMPARE_GT,
        BRANCH,
        JUMP,
        LABEL,
        IMPRINT,
        SCANQ,
        CALL,
        RETURN,
        RAW_START,
        RAW_END,
        RESC_START,
        RESC_END
    };

    OpCode op;
    std::string dest;
    std::string arg1;
    std::string arg2;

    std::string toString() const {
        std::string opStr;
        switch (op) {
            case OpCode::ALLOC: opStr = "ALLOC"; break;
            case OpCode::LOAD: opStr = "LOAD"; break;
            case OpCode::STORE: opStr = "STORE"; break;
            case OpCode::ADD: opStr = "ADD"; break;
            case OpCode::SUB: opStr = "SUB"; break;
            case OpCode::MUL: opStr = "MUL"; break;
            case OpCode::DIV: opStr = "DIV"; break;
            case OpCode::COMPARE_EQ: opStr = "CMP_EQ"; break;
            case OpCode::COMPARE_LT: opStr = "CMP_LT"; break;
            case OpCode::COMPARE_GT: opStr = "CMP_GT"; break;
            case OpCode::BRANCH: opStr = "BRANCH"; break;
            case OpCode::JUMP: opStr = "JUMP"; break;
            case OpCode::LABEL: opStr = "LABEL"; break;
            case OpCode::IMPRINT: opStr = "IMPRINT"; break;
            case OpCode::SCANQ: opStr = "SCANQ"; break;
            case OpCode::CALL: opStr = "CALL"; break;
            case OpCode::RETURN: opStr = "RET"; break;
            case OpCode::RAW_START: opStr = "UNSAFE_RAW_START"; break;
            case OpCode::RAW_END: opStr = "UNSAFE_RAW_END"; break;
            case OpCode::RESC_START: opStr = "TRY_RESC_START"; break;
            case OpCode::RESC_END: opStr = "TRY_RESC_END"; break;
        }
        return dest + " = " + opStr + " " + arg1 + (arg2.empty() ? "" : ", " + arg2);
    }
};

class HIRGenerator : public ASTVisitor {
private:
    std::vector<HIROp> instructions;
    int tempCounter = 0;
    int labelCounter = 0;
    std::string lastVal = "";

    std::string nextTemp() {
        return "%h" + std::to_string(tempCounter++);
    }

    std::string nextLabel() {
        return "L" + std::to_string(labelCounter++);
    }

public:
    void emit(HIROp::OpCode op, std::string dest, std::string arg1, std::string arg2 = "") {
        instructions.push_back(HIROp{op, dest, arg1, arg2});
    }

    std::vector<HIROp> generate(std::shared_ptr<ProgramNode> root) {
        instructions.clear();
        if (root) {
            root->accept(this);
        }
        return instructions;
    }

    std::vector<HIROp> getInstructions() const { return instructions; }

    // ASTVisitor interface overrides
    void visit(LiteralIntNode* node) override {
        lastVal = std::to_string(node->value);
    }

    void visit(LiteralFloatNode* node) override {
        lastVal = std::to_string(node->value);
    }

    void visit(LiteralStringNode* node) override {
        lastVal = "\"" + node->value + "\"";
    }

    void visit(IdentifierNode* node) override {
        lastVal = node->name;
    }

    void visit(BinaryOpNode* node) override {
        node->left->accept(this);
        std::string lArg = lastVal;
        node->right->accept(this);
        std::string rArg = lastVal;

        std::string temp = nextTemp();
        HIROp::OpCode opCode = HIROp::OpCode::ADD;
        if (node->op == "+") opCode = HIROp::OpCode::ADD;
        else if (node->op == "-") opCode = HIROp::OpCode::SUB;
        else if (node->op == "*") opCode = HIROp::OpCode::MUL;
        else if (node->op == "/") opCode = HIROp::OpCode::DIV;
        else if (node->op == "==") opCode = HIROp::OpCode::COMPARE_EQ;
        else if (node->op == "<") opCode = HIROp::OpCode::COMPARE_LT;
        else if (node->op == ">") opCode = HIROp::OpCode::COMPARE_GT;

        emit(opCode, temp, lArg, rArg);
        lastVal = temp;
    }

    void visit(UnaryOpNode* node) override {
        node->expr->accept(this);
        std::string temp = nextTemp();
        emit(HIROp::OpCode::SUB, temp, "0", lastVal); // Negation is modeled as 0 - val
        lastVal = temp;
    }

    void visit(ReferenceNode* node) override {
        node->expr->accept(this);
        std::string temp = nextTemp();
        emit(HIROp::OpCode::LOAD, temp, "&" + lastVal);
        lastVal = temp;
    }

    void visit(PointerNode* node) override {
        node->expr->accept(this);
        std::string temp = nextTemp();
        emit(HIROp::OpCode::LOAD, temp, "*" + lastVal);
        lastVal = temp;
    }

    void visit(ScanqNode* node) override {
        std::string temp = nextTemp();
        emit(HIROp::OpCode::SCANQ, temp, "");
        lastVal = temp;
    }

    void visit(ImportNode* node) override {}

    void visit(VarDeclNode* node) override {
        emit(HIROp::OpCode::ALLOC, node->name, node->type);
        if (node->initializer) {
            node->initializer->accept(this);
            emit(HIROp::OpCode::STORE, node->name, lastVal);
        }
    }

    void visit(AssignNode* node) override {
        node->value->accept(this);
        std::string valArg = lastVal;
        node->target->accept(this);
        std::string tgtArg = lastVal;
        emit(HIROp::OpCode::STORE, tgtArg, valArg);
    }

    void visit(SuffixOpNode* node) override {
        std::string temp = nextTemp();
        HIROp::OpCode code = (node->op == "+>") ? HIROp::OpCode::ADD : HIROp::OpCode::SUB;
        emit(code, temp, node->name, "1");
        emit(HIROp::OpCode::STORE, node->name, temp);
    }

    void visit(ImprintNode* node) override {
        node->expr->accept(this);
        emit(HIROp::OpCode::IMPRINT, "", lastVal);
    }

    void visit(BlockNode* node) override {
        for (auto const& stmt : node->statements) {
            stmt->accept(this);
        }
    }

    void visit(IfNode* node) override {
        node->condition->accept(this);
        std::string condVal = lastVal;

        std::string elseLabel = nextLabel();
        std::string endLabel = nextLabel();

        emit(HIROp::OpCode::BRANCH, condVal, elseLabel); // Branch to else if condition false
        node->thenBranch->accept(this);
        emit(HIROp::OpCode::JUMP, endLabel, "");

        emit(HIROp::OpCode::LABEL, elseLabel, "");
        if (node->elseBranch) {
            node->elseBranch->accept(this);
        }
        emit(HIROp::OpCode::LABEL, endLabel, "");
    }

    void visit(ReturnNode* node) override {
        if (node->isFin) {
            emit(HIROp::OpCode::RETURN, "", "0");
        } else if (node->expr) {
            node->expr->accept(this);
            emit(HIROp::OpCode::RETURN, "", lastVal);
        } else {
            emit(HIROp::OpCode::RETURN, "", "");
        }
    }

    void visit(RecNode* node) override {}

    void visit(CallNode* node) override {
        std::vector<std::string> argTemps;
        for (auto const& arg : node->args) {
            arg->accept(this);
            argTemps.push_back(lastVal);
        }
        
        std::string temp = nextTemp();
        std::string argsList;
        for (size_t i = 0; i < argTemps.size(); ++i) {
            argsList += argTemps[i] + (i+1 < argTemps.size() ? ", " : "");
        }
        emit(HIROp::OpCode::CALL, temp, node->callee, argsList);
        lastVal = temp;
    }

    void visit(CallStmtNode* node) override {
        node->call->accept(this);
    }

    void visit(CraftNode* node) override {
        emit(HIROp::OpCode::LABEL, "craft_" + node->name, "");
        node->body->accept(this);
    }

    void visit(RescAttackNode* node) override {
        std::string startRescue = nextLabel();
        std::string endRescue = nextLabel();

        emit(HIROp::OpCode::RESC_START, startRescue, "");
        node->tryBlock->accept(this);
        emit(HIROp::OpCode::RESC_END, "", "");
        emit(HIROp::OpCode::JUMP, endRescue, "");

        emit(HIROp::OpCode::LABEL, startRescue, "");
        node->rescueBlock->accept(this);
        emit(HIROp::OpCode::LABEL, endRescue, "");
    }

    void visit(AttackNode* node) override {
        node->errorExpr->accept(this);
        emit(HIROp::OpCode::RETURN, "", lastVal); // modeled as premature error handler return
    }

    void visit(RawBlockNode* node) override {
        emit(HIROp::OpCode::RAW_START, "", "");
        node->body->accept(this);
        emit(HIROp::OpCode::RAW_END, "", "");
    }

    void visit(CoreNode* node) override {
        node->body->accept(this);
    }

    void visit(TraitNode* node) override {}
    void visit(MethodCallNode* node) override {
        node->instance->accept(this);
        std::string instVal = lastVal;
        std::vector<std::string> argsList;
        for (auto const& a : node->args) {
            a->accept(this);
            argsList.push_back(lastVal);
        }
        
        std::string temp = nextTemp();
        std::string formatArgs = instVal;
        for (const auto& a : argsList) {
            formatArgs += ", " + a;
        }
        emit(HIROp::OpCode::CALL, temp, node->methodName, formatArgs);
        lastVal = temp;
    }

    void visit(ProgramNode* node) override {
        for (auto const& n : node->nodes) {
            n->accept(this);
        }
    }
};

} // namespace Riven

#endif // RIVEN_HIR_HPP
