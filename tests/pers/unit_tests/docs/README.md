# Pers Graphics Engine - Unit Test Framework Documentation

This documentation covers the comprehensive unit testing framework developed for the Pers Graphics Engine, including the JSON-driven test execution system and web-based result viewer.

## Documentation Structure

- **[design/](design/)** - Architecture and design documentation
  - [architecture.md](design/architecture.md) - Overall system architecture
  - [test-framework.md](design/test-framework.md) - Test framework design
  - [log-capture.md](design/log-capture.md) - Log capture mechanism
  
- **[api/](api/)** - API reference documentation
  - [test-runner.md](api/test-runner.md) - Test runner API
  - [result-writer.md](api/result-writer.md) - Result writer API
  - [web-api.md](api/web-api.md) - Web viewer API endpoints
  
- **[web/](web/)** - Web viewer documentation
  - [frontend.md](web/frontend.md) - Frontend implementation
  - [backend.md](web/backend.md) - Backend server implementation
  - [features.md](web/features.md) - Feature specifications

## Quick Links

- [Getting Started](design/architecture.md#getting-started)
- [Test Case Format](design/test-framework.md#test-case-format)
- [Web Viewer Usage](web/frontend.md#usage)
- [API Reference](api/test-runner.md)

## Overview

The Pers unit test framework is a comprehensive testing solution that provides:

1. **JSON-Driven Test Execution** - Define test cases in JSON format for easy maintenance
2. **Per-Test Log Capture** - Capture and associate engine logs with individual tests
3. **Web-Based Result Viewer** - Interactive web interface for analyzing test results
4. **Multi-Status Support** - Tests can have multiple statuses (PASS, FAIL, N/A, NYI)
5. **VSCode Integration** - Click to open source files directly in VSCode

## Key Features

- Thread-safe log capture during test execution
- Real-time test result visualization
- Clickable status filters
- Expandable test details with log messages
- Local JSON file loading support
- Session-based result storage
- Automatic server management

## Technology Stack

- **Backend**: C++17 with WebGPU/Dawn
- **Frontend**: Node.js, Express, Vanilla JavaScript
- **Data Format**: JSON
- **Logging**: Custom thread-safe logger with callbacks