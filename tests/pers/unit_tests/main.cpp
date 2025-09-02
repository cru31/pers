#include "test_framework.h"
#include "pers/utils/Logger.h"

using namespace pers;
using namespace pers::tests;

// Declare test suite registration functions
extern void registerCriticalPathTests(TestRegistry& registry);
extern void registerMemorySafetyTests(TestRegistry& registry);
extern void registerTypeConversionTests(TestRegistry& registry);
extern void registerResourceManagementTests(TestRegistry& registry);
extern void registerErrorHandlingTests(TestRegistry& registry);

int main(int argc, char* argv[]) {
    // Initialize logger
    Logger::Instance().SetMinLevel(LogLevel::Info);
    Logger::Instance().AddOutput(std::make_shared<ConsoleOutput>(true));
    
    // Create file output for detailed logs
    auto fileOutput = std::make_shared<FileOutput>("unit_test_log.txt", false);
    Logger::Instance().AddOutput(fileOutput);
    
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════╗\n";
    std::cout << "║      PERS GRAPHICS ENGINE - UNIT TEST SUITE         ║\n";
    std::cout << "║         Windows Platform - WebGPU Backend           ║\n";
    std::cout << "╚══════════════════════════════════════════════════════╝\n";
    std::cout << "\n";
    
    // Create test registry
    TestRegistry registry;
    
    // Register all test suites
    std::cout << "Registering test suites...\n";
    registerCriticalPathTests(registry);
    registerMemorySafetyTests(registry);
    registerTypeConversionTests(registry);
    registerResourceManagementTests(registry);
    registerErrorHandlingTests(registry);
    
    // Run all tests
    registry.runAll();
    
    // Flush logs
    Logger::Instance().Flush();
    
    std::cout << "\nTest execution complete. Check test_results.md for detailed report.\n";
    std::cout << "Log file: unit_test_log.txt\n\n";
    
    return 0;
}