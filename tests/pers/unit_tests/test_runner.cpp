#include <iostream>
#include <vector>
#include <string>
#include <functional>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <fstream>

// Include our graphics headers
#include "pers/graphics/backends/webgpu/WebGPUBackendFactory.h"
#include "pers/graphics/backends/webgpu/WebGPUInstance.h"
#include "pers/graphics/backends/webgpu/WebGPUPhysicalDevice.h"
#include "pers/graphics/backends/webgpu/WebGPULogicalDevice.h"
#include "pers/graphics/backends/webgpu/WebGPUBuffer.h"
#include "pers/graphics/backends/webgpu/WebGPUQueue.h"
#include "pers/graphics/backends/webgpu/WebGPUCommandEncoder.h"
#include "pers/graphics/backends/webgpu/WebGPURenderPassEncoder.h"
#include "pers/graphics/backends/webgpu/WebGPURenderPipeline.h"
#include "pers/utils/Logger.h"

using namespace pers;

// Test result structure
struct TestResult {
    std::string id;
    std::string category;
    std::string testType;
    std::string input;
    std::string expectedResult;
    std::string actualResult;
    std::string expectedCallstack;
    bool passed;
    std::string failureReason;
    double executionTimeMs;
};

// Test function signature
using TestFunction = std::function<TestResult()>;

class TestRunner {
public:
    TestRunner() {
        // Initialize logger
        Logger::Instance().SetMinLevel(LogLevel::Debug);
        Logger::Instance().AddOutput(std::make_shared<ConsoleOutput>(true));
        
        // Register all tests
        registerTests();
    }
    
    void runAll() {
        std::cout << "\n========================================\n";
        std::cout << "  PERS GRAPHICS ENGINE TEST SUITE\n";
        std::cout << "  Total Tests: " << _tests.size() << "\n";
        std::cout << "========================================\n\n";
        
        int passed = 0;
        int failed = 0;
        
        // Group tests by category
        std::map<std::string, std::vector<TestResult>> resultsByCategory;
        
        for (const auto& test : _tests) {
            auto start = std::chrono::high_resolution_clock::now();
            TestResult result = test.second();
            auto end = std::chrono::high_resolution_clock::now();
            
            result.executionTimeMs = std::chrono::duration<double, std::milli>(end - start).count();
            
            resultsByCategory[result.category].push_back(result);
            
            if (result.passed) {
                passed++;
                std::cout << "[PASS] ";
            } else {
                failed++;
                std::cout << "[FAIL] ";
            }
            
            std::cout << result.id << " - " << result.testType 
                     << " (" << std::fixed << std::setprecision(2) 
                     << result.executionTimeMs << "ms)\n";
            
            if (!result.passed && !result.failureReason.empty()) {
                std::cout << "       Reason: " << result.failureReason << "\n";
            }
        }
        
        // Print summary
        std::cout << "\n========================================\n";
        std::cout << "  TEST SUMMARY\n";
        std::cout << "========================================\n";
        std::cout << "  Passed: " << passed << "/" << _tests.size() << "\n";
        std::cout << "  Failed: " << failed << "/" << _tests.size() << "\n";
        std::cout << "  Pass Rate: " << std::fixed << std::setprecision(1) 
                 << (passed * 100.0 / _tests.size()) << "%\n";
        
        // Generate HTML report
        generateHtmlReport(resultsByCategory);
    }
    
private:
    std::vector<std::pair<std::string, TestFunction>> _tests;
    std::shared_ptr<WebGPUBackendFactory> _factory;
    std::shared_ptr<IInstance> _instance;
    std::shared_ptr<IPhysicalDevice> _physicalDevice;
    std::shared_ptr<ILogicalDevice> _device;
    
    void registerTests() {
        // Initialize factory for tests
        _factory = std::make_shared<WebGPUBackendFactory>();
        
        // Test 001: WebGPU Instance Creation
        _tests.push_back({"001", [this]() -> TestResult {
            TestResult result;
            result.id = "001";
            result.category = "Critical Path";
            result.testType = "WebGPU Instance Creation";
            result.input = "InstanceDesc{enableValidation=true}";
            result.expectedResult = "Valid instance created";
            result.expectedCallstack = "WebGPUBackendFactory::createInstance() -> WebGPUInstance::WebGPUInstance() -> wgpuCreateInstance()";
            
            try {
                InstanceDesc desc;
                desc.applicationName = "Test";
                desc.enableValidation = true;
                
                _instance = _factory->createInstance(desc);
                
                if (_instance) {
                    result.actualResult = "Valid instance created";
                    result.passed = true;
                } else {
                    result.actualResult = "Null instance returned";
                    result.passed = false;
                    result.failureReason = "createInstance returned nullptr";
                }
            } catch (const std::exception& e) {
                result.actualResult = "Exception thrown";
                result.passed = false;
                result.failureReason = e.what();
            }
            
            return result;
        }});
        
        // Test 002: Adapter Enumeration
        _tests.push_back({"002", [this]() -> TestResult {
            TestResult result;
            result.id = "002";
            result.category = "Critical Path";
            result.testType = "Adapter Enumeration";
            result.input = "Valid instance";
            result.expectedResult = "At least 1 adapter found";
            result.expectedCallstack = "WebGPUInstance::enumerateAdapters() -> wgpuInstanceRequestAdapter()";
            
            if (!_instance) {
                result.actualResult = "No instance available";
                result.passed = false;
                result.failureReason = "Test 001 must pass first";
                return result;
            }
            
            try {
                auto adapters = _instance->enumerateAdapters();
                
                if (!adapters.empty()) {
                    result.actualResult = std::to_string(adapters.size()) + " adapter(s) found";
                    result.passed = true;
                    _physicalDevice = adapters[0]; // Save for next tests
                } else {
                    result.actualResult = "No adapters found";
                    result.passed = false;
                    result.failureReason = "enumerateAdapters returned empty vector";
                }
            } catch (const std::exception& e) {
                result.actualResult = "Exception thrown";
                result.passed = false;
                result.failureReason = e.what();
            }
            
            return result;
        }});
        
        // Test 003: Device Creation
        _tests.push_back({"003", [this]() -> TestResult {
            TestResult result;
            result.id = "003";
            result.category = "Critical Path";
            result.testType = "Device Creation";
            result.input = "Valid adapter";
            result.expectedResult = "Valid device created";
            result.expectedCallstack = "WebGPUPhysicalDevice::createLogicalDevice() -> wgpuAdapterRequestDevice()";
            
            if (!_physicalDevice) {
                result.actualResult = "No physical device available";
                result.passed = false;
                result.failureReason = "Test 002 must pass first";
                return result;
            }
            
            try {
                DeviceDesc deviceDesc;
                deviceDesc.label = "Test Device";
                
                _device = _physicalDevice->createLogicalDevice(deviceDesc);
                
                if (_device) {
                    result.actualResult = "Valid device created";
                    result.passed = true;
                } else {
                    result.actualResult = "Null device returned";
                    result.passed = false;
                    result.failureReason = "createLogicalDevice returned nullptr";
                }
            } catch (const std::exception& e) {
                result.actualResult = "Exception thrown";
                result.passed = false;
                result.failureReason = e.what();
            }
            
            return result;
        }});
        
        // Test 004: Queue Creation
        _tests.push_back({"004", [this]() -> TestResult {
            TestResult result;
            result.id = "004";
            result.category = "Critical Path";
            result.testType = "Queue Creation";
            result.input = "Valid device";
            result.expectedResult = "Valid queue created";
            result.expectedCallstack = "WebGPULogicalDevice::createDefaultQueue() -> wgpuDeviceGetQueue()";
            
            if (!_device) {
                result.actualResult = "No device available";
                result.passed = false;
                result.failureReason = "Test 003 must pass first";
                return result;
            }
            
            try {
                auto queue = _device->getQueue();
                
                if (queue) {
                    result.actualResult = "Valid queue created";
                    result.passed = true;
                } else {
                    result.actualResult = "Null queue returned";
                    result.passed = false;
                    result.failureReason = "getQueue returned nullptr";
                }
            } catch (const std::exception& e) {
                result.actualResult = "Exception thrown";
                result.passed = false;
                result.failureReason = e.what();
            }
            
            return result;
        }});
        
        // Test 005: Command Encoder Creation
        _tests.push_back({"005", [this]() -> TestResult {
            TestResult result;
            result.id = "005";
            result.category = "Critical Path";
            result.testType = "Command Encoder Creation";
            result.input = "Valid device";
            result.expectedResult = "Valid encoder created";
            result.expectedCallstack = "WebGPULogicalDevice::createCommandEncoder() -> wgpuDeviceCreateCommandEncoder()";
            
            if (!_device) {
                result.actualResult = "No device available";
                result.passed = false;
                result.failureReason = "Test 003 must pass first";
                return result;
            }
            
            try {
                auto encoder = _device->createCommandEncoder();
                
                if (encoder) {
                    result.actualResult = "Valid encoder created";
                    result.passed = true;
                } else {
                    result.actualResult = "Null encoder returned";
                    result.passed = false;
                    result.failureReason = "createCommandEncoder returned nullptr";
                }
            } catch (const std::exception& e) {
                result.actualResult = "Exception thrown";
                result.passed = false;
                result.failureReason = e.what();
            }
            
            return result;
        }});
        
        // Add more tests here following the same pattern...
        // For now, adding placeholders for the remaining 95 tests
        
        for (int i = 6; i <= 100; i++) {
            std::string id = std::to_string(i);
            if (id.length() == 1) id = "00" + id;
            else if (id.length() == 2) id = "0" + id;
            
            _tests.push_back({id, [id]() -> TestResult {
                TestResult result;
                result.id = id;
                result.category = "Not Implemented";
                result.testType = "Test " + id;
                result.input = "TBD";
                result.expectedResult = "TBD";
                result.actualResult = "Not implemented yet";
                result.expectedCallstack = "TBD";
                result.passed = false;
                result.failureReason = "Test not implemented";
                return result;
            }});
        }
    }
    
    void generateHtmlReport(const std::map<std::string, std::vector<TestResult>>& resultsByCategory) {
        std::ofstream html("test_report.html");
        
        html << R"(<!DOCTYPE html>
<html>
<head>
    <title>Pers Graphics Engine Test Report</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; background: #f5f5f5; }
        h1 { color: #333; }
        .summary { background: white; padding: 20px; border-radius: 8px; margin-bottom: 20px; }
        .category { background: white; padding: 20px; border-radius: 8px; margin-bottom: 20px; }
        table { width: 100%; border-collapse: collapse; }
        th { background: #4CAF50; color: white; padding: 12px; text-align: left; }
        td { padding: 8px; border-bottom: 1px solid #ddd; }
        .pass { background: #d4edda; }
        .fail { background: #f8d7da; }
        .execution-time { color: #666; font-size: 0.9em; }
    </style>
</head>
<body>
    <h1>Pers Graphics Engine Test Report</h1>
    <div class="summary">
        <h2>Summary</h2>
        <p>Generated: )" << getCurrentTimestamp() << R"(</p>
        <p>Total Tests: )" << _tests.size() << R"(</p>
    </div>
)";
        
        for (const auto& [category, results] : resultsByCategory) {
            html << R"(
    <div class="category">
        <h2>)" << category << R"(</h2>
        <table>
            <tr>
                <th>ID</th>
                <th>Test Type</th>
                <th>Input</th>
                <th>Expected</th>
                <th>Actual</th>
                <th>Time (ms)</th>
                <th>Status</th>
                <th>Failure Reason</th>
            </tr>
)";
            
            for (const auto& result : results) {
                html << "            <tr class=\"" << (result.passed ? "pass" : "fail") << "\">\n";
                html << "                <td>" << result.id << "</td>\n";
                html << "                <td>" << result.testType << "</td>\n";
                html << "                <td>" << result.input << "</td>\n";
                html << "                <td>" << result.expectedResult << "</td>\n";
                html << "                <td>" << result.actualResult << "</td>\n";
                html << "                <td class=\"execution-time\">" 
                     << std::fixed << std::setprecision(2) << result.executionTimeMs << "</td>\n";
                html << "                <td>" << (result.passed ? "PASS" : "FAIL") << "</td>\n";
                html << "                <td>" << result.failureReason << "</td>\n";
                html << "            </tr>\n";
            }
            
            html << R"(        </table>
    </div>
)";
        }
        
        html << R"(</body>
</html>)";
        
        html.close();
        std::cout << "\nHTML report generated: test_report.html\n";
    }
    
    std::string getCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }
};

int main() {
    TestRunner runner;
    runner.runAll();
    return 0;
}