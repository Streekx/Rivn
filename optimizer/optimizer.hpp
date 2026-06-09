#ifndef RIVEN_OPTIMIZER_HPP
#define RIVEN_OPTIMIZER_HPP

#include "../mir/mir.hpp"
#include <iostream>
#include <map>
#include <set>
#include <vector>
#include <string>

namespace Riven {

class Optimizer {
public:
    static ControlFlowGraph optimize(const ControlFlowGraph& inputCFG, int level = 2) {
        ControlFlowGraph optimized = inputCFG;
        bool changed = true;
        int maxIterations = 5; // limit passes to guarantee optimization convergence

        while (changed && maxIterations-- > 0) {
            changed = false;
            for (auto& [label, block] : optimized.blocks) {
                // Pass 1: Constant Folding
                if (level >= 1 && foldConstants(block)) {
                    changed = true;
                }
                // Pass 2: Copy Propagation
                if (level >= 1 && propagateCopies(block)) {
                    changed = true;
                }
                // Pass 3: Common Subexpression Elimination (CSE)
                if (level >= 2 && eliminateCommonSubexpressions(block)) {
                    changed = true;
                }
                // Pass 4: Dead Code Elimination (DCE)
                if (level >= 2 && eliminateDeadLocalCode(block)) {
                    changed = true;
                }
            }
        }
        return optimized;
    }

private:
    static bool foldConstants(BasicBlock& block) {
        bool changed = false;
        std::map<std::string, int> knownVals;

        for (auto& ins : block.instructions) {
            if (ins.op == MIRInstruction::OpCode::MOVE && ins.args.size() == 1) {
                try {
                    knownVals[ins.dest] = std::stoi(ins.args[0]);
                } catch (...) {}
            } else if (ins.op == MIRInstruction::OpCode::ADD && ins.args.size() == 2) {
                if (knownVals.find(ins.args[0]) != knownVals.end() && 
                    knownVals.find(ins.args[1]) != knownVals.end()) {
                    int val = knownVals[ins.args[0]] + knownVals[ins.args[1]];
                    ins.op = MIRInstruction::OpCode::MOVE;
                    ins.args = {std::to_string(val)};
                    knownVals[ins.dest] = val;
                    changed = true;
                }
            } else if (ins.op == MIRInstruction::OpCode::SUB && ins.args.size() == 2) {
                if (knownVals.find(ins.args[0]) != knownVals.end() && 
                    knownVals.find(ins.args[1]) != knownVals.end()) {
                    int val = knownVals[ins.args[0]] - knownVals[ins.args[1]];
                    ins.op = MIRInstruction::OpCode::MOVE;
                    ins.args = {std::to_string(val)};
                    knownVals[ins.dest] = val;
                    changed = true;
                }
            } else if (ins.op == MIRInstruction::OpCode::MUL && ins.args.size() == 2) {
                if (knownVals.find(ins.args[0]) != knownVals.end() && 
                    knownVals.find(ins.args[1]) != knownVals.end()) {
                    int val = knownVals[ins.args[0]] * knownVals[ins.args[1]];
                    ins.op = MIRInstruction::OpCode::MOVE;
                    ins.args = {std::to_string(val)};
                    knownVals[ins.dest] = val;
                    changed = true;
                }
            }
        }
        return changed;
    }

    static bool propagateCopies(BasicBlock& block) {
        bool changed = false;
        std::map<std::string, std::string> copies;

        for (auto& ins : block.instructions) {
            if (ins.op == MIRInstruction::OpCode::MOVE && ins.args.size() == 1) {
                if (!ins.args[0].empty() && ins.args[0][0] == '%') {
                    copies[ins.dest] = ins.args[0];
                }
            } else {
                for (auto& arg : ins.args) {
                    if (copies.find(arg) != copies.end()) {
                        arg = copies[arg];
                        changed = true;
                    }
                }
            }
        }
        return changed;
    }

    static bool eliminateCommonSubexpressions(BasicBlock& block) {
        bool changed = false;
        // Map arithmetic subexpressions like "ADD %v1, %v2" to their first computed destination temporary
        std::map<std::string, std::string> subexprToTemp;

        for (auto& ins : block.instructions) {
            if ((ins.op == MIRInstruction::OpCode::ADD || 
                 ins.op == MIRInstruction::OpCode::SUB || 
                 ins.op == MIRInstruction::OpCode::MUL) && ins.args.size() == 2) {
                
                std::string opStr = (ins.op == MIRInstruction::OpCode::ADD) ? "ADD" : 
                                    (ins.op == MIRInstruction::OpCode::SUB) ? "SUB" : "MUL";
                std::string arg1 = ins.args[0];
                std::string arg2 = ins.args[1];
                
                // Commutative operand canonical form normalization (ADD %x, %y == ADD %y, %x)
                if (ins.op == MIRInstruction::OpCode::ADD || ins.op == MIRInstruction::OpCode::MUL) {
                    if (arg1 > arg2) {
                        std::swap(arg1, arg2);
                    }
                }

                std::string exprKey = opStr + " " + arg1 + "," + arg2;
                if (subexprToTemp.find(exprKey) != subexprToTemp.end()) {
                    // Common subexpression found! Replace calculation with a copy MOVE
                    ins.op = MIRInstruction::OpCode::MOVE;
                    ins.args = {subexprToTemp[exprKey]};
                    changed = true;
                } else {
                    subexprToTemp[exprKey] = ins.dest;
                }
            }
        }
        return changed;
    }

    static bool eliminateDeadLocalCode(BasicBlock& block) {
        bool changed = false;
        std::set<std::string> usedVars;

        // Trace read registers (bottom-up liveness check)
        for (auto it = block.instructions.rbegin(); it != block.instructions.rend(); ++it) {
            if (it->op != MIRInstruction::OpCode::IMPRINT && 
                it->op != MIRInstruction::OpCode::RETURN &&
                it->op != MIRInstruction::OpCode::BRANCH && 
                it->op != MIRInstruction::OpCode::JUMP &&
                it->op != MIRInstruction::OpCode::CALL) { // Keep function calls alive due to potential nested side effects
                
                if (!it->dest.empty() && usedVars.find(it->dest) == usedVars.end()) {
                    // This node writes to a register that is never read in subsequent streams! DEAD code.
                    it->op = MIRInstruction::OpCode::MOVE;
                    it->dest = "";
                    it->args = {};
                    changed = true;
                    continue;
                }
            }
            // Add inputs to usage map
            for (auto const& arg : it->args) {
                if (!arg.empty() && arg[0] == '%') {
                    usedVars.insert(arg);
                }
            }
        }

        // Clean out empty/dead instructions
        auto prevSize = block.instructions.size();
        block.instructions.erase(
            std::remove_if(block.instructions.begin(), block.instructions.end(), [](const MIRInstruction& ins) {
                return ins.op == MIRInstruction::OpCode::MOVE && ins.dest.empty() && ins.args.empty();
            }),
            block.instructions.end()
        );
        if (block.instructions.size() != prevSize) {
            changed = true;
        }

        return changed;
    }
};

} // namespace Riven

#endif // RIVEN_OPTIMIZER_HPP
