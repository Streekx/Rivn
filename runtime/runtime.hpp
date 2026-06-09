#ifndef RIVEN_RUNTIME_HPP
#define RIVEN_RUNTIME_HPP

#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <stdexcept>
#include <sstream>

namespace Riven {

struct CallStackFrame {
    std::string craftName;
    size_t lineNum;
};

class RivenRuntime {
private:
    std::vector<CallStackFrame> callStack;
    std::map<size_t, int> rawMemoryHeap; // Simulated physical heap
    size_t heapPtr = 1000;
    size_t maxHeapBytes = 64 * 1024; // Simulated physical 64KB barrier

public:
    void pushStackFrame(std::string craftName, size_t line) {
        callStack.push_back({craftName, line});
    }

    void popStackFrame() {
        if (!callStack.empty()) {
            callStack.pop_back();
        }
    }

    size_t allocateMemory(size_t size) {
        if (heapPtr + size > 1000 + maxHeapBytes) {
            panic("Out of Memory Fault: Heap segment storage bounds exceeded! Request of size " + 
                  std::to_string(size) + " bytes was denied.");
        }
        size_t addr = heapPtr;
        heapPtr += size;
        for (size_t i = 0; i < size; ++i) {
            rawMemoryHeap[addr + i] = 0; // Initialize memory cell
        }
        return addr;
    }

    void writeMemory(size_t addr, int val, bool secureRawMode) {
        if (!secureRawMode && addr >= 1000) {
            panic("Unsafe Memory Access: Attempting raw pointer mutation (write address 0x" + 
                  std::to_string(addr) + ") outside of designated 'raw { ... }' execution container.");
        }
        if (addr >= heapPtr) {
            panic("Segmentation Fault: Attempted heap write overflow at invalid address 0x" + std::to_string(addr));
        }
        rawMemoryHeap[addr] = val;
    }

    int readMemory(size_t addr, bool secureRawMode) {
        if (!secureRawMode && addr >= 1000) {
            panic("Unsafe Memory Access: Attempting raw pointer dereference (read address 0x" + 
                  std::to_string(addr) + ") outside of designated 'raw { ... }' execution container.");
        }
        if (rawMemoryHeap.find(addr) == rawMemoryHeap.end()) {
            panic("Segmentation fault: Dereferenced unallocated or dangling memory page at address 0x" + std::to_string(addr));
        }
        return rawMemoryHeap[addr];
    }

    void panic(std::string message) {
        std::cerr << "\n========== !!! RIVEN CORE PANIC !!! ==========\n";
        std::cerr << "Description: " << message << "\n\n";
        std::cerr << "Backtrace (Active Stack Tracks):\n";
        if (callStack.empty()) {
            std::cerr << "  -> <empty call stack>\n";
        } else {
            for (auto it = callStack.rbegin(); it != callStack.rend(); ++it) {
                std::cerr << "  -> At craft '" << it->craftName << "' (Source Code Line: " << it->lineNum << ")\n";
            }
        }
        std::cerr << "===============================================\n\n";
        throw std::runtime_error("RIVEN PANIC: " + message);
    }
};

} // namespace Riven

#endif // RIVEN_RUNTIME_HPP
