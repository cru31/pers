# Test Result Display Documentation

## Overview
This document describes how test results are displayed in the WebView and how to format test case JSON files and C++ test results for optimal display.

## JSON Test Case Format

### Test Type Definition Fields

```json
{
  "testTypes": [
    {
      "category": "Data Integrity",
      "testType": "Buffer Data Verification",
      "handlerClass": "BufferDataVerificationHandler",
      "testOverview": "CPU→GPU→CPU Round Trip: [SYNC] Write → [ASYNC] Transfer → [WAIT] GPU → [ASYNC] Read → [SYNC] Verify",
      "variations": [...]
    }
  ]
}
```

#### Fields:
- **`testOverview`**: Brief summary displayed in the table's Overview column. Keep it concise (60 chars max for best display)

### Test Variation Fields

```json
{
  "id": 1,
  "variationName": "Small Sequential Data Upload (1KB)",
  "description": "[SYNC] Generate sequential → [SYNC] Write to staging → [ASYNC] GPU transfer → [WAIT] GPU → [ASYNC] Read → [SYNC] Verify",
  "execution_details": {
    "flow": "CPU write → GPU copy → Queue submit → Wait → Read back → Verify",
    "what_measured": "End-to-end round-trip time including GPU wait",
    "sync_async_flow": "[SYNC] Write → [ASYNC] Submit → [WAIT] GPU → [ASYNC] Map → [SYNC] Verify"
  },
  "options": {...},
  "expectedBehavior": {...}
}
```

#### Fields:
- **`variationName`**: Name of the specific test variation
- **`description`**: Detailed description (not displayed in expanded view to avoid duplication)
- **`execution_details`**: Object containing execution flow information
  - **`sync_async_flow`**: Primary execution flow with sync/async annotations (displayed in expanded view)
  - **`what_measured`**: Performance metrics description
  - **`flow`**: Simple flow description (not displayed if sync_async_flow exists)

## WebView Display Formatting

### Execution Flow Display

The WebView automatically formats `execution_details.sync_async_flow` with:

1. **Step Separation**: Arrow-separated (→) steps are split into numbered lines
2. **Color Coding**:
   - `[SYNC]` - Blue (#2196F3)
   - `[ASYNC]` - Orange (#FF9800)
   - `[WAIT]` - Purple (#9C27B0)
3. **Numbering**: Each step gets a padded number (01, 02, 03...)

#### Example Input:
```json
"sync_async_flow": "[SYNC] CPU writes 1KB → [ASYNC] Queue.submit() → [WAIT] GPU → [SYNC] Verify"
```

#### Displayed As:
```
Execution Flow:
  01. [SYNC] CPU writes 1KB
  02. [ASYNC] Queue.submit()
  03. [WAIT] GPU
  04. [SYNC] Verify
```

### Table Overview Column

The `testOverview` field is displayed in the table with:
- Standard text color matching other table columns
- Truncated to 60 characters for optimal display
- Shows sync/async flow summary at a glance

## C++ Test Result Structure

### Required Fields in TestResult

For proper display in WebView, the C++ `TestResult` struct must populate:

```cpp
struct TestResult {
    bool passed;
    std::string actualBehavior;
    std::string failureReason;
    std::vector<SourceLocation> handlerSourceLocations;  // Handler execution path
    // ... other fields
};
```

### Adding Source Locations

In your test handler, add source locations to track execution path:

```cpp
TestResult MyHandler::execute(const TestVariation& variation) {
    TestResult result;
    
    // Add source location at key execution points
    result.addSourceLocation(__FUNCTION__, __FILE__, __LINE__);
    
    // Your test logic here...
    
    return result;
}
```

Source locations are displayed as clickable VS Code links in the format:
```
Handler Source Locations:
  MyHandler::execute buffer_handler.cpp:104
  MyHandler::verifyData buffer_handler.cpp:173
```

### JSON Serialization

The `json_test_loader.cpp` automatically serializes these fields to result.json:

```cpp
// These fields are automatically added from TestTypeDefinition and TestVariation:
resultObj.AddMember("testOverview", testType.testOverview);
resultObj.AddMember("variationName", variation.variationName);
resultObj.AddMember("description", variation.description);

// execution_details object is serialized with all its properties
if (!variation.executionDetails.empty()) {
    Value execDetails(kObjectType);
    for (const auto& [key, value] : variation.executionDetails) {
        // Converts std::any to JSON value
        anyToValue(valVal, value);
        execDetails.AddMember(keyVal, valVal, allocator);
    }
    resultObj.AddMember("execution_details", execDetails);
}

// Handler source locations are serialized as array
if (!result.handlerSourceLocations.empty()) {
    Value sourceLocs(kArrayType);
    for (const auto& loc : result.handlerSourceLocations) {
        sourceLocs.PushBack(loc.toString());  // "function file.cpp:line"
    }
    resultObj.AddMember("handler_source_locations", sourceLocs);
}
```

## Best Practices

### 1. Sync/Async Flow Formatting
- Use `[SYNC]`, `[ASYNC]`, `[WAIT]` prefixes for automatic color coding
- Separate steps with arrows (→) for automatic line breaks
- Keep each step concise but descriptive

### 2. Test Overview
- Keep under 60 characters for table display
- Include key sync/async pattern summary
- Make it scannable at a glance

### 3. Execution Details
- Use `sync_async_flow` for detailed step-by-step flow
- Use `what_measured` to describe performance metrics
- Avoid duplicating information between fields

### 4. Source Locations
- Add `result.addSourceLocation()` at key execution points
- Include at entry points and critical validation steps
- Helps with debugging and understanding test flow

## Example Complete Test Case

```json
{
  "testTypes": [
    {
      "category": "Buffer Operations",
      "testType": "Buffer Data Verification",
      "handlerClass": "BufferDataVerificationHandler",
      "testOverview": "[SYNC] Write → [ASYNC] GPU → [WAIT] → [SYNC] Read",
      "variations": [
        {
          "id": 1,
          "variationName": "1MB Buffer Round-Trip",
          "description": "Full round-trip data integrity test",
          "execution_details": {
            "sync_async_flow": "[SYNC] Generate 1MB data → [SYNC] Write to staging → [ASYNC] Copy to GPU → [WAIT] Queue.onSubmittedWorkDone → [ASYNC] mapAsync readback → [WAIT] Promise → [SYNC] Verify data integrity",
            "what_measured": "Total round-trip time including all GPU operations"
          },
          "options": {
            "size": "1MB",
            "pattern": "sequential",
            "enable_logging": false,
            "enable_validation": true
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

This will display in the WebView with:
- Table showing overview with sync/async pattern
- Expanded view showing formatted execution flow with color-coded steps
- Performance metrics clearly displayed
- All source locations as clickable VS Code links