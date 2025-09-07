# Handler Class Implementation Guide

## Overview
This guide explains how to implement handler classes in C++ that process test cases defined in JSON files. Each handler class is responsible for executing a specific type of test with various parameter configurations.

## Handler Class Structure

### Base Class Interface
All handlers must inherit from the base `ITestHandler` interface:

```cpp
class ITestHandler {
public:
    virtual ~ITestHandler() = default;
    
    // Initialize handler with test configuration
    virtual bool initialize(const TestConfig& config) = 0;
    
    // Execute test with given parameters
    virtual TestResult execute(const json& options) = 0;
    
    // Clean up after test
    virtual void cleanup() = 0;
    
    // Get handler name for registration
    virtual std::string getName() const = 0;
};
```

### Basic Handler Implementation

```cpp
class InstanceCreationHandler : public ITestHandler {
private:
    std::shared_ptr<IInstance> _instance;
    TestConfig _config;
    
public:
    bool initialize(const TestConfig& config) override {
        _config = config;
        return true;
    }
    
    TestResult execute(const json& options) override {
        TestResult result;
        
        // Parse options
        bool debugEnabled = options.value("debugEnabled", false);
        bool validationEnabled = options.value("validationEnabled", false);
        
        // Create instance with options
        InstanceCreateInfo createInfo{};
        createInfo.enableDebug = debugEnabled;
        createInfo.enableValidation = validationEnabled;
        
        _instance = IInstance::create(createInfo);
        
        // Check results
        if (_instance) {
            result.returnValue = "not_null";
            result.properties["isValid"] = _instance->isValid();
            result.properties["adapterCount"] = _instance->getAdapterCount();
            
            if (debugEnabled) {
                result.properties["debugLayerActive"] = _instance->isDebugLayerActive();
            }
        } else {
            result.returnValue = "null";
            result.success = false;
        }
        
        return result;
    }
    
    void cleanup() override {
        _instance.reset();
    }
    
    std::string getName() const override {
        return "InstanceCreationHandler";
    }
};
```

## TestResult Structure

```cpp
struct TestResult {
    bool success = true;
    std::string returnValue;
    std::map<std::string, std::any> properties;
    std::string errorMessage;
    int errorCode = 0;
    double executionTimeMs = 0.0;
};
```

## Parameter Parsing

### Basic Types
```cpp
// Boolean
bool debugEnabled = options.value("debugEnabled", false);

// Integer
int width = options.value("width", 1920);

// String
std::string format = options.value("format", "bgra8unorm");

// Float
float scale = options.value("scale", 1.0f);
```

### Arrays
```cpp
// String array
std::vector<std::string> extensions;
if (options.contains("extensions") && options["extensions"].is_array()) {
    for (const auto& ext : options["extensions"]) {
        extensions.push_back(ext.get<std::string>());
    }
}

// Numeric array
std::vector<int> sizes;
if (options.contains("sizes")) {
    sizes = options["sizes"].get<std::vector<int>>();
}
```

### Nested Objects
```cpp
// Object parameter
if (options.contains("limits")) {
    const auto& limits = options["limits"];
    int maxTextureSize = limits.value("maxTextureSize", 2048);
    int maxBindGroups = limits.value("maxBindGroups", 4);
}
```

## Handler Registration

### Registration System
```cpp
class HandlerRegistry {
private:
    static std::map<std::string, std::unique_ptr<ITestHandler>> handlers;
    
public:
    template<typename T>
    static void registerHandler() {
        auto handler = std::make_unique<T>();
        handlers[handler->getName()] = std::move(handler);
    }
    
    static ITestHandler* getHandler(const std::string& name) {
        auto it = handlers.find(name);
        return it != handlers.end() ? it->second.get() : nullptr;
    }
};

// Registration macro
#define REGISTER_HANDLER(HandlerClass) \
    static struct HandlerClass##_Registrar { \
        HandlerClass##_Registrar() { \
            HandlerRegistry::registerHandler<HandlerClass>(); \
        } \
    } HandlerClass##_registrar;
```

### Auto-Registration
```cpp
// In handler implementation file
REGISTER_HANDLER(InstanceCreationHandler)
REGISTER_HANDLER(DeviceCreationHandler)
REGISTER_HANDLER(BufferOperationsHandler)
```

## Advanced Handler Features

### Setup and Teardown
```cpp
class BufferOperationsHandler : public ITestHandler {
private:
    std::shared_ptr<IDevice> _device;
    std::vector<std::shared_ptr<IBuffer>> _buffers;
    
public:
    bool initialize(const TestConfig& config) override {
        // Create device for buffer tests
        _device = createTestDevice(config);
        return _device != nullptr;
    }
    
    TestResult execute(const json& options) override {
        TestResult result;
        
        // Setup phase
        if (!setupTestEnvironment(options)) {
            result.success = false;
            result.errorMessage = "Failed to setup test environment";
            return result;
        }
        
        // Test execution
        result = performBufferTest(options);
        
        // Teardown phase
        teardownTestEnvironment();
        
        return result;
    }
    
private:
    bool setupTestEnvironment(const json& options) {
        // Pre-test setup
        return true;
    }
    
    void teardownTestEnvironment() {
        // Post-test cleanup
        _buffers.clear();
    }
};
```

### Error Handling
```cpp
TestResult execute(const json& options) override {
    TestResult result;
    
    try {
        // Validate required parameters
        if (!options.contains("size")) {
            throw std::invalid_argument("Missing required parameter: size");
        }
        
        size_t bufferSize = options["size"].get<size_t>();
        if (bufferSize == 0 || bufferSize > MAX_BUFFER_SIZE) {
            throw std::out_of_range("Invalid buffer size");
        }
        
        // Execute test
        auto buffer = createBuffer(bufferSize);
        result.returnValue = buffer ? "not_null" : "null";
        
    } catch (const std::exception& e) {
        result.success = false;
        result.returnValue = "error";
        result.errorMessage = e.what();
        result.errorCode = -1;
    }
    
    return result;
}
```

### Performance Measurement
```cpp
TestResult execute(const json& options) override {
    TestResult result;
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Execute test operation
    performOperation(options);
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        endTime - startTime);
    
    result.executionTimeMs = duration.count() / 1000.0;
    result.properties["performanceMs"] = result.executionTimeMs;
    
    return result;
}
```

## Complex Handler Example

```cpp
class PipelineCreationHandler : public ITestHandler {
private:
    std::shared_ptr<IDevice> _device;
    std::shared_ptr<IShaderModule> _vertexShader;
    std::shared_ptr<IShaderModule> _fragmentShader;
    std::shared_ptr<IPipeline> _pipeline;
    
public:
    bool initialize(const TestConfig& config) override {
        _device = createTestDevice(config);
        if (!_device) return false;
        
        // Load default shaders
        _vertexShader = loadShader("default.vert");
        _fragmentShader = loadShader("default.frag");
        
        return _vertexShader && _fragmentShader;
    }
    
    TestResult execute(const json& options) override {
        TestResult result;
        
        // Parse pipeline configuration
        PipelineCreateInfo info{};
        
        // Vertex layout
        if (options.contains("vertexLayout")) {
            parseVertexLayout(options["vertexLayout"], info.vertexLayout);
        }
        
        // Blend state
        if (options.contains("blendState")) {
            parseBlendState(options["blendState"], info.blendState);
        }
        
        // Depth stencil state
        if (options.contains("depthStencil")) {
            parseDepthStencilState(options["depthStencil"], info.depthStencilState);
        }
        
        // Create pipeline
        _pipeline = _device->createPipeline(info);
        
        // Validate results
        if (_pipeline) {
            result.returnValue = "not_null";
            result.properties["isValid"] = _pipeline->isValid();
            result.properties["bindGroupCount"] = _pipeline->getBindGroupLayoutCount();
            
            // Test compilation if requested
            if (options.value("testCompilation", false)) {
                bool compiled = _pipeline->compile();
                result.properties["compiled"] = compiled;
            }
        } else {
            result.returnValue = "null";
            result.success = false;
            result.errorMessage = _device->getLastError();
        }
        
        return result;
    }
    
    void cleanup() override {
        _pipeline.reset();
        _fragmentShader.reset();
        _vertexShader.reset();
        _device.reset();
    }
    
    std::string getName() const override {
        return "PipelineCreationHandler";
    }
    
private:
    void parseVertexLayout(const json& layout, VertexLayout& out) {
        // Parse vertex attributes
        for (const auto& attr : layout["attributes"]) {
            VertexAttribute attribute;
            attribute.location = attr["location"];
            attribute.format = parseFormat(attr["format"]);
            attribute.offset = attr["offset"];
            out.attributes.push_back(attribute);
        }
        out.stride = layout.value("stride", 0);
    }
    
    void parseBlendState(const json& blend, BlendState& out) {
        out.enabled = blend.value("enabled", false);
        if (out.enabled) {
            out.srcFactor = parseBlendFactor(blend.value("srcFactor", "one"));
            out.dstFactor = parseBlendFactor(blend.value("dstFactor", "zero"));
            out.operation = parseBlendOp(blend.value("operation", "add"));
        }
    }
    
    void parseDepthStencilState(const json& ds, DepthStencilState& out) {
        out.depthTestEnabled = ds.value("depthTest", true);
        out.depthWriteEnabled = ds.value("depthWrite", true);
        out.depthCompareOp = parseCompareOp(ds.value("depthCompare", "less"));
    }
};
```

## Best Practices

1. **Parameter Validation**: Always validate required parameters before use
2. **Error Handling**: Use try-catch blocks for robust error handling
3. **Resource Management**: Clean up resources properly in cleanup()
4. **RAII Pattern**: Use RAII for automatic resource management
5. **Logging**: Log important steps for debugging
6. **Performance**: Measure execution time for performance-critical tests
7. **Thread Safety**: Ensure handlers are thread-safe if tests run in parallel

## Testing Handler Implementation

```cpp
// Unit test for handler
TEST(HandlerTest, InstanceCreationHandler) {
    InstanceCreationHandler handler;
    
    TestConfig config;
    ASSERT_TRUE(handler.initialize(config));
    
    json options;
    options["debugEnabled"] = true;
    options["validationEnabled"] = false;
    
    TestResult result = handler.execute(options);
    
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.returnValue, "not_null");
    EXPECT_TRUE(result.properties["isValid"].get<bool>());
    
    handler.cleanup();
}
```

## Integration with Test Runner

```cpp
class TestRunner {
public:
    void runTest(const json& testCase) {
        // Get handler for test type
        std::string handlerName = testCase["handlerClass"];
        ITestHandler* handler = HandlerRegistry::getHandler(handlerName);
        
        if (!handler) {
            logError("Handler not found: " + handlerName);
            return;
        }
        
        // Initialize handler
        TestConfig config;
        if (!handler->initialize(config)) {
            logError("Handler initialization failed");
            return;
        }
        
        // Run each variation
        for (const auto& variation : testCase["variations"]) {
            TestResult result = handler->execute(variation["options"]);
            
            // Validate against expected behavior
            validateResult(result, variation["expectedBehavior"]);
            
            // Log results
            logTestResult(variation["variationName"], result);
        }
        
        // Cleanup
        handler->cleanup();
    }
};
```