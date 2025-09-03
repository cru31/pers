#include "../test_handler_base.h"
#include <pers/graphics/backends/webgpu/WebGPUBackendFactory.h>
#include <pers/graphics/IInstance.h>
#include <pers/graphics/IPhysicalDevice.h>
#include <pers/graphics/ILogicalDevice.h>
#include <pers/graphics/IResourceFactory.h>
#include <pers/graphics/IBuffer.h>

namespace pers::tests {

class BufferCreationHandler : public TestHandlerBase {
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
        instanceDesc.applicationName = "Buffer Test";
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
    BufferCreationHandler() 
        : _factory(std::make_shared<WebGPUBackendFactory>()) {
    }
    
    std::string getTestType() const override {
        return "Buffer Creation";
    }
    
    TestResult execute(const TestVariation& variation) override {
        TestResult result;
        
        // Setup log capture
        setupLogCapture();
        
        // Initialize device if needed
        if (!initializeDevice()) {
            result.passed = false;
            result.actualBehavior = "Device initialization failed";
            result.failureReason = "Failed to initialize device for buffer test";
            result.actualProperties["deviceInitialized"] = false;
            return result;
        }
        
        // Extract options
        size_t size = getOption<size_t>(variation.options, "size", 0);
        std::string usageStr = getOption<std::string>(variation.options, "usage", "Vertex");
        bool mappedAtCreation = getOption<bool>(variation.options, "mappedAtCreation", false);
        
        // Convert usage string to enum
        BufferUsage usage = BufferUsage::None;
        if (usageStr == "Vertex") {
            usage = BufferUsage::Vertex;
        } else if (usageStr == "Index") {
            usage = BufferUsage::Index;
        } else if (usageStr == "Uniform") {
            usage = BufferUsage::Uniform;
        } else if (usageStr == "Storage") {
            usage = BufferUsage::Storage;
        } else if (usageStr == "CopySrc") {
            usage = BufferUsage::CopySrc;
        } else if (usageStr == "CopyDst") {
            usage = BufferUsage::CopyDst;
        } else if (usageStr == "MapWrite") {
            usage = static_cast<BufferUsage>(
                static_cast<uint32_t>(BufferUsage::MapWrite) | 
                static_cast<uint32_t>(BufferUsage::CopySrc));
        } else if (usageStr == "MapRead") {
            usage = static_cast<BufferUsage>(
                static_cast<uint32_t>(BufferUsage::MapRead) | 
                static_cast<uint32_t>(BufferUsage::CopyDst));
        }
        
        // Create buffer
        BufferDesc desc;
        desc.size = size;
        desc.usage = usage;
        desc.mappedAtCreation = mappedAtCreation;
        
        auto buffer = _resourceFactory->createBuffer(desc);
        
        // Transfer captured logs to result
        transferLogsToResult(result);
        
        // Check expectations
        bool bufferCreated = (buffer != nullptr);
        bool expectedCreated = variation.expectedBehavior.properties.count("bufferCreated") 
            ? std::any_cast<bool>(variation.expectedBehavior.properties.at("bufferCreated"))
            : true;
        
        // For return value check
        bool expectedNotNull = (variation.expectedBehavior.returnValue == "not_null");
        bool expectedNull = (variation.expectedBehavior.returnValue == "nullptr");
        
        // Populate actual properties
        result.actualProperties["bufferCreated"] = bufferCreated;
        result.actualProperties["requestedSize"] = size;
        result.actualProperties["usage"] = static_cast<uint32_t>(usage);
        result.actualProperties["mappedAtCreation"] = mappedAtCreation;
        
        if (buffer) {
            // Get actual buffer properties
            result.actualProperties["actualSize"] = buffer->getSize();
            result.actualProperties["returnValue"] = std::string("not_null");
            
            // Check mapped pointer if mapped at creation
            if (mappedAtCreation) {
                void* mappedData = buffer->map();
                result.actualProperties["mappedPointer"] = mappedData ? std::string("not_null") : std::string("nullptr");
                if (mappedData) {
                    buffer->unmap();
                }
            }
        } else {
            result.actualProperties["returnValue"] = std::string("nullptr");
        }
        
        // Determine pass/fail based on expectations
        bool returnValueMatches = true;
        if (expectedNull && bufferCreated) {
            returnValueMatches = false;
            result.failureReason = "Expected nullptr but got valid buffer";
        } else if (expectedNotNull && !bufferCreated) {
            returnValueMatches = false;
            result.failureReason = "Expected valid buffer but got nullptr";
        }
        
        // Check numeric conditions
        if (buffer && variation.expectedBehavior.numericChecks.size() > 0) {
            for (const auto& [property, condition] : variation.expectedBehavior.numericChecks) {
                if (property == "actualSize") {
                    size_t actualSize = buffer->getSize();
                    if (!checkNumericCondition(condition, static_cast<double>(actualSize))) {
                        result.passed = false;
                        result.failureReason = "Size check failed: " + std::to_string(actualSize) + 
                                             " does not satisfy " + condition;
                        return result;
                    }
                }
            }
        }
        
        // Special case: zero size should fail
        if (size == 0) {
            if (!bufferCreated) {
                result.passed = true;
                result.actualBehavior = "Buffer creation failed as expected for zero size";
            } else {
                result.passed = false;
                result.actualBehavior = "Buffer created with zero size";
                result.failureReason = "Zero size buffer should fail";
            }
        } else {
            // Normal cases
            result.passed = returnValueMatches && (bufferCreated == expectedCreated);
            if (result.passed) {
                result.actualBehavior = bufferCreated ? "Buffer created successfully" : "Buffer creation failed as expected";
            } else {
                result.actualBehavior = bufferCreated ? "Buffer created" : "Buffer creation failed";
                if (result.failureReason.empty()) {
                    result.failureReason = "Buffer creation result doesn't match expectation";
                }
            }
        }
        
        return result;
    }
};

// Register the handler
REGISTER_TEST_HANDLER("Buffer Creation", BufferCreationHandler)

} // namespace pers::tests