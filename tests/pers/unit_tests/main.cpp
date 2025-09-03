#include "test_handler_base.h"
#include "json_test_loader.h"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <cstdlib>
#include <thread>
#include <fstream>
#include <sstream>
#include <ctime>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#include <tlhelp32.h>
#pragma comment(lib, "Ws2_32.lib")
#endif

using namespace pers::tests;
namespace fs = std::filesystem;

#ifdef _WIN32
void startWebServer(const std::string& sessionDir, const std::string& resultPath) {
    // Get the executable directory's absolute path
    char exePathBuffer[MAX_PATH];
    GetModuleFileNameA(NULL, exePathBuffer, MAX_PATH);
    fs::path exeFullPath = fs::path(exePathBuffer);
    fs::path exeDir = exeFullPath.parent_path();
    fs::path nodeModulesPath = exeDir / "web_resources" / "node_modules";
    
    // Kill existing server on port 5000 using batch file
    std::cout << "[INFO] Checking for existing web server on port 5000..." << std::endl;
    
    std::string killBatchFile = sessionDir + "/kill_server.bat";
    std::ofstream killBatch(killBatchFile);
    killBatch << "@echo off\n";
    killBatch << "for /f \"tokens=5\" %%a in ('netstat -aon ^| findstr :5000 ^| findstr LISTENING') do (\n";
    killBatch << "    echo Killing existing server with PID %%a\n";
    killBatch << "    taskkill /F /PID %%a >nul 2>&1\n";
    killBatch << ")\n";
    killBatch.close();
    
    // Execute the kill script
    system(killBatchFile.c_str());
    
    // Wait for port to be released
    Sleep(500);
    
    // Create a batch file to run the server
    std::string runBatchFile = sessionDir + "/run_server.bat";
    std::ofstream runBatch(runBatchFile);
    runBatch << "@echo off\n";
    runBatch << "cd /d \"" << sessionDir << "\"\n";
    
    // Set NODE_MODULES_PATH environment variable
    runBatch << "set NODE_MODULES_PATH=\"" << nodeModulesPath.string() << "\"\n";
    runBatch << "node server.js \"" << resultPath << "\"\n";
    runBatch.close();
    
    // Start the server in a new hidden window
    std::string serverCmd = "start /B \"\" \"" + runBatchFile + "\"";
    system(serverCmd.c_str());
    
    std::cout << "[INFO] Starting web server from: " << sessionDir << std::endl;
    Sleep(2000);
    
    // Open browser
    std::cout << "[INFO] Opening test results viewer at http://localhost:5000" << std::endl;
    system("start http://localhost:5000");
}
#endif

void printTestResult(const TestTypeDefinition& testType, 
                     const TestVariation& variation, 
                     const TestResult& result) {
    std::cout << "[" << (result.passed ? "PASS" : "FAIL") << "] ";
    std::cout << testType.category << " - " << testType.testType << " - " << variation.variationName;
    std::cout << " (ID: " << variation.id << ")";
    
    if (!result.passed) {
        std::cout << "\n  Reason: " << result.failureReason;
        std::cout << "\n  Actual: " << result.actualBehavior;
    }
    std::cout << std::endl;
}

int main(int argc, char* argv[]) {
    std::string inputFile;
    
    // Create timestamp directory for results
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S");
    ss << "_" << std::setfill('0') << std::setw(3) << ms.count();
    
    std::string resultsDir = "./unit_test_results/" + ss.str();
    fs::create_directories(resultsDir);
    
    // Copy web server files to session directory
#ifdef WEB_SERVER_DIR
    std::string webSourceDir = WEB_SERVER_DIR;
    
    // Copy server.js
    fs::copy_file(webSourceDir + "/server.js", resultsDir + "/server.js", 
                  fs::copy_options::overwrite_existing);
    
    // Copy public directory
    if (fs::exists(webSourceDir + "/public")) {
        fs::copy(webSourceDir + "/public", resultsDir + "/public",
                 fs::copy_options::recursive | fs::copy_options::overwrite_existing);
    }
    
    // Copy package.json if exists
    if (fs::exists(webSourceDir + "/package.json")) {
        fs::copy_file(webSourceDir + "/package.json", resultsDir + "/package.json", 
                      fs::copy_options::overwrite_existing);
    }
#endif
    
    std::string outputFile = resultsDir + "/result.json";
    
    if (argc >= 2) {
        inputFile = argv[1];
        if (argc >= 3) {
            outputFile = argv[2];
        }
    } else {
        // Use default path from CMake or fallback
#ifdef TEST_CASES_JSON
        inputFile = TEST_CASES_JSON;
#else
        inputFile = "test_cases.json";
#endif
        std::cout << "Using default test cases file: " << inputFile << std::endl;
    }
    
    std::cout << "================================" << std::endl;
    std::cout << "Pers Graphics Engine Unit Tests" << std::endl;
    std::cout << "================================" << std::endl;
    std::cout << "Loading tests from: " << inputFile << std::endl;
    
    // Load test definitions
    std::vector<TestTypeDefinition> testTypes;
    if (!JsonTestLoader::loadTestTypes(inputFile, testTypes)) {
        std::cerr << "Failed to load test cases from " << inputFile << std::endl;
        return 1;
    }
    
    std::cout << "Loaded " << testTypes.size() << " test types" << std::endl;
    
    // Count total variations
    size_t totalTests = 0;
    for (const auto& testType : testTypes) {
        totalTests += testType.variations.size();
    }
    std::cout << "Total test variations: " << totalTests << std::endl << std::endl;
    
    // Execute tests
    auto& registry = TestHandlerRegistry::Instance();
    std::vector<std::vector<TestResult>> allResults;
    
    size_t passedCount = 0;
    size_t failedCount = 0;
    size_t testNotImplementedCount = 0;  // Test handler not implemented
    size_t engineFeatureNYICount = 0;    // Engine feature not yet implemented (TodoOrDie)
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    for (const auto& testType : testTypes) {
        std::vector<TestResult> typeResults;
        
        // Get handler for this test type
        auto handler = registry.getHandler(testType.testType);
        
        if (!handler) {
            std::cout << "No handler for test type: " << testType.testType << std::endl;
            
            // Mark all variations as NYI
            for (const auto& variation : testType.variations) {
                TestResult result;
                result.passed = false;
                result.actualBehavior = "Test Not Implemented";
                result.failureReason = "No test handler for " + testType.testType;
                typeResults.push_back(result);
                testNotImplementedCount++;
                
                std::cout << "[NYI ] " << testType.category << " - " << testType.testType 
                         << " - " << variation.variationName << " (ID: " << variation.id << ")" << std::endl;
            }
        } else {
            // Execute each variation
            for (const auto& variation : testType.variations) {
                // Measure execution time
                auto testStart = std::chrono::high_resolution_clock::now();
                TestResult result = handler->execute(variation);
                auto testEnd = std::chrono::high_resolution_clock::now();
                
                // Calculate execution time in milliseconds
                auto testDuration = std::chrono::duration_cast<std::chrono::microseconds>(testEnd - testStart);
                double executionTimeMs = testDuration.count() / 1000.0;
                result.actualProperties["executionTime"] = executionTimeMs;
                
                // Check for TodoOrDie logs to detect engine NYI
                bool hasTodoOrDie = false;
                for (const auto& log : result.logMessages) {
                    if (log.find("[TODO_OR_DIE]") != std::string::npos ||
                        log.find("[TODO!]") != std::string::npos) {
                        hasTodoOrDie = true;
                        break;
                    }
                }
                
                typeResults.push_back(result);
                
                if (result.passed) {
                    passedCount++;
                } else if (hasTodoOrDie) {
                    engineFeatureNYICount++;
                } else {
                    failedCount++;
                }
                
                printTestResult(testType, variation, result);
            }
        }
        
        allResults.push_back(typeResults);
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    // Print summary
    std::cout << std::endl;
    std::cout << "================================" << std::endl;
    std::cout << "Test Summary" << std::endl;
    std::cout << "================================" << std::endl;
    std::cout << "Total Tests: " << totalTests << std::endl;
    std::cout << "Passed:      " << passedCount << " (" 
              << std::fixed << std::setprecision(1) 
              << (100.0 * passedCount / totalTests) << "%)" << std::endl;
    std::cout << "Failed:      " << failedCount << " (" 
              << std::fixed << std::setprecision(1)
              << (100.0 * failedCount / totalTests) << "%)" << std::endl;
    std::cout << "Test NYI:    " << testNotImplementedCount << " (" 
              << std::fixed << std::setprecision(1)
              << (100.0 * testNotImplementedCount / totalTests) << "%)" << std::endl;
    std::cout << "Engine NYI:  " << engineFeatureNYICount << " (" 
              << std::fixed << std::setprecision(1)
              << (100.0 * engineFeatureNYICount / totalTests) << "%)" << std::endl;
    std::cout << "Time:        " << duration.count() << " ms" << std::endl;
    
    // Get absolute path for input file
    fs::path inputFilePath = fs::absolute(inputFile);
    
    // Save results
    if (JsonTestLoader::saveTestResults(outputFile, testTypes, allResults, inputFilePath.string())) {
        std::cout << "Results saved to: " << outputFile << std::endl;
    } else {
        std::cerr << "Failed to save results to: " << outputFile << std::endl;
    }
    
#ifdef _WIN32
    // Get absolute paths
    fs::path absoluteResultPath = fs::absolute(outputFile);
    fs::path absoluteSessionDir = fs::absolute(resultsDir);
    
    // Start web server to view results
    startWebServer(absoluteSessionDir.string(), absoluteResultPath.string());
#endif
    
    return (failedCount == 0) ? 0 : 1;
}