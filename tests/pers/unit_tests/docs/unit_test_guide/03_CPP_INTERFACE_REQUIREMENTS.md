# C++ Interface Requirements

## Overview
This document defines the required C++ interfaces and structures for the Pers Graphics Engine unit testing framework. All components must adhere to these interfaces to ensure compatibility with the test runner and result processing systems.

## Core Interfaces

### ITestHandler Interface
```cpp
#pragma once
#include <memory>
#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class ITestHandler {
public:
    virtual ~ITestHandler() = default;
    
    // Initialize the handler with test configuration
    virtual bool initialize(const TestConfig& config) = 0;
    
    // Execute test with given parameters
    virtual TestResult execute(const json& options) = 0;
    
    // Clean up resources after test execution
    virtual void cleanup() = 0;
    
    // Get unique handler name for registration
    virtual std::string getName() const = 0;
    
    // Optional: Get supported parameter schema
    virtual json getParameterSchema() const { return json::object(); }
    
    // Optional: Validate parameters before execution
    virtual bool validateParameters(const json& options) const { return true; }
};
```

### TestResult Structure
```cpp
#pragma once
#include <string>
#include <map>
#include <any>
#include <chrono>

struct TestResult {
    // Core result fields
    bool success = true;
    std::string returnValue;
    std::string errorMessage;
    int errorCode = 0;
    
    // Properties for validation
    std::map<std::string, std::any> properties;
    
    // Performance metrics
    double executionTimeMs = 0.0;
    size_t memoryUsedBytes = 0;
    
    // Optional detailed results
    std::vector<std::string> logs;
    std::map<std::string, double> performanceMetrics;
    
    // Helper methods
    void setProperty(const std::string& key, const std::any& value) {
        properties[key] = value;
    }
    
    template<typename T>
    std::optional<T> getProperty(const std::string& key) const {
        auto it = properties.find(key);
        if (it != properties.end()) {
            try {
                return std::any_cast<T>(it->second);
            } catch (...) {
                return std::nullopt;
            }
        }
        return std::nullopt;
    }
    
    json toJson() const;
    static TestResult fromJson(const json& j);
};
```

### TestConfig Structure
```cpp
#pragma once
#include <string>
#include <filesystem>

struct TestConfig {
    // Test environment configuration
    std::filesystem::path workingDirectory;
    std::filesystem::path resourceDirectory;
    std::filesystem::path outputDirectory;
    
    // Test execution settings
    bool verbose = false;
    bool captureOutput = true;
    bool measurePerformance = true;
    int timeoutMs = 30000; // 30 seconds default
    
    // Platform/device settings
    std::string preferredAdapter;
    bool useDebugLayers = false;
    bool useValidationLayers = false;
    
    // Resource limits
    size_t maxMemoryMB = 1024;
    int maxThreads = 4;
};
```

## Test Runner Interfaces

### ITestRunner Interface
```cpp
class ITestRunner {
public:
    virtual ~ITestRunner() = default;
    
    // Load test cases from JSON
    virtual bool loadTestCases(const std::filesystem::path& jsonPath) = 0;
    
    // Run all loaded tests
    virtual void runAllTests() = 0;
    
    // Run specific test by ID
    virtual TestResult runTest(int testId) = 0;
    
    // Get test results
    virtual std::vector<TestResult> getResults() const = 0;
    
    // Export results to JSON
    virtual void exportResults(const std::filesystem::path& outputPath) = 0;
    
    // Set test configuration
    virtual void setConfig(const TestConfig& config) = 0;
};
```

### ITestValidator Interface
```cpp
class ITestValidator {
public:
    virtual ~ITestValidator() = default;
    
    // Validate test result against expected behavior
    virtual bool validate(const TestResult& result, 
                         const json& expectedBehavior) = 0;
    
    // Get validation details
    virtual std::string getValidationMessage() const = 0;
    
    // Compare values with operators
    virtual bool compareValue(const std::any& actual, 
                              const std::string& expected) = 0;
};
```

## Handler Registry System

### HandlerRegistry Class
```cpp
class HandlerRegistry {
public:
    using HandlerFactory = std::function<std::unique_ptr<ITestHandler>()>;
    
    // Register handler factory
    static void registerHandler(const std::string& name, HandlerFactory factory);
    
    // Create handler instance
    static std::unique_ptr<ITestHandler> createHandler(const std::string& name);
    
    // Check if handler exists
    static bool hasHandler(const std::string& name);
    
    // Get all registered handler names
    static std::vector<std::string> getHandlerNames();
    
private:
    static std::map<std::string, HandlerFactory> _factories;
};

// Automatic registration macro
#define REGISTER_TEST_HANDLER(HandlerClass) \
    namespace { \
        struct HandlerClass##Registrar { \
            HandlerClass##Registrar() { \
                HandlerRegistry::registerHandler( \
                    #HandlerClass, \
                    []() { return std::make_unique<HandlerClass>(); } \
                ); \
            } \
        }; \
        static HandlerClass##Registrar HandlerClass##_registrar; \
    }
```

## Result Processing Interfaces

### IResultProcessor Interface
```cpp
class IResultProcessor {
public:
    virtual ~IResultProcessor() = default;
    
    // Process single test result
    virtual void processResult(const TestResult& result, 
                               const json& testCase) = 0;
    
    // Generate summary report
    virtual json generateSummary() const = 0;
    
    // Export results in specific format
    virtual void exportResults(const std::filesystem::path& path, 
                               const std::string& format) = 0;
};
```

### IResultFormatter Interface
```cpp
class IResultFormatter {
public:
    virtual ~IResultFormatter() = default;
    
    // Format result for display
    virtual std::string format(const TestResult& result) const = 0;
    
    // Format as JSON
    virtual json toJson(const TestResult& result) const = 0;
    
    // Format as XML (JUnit compatible)
    virtual std::string toXml(const TestResult& result) const = 0;
    
    // Format as Markdown
    virtual std::string toMarkdown(const TestResult& result) const = 0;
};
```

## Logging Interface

### ITestLogger Interface
```cpp
enum class LogLevel {
    TRACE,
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    CRITICAL
};

class ITestLogger {
public:
    virtual ~ITestLogger() = default;
    
    // Log message with level
    virtual void log(LogLevel level, const std::string& message) = 0;
    
    // Log with test context
    virtual void logTest(int testId, const std::string& message) = 0;
    
    // Log performance metrics
    virtual void logPerformance(const std::string& operation, 
                                double timeMs) = 0;
    
    // Flush logs
    virtual void flush() = 0;
};
```

## Exception Types

```cpp
// Base exception for test framework
class TestException : public std::exception {
protected:
    std::string _message;
    int _errorCode;
    
public:
    TestException(const std::string& message, int errorCode = -1)
        : _message(message), _errorCode(errorCode) {}
    
    const char* what() const noexcept override {
        return _message.c_str();
    }
    
    int getErrorCode() const { return _errorCode; }
};

// Specific exception types
class TestInitializationException : public TestException {
public:
    using TestException::TestException;
};

class TestExecutionException : public TestException {
public:
    using TestException::TestException;
};

class TestValidationException : public TestException {
public:
    using TestException::TestException;
};

class TestTimeoutException : public TestException {
public:
    TestTimeoutException(int timeoutMs)
        : TestException("Test execution timeout: " + 
                       std::to_string(timeoutMs) + "ms", -2) {}
};
```

## Utility Interfaces

### IResourceManager Interface
```cpp
class IResourceManager {
public:
    virtual ~IResourceManager() = default;
    
    // Load resource file
    virtual std::vector<uint8_t> loadResource(const std::string& name) = 0;
    
    // Get resource path
    virtual std::filesystem::path getResourcePath(const std::string& name) = 0;
    
    // Check if resource exists
    virtual bool hasResource(const std::string& name) = 0;
    
    // Clean up temporary resources
    virtual void cleanup() = 0;
};
```

### IPerformanceMonitor Interface
```cpp
struct PerformanceMetrics {
    double cpuTimeMs;
    double gpuTimeMs;
    size_t memoryUsedBytes;
    size_t memoryPeakBytes;
    int drawCalls;
    int triangleCount;
};

class IPerformanceMonitor {
public:
    virtual ~IPerformanceMonitor() = default;
    
    // Start monitoring
    virtual void beginMonitoring() = 0;
    
    // End monitoring and get metrics
    virtual PerformanceMetrics endMonitoring() = 0;
    
    // Get current metrics without stopping
    virtual PerformanceMetrics getCurrentMetrics() const = 0;
    
    // Reset metrics
    virtual void reset() = 0;
};
```

## Implementation Requirements

### Thread Safety
- All handlers must be thread-safe if parallel execution is enabled
- Use appropriate synchronization for shared resources
- Logger implementations must handle concurrent access

### Resource Management
- Follow RAII principles
- Clean up all resources in destructors
- Handle exceptions properly to prevent resource leaks

### Error Handling
- Never throw exceptions from destructors
- Catch and convert all exceptions to TestResult errors
- Provide meaningful error messages

### Performance
- Minimize allocations in hot paths
- Use move semantics where appropriate
- Consider caching for repeated operations

## Compliance Checklist

When implementing test components, ensure:

- [ ] All virtual functions are implemented
- [ ] Proper error handling is in place
- [ ] Resources are managed correctly
- [ ] Thread safety is maintained
- [ ] Performance metrics are collected
- [ ] Results are properly formatted
- [ ] Logging is comprehensive
- [ ] Parameters are validated
- [ ] Timeouts are respected
- [ ] Memory limits are enforced

## Example Implementation Pattern

```cpp
class MyTestHandler : public ITestHandler {
private:
    TestConfig _config;
    std::unique_ptr<ITestLogger> _logger;
    std::unique_ptr<IPerformanceMonitor> _perfMonitor;
    
public:
    bool initialize(const TestConfig& config) override {
        _config = config;
        _logger = std::make_unique<ConsoleLogger>();
        _perfMonitor = std::make_unique<PerformanceMonitor>();
        
        _logger->log(LogLevel::INFO, "Handler initialized: " + getName());
        return true;
    }
    
    TestResult execute(const json& options) override {
        TestResult result;
        
        try {
            // Validate parameters
            if (!validateParameters(options)) {
                throw TestValidationException("Invalid parameters");
            }
            
            // Start performance monitoring
            _perfMonitor->beginMonitoring();
            
            // Execute test logic
            performTest(options, result);
            
            // Collect metrics
            auto metrics = _perfMonitor->endMonitoring();
            result.executionTimeMs = metrics.cpuTimeMs;
            result.memoryUsedBytes = metrics.memoryUsedBytes;
            
        } catch (const TestException& e) {
            result.success = false;
            result.errorMessage = e.what();
            result.errorCode = e.getErrorCode();
            _logger->log(LogLevel::ERROR, e.what());
        } catch (const std::exception& e) {
            result.success = false;
            result.errorMessage = e.what();
            result.errorCode = -1;
            _logger->log(LogLevel::CRITICAL, e.what());
        }
        
        return result;
    }
    
    void cleanup() override {
        _perfMonitor.reset();
        _logger->flush();
    }
    
    std::string getName() const override {
        return "MyTestHandler";
    }
    
private:
    void performTest(const json& options, TestResult& result) {
        // Actual test implementation
    }
};

REGISTER_TEST_HANDLER(MyTestHandler)
```