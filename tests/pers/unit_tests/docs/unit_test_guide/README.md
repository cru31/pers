# Pers Graphics Engine - Unit Test Documentation

## Overview
This directory contains comprehensive documentation for the Pers Graphics Engine unit testing framework. The framework provides automated testing for WebGPU graphics operations with a JSON-based test definition system and web-based result viewer.

## Documentation Structure

### 📄 [01_TEST_CASE_JSON_WRITING_GUIDE.md](01_TEST_CASE_JSON_WRITING_GUIDE.md)
Learn how to write test case JSON files that define test parameters and expected behaviors.
- JSON structure and field descriptions
- Parameter types and formats
- Best practices and examples
- File organization guidelines

### 📄 [02_HANDLER_CLASS_IMPLEMENTATION_GUIDE.md](02_HANDLER_CLASS_IMPLEMENTATION_GUIDE.md)
Comprehensive guide for implementing C++ handler classes that execute tests.
- ITestHandler interface implementation
- Parameter parsing techniques
- Error handling and validation
- Performance measurement
- Complex handler examples

### 📄 [03_CPP_INTERFACE_REQUIREMENTS.md](03_CPP_INTERFACE_REQUIREMENTS.md)
Complete C++ interface specifications for the testing framework.
- Core interfaces (ITestHandler, ITestRunner, etc.)
- Data structures (TestResult, TestConfig)
- Handler registry system
- Exception types and utility interfaces
- Implementation requirements and compliance checklist

### 📄 [04_UNIT_TEST_PROGRAM_OPERATION.md](04_UNIT_TEST_PROGRAM_OPERATION.md)
Detailed explanation of the test program's execution sequence.
- Program architecture overview
- Step-by-step operation phases
- Execution flow diagrams
- Command-line interface
- Configuration options
- Exit codes and error handling

### 📄 [05_OUTPUT_FORMAT_FOR_WEBVIEW.md](05_OUTPUT_FORMAT_FOR_WEBVIEW.md)
Specifications for the JSON output format consumed by the web viewer.
- Output file structure and location
- JSON schema for test results
- Status values and display mapping
- Performance metrics format
- WebView integration points
- Real-time update support

### 📄 [06_JSON_SCHEMA_DOCUMENTATION.md](06_JSON_SCHEMA_DOCUMENTATION.md)
Formal JSON Schema definitions for validation and tooling.
- Complete test case file schema
- Test result schema
- Validation examples (JavaScript/C++)
- Schema evolution and versioning
- CI/CD integration
- Tools and resources

## Quick Start

### Writing Your First Test Case

1. **Create a test case JSON file:**
```json
{
  "fileType": "test_cases",
  "fileId": "my_test",
  "metadata": {
    "version": "1.0",
    "category": "My Tests",
    "description": "Example test suite"
  },
  "testTypes": [
    {
      "category": "Example",
      "testType": "Basic Test",
      "handlerClass": "BasicTestHandler",
      "variations": [
        {
          "id": 1,
          "variationName": "Simple Test",
          "options": {
            "param1": true,
            "param2": 42
          },
          "expectedBehavior": {
            "returnValue": "success"
          }
        }
      ]
    }
  ]
}
```

2. **Implement the handler class:**
```cpp
class BasicTestHandler : public ITestHandler {
public:
    TestResult execute(const json& options) override {
        TestResult result;
        // Test implementation
        result.returnValue = "success";
        return result;
    }
    // ... other required methods
};
```

3. **Run the test:**
```bash
./unit_tests --test-file my_test.json
```

4. **View results in web viewer:**
```bash
cd tests/pers/unit_tests/web
npm start
# Open http://localhost:5000
```

## System Architecture

```
┌──────────────┐     ┌──────────────┐     ┌──────────────┐
│  Test Cases  │────▶│ Test Runner  │────▶│   Results    │
│   (JSON)     │     │    (C++)     │     │   (JSON)     │
└──────────────┘     └──────────────┘     └──────────────┘
                            │                      │
                            ▼                      ▼
                     ┌──────────────┐     ┌──────────────┐
                     │   Handlers   │     │  Web Viewer  │
                     │    (C++)     │     │ (JavaScript) │
                     └──────────────┘     └──────────────┘
```

## Key Features

### 🎯 Test Definition
- **JSON-based**: Human-readable test definitions
- **Parameterized**: Multiple test variations with different inputs
- **Flexible**: Support for various data types and structures
- **Validated**: JSON Schema validation ensures correctness

### ⚙️ Test Execution
- **Modular handlers**: Separate handler for each test type
- **Performance tracking**: Automatic timing and memory measurement
- **Error handling**: Comprehensive error reporting and recovery
- **Parallel execution**: Optional multi-threaded test execution

### 📊 Result Analysis
- **Web viewer**: Interactive result visualization
- **Real-time updates**: Live test execution monitoring
- **Filtering & search**: Find specific tests quickly
- **Export options**: JSON, Markdown, JUnit XML formats

### 🔧 Development Tools
- **Test editor**: Visual test case creation in web viewer
- **Autocomplete**: Parameter and value suggestions
- **Validation**: Real-time JSON validation
- **Documentation**: Comprehensive guides and examples

## File Structure

```
tests/pers/unit_tests/
├── docs/
│   └── unit_test_guide/        # This documentation
├── test_cases/
│   ├── set_01/                 # Test case JSON files
│   │   ├── instance_tests.json
│   │   └── device_tests.json
│   └── set_02/
│       └── pipeline_tests.json
├── handlers/                   # C++ handler implementations
│   ├── InstanceCreationHandler.cpp
│   └── DeviceCreationHandler.cpp
├── web/                        # Web viewer application
│   ├── public/
│   │   ├── index.html
│   │   ├── app.js
│   │   └── styles.css
│   └── server.js
└── results/                    # Test execution results
    └── 20240905_174408_500/
        └── result.json
```

## Common Use Cases

### Running All Tests
```bash
./unit_tests
```

### Running Specific Test Set
```bash
./unit_tests --test-file test_cases/set_01/instance_tests.json
```

### Debugging Failed Tests
```bash
./unit_tests --test-ids 5,12,23 --verbose --debug
```

### Generating Reports
```bash
./unit_tests --output-format markdown --output-path report.md
```

### Performance Profiling
```bash
./unit_tests --profile --profile-output performance.json
```

## Troubleshooting

### Test Fails to Load
- Validate JSON syntax using online validator
- Check handler class name matches implementation
- Ensure all required fields are present

### Handler Not Found
- Verify handler is registered with REGISTER_TEST_HANDLER macro
- Check handler class name spelling
- Ensure handler is linked into executable

### Web Viewer Issues
- Check Node.js version (requires v14+)
- Verify result.json exists in expected location
- Check browser console for JavaScript errors

### Performance Issues
- Enable parallel execution with --parallel flag
- Increase timeout for slow tests
- Profile with --profile flag to identify bottlenecks

## Contributing

### Adding New Test Types
1. Define test cases in JSON
2. Implement handler class
3. Register handler
4. Add documentation
5. Submit pull request

### Reporting Issues
- Include test case JSON
- Provide error messages
- Attach result.json if available
- Describe expected vs actual behavior

## Version History

### v1.0.0 (Current)
- Initial release
- Basic test framework
- Web viewer application
- Core handler implementations

### Planned Features
- [ ] Test coverage reporting
- [ ] Continuous integration support
- [ ] Visual regression testing
- [ ] Performance regression detection
- [ ] Test case generation tools

## License
See LICENSE file in repository root.

## Contact
For questions or support, please open an issue on the project repository.