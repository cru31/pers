# Test Case JSON Writing Guide

## Overview
This guide explains how to write test case JSON files for the Pers Graphics Engine unit testing framework. Test cases define the input parameters and expected behaviors for automated testing.

## JSON Structure

### Top-Level Structure
```json
{
  "fileType": "test_cases",
  "fileId": "unique_identifier",
  "metadata": {
    "version": "1.0",
    "category": "Category Name",
    "description": "Description of what these tests cover"
  },
  "testTypes": [...]
}
```

### Test Type Structure
Each test type represents a group of related test variations:

```json
{
  "category": "Instance Management",
  "testType": "Instance Creation",
  "handlerClass": "InstanceCreationHandler",
  "variations": [...]
}
```

### Variation Structure
Each variation is a specific test case with different parameters:

```json
{
  "id": 1,
  "variationName": "Default Instance",
  "options": {
    "debugEnabled": false,
    "validationEnabled": false
  },
  "expectedBehavior": {
    "returnValue": "not_null",
    "properties": {
      "isValid": true,
      "adapterCount": ">0"
    }
  }
}
```

## Field Descriptions

### Required Fields

| Field | Type | Description |
|-------|------|-------------|
| `id` | integer | Unique identifier within the test file |
| `variationName` | string | Descriptive name for this test variation |
| `options` | object | Input parameters for the test |
| `expectedBehavior` | object | Expected test outcomes |

### Expected Behavior Options

#### Return Values
- `"success"` - Operation completed successfully
- `"not_null"` - Returned pointer/object is not null
- `"true"` / `"false"` - Boolean return value
- `"error"` - Operation should fail
- Numeric values - Specific return codes

#### Property Comparisons
- `">0"` - Greater than zero
- `"<100"` - Less than 100
- `">=1"` - Greater than or equal to 1
- `"!=0"` - Not equal to zero
- Exact values - Must match exactly

## Complete Example

```json
{
  "fileType": "test_cases",
  "fileId": "inst",
  "metadata": {
    "version": "1.0",
    "category": "Instance Management",
    "description": "Tests for WebGPU instance creation and configuration"
  },
  "testTypes": [
    {
      "category": "Instance Management",
      "testType": "Instance Creation",
      "handlerClass": "InstanceCreationHandler",
      "variations": [
        {
          "id": 1,
          "variationName": "Default Instance",
          "options": {
            "debugEnabled": false,
            "validationEnabled": false
          },
          "expectedBehavior": {
            "returnValue": "not_null",
            "properties": {
              "isValid": true,
              "adapterCount": ">0"
            }
          }
        },
        {
          "id": 2,
          "variationName": "Debug Enabled Instance",
          "options": {
            "debugEnabled": true,
            "validationEnabled": false
          },
          "expectedBehavior": {
            "returnValue": "not_null",
            "properties": {
              "isValid": true,
              "debugLayerActive": true
            }
          }
        }
      ]
    }
  ]
}
```

## Parameter Types

### Boolean Parameters
```json
"options": {
  "debugEnabled": true,
  "validationEnabled": false
}
```

### Numeric Parameters
```json
"options": {
  "width": 1920,
  "height": 1080,
  "sampleCount": 4
}
```

### String Parameters
```json
"options": {
  "format": "bgra8unorm",
  "presentMode": "fifo"
}
```

### Array Parameters
```json
"options": {
  "extensions": ["ext1", "ext2"],
  "formats": ["rgba8", "bgra8"]
}
```

### Object Parameters
```json
"options": {
  "limits": {
    "maxTextureSize": 4096,
    "maxBindGroups": 4
  }
}
```

## Special Options

### Performance Control Options
These options control logging and validation for performance testing:

```json
"options": {
  "enable_logging": false,      // Disable all engine logging for performance tests
  "enable_validation": false,   // Disable WebGPU validation layers
  "verbose": false              // Control verbose output from handler
}
```

- **enable_logging** (bool): Controls whether engine logging is enabled. Default: true
  - Set to `false` for performance benchmarks to reduce overhead
  - Affects all log levels (INFO, DEBUG, TRACE, WARNING)
  
- **enable_validation** (bool): Controls WebGPU validation layers. Default: true
  - Set to `false` for performance tests to reduce GPU driver overhead
  - Should be `true` for correctness testing

### Buffer Test Specific Options
```json
"options": {
  "size": 268435456,           // Buffer size in bytes (256MB)
  "pattern": "sequential",      // Data pattern: sequential, random, gradient, binary
  "verify_method": "readback"   // Verification method: readback, mapping, compute_shader
}
```

## Best Practices

1. **Unique IDs**: Ensure each variation has a unique ID within the file
2. **Descriptive Names**: Use clear, descriptive variation names
3. **Complete Coverage**: Include both positive and negative test cases
4. **Edge Cases**: Test boundary conditions and error scenarios
5. **Minimal Options**: Only include parameters relevant to what's being tested
6. **Clear Expectations**: Be explicit about expected outcomes
7. **Performance Testing**: Use `enable_logging: false` and `enable_validation: false` for accurate benchmarks
8. **Debug Testing**: Keep logging and validation enabled when testing correctness

## Common Patterns

### Testing Success Cases
```json
{
  "id": 1,
  "variationName": "Valid Configuration",
  "options": {
    "width": 1024,
    "height": 768
  },
  "expectedBehavior": {
    "returnValue": "success"
  }
}
```

### Testing Error Handling
```json
{
  "id": 2,
  "variationName": "Invalid Size",
  "options": {
    "width": -1,
    "height": 0
  },
  "expectedBehavior": {
    "returnValue": "error",
    "errorCode": "INVALID_DIMENSIONS"
  }
}
```

### Testing Resource Creation
```json
{
  "id": 3,
  "variationName": "Create Buffer",
  "options": {
    "size": 1024,
    "usage": "VERTEX"
  },
  "expectedBehavior": {
    "returnValue": "not_null",
    "properties": {
      "size": 1024,
      "mapped": false
    }
  }
}
```

## File Organization

```
test_cases/
├── set_01/
│   ├── instance_tests.json      # Instance management tests
│   ├── device_tests.json        # Device creation tests
│   └── buffer_tests.json        # Buffer operation tests
├── set_02/
│   ├── pipeline_tests.json      # Pipeline tests
│   └── shader_tests.json        # Shader compilation tests
└── README.md
```

## Validation

Before using test case files:

1. Validate JSON syntax using a JSON validator
2. Ensure all required fields are present
3. Check that handler classes exist in the C++ implementation
4. Verify parameter names match the handler's expectations
5. Test with a small subset before running full suite

## Integration with Web Viewer

The web viewer can:
- Import these JSON files
- Edit test cases visually
- Export modified test cases
- Generate new test variations

Use the "Test Case JSON Editor" in the web viewer to:
1. Create new test cases with proper structure
2. Validate JSON format automatically
3. Use autocomplete for common values
4. Preview JSON before export