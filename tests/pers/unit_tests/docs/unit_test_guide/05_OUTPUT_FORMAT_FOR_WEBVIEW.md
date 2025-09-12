# Output Format Requirements for WebView

## Overview
This document specifies the JSON output format that the C++ unit test program must generate for consumption by the web viewer application.

## Output File Structure

### File Location
```
build/
└── bin/
    └── Debug/
        └── unit_test_results/
            └── {timestamp}/
                ├── result.json          # Main result file
                ├── logs.txt             # Full text logs
                └── performance.json     # Detailed performance metrics
```

### Timestamp Format
- Format: `YYYYMMDD_HHMMSS_mmm`
- Example: `20240905_174408_500`

## JSON Output Schema

### Root Structure
```json
{
  "session": {
    "id": "string",
    "timestamp": "ISO8601 datetime",
    "duration_ms": "number",
    "platform": "object",
    "gpu_info": "object"
  },
  "summary": {
    "total_tests": "integer",
    "passed": "integer",
    "failed": "integer",
    "skipped": "integer",
    "nyi": "integer",
    "na": "integer",
    "total_time_ms": "number"
  },
  "results": ["array of test results"],
  "logs": ["array of log entries"]
}
```

### Session Object
```json
{
  "session": {
    "id": "20240905_174408_500",
    "timestamp": "2024-09-05T17:44:08.500Z",
    "duration_ms": 15234.56,
    "platform": {
      "os": "Windows",
      "os_version": "10.0.19045",
      "cpu": "Intel Core i9-12900K",
      "ram_gb": 32,
      "architecture": "x64"
    },
    "gpu_info": {
      "adapter": "NVIDIA GeForce RTX 4090",
      "driver_version": "537.13",
      "vram_mb": 24576,
      "backend": "WebGPU",
      "features": ["timestamp-query", "depth-clip-control"]
    }
  }
}
```

### Test Result Object
```json
{
  "id": 1,
  "category": "Instance Management",
  "testType": "Instance Creation",
  "variationName": "Default Instance",
  "status": "passed|failed|skipped|nyi|na",
  "execution_time_ms": 3.456,
  "input_parameters": {
    "debugEnabled": false,
    "validationEnabled": false
  },
  "expected_result": "not_null",
  "actual_result": "not_null",
  "properties": {
    "isValid": true,
    "adapterCount": 2
  },
  "error": {
    "message": "string (optional)",
    "code": "integer (optional)",
    "stack": "string (optional)"
  },
  "logs": ["array of related log entries"],
  "handler_source_locations": [
    "BufferDataVerificationHandler::execute buffer_data_verification_handler.cpp:106",
    "BufferDataVerificationHandler::verifyThroughStagingCopy buffer_data_verification_handler.cpp:245"
  ],
  "performance": {
    "cpu_time_ms": 2.1,
    "gpu_time_ms": 1.3,
    "memory_used_mb": 45.2,
    "draw_calls": 0
  }
}
```

### Handler Source Locations

The `handler_source_locations` field contains an array of source code locations from the test handler execution path.

| Field | Type | Description |
|-------|------|-------------|
| `handler_source_locations` | array | List of source locations in format "functionName fileName.cpp:lineNumber" |

Each entry follows the format: `"functionName fileName.cpp:lineNumber"`

This allows:
- Tracking the execution path through the handler
- Creating clickable VS Code links in the WebView
- Multiple source locations from different parts of the handler

Example:
```json
"handler_source_locations": [
  "BufferHandler::execute buffer_handler.cpp:45",
  "BufferHandler::verifyData buffer_handler.cpp:123",
  "HelperClass::validate helper.cpp:67"
]
```

### Status Values
| Status | Description | Color in UI |
|--------|-------------|-------------|
| `passed` | Test completed successfully | Green (#48bb78) |
| `failed` | Test failed validation | Red (#f56565) |
| `skipped` | Test was skipped | Gray (#718096) |
| `nyi` | Not Yet Implemented (Engine) | Orange (#ed8936) |
| `na` | Not Applicable (Test) | Yellow (#f6e05e) |

### Log Entry Object
```json
{
  "timestamp": "2024-09-05T17:44:08.123Z",
  "level": "TRACE|DEBUG|INFO|WARNING|ERROR|CRITICAL",
  "test_id": 1,
  "message": "Creating instance with default parameters",
  "source": "InstanceCreationHandler",
  "thread_id": "0x1234"
}
```

## Required Fields for WebView

### Minimum Required Fields
These fields MUST be present for the webview to function:

```json
{
  "session": {
    "id": "required",
    "timestamp": "required"
  },
  "results": [
    {
      "id": "required",
      "category": "required",
      "testType": "required",
      "status": "required",
      "execution_time_ms": "required",
      "expected_result": "required",
      "actual_result": "required"
    }
  ]
}
```

### Optional Enhancement Fields
These fields enhance the webview experience but are not required:

- `variationName`: Displays test description
- `input_parameters`: Shows test configuration
- `properties`: Displays validation details
- `error`: Shows failure information
- `logs`: Enables log viewing
- `performance`: Shows performance metrics

## Data Type Specifications

### Parameter Values
Parameters can be of various types and the webview will render them appropriately:

```json
{
  "input_parameters": {
    "booleanParam": true,
    "integerParam": 42,
    "floatParam": 3.14,
    "stringParam": "value",
    "arrayParam": [1, 2, 3],
    "objectParam": {
      "nested": "value"
    }
  }
}
```

### Expected/Actual Result Values
```json
{
  "expected_result": "success|error|not_null|true|false|<numeric>|<string>",
  "actual_result": "success|error|null|true|false|42|custom_value"
}
```

### Property Comparisons
Properties support comparison operators in expected values:

```json
{
  "properties": {
    "count": 5,
    "isValid": true,
    "name": "test"
  }
}
```

Expected property format in test case:
```json
{
  "expectedBehavior": {
    "properties": {
      "count": ">0",
      "isValid": true,
      "name": "test"
    }
  }
}
```

## Large Dataset Handling

### Pagination Support
For large test suites, results can be paginated:

```json
{
  "pagination": {
    "total_results": 10000,
    "page": 1,
    "per_page": 1000,
    "total_pages": 10
  },
  "results": ["1000 results for current page"]
}
```

### Streaming Format
For real-time test execution, use newline-delimited JSON:

```jsonl
{"type":"session_start","data":{"id":"20240905_174408_500"}}
{"type":"test_result","data":{"id":1,"status":"passed"}}
{"type":"test_result","data":{"id":2,"status":"failed"}}
{"type":"session_end","data":{"total":2,"passed":1,"failed":1}}
```

## WebView Display Mapping

### Status Badge Rendering
```javascript
const statusConfig = {
  passed: { color: '#48bb78', icon: '✓', label: 'PASS' },
  failed: { color: '#f56565', icon: '✗', label: 'FAIL' },
  skipped: { color: '#718096', icon: '○', label: 'SKIP' },
  nyi: { color: '#ed8936', icon: '⚠', label: 'NYI' },
  na: { color: '#f6e05e', icon: '—', label: 'N/A' }
};
```

### Parameter Display
```javascript
// Boolean: Checkbox or badge
<input type="checkbox" checked={value} disabled />

// Array: Comma-separated list
<span>[item1, item2, item3]</span>

// Object: Formatted JSON
<pre>{JSON.stringify(value, null, 2)}</pre>
```

## Error Reporting Format

### Test Execution Errors
```json
{
  "error": {
    "message": "Failed to create instance",
    "code": -1001,
    "type": "INITIALIZATION_ERROR",
    "details": {
      "reason": "No compatible GPU adapter found",
      "suggestion": "Ensure WebGPU-compatible GPU is available"
    },
    "stack": "at InstanceCreationHandler::execute (line 45)"
  }
}
```

### Validation Errors
```json
{
  "validation_errors": [
    {
      "property": "adapterCount",
      "expected": ">0",
      "actual": 0,
      "message": "No adapters found"
    }
  ]
}
```

## Performance Metrics Format

### Detailed Performance Data
```json
{
  "performance": {
    "timing": {
      "setup_ms": 1.2,
      "execution_ms": 5.3,
      "validation_ms": 0.8,
      "teardown_ms": 0.5,
      "total_ms": 7.8
    },
    "memory": {
      "heap_used_mb": 45.2,
      "heap_peak_mb": 52.1,
      "vram_used_mb": 128.5,
      "vram_peak_mb": 145.3
    },
    "gpu": {
      "draw_calls": 15,
      "triangles": 50000,
      "texture_uploads": 3,
      "buffer_uploads": 5,
      "pipeline_switches": 2
    }
  }
}
```

## Backward Compatibility

### Version Information
```json
{
  "format_version": "1.0",
  "generator": {
    "name": "pers_unit_tests",
    "version": "0.1.0",
    "build": "debug"
  }
}
```

### Legacy Field Mapping
To support older webview versions:

```json
{
  "results": [
    {
      // New format
      "variationName": "Test Name",
      // Legacy support
      "description": "Test Name",
      
      // New format
      "input_parameters": {},
      // Legacy support
      "input": "param1=value1;param2=value2"
    }
  ]
}
```

## Example Complete Output

```json
{
  "format_version": "1.0",
  "session": {
    "id": "20240905_174408_500",
    "timestamp": "2024-09-05T17:44:08.500Z",
    "duration_ms": 1234.56,
    "platform": {
      "os": "Windows",
      "os_version": "10.0.19045",
      "cpu": "Intel Core i9-12900K",
      "ram_gb": 32
    },
    "gpu_info": {
      "adapter": "NVIDIA GeForce RTX 4090",
      "driver_version": "537.13",
      "backend": "WebGPU"
    }
  },
  "summary": {
    "total_tests": 3,
    "passed": 2,
    "failed": 1,
    "skipped": 0,
    "nyi": 0,
    "na": 0,
    "total_time_ms": 15.23
  },
  "results": [
    {
      "id": 1,
      "category": "Instance Management",
      "testType": "Instance Creation",
      "variationName": "Default Instance",
      "status": "passed",
      "execution_time_ms": 3.45,
      "input_parameters": {
        "debugEnabled": false,
        "validationEnabled": false
      },
      "expected_result": "not_null",
      "actual_result": "not_null",
      "properties": {
        "isValid": true,
        "adapterCount": 1
      }
    },
    {
      "id": 2,
      "category": "Instance Management",
      "testType": "Instance Creation",
      "variationName": "Debug Enabled",
      "status": "failed",
      "execution_time_ms": 5.12,
      "input_parameters": {
        "debugEnabled": true,
        "validationEnabled": false
      },
      "expected_result": "not_null",
      "actual_result": "null",
      "error": {
        "message": "Debug layer not available",
        "code": -2001
      }
    }
  ],
  "logs": [
    {
      "timestamp": "2024-09-05T17:44:08.501Z",
      "level": "INFO",
      "test_id": 1,
      "message": "Starting test: Default Instance"
    },
    {
      "timestamp": "2024-09-05T17:44:08.504Z",
      "level": "INFO",
      "test_id": 1,
      "message": "Instance created successfully"
    },
    {
      "timestamp": "2024-09-05T17:44:08.506Z",
      "level": "ERROR",
      "test_id": 2,
      "message": "Failed to enable debug layer"
    }
  ]
}
```

## WebView Integration Points

### Loading Results
```javascript
// WebView loads results from server
fetch('/api/results/20240905_174408_500')
  .then(res => res.json())
  .then(data => {
    updateSummaryCards(data.summary);
    renderResultsTable(data.results);
    populateLogsView(data.logs);
  });
```

### Real-time Updates
```javascript
// WebSocket for live updates
const ws = new WebSocket('ws://localhost:5000/live');
ws.onmessage = (event) => {
  const update = JSON.parse(event.data);
  if (update.type === 'test_result') {
    addResultToTable(update.data);
  }
};
```

### Export Functionality
The webview can export filtered results back to JSON:

```javascript
function exportResults(filteredResults) {
  const exportData = {
    format_version: "1.0",
    exported_at: new Date().toISOString(),
    filter_criteria: getActiveFilters(),
    results: filteredResults
  };
  downloadJSON(exportData, 'filtered_results.json');
}
```