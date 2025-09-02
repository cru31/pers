#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <map>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>

namespace pers::tests {

// Test case result structure
struct TestCase {
    std::string input;
    std::string expectedResult;
    std::string actualResult;
    std::string expectedCallstack;
    bool passed = false;
    std::string failureReason;
};

// Test metadata
struct TestInfo {
    std::string id;
    std::string name;
    std::string category;
    std::function<TestCase()> testFunction;
};

// Test result with timing
struct TestResult {
    std::string id;
    std::string category;
    std::string name;
    TestCase testCase;
    double executionTimeMs;
};

// Base class for test suites
class TestSuite {
public:
    TestSuite(const std::string& category) : _category(category) {}
    virtual ~TestSuite() = default;
    
    virtual void registerTests() = 0;
    
    std::vector<TestInfo> getTests() const { return _tests; }
    std::string getCategory() const { return _category; }
    
protected:
    void addTest(const std::string& id, const std::string& name, 
                 std::function<TestCase()> testFunc) {
        TestInfo info;
        info.id = id;
        info.name = name;
        info.category = _category;
        info.testFunction = testFunc;
        _tests.push_back(info);
    }
    
private:
    std::string _category;
    std::vector<TestInfo> _tests;
};

// Test registry to manage all test suites
class TestRegistry {
public:
    void addSuite(std::unique_ptr<TestSuite> suite) {
        suite->registerTests();
        _suites.push_back(std::move(suite));
    }
    
    std::vector<TestInfo> getAllTests() const {
        std::vector<TestInfo> allTests;
        for (const auto& suite : _suites) {
            auto tests = suite->getTests();
            allTests.insert(allTests.end(), tests.begin(), tests.end());
        }
        return allTests;
    }
    
    void runAll() {
        auto tests = getAllTests();
        std::vector<TestResult> results;
        
        std::cout << "\n";
        std::cout << "========================================\n";
        std::cout << "  PERS GRAPHICS ENGINE UNIT TESTS\n";
        std::cout << "  Total Tests: " << tests.size() << "\n";
        std::cout << "========================================\n\n";
        
        int passed = 0;
        int failed = 0;
        
        for (const auto& test : tests) {
            auto start = std::chrono::high_resolution_clock::now();
            TestCase result = test.testFunction();
            auto end = std::chrono::high_resolution_clock::now();
            
            TestResult testResult;
            testResult.id = test.id;
            testResult.category = test.category;
            testResult.name = test.name;
            testResult.testCase = result;
            testResult.executionTimeMs = std::chrono::duration<double, std::milli>(end - start).count();
            
            results.push_back(testResult);
            
            if (result.passed) {
                passed++;
                std::cout << "[" << std::setw(3) << test.id << "] ";
                std::cout << "\033[32mPASS\033[0m ";
            } else {
                failed++;
                std::cout << "[" << std::setw(3) << test.id << "] ";
                std::cout << "\033[31mFAIL\033[0m ";
            }
            
            std::cout << std::left << std::setw(30) << test.name.substr(0, 30);
            std::cout << " (" << std::fixed << std::setprecision(2) 
                     << testResult.executionTimeMs << "ms)";
            
            if (!result.passed && !result.failureReason.empty()) {
                std::cout << "\n        Reason: " << result.failureReason;
            }
            std::cout << "\n";
        }
        
        std::cout << "\n";
        std::cout << "========================================\n";
        std::cout << "  TEST SUMMARY\n";
        std::cout << "========================================\n";
        std::cout << "  Passed: " << passed << "/" << tests.size() << "\n";
        std::cout << "  Failed: " << failed << "/" << tests.size() << "\n";
        std::cout << "  Pass Rate: " << std::fixed << std::setprecision(1) 
                 << (tests.size() > 0 ? (passed * 100.0 / tests.size()) : 0) << "%\n\n";
        
        generateReport(results);
    }
    
private:
    std::vector<std::unique_ptr<TestSuite>> _suites;
    
    void generateReport(const std::vector<TestResult>& results) {
        // Generate detailed markdown report
        std::ofstream md("test_results.md");
        
        md << "# Pers Graphics Engine Unit Test Results\n\n";
        md << "Generated: " << getCurrentTimestamp() << "\n\n";
        md << "## Summary\n\n";
        
        int passed = 0;
        int failed = 0;
        for (const auto& result : results) {
            if (result.testCase.passed) passed++;
            else failed++;
        }
        
        md << "- **Total Tests**: " << results.size() << "\n";
        md << "- **Passed**: " << passed << "\n";
        md << "- **Failed**: " << failed << "\n";
        md << "- **Pass Rate**: " << std::fixed << std::setprecision(1) 
           << (results.size() > 0 ? (passed * 100.0 / results.size()) : 0) << "%\n\n";
        
        // Group by category
        std::map<std::string, std::vector<TestResult>> byCategory;
        for (const auto& result : results) {
            byCategory[result.category].push_back(result);
        }
        
        md << "## Detailed Results\n\n";
        
        for (const auto& [category, catResults] : byCategory) {
            md << "### " << category << "\n\n";
            md << "| ID | Test | Input | Expected | Actual | Time (ms) | Status | Failure Reason |\n";
            md << "|----|------|-------|----------|--------|-----------|--------|----------------|\n";
            
            for (const auto& result : catResults) {
                md << "| " << result.id;
                md << " | " << result.name;
                md << " | " << result.testCase.input;
                md << " | " << result.testCase.expectedResult;
                md << " | " << result.testCase.actualResult;
                md << " | " << std::fixed << std::setprecision(2) << result.executionTimeMs;
                md << " | " << (result.testCase.passed ? "✅ PASS" : "❌ FAIL");
                md << " | " << result.testCase.failureReason;
                md << " |\n";
            }
            md << "\n";
        }
        
        md.close();
        std::cout << "Test report generated: test_results.md\n";
    }
    
    std::string getCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }
};

} // namespace pers::tests