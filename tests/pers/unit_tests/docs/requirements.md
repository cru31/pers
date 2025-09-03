# Unit Test Framework Requirements

## 1. Core Terminology

### Definitions
- **TestHandler**: A class that handles one specific TestType (1:1 relationship)
- **TestType**: A unit that tests specific functionality (e.g., "Instance Creation", "Buffer Creation")
- **TestVariation**: Individual test case with different options within a TestType
- **TestOption**: Parameters that can be changed in a variation
- **ExpectedBehavior**: Concrete, verifiable expected results

## 2. JSON Structure Requirements

### Test Definition Format
```json
{
  "metadata": {
    "version": "2.0",
    "totalTests": 500,
    "generatedAt": "2025-01-03"
  },
  "testTypes": [
    {
      "category": "Category Name",
      "testType": "Test Type Name",
      "handlerClass": "HandlerClassName",
      "variations": [
        {
          "id": unique_number,
          "variationName": "Descriptive Name",
          "options": {
            // Test-specific parameters
          },
          "expectedBehavior": {
            // Concrete verifiable results
            "returnValue": "not_null/nullptr/specific_value",
            "properties": {
              // Properties to verify
            },
            "errorCode": "EXPECTED_ERROR_CODE",
            "numericChecks": {
              // Numeric validations like ">=256"
            }
          }
        }
      ]
    }
  ]
}
```

### Expected Behavior Specifications
- **returnValue**: Concrete values ("not_null", "nullptr", specific values)
- **properties**: Measurable attributes (size, memory, flags)
- **errorCode**: Specific error codes when failure is expected
- **numericChecks**: Comparative checks (">=8192", ">0", "==256")

## 3. Handler Class Requirements

### Interface Requirements
- Each TestType must have exactly one Handler class
- Must implement `ITestHandler` interface
- Must implement `execute(const TestVariation& variation)` method
- Must use Pers components (WebGPUBackendFactory, IInstance, IBuffer, etc.)
- **MUST NOT use pers::Logger** (critical requirement)

### Handler Implementation Pattern
```cpp
class [TestType]Handler : public ITestHandler {
public:
    std::string getTestType() const override {
        return "[TestType Name]";
    }
    
    TestResult execute(const TestVariation& variation) override {
        // 1. Extract options from variation
        // 2. Execute test with Pers components
        // 3. Verify against expectedBehavior
        // 4. Return TestResult with pass/fail and details
    }
};
```

## 4. Test Execution Flow

1. Load testTypes from JSON file
2. Find Handler for each TestType
3. Iterate through variations array
4. Pass each variation to Handler's execute()
5. Compare expectedBehavior with actual results
6. Determine status: PASS/FAIL/N/A/NYI

## 5. Test Categories (500 tests total)

### Instance Management
- Instance Creation (validation enabled/disabled, empty names, special chars, unicode)
- Instance Double Creation
- Instance Lifecycle

### Physical Device
- Request Adapter (high performance, low power, fallback)
- Query Features
- Query Limits
- Query Properties

### Logical Device  
- Device Creation
- Queue Creation
- Queue Operations

### Buffer Management
- Buffer Creation (various sizes, usages, mapped at creation)
- Buffer Map Operations (read, write, async)
- Buffer Copy Operations

### Texture Management
- Texture Creation (2D, 3D, various formats)
- Texture View Creation
- Sampler Creation

### Shader Management
- Shader Compilation (valid/invalid WGSL)
- Shader Module Creation
- Entry Point Validation

### Pipeline Management
- Render Pipeline Creation
- Compute Pipeline Creation
- Pipeline Layout

### Render Operations
- Command Encoding
- Render Pass
- Draw Operations

## 6. File Structure

```
tests/pers/unit_tests/
├── docs/
│   ├── requirements.md           # This document
│   └── backup/                   # Old documentation
├── handlers/
│   ├── instance_creation_handler.cpp
│   ├── buffer_creation_handler.cpp
│   ├── request_adapter_handler.cpp
│   └── ... (one handler per TestType)
├── test_handler_base.h           # ITestHandler interface
├── test_variation.h               # TestVariation structures
├── json_test_loader.h/cpp        # JSON parsing
├── test_executor.h/cpp           # Test execution engine
├── test_cases.json               # 500 test cases
└── main.cpp                      # Entry point

```

## 7. Development Order

1. ✅ Define base interfaces (ITestHandler, TestVariation)
2. ✅ Design JSON structure
3. ⏳ Implement Handler classes (per TestType)
4. ⏳ Implement JSON loader
5. ⏳ Implement test execution engine
6. ⏳ Write 500 test cases in JSON
7. ⏳ Update CMakeLists.txt

## 8. Critical Requirements

### Must Have
- Each TestType has exactly one Handler class
- Handlers use Pers components (not raw WebGPU)
- No usage of pers::Logger in test code
- Concrete, verifiable expected behaviors (not abstract descriptions)
- All 500 test cases properly categorized

### Must Not Have
- Abstract expected results like "Valid instance created"
- Raw WebGPU API usage (must use Pers wrappers)
- pers::Logger usage in any test handler
- Multiple handlers for same TestType
- Handlers that handle multiple TestTypes

## 9. Test Status Definitions

- **PASS**: Test executed and matched expected behavior
- **FAIL**: Test executed but didn't match expected behavior  
- **N/A**: Test not applicable (feature not available on platform)
- **NYI**: Not Yet Implemented (handler or feature not implemented)

## 10. Validation Examples

### Good Expected Behavior
```json
{
  "returnValue": "not_null",
  "properties": {
    "bufferSize": 256,
    "usage": 32,
    "mappedPointer": "nullptr"
  }
}
```

### Bad Expected Behavior (Too Abstract)
```json
{
  "expectedResult": "Buffer created successfully"
}
```

## 11. Error Handling

- Handlers must gracefully handle initialization failures
- Return appropriate N/A status when prerequisites fail
- Include detailed failure reasons in TestResult
- Never crash or throw unhandled exceptions