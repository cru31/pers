#pragma once

#include "json_test_loader.h"
#include "test_result_writer.h"
#include <chrono>

namespace pers::tests::json {

class JsonTestExecutor {
public:
    struct TestExecutionResult {
        JsonTestCase testCase;
        bool passed = false;
        std::string actualResult;
        std::string failureReason;
        double executionTimeMs = 0;
        std::vector<std::string> logMessages;  // Captured log messages for this test
    };
    
    JsonTestExecutor();
    ~JsonTestExecutor() = default;
    
    void runTestsFromFile(const std::string& filePath, bool printOptions = true, bool printEngineLogsAtEnd = true, bool printExecutionTime = false, bool truncateLongOutput = true);
    void runTestSuite(const std::string& filePath, const std::string& suiteName, bool printOptions = true, bool printEngineLogsAtEnd = true, bool printExecutionTime = false, bool truncateLongOutput = true);
    void generateReport(const std::string& outputPath);
    void generateStructuredResults(const std::string& jsonPath = "test_results.json", 
                                  const std::string& tablePath = "test_results_table.md");
    
    const std::vector<TestExecutionResult>& getResults() const { return _results; }
    
private:
    TestExecutionResult executeTestWithTimeout(const JsonTestCase& testCase);
    bool checkDependencies(const JsonTestCase& testCase);
    void printTestResult(const TestExecutionResult& result);
    void printTestResult(const TestExecutionResult& result, bool printOptions, bool printExecutionTime = false, bool truncateLongOutput = true);
    void setupLogCapture();
    void printCapturedLogs();
    
    JsonTestLoader _loader;
    std::vector<TestExecutionResult> _results;
    std::vector<std::string> _capturedLogs;
    bool _captureEngineLogs = false;
};

} // namespace pers::tests::json