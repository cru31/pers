#include "json_test_executor.h"
#include "pers/utils/Logger.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <thread>
#include <future>
#include <filesystem>
#include <map>
#ifdef _WIN32
#include <windows.h>
#endif

namespace pers::tests::json {

JsonTestExecutor::JsonTestExecutor() {
#ifdef _WIN32
    // Enable ANSI color codes on Windows
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    if (hOut != INVALID_HANDLE_VALUE && GetConsoleMode(hOut, &dwMode)) {
        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hOut, dwMode);
    }
#endif
}

void JsonTestExecutor::setupLogCapture() {
    _capturedLogs.clear();
    
    // Register callbacks for all log levels to capture engine messages
    auto captureCallback = [this](pers::LogLevel level, const std::string& category, 
                                  const std::string& message, const pers::LogSource& source, 
                                  bool& skipLogging) {
        if (_captureEngineLogs) {
            std::stringstream ss;
            
            // Convert LogLevel to string
            const char* levelStr = "UNKNOWN";
            switch (level) {
                case pers::LogLevel::Trace: levelStr = "TRACE"; break;
                case pers::LogLevel::Debug: levelStr = "DEBUG"; break;
                case pers::LogLevel::Info: levelStr = "INFO"; break;
                case pers::LogLevel::TodoSomeday: levelStr = "TODO_SOMEDAY"; break;
                case pers::LogLevel::Warning: levelStr = "WARNING"; break;
                case pers::LogLevel::TodoOrDie: levelStr = "TODO_OR_DIE"; break;
                case pers::LogLevel::Error: levelStr = "ERROR"; break;
                case pers::LogLevel::Critical: levelStr = "CRITICAL"; break;
            }
            
            ss << "[" << levelStr << "] ";
            ss << "[" << category << "] ";
            ss << message;
            ss << " (" << source.function << " @ " << source.file << ":" << source.line << ")";
            _capturedLogs.push_back(ss.str());
            skipLogging = true; // Don't print immediately
        }
    };
    
    // Register for all log levels
    pers::Logger::Instance().setCallback(pers::LogLevel::Trace, captureCallback);
    pers::Logger::Instance().setCallback(pers::LogLevel::Debug, captureCallback);
    pers::Logger::Instance().setCallback(pers::LogLevel::Info, captureCallback);
    pers::Logger::Instance().setCallback(pers::LogLevel::TodoSomeday, captureCallback);
    pers::Logger::Instance().setCallback(pers::LogLevel::Warning, captureCallback);
    pers::Logger::Instance().setCallback(pers::LogLevel::TodoOrDie, captureCallback);
    pers::Logger::Instance().setCallback(pers::LogLevel::Error, captureCallback);
    pers::Logger::Instance().setCallback(pers::LogLevel::Critical, captureCallback);
}

void JsonTestExecutor::printCapturedLogs() {
    if (!_capturedLogs.empty()) {
        std::cout << "\n";
        std::cout << "========================================\n";
        std::cout << "  ENGINE LOGS\n";
        std::cout << "========================================\n";
        for (const auto& log : _capturedLogs) {
            std::cout << log << "\n";
        }
        std::cout << "========================================\n\n";
    }
}

void JsonTestExecutor::runTestsFromFile(const std::string& filePath, bool printOptions, bool printEngineLogsAtEnd, bool printExecutionTime, bool truncateLongOutput) {
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
    
    // Setup log capture if requested
    _captureEngineLogs = printEngineLogsAtEnd;
    if (_captureEngineLogs) {
        setupLogCapture();
    }
    
    // Group tests by test_type for display purposes
    std::string currentGroup = "";
    std::string currentCategory = "";
    
    for (const auto& testCase : testCases) {
        // Print group header when test type changes
        if (testCase.testType != currentGroup || testCase.category != currentCategory) {
            if (!currentGroup.empty()) {
                std::cout << "\n";  // Space between groups
            }
            std::cout << "\n" << testCase.category << " - " << testCase.testType << "\n";
            std::cout << "----------------------------------------\n";
            currentGroup = testCase.testType;
            currentCategory = testCase.category;
        }
        
        if (!testCase.enabled) {
            std::cout << std::setw(4) << std::left << testCase.id << "] ";
            std::cout << "\033[90mSKIP\033[0m  ";  // Gray
            std::cout << "Test disabled";
            if (!testCase.reason.empty()) {
                std::cout << " (" << testCase.reason << ")";
            }
            std::cout << "\n";
            continue;
        }
        
        if (!checkDependencies(testCase)) {
            std::cout << std::setw(4) << std::left << testCase.id << "] ";
            std::cout << "\033[90mSKIP\033[0m  ";  // Gray
            std::cout << "Dependencies not met";
            std::cout << "\n";
            continue;
        }
        
        auto result = executeTestWithTimeout(testCase);
        _results.push_back(result);
        printTestResult(result, printOptions, printExecutionTime, truncateLongOutput);
    }
    
    // Print summary
    int passed = 0;
    int failed = 0;
    int notImplemented = 0;
    for (const auto& result : _results) {
        if (result.actualResult.find("N/A") == 0) {
            notImplemented++;
        } else if (result.passed) {
            passed++;
        } else {
            failed++;
        }
    }
    
    std::cout << "\n";
    std::cout << "========================================\n";
    std::cout << "  TEST SUMMARY\n";
    std::cout << "========================================\n";
    std::cout << "  Total: " << testCases.size() << "\n";
    std::cout << "  Implemented Tests: " << (passed + failed) << "\n";
    std::cout << "  Passed: " << passed << "\n";
    std::cout << "  Failed: " << failed << "\n";
    std::cout << "  Not Test Targets: " << notImplemented << "\n";
    std::cout << "  Pass Rate (of implemented): " << std::fixed << std::setprecision(1) 
             << ((passed + failed) > 0 ? (passed * 100.0 / (passed + failed)) : 0) << "%\n\n";
    
    // Print captured engine logs if requested
    if (_captureEngineLogs) {
        printCapturedLogs();
    }
    
    // Generate structured results in addition to the report
    generateStructuredResults();
}

void JsonTestExecutor::runTestSuite(const std::string& filePath, const std::string& suiteName, bool printOptions, bool printEngineLogsAtEnd, bool printExecutionTime, bool truncateLongOutput) {
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
    
    // Setup log capture if requested
    _captureEngineLogs = printEngineLogsAtEnd;
    if (_captureEngineLogs) {
        setupLogCapture();
    }
    
    // Group tests by test_type for display purposes
    std::string currentGroup = "";
    std::string currentCategory = "";
    
    for (const auto& testCase : suiteTests) {
        if (!testCase.enabled) {
            continue;
        }
        
        // Print group header when test type changes
        if (testCase.testType != currentGroup || testCase.category != currentCategory) {
            if (!currentGroup.empty()) {
                std::cout << "\n";  // Space between groups
            }
            std::cout << "\n" << testCase.category << " - " << testCase.testType << "\n";
            std::cout << "----------------------------------------\n";
            currentGroup = testCase.testType;
            currentCategory = testCase.category;
        }
        
        auto result = executeTestWithTimeout(testCase);
        _results.push_back(result);
        printTestResult(result, printOptions, printExecutionTime, truncateLongOutput);
    }
    
    // Print summary
    int passed = 0;
    int failed = 0;
    int notImplemented = 0;
    for (const auto& result : _results) {
        if (result.actualResult.find("N/A") == 0) {
            notImplemented++;
        } else if (result.passed) {
            passed++;
        } else {
            failed++;
        }
    }
    
    std::cout << "\n";
    std::cout << "========================================\n";
    std::cout << "  TEST SUMMARY\n";
    std::cout << "========================================\n";
    std::cout << "  Total: " << suiteTests.size() << "\n";
    std::cout << "  Implemented Tests: " << (passed + failed) << "\n";
    std::cout << "  Passed: " << passed << "\n";
    std::cout << "  Failed: " << failed << "\n";
    std::cout << "  Not Test Targets: " << notImplemented << "\n";
    std::cout << "  Pass Rate (of implemented): " << std::fixed << std::setprecision(1) 
             << ((passed + failed) > 0 ? (passed * 100.0 / (passed + failed)) : 0) << "%\n\n";
    
    // Print captured engine logs if requested
    if (_captureEngineLogs) {
        printCapturedLogs();
    }
    
    // Generate structured results
    generateStructuredResults();
}

JsonTestExecutor::TestExecutionResult JsonTestExecutor::executeTestWithTimeout(const JsonTestCase& testCase) {
    TestExecutionResult result;
    result.testCase = testCase;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Clear captured logs for this test
    std::vector<std::string> testLogs;
    
    // Execute with timeout - use shared_ptr to avoid dangling reference
    auto promise = std::make_shared<std::promise<bool>>();
    std::future<bool> future = promise->get_future();
    
    std::thread testThread([this, &testCase, &result, &testLogs, promise]() {
        // Set up log capture for all levels for this test
        auto captureTestLog = [&testLogs](pers::LogLevel level, const std::string& category, 
                                          const std::string& message, const pers::LogSource& source, 
                                          bool& skipLogging) {
            std::stringstream ss;
            
            // Format log message with level and category
            switch(level) {
                case pers::LogLevel::Trace: ss << "[TRACE] "; break;
                case pers::LogLevel::Debug: ss << "[DEBUG] "; break;
                case pers::LogLevel::Info: ss << "[INFO] "; break;
                case pers::LogLevel::TodoSomeday: ss << "[TODO_SOMEDAY] "; break;
                case pers::LogLevel::Warning: ss << "[WARNING] "; break;
                case pers::LogLevel::TodoOrDie: ss << "[TODO_OR_DIE] "; break;
                case pers::LogLevel::Error: ss << "[ERROR] "; break;
                case pers::LogLevel::Critical: ss << "[CRITICAL] "; break;
            }
            
            if (!category.empty()) {
                ss << category << ": ";
            }
            ss << message;
            ss << " (" << source.function << " @ " << source.file << ":" << source.line << ")";
            
            testLogs.push_back(ss.str());
            
            // For TodoOrDie, also print to console
            if (level == pers::LogLevel::TodoOrDie) {
                std::cout << "\n[TodoOrDie Intercepted in Test Thread] " << category << " - " << message 
                          << " at " << source.file << ":" << source.line << "\n";
            }
            
            skipLogging = false;  // Allow normal logging to continue
        };
        
        // Register callbacks for all log levels
        pers::Logger::Instance().setCallback(pers::LogLevel::Trace, captureTestLog);
        pers::Logger::Instance().setCallback(pers::LogLevel::Debug, captureTestLog);
        pers::Logger::Instance().setCallback(pers::LogLevel::Info, captureTestLog);
        pers::Logger::Instance().setCallback(pers::LogLevel::TodoSomeday, captureTestLog);
        pers::Logger::Instance().setCallback(pers::LogLevel::Warning, captureTestLog);
        pers::Logger::Instance().setCallback(pers::LogLevel::TodoOrDie, captureTestLog);
        pers::Logger::Instance().setCallback(pers::LogLevel::Error, captureTestLog);
        pers::Logger::Instance().setCallback(pers::LogLevel::Critical, captureTestLog);
        
        try {
            // Note: pers doesn't throw exceptions, it uses abort() for fatal errors
            // So we don't need try-catch here
            result.passed = _loader.executeTest(testCase, result.actualResult, result.failureReason);
            promise->set_value(true);
        } catch (...) {
            // In case promise is already satisfied (shouldn't happen but safe guard)
        }
    });
    
    if (future.wait_for(std::chrono::milliseconds(testCase.timeoutMs)) == std::future_status::timeout) {
        result.actualResult = "Timeout";
        result.failureReason = "Test exceeded timeout of " + std::to_string(testCase.timeoutMs) + "ms";
        result.passed = false;
        testThread.detach();
    } else {
        testThread.join();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    result.executionTimeMs = std::chrono::duration<double, std::milli>(end - start).count();
    
    // Store captured logs in result
    result.logMessages = std::move(testLogs);
    
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
    printTestResult(result, true);  // Default: print options
}

void JsonTestExecutor::printTestResult(const TestExecutionResult& result, bool printOptions, bool printExecutionTime, bool truncateLongOutput) {
    // Simple output for each test
    std::cout << std::setw(4) << std::left << result.testCase.id << "] ";
    
    // Status with colors (Windows ANSI enabled in constructor)
    if (result.actualResult.find("N/A") == 0) {
        std::cout << "\033[33mN/A  \033[0m  ";  // Yellow
    } else if (result.passed) {
        std::cout << "\033[32mPASS\033[0m  ";  // Green
    } else {
        std::cout << "\033[31mFAIL\033[0m  ";  // Red
    }
    
    // Shortened Expected/Actual (E:/A:)
    std::string expected = result.testCase.expectedResult;
    if (expected.length() > 15) {
        expected = expected.substr(0, 14) + ".";
    }
    std::cout << "E:" << std::left << std::setw(15) << expected << " ";
    
    std::string actual = result.actualResult;
    if (actual.length() > 15) {
        actual = actual.substr(0, 14) + ".";
    }
    std::cout << "A:" << std::left << std::setw(15) << actual << " ";
    
    // Execution Time (only if enabled)
    if (printExecutionTime) {
        std::cout << std::right << std::setw(8) << std::fixed << std::setprecision(2) 
                  << result.executionTimeMs << "ms";
    }
    
    // Print options if requested and available
    if (printOptions && !result.testCase.inputValues.empty()) {
        std::cout << " | ";
        
        std::stringstream optionsStream;
        bool first = true;
        for (const auto& [key, value] : result.testCase.inputValues) {
            if (!first) optionsStream << ", ";
            // Shorten key names for common ones
            std::string shortKey = key;
            if (key == "enableValidation") shortKey = "vali";
            else if (key == "applicationName") shortKey = "app";
            else if (key == "engineName") shortKey = "engine";
            else if (key == "type") shortKey = "type";
            else if (key == "enum_type") shortKey = "enum";
            else if (key == "value") shortKey = "val";
            else if (key == "requiredFeatures") shortKey = "feat";
            else if (key == "requiredLimits") shortKey = "limit";
            
            // Truncate individual values if needed
            std::string truncatedValue = value;
            if (truncateLongOutput) {
                const size_t maxValueLength = 11; // Maximum length for each value
                if (truncatedValue.length() > maxValueLength) {
                    truncatedValue = truncatedValue.substr(0, maxValueLength - 1) + ".";
                }
            }

            optionsStream << shortKey << "=" << truncatedValue;
            first = false;
        }
        
        std::string options = optionsStream.str();
        std::cout << options;
    }
    
    std::cout << "\n";
}

void JsonTestExecutor::generateStructuredResults(const std::string& jsonPath, const std::string& tablePath) {
    TestResultWriter writer;
    
    // Convert results to TestExecutionDetail format
    for (const auto& result : _results) {
        TestExecutionDetail detail;
        detail.id = result.testCase.id;
        detail.category = result.testCase.category;
        detail.testType = result.testCase.testType;
        
        // Format input
        if (result.testCase.inputValues.empty()) {
            detail.input = "None";
        } else {
            std::stringstream ss;
            for (const auto& kvp : result.testCase.inputValues) {
                const std::string& key = kvp.first;
                const std::string& value = kvp.second;
                if (ss.tellp() > 0) ss << ", ";
                ss << key << "=" << value;
            }
            detail.input = ss.str();
        }
        
        detail.expectedResult = result.testCase.expectedResult;
        detail.actualResult = result.actualResult;
        detail.expectedCallstack = result.testCase.expectedCallstack;
        // Note: actual callstack would need to be captured during execution
        detail.actualCallstack = {}; // Empty for now
        detail.passed = result.passed;
        detail.failureReason = result.failureReason;
        detail.executionTimeMs = result.executionTimeMs;
        detail.logMessages = result.logMessages;  // Add captured log messages
        
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream timeStream;
        timeStream << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        detail.timestamp = timeStream.str();
        
        writer.addResult(detail);
    }
    
    // Write results in both formats
    bool success = writer.writeResults(jsonPath, tablePath);
    if (success) {
        std::cout << "\nStructured results saved:\n";
        
        // Get absolute paths for clickable links
        std::filesystem::path jsonAbsPath = std::filesystem::absolute(jsonPath);
        std::filesystem::path tableAbsPath = std::filesystem::absolute(tablePath);
        
        // Output clickable file URLs (works in many terminals)
        std::cout << "  JSON:  \033[4;36mfile:///" << jsonAbsPath.string() << "\033[0m\n";
        std::cout << "  Table: \033[4;36mfile:///" << tableAbsPath.string() << "\033[0m\n";
        
        // Also output as plain paths for terminals that don't support URLs
        std::cout << "\nPlain paths:\n";
        std::cout << "  JSON:  " << jsonAbsPath.string() << "\n";
        std::cout << "  Table: " << tableAbsPath.string() << "\n";
    } else {
        std::cerr << "Failed to write structured results\n";
    }
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
    
    for (const auto& pair : byCategory) {
        const std::string& category = pair.first;
        const std::vector<TestExecutionResult>& results = pair.second;
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