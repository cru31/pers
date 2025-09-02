#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <chrono>
#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>
#include "test_result_writer.h"

namespace pers::tests::json {

struct JsonTestCase {
    std::string id;
    std::string category;
    std::string testType;
    std::map<std::string, std::string> inputValues;
    std::string expectedResult;
    std::vector<std::string> expectedCallstack;
    int timeoutMs = 1000;
    bool enabled = true;
    std::vector<std::string> dependencies;
    std::string reason; // for disabled tests
};

struct JsonTestMetadata {
    std::string version;
    std::string date;
    int totalTests;
    std::vector<std::string> categories;
};

class JsonTestLoader {
public:
    JsonTestLoader() = default;
    
    // Load test cases from JSON file
    bool loadFromFile(const std::string& filePath);
    
    // Get loaded test cases
    const std::vector<JsonTestCase>& getTestCases() const { return _testCases; }
    
    // Get test cases by category
    std::vector<JsonTestCase> getTestsByCategory(const std::string& category) const;
    
    // Get test suite
    std::vector<JsonTestCase> getTestSuite(const std::string& suiteName) const;
    
    // Get metadata
    const JsonTestMetadata& getMetadata() const { return _metadata; }
    
    // Execute a test case (returns pass/fail)
    bool executeTest(const JsonTestCase& testCase, std::string& actualResult, std::string& failureReason);
    
    // Execute option-based test
    bool executeOptionBasedTest(const JsonTestCase& testCase, std::string& actualResult, std::string& failureReason);
    
private:
    JsonTestMetadata _metadata;
    std::vector<JsonTestCase> _testCases;
    std::map<std::string, std::vector<std::string>> _testSuites;
    rapidjson::Document _document;
    
    // Parse test case from JSON value
    JsonTestCase parseTestCase(const rapidjson::Value& value);
};

// Test executor that uses JsonTestLoader
class JsonTestExecutor {
public:
    JsonTestExecutor();
    
    // Load and execute tests from JSON file
    void runTestsFromFile(const std::string& filePath);
    
    // Execute specific test suite
    void runTestSuite(const std::string& filePath, const std::string& suiteName);
    
    // Generate report
    void generateReport(const std::string& outputPath = "json_test_results.md");
    
    // Generate structured result tables
    void generateStructuredResults(const std::string& jsonPath = "test_results.json", 
                                  const std::string& tablePath = "test_results_table.md");
    
private:
    struct TestExecutionResult {
        JsonTestCase testCase;
        std::string actualResult;
        bool passed;
        std::string failureReason;
        double executionTimeMs;
    };
    
    std::vector<TestExecutionResult> _results;
    JsonTestLoader _loader;
    
    // Execute a single test with timeout
    TestExecutionResult executeTestWithTimeout(const JsonTestCase& testCase);
    
    // Check dependencies
    bool checkDependencies(const JsonTestCase& testCase);
    
    // Print colored output
    void printTestResult(const TestExecutionResult& result);
};

} // namespace pers::tests::json