#include "test_result_writer.h"
#include <algorithm>
#include <iomanip>
#include <map>

namespace pers::tests::json {

void TestResultWriter::addResult(const TestExecutionDetail& result) {
    _results.push_back(result);
}

bool TestResultWriter::writeToJson(const std::string& filePath) {
    rapidjson::Document doc;
    doc.SetObject();
    auto& allocator = doc.GetAllocator();
    
    // Add metadata
    rapidjson::Value metadata(rapidjson::kObjectType);
    metadata.AddMember("timestamp", rapidjson::Value(getCurrentTimestamp().c_str(), allocator), allocator);
    metadata.AddMember("total_tests", static_cast<int>(_results.size()), allocator);
    
    auto summary = getSummary();
    metadata.AddMember("passed", summary.passed, allocator);
    metadata.AddMember("failed", summary.failed, allocator);
    metadata.AddMember("skipped", summary.skipped, allocator);
    metadata.AddMember("not_applicable", summary.notApplicable, allocator);
    metadata.AddMember("not_yet_implemented", summary.notYetImplemented, allocator);
    metadata.AddMember("pass_rate", summary.passRate, allocator);
    metadata.AddMember("total_time_ms", summary.totalTimeMs, allocator);
    
    doc.AddMember("metadata", metadata, allocator);
    
    // Add test results
    rapidjson::Value results(rapidjson::kArrayType);
    
    for (const auto& result : _results) {
        rapidjson::Value testResult(rapidjson::kObjectType);
        
        testResult.AddMember("id", rapidjson::Value(result.id.c_str(), allocator), allocator);
        testResult.AddMember("category", rapidjson::Value(result.category.c_str(), allocator), allocator);
        testResult.AddMember("test_type", rapidjson::Value(result.testType.c_str(), allocator), allocator);
        testResult.AddMember("input", rapidjson::Value(result.input.c_str(), allocator), allocator);
        testResult.AddMember("expected_result", rapidjson::Value(result.expectedResult.c_str(), allocator), allocator);
        testResult.AddMember("actual_result", rapidjson::Value(result.actualResult.c_str(), allocator), allocator);
        
        // Add expected callstack
        rapidjson::Value expectedCallstack(rapidjson::kArrayType);
        for (const auto& call : result.expectedCallstack) {
            expectedCallstack.PushBack(rapidjson::Value(call.c_str(), allocator), allocator);
        }
        testResult.AddMember("expected_callstack", expectedCallstack, allocator);
        
        // Add actual callstack
        rapidjson::Value actualCallstack(rapidjson::kArrayType);
        for (const auto& call : result.actualCallstack) {
            actualCallstack.PushBack(rapidjson::Value(call.c_str(), allocator), allocator);
        }
        testResult.AddMember("actual_callstack", actualCallstack, allocator);
        
        testResult.AddMember("passed", result.passed, allocator);
        testResult.AddMember("failure_reason", rapidjson::Value(result.failureReason.c_str(), allocator), allocator);
        testResult.AddMember("execution_time_ms", result.executionTimeMs, allocator);
        testResult.AddMember("timestamp", rapidjson::Value(result.timestamp.c_str(), allocator), allocator);
        
        // Add log messages
        rapidjson::Value logMessages(rapidjson::kArrayType);
        for (const auto& log : result.logMessages) {
            logMessages.PushBack(rapidjson::Value(log.c_str(), allocator), allocator);
        }
        testResult.AddMember("log_messages", logMessages, allocator);
        
        results.PushBack(testResult, allocator);
    }
    
    doc.AddMember("results", results, allocator);
    
    // Write to file
    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);
    
    std::ofstream file(filePath);
    if (!file.is_open()) {
        return false;
    }
    
    file << buffer.GetString();
    file.close();
    
    return true;
}

bool TestResultWriter::writeToMarkdown(const std::string& filePath) {
    std::ofstream file(filePath);
    if (!file.is_open()) {
        return false;
    }
    
    file << "# Test Results Table\n\n";
    file << "Generated: " << getCurrentTimestamp() << "\n\n";
    
    // Summary
    auto summary = getSummary();
    file << "## Summary\n\n";
    file << "- **Total Tests**: " << summary.totalTests << "\n";
    file << "- **Passed**: " << summary.passed << "\n";
    file << "- **Failed**: " << summary.failed << "\n";
    file << "- **N/A (Not Test Targets)**: " << summary.notApplicable << "\n";
    file << "- **Not Yet Implemented**: " << summary.notYetImplemented << "\n";
    file << "- **Skipped**: " << summary.skipped << "\n";
    file << "- **Pass Rate**: " << std::fixed << std::setprecision(1) << summary.passRate << "%\n";
    file << "- **Total Time**: " << std::fixed << std::setprecision(2) << summary.totalTimeMs << "ms\n\n";
    
    // Group by category
    std::map<std::string, std::vector<TestExecutionDetail>> byCategory;
    for (const auto& result : _results) {
        byCategory[result.category].push_back(result);
    }
    
    // Write each category
    for (const auto& pair : byCategory) {
        const std::string& category = pair.first;
        const std::vector<TestExecutionDetail>& categoryResults = pair.second;
        file << "## " << category << "\n\n";
        
        // Table header - matching the exact format from the original specification
        file << "| ID | Test Type | Input | Expected Result | Actual Result | Expected Callstack | Pass/Fail | Failure Reason |\n";
        file << "|----|-----------|-------|-----------------|---------------|-------------------|-----------|----------------|\n";
        
        for (const auto& result : categoryResults) {
            file << "| " << result.id;
            file << " | " << result.testType;
            file << " | " << result.input;
            file << " | " << result.expectedResult;
            file << " | " << result.actualResult;
            file << " | " << formatCallstack(result.expectedCallstack);
            file << " | " << (result.passed ? "✅ PASS" : "❌ FAIL");
            file << " | " << result.failureReason;
            file << " |\n";
        }
        file << "\n";
    }
    
    // Detailed execution times
    file << "## Execution Times\n\n";
    file << "| ID | Test Type | Time (ms) |\n";
    file << "|----|-----------|----------|\n";
    
    for (const auto& result : _results) {
        file << "| " << result.id;
        file << " | " << result.testType;
        file << " | " << std::fixed << std::setprecision(2) << result.executionTimeMs;
        file << " |\n";
    }
    
    file.close();
    return true;
}

bool TestResultWriter::writeResults(const std::string& jsonPath, const std::string& markdownPath) {
    bool jsonSuccess = writeToJson(jsonPath);
    bool mdSuccess = writeToMarkdown(markdownPath);
    return jsonSuccess && mdSuccess;
}

TestResultWriter::Summary TestResultWriter::getSummary() const {
    Summary summary;
    summary.totalTests = static_cast<int>(_results.size());
    
    for (const auto& result : _results) {
        if (result.passed) {
            summary.passed++;
        } else if (result.actualResult.find("N/A") != std::string::npos) {
            summary.notApplicable++;
        } else if (hasTodoOrDieInLogs(result.logMessages)) {
            // Test failed because of TodoOrDie - not yet implemented
            summary.notYetImplemented++;
        } else if (result.failureReason.find("SKIP") != std::string::npos || 
                   result.actualResult == "SKIPPED") {
            summary.skipped++;
        } else {
            summary.failed++;
        }
        summary.totalTimeMs += result.executionTimeMs;
    }
    
    if (summary.totalTests > 0) {
        summary.passRate = (summary.passed * 100.0) / summary.totalTests;
    }
    
    return summary;
}

std::string TestResultWriter::getCurrentTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

std::string TestResultWriter::formatCallstack(const std::vector<std::string>& callstack) const {
    if (callstack.empty()) {
        return "-";
    }
    
    std::string result;
    for (size_t i = 0; i < callstack.size(); i++) {
        if (i > 0) result += " → ";
        result += callstack[i];
    }
    
    // Truncate if too long for table
    if (result.length() > 50) {
        result = result.substr(0, 47) + "...";
    }
    
    return result;
}

bool TestResultWriter::hasTodoOrDieInLogs(const std::vector<std::string>& logMessages) const {
    for (const auto& log : logMessages) {
        // Check if the log contains [TODO_OR_DIE] level marker
        if (log.find("[TODO_OR_DIE]") != std::string::npos ||
            log.find("[TODO_OR_DIE ]") != std::string::npos) {
            return true;
        }
    }
    return false;
}

} // namespace pers::tests::json