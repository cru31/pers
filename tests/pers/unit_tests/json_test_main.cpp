#include "json_test_loader_rapidjson.h"
#include "pers/utils/Logger.h"
#include <iostream>
#include <cstring>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#ifdef _WIN32
#include <windows.h>
#include <process.h>
#else
#include <unistd.h>
#endif

using namespace pers::tests::json;

void printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " [options] <test_file.json>\n";
    std::cout << "\nOptions:\n";
    std::cout << "  -s, --suite <name>    Run specific test suite\n";
    std::cout << "  -c, --category <name> Run tests from specific category\n";
    std::cout << "  -r, --report <file>   Generate report to specified file\n";
    std::cout << "  --jsviewer            Launch web viewer for test results\n";
    std::cout << "  -h, --help           Show this help message\n";
    std::cout << "\nExamples:\n";
    std::cout << "  " << programName << " test_cases_2025_01_02.json\n";
    std::cout << "  " << programName << " -s quick_test test_cases_2025_01_02.json\n";
    std::cout << "  " << programName << " -c \"Critical Path\" test_cases_2025_01_02.json\n";
    std::cout << "  " << programName << " -r report.md test_cases_2025_01_02.json\n";
    std::cout << "  " << programName << " --jsviewer test_cases_2025_01_02.json\n";
}

int main(int argc, char* argv[]) {
    // Set up TodoOrDie callback for testing
    // This allows us to capture TodoOrDie events without aborting
    pers::Logger::Instance().setCallback(pers::LogLevel::TodoOrDie, 
        [](pers::LogLevel level, const std::string& category, const std::string& message,
           const pers::LogSource& source, bool& skipLogging) {
        std::cout << "\n[TodoOrDie Intercepted] " << category << " - " << message 
                  << " at " << source.file << ":" << source.line << "\n";
        // Don't skip normal logging
        skipLogging = false;
        // App decides what to do - for testing, we don't abort
    });
    
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }
    
    std::string testFile;
    std::string suiteName;
    std::string categoryName;
    std::string reportFile = "json_test_results.md";
    bool showHelp = false;
    bool launchJsViewer = false;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            showHelp = true;
        }
        else if (strcmp(argv[i], "--jsviewer") == 0) {
            launchJsViewer = true;
        }
        else if ((strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--suite") == 0) && i + 1 < argc) {
            suiteName = argv[++i];
        }
        else if ((strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--category") == 0) && i + 1 < argc) {
            categoryName = argv[++i];
        }
        else if ((strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--report") == 0) && i + 1 < argc) {
            reportFile = argv[++i];
        }
        else if (argv[i][0] != '-') {
            testFile = argv[i];
        }
    }
    
    if (showHelp) {
        printUsage(argv[0]);
        return 0;
    }
    
    if (testFile.empty()) {
        std::cerr << "Error: No test file specified\n\n";
        printUsage(argv[0]);
        return 1;
    }
    
    std::cout << "========================================\n";
    std::cout << "  PERS GRAPHICS ENGINE JSON TEST RUNNER\n";
    std::cout << "========================================\n";
    std::cout << "Test File: " << testFile << "\n";
    
    JsonTestExecutor executor;
    
    try {
        if (!suiteName.empty()) {
            std::cout << "Running Suite: " << suiteName << "\n";
            executor.runTestSuite(testFile, suiteName);
        }
        else if (!categoryName.empty()) {
            std::cout << "Running Category: " << categoryName << "\n";
            // Load and filter by category
            JsonTestLoader loader;
            if (!loader.loadFromFile(testFile)) {
                std::cerr << "Failed to load test file\n";
                return 1;
            }
            
            auto categoryTests = loader.getTestsByCategory(categoryName);
            std::cout << "Found " << categoryTests.size() << " tests in category\n\n";
            
            // Execute category tests
            for (const auto& test : categoryTests) {
                if (test.enabled) {
                    std::string actualResult, failureReason;
                    bool passed = loader.executeTest(test, actualResult, failureReason);
                    
                    std::cout << "[" << test.id << "] ";
                    if (passed) {
                        std::cout << "\033[32mPASS\033[0m ";
                    } else {
                        std::cout << "\033[31mFAIL\033[0m ";
                    }
                    std::cout << test.testType << "\n";
                    
                    if (!passed && !failureReason.empty()) {
                        std::cout << "  Reason: " << failureReason << "\n";
                    }
                }
            }
        }
        else {
            // Run all tests
            executor.runTestsFromFile(testFile);
        }
        
        // Generate report
        executor.generateReport(reportFile);
        
        // Launch JS viewer if requested
        if (launchJsViewer) {
            // Generate unique session ID based on timestamp
            auto now = std::chrono::system_clock::now();
            auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()).count();
            std::string sessionId = std::to_string(timestamp);
            
            // Get absolute path for web directory
            std::filesystem::path webBaseDir = std::filesystem::absolute("D:/cru31.dev/pers_work/pers_repo/tests/pers/unit_tests/web");
            std::filesystem::path webDir = webBaseDir / sessionId;
            std::filesystem::path dataDir = webDir / "data";
            std::filesystem::create_directories(dataDir);
            
            // Copy test results JSON to data directory
            std::filesystem::path jsonResultPath = std::filesystem::current_path() / "test_results.json";
            std::filesystem::path targetPath = dataDir / "result.json";
            
            if (std::filesystem::exists(jsonResultPath)) {
                std::filesystem::copy_file(jsonResultPath, targetPath,
                    std::filesystem::copy_options::overwrite_existing);
                std::cout << "\nTest results copied to: " << targetPath.string() << "\n";
            } else {
                std::cerr << "Warning: test_results.json not found at " << jsonResultPath << "\n";
            }
            
            std::cout << "\n========================================\n";
            std::cout << "  LAUNCHING WEB VIEWER\n";
            std::cout << "  Session ID: " << sessionId << "\n";
            std::cout << "  Data Path: " << targetPath.string() << "\n";
            std::cout << "========================================\n";
            
            // Kill any existing server on port 5000
            std::cout << "Checking for existing server on port 5000...\n";
            
#ifdef _WIN32
            // Find and kill any Node.js process using port 5000
            // First, find the PID using netstat
            std::string findCommand = "for /f \"tokens=5\" %a in ('netstat -aon ^| findstr :5000 ^| findstr LISTENING') do @echo %a";
            std::string killBatchFile = webBaseDir.string() + "/kill_server.bat";
            std::ofstream killBatch(killBatchFile);
            killBatch << "@echo off\n";
            killBatch << "for /f \"tokens=5\" %%a in ('netstat -aon ^| findstr :5000 ^| findstr LISTENING') do (\n";
            killBatch << "    echo Killing existing server with PID %%a\n";
            killBatch << "    taskkill /F /PID %%a >nul 2>&1\n";
            killBatch << ")\n";
            killBatch.close();
            
            // Execute the kill script
            system(killBatchFile.c_str());
            
            // Small delay to ensure process is terminated
            Sleep(500);
            
            // Launch Node.js server
            std::string serverScript = webBaseDir.string() + "/server.js";
            
            // Create a batch file to run the server
            std::string batchFile = webBaseDir.string() + "/run_server.bat";
            std::ofstream batch(batchFile);
            batch << "@echo off\n";
            batch << "cd /d \"" << webBaseDir.string() << "\"\n";
            batch << "node server.js " << sessionId << "\n";
            batch.close();
            
            // Launch server in new window
            std::string serverCommand = "start \"Test Results Viewer\" cmd /c \"" + batchFile + "\"";
            system(serverCommand.c_str());
            
            // Wait for server to start
            std::cout << "Starting server...\n";
            Sleep(2000);
            
            // Open browser
            std::string browserCommand = "start http://localhost:5000";
            system(browserCommand.c_str());
#else
            // Kill any existing server on port 5000
            std::cout << "Checking for existing server on port 5000...\n";
            system("lsof -ti:5000 | xargs -r kill -9 2>/dev/null");
            
            // Small delay to ensure process is terminated
            usleep(500000); // 500ms
            
            // Launch server in background
            std::string command = "cd " + webBaseDir.string() + " && node server.js " + sessionId + " &";
            system(command.c_str());
            
            // Wait a bit for server to start
            sleep(2);
            
            // Open browser (try different commands for different systems)
            system("xdg-open http://localhost:5000 2>/dev/null || open http://localhost:5000");
#endif
            
            std::cout << "\nWeb viewer launched at http://localhost:5000\n";
            std::cout << "Server is running in a separate window\n";
            std::cout << "Close the server window when done viewing results\n";
        }
        
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}