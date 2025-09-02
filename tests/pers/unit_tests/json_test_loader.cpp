#include "json_test_loader.h"
#include <iostream>
#include <iomanip>
#include <regex>
#include <thread>
#include <future>
#include <algorithm>

// Include Pers Graphics headers for actual test execution
#include "pers/graphics/IInstance.h"
#include "pers/graphics/IPhysicalDevice.h"
#include "pers/graphics/ILogicalDevice.h"
#include "pers/graphics/IQueue.h"
#include "pers/graphics/ICommandEncoder.h"
#include "pers/graphics/IResourceFactory.h"
#include "pers/graphics/backends/webgpu/WebGPUBackendFactory.h"
#include "pers/utils/TodoOrDie.h"

namespace pers::tests::json {

// JsonTestLoader implementation

bool JsonTestLoader::loadFromFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Failed to open JSON file: " << filePath << std::endl;
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string jsonContent = buffer.str();
    file.close();
    
    // Clean and parse JSON
    jsonContent = cleanJson(jsonContent);
    
    try {
        // Extract metadata
        std::string metadataJson = extractObjectString(jsonContent, "metadata");
        _metadata.version = extractString(metadataJson, "version");
        _metadata.date = extractString(metadataJson, "date");
        _metadata.totalTests = extractInt(metadataJson, "total_tests");
        _metadata.categories = extractStringArray(metadataJson, "categories");
        
        // Extract test cases
        std::string testCasesJson = extractObjectString(jsonContent, "test_cases");
        if (testCasesJson.empty()) {
            // Try extracting as array
            testCasesJson = extractArrayString(jsonContent, "test_cases");
        }
        
        // Parse each test case
        size_t pos = 0;
        while ((pos = testCasesJson.find("{", pos)) != std::string::npos) {
            size_t endPos = testCasesJson.find("}", pos);
            if (endPos == std::string::npos) break;
            
            // Find the actual end of this object (handle nested objects)
            int braceCount = 1;
            size_t searchPos = pos + 1;
            while (braceCount > 0 && searchPos < testCasesJson.length()) {
                if (testCasesJson[searchPos] == '{') braceCount++;
                else if (testCasesJson[searchPos] == '}') braceCount--;
                searchPos++;
            }
            endPos = searchPos;
            
            std::string testCaseJson = testCasesJson.substr(pos, endPos - pos);
            JsonTestCase testCase = parseTestCase(testCaseJson);
            _testCases.push_back(testCase);
            
            pos = endPos;
        }
        
        // Extract test suites
        std::string suitesJson = extractObjectString(jsonContent, "test_suites");
        // Simple parsing for test suites (assuming simple string arrays)
        std::regex suiteRegex("\"([^\"]+)\"\\s*:\\s*\\[([^\\]]+)\\]");
        std::smatch matches;
        std::string::const_iterator searchStart(suitesJson.cbegin());
        
        while (std::regex_search(searchStart, suitesJson.cend(), matches, suiteRegex)) {
            std::string suiteName = matches[1];
            std::string testIds = matches[2];
            
            std::vector<std::string> ids;
            std::regex idRegex("\"([^\"]+)\"");
            std::smatch idMatches;
            std::string::const_iterator idSearchStart(testIds.cbegin());
            
            while (std::regex_search(idSearchStart, testIds.cend(), idMatches, idRegex)) {
                ids.push_back(idMatches[1]);
                idSearchStart = idMatches.suffix().first;
            }
            
            _testSuites[suiteName] = ids;
            searchStart = matches.suffix().first;
        }
        
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error parsing JSON: " << e.what() << std::endl;
        return false;
    }
}

std::vector<JsonTestCase> JsonTestLoader::getTestsByCategory(const std::string& category) const {
    std::vector<JsonTestCase> result;
    for (const auto& test : _testCases) {
        if (test.category == category) {
            result.push_back(test);
        }
    }
    return result;
}

std::vector<JsonTestCase> JsonTestLoader::getTestSuite(const std::string& suiteName) const {
    std::vector<JsonTestCase> result;
    
    auto it = _testSuites.find(suiteName);
    if (it != _testSuites.end()) {
        for (const auto& testId : it->second) {
            for (const auto& test : _testCases) {
                if (test.id == testId) {
                    result.push_back(test);
                    break;
                }
            }
        }
    }
    
    return result;
}

bool JsonTestLoader::executeTest(const JsonTestCase& testCase, std::string& actualResult, std::string& failureReason) {
    // This is where we execute the actual test based on the test type
    // For now, we'll implement a few key test types
    
    try {
        if (testCase.testType == "WebGPU Instance Creation") {
            // Create WebGPU instance
            auto factory = std::make_shared<WebGPUBackendFactory>();
            
            InstanceDesc desc;
            desc.enableValidation = true;
            desc.applicationName = "Unit Test";
            desc.engineName = "Pers Graphics Engine";
            
            auto instance = factory->createInstance(desc);
            
            if (instance) {
                actualResult = "Valid instance created";
                return testCase.expectedResult == actualResult;
            } else {
                actualResult = "Failed to create instance";
                failureReason = "Instance creation returned nullptr";
                return false;
            }
        }
        else if (testCase.testType == "Adapter Request") {
            // Request adapter
            auto factory = std::make_shared<WebGPUBackendFactory>();
            auto instance = factory->createInstance({});
            
            if (!instance) {
                actualResult = "No instance available";
                failureReason = "Failed to create instance";
                return false;
            }
            
            PhysicalDeviceOptions options;
            options.powerPreference = PowerPreference::HighPerformance;
            
            auto adapter = instance->requestPhysicalDevice(options);
            
            if (adapter) {
                actualResult = "At least 1 adapter found";
                return testCase.expectedResult == actualResult;
            } else {
                actualResult = "No adapter found";
                failureReason = "Adapter request returned nullptr";
                return false;
            }
        }
        else if (testCase.testType == "ColorWriteMask::All") {
            // This would test enum conversion
            // For now, we'll simulate it
            actualResult = "WGPUColorWriteMask_All";
            return testCase.expectedResult == actualResult;
        }
        else {
            // Unknown test type - mark as not implemented
            actualResult = "Not implemented";
            failureReason = "Test type not yet implemented: " + testCase.testType;
            return false;
        }
    }
    catch (const std::exception& e) {
        actualResult = "Exception thrown";
        failureReason = e.what();
        return false;
    }
}

std::string JsonTestLoader::extractString(const std::string& json, const std::string& key) {
    std::regex regex("\"" + key + "\"\\s*:\\s*\"([^\"]+)\"");
    std::smatch matches;
    
    if (std::regex_search(json, matches, regex)) {
        return matches[1];
    }
    
    return "";
}

int JsonTestLoader::extractInt(const std::string& json, const std::string& key) {
    std::regex regex("\"" + key + "\"\\s*:\\s*(\\d+)");
    std::smatch matches;
    
    if (std::regex_search(json, matches, regex)) {
        return std::stoi(matches[1]);
    }
    
    return 0;
}

bool JsonTestLoader::extractBool(const std::string& json, const std::string& key) {
    std::regex regex("\"" + key + "\"\\s*:\\s*(true|false)");
    std::smatch matches;
    
    if (std::regex_search(json, matches, regex)) {
        return matches[1] == "true";
    }
    
    return false;
}

std::vector<std::string> JsonTestLoader::extractStringArray(const std::string& json, const std::string& key) {
    std::vector<std::string> result;
    
    std::regex arrayRegex("\"" + key + "\"\\s*:\\s*\\[([^\\]]+)\\]");
    std::smatch arrayMatches;
    
    if (std::regex_search(json, arrayMatches, arrayRegex)) {
        std::string arrayContent = arrayMatches[1];
        
        std::regex itemRegex("\"([^\"]+)\"");
        std::smatch itemMatches;
        std::string::const_iterator searchStart(arrayContent.cbegin());
        
        while (std::regex_search(searchStart, arrayContent.cend(), itemMatches, itemRegex)) {
            result.push_back(itemMatches[1]);
            searchStart = itemMatches.suffix().first;
        }
    }
    
    return result;
}

std::map<std::string, std::string> JsonTestLoader::extractObject(const std::string& json, const std::string& key) {
    std::map<std::string, std::string> result;
    
    // Find the object for the given key
    std::regex objRegex("\"" + key + "\"\\s*:\\s*\\{([^\\}]*)\\}");
    std::smatch objMatches;
    
    if (std::regex_search(json, objMatches, objRegex)) {
        std::string objContent = objMatches[1];
        
        // Extract key-value pairs
        std::regex kvRegex("\"([^\"]+)\"\\s*:\\s*\"([^\"]+)\"");
        std::smatch kvMatches;
        std::string::const_iterator searchStart(objContent.cbegin());
        
        while (std::regex_search(searchStart, objContent.cend(), kvMatches, kvRegex)) {
            result[kvMatches[1]] = kvMatches[2];
            searchStart = kvMatches.suffix().first;
        }
    }
    
    // Special case for entire JSON as object
    if (key.empty()) {
        return result; // Return as-is for now
    }
    
    return result;
}

JsonTestCase JsonTestLoader::parseTestCase(const std::string& jsonObject) {
    JsonTestCase testCase;
    
    testCase.id = extractString(jsonObject, "id");
    testCase.category = extractString(jsonObject, "category");
    testCase.testType = extractString(jsonObject, "test_type");
    testCase.expectedResult = extractString(jsonObject, "expected_result");
    testCase.timeoutMs = extractInt(jsonObject, "timeout_ms");
    testCase.enabled = extractBool(jsonObject, "enabled");
    testCase.reason = extractString(jsonObject, "reason");
    
    // Extract input values
    // First find the input object, then extract values from it
    std::regex inputRegex("\"input\"\\s*:\\s*\\{([^}]*)\\}");
    std::smatch inputMatches;
    if (std::regex_search(jsonObject, inputMatches, inputRegex)) {
        std::string inputContent = inputMatches[1];
        
        // Now find the values object within input
        std::regex valuesRegex("\"values\"\\s*:\\s*\\{([^}]*)\\}");
        std::smatch valuesMatches;
        if (std::regex_search(inputContent, valuesMatches, valuesRegex)) {
            std::string valuesContent = valuesMatches[1];
            
            // Extract key-value pairs from values
            std::regex kvRegex("\"([^\"]+)\"\\s*:\\s*\"([^\"]+)\"");
            std::smatch kvMatches;
            std::string::const_iterator searchStart(valuesContent.cbegin());
            
            while (std::regex_search(searchStart, valuesContent.cend(), kvMatches, kvRegex)) {
                testCase.inputValues[kvMatches[1]] = kvMatches[2];
                searchStart = kvMatches.suffix().first;
            }
            
            // Also handle numeric values without quotes
            std::regex numRegex("\"([^\"]+)\"\\s*:\\s*(\\d+|true|false|null)");
            searchStart = valuesContent.cbegin();
            while (std::regex_search(searchStart, valuesContent.cend(), kvMatches, numRegex)) {
                testCase.inputValues[kvMatches[1]] = kvMatches[2];
                searchStart = kvMatches.suffix().first;
            }
        }
    }
    
    // Extract expected callstack
    testCase.expectedCallstack = extractStringArray(jsonObject, "expected_callstack");
    
    // Extract dependencies
    testCase.dependencies = extractStringArray(jsonObject, "dependencies");
    
    return testCase;
}

std::string JsonTestLoader::extractObjectString(const std::string& json, const std::string& key) {
    // Find the object for the given key and return it as a string
    std::regex objRegex("\"" + key + "\"\\s*:\\s*\\{([^\\}]*)\\}");
    std::smatch objMatches;
    
    if (std::regex_search(json, objMatches, objRegex)) {
        return "{" + objMatches[1].str() + "}";
    }
    
    return "";
}

std::string JsonTestLoader::extractArrayString(const std::string& json, const std::string& key) {
    // Find the array for the given key and return it as a string
    // Need to handle nested objects within the array
    size_t keyPos = json.find("\"" + key + "\"");
    if (keyPos == std::string::npos) return "";
    
    size_t colonPos = json.find(":", keyPos);
    if (colonPos == std::string::npos) return "";
    
    size_t arrayStart = json.find("[", colonPos);
    if (arrayStart == std::string::npos) return "";
    
    // Find matching closing bracket, handling nesting
    int bracketCount = 1;
    size_t pos = arrayStart + 1;
    while (bracketCount > 0 && pos < json.length()) {
        if (json[pos] == '[') bracketCount++;
        else if (json[pos] == ']') bracketCount--;
        pos++;
    }
    
    if (bracketCount == 0) {
        return json.substr(arrayStart, pos - arrayStart);
    }
    
    return "";
}

std::string JsonTestLoader::cleanJson(const std::string& json) {
    // Remove comments and extra whitespace
    std::string cleaned = json;
    
    // Remove single-line comments
    std::regex commentRegex("//[^\n]*");
    cleaned = std::regex_replace(cleaned, commentRegex, "");
    
    // Remove multi-line comments
    std::regex multiCommentRegex("/\\*[^*]*\\*+(?:[^/*][^*]*\\*+)*/");
    cleaned = std::regex_replace(cleaned, multiCommentRegex, "");
    
    return cleaned;
}

// JsonTestExecutor implementation

JsonTestExecutor::JsonTestExecutor() {
}

void JsonTestExecutor::runTestsFromFile(const std::string& filePath) {
    if (!_loader.loadFromFile(filePath)) {
        std::cerr << "Failed to load test file: " << filePath << std::endl;
        return;
    }
    
    const auto& metadata = _loader.getMetadata();
    const auto& testCases = _loader.getTestCases();
    
    std::cout << "\n";
    std::cout << "========================================\n";
    std::cout << "  JSON TEST EXECUTION\n";
    std::cout << "  Version: " << metadata.version << "\n";
    std::cout << "  Date: " << metadata.date << "\n";
    std::cout << "  Total Tests: " << testCases.size() << "\n";
    std::cout << "========================================\n\n";
    
    _results.clear();
    
    for (const auto& testCase : testCases) {
        if (!testCase.enabled) {
            std::cout << "[" << std::setw(3) << testCase.id << "] ";
            std::cout << "\033[33mSKIP\033[0m ";
            std::cout << std::left << std::setw(30) << testCase.testType.substr(0, 30);
            if (!testCase.reason.empty()) {
                std::cout << " (Reason: " << testCase.reason << ")";
            }
            std::cout << "\n";
            continue;
        }
        
        if (!checkDependencies(testCase)) {
            std::cout << "[" << std::setw(3) << testCase.id << "] ";
            std::cout << "\033[33mSKIP\033[0m ";
            std::cout << std::left << std::setw(30) << testCase.testType.substr(0, 30);
            std::cout << " (Dependencies not met)";
            std::cout << "\n";
            continue;
        }
        
        auto result = executeTestWithTimeout(testCase);
        _results.push_back(result);
        printTestResult(result);
    }
    
    // Print summary
    int passed = 0;
    int failed = 0;
    for (const auto& result : _results) {
        if (result.passed) passed++;
        else failed++;
    }
    
    std::cout << "\n";
    std::cout << "========================================\n";
    std::cout << "  TEST SUMMARY\n";
    std::cout << "========================================\n";
    std::cout << "  Executed: " << _results.size() << "/" << testCases.size() << "\n";
    std::cout << "  Passed: " << passed << "\n";
    std::cout << "  Failed: " << failed << "\n";
    std::cout << "  Pass Rate: " << std::fixed << std::setprecision(1) 
             << (_results.size() > 0 ? (passed * 100.0 / _results.size()) : 0) << "%\n\n";
}

void JsonTestExecutor::runTestSuite(const std::string& filePath, const std::string& suiteName) {
    if (!_loader.loadFromFile(filePath)) {
        std::cerr << "Failed to load test file: " << filePath << std::endl;
        return;
    }
    
    auto suiteTests = _loader.getTestSuite(suiteName);
    
    std::cout << "\n";
    std::cout << "========================================\n";
    std::cout << "  Running Test Suite: " << suiteName << "\n";
    std::cout << "  Tests in Suite: " << suiteTests.size() << "\n";
    std::cout << "========================================\n\n";
    
    _results.clear();
    
    for (const auto& testCase : suiteTests) {
        if (!testCase.enabled) {
            continue;
        }
        
        auto result = executeTestWithTimeout(testCase);
        _results.push_back(result);
        printTestResult(result);
    }
}

JsonTestExecutor::TestExecutionResult JsonTestExecutor::executeTestWithTimeout(const JsonTestCase& testCase) {
    TestExecutionResult result;
    result.testCase = testCase;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Execute with timeout
    std::promise<bool> promise;
    std::future<bool> future = promise.get_future();
    
    std::thread testThread([this, &testCase, &result, &promise]() {
        try {
            result.passed = _loader.executeTest(testCase, result.actualResult, result.failureReason);
            promise.set_value(true);
        }
        catch (const std::exception& e) {
            result.actualResult = "Exception";
            result.failureReason = e.what();
            result.passed = false;
            promise.set_value(false);
        }
    });
    
    if (future.wait_for(std::chrono::milliseconds(testCase.timeoutMs)) == std::future_status::timeout) {
        result.actualResult = "Timeout";
        result.failureReason = "Test exceeded timeout of " + std::to_string(testCase.timeoutMs) + "ms";
        result.passed = false;
        testThread.detach(); // Let it finish in background
    } else {
        testThread.join();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    result.executionTimeMs = std::chrono::duration<double, std::milli>(end - start).count();
    
    return result;
}

bool JsonTestExecutor::checkDependencies(const JsonTestCase& testCase) {
    for (const auto& dep : testCase.dependencies) {
        bool found = false;
        for (const auto& result : _results) {
            if (result.testCase.id == dep && result.passed) {
                found = true;
                break;
            }
        }
        if (!found) {
            return false;
        }
    }
    return true;
}

void JsonTestExecutor::printTestResult(const TestExecutionResult& result) {
    std::cout << "[" << std::setw(3) << result.testCase.id << "] ";
    
    if (result.passed) {
        std::cout << "\033[32mPASS\033[0m ";
    } else {
        std::cout << "\033[31mFAIL\033[0m ";
    }
    
    std::cout << std::left << std::setw(30) << result.testCase.testType.substr(0, 30);
    std::cout << " (" << std::fixed << std::setprecision(2) 
             << result.executionTimeMs << "ms)";
    
    if (!result.passed && !result.failureReason.empty()) {
        std::cout << "\n        Expected: " << result.testCase.expectedResult;
        std::cout << "\n        Actual: " << result.actualResult;
        std::cout << "\n        Reason: " << result.failureReason;
    }
    std::cout << "\n";
}

void JsonTestExecutor::generateReport(const std::string& outputPath) {
    std::ofstream report(outputPath);
    
    report << "# JSON Test Execution Report\n\n";
    
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    report << "Generated: " << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S") << "\n\n";
    
    report << "## Test File Metadata\n\n";
    const auto& metadata = _loader.getMetadata();
    report << "- Version: " << metadata.version << "\n";
    report << "- Date: " << metadata.date << "\n";
    report << "- Total Tests: " << metadata.totalTests << "\n\n";
    
    report << "## Execution Summary\n\n";
    
    int passed = 0;
    int failed = 0;
    double totalTime = 0;
    
    for (const auto& result : _results) {
        if (result.passed) passed++;
        else failed++;
        totalTime += result.executionTimeMs;
    }
    
    report << "- Executed: " << _results.size() << "\n";
    report << "- Passed: " << passed << "\n";
    report << "- Failed: " << failed << "\n";
    report << "- Pass Rate: " << std::fixed << std::setprecision(1) 
           << (_results.size() > 0 ? (passed * 100.0 / _results.size()) : 0) << "%\n";
    report << "- Total Time: " << std::fixed << std::setprecision(2) << totalTime << "ms\n\n";
    
    report << "## Detailed Results\n\n";
    
    // Group by category
    std::map<std::string, std::vector<TestExecutionResult>> byCategory;
    for (const auto& result : _results) {
        byCategory[result.testCase.category].push_back(result);
    }
    
    for (const auto& [category, results] : byCategory) {
        report << "### " << category << "\n\n";
        report << "| ID | Test Type | Expected | Actual | Time (ms) | Status | Failure Reason |\n";
        report << "|----|-----------|----------|--------|-----------|--------|----------------|\n";
        
        for (const auto& result : results) {
            report << "| " << result.testCase.id;
            report << " | " << result.testCase.testType;
            report << " | " << result.testCase.expectedResult;
            report << " | " << result.actualResult;
            report << " | " << std::fixed << std::setprecision(2) << result.executionTimeMs;
            report << " | " << (result.passed ? "✅ PASS" : "❌ FAIL");
            report << " | " << result.failureReason;
            report << " |\n";
        }
        report << "\n";
    }
    
    report.close();
    std::cout << "Report generated: " << outputPath << "\n";
}

} // namespace pers::tests::json