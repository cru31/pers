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
    STARTUPINFOA si = {0};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    
    PROCESS_INFORMATION pi = {0};
    std::string serverCmd = "cmd.exe /c \"" + runBatchFile + "\"";
    
    if (CreateProcessA(NULL, (LPSTR)serverCmd.c_str(), NULL, NULL, FALSE, 
                       CREATE_NO_WINDOW, NULL, sessionDir.c_str(), &si, &pi)) {
        std::cout << "[INFO] Starting web server from: " << sessionDir << std::endl;
        Sleep(2000);
        
        // Open browser
        std::cout << "[INFO] Opening test results viewer at http://localhost:5000" << std::endl;
        system("start http://localhost:5000");
        
        // Wait for browser to load the page
        std::cout << "[INFO] Waiting for browser to load..." << std::endl;
        Sleep(5000);
        
        // Kill the server process
        std::cout << "[INFO] Shutting down web server..." << std::endl;
        TerminateProcess(pi.hProcess, 0);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    } else {
        std::cerr << "[ERROR] Failed to start web server" << std::endl;
    }
}
#endif

void printTestResult(const TestTypeDefinition& testType, 
                     const TestVariation& variation, 
                     const TestResult& result) {
    std::cout << "[" << (result.passed ? "PASS" : "FAIL") << "] ";
    std::cout << testType.category << " - " << testType.testType << " - " << variation.variationName;
    std::cout << " (ID: " << variation.combinedId << ")";
    
    if (!result.passed) {
        std::cout << "\n  Reason: " << result.failureReason;
        std::cout << "\n  Actual: " << result.actualBehavior;
    }
    std::cout << std::endl;
}

// Include handler headers
#include "handlers/instance_creation_handler.h"
#include "handlers/request_adapter_handler.h"  
#include "handlers/device_creation_handler.h"
#include "handlers/swapchain_builder_handler.h"
#include "handlers/buffer/buffer_data_verification_handler.h"
#include "handlers/buffer/webgpu_buffer_handler.h"

void registerAllHandlers() {
    auto& registry = TestHandlerRegistry::Instance();
    
    // Manually register all handlers here
    registry.registerHandler("Instance Creation", 
        std::make_shared<InstanceCreationHandler>());
    registry.registerHandler("Request Adapter", 
        std::make_shared<RequestAdapterHandler>());
    registry.registerHandler("Device Creation", 
        std::make_shared<DeviceCreationHandler>());
    registry.registerHandler("SwapChain Builder Negotiation", 
        std::make_shared<SwapChainBuilderNegotiationHandler>());
    registry.registerHandler("Buffer Data Verification",
        std::make_shared<BufferDataVerificationHandler>());
    registry.registerHandler("CPU → GPU → CPU Data Integrity Verification",
        std::make_shared<BufferDataVerificationHandler>());
    registry.registerHandler("WebGPUBuffer Test",
        std::make_shared<WebGPUBufferHandler>());

}
int main(int argc, char* argv[]) {
    // Register all handlers at the start of main
    registerAllHandlers();
    
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
    std::string inputPath;
    
    if (argc >= 2) {
        inputPath = argv[1];
        if (argc >= 3) {
            outputFile = argv[2];
        }
    } else {
        // Use default directory from CMake or fallback
#ifdef TEST_CASES_DIR
        inputPath = TEST_CASES_DIR;
#else
        inputPath = ".";
#endif
        std::cout << "Using default test cases directory: " << inputPath << std::endl;
    }
    
    std::cout << "================================" << std::endl;
    std::cout << "Pers Graphics Engine Unit Tests" << std::endl;
    std::cout << "================================" << std::endl;
    
    // Load test definitions from directory or single file
    std::vector<TestTypeDefinition> testTypes;
    fs::path inputFs(inputPath);
    
    if (fs::is_directory(inputFs)) {
        std::cout << "Loading test cases from directory: " << inputPath << std::endl;
        
        // Find all JSON files in the directory
        std::vector<std::string> jsonFiles;
        for (const auto& entry : fs::directory_iterator(inputFs)) {
            if (entry.path().extension() == ".json") {
                jsonFiles.push_back(entry.path().string());
            }
        }
        
        if (jsonFiles.empty()) {
            std::cerr << "No JSON files found in directory: " << inputPath << std::endl;
            return 1;
        }
        
        std::cout << "Found " << jsonFiles.size() << " JSON file(s)" << std::endl;
        
        // Load test types from all JSON files
        for (const auto& jsonFile : jsonFiles) {
            std::cout << "  Loading: " << fs::path(jsonFile).filename().string() << std::endl;
            if (!JsonTestLoader::loadTestTypes(jsonFile, testTypes)) {
                std::cerr << "  Warning: Failed to load test cases from " << jsonFile << std::endl;
            }
        }
        
        if (testTypes.empty()) {
            std::cerr << "No test types loaded from any JSON files" << std::endl;
            return 1;
        }
    } else if (fs::is_regular_file(inputFs)) {
        std::cout << "Loading test cases from file: " << inputPath << std::endl;
        if (!JsonTestLoader::loadTestTypes(inputPath, testTypes)) {
            std::cerr << "Failed to load test cases from " << inputPath << std::endl;
            return 1;
        }
    } else {
        std::cerr << "Input path is neither a file nor a directory: " << inputPath << std::endl;
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
                         << " - " << variation.variationName << " (ID: " << variation.combinedId << ")" << std::endl;
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
                    if (log.level == "TODO_OR_DIE") {
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
    
    // Get absolute path for input path (directory or file)
    fs::path inputAbsolutePath = fs::absolute(inputPath);
    
    // Save results
    if (JsonTestLoader::saveTestResults(outputFile, testTypes, allResults, inputAbsolutePath.string())) {
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
    
    // Clear all handlers before main exits to ensure proper destruction order
    registry.clear();
    
    return (failedCount == 0) ? 0 : 1;
}