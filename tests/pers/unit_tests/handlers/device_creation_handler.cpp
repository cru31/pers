#include "../test_handler_base.h"
#include <pers/graphics/backends/webgpu/WebGPUBackendFactory.h>
#include <pers/graphics/IInstance.h>
#include <pers/graphics/IPhysicalDevice.h>
#include <pers/graphics/ILogicalDevice.h>

namespace pers::tests {

class DeviceCreationHandler : public TestHandlerBase {
private:
    std::shared_ptr<WebGPUBackendFactory> _factory;
    std::shared_ptr<IInstance> _instance;
    std::shared_ptr<IPhysicalDevice> _adapter;
    
    bool initializeAdapter() {
        if (_adapter) {
            return true;
        }
        
        // Create instance
        if (!_instance) {
            InstanceDesc instanceDesc;
            instanceDesc.applicationName = "Device Test";
            instanceDesc.enableValidation = false;
            _instance = _factory->createInstance(instanceDesc);
            if (!_instance) return false;
        }
        
        // Get adapter
        PhysicalDeviceOptions options;
        _adapter = _instance->requestPhysicalDevice(options);
        return _adapter != nullptr;
    }
    
public:
    DeviceCreationHandler() 
        : _factory(std::make_shared<WebGPUBackendFactory>()) {
    }
    
    std::string getTestType() const override {
        return "Device Creation";
    }
    
    TestResult execute(const TestVariation& variation) override {
        TestResult result;
        
        // Setup log capture
        setupLogCapture();
        
        // Initialize adapter if needed
        if (!initializeAdapter()) {
            result.passed = false;
            result.actualBehavior = "Adapter initialization failed";
            result.failureReason = "Failed to get adapter for device creation";
            result.actualProperties["adapterInitialized"] = false;
            return result;
        }
        
        // Extract options
        bool enableValidation = getOption<bool>(variation.options, "enableValidation", false);
        std::string debugName = getOption<std::string>(variation.options, "debugName", "");
        
        // Handle special case
        if (debugName == "[LONG_STRING_1024]") {
            debugName = std::string(1024, 'A');
        }
        
        // Create device
        LogicalDeviceDesc desc;
        desc.enableValidation = enableValidation;
        desc.debugName = debugName;
        
        auto device = _adapter->createLogicalDevice(desc);
        
        // Transfer captured logs to result
        transferLogsToResult(result);
        
        // Check result
        bool deviceCreated = (device != nullptr);
        bool expectedNotNull = (variation.expectedBehavior.returnValue == "not_null");
        
        // Populate actual properties
        result.actualProperties["deviceCreated"] = deviceCreated;
        result.actualProperties["validationEnabled"] = enableValidation;
        
        if (device) {
            result.actualProperties["returnValue"] = std::string("not_null");
            
            // Check if resource factory is available
            auto resourceFactory = device->getResourceFactory();
            result.actualProperties["resourceFactoryAvailable"] = (resourceFactory != nullptr);
            
            // Check if queue is available
            auto queue = device->getQueue();
            result.actualProperties["queueAvailable"] = (queue != nullptr);
        } else {
            result.actualProperties["returnValue"] = std::string("nullptr");
        }
        
        // Check expected properties
        if (variation.expectedBehavior.properties.count("validationEnabled")) {
            bool expectedValidation = std::any_cast<bool>(
                variation.expectedBehavior.properties.at("validationEnabled"));
            if (deviceCreated && expectedValidation != enableValidation) {
                result.passed = false;
                result.failureReason = "Validation state mismatch";
                return result;
            }
        }
        
        // Determine pass/fail
        result.passed = (deviceCreated == expectedNotNull);
        
        if (result.passed) {
            result.actualBehavior = deviceCreated 
                ? "Device created successfully"
                : "Device creation failed as expected";
        } else {
            result.actualBehavior = deviceCreated 
                ? "Device created unexpectedly"
                : "Failed to create device";
            if (result.failureReason.empty()) {
                result.failureReason = deviceCreated
                    ? "Expected nullptr but got valid device"
                    : "Expected valid device but got nullptr";
            }
        }
        
        return result;
    }
};

// Register the handler
REGISTER_TEST_HANDLER("Device Creation", DeviceCreationHandler)

} // namespace pers::tests