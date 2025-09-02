# Test Runner API Reference

## Classes

### TestRunner

Main class responsible for executing JSON-defined test cases.

```cpp
class TestRunner {
public:
    TestRunner();
    ~TestRunner();
    
    bool loadTestCases(const std::string& jsonPath);
    void executeTests();
    void executeTestSuite(const std::string& suiteName);
    void saveResults(const std::string& outputPath);
    
private:
    std::vector<TestCase> _testCases;
    std::vector<TestResult> _results;
    TestDispatcher _dispatcher;
};
```

#### Methods

##### `loadTestCases(const std::string& jsonPath)`

Loads test cases from a JSON file.

**Parameters:**
- `jsonPath`: Path to the JSON test definition file

**Returns:** `bool` - true if loading succeeded

**Example:**
```cpp
TestRunner runner;
if (!runner.loadTestCases("test_cases.json")) {
    std::cerr << "Failed to load test cases" << std::endl;
    return -1;
}
```

##### `executeTests()`

Executes all enabled test cases in sequence.

**Behavior:**
1. Checks test dependencies
2. Sets up log capture
3. Executes test handler
4. Captures logs and timing
5. Stores results

**Example:**
```cpp
runner.executeTests();
```

##### `executeTestSuite(const std::string& suiteName)`

Executes a specific test suite.

**Parameters:**
- `suiteName`: Name of the suite defined in JSON

**Example:**
```cpp
runner.executeTestSuite("critical_path");
```

### TestCase

Represents a single test case loaded from JSON.

```cpp
struct TestCase {
    std::string id;
    std::string category;
    std::string testType;
    json input;
    std::string expectedResult;
    std::vector<std::string> expectedCallstack;
    int timeoutMs;
    bool enabled;
    std::vector<std::string> dependencies;
};
```

#### Fields

| Field | Type | Description |
|-------|------|-------------|
| `id` | string | Unique test identifier |
| `category` | string | Test category for grouping |
| `testType` | string | Descriptive test name |
| `input` | json | Test input parameters |
| `expectedResult` | string | Expected outcome |
| `expectedCallstack` | vector<string> | Expected function calls |
| `timeoutMs` | int | Test timeout in milliseconds |
| `enabled` | bool | Whether to run this test |
| `dependencies` | vector<string> | Required test IDs |

### TestResult

Stores the results of a test execution.

```cpp
struct TestResult {
    std::string id;
    std::string category;
    std::string testType;
    bool passed;
    std::string actualResult;
    std::string failureReason;
    double executionTimeMs;
    std::vector<std::string> logMessages;
    std::chrono::time_point<> timestamp;
    
    // Derived status
    TestStatus getStatus() const;
    bool hasNotYetImplemented() const;
};
```

#### Methods

##### `getStatus() const`

Determines the test status based on results.

**Returns:** `TestStatus` enum value

**Status Values:**
- `PASSED`: Test succeeded
- `FAILED`: Test failed
- `NOT_APPLICABLE`: Test target not available
- `SKIPPED`: Test was skipped
- `NOT_YET_IMPLEMENTED`: Contains TodoOrDie

### TestDispatcher

Manages test handler registration and execution.

```cpp
class TestDispatcher {
public:
    using Handler = std::function<TestResult(const json&)>;
    
    void registerHandler(const std::string& inputType, Handler handler);
    TestResult dispatch(const TestCase& testCase);
    bool hasHandler(const std::string& inputType) const;
    
private:
    std::map<std::string, Handler> _handlers;
};
```

#### Methods

##### `registerHandler(const std::string& inputType, Handler handler)`

Registers a handler for a specific input type.

**Parameters:**
- `inputType`: The input type string from JSON
- `handler`: Function to handle this input type

**Example:**
```cpp
dispatcher.registerHandler("InstanceDesc", 
    [](const json& input) -> TestResult {
        // Handle InstanceDesc input
        return result;
    });
```

##### `dispatch(const TestCase& testCase)`

Executes the appropriate handler for a test case.

**Parameters:**
- `testCase`: The test case to execute

**Returns:** `TestResult` with execution results

## Test Handlers

### Handler Function Signature

```cpp
TestResult handleTestType(const json& input);
```

### Built-in Handlers

#### InstanceDesc Handler

```cpp
TestResult handleInstanceDesc(const json& input) {
    TestResult result;
    
    // Parse input
    InstanceDesc desc;
    desc.enableValidation = input.value("enableValidation", false);
    desc.applicationName = input.value("applicationName", "");
    desc.engineName = input.value("engineName", "");
    
    // Execute test
    auto factory = std::make_shared<WebGPUBackendFactory>();
    auto instance = factory->createInstance(desc);
    
    // Evaluate result
    if (instance) {
        result.actualResult = "Valid instance created";
        result.passed = true;
    } else {
        result.actualResult = "Failed to create instance";
        result.passed = false;
    }
    
    return result;
}
```

#### BufferDesc Handler

```cpp
TestResult handleBufferDesc(const json& input) {
    TestResult result;
    
    // Parse input
    size_t size = input.value("size", 0);
    std::string usage = input.value("usage", "");
    std::string debugName = input.value("debugName", "");
    
    // Create device and resource factory
    auto device = createTestDevice();
    auto factory = device->getResourceFactory();
    
    // Create buffer
    BufferDesc desc;
    desc.size = size;
    desc.usage = parseBufferUsage(usage);
    desc.debugName = debugName;
    
    auto buffer = factory->createBuffer(desc);
    
    // Evaluate
    if (size == 0) {
        result.actualResult = buffer ? "Unexpected buffer" : "Returns nullptr";
        result.passed = (buffer == nullptr);
    } else {
        result.actualResult = buffer ? "Valid buffer" : "Failed to create";
        result.passed = (buffer != nullptr);
    }
    
    return result;
}
```

## Log Capture Integration

### LogCapture Class

```cpp
class LogCapture {
public:
    LogCapture();
    ~LogCapture();
    
    void startCapture();
    void stopCapture();
    std::vector<std::string> getCapturedLogs() const;
    
private:
    std::vector<std::string> _logs;
    std::mutex _mutex;
    Logger::CallbackHandle _handle;
    bool _capturing;
};
```

### Usage in Test Execution

```cpp
void TestRunner::executeTest(const TestCase& test) {
    // Setup log capture
    LogCapture capture;
    capture.startCapture();
    
    // Execute test
    TestResult result;
    try {
        result = _dispatcher.dispatch(test);
    } catch (const std::exception& e) {
        result.passed = false;
        result.actualResult = "Exception thrown";
        result.failureReason = e.what();
    }
    
    // Capture logs
    capture.stopCapture();
    result.logMessages = capture.getCapturedLogs();
    
    // Store result
    _results.push_back(result);
}
```

## Utility Functions

### JSON Parsing Helpers

```cpp
namespace TestUtils {
    // Parse buffer usage from string
    BufferUsage parseBufferUsage(const std::string& usage) {
        static std::map<std::string, BufferUsage> usageMap = {
            {"Vertex", BufferUsage::Vertex},
            {"Index", BufferUsage::Index},
            {"Uniform", BufferUsage::Uniform},
            {"Storage", BufferUsage::Storage}
        };
        
        auto it = usageMap.find(usage);
        return it != usageMap.end() ? it->second : BufferUsage::None;
    }
    
    // Parse power preference
    PowerPreference parsePowerPreference(const std::string& pref) {
        if (pref == "HighPerformance") return PowerPreference::HighPerformance;
        if (pref == "LowPower") return PowerPreference::LowPower;
        return PowerPreference::Default;
    }
    
    // Format test input for display
    std::string formatInput(const json& input) {
        std::stringstream ss;
        
        if (input.contains("type")) {
            ss << "type=" << input["type"];
        }
        
        if (input.contains("values")) {
            for (auto& [key, value] : input["values"].items()) {
                ss << ", " << key << "=" << value;
            }
        }
        
        return ss.str();
    }
}
```

### Test Helpers

```cpp
namespace TestHelpers {
    // Create a test device
    std::shared_ptr<ILogicalDevice> createTestDevice() {
        auto factory = std::make_shared<WebGPUBackendFactory>();
        auto instance = factory->createInstance({});
        auto physicalDevice = instance->requestPhysicalDevice({});
        return physicalDevice->createLogicalDevice({});
    }
    
    // Check if result contains TodoOrDie
    bool hasNotYetImplemented(const std::vector<std::string>& logs) {
        return std::any_of(logs.begin(), logs.end(),
            [](const auto& log) {
                return log.find("[TODO_OR_DIE]") != std::string::npos;
            });
    }
    
    // Wait for async operation with timeout
    template<typename Func>
    bool waitWithTimeout(Func func, int timeoutMs) {
        auto future = std::async(std::launch::async, func);
        auto status = future.wait_for(
            std::chrono::milliseconds(timeoutMs)
        );
        return status == std::future_status::ready;
    }
}
```

## Error Handling

### Exception Types

```cpp
class TestException : public std::runtime_error {
public:
    TestException(const std::string& message) 
        : std::runtime_error(message) {}
};

class TestTimeoutException : public TestException {
public:
    TestTimeoutException(const std::string& testId, int timeoutMs)
        : TestException("Test " + testId + " timed out after " + 
                       std::to_string(timeoutMs) + "ms") {}
};

class DependencyException : public TestException {
public:
    DependencyException(const std::string& testId, 
                       const std::string& dependencyId)
        : TestException("Test " + testId + " dependency " + 
                       dependencyId + " not met") {}
};
```

### Error Recovery

```cpp
TestResult handleTestError(const TestCase& test, 
                          const std::exception& e) {
    TestResult result;
    result.id = test.id;
    result.passed = false;
    
    if (dynamic_cast<const TodoOrDie::Exception*>(&e)) {
        result.actualResult = "Not yet implemented";
        result.failureReason = e.what();
    } else if (dynamic_cast<const TestTimeoutException*>(&e)) {
        result.actualResult = "Test timed out";
        result.failureReason = e.what();
    } else {
        result.actualResult = "Exception thrown";
        result.failureReason = e.what();
    }
    
    return result;
}
```

## Usage Examples

### Basic Test Execution

```cpp
int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <test_cases.json>" << std::endl;
        return 1;
    }
    
    TestRunner runner;
    
    // Load test cases
    if (!runner.loadTestCases(argv[1])) {
        std::cerr << "Failed to load test cases" << std::endl;
        return 1;
    }
    
    // Execute all tests
    runner.executeTests();
    
    // Save results
    runner.saveResults("test_results.json");
    
    return 0;
}
```

### Custom Handler Registration

```cpp
// Register custom handler
runner.registerHandler("CustomInput", 
    [](const json& input) -> TestResult {
        TestResult result;
        
        // Custom test logic
        auto value = input.value("customValue", 0);
        if (value > 0) {
            result.passed = true;
            result.actualResult = "Custom test passed";
        } else {
            result.passed = false;
            result.actualResult = "Custom test failed";
        }
        
        return result;
    });
```

### Web Viewer Integration

```cpp
// Execute with web viewer
if (hasFlag(argc, argv, "--jsviewer")) {
    runner.executeTests();
    runner.saveResults("test_results.json");
    
    // Launch web viewer
    std::string sessionId = std::to_string(
        std::chrono::system_clock::now().time_since_epoch().count()
    );
    launchWebViewer(sessionId, "test_results.json");
}
```