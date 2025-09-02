#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>

namespace pers::tests::json {

struct TestExecutionDetail {
    std::string id;
    std::string category;
    std::string testType;
    std::string input;
    std::string expectedResult;
    std::string actualResult;
    std::vector<std::string> expectedCallstack;
    std::vector<std::string> actualCallstack;
    bool passed;
    std::string failureReason;
    double executionTimeMs;
    std::string timestamp;
};

class TestResultWriter {
public:
    TestResultWriter() = default;
    
    // Add a test result
    void addResult(const TestExecutionDetail& result);
    
    // Write results to JSON file
    bool writeToJson(const std::string& filePath);
    
    // Write results to Markdown table
    bool writeToMarkdown(const std::string& filePath);
    
    // Write results to both formats
    bool writeResults(const std::string& jsonPath, const std::string& markdownPath);
    
    // Get summary statistics
    struct Summary {
        int totalTests = 0;
        int passed = 0;
        int failed = 0;
        int skipped = 0;
        double totalTimeMs = 0.0;
        double passRate = 0.0;
    };
    
    Summary getSummary() const;
    
private:
    std::vector<TestExecutionDetail> _results;
    
    // Helper to get current timestamp
    std::string getCurrentTimestamp() const;
    
    // Helper to format callstack for display
    std::string formatCallstack(const std::vector<std::string>& callstack) const;
};

} // namespace pers::tests::json