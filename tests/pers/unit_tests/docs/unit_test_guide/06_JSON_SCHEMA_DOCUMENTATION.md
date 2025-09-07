# JSON Schema Documentation

## Overview
This document provides the formal JSON Schema definitions for all JSON formats used in the Pers Graphics Engine unit testing framework.

## Test Case Schema

### Complete Test Case File Schema
```json
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "$id": "https://pers-engine.dev/schemas/test-case-file.json",
  "title": "Test Case File",
  "description": "Schema for test case definition files",
  "type": "object",
  "required": ["fileType", "fileId", "metadata", "testTypes"],
  "properties": {
    "fileType": {
      "type": "string",
      "const": "test_cases",
      "description": "File type identifier"
    },
    "fileId": {
      "type": "string",
      "pattern": "^[a-z_]+$",
      "description": "Unique file identifier"
    },
    "metadata": {
      "type": "object",
      "required": ["version", "category", "description"],
      "properties": {
        "version": {
          "type": "string",
          "pattern": "^\\d+\\.\\d+$",
          "description": "Schema version"
        },
        "category": {
          "type": "string",
          "description": "Test category name"
        },
        "description": {
          "type": "string",
          "description": "Test suite description"
        }
      }
    },
    "testTypes": {
      "type": "array",
      "minItems": 1,
      "items": {
        "$ref": "#/definitions/testType"
      }
    }
  },
  "definitions": {
    "testType": {
      "type": "object",
      "required": ["category", "testType", "handlerClass", "variations"],
      "properties": {
        "category": {
          "type": "string",
          "description": "Test category"
        },
        "testType": {
          "type": "string",
          "description": "Type of test"
        },
        "handlerClass": {
          "type": "string",
          "pattern": "^[A-Z][a-zA-Z0-9]*Handler$",
          "description": "C++ handler class name"
        },
        "variations": {
          "type": "array",
          "minItems": 1,
          "items": {
            "$ref": "#/definitions/variation"
          }
        }
      }
    },
    "variation": {
      "type": "object",
      "required": ["id", "variationName", "options", "expectedBehavior"],
      "properties": {
        "id": {
          "type": "integer",
          "minimum": 1,
          "description": "Unique test ID"
        },
        "variationName": {
          "type": "string",
          "description": "Test variation description"
        },
        "options": {
          "type": "object",
          "description": "Input parameters for test",
          "additionalProperties": true
        },
        "expectedBehavior": {
          "$ref": "#/definitions/expectedBehavior"
        }
      }
    },
    "expectedBehavior": {
      "type": "object",
      "required": ["returnValue"],
      "properties": {
        "returnValue": {
          "type": "string",
          "description": "Expected return value"
        },
        "properties": {
          "type": "object",
          "description": "Expected property values",
          "additionalProperties": {
            "oneOf": [
              {"type": "boolean"},
              {"type": "number"},
              {"type": "string"}
            ]
          }
        },
        "shouldFail": {
          "type": "boolean",
          "description": "Whether test should fail"
        },
        "errorCode": {
          "type": ["string", "integer"],
          "description": "Expected error code"
        }
      }
    }
  }
}
```

### Parameter Types Schema
```json
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "$id": "https://pers-engine.dev/schemas/parameter-types.json",
  "title": "Parameter Types",
  "definitions": {
    "parameterValue": {
      "oneOf": [
        {
          "type": "boolean",
          "description": "Boolean parameter"
        },
        {
          "type": "integer",
          "description": "Integer parameter"
        },
        {
          "type": "number",
          "description": "Floating point parameter"
        },
        {
          "type": "string",
          "description": "String parameter"
        },
        {
          "type": "array",
          "description": "Array parameter",
          "items": {
            "$ref": "#/definitions/parameterValue"
          }
        },
        {
          "type": "object",
          "description": "Object parameter",
          "additionalProperties": {
            "$ref": "#/definitions/parameterValue"
          }
        }
      ]
    }
  }
}
```

## Test Result Schema

### Complete Result File Schema
```json
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "$id": "https://pers-engine.dev/schemas/test-results.json",
  "title": "Test Results",
  "description": "Schema for test execution results",
  "type": "object",
  "required": ["session", "summary", "results"],
  "properties": {
    "format_version": {
      "type": "string",
      "pattern": "^\\d+\\.\\d+$",
      "default": "1.0"
    },
    "session": {
      "$ref": "#/definitions/session"
    },
    "summary": {
      "$ref": "#/definitions/summary"
    },
    "results": {
      "type": "array",
      "items": {
        "$ref": "#/definitions/testResult"
      }
    },
    "logs": {
      "type": "array",
      "items": {
        "$ref": "#/definitions/logEntry"
      }
    },
    "pagination": {
      "$ref": "#/definitions/pagination"
    }
  },
  "definitions": {
    "session": {
      "type": "object",
      "required": ["id", "timestamp"],
      "properties": {
        "id": {
          "type": "string",
          "pattern": "^\\d{8}_\\d{6}_\\d{3}$"
        },
        "timestamp": {
          "type": "string",
          "format": "date-time"
        },
        "duration_ms": {
          "type": "number",
          "minimum": 0
        },
        "platform": {
          "$ref": "#/definitions/platform"
        },
        "gpu_info": {
          "$ref": "#/definitions/gpuInfo"
        }
      }
    },
    "platform": {
      "type": "object",
      "properties": {
        "os": {
          "type": "string",
          "enum": ["Windows", "Linux", "macOS"]
        },
        "os_version": {
          "type": "string"
        },
        "cpu": {
          "type": "string"
        },
        "ram_gb": {
          "type": "integer",
          "minimum": 1
        },
        "architecture": {
          "type": "string",
          "enum": ["x64", "x86", "arm64"]
        }
      }
    },
    "gpuInfo": {
      "type": "object",
      "properties": {
        "adapter": {
          "type": "string"
        },
        "driver_version": {
          "type": "string"
        },
        "vram_mb": {
          "type": "integer",
          "minimum": 0
        },
        "backend": {
          "type": "string",
          "enum": ["WebGPU", "Vulkan", "D3D12", "Metal"]
        },
        "features": {
          "type": "array",
          "items": {
            "type": "string"
          }
        }
      }
    },
    "summary": {
      "type": "object",
      "required": ["total_tests", "passed", "failed"],
      "properties": {
        "total_tests": {
          "type": "integer",
          "minimum": 0
        },
        "passed": {
          "type": "integer",
          "minimum": 0
        },
        "failed": {
          "type": "integer",
          "minimum": 0
        },
        "skipped": {
          "type": "integer",
          "minimum": 0
        },
        "nyi": {
          "type": "integer",
          "minimum": 0
        },
        "na": {
          "type": "integer",
          "minimum": 0
        },
        "total_time_ms": {
          "type": "number",
          "minimum": 0
        }
      }
    },
    "testResult": {
      "type": "object",
      "required": ["id", "category", "testType", "status", "execution_time_ms", "expected_result", "actual_result"],
      "properties": {
        "id": {
          "type": "integer",
          "minimum": 1
        },
        "category": {
          "type": "string"
        },
        "testType": {
          "type": "string"
        },
        "variationName": {
          "type": "string"
        },
        "status": {
          "type": "string",
          "enum": ["passed", "failed", "skipped", "nyi", "na"]
        },
        "execution_time_ms": {
          "type": "number",
          "minimum": 0
        },
        "input_parameters": {
          "type": "object",
          "additionalProperties": true
        },
        "expected_result": {
          "type": "string"
        },
        "actual_result": {
          "type": "string"
        },
        "properties": {
          "type": "object",
          "additionalProperties": true
        },
        "error": {
          "$ref": "#/definitions/error"
        },
        "logs": {
          "type": "array",
          "items": {
            "$ref": "#/definitions/logEntry"
          }
        },
        "performance": {
          "$ref": "#/definitions/performance"
        }
      }
    },
    "error": {
      "type": "object",
      "properties": {
        "message": {
          "type": "string"
        },
        "code": {
          "type": "integer"
        },
        "type": {
          "type": "string"
        },
        "stack": {
          "type": "string"
        },
        "details": {
          "type": "object",
          "additionalProperties": true
        }
      }
    },
    "logEntry": {
      "type": "object",
      "required": ["timestamp", "level", "message"],
      "properties": {
        "timestamp": {
          "type": "string",
          "format": "date-time"
        },
        "level": {
          "type": "string",
          "enum": ["TRACE", "DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL"]
        },
        "test_id": {
          "type": "integer"
        },
        "message": {
          "type": "string"
        },
        "source": {
          "type": "string"
        },
        "thread_id": {
          "type": "string"
        }
      }
    },
    "performance": {
      "type": "object",
      "properties": {
        "cpu_time_ms": {
          "type": "number",
          "minimum": 0
        },
        "gpu_time_ms": {
          "type": "number",
          "minimum": 0
        },
        "memory_used_mb": {
          "type": "number",
          "minimum": 0
        },
        "draw_calls": {
          "type": "integer",
          "minimum": 0
        }
      }
    },
    "pagination": {
      "type": "object",
      "required": ["total_results", "page", "per_page", "total_pages"],
      "properties": {
        "total_results": {
          "type": "integer",
          "minimum": 0
        },
        "page": {
          "type": "integer",
          "minimum": 1
        },
        "per_page": {
          "type": "integer",
          "minimum": 1
        },
        "total_pages": {
          "type": "integer",
          "minimum": 1
        }
      }
    }
  }
}
```

## Validation Examples

### JavaScript Validation
```javascript
import Ajv from 'ajv';
import testCaseSchema from './schemas/test-case-file.json';
import testResultSchema from './schemas/test-results.json';

const ajv = new Ajv();

// Validate test case file
function validateTestCaseFile(data) {
  const validate = ajv.compile(testCaseSchema);
  const valid = validate(data);
  
  if (!valid) {
    console.error('Validation errors:', validate.errors);
    return false;
  }
  
  return true;
}

// Validate test results
function validateTestResults(data) {
  const validate = ajv.compile(testResultSchema);
  const valid = validate(data);
  
  if (!valid) {
    console.error('Validation errors:', validate.errors);
    return false;
  }
  
  return true;
}
```

### C++ Validation
```cpp
#include <nlohmann/json.hpp>
#include <nlohmann/json-schema.hpp>

using json = nlohmann::json;
using nlohmann::json_schema::json_validator;

class SchemaValidator {
private:
    json_validator testCaseValidator;
    json_validator resultValidator;
    
public:
    SchemaValidator() {
        // Load schemas
        json testCaseSchema = loadSchema("test-case-file.json");
        json resultSchema = loadSchema("test-results.json");
        
        // Initialize validators
        testCaseValidator.set_root_schema(testCaseSchema);
        resultValidator.set_root_schema(resultSchema);
    }
    
    bool validateTestCase(const json& data) {
        try {
            testCaseValidator.validate(data);
            return true;
        } catch (const std::exception& e) {
            LOG_ERROR("Test case validation failed: " + std::string(e.what()));
            return false;
        }
    }
    
    bool validateResults(const json& data) {
        try {
            resultValidator.validate(data);
            return true;
        } catch (const std::exception& e) {
            LOG_ERROR("Result validation failed: " + std::string(e.what()));
            return false;
        }
    }
};
```

## Schema Evolution

### Version Migration
```json
{
  "$id": "https://pers-engine.dev/schemas/migration-1.0-to-2.0.json",
  "title": "Migration Schema",
  "description": "Migrates test data from version 1.0 to 2.0",
  "type": "object",
  "properties": {
    "migrations": {
      "type": "array",
      "items": {
        "type": "object",
        "properties": {
          "field": {"type": "string"},
          "action": {"enum": ["rename", "transform", "remove", "add"]},
          "from": {"type": "string"},
          "to": {"type": "string"},
          "transform": {"type": "string"},
          "default": {}
        }
      }
    }
  }
}
```

### Backward Compatibility Rules
1. **Never remove required fields** - Mark as deprecated instead
2. **Add new fields as optional** - Provide defaults
3. **Version all schemas** - Include version in $id
4. **Document breaking changes** - In migration guides
5. **Support multiple versions** - For gradual migration

## Custom Validation Rules

### Business Logic Validation
```javascript
// Beyond JSON Schema validation
function validateBusinessRules(testCase) {
  const errors = [];
  
  // Test IDs must be unique within file
  const ids = new Set();
  for (const variation of testCase.variations) {
    if (ids.has(variation.id)) {
      errors.push(`Duplicate test ID: ${variation.id}`);
    }
    ids.add(variation.id);
  }
  
  // Handler class must exist
  if (!isValidHandlerClass(testCase.handlerClass)) {
    errors.push(`Unknown handler class: ${testCase.handlerClass}`);
  }
  
  // Parameter compatibility
  for (const variation of testCase.variations) {
    const paramErrors = validateParameters(
      variation.options,
      testCase.handlerClass
    );
    errors.push(...paramErrors);
  }
  
  return errors;
}
```

### Cross-Reference Validation
```javascript
function validateCrossReferences(testFile, resultFile) {
  const errors = [];
  
  // All result IDs must exist in test definitions
  for (const result of resultFile.results) {
    const testDef = findTestById(testFile, result.id);
    if (!testDef) {
      errors.push(`Result references unknown test ID: ${result.id}`);
    }
  }
  
  // Category and test type must match
  for (const result of resultFile.results) {
    const testDef = findTestById(testFile, result.id);
    if (testDef) {
      if (result.category !== testDef.category) {
        errors.push(`Category mismatch for test ${result.id}`);
      }
      if (result.testType !== testDef.testType) {
        errors.push(`Test type mismatch for test ${result.id}`);
      }
    }
  }
  
  return errors;
}
```

## Schema Documentation Generator

### Generate HTML Documentation
```javascript
function generateSchemaDoc(schema) {
  const html = `
    <!DOCTYPE html>
    <html>
    <head>
      <title>${schema.title}</title>
      <style>
        .property { margin-left: 20px; }
        .required { color: red; }
        .type { color: blue; }
      </style>
    </head>
    <body>
      <h1>${schema.title}</h1>
      <p>${schema.description}</p>
      
      <h2>Properties</h2>
      ${generateProperties(schema.properties, schema.required)}
      
      <h2>Definitions</h2>
      ${generateDefinitions(schema.definitions)}
    </body>
    </html>
  `;
  
  return html;
}
```

## Usage in CI/CD

### GitHub Actions Example
```yaml
name: Validate Test Files

on:
  pull_request:
    paths:
      - 'tests/**/*.json'

jobs:
  validate:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      
      - name: Setup Node.js
        uses: actions/setup-node@v2
        with:
          node-version: '16'
          
      - name: Install dependencies
        run: npm install ajv ajv-cli
        
      - name: Validate test cases
        run: |
          for file in tests/test_cases/**/*.json; do
            npx ajv validate -s schemas/test-case-file.json -d "$file"
          done
          
      - name: Validate results
        run: |
          for file in tests/results/**/*.json; do
            npx ajv validate -s schemas/test-results.json -d "$file"
          done
```

## Schema Registry

### Central Schema Repository
```
schemas/
├── v1.0/
│   ├── test-case-file.json
│   └── test-results.json
├── v2.0/
│   ├── test-case-file.json
│   ├── test-results.json
│   └── migration-1.0-to-2.0.json
├── current -> v2.0/
└── README.md
```

### Schema Versioning Strategy
- **Major version**: Breaking changes
- **Minor version**: New optional fields
- **Patch version**: Documentation updates

## Tools and Resources

### Online Tools
- [JSON Schema Validator](https://www.jsonschemavalidator.net/)
- [JSON Schema Generator](https://jsonschema.net/)
- [Schema Store](https://www.schemastore.org/)

### Libraries
- **JavaScript**: ajv, json-schema
- **C++**: nlohmann/json-schema
- **Python**: jsonschema
- **Go**: gojsonschema

### IDE Support
- **VS Code**: JSON Schema extension
- **IntelliJ**: Built-in JSON Schema support
- **Sublime Text**: JSON Schema plugin