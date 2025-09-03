#include "../test_handler_base.h"
#include <pers/graphics/backends/webgpu/WebGPUBackendFactory.h>
#include <pers/graphics/IInstance.h>
#include <pers/graphics/IPhysicalDevice.h>
#include <pers/graphics/ILogicalDevice.h>
#include <pers/graphics/IResourceFactory.h>
#include <pers/graphics/IShaderModule.h>

namespace pers::tests {

class ShaderCompilationHandler : public TestHandlerBase {
private:
    std::shared_ptr<WebGPUBackendFactory> _factory;
    std::shared_ptr<ILogicalDevice> _device;
    std::shared_ptr<IResourceFactory> _resourceFactory;
    
    bool initializeDevice() {
        if (_device && _resourceFactory) {
            return true;
        }
        
        // Create instance
        InstanceDesc instanceDesc;
        instanceDesc.applicationName = "Shader Test";
        instanceDesc.enableValidation = false;
        auto instance = _factory->createInstance(instanceDesc);
        if (!instance) return false;
        
        // Get adapter
        PhysicalDeviceOptions adapterOptions;
        auto adapter = instance->requestPhysicalDevice(adapterOptions);
        if (!adapter) return false;
        
        // Create logical device
        LogicalDeviceDesc deviceDesc;
        deviceDesc.enableValidation = false;
        _device = adapter->createLogicalDevice(deviceDesc);
        if (!_device) return false;
        
        // Get resource factory
        _resourceFactory = _device->getResourceFactory();
        return _resourceFactory != nullptr;
    }
    
public:
    ShaderCompilationHandler() 
        : _factory(std::make_shared<WebGPUBackendFactory>()) {
    }
    
    std::string getTestType() const override {
        return "Shader Compilation";
    }
    
    TestResult execute(const TestVariation& variation) override {
        TestResult result;
        
        // Setup log capture
        setupLogCapture();
        
        // Initialize device if needed
        if (!initializeDevice()) {
            result.passed = false;
            result.actualBehavior = "Device initialization failed";
            result.failureReason = "Failed to initialize device for shader test";
            result.actualProperties["deviceInitialized"] = false;
            return result;
        }
        
        // Extract options
        std::string shaderCode = getOption<std::string>(variation.options, "shaderCode", "");
        std::string stage = getOption<std::string>(variation.options, "stage", "vertex");
        
        // Create shader module
        ShaderModuleDesc desc;
        desc.code = shaderCode;
        desc.debugName = "Test Shader";
        
        // Set shader stage
        if (stage == "vertex") {
            desc.stage = ShaderStage::Vertex;
        } else if (stage == "fragment") {
            desc.stage = ShaderStage::Fragment;
        } else if (stage == "compute") {
            desc.stage = ShaderStage::Compute;
        }
        
        auto shaderModule = _resourceFactory->createShaderModule(desc);
        
        // Transfer captured logs to result
        transferLogsToResult(result);
        
        // Check result
        bool compilationSuccess = (shaderModule != nullptr);
        bool expectedNotNull = (variation.expectedBehavior.returnValue == "not_null");
        
        // Populate actual properties
        result.actualProperties["compilationSuccess"] = compilationSuccess;
        result.actualProperties["shaderStage"] = stage;
        
        if (shaderModule) {
            result.actualProperties["returnValue"] = std::string("not_null");
            
            // Check expected entry point
            if (variation.expectedBehavior.properties.count("entryPoint")) {
                std::string expectedEntryPoint = std::any_cast<std::string>(
                    variation.expectedBehavior.properties.at("entryPoint"));
                result.actualProperties["entryPoint"] = expectedEntryPoint;
            }
        } else {
            result.actualProperties["returnValue"] = std::string("nullptr");
            
            // Check if error code matches
            if (!variation.expectedBehavior.errorCode.empty()) {
                if (shaderCode.empty()) {
                    result.actualProperties["errorCode"] = std::string("EMPTY_CODE");
                } else if (shaderCode.find("invalid") != std::string::npos) {
                    result.actualProperties["errorCode"] = std::string("SYNTAX_ERROR");
                } else {
                    result.actualProperties["errorCode"] = std::string("COMPILATION_ERROR");
                }
                
                // Check if error code matches expected
                std::string actualError = std::any_cast<std::string>(result.actualProperties["errorCode"]);
                if (actualError == variation.expectedBehavior.errorCode) {
                    // Error occurred as expected
                    compilationSuccess = false; // Expected to fail
                    expectedNotNull = false; // Expected nullptr
                }
            }
        }
        
        // Determine pass/fail
        bool returnValueMatches = (compilationSuccess == expectedNotNull);
        
        // Special handling for expected failures
        if (variation.expectedBehavior.properties.count("compilationSuccess")) {
            bool expectedSuccess = std::any_cast<bool>(
                variation.expectedBehavior.properties.at("compilationSuccess"));
            returnValueMatches = (compilationSuccess == expectedSuccess);
        }
        
        result.passed = returnValueMatches;
        
        if (result.passed) {
            if (compilationSuccess) {
                result.actualBehavior = "Shader compiled successfully";
            } else {
                result.actualBehavior = "Shader compilation failed as expected";
            }
        } else {
            result.actualBehavior = compilationSuccess 
                ? "Shader compiled unexpectedly"
                : "Failed to compile shader";
            result.failureReason = compilationSuccess
                ? "Expected compilation failure but succeeded"
                : "Expected successful compilation but failed";
        }
        
        return result;
    }
};

// Register the handler
REGISTER_TEST_HANDLER("Shader Compilation", ShaderCompilationHandler)

} // namespace pers::tests