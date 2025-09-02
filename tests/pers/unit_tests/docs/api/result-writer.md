# Result Writer API Reference

## Classes

### TestResultWriter

Manages test result serialization to JSON and Markdown formats.

```cpp
class TestResultWriter {
public:
    TestResultWriter();
    
    void addResult(const TestResult& result);
    bool writeToJson(const std::string& filepath);
    bool writeToMarkdown(const std::string& filepath);
    bool writeResults(const std::string& jsonPath, 
                     const std::string& markdownPath);
    
    Summary getSummary() const;
    void clear();
    
private:
    std::vector<TestResult> _results;
    std::string getCurrentTimestamp() const;
    bool hasTodoOrDieInLogs(const std::vector<std::string>& logs) const;
};
```

#### Methods

##### `addResult(const TestResult& result)`

Adds a test result to the writer.

**Parameters:**
- `result`: Test result to add

**Example:**
```cpp
TestResultWriter writer;
writer.addResult(testResult);
```

##### `writeToJson(const std::string& filepath)`

Writes results to a JSON file.

**Parameters:**
- `filepath`: Output JSON file path

**Returns:** `bool` - true if write succeeded

**Output Format:**
```json
{
    "metadata": {
        "version": "1.0.0",
        "timestamp": "2025-01-02 12:34:56",
        "total_tests": 24,
        "passed": 9,
        "failed": 0,
        "not_applicable": 15,
        "not_yet_implemented": 9,
        "skipped": 0,
        "pass_rate": 37.5,
        "total_time_ms": 964.42
    },
    "results": [
        {
            "id": "001",
            "category": "Critical Path",
            "test_type": "WebGPU Instance Creation",
            "passed": true,
            "expected_result": "Valid instance created",
            "actual_result": "Valid instance created",
            "failure_reason": "",
            "execution_time_ms": 215.05,
            "timestamp": "2025-01-02 12:34:56",
            "input": "type=InstanceDesc, ...",
            "expected_callstack": [...],
            "log_messages": [...]
        }
    ]
}
```

##### `writeToMarkdown(const std::string& filepath)`

Writes results to a Markdown table file.

**Parameters:**
- `filepath`: Output Markdown file path

**Returns:** `bool` - true if write succeeded

**Output Format:**
```markdown
# Test Results Table

Generated: 2025-01-02 12:34:56

## Summary

- **Total Tests**: 24
- **Passed**: 9
- **Failed**: 0
- **N/A (Not Test Targets)**: 15
- **Not Yet Implemented**: 9
- **Pass Rate**: 37.5%
- **Total Time**: 964.42ms

## Critical Path

| ID | Test Type | Input | Expected | Actual | Pass/Fail | Reason |
|----|-----------|-------|----------|--------|-----------|--------|
| 001| WebGPU... | ... | Valid... | Valid...| PASS | |
```

##### `getSummary() const`

Calculates summary statistics for all results.

**Returns:** `Summary` struct with statistics

**Example:**
```cpp
auto summary = writer.getSummary();
std::cout << "Pass rate: " << summary.passRate << "%" << std::endl;
```

### Summary

Summary statistics structure.

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
```

#### Fields

| Field | Type | Description |
|-------|------|-------------|
| `totalTests` | int | Total number of tests |
| `passed` | int | Tests that passed |
| `failed` | int | Tests that failed |
| `notApplicable` | int | N/A test targets |
| `notYetImplemented` | int | Tests with TodoOrDie |
| `skipped` | int | Skipped tests |
| `totalTimeMs` | double | Total execution time |
| `passRate` | double | Percentage of passed tests |

## JSON Serialization

### Result Serialization

```cpp
json TestResultWriter::serializeResult(const TestResult& result) {
    json j;
    
    j["id"] = result.id;
    j["category"] = result.category;
    j["test_type"] = result.testType;
    j["passed"] = result.passed;
    j["expected_result"] = result.expectedResult;
    j["actual_result"] = result.actualResult;
    j["failure_reason"] = result.failureReason;
    j["execution_time_ms"] = result.executionTimeMs;
    j["timestamp"] = formatTimestamp(result.timestamp);
    
    // Add input string
    j["input"] = formatInput(result.input);
    
    // Add expected callstack
    j["expected_callstack"] = result.expectedCallstack;
    
    // Add log messages
    j["log_messages"] = result.logMessages;
    
    return j;
}
```

### Metadata Generation

```cpp
json TestResultWriter::generateMetadata() {
    json metadata;
    auto summary = getSummary();
    
    metadata["version"] = "1.0.0";
    metadata["timestamp"] = getCurrentTimestamp();
    metadata["total_tests"] = summary.totalTests;
    metadata["passed"] = summary.passed;
    metadata["failed"] = summary.failed;
    metadata["not_applicable"] = summary.notApplicable;
    metadata["not_yet_implemented"] = summary.notYetImplemented;
    metadata["skipped"] = summary.skipped;
    metadata["pass_rate"] = summary.passRate;
    metadata["total_time_ms"] = summary.totalTimeMs;
    
    return metadata;
}
```

## Markdown Generation

### Table Formatting

```cpp
void TestResultWriter::writeMarkdownTable(std::ofstream& file,
                                         const std::string& category) {
    file << "## " << category << "\n\n";
    file << "| ID | Test Type | Input | Expected | Actual | ";
    file << "Callstack | Pass/Fail | Reason |\n";
    file << "|" << std::string(100, '-') << "|\n";
    
    for (const auto& result : _results) {
        if (result.category != category) continue;
        
        file << "| " << result.id;
        file << " | " << result.testType;
        file << " | " << truncate(formatInput(result.input), 30);
        file << " | " << truncate(result.expectedResult, 20);
        file << " | " << truncate(result.actualResult, 20);
        file << " | " << formatCallstack(result.expectedCallstack);
        file << " | " << getStatusBadge(result);
        file << " | " << result.failureReason;
        file << " |\n";
    }
}
```

### Status Badge Generation

```cpp
std::string TestResultWriter::getStatusBadge(const TestResult& result) {
    std::vector<std::string> badges;
    
    // Primary status
    if (result.actualResult.find("N/A") != std::string::npos) {
        badges.push_back("🔵N/A");
    } else if (result.passed) {
        badges.push_back("✅PASS");
    } else {
        badges.push_back("❌FAIL");
    }
    
    // Secondary status
    if (hasTodoOrDieInLogs(result.logMessages)) {
        badges.push_back("🚧NYI");
    }
    
    // Join badges
    std::string badge;
    for (size_t i = 0; i < badges.size(); ++i) {
        if (i > 0) badge += " ";
        badge += badges[i];
    }
    
    return badge;
}
```

## Utility Functions

### String Formatting

```cpp
namespace FormatUtils {
    // Truncate string with ellipsis
    std::string truncate(const std::string& str, size_t maxLen) {
        if (str.length() <= maxLen) return str;
        return str.substr(0, maxLen - 3) + "...";
    }
    
    // Format timestamp
    std::string formatTimestamp(const std::chrono::time_point<>& tp) {
        auto time_t = std::chrono::system_clock::to_time_t(tp);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }
    
    // Format input parameters
    std::string formatInput(const json& input) {
        std::stringstream ss;
        
        if (input.contains("type")) {
            ss << "type=" << input["type"].get<std::string>();
        }
        
        if (input.contains("values")) {
            for (auto& [key, value] : input["values"].items()) {
                ss << ", " << key << "=";
                
                if (value.is_string()) {
                    ss << value.get<std::string>();
                } else if (value.is_number()) {
                    ss << value;
                } else if (value.is_boolean()) {
                    ss << (value.get<bool>() ? "true" : "false");
                } else if (value.is_array()) {
                    ss << "[" << value.size() << " items]";
                }
            }
        }
        
        return ss.str();
    }
    
    // Format callstack
    std::string formatCallstack(const std::vector<std::string>& stack) {
        if (stack.empty()) return "N/A";
        
        // Take first and last entries
        std::string result = stack.front();
        if (stack.size() > 1) {
            result += " →...→ " + stack.back();
        }
        
        return truncate(result, 50);
    }
}
```

### TodoOrDie Detection

```cpp
bool TestResultWriter::hasTodoOrDieInLogs(
    const std::vector<std::string>& logMessages) const {
    
    for (const auto& log : logMessages) {
        // Check for TodoOrDie markers
        if (log.find("[TODO_OR_DIE]") != std::string::npos ||
            log.find("[TODO!]") != std::string::npos ||
            log.find("TodoOrDie") != std::string::npos) {
            return true;
        }
    }
    
    return false;
}
```

## File Operations

### Safe File Writing

```cpp
bool TestResultWriter::safeWriteFile(const std::string& filepath,
                                    const std::function<void(std::ofstream&)>& writer) {
    try {
        // Create temp file
        std::string tempPath = filepath + ".tmp";
        std::ofstream file(tempPath);
        
        if (!file.is_open()) {
            std::cerr << "Failed to open: " << tempPath << std::endl;
            return false;
        }
        
        // Write content
        writer(file);
        file.close();
        
        // Atomic rename
        std::filesystem::rename(tempPath, filepath);
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Write error: " << e.what() << std::endl;
        return false;
    }
}
```

### Directory Creation

```cpp
bool TestResultWriter::ensureDirectory(const std::string& filepath) {
    try {
        std::filesystem::path path(filepath);
        std::filesystem::path dir = path.parent_path();
        
        if (!dir.empty() && !std::filesystem::exists(dir)) {
            std::filesystem::create_directories(dir);
        }
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to create directory: " << e.what() << std::endl;
        return false;
    }
}
```

## Usage Examples

### Basic Usage

```cpp
// Create writer
TestResultWriter writer;

// Add results
for (const auto& result : testResults) {
    writer.addResult(result);
}

// Write to files
writer.writeResults("results.json", "results.md");

// Get summary
auto summary = writer.getSummary();
std::cout << "Tests passed: " << summary.passed 
          << "/" << summary.totalTests << std::endl;
```

### Custom Output

```cpp
// Custom JSON output
class CustomResultWriter : public TestResultWriter {
protected:
    json serializeResult(const TestResult& result) override {
        json j = TestResultWriter::serializeResult(result);
        
        // Add custom fields
        j["custom_field"] = "custom_value";
        j["test_environment"] = getEnvironmentInfo();
        
        return j;
    }
};
```

### Streaming Results

```cpp
// Write results as they complete
class StreamingResultWriter {
private:
    std::ofstream _stream;
    bool _firstResult = true;
    
public:
    StreamingResultWriter(const std::string& filepath) 
        : _stream(filepath) {
        _stream << "{\"results\":[";
    }
    
    ~StreamingResultWriter() {
        _stream << "]}";
        _stream.close();
    }
    
    void writeResult(const TestResult& result) {
        if (!_firstResult) {
            _stream << ",";
        }
        _firstResult = false;
        
        json j = serializeResult(result);
        _stream << j.dump() << std::flush;
    }
};
```

### Report Generation

```cpp
// Generate HTML report
void generateHtmlReport(const TestResultWriter& writer,
                       const std::string& outputPath) {
    std::ofstream html(outputPath);
    auto summary = writer.getSummary();
    
    html << "<!DOCTYPE html>\n<html>\n<head>\n";
    html << "<title>Test Results</title>\n";
    html << "<style>/* CSS styles */</style>\n";
    html << "</head>\n<body>\n";
    
    // Summary section
    html << "<h1>Test Results</h1>\n";
    html << "<div class='summary'>\n";
    html << "<p>Total: " << summary.totalTests << "</p>\n";
    html << "<p>Passed: " << summary.passed << "</p>\n";
    html << "<p>Failed: " << summary.failed << "</p>\n";
    html << "<p>Pass Rate: " << summary.passRate << "%</p>\n";
    html << "</div>\n";
    
    // Results table
    html << "<table>\n";
    // ... generate table rows
    html << "</table>\n";
    
    html << "</body>\n</html>\n";
}
```

## Error Handling

### Write Error Recovery

```cpp
bool TestResultWriter::writeWithRetry(const std::string& filepath,
                                     int maxRetries = 3) {
    for (int i = 0; i < maxRetries; ++i) {
        if (writeToJson(filepath)) {
            return true;
        }
        
        std::cerr << "Write failed, retry " << (i + 1) 
                  << "/" << maxRetries << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    return false;
}
```

### Validation

```cpp
bool TestResultWriter::validateResults() const {
    for (const auto& result : _results) {
        // Check required fields
        if (result.id.empty()) {
            std::cerr << "Result missing ID" << std::endl;
            return false;
        }
        
        if (result.category.empty()) {
            std::cerr << "Result " << result.id 
                      << " missing category" << std::endl;
            return false;
        }
        
        // Check consistency
        if (result.passed && !result.failureReason.empty()) {
            std::cerr << "Result " << result.id 
                      << " passed but has failure reason" << std::endl;
            return false;
        }
    }
    
    return true;
}
```