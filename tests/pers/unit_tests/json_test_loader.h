#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <fstream>
#include <sstream>
#include <chrono>

// Simple JSON parser for test cases
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

struct JsonTestFile {
    JsonTestMetadata metadata;
    std::vector<JsonTestCase> testCases;
    std::map<std::string, std::vector<std::string>> testSuites;
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
    
private:
    JsonTestMetadata _metadata;
    std::vector<JsonTestCase> _testCases;
    std::map<std::string, std::vector<std::string>> _testSuites;
    
    // JSON parsing helpers
    std::string extractString(const std::string& json, const std::string& key);
    int extractInt(const std::string& json, const std::string& key);
    bool extractBool(const std::string& json, const std::string& key);
    std::vector<std::string> extractStringArray(const std::string& json, const std::string& key);
    std::map<std::string, std::string> extractObject(const std::string& json, const std::string& key);
    std::string extractObjectString(const std::string& json, const std::string& key);  // Extract object as raw JSON string
    std::string extractArrayString(const std::string& json, const std::string& key);  // Extract array as raw JSON string
    
    // Parse individual test case from JSON object
    JsonTestCase parseTestCase(const std::string& jsonObject);
    
    // Clean JSON string (remove whitespace, comments)
    std::string cleanJson(const std::string& json);
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