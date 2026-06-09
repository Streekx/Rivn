#ifndef RIVEN_MIR_HPP
#define RIVEN_MIR_HPP

#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>
#include <sstream>
#include <algorithm>
#include "../hir/hir.hpp"

namespace Riven {

struct MIRInstruction {
    enum class OpCode {
        MOVE,          // %v2 = %v1
        PHI,           // %v3 = PHI %v1, %v2
        ADD,
        SUB,
        MUL,
        DIV,
        COMPARE_EQ,
        COMPARE_LT,
        COMPARE_GT,
        IMPRINT,
        SCANQ,
        CALL,
        RETURN,
        JUMP,          // jump BasicBlock
        BRANCH,        // branch condition, TrueBlock, FalseBlock
    };

    OpCode op;
    std::string dest;
    std::vector<std::string> args;

    std::string toString() const {
        std::string opStr;
        switch (op) {
            case OpCode::MOVE: opStr = "MOVE"; break;
            case OpCode::PHI: opStr = "PHI"; break;
            case OpCode::ADD: opStr = "ADD"; break;
            case OpCode::SUB: opStr = "SUB"; break;
            case OpCode::MUL: opStr = "MUL"; break;
            case OpCode::DIV: opStr = "DIV"; break;
            case OpCode::COMPARE_EQ: opStr = "CMP_EQ"; break;
            case OpCode::COMPARE_LT: opStr = "CMP_LT"; break;
            case OpCode::COMPARE_GT: opStr = "CMP_GT"; break;
            case OpCode::IMPRINT: opStr = "IMPRINT"; break;
            case OpCode::SCANQ: opStr = "SCANQ"; break;
            case OpCode::CALL: opStr = "CALL"; break;
            case OpCode::RETURN: opStr = "RET"; break;
            case OpCode::JUMP: opStr = "JUMP"; break;
            case OpCode::BRANCH: opStr = "BRANCH"; break;
        }
        std::string argsStr;
        for (size_t i = 0; i < args.size(); ++i) {
            argsStr += args[i] + (i + 1 < args.size() ? ", " : "");
        }
        return dest + " = " + opStr + " " + argsStr;
    }
};

struct BasicBlock {
    std::string label;
    std::vector<MIRInstruction> instructions;
    std::vector<std::string> predecessors;
    std::vector<std::string> successors;
};

struct ControlFlowGraph {
    std::map<std::string, BasicBlock> blocks;
    std::string entryBlock;

    std::string toDot() const {
        std::stringstream ss;
        ss << "digraph CFG {\n";
        for (auto const& [label, block] : blocks) {
            ss << "  \"" << label << "\" [shape=box, label=\"" << label << "\\n";
            for (auto const& ins : block.instructions) {
                // escape quotes for DOT compatibility
                std::string rawStr = ins.toString();
                std::string escaped;
                for (char c : rawStr) {
                    if (c == '"') escaped += "\\\"";
                    else escaped += c;
                }
                ss << escaped << "\\n";
            }
            ss << "\"];\n";
            for (auto const& succ : block.successors) {
                ss << "  \"" << label << "\" -> \"" << succ << "\";\n";
            }
        }
        ss << "}\n";
        return ss.str();
    }
};

class SSAConverter {
private:
    std::map<std::string, int> variableVersion;
    std::map<std::string, std::string> currentDefinition;

public:
    std::string renameVariable(const std::string& name) {
        if (name.empty() || name[0] == '%' || std::isdigit(name[0]) || name[0] == '"') {
            return name; // Already temporary, literal or constant
        }
        
        variableVersion[name]++;
        std::string versionedName = "%" + name + "_" + std::to_string(variableVersion[name]);
        currentDefinition[name] = versionedName;
        return versionedName;
    }

    std::string getLatestVersion(const std::string& name) {
        if (variableVersion.find(name) == variableVersion.end()) {
            return renameVariable(name);
        }
        return currentDefinition[name];
    }

    ControlFlowGraph computeCFG(const std::vector<HIROp>& hirOps) {
        ControlFlowGraph cfg;
        std::string curLabel = "entry";
        BasicBlock curBlock;
        curBlock.label = curLabel;
        cfg.entryBlock = "entry";

        auto finishBlock = [&](BasicBlock& b) {
            if (!b.instructions.empty() || b.label == "entry") {
                cfg.blocks[b.label] = b;
            }
        };

        for (const auto& op : hirOps) {
            if (op.op == HIROp::OpCode::LABEL) {
                finishBlock(curBlock);
                curLabel = op.dest;
                curBlock = BasicBlock();
                curBlock.label = curLabel;
                continue;
            }

            MIRInstruction mir;
            bool isTerminator = false;

            if (op.op == HIROp::OpCode::ADD) {
                mir.op = MIRInstruction::OpCode::ADD;
                mir.dest = renameVariable(op.dest);
                mir.args = {getLatestVersion(op.arg1), getLatestVersion(op.arg2)};
            } else if (op.op == HIROp::OpCode::SUB) {
                mir.op = MIRInstruction::OpCode::SUB;
                mir.dest = renameVariable(op.dest);
                mir.args = {getLatestVersion(op.arg1), getLatestVersion(op.arg2)};
            } else if (op.op == HIROp::OpCode::MUL) {
                mir.op = MIRInstruction::OpCode::MUL;
                mir.dest = renameVariable(op.dest);
                mir.args = {getLatestVersion(op.arg1), getLatestVersion(op.arg2)};
            } else if (op.op == HIROp::OpCode::DIV) {
                mir.op = MIRInstruction::OpCode::DIV;
                mir.dest = renameVariable(op.dest);
                mir.args = {getLatestVersion(op.arg1), getLatestVersion(op.arg2)};
            } else if (op.op == HIROp::OpCode::STORE) {
                mir.op = MIRInstruction::OpCode::MOVE;
                mir.dest = renameVariable(op.dest);
                mir.args = {getLatestVersion(op.arg1)};
            } else if (op.op == HIROp::OpCode::LOAD || op.op == HIROp::OpCode::ALLOC) {
                mir.op = MIRInstruction::OpCode::MOVE;
                mir.dest = renameVariable(op.dest);
                mir.args = {getLatestVersion(op.arg1)};
            } else if (op.op == HIROp::OpCode::COMPARE_EQ) {
                mir.op = MIRInstruction::OpCode::COMPARE_EQ;
                mir.dest = renameVariable(op.dest);
                mir.args = {getLatestVersion(op.arg1), getLatestVersion(op.arg2)};
            } else if (op.op == HIROp::OpCode::COMPARE_LT) {
                mir.op = MIRInstruction::OpCode::COMPARE_LT;
                mir.dest = renameVariable(op.dest);
                mir.args = {getLatestVersion(op.arg1), getLatestVersion(op.arg2)};
            } else if (op.op == HIROp::OpCode::COMPARE_GT) {
                mir.op = MIRInstruction::OpCode::COMPARE_GT;
                mir.dest = renameVariable(op.dest);
                mir.args = {getLatestVersion(op.arg1), getLatestVersion(op.arg2)};
            } else if (op.op == HIROp::OpCode::IMPRINT) {
                mir.op = MIRInstruction::OpCode::IMPRINT;
                mir.dest = "";
                mir.args = {getLatestVersion(op.arg1)};
            } else if (op.op == HIROp::OpCode::SCANQ) {
                mir.op = MIRInstruction::OpCode::SCANQ;
                mir.dest = renameVariable(op.dest);
                mir.args = {};
            } else if (op.op == HIROp::OpCode::CALL) {
                mir.op = MIRInstruction::OpCode::CALL;
                mir.dest = renameVariable(op.dest);
                mir.args = {op.arg1, op.arg2}; // Callee + formatted args list
            } else if (op.op == HIROp::OpCode::RETURN) {
                mir.op = MIRInstruction::OpCode::RETURN;
                mir.dest = "";
                mir.args = {getLatestVersion(op.arg1)};
                isTerminator = true;
            } else if (op.op == HIROp::OpCode::JUMP) {
                mir.op = MIRInstruction::OpCode::JUMP;
                mir.dest = "";
                mir.args = {op.dest};
                isTerminator = true;
            } else if (op.op == HIROp::OpCode::BRANCH) {
                mir.op = MIRInstruction::OpCode::BRANCH;
                mir.dest = "";
                mir.args = {getLatestVersion(op.dest), op.arg1}; // Condition variable and false block path
                isTerminator = true;
            }

            if (!mir.dest.empty() || mir.op == MIRInstruction::OpCode::IMPRINT || 
                mir.op == MIRInstruction::OpCode::RETURN || mir.op == MIRInstruction::OpCode::JUMP || 
                mir.op == MIRInstruction::OpCode::BRANCH) {
                curBlock.instructions.push_back(mir);
            }

            if (isTerminator) {
                finishBlock(curBlock);
                curLabel = op.dest + "_post";
                curBlock = BasicBlock();
                curBlock.label = curLabel;
            }
        }
        finishBlock(curBlock);

        // Build linkages of edges
        for (auto& [label, block] : cfg.blocks) {
            if (block.instructions.empty()) continue;
            auto lastIns = block.instructions.back();
            if (lastIns.op == MIRInstruction::OpCode::JUMP) {
                std::string target = lastIns.args[0];
                block.successors.push_back(target);
                cfg.blocks[target].predecessors.push_back(label);
            } else if (lastIns.op == MIRInstruction::OpCode::BRANCH) {
                std::string falseTarget = lastIns.args[1];
                block.successors.push_back(falseTarget);
                cfg.blocks[falseTarget].predecessors.push_back(label);
                
                // Also link to post block or subsequent labels dynamically
                auto posIt = cfg.blocks.upper_bound(label);
                if (posIt != cfg.blocks.end()) {
                    block.successors.push_back(posIt->first);
                    posIt->second.predecessors.push_back(label);
                }
            } else if (lastIns.op != MIRInstruction::OpCode::RETURN) {
                auto nextIt = cfg.blocks.upper_bound(label);
                if (nextIt != cfg.blocks.end()) {
                    block.successors.push_back(nextIt->first);
                    nextIt->second.predecessors.push_back(label);
                }
            }
        }
        return cfg;
    }
};

} // namespace Riven

#endif // RIVEN_MIR_HPP
