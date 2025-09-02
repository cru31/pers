#include "json_test_loader_rapidjson.h"
#include "pers/utils/Logger.h"
#include <iostream>
#include <cstring>

using namespace pers::tests::json;

void printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " [options] <test_file.json>\n";
    std::cout << "\nOptions:\n";
    std::cout << "  -s, --suite <name>    Run specific test suite\n";
    std::cout << "  -c, --category <name> Run tests from specific category\n";
    std::cout << "  -r, --report <file>   Generate report to specified file\n";
    std::cout << "  -h, --help           Show this help message\n";
    std::cout << "\nExamples:\n";
    std::cout << "  " << programName << " test_cases_2025_01_02.json\n";
    std::cout << "  " << programName << " -s quick_test test_cases_2025_01_02.json\n";
    std::cout << "  " << programName << " -c \"Critical Path\" test_cases_2025_01_02.json\n";
    std::cout << "  " << programName << " -r report.md test_cases_2025_01_02.json\n";
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
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            showHelp = true;
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
        
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}