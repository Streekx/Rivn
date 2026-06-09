#ifndef RIVEN_PROFILER_HPP
#define RIVEN_PROFILER_HPP

#include <iostream>
#include <chrono>
#include <string>
#include <map>
#include <algorithm>
#include <cstdio>
#include <vector>

namespace Riven {

struct FunctionExecutionMetrics {
    std::string craftName;
    size_t callCount = 0;
    double cumulativeTimeUs = 0.0; // Microseconds
    size_t asmInstructionsExecuted = 0;
    double maxLatencyUs = 0.0;
};

class Profiler {
private:
    std::map<std::string, FunctionExecutionMetrics> metrics;
    std::chrono::high_resolution_clock::time_point lastTrigger;

public:
    void triggerStart() {
        lastTrigger = std::chrono::high_resolution_clock::now();
    }

    void logFunctionExecution(const std::string& name, size_t instructionCount) {
        auto end = std::chrono::high_resolution_clock::now();
        double delta = std::chrono::duration<double, std::micro>(end - lastTrigger).count();
        
        metrics[name].craftName = name;
        metrics[name].callCount++;
        metrics[name].cumulativeTimeUs += delta;
        metrics[name].asmInstructionsExecuted += instructionCount;
        metrics[name].maxLatencyUs = std::max(metrics[name].maxLatencyUs, delta);
    }

    void dumpProfilerResults() const {
        std::cout << "\n================= RIVEN EXECUTION PROFILE & BENCHMARK =================\n";
        std::printf("%-18s | %5s | %15s | %12s | %15s\n", 
                    "Function Name", "Calls", "Cum. Time (us)", "Instructions", "Max Latency(us)");
        std::cout << "-------------------|-------|-----------------|--------------|----------------\n";
        
        std::string hotPath = "None";
        double maxTime = 0.0;

        for (auto const& [name, m] : metrics) {
            std::printf("%-18s | %5zu | %15.2f | %12zu | %15.2f\n", 
                        m.craftName.c_str(), m.callCount, m.cumulativeTimeUs, m.asmInstructionsExecuted, m.maxLatencyUs);
            
            if (m.cumulativeTimeUs > maxTime) {
                maxTime = m.cumulativeTimeUs;
                hotPath = m.craftName;
            }
        }
        std::cout << "-----------------------------------------------------------------------\n";
        std::cout << "Performance Hot-Path (Critical branch): " << hotPath << " (" << maxTime << " us)\n";
        std::cout << "=======================================================================\n\n";
    }
};

} // namespace Riven

#endif // RIVEN_PROFILER_HPP
