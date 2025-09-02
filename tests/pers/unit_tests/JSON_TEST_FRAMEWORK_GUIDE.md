# JSON Test Framework Developer Guide

## Table of Contents
1. [Overview](#overview)
2. [Architecture](#architecture)
3. [Test Case Structure](#test-case-structure)
4. [Option-Based Testing](#option-based-testing)
5. [Test Case Generation](#test-case-generation)
6. [Writing New Tests](#writing-new-tests)
7. [Running Tests](#running-tests)
8. [Extending the Framework](#extending-the-framework)
9. [Troubleshooting](#troubleshooting)
10. [Best Practices](#best-practices)

## Overview

The JSON Test Framework is a comprehensive testing system for the Pers Graphics Engine that uses JSON files to define test cases. It supports both traditional hardcoded tests and dynamic option-based tests with automatic test case generation.

### Key Features
- **JSON-based test definitions**: Tests are defined in JSON files, making them easy to read, write, and version control
- **Option-based testing**: Automatically generate test variations based on option combinations
- **Timeout protection**: Each test runs with a configurable timeout to prevent hanging
- **Parallel execution support**: Tests run in separate threads with proper synchronization
- **Detailed reporting**: Multiple output formats including console, JSON, and Markdown
- **TodoOrDie interception**: Captures and handles TODO markers without aborting tests

## Architecture

### Core Components

```
┌─────────────────────────────────────────────┐
│              JSON Test Files                 │
│  (test_cases.json, all_generated_tests.json) │
└─────────────────┬───────────────────────────┘
                  │
                  ▼
┌─────────────────────────────────────────────┐
│           JsonTestLoader                     │
│  - Parses JSON files                         │
│  - Manages test cases                        │
│  - Executes individual tests                 │
└─────────────────┬───────────────────────────┘
                  │
                  ▼
┌─────────────────────────────────────────────┐
│          JsonTestExecutor                    │
│  - Manages test execution flow               │
│  - Handles timeouts and threading            │
│  - Generates reports                         │
└─────────────────┬───────────────────────────┘
                  │
                  ▼
┌─────────────────────────────────────────────┐
│         TestResultWriter                     │
│  - Outputs results in multiple formats       │
│  - Creates structured reports                │
└─────────────────────────────────────────────┘
```

### File Structure

```
tests/pers/unit_tests/
├── json_test_loader_rapidjson.h     # Main test framework header
├── json_test_loader_rapidjson.cpp   # Test loader and executor implementation
├── json_test_main.cpp               # Entry point
├── test_result_writer.h/cpp         # Result formatting and output
├── generate_test_cases.py           # Python script for test generation
└── test_case_data/                  # Test data directory
    └── 20250102/                     # Date-organized test cases
        ├── test_option_schema.json   # Option definitions
        ├── all_generated_tests.json  # Generated test cases
        └── {category}/               # Category-specific tests
            └── option_set_*.json     # Batched test cases
```

## Test Case Structure

### Basic Test Case Format

```json
{
  "id": "001",
  "category": "Critical Path",
  "test_type": "WebGPU Instance Creation",
  "input": {
    "type": "InstanceDesc",
    "values": {
      "enableValidation": true,
      "applicationName": "Unit Test",
      "engineName": "Pers Graphics Engine"
    }
  },
  "expected_result": "Valid instance created",
  "expected_callstack": [
    "WebGPUBackendFactory::createInstance()",
    "WebGPUInstance::WebGPUInstance()"
  ],
  "timeout_ms": 1000,
  "enabled": true,
  "dependencies": [],
  "reason": "Optional disable reason"
}
```

### Option-Based Test Case Format

```json
{
  "id": "0001",
  "category": "instance_creation",
  "test_type": "WebGPU Instance Creation",
  "option_set_id": "a1b2c3d4",
  "input": {
    "type": "OptionBased",
    "options": {
      "validation": true,
      "backend_type": "Vulkan",
      "power_preference": "HighPerformance",
      "debug_mode": false,
      "application_name": "Test App"
    }
  },
  "expected_result": "Success with options",
  "timeout_ms": 1000,
  "enabled": true
}
```

### Field Descriptions

| Field | Type | Description | Required |
|-------|------|-------------|----------|
| `id` | string | Unique test identifier | Yes |
| `category` | string | Test category for grouping | Yes |
| `test_type` | string | Type of test to execute | Yes |
| `input` | object | Input parameters for the test | Yes |
| `input.type` | string | "OptionBased" for dynamic tests | No |
| `input.values` | object | Key-value pairs for traditional tests | No |
| `input.options` | object | Options for option-based tests | No |
| `expected_result` | string | Expected outcome | Yes |
| `expected_callstack` | array | Expected function call sequence | No |
| `timeout_ms` | integer | Test timeout in milliseconds | No (default: 1000) |
| `enabled` | boolean | Whether test is enabled | No (default: true) |
| `dependencies` | array | IDs of tests that must pass first | No |
| `reason` | string | Reason for disabling test | No |

## Option-Based Testing

### Option Schema Definition

The option schema (`test_option_schema.json`) defines the available options and their possible values:

```json
{
  "test_categories": {
    "instance_creation": {
      "base_type": "WebGPU Instance Creation",
      "option_dimensions": {
        "validation": {
          "type": "boolean",
          "values": [true, false],
          "description": "Enable/disable validation layers"
        },
        "backend_type": {
          "type": "enum",
          "values": ["Vulkan", "D3D12", "Metal", "Primary"],
          "description": "WebGPU backend selection"
        }
      }
    }
  }
}
```

### Combination Strategies

The framework supports multiple strategies for generating option combinations:

1. **Exhaustive**: Test all possible combinations
2. **Critical Path**: Test most important combinations only
3. **Boundary**: Test boundary values and edge cases
4. **Pairwise**: Ensure each pair of options is tested at least once
5. **Random Sample**: Random sampling of combinations

## Test Case Generation

### Using the Generation Script

```bash
# Navigate to the test directory
cd tests/pers/unit_tests/

# Run the generation script
python generate_test_cases.py

# Output:
# Generating test cases for instance_creation using critical_path strategy...
# Generated 8 option combinations
# Total test cases generated: 40
```

### Customizing Generation

Edit `generate_test_cases.py` to modify generation strategies:

```python
category_strategies = {
    "instance_creation": "critical_path",  # Change strategy here
    "adapter_request": "boundary",
    "device_creation": "exhaustive",       # Use all combinations
    "buffer_creation": "pairwise"
}
```

### Adding New Option Dimensions

1. Edit `test_option_schema.json`:
```json
"new_option": {
  "type": "enum",
  "values": ["Option1", "Option2", "Option3"],
  "description": "Description of the option"
}
```

2. Regenerate test cases:
```bash
python generate_test_cases.py
```

## Writing New Tests

### Step 1: Define the Test Type

Add the test type to `test_option_schema.json`:

```json
"my_new_test": {
  "base_type": "My New Test Type",
  "option_dimensions": {
    "param1": {
      "type": "integer",
      "values": [10, 100, 1000],
      "description": "Test parameter 1"
    }
  }
}
```

### Step 2: Implement Test Execution

Add to `json_test_loader_rapidjson.cpp`:

```cpp
else if (baseType == "My New Test Type") {
    // Parse options
    const auto& options = testCase.inputValues;
    int param1 = 10;  // default
    if (options.find("param1") != options.end()) {
        param1 = std::stoi(options.at("param1"));
    }
    
    // Execute test
    auto result = myTestFunction(param1);
    
    // Check result
    if (result.success) {
        actualResult = "Success with options";
        return true;
    } else {
        actualResult = "Failed";
        failureReason = result.errorMessage;
        return false;
    }
}
```

### Step 3: Generate Test Cases

```bash
python generate_test_cases.py
```

## Running Tests

### Command Line Usage

```bash
# Basic usage
./pers_json_tests.exe [timeout_ms] [test_file.json]

# Examples:
./pers_json_tests.exe 100 test_cases.json           # 100ms timeout
./pers_json_tests.exe 500 all_generated_tests.json  # 500ms timeout
./pers_json_tests.exe 1000 my_custom_tests.json     # 1 second timeout
```

### Output Formats

The framework generates multiple output formats:

1. **Console Output**: Color-coded real-time results
```
[001] PASS WebGPU Instance Creation (265.07ms) with options: validation=true, backend_type=Vulkan
[002] FAIL Buffer Creation (0.25ms) with options: size=0
      Expected: Returns nullptr
      Actual: Buffer created with 0 size
      Reason: Should have returned nullptr for 0 size
```

2. **JSON Results** (`test_results.json`): Structured data for automation
3. **Markdown Report** (`json_test_results.md`): Human-readable report

### Filtering Tests

Currently, tests can be filtered by:
- Category (modify JSON file)
- Enable/disable flag in JSON
- Dependencies

Future enhancements could include command-line filters.

## Extending the Framework

### Adding New Test Categories

1. **Define Category in Schema**:
```json
"new_category": {
  "base_type": "New Category Base",
  "option_dimensions": { ... }
}
```

2. **Implement Execution Logic**:
```cpp
if (baseType == "New Category Base") {
    // Implementation
}
```

3. **Update Generation Script**:
```python
category_strategies["new_category"] = "boundary"
```

### Adding New Output Formats

Extend `TestResultWriter` class:

```cpp
void TestResultWriter::writeCustomFormat(const std::string& path) {
    // Custom format implementation
}
```

### Integrating with CI/CD

The framework is designed for CI/CD integration:

```yaml
# Example GitHub Actions workflow
- name: Run Tests
  run: |
    ./pers_json_tests.exe 1000 all_generated_tests.json
    
- name: Upload Results
  uses: actions/upload-artifact@v2
  with:
    name: test-results
    path: |
      test_results.json
      json_test_results.md
```

## Troubleshooting

### Common Issues and Solutions

#### 1. Test Timeout
**Problem**: Test exceeds timeout and is marked as failed
**Solution**: 
- Increase timeout in JSON: `"timeout_ms": 5000`
- Or increase global timeout: `./pers_json_tests.exe 5000 test.json`

#### 2. JSON Parse Error
**Problem**: "JSON parse error at offset X"
**Solution**: 
- Validate JSON syntax using online validator
- Check for trailing commas
- Ensure proper UTF-8 encoding

#### 3. Option Not Applied
**Problem**: Options defined but not affecting test
**Solution**:
- Check option parsing in `executeOptionBasedTest()`
- Verify option name matches exactly (case-sensitive)
- Add debug logging to verify option values

#### 4. Memory Leaks
**Problem**: Memory usage increases over time
**Solution**:
- Ensure all WebGPU resources are properly released
- Check shared_ptr circular references
- Use memory profiling tools

#### 5. Thread Safety Issues
**Problem**: Random crashes or incorrect results
**Solution**:
- Logger callbacks are set per-thread
- Ensure thread-local storage for callbacks
- Avoid sharing mutable state between test threads

### Debug Mode

Enable verbose logging by modifying the test executor:

```cpp
// In json_test_main.cpp
pers::Logger::Instance().setLevel(pers::LogLevel::Debug);
```

## Best Practices

### 1. Test Organization

- **Use meaningful categories**: Group related tests together
- **Date-based folders**: Organize by date for historical tracking
- **Descriptive IDs**: Use sequential IDs with category prefixes

### 2. Option Design

- **Keep options orthogonal**: Each option should be independent
- **Use sensible defaults**: First value should be the default
- **Document options**: Always include descriptions

### 3. Test Implementation

- **Fast execution**: Keep individual tests under 1 second
- **Clean state**: Each test should be independent
- **Proper cleanup**: Always release resources
- **Clear error messages**: Provide actionable failure reasons

### 4. Maintenance

- **Version control test data**: Track all JSON files in git
- **Regular regeneration**: Regenerate tests when options change
- **Monitor test coverage**: Ensure all code paths are tested
- **Review failures**: Don't ignore intermittent failures

### 5. Performance

- **Batch small tests**: Group related quick tests
- **Parallelize when possible**: Use thread pool for independent tests
- **Cache resources**: Reuse expensive resources across tests
- **Profile slow tests**: Identify and optimize bottlenecks

## Example Workflows

### Adding a New Feature Test

```bash
# 1. Define options in schema
vim test_option_schema.json

# 2. Implement test logic
vim json_test_loader_rapidjson.cpp

# 3. Generate test cases
python generate_test_cases.py

# 4. Run tests
./pers_json_tests.exe 1000 all_generated_tests.json

# 5. Review results
cat json_test_results.md
```

### Debugging a Failing Test

```bash
# 1. Run with increased timeout
./pers_json_tests.exe 10000 test.json

# 2. Check detailed output
grep "FAIL" test_results.json -A 5

# 3. Run single test (create minimal JSON)
echo '{"test_cases": [<failing_test>]}' > debug.json
./pers_json_tests.exe 10000 debug.json

# 4. Add debug logging and rebuild
cmake --build build --config Debug
```

### Regression Testing

```bash
# 1. Save baseline results
cp test_results.json baseline_results.json

# 2. Make changes and rerun
./pers_json_tests.exe 1000 all_generated_tests.json

# 3. Compare results
diff baseline_results.json test_results.json
```

## Future Enhancements

### Planned Features

1. **Test Filtering**: Command-line filters for categories, IDs, etc.
2. **Parallel Execution**: Thread pool for faster execution
3. **Performance Benchmarking**: Track performance over time
4. **Visual Test Runner**: GUI for interactive testing
5. **Test Coverage Integration**: Link with code coverage tools
6. **Mutation Testing**: Automatically generate test variations
7. **Fuzzing Support**: Random input generation for robustness
8. **Remote Execution**: Distribute tests across machines

### Contributing

When contributing new features:

1. Update this documentation
2. Add example test cases
3. Include unit tests for the framework itself
4. Follow existing code style and patterns
5. Ensure backward compatibility

## Appendix

### JSON Schema Reference

Full JSON schema for test files:

```json
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "type": "object",
  "properties": {
    "metadata": {
      "type": "object",
      "properties": {
        "version": { "type": "string" },
        "date": { "type": "string" },
        "total_tests": { "type": "integer" }
      }
    },
    "test_cases": {
      "type": "array",
      "items": {
        "type": "object",
        "required": ["id", "category", "test_type", "expected_result"],
        "properties": {
          "id": { "type": "string" },
          "category": { "type": "string" },
          "test_type": { "type": "string" },
          "input": { "type": "object" },
          "expected_result": { "type": "string" },
          "timeout_ms": { "type": "integer", "default": 1000 },
          "enabled": { "type": "boolean", "default": true }
        }
      }
    }
  }
}
```

### Error Codes

| Code | Description | Resolution |
|------|-------------|------------|
| 0 | Success | - |
| 1 | JSON parse error | Fix JSON syntax |
| 2 | File not found | Check file path |
| 3 | Test timeout | Increase timeout |
| 4 | Assertion failure | Fix test logic |
| 5 | Resource creation failed | Check GPU/memory |

---

*Last Updated: 2025-01-02*
*Version: 1.0.0*
*Maintainer: Pers Graphics Engine Team*