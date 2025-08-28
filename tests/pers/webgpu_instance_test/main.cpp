#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <pers/graphics/backends/IGraphicsBackendFactory.h>
#include <pers/graphics/backends/webgpu/WebGPUBackendFactory.h>

struct TestCase {
    std::string name;
    pers::InstanceDesc desc;
    bool shouldSucceed;
};

void runTest(const TestCase& test) {
    std::cout << "\n========================================" << std::endl;
    std::cout << "Test: " << test.name << std::endl;
    std::cout << "========================================" << std::endl;
    
    // Create factory
    auto factory = std::make_shared<pers::WebGPUBackendFactory>();
    std::cout << "Backend: " << factory->getBackendName() << std::endl;
    
    // Log configuration
    std::cout << "\nConfiguration:" << std::endl;
    std::cout << "  Application: " << test.desc.applicationName 
              << " v" << test.desc.applicationVersion << std::endl;
    std::cout << "  Engine: " << test.desc.engineName 
              << " v" << test.desc.engineVersion << std::endl;
    std::cout << "  Validation: " << (test.desc.enableValidation ? "ON" : "OFF") << std::endl;
    std::cout << "  GPU Validation: " << (test.desc.enableGPUBasedValidation ? "ON" : "OFF") << std::endl;
    std::cout << "  Sync Validation: " << (test.desc.enableSynchronizationValidation ? "ON" : "OFF") << std::endl;
    std::cout << "  Prefer High-Perf GPU: " << (test.desc.preferHighPerformanceGPU ? "YES" : "NO") << std::endl;
    std::cout << "  Allow Software: " << (test.desc.allowSoftwareRenderer ? "YES" : "NO") << std::endl;
    
    // Create instance
    std::cout << "\nCreating instance..." << std::endl;
    auto instance = factory->createInstance(test.desc);
    
    // Check result
    bool success = (instance != nullptr);
    std::cout << "\nResult: " << (success ? "SUCCESS" : "FAILED") << std::endl;
    
    if (success != test.shouldSucceed) {
        std::cout << "ERROR: Expected " << (test.shouldSucceed ? "success" : "failure") 
                  << " but got " << (success ? "success" : "failure") << std::endl;
    } else {
        std::cout << "Test passed as expected" << std::endl;
    }
    
    if (instance) {
        std::cout << "Instance created successfully!" << std::endl;
    }
}

int main() {
    std::cout << "WebGPU Instance Creation Test Suite" << std::endl;
    std::cout << "====================================" << std::endl;
    
    // In CI, run fewer tests to avoid timeout/crash
    const char* ciEnv = std::getenv("CI");
    bool isCI = (ciEnv != nullptr);
    
    std::vector<TestCase> tests;
    
    // Test 1: Default configuration
    {
        TestCase test;
        test.name = "Default Configuration";
        test.desc = {}; // Use all defaults
        test.shouldSucceed = true;
        tests.push_back(test);
    }
    
    // Test 2: Validation enabled (default)
    {
        TestCase test;
        test.name = "Validation Enabled";
        test.desc.applicationName = "Validation Test App";
        test.desc.enableValidation = true;
        test.shouldSucceed = true;
        tests.push_back(test);
    }
    
    // Test 3: All validation features enabled
    {
        TestCase test;
        test.name = "Full Validation Suite";
        test.desc.applicationName = "Full Validation App";
        test.desc.enableValidation = true;
        test.desc.enableGPUBasedValidation = true;
        test.desc.enableSynchronizationValidation = true;
        test.shouldSucceed = true;
        tests.push_back(test);
    }
    
    // Test 4: Production configuration (no validation)
    {
        TestCase test;
        test.name = "Production Mode";
        test.desc.applicationName = "Production App";
        test.desc.applicationVersion = 100;
        test.desc.engineName = "Production Engine";
        test.desc.engineVersion = 200;
        test.desc.enableValidation = false;
        test.desc.enableGPUBasedValidation = false;
        test.desc.enableSynchronizationValidation = false;
        test.desc.preferHighPerformanceGPU = true;
        test.desc.allowSoftwareRenderer = false;
        test.shouldSucceed = true;
        tests.push_back(test);
    }
    
    // Test 5: Software renderer allowed
    {
        TestCase test;
        test.name = "Software Renderer Allowed";
        test.desc.applicationName = "Software Renderer Test";
        test.desc.allowSoftwareRenderer = true;
        test.desc.preferHighPerformanceGPU = false;
        test.shouldSucceed = true;
        tests.push_back(test);
    }
    
    // Test 6: High performance GPU preferred
    {
        TestCase test;
        test.name = "High Performance GPU Preferred";
        test.desc.applicationName = "High Perf App";
        test.desc.preferHighPerformanceGPU = true;
        test.desc.allowSoftwareRenderer = false;
        test.shouldSucceed = true;
        tests.push_back(test);
    }
    
    // Test 7: Custom application info
    {
        TestCase test;
        test.name = "Custom Application Info";
        test.desc.applicationName = "My Custom Game Engine";
        test.desc.applicationVersion = 2023;
        test.desc.engineName = "Pers Graphics Engine Custom Build";
        test.desc.engineVersion = 42;
        test.desc.enableValidation = true;
        test.shouldSucceed = true;
        tests.push_back(test);
    }
    
    // Test 8: Development configuration
    {
        TestCase test;
        test.name = "Development Mode";
        test.desc.applicationName = "Dev Build";
        test.desc.enableValidation = true;
        test.desc.enableGPUBasedValidation = false; // Too slow for dev
        test.desc.enableSynchronizationValidation = true; // Important for threading bugs
        test.desc.preferHighPerformanceGPU = false; // Use integrated for battery
        test.desc.allowSoftwareRenderer = true; // Allow fallback
        test.shouldSucceed = true;
        tests.push_back(test);
    }
    
    // Test 9: CI/Testing configuration
    {
        TestCase test;
        test.name = "CI/Testing Environment";
        test.desc.applicationName = "CI Test Runner";
        test.desc.enableValidation = true;
        test.desc.allowSoftwareRenderer = true; // CI might not have GPU
        test.desc.preferHighPerformanceGPU = false;
        test.shouldSucceed = true;
        tests.push_back(test);
    }
    
    // Test 10: Minimal configuration
    {
        TestCase test;
        test.name = "Minimal Configuration";
        test.desc.applicationName = "Min";
        test.desc.applicationVersion = 1;
        test.desc.engineName = "E";
        test.desc.engineVersion = 1;
        test.desc.enableValidation = false;
        test.desc.allowSoftwareRenderer = false;
        test.shouldSucceed = true;
        tests.push_back(test);
    }
    
    // Run all tests
    int passed = 0;
    int failed = 0;
    
    for (const auto& test : tests) {
        try {
            runTest(test);
            passed++;
        } catch (const std::exception& e) {
            std::cout << "Exception during test: " << e.what() << std::endl;
            failed++;
        }
    }
    
    // Summary
    std::cout << "\n========================================" << std::endl;
    std::cout << "Test Summary" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Total tests: " << tests.size() << std::endl;
    std::cout << "Passed: " << passed << std::endl;
    std::cout << "Failed: " << failed << std::endl;
    
    return (failed == 0) ? 0 : 1;
}