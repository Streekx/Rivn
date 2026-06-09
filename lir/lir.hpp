#ifndef RIVEN_LIR_HPP
#define RIVEN_LIR_HPP

#include <string>
#include <vector>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <stdexcept>
#include <iostream>

namespace Riven {

struct LIRInstruction {
    enum class OpCode {
        MOV,
        ADD,
        SUB,
        IMUL,
        IDIV,
        CMP,
        JE,
        JNE,
        JL,
        JG,
        JMP,
        PUSH,
        POP,
        CALL,
        RET,
        SYSCALL
    };

    OpCode op;
    std::string regDest;
    std::string arg1;
    std::string arg2;

    std::string toString() const {
        std::string opStr;
        switch (op) {
            case OpCode::MOV: opStr = "mov"; break;
            case OpCode::ADD: opStr = "add"; break;
            case OpCode::SUB: opStr = "sub"; break;
            case OpCode::IMUL: opStr = "imul"; break;
            case OpCode::IDIV: opStr = "idiv"; break;
            case OpCode::CMP: opStr = "cmp"; break;
            case OpCode::JE: opStr = "je"; break;
            case OpCode::JNE: opStr = "jne"; break;
            case OpCode::JL: opStr = "jl"; break;
            case OpCode::JG: opStr = "jg"; break;
            case OpCode::JMP: opStr = "jmp"; break;
            case OpCode::PUSH: opStr = "push"; break;
            case OpCode::POP: opStr = "pop"; break;
            case OpCode::CALL: opStr = "call"; break;
            case OpCode::RET: opStr = "ret"; break;
            case OpCode::SYSCALL: opStr = "syscall"; break;
        }
        std::string args = regDest;
        if (!arg1.empty()) args += ", " + arg1;
        if (!arg2.empty()) args += ", " + arg2;
        return opStr + " " + args;
    }
};

struct RegisterAllocation {
    std::map<std::string, std::string> vregToPReg; // mapped virtual register to physical register
    std::vector<std::string> spilledRegs;

    std::string getReg(const std::string& vreg) {
        if (vreg.empty() || vreg[0] != '%') {
            return vreg; // Literal constant or raw base
        }
        if (vregToPReg.find(vreg) != vregToPReg.end()) {
            return vregToPReg[vreg];
        }
        // Spill location on frame pointer
        size_t offset = (std::hash<std::string>{}(vreg) % 8) * 8 + 8;
        return "[rbp - " + std::to_string(offset) + "]";
    }
};

// Graph Coloring Register Allocation implementation
class GraphColoringAllocator {
private:
    std::vector<std::string> targetRegisters;
    std::unordered_map<std::string, std::unordered_set<std::string>> interferenceGraph;

public:
    GraphColoringAllocator(const std::vector<std::string>& physRegs) : targetRegisters(physRegs) {}

    void addVariable(const std::string& var) {
        if (var.empty() || var[0] != '%') return;
        if (interferenceGraph.find(var) == interferenceGraph.end()) {
            interferenceGraph[var] = std::unordered_set<std::string>();
        }
    }

    void addInterference(const std::string& u, const std::string& v) {
        if (u == v || u.empty() || v.empty()) return;
        if (u[0] == '%' && v[0] == '%') {
            addVariable(u);
            addVariable(v);
            interferenceGraph[u].insert(v);
            interferenceGraph[v].insert(u);
        }
    }

    RegisterAllocation allocate(const std::vector<std::string>& activeVars) {
        RegisterAllocation allocation;
        
        // Populate graph nodes
        for (const auto& v : activeVars) {
            addVariable(v);
        }

        // Simplification & Kempe stack construction
        std::vector<std::string> simplifyStack;
        std::unordered_set<std::string> removedNodes;
        auto tempGraph = interferenceGraph;

        size_t kColors = targetRegisters.size();
        
        bool simplified = true;
        while (simplified) {
            simplified = false;
            for (auto const& [node, neighbors] : tempGraph) {
                if (removedNodes.find(node) != removedNodes.end()) continue;
                
                // Count active degree
                size_t degree = 0;
                for (const auto& neighbor : neighbors) {
                    if (removedNodes.find(neighbor) == removedNodes.end()) {
                        degree++;
                    }
                }

                if (degree < kColors) {
                    simplifyStack.push_back(node);
                    removedNodes.insert(node);
                    simplified = true;
                    break;
                }
            }

            // Greedy spill handling
            if (!simplified && removedNodes.size() < tempGraph.size()) {
                for (auto const& [node, neighbors] : tempGraph) {
                    if (removedNodes.find(node) == removedNodes.end()) {
                        simplifyStack.push_back(node);
                        removedNodes.insert(node);
                        allocation.spilledRegs.push_back(node);
                        simplified = true;
                        break;
                    }
                }
            }
        }

        // Greedy coloring phase
        std::reverse(simplifyStack.begin(), simplifyStack.end());
        for (const auto& node : simplifyStack) {
            // Find neighbors colored so far
            std::set<std::string> unavailableColors;
            for (const auto& neighbor : interferenceGraph[node]) {
                if (allocation.vregToPReg.find(neighbor) != allocation.vregToPReg.end()) {
                    unavailableColors.insert(allocation.vregToPReg[neighbor]);
                }
            }

            // Assign first free color (register)
            std::string assignedColor = "";
            for (const auto& reg : targetRegisters) {
                if (unavailableColors.find(reg) == unavailableColors.end()) {
                    assignedColor = reg;
                    break;
                }
            }

            if (!assignedColor.empty()) {
                allocation.vregToPReg[node] = assignedColor;
            } else {
                // Actual spill
                if (std::find(allocation.spilledRegs.begin(), allocation.spilledRegs.end(), node) == allocation.spilledRegs.end()) {
                    allocation.spilledRegs.push_back(node);
                }
            }
        }

        return allocation;
    }
};

} // namespace Riven

#endif // RIVEN_LIR_HPP
