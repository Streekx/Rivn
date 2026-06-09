#ifndef RIVEN_TEST_FRAMEWORK_HPP
#define RIVEN_TEST_FRAMEWORK_HPP

#include <string>
#include <vector>
#include <iostream>
#include <functional>

namespace Riven {

struct TestCase {
    std::string testSuite;
    std::string testName;
    std::function<bool()> run;
};

class TestFramework {
private:
    std::vector<TestCase> testCases;
    size_t passedTests = 0;
    size_t failedTests = 0;

public:
    void addTest(const std::string& suite, const std::string& name, std::function<bool()> func) {
        testCases.push_back({suite, name, func});
    }

    void runAllTests() {
        std::cout << "\n========== RUNNING RIVEN TEST SUITES ==========\n";
        for (const auto& test : testCases) {
            std::cout << "[RUN] " << test.testSuite << " / " << test.testName << " -> ";
            try {
                if (test.run()) {
                    std::cout << "\033[32m[PASS]\033[0m\n";
                    passedTests++;
                } else {
                    std::cout << "\033[31m[FAIL]\033[0m\n";
                    failedTests++;
                }
            } catch (const std::exception& e) {
                std::cout << "\033[31m[CRASHED]\033[0m (" << e.what() << ")\n";
                failedTests++;
            }
        }
        std::cout << "-----------------------------------------------\n";
        std::cout << "Test run summary:\n";
        std::cout << "  Passed: " << passedTests << "\n";
        std::cout << "  Failed: " << failedTests << "\n";
        std::cout << "===============================================\n\n";
    }
};

} // namespace Riven

#endif // RIVEN_TEST_FRAMEWORK_HPP
