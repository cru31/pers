#include "json_test_loader_rapidjson.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <thread>
#include <future>
#include <algorithm>
#include <cstdio>
#include <filesystem>

// Include Pers Graphics headers for actual test execution
#include "pers/graphics/IInstance.h"
#include "pers/graphics/IPhysicalDevice.h"
#include "pers/graphics/ILogicalDevice.h"
#include "pers/graphics/IQueue.h"
#include "pers/graphics/ICommandEncoder.h"
#include "pers/graphics/IResourceFactory.h"
#include "pers/graphics/backends/webgpu/WebGPUBackendFactory.h"
#include "pers/utils/Logger.h"

namespace pers::tests::json {

bool JsonTestLoader::loadFromFile(const std::string& filePath) {
    FILE* fp = std::fopen(filePath.c_str(), "rb");
    if (!fp) {
        std::cerr << "Failed to open JSON file: " << filePath << std::endl;
        return false;
    }
    
    char readBuffer[65536];
    rapidjson::FileReadStream is(fp, readBuffer, sizeof(readBuffer));
    
    _document.ParseStream(is);
    std::fclose(fp);
    
    if (_document.HasParseError()) {
        std::cerr << "JSON parse error at offset " << _document.GetErrorOffset() << std::endl;
        return false;
    }
    
    // Parse metadata
    if (_document.HasMember("metadata") && _document["metadata"].IsObject()) {
        const auto& metadata = _document["metadata"];
        
        if (metadata.HasMember("version") && metadata["version"].IsString()) {
            _metadata.version = metadata["version"].GetString();
        }
        if (metadata.HasMember("date") && metadata["date"].IsString()) {
            _metadata.date = metadata["date"].GetString();
        }
        if (metadata.HasMember("total_tests") && metadata["total_tests"].IsInt()) {
            _metadata.totalTests = metadata["total_tests"].GetInt();
        }
        if (metadata.HasMember("categories") && metadata["categories"].IsArray()) {
            const auto& categories = metadata["categories"];
            for (rapidjson::SizeType i = 0; i < categories.Size(); i++) {
                if (categories[i].IsString()) {
                    _metadata.categories.push_back(categories[i].GetString());
                }
            }
        }
    }
    
    // Parse test cases
    if (_document.HasMember("test_cases") && _document["test_cases"].IsArray()) {
        const auto& testCases = _document["test_cases"];
        for (rapidjson::SizeType i = 0; i < testCases.Size(); i++) {
            if (testCases[i].IsObject()) {
                JsonTestCase testCase = parseTestCase(testCases[i]);
                _testCases.push_back(testCase);
            }
        }
    }
    
    // Parse test suites
    if (_document.HasMember("test_suites") && _document["test_suites"].IsObject()) {
        const auto& suites = _document["test_suites"];
        for (auto it = suites.MemberBegin(); it != suites.MemberEnd(); ++it) {
            std::string suiteName = it->name.GetString();
            std::vector<std::string> testIds;
            
            if (it->value.IsArray()) {
                const auto& ids = it->value;
                for (rapidjson::SizeType i = 0; i < ids.Size(); i++) {
                    if (ids[i].IsString()) {
                        testIds.push_back(ids[i].GetString());
                    }
                }
            } else if (it->value.IsString() && std::string(it->value.GetString()) == "all") {
                // Special case for "all"
                for (const auto& tc : _testCases) {
                    testIds.push_back(tc.id);
                }
            }
            
            _testSuites[suiteName] = testIds;
        }
    }
    
    return true;
}

JsonTestCase JsonTestLoader::parseTestCase(const rapidjson::Value& value) {
    JsonTestCase testCase;
    
    if (value.HasMember("id") && value["id"].IsString()) {
        testCase.id = value["id"].GetString();
    }
    
    if (value.HasMember("category") && value["category"].IsString()) {
        testCase.category = value["category"].GetString();
    }
    
    if (value.HasMember("test_type") && value["test_type"].IsString()) {
        testCase.testType = value["test_type"].GetString();
    }
    
    if (value.HasMember("input") && value["input"].IsObject()) {
        const auto& input = value["input"];
        
        // Check for type field
        if (input.HasMember("type") && input["type"].IsString()) {
            testCase.inputValues["type"] = input["type"].GetString();
        }
        
        // Parse options for option-based tests
        if (input.HasMember("options") && input["options"].IsObject()) {
            const auto& options = input["options"];
            for (auto it = options.MemberBegin(); it != options.MemberEnd(); ++it) {
                std::string key = it->name.GetString();
                std::string val;
                
                if (it->value.IsString()) {
                    val = it->value.GetString();
                } else if (it->value.IsInt()) {
                    val = std::to_string(it->value.GetInt());
                } else if (it->value.IsBool()) {
                    val = it->value.GetBool() ? "true" : "false";
                } else if (it->value.IsNull()) {
                    val = "null";
                } else if (it->value.IsArray()) {
                    // Handle array values
                    std::stringstream ss;
                    ss << "[";
                    for (rapidjson::SizeType i = 0; i < it->value.Size(); i++) {
                        if (i > 0) ss << ",";
                        if (it->value[i].IsString()) {
                            ss << "\"" << it->value[i].GetString() << "\"";
                        }
                    }
                    ss << "]";
                    val = ss.str();
                }
                
                testCase.inputValues[key] = val;
            }
        }
        
        // Parse values for regular tests
        if (input.HasMember("values") && input["values"].IsObject()) {
            const auto& values = input["values"];
            for (auto it = values.MemberBegin(); it != values.MemberEnd(); ++it) {
                std::string key = it->name.GetString();
                std::string val;
                
                if (it->value.IsString()) {
                    val = it->value.GetString();
                } else if (it->value.IsInt()) {
                    val = std::to_string(it->value.GetInt());
                } else if (it->value.IsBool()) {
                    val = it->value.GetBool() ? "true" : "false";
                } else if (it->value.IsNull()) {
                    val = "null";
                } else if (it->value.IsArray()) {
                    // Handle array values
                    std::stringstream ss;
                    ss << "[";
                    for (rapidjson::SizeType i = 0; i < it->value.Size(); i++) {
                        if (i > 0) ss << ",";
                        if (it->value[i].IsString()) {
                            ss << "\"" << it->value[i].GetString() << "\"";
                        }
                    }
                    ss << "]";
                    val = ss.str();
                }
                
                testCase.inputValues[key] = val;
            }
        }
    }
    
    if (value.HasMember("expected_result") && value["expected_result"].IsString()) {
        testCase.expectedResult = value["expected_result"].GetString();
    }
    
    if (value.HasMember("expected_callstack") && value["expected_callstack"].IsArray()) {
        const auto& callstack = value["expected_callstack"];
        for (rapidjson::SizeType i = 0; i < callstack.Size(); i++) {
            if (callstack[i].IsString()) {
                testCase.expectedCallstack.push_back(callstack[i].GetString());
            }
        }
    }
    
    if (value.HasMember("timeout_ms") && value["timeout_ms"].IsInt()) {
        testCase.timeoutMs = value["timeout_ms"].GetInt();
    }
    
    if (value.HasMember("enabled") && value["enabled"].IsBool()) {
        testCase.enabled = value["enabled"].GetBool();
    }
    
    if (value.HasMember("dependencies") && value["dependencies"].IsArray()) {
        const auto& deps = value["dependencies"];
        for (rapidjson::SizeType i = 0; i < deps.Size(); i++) {
            if (deps[i].IsString()) {
                testCase.dependencies.push_back(deps[i].GetString());
            }
        }
    }
    
    if (value.HasMember("reason") && value["reason"].IsString()) {
        testCase.reason = value["reason"].GetString();
    }
    
    return testCase;
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

bool JsonTestLoader::executeOptionBasedTest(const JsonTestCase& testCase, std::string& actualResult, std::string& failureReason) {
    // Handle option-based tests
    auto optionsIt = testCase.inputValues.find("options");
    if (optionsIt == testCase.inputValues.end()) {
        // Options are stored directly in inputValues for option-based tests
        // Check if we have any input values at all
        if (testCase.inputValues.empty()) {
            failureReason = "No options found in option-based test";
            return false;
        }
    }
    
    // Extract base test type
    std::string baseType = testCase.testType;
    
    if (baseType == "WebGPU Instance Creation") {
        auto factory = std::make_shared<WebGPUBackendFactory>();
        InstanceDesc desc;
        
        // Apply options - use inputValues directly if no "options" key
        const auto& options = (optionsIt != testCase.inputValues.end()) ? 
            testCase.inputValues : testCase.inputValues;
        if (options.find("validation") != options.end()) {
            desc.enableValidation = (options.at("validation") == "true");
        }
        if (options.find("application_name") != options.end()) {
            desc.applicationName = options.at("application_name");
        }
        if (options.find("debug_mode") != options.end()) {
            // This would set debug mode if available in InstanceDesc
        }
        
        auto instance = factory->createInstance(desc);
        if (instance) {
            actualResult = "Success with options";
            return testCase.expectedResult == actualResult || testCase.expectedResult == "Valid with validation";
        } else {
            actualResult = "Failed to create instance";
            failureReason = "Instance creation returned nullptr with options";
            return false;
        }
    }
    else if (baseType == "Buffer Creation") {
        auto factory = std::make_shared<WebGPUBackendFactory>();
        auto instance = factory->createInstance({});
        if (!instance) {
            actualResult = "No instance";
            return false;
        }
        
        auto physicalDevice = instance->requestPhysicalDevice({});
        if (!physicalDevice) {
            actualResult = "No physical device";
            return false;
        }
        
        auto device = physicalDevice->createLogicalDevice({});
        if (!device) {
            actualResult = "No device";
            return false;
        }
        
        auto resourceFactory = device->getResourceFactory();
        if (!resourceFactory) {
            actualResult = "No resource factory";
            return false;
        }
        
        // Parse buffer options
        BufferDesc bufferDesc;
        const auto& options = (optionsIt != testCase.inputValues.end()) ? 
            testCase.inputValues : testCase.inputValues;
        
        if (options.find("size") != options.end()) {
            bufferDesc.size = std::stoull(options.at("size"));
        }
        if (options.find("label") != options.end()) {
            bufferDesc.debugName = options.at("label");
        }
        if (options.find("usage") != options.end()) {
            // Parse usage flags
            std::string usage = options.at("usage");
            bufferDesc.usage = BufferUsage::None;
            if (usage.find("Vertex") != std::string::npos) {
                bufferDesc.usage = static_cast<BufferUsage>(
                    static_cast<uint32_t>(bufferDesc.usage) | static_cast<uint32_t>(BufferUsage::Vertex));
            }
            if (usage.find("Index") != std::string::npos) {
                bufferDesc.usage = static_cast<BufferUsage>(
                    static_cast<uint32_t>(bufferDesc.usage) | static_cast<uint32_t>(BufferUsage::Index));
            }
            if (usage.find("Uniform") != std::string::npos) {
                bufferDesc.usage = static_cast<BufferUsage>(
                    static_cast<uint32_t>(bufferDesc.usage) | static_cast<uint32_t>(BufferUsage::Uniform));
            }
            if (usage.find("Storage") != std::string::npos) {
                bufferDesc.usage = static_cast<BufferUsage>(
                    static_cast<uint32_t>(bufferDesc.usage) | static_cast<uint32_t>(BufferUsage::Storage));
            }
            if (usage.find("CopySrc") != std::string::npos) {
                bufferDesc.usage = static_cast<BufferUsage>(
                    static_cast<uint32_t>(bufferDesc.usage) | static_cast<uint32_t>(BufferUsage::CopySrc));
            }
            if (usage.find("CopyDst") != std::string::npos) {
                bufferDesc.usage = static_cast<BufferUsage>(
                    static_cast<uint32_t>(bufferDesc.usage) | static_cast<uint32_t>(BufferUsage::CopyDst));
            }
        }
        
        // Special case for size 0
        if (bufferDesc.size == 0) {
            auto buffer = resourceFactory->createBuffer(bufferDesc);
            if (!buffer) {
                actualResult = "Returns nullptr";
                return testCase.expectedResult == actualResult;
            } else {
                actualResult = "Buffer created with 0 size";
                failureReason = "Should have returned nullptr for 0 size";
                return false;
            }
        }
        
        auto buffer = resourceFactory->createBuffer(bufferDesc);
        if (buffer) {
            actualResult = "Success with options";
            return true;
        } else {
            actualResult = "Failed to create buffer";
            failureReason = "Buffer creation failed with options";
            return false;
        }
    }
    
    // Add more base types as needed
    failureReason = "Option-based test type not implemented: " + baseType;
    return false;
}

bool JsonTestLoader::executeTest(const JsonTestCase& testCase, std::string& actualResult, std::string& failureReason) {
    // This is where we execute the actual test based on the test type
    // Note: pers doesn't use exceptions, it uses abort() for fatal errors
    
    // Check if this is an option-based test
    if (testCase.inputValues.find("type") != testCase.inputValues.end() && 
        testCase.inputValues.at("type") == "OptionBased") {
        return executeOptionBasedTest(testCase, actualResult, failureReason);
    }
    
    if (testCase.testType == "WebGPU Instance Creation") {
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
        else if (testCase.testType == "Device Creation") {
            auto factory = std::make_shared<WebGPUBackendFactory>();
            auto instance = factory->createInstance({});
            if (!instance) {
                actualResult = "No instance";
                failureReason = "Failed to create instance";
                return false;
            }
            
            auto adapter = instance->requestPhysicalDevice({});
            if (!adapter) {
                actualResult = "No adapter";
                failureReason = "Failed to get adapter";
                return false;
            }
            
            LogicalDeviceDesc deviceDesc;
            deviceDesc.debugName = "Test Device";
            
            auto device = adapter->createLogicalDevice(deviceDesc);
            if (device) {
                actualResult = "Valid device created";
                return testCase.expectedResult == actualResult;
            } else {
                actualResult = "Failed to create device";
                failureReason = "Device creation returned nullptr";
                return false;
            }
        }
        else if (testCase.testType == "Queue Creation") {
            // Actually test queue creation
            auto factory = std::make_shared<WebGPUBackendFactory>();
            auto instance = factory->createInstance({});
            if (!instance) {
                actualResult = "Failed to create instance";
                failureReason = "Instance required for queue test";
                return false;
            }
            
            auto physicalDevice = instance->requestPhysicalDevice({});
            if (!physicalDevice) {
                actualResult = "Failed to get physical device";
                failureReason = "Physical device required for queue test";
                return false;
            }
            
            auto device = physicalDevice->createLogicalDevice({});
            if (!device) {
                actualResult = "Failed to create device";
                failureReason = "Device required for queue test";
                return false;
            }
            
            // Test the actual queue creation
            auto queue = device->getQueue();
            if (queue) {
                actualResult = "Valid queue created";
                return testCase.expectedResult == actualResult;
            } else {
                actualResult = "Queue is nullptr";
                failureReason = "getQueue() returned nullptr";
                return false;
            }
        }
        else if (testCase.testType == "Command Encoder Creation") {
            // Actually test command encoder creation
            auto factory = std::make_shared<WebGPUBackendFactory>();
            auto instance = factory->createInstance({});
            auto physicalDevice = instance->requestPhysicalDevice({});
            auto device = physicalDevice->createLogicalDevice({});
            
            if (!device) {
                actualResult = "Failed to create device";
                failureReason = "Device required for command encoder test";
                return false;
            }
            
            // Test command encoder creation
            auto commandEncoder = device->createCommandEncoder();
            if (commandEncoder) {
                actualResult = "Valid encoder created";
                return testCase.expectedResult == actualResult;
            } else {
                actualResult = "Command encoder is nullptr";
                failureReason = "createCommandEncoder() returned nullptr";
                return false;
            }
        }
        else if (testCase.testType.find("Buffer") != std::string::npos) {
            // Buffer related tests
            auto factory = std::make_shared<WebGPUBackendFactory>();
            auto instance = factory->createInstance({});
            auto physicalDevice = instance->requestPhysicalDevice({});
            auto device = physicalDevice->createLogicalDevice({});
            
            if (!device) {
                actualResult = "Failed to create device";
                failureReason = "Device required for buffer test";
                return false;
            }
            
            auto resourceFactory = device->getResourceFactory();
            if (!resourceFactory) {
                actualResult = "No resource factory";
                failureReason = "getResourceFactory() returned nullptr";
                return false;
            }
            
            if (testCase.testType == "Buffer Creation 64KB") {
                BufferDesc desc;
                desc.size = 65536;
                desc.usage = BufferUsage::Storage;
                desc.debugName = "Test Buffer 64KB";
                
                auto buffer = resourceFactory->createBuffer(desc);
                if (buffer) {
                    actualResult = "Valid buffer";
                    return testCase.expectedResult == actualResult;
                } else {
                    actualResult = "Buffer is nullptr";
                    failureReason = "createBuffer() returned nullptr";
                    return false;
                }
            }
            else if (testCase.testType == "Buffer Creation 0 Size") {
                BufferDesc desc;
                desc.size = 0;
                desc.usage = BufferUsage::Vertex;
                
                auto buffer = resourceFactory->createBuffer(desc);
                if (!buffer) {
                    actualResult = "Returns nullptr";
                    return testCase.expectedResult == actualResult;
                } else {
                    actualResult = "Buffer created with 0 size";
                    failureReason = "Should have returned nullptr for 0 size";
                    return false;
                }
            }
            else {
                // Other buffer tests not yet implemented
                actualResult = "Not implemented";
                failureReason = "Buffer test not yet implemented: " + testCase.testType;
                return false;
            }
        }
        else if (testCase.testType.find("ColorWriteMask") != std::string::npos ||
                 testCase.testType.find("BufferUsage") != std::string::npos ||
                 testCase.testType.find("TextureFormat") != std::string::npos) {
            // Type conversion tests - these need the actual conversion functions to be tested
            // For now, we can't test these properly without exposing the conversion functions
            actualResult = "Not implemented";
            failureReason = "Type conversion test requires exposing conversion functions: " + testCase.testType;
            return false;
        }
        else {
            // Other test types not implemented
            actualResult = "Not implemented";
            failureReason = "Test type not yet implemented: " + testCase.testType;
            return false;
        }
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
    
    // Generate structured results in addition to the report
    generateStructuredResults();
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
    
    // Execute with timeout - use shared_ptr to avoid dangling reference
    auto promise = std::make_shared<std::promise<bool>>();
    std::future<bool> future = promise->get_future();
    
    std::thread testThread([this, &testCase, &result, promise]() {
        // Set up TodoOrDie callback for this thread
        pers::Logger::Instance().setCallback(pers::LogLevel::TodoOrDie,
            [](pers::LogLevel level, const std::string& category, const std::string& message,
               const pers::LogSource& source, bool& skipLogging) {
            std::cout << "\n[TodoOrDie Intercepted in Test Thread] " << category << " - " << message 
                      << " at " << source.file << ":" << source.line << "\n";
            skipLogging = false;
            // Don't abort during tests
        });
        
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
    
    // Print option details if this is an option-based test
    bool hasOptions = false;
    if (result.testCase.inputValues.find("type") != result.testCase.inputValues.end() &&
        result.testCase.inputValues.at("type") == "OptionBased") {
        hasOptions = true;
        std::cout << " with options: ";
        
        // Print key options
        bool first = true;
        for (const auto& [key, value] : result.testCase.inputValues) {
            if (key != "type") {  // Skip the type field
                if (!first) std::cout << ", ";
                std::cout << key << "=" << value;
                first = false;
            }
        }
    }
    
    if (!result.passed && !result.failureReason.empty()) {
        std::cout << "\n        Expected: " << result.testCase.expectedResult;
        std::cout << "\n        Actual: " << result.actualResult;
        std::cout << "\n        Reason: " << result.failureReason;
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