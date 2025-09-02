# TODO Tracking System

> **Status**: Stable  
> **Since**: v1.2.0  
> **Backend**: All

## Overview

The TODO tracking system provides a structured way to mark unimplemented features and track technical debt in the codebase. It integrates with the logging system to provide visibility into what needs to be implemented and the priority of each TODO item.

## Table of Contents
- [Key Concepts](#key-concepts)
- [API Reference](#api-reference)
- [Usage](#usage)
- [Best Practices](#best-practices)
- [Log Level Integration](#log-level-integration)
- [See Also](#see-also)

## Key Concepts

### TODO Categories
The system provides two categories of TODOs:

1. **TodoOrDie**: Critical functionality that MUST be implemented
   - Blocks core features
   - Causes runtime failures if not implemented
   - Logged at `LogLevel::TodoOrDie` (level 5)
   - Displayed as `[TODO!]` in logs

2. **TodoSomeday**: Nice-to-have improvements
   - Performance optimizations
   - Additional features
   - API refinements waiting for stabilization
   - Logged at `LogLevel::TodoSomeday` (level 3)
   - Displayed as `[TODO ]` in logs

### Log Level Hierarchy
```cpp
enum class LogLevel {
    Trace = 0,
    Debug = 1,
    Info = 2,
    TodoSomeday = 3,  // Non-critical future improvements
    Warning = 4,
    TodoOrDie = 5,    // Critical TODOs that must be implemented
    Error = 6,
    Critical = 7
};
```

## API Reference

### TodoOrDie

#### Header
`pers/utils/TodoOrDie.h`

#### `Log` Method
```cpp
static void Log(const std::string& functionName, 
                const std::string& todoDescription, 
                const LogSource& source);
```

**Parameters:**
- `functionName`: Name of the function that needs implementation
- `todoDescription`: Description of what needs to be implemented
- `source`: Source location information (use `PERS_SOURCE_LOC` macro)

**Example:**
```cpp
std::shared_ptr<ICommandEncoder> WebGPULogicalDevice::createCommandEncoder() {
    TodoOrDie::Log(
        "WebGPULogicalDevice::createCommandEncoder",
        "Create WebGPUCommandEncoder from device",
        PERS_SOURCE_LOC
    );
    
    // TODO: Implementation steps:
    // 1. Create WGPUCommandEncoderDescriptor
    // 2. Call wgpuDeviceCreateCommandEncoder
    // 3. Wrap in WebGPUCommandEncoder class
    // 4. Return shared_ptr
    
    return nullptr;
}
```

### TodoSomeday

#### Header
`pers/utils/TodoSomeday.h`

#### `Log` Method
```cpp
static void Log(const std::string& context,
                const std::string& description,
                const LogSource& source);
```

**Parameters:**
- `context`: Context or component name
- `description`: Description of the future improvement
- `source`: Source location information (use `PERS_SOURCE_LOC` macro)

**Example:**
```cpp
// In querySurfaceCapabilities
TodoSomeday::Log("WebGPUSwapChain", 
    "Using default texture limits (8192x8192). "
    "Actual adapter limits will be queried when API stabilizes.", 
    PERS_SOURCE_LOC);

// Use conservative defaults for now
caps.maxWidth = 8192;
caps.maxHeight = 8192;
```

## Usage

### When to Use TodoOrDie
Use `TodoOrDie` for:
- Core API functions that return nullptr/empty
- Essential features blocking other development
- Functions that will cause crashes if not implemented
- Platform-specific implementations that are mandatory

```cpp
std::shared_ptr<IResourceFactory> WebGPULogicalDevice::getResourceFactory() const {
    TodoOrDie::Log(
        "WebGPULogicalDevice::getResourceFactory",
        "Create WebGPUResourceFactory for buffer/texture/shader creation",
        PERS_SOURCE_LOC
    );
    
    // This MUST be implemented for the engine to function
    return nullptr;
}
```

### When to Use TodoSomeday
Use `TodoSomeday` for:
- Performance optimizations
- Additional features that aren't critical
- Workarounds waiting for API stabilization
- Platform-specific optimizations

```cpp
void WebGPUDevice::enableAdvancedFeatures() {
    // Basic features enabled...
    
    TodoSomeday::Log("WebGPUDevice", 
        "Enable timestamp queries when wgpu-native stabilizes the API", 
        PERS_SOURCE_LOC);
    
    // Continue with current functionality
}
```

## Best Practices

### 1. Provide Clear Descriptions
```cpp
// Good: Specific and actionable
TodoOrDie::Log(
    "ResourceManager::createBuffer",
    "Implement buffer creation with usage flags and initial data support",
    PERS_SOURCE_LOC
);

// Bad: Vague
TodoOrDie::Log(
    "ResourceManager::createBuffer",
    "Implement this",
    PERS_SOURCE_LOC
);
```

### 2. Include Implementation Steps
```cpp
TodoOrDie::Log(
    "WebGPUCommandEncoder::beginRenderPass",
    "Implement render pass creation",
    PERS_SOURCE_LOC
);

// TODO: Implementation steps:
// 1. Validate render pass descriptor
// 2. Create WGPURenderPassDescriptor
// 3. Convert color attachments
// 4. Handle depth-stencil attachment
// 5. Call wgpuCommandEncoderBeginRenderPass
// 6. Wrap in WebGPURenderPassEncoder
```

### 3. Choose Appropriate Category
- If the application cannot function without it → `TodoOrDie`
- If it's an enhancement or optimization → `TodoSomeday`
- If you're unsure → `TodoOrDie` (better to over-prioritize)

### 4. Filter TODO Logs in Production
```cpp
// Disable TodoSomeday logs in release builds
#ifdef NDEBUG
    Logger::Instance().SetLogLevelEnabled(LogLevel::TodoSomeday, false);
#endif

// Keep TodoOrDie visible even in production to track critical gaps
```

## Log Level Integration

### Filtering TODO Logs
```cpp
// Show only critical TODOs
Logger::Instance().SetMinLevel(LogLevel::TodoOrDie);

// Hide all TODO logs
Logger::Instance().SetLogLevelEnabled(LogLevel::TodoOrDie, false);
Logger::Instance().SetLogLevelEnabled(LogLevel::TodoSomeday, false);

// Show only TODO logs (useful for implementation planning)
Logger::Instance().SetMinLevel(LogLevel::TodoSomeday);
Logger::Instance().SetLogLevelEnabled(LogLevel::Info, false);
Logger::Instance().SetLogLevelEnabled(LogLevel::Debug, false);
Logger::Instance().SetLogLevelEnabled(LogLevel::Trace, false);
```

### Log Output Format
TODOs appear in logs with distinctive markers:
```
[14:23:45] [TODO!] [TodoOrDie] [device.cpp:142] Function must be implemented: createCommandEncoder - TODO: Create WebGPUCommandEncoder from device
[14:23:46] [TODO ] [WebGPUSwapChain] [swap_chain.cpp:215] Using default texture limits (8192x8192). Actual adapter limits will be queried when API stabilizes.
```

### Console Output Behavior
- `TodoOrDie` logs output to `stderr` (like errors)
- `TodoSomeday` logs output to `stdout` (like info)

## Migration from NotImplemented

If migrating from the old `NotImplemented` system:

```cpp
// Old approach
void someFunction() {
    NotImplemented::Log("someFunction", "Need to implement");
}

// New approach - evaluate criticality
void someFunction() {
    // If critical:
    TodoOrDie::Log("someFunction", "Need to implement");
    
    // If nice-to-have:
    TodoSomeday::Log("Component", "Implement someFunction for better performance");
}
```

## Version History

| Version | Changes | Migration Notes |
|---------|---------|-----------------|
| v1.0.0 | NotImplemented utility | Legacy approach |
| v1.1.0 | Renamed to TodoOrDie | Simple rename |
| v1.2.0 | Added TodoSomeday and dedicated log levels | Evaluate TODO criticality |

## See Also
- [Logger](logger.md)
- [Error Handling](../../guides/error-handling.md)
- [Development Workflow](../../guides/development-workflow.md)