#ifndef RIVEN_DEBUGGER_HPP
#define RIVEN_DEBUGGER_HPP

#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <algorithm>

namespace Riven {

struct Breakpoint {
    std::string filename;
    size_t lineNum;
    bool active;
};

class Debugger {
private:
    std::vector<Breakpoint> breakpoints;
    std::map<std::string, std::string> currentVariablesRegister;
    size_t currentLineCursor = 0;
    std::map<std::string, std::string> cpuPhysicalRegisters;

public:
    Debugger() {
        // Initialize simulated machine physical CPU registers
        cpuPhysicalRegisters["rax"] = "0x00000000";
        cpuPhysicalRegisters["rsi"] = "0x00000000";
        cpuPhysicalRegisters["rdi"] = "0x00000000";
        cpuPhysicalRegisters["rsp"] = "0x00007ffe";
        cpuPhysicalRegisters["rbp"] = "0x00007ffe";
    }

    void addBreakpoint(const std::string& file, size_t line) {
        breakpoints.push_back({file, line, true});
        std::cout << "[Debugger] Placed active hardware-breakpoint on " << file << ":" << line << "\n";
    }

    void dumpVariables() const {
        std::cout << "\n========== RIVEN SYMBOLIC WATCH FRAME ==========\n";
        if (currentVariablesRegister.empty()) {
            std::cout << "  No symbols active in current local frame stack.\n";
        } else {
            for (auto const& [name, val] : currentVariablesRegister) {
                std::cout << "  symbol `" << name << "` => value: " << val << "\n";
            }
        }
        std::cout << "------------------------------------------------\n";
        std::cout << "Simulated Core Assembly CPU Registers:\n";
        for (auto const& [reg, val] : cpuPhysicalRegisters) {
            std::cout << "  " << reg << ": " << val << "  ";
        }
        std::cout << "\n================================================\n\n";
    }

    void updateSymbolState(const std::string& sym, const std::string& representation) {
        currentVariablesRegister[sym] = representation;
    }

    bool checkBreakpointTrigger(const std::string& file, size_t line) {
        for (const auto& bp : breakpoints) {
            if (bp.active && bp.filename == file && bp.lineNum == line) {
                std::cout << "[Debugger] HARDWARE BREAKPOINT TRIGGERED at " << file << ":" << line << "\n";
                dumpVariables();
                return true;
            }
        }
        return false;
    }

    void handleStep() {
        currentLineCursor++;
        // Update hardware registers dynamically during stepping execution
        cpuPhysicalRegisters["rax"] = "0x" + std::to_string(currentLineCursor * 8 + 0xFF);
        std::cout << "[Debugger] Stepping instructions -> program counter: " << currentLineCursor << "\n";
    }
};

} // namespace Riven

#endif // RIVEN_DEBUGGER_HPP
