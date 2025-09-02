#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include "json_test_loader_rapidjson.h"

namespace pers::tests::json {

// Base interface for all test handlers
class ITestHandler {
public:
    virtual ~ITestHandler() = default;
    
    // Execute the test with given test case
    virtual bool execute(const JsonTestCase& testCase, 
                        std::string& actualResult, 
                        std::string& failureReason) = 0;
    
    // Get the test type this handler handles
    virtual std::string getTestType() const = 0;
    
    // Check if this handler can handle the given test case
    virtual bool canHandle(const JsonTestCase& testCase) const {
        return testCase.testType == getTestType();
    }
};

// Registry for test handlers
class TestHandlerRegistry {
public:
    static TestHandlerRegistry& Instance() {
        static TestHandlerRegistry instance;
        return instance;
    }
    
    // Register a handler for a specific test type
    void registerHandler(const std::string& testType, std::shared_ptr<ITestHandler> handler) {
        _handlers[testType] = handler;
    }
    
    // Get handler for a test type
    std::shared_ptr<ITestHandler> getHandler(const std::string& testType) const {
        auto it = _handlers.find(testType);
        if (it != _handlers.end()) {
            return it->second;
        }
        return nullptr;
    }
    
    // Execute test using appropriate handler
    bool executeTest(const JsonTestCase& testCase, 
                    std::string& actualResult, 
                    std::string& failureReason) {
        auto handler = getHandler(testCase.testType);
        if (handler) {
            return handler->execute(testCase, actualResult, failureReason);
        }
        
        // No handler found - this is not a test target yet
        actualResult = "N/A - Not a test target";
        failureReason = "No handler implemented (feature not in Pers or not testable yet)";
        // Return special value to indicate this is not a valid test
        return false;  // Will be handled specially by executor
    }
    
    // Check if handler exists for test type
    bool hasHandler(const std::string& testType) const {
        return _handlers.find(testType) != _handlers.end();
    }
    
private:
    TestHandlerRegistry() = default;
    std::unordered_map<std::string, std::shared_ptr<ITestHandler>> _handlers;
};

// Helper macro for registering handlers
#define REGISTER_TEST_HANDLER(TestType, HandlerClass) \
    static bool _registered_##HandlerClass = []() { \
        TestHandlerRegistry::Instance().registerHandler(TestType, std::make_shared<HandlerClass>()); \
        return true; \
    }();

} // namespace pers::tests::json