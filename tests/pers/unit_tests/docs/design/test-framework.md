# Test Framework Design

## Overview

The Pers unit test framework is a JSON-driven testing system designed to validate the WebGPU graphics backend implementation. It provides a declarative approach to defining test cases with comprehensive logging and result tracking.

## Test Case Format

### JSON Schema

```json
{
  "metadata": {
    "version": "1.0.0",
    "date": "2025-01-02",
    "total_tests": 100,
    "categories": ["Critical Path", "Memory Safety", ...]
  },
  "test_cases": [
    {
      "id": "001",
      "category": "Critical Path",
      "test_type": "WebGPU Instance Creation",
      "input": {
        "type": "InstanceDesc",
        "values": {
          "enableValidation": true,
          "applicationName": "Unit Test"
        }
      },
      "expected_result": "Valid instance created",
      "expected_callstack": [...],
      "timeout_ms": 1000,
      "enabled": true,
      "dependencies": []
    }
  ],
  "test_suites": {
    "critical_path": ["001", "002", ...],
    "quick_test": ["001", "003", ...]
  }
}
```

### Field Definitions

| Field | Type | Description |
|-------|------|-------------|
| `id` | string | Unique test identifier (e.g., "001") |
| `category` | string | Test category for grouping |
| `test_type` | string | Descriptive name of the test |
| `input` | object | Test input parameters |
| `input.type` | string | Input type identifier |
| `input.values` | object | Key-value pairs of input data |
| `expected_result` | string | Expected test outcome |
| `expected_callstack` | array | Expected function call sequence |
| `timeout_ms` | number | Test timeout in milliseconds |
| `enabled` | boolean | Whether test should run |
| `dependencies` | array | Test IDs that must pass first |

## Test Categories

### 1. Critical Path
Tests core functionality required for basic operation:
- Instance creation
- Adapter enumeration
- Device creation
- Queue operations
- Command encoding

### 2. Memory Safety
Tests memory management and safety:
- Buffer double delete prevention
- Null pointer handling
- Buffer overflow protection
- Unmapped buffer access
- Reference counting

### 3. Type Conversion
Tests enum and flag conversions:
- ColorWriteMask mappings
- BufferUsage flags
- TextureFormat conversions
- Blend modes
- Compare functions

### 4. Resource Management
Tests resource creation and lifecycle:
- Buffer creation (various sizes)
- Shader module compilation
- Pipeline creation
- Texture management
- Sampler creation

### 5. Error Handling
Tests error conditions and recovery:
- Null device operations
- Invalid surface handling
- Empty queue submission
- RenderPass state violations
- Resource limit exceeded

## Test Execution Model

### State Machine

```
┌─────────┐
│ PENDING │
└────┬────┘
     │ Dependencies satisfied
     ▼
┌─────────┐
│ RUNNING │──────┐
└────┬────┘      │ Timeout
     │           ▼
     │      ┌─────────┐
     │      │ TIMEOUT │
     │      └─────────┘
     │ Complete
     ▼
┌─────────┐
│COMPLETE │
└────┬────┘
     │ Evaluate
     ▼
┌─────────────────────────┐
│ PASS / FAIL / N/A / NYI │
└─────────────────────────┘
```

### Execution Flow

```cpp
class TestRunner {
    void executeTest(const TestCase& test) {
        // 1. Check dependencies
        if (!checkDependencies(test)) {
            skipTest(test, "Dependencies not met");
            return;
        }
        
        // 2. Setup log capture
        LogCapture capture;
        capture.start();
        
        // 3. Execute based on input type
        TestResult result;
        try {
            result = dispatchTest(test);
        } catch (...) {
            result = handleException();
        }
        
        // 4. Capture logs
        result.logMessages = capture.stop();
        
        // 5. Evaluate result
        evaluateResult(test, result);
        
        // 6. Store result
        _results.push_back(result);
    }
};
```

## Test Status Model

### Primary Status
Every test has exactly one primary status:
- **PASS**: Test executed successfully and met expectations
- **FAIL**: Test executed but did not meet expectations
- **N/A**: Test target not available or not applicable
- **SKIP**: Test was skipped (disabled or dependencies failed)

### Secondary Status
Tests can additionally have:
- **NYI (Not Yet Implemented)**: Contains TodoOrDie calls indicating incomplete implementation

### Status Determination Logic

```cpp
TestStatus determineStatus(const TestCase& test, const TestResult& result) {
    // Check for N/A first (highest priority)
    if (result.actualResult.contains("N/A")) {
        return TestStatus::NotApplicable;
    }
    
    // Check for skip conditions
    if (!test.enabled || !dependenciesMet) {
        return TestStatus::Skipped;
    }
    
    // Check pass/fail
    if (result.actualResult == test.expectedResult) {
        return TestStatus::Passed;
    } else {
        return TestStatus::Failed;
    }
}

bool hasNotYetImplemented(const TestResult& result) {
    return std::any_of(result.logMessages.begin(), 
                       result.logMessages.end(),
                       [](const auto& log) {
                           return log.contains("[TODO_OR_DIE]");
                       });
}
```

## Test Input Types

### 1. InstanceDesc
```cpp
struct InstanceDescInput {
    bool enableValidation;
    std::string applicationName;
    std::string engineName;
};
```

### 2. PhysicalDeviceOptions
```cpp
struct PhysicalDeviceOptionsInput {
    PowerPreference powerPreference;
    Surface* compatibleSurface;
};
```

### 3. BufferDesc
```cpp
struct BufferDescInput {
    size_t size;
    BufferUsage usage;
    bool mappedAtCreation;
    std::string debugName;
};
```

### 4. ShaderModuleDesc
```cpp
struct ShaderModuleDescInput {
    std::string code;  // WGSL source
    std::string debugName;
    std::string entryPoint;
};
```

## Test Handlers

### Handler Registration

```cpp
class TestDispatcher {
    using Handler = std::function<TestResult(const json&)>;
    std::map<std::string, Handler> _handlers;
    
    void registerHandlers() {
        _handlers["InstanceDesc"] = handleInstanceDesc;
        _handlers["BufferDesc"] = handleBufferDesc;
        _handlers["ShaderModuleDesc"] = handleShaderModule;
        // ...
    }
};
```

### Handler Implementation

```cpp
TestResult handleInstanceDesc(const json& input) {
    // Parse input
    InstanceDesc desc;
    desc.enableValidation = input["enableValidation"];
    desc.applicationName = input["applicationName"];
    
    // Execute test
    auto factory = std::make_shared<WebGPUBackendFactory>();
    auto instance = factory->createInstance(desc);
    
    // Evaluate result
    TestResult result;
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

## Dependency Management

### Dependency Resolution

```cpp
class DependencyResolver {
    bool resolve(const TestCase& test, 
                 const std::vector<TestResult>& results) {
        for (const auto& depId : test.dependencies) {
            auto it = std::find_if(results.begin(), results.end(),
                [&](const auto& r) { return r.id == depId; });
                
            if (it == results.end() || !it->passed) {
                return false;  // Dependency not met
            }
        }
        return true;
    }
};
```

### Dependency Graph

```
001 (Instance) ──┐
                 ├──> 003 (Device) ──┬──> 004 (Queue)
002 (Adapter) ───┘                   └──> 005 (CommandEncoder)
```

## Result Aggregation

### Summary Generation

```cpp
struct Summary {
    int totalTests = 0;
    int passed = 0;
    int failed = 0;
    int notApplicable = 0;
    int notYetImplemented = 0;
    int skipped = 0;
    double totalTimeMs = 0.0;
    double passRate = 0.0;
};

Summary calculateSummary(const std::vector<TestResult>& results) {
    Summary summary;
    for (const auto& result : results) {
        summary.totalTests++;
        
        // Count NYI independently
        if (hasNotYetImplemented(result)) {
            summary.notYetImplemented++;
        }
        
        // Count primary status
        switch (result.status) {
            case TestStatus::Passed:
                summary.passed++;
                break;
            case TestStatus::Failed:
                summary.failed++;
                break;
            case TestStatus::NotApplicable:
                summary.notApplicable++;
                break;
            case TestStatus::Skipped:
                summary.skipped++;
                break;
        }
        
        summary.totalTimeMs += result.executionTimeMs;
    }
    
    if (summary.totalTests > 0) {
        summary.passRate = (summary.passed * 100.0) / summary.totalTests;
    }
    
    return summary;
}
```

## Test Suite Management

### Suite Definition

```json
"test_suites": {
    "critical_path": ["001", "002", "003", "004", "005"],
    "memory_safety": ["006", "007", "008", "009", "010"],
    "quick_test": ["001", "003", "011", "013", "023"],
    "full_test": "all"
}
```

### Suite Execution

```cpp
void runTestSuite(const std::string& suiteName) {
    auto suite = _testSuites[suiteName];
    
    if (suite == "all") {
        runAllTests();
    } else {
        for (const auto& testId : suite) {
            runTest(testId);
        }
    }
}
```

## Performance Metrics

### Timing Measurement

```cpp
class TestTimer {
    using Clock = std::chrono::high_resolution_clock;
    
    double measureExecution(std::function<void()> test) {
        auto start = Clock::now();
        test();
        auto end = Clock::now();
        
        auto duration = std::chrono::duration_cast<
            std::chrono::microseconds>(end - start);
        return duration.count() / 1000.0;  // Convert to ms
    }
};
```

### Performance Tracking

- Per-test execution time
- Total suite execution time
- Setup/teardown overhead
- Log capture overhead

## Best Practices

### Test Design

1. **Isolation**: Each test should be independent
2. **Determinism**: Tests should produce consistent results
3. **Clarity**: Test names should clearly describe what's being tested
4. **Coverage**: Test both success and failure paths
5. **Performance**: Keep tests fast (< 1 second each)

### Test Organization

1. **Categorization**: Group related tests together
2. **Dependencies**: Minimize inter-test dependencies
3. **Suites**: Create focused test suites for different scenarios
4. **Documentation**: Document expected behavior and edge cases

### Error Handling

1. **Graceful Failure**: Catch and report all exceptions
2. **Detailed Logging**: Capture sufficient context for debugging
3. **Timeout Protection**: Prevent hanging tests
4. **State Cleanup**: Ensure proper cleanup even on failure