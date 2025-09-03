#include "../test_handler_base.h"
#include <pers/graphics/backends/webgpu/WebGPUBackendFactory.h>
#include <pers/graphics/IInstance.h>
#include <pers/graphics/backends/IGraphicsBackendFactory.h>
#include <iostream>

namespace pers::tests {

class InstanceCreationHandler : public TestHandlerBase {
private:
    std::shared_ptr<WebGPUBackendFactory> _factory;
    
public:
    InstanceCreationHandler() 
        : _factory(std::make_shared<WebGPUBackendFactory>()) {
    }
    
    std::string getTestType() const override {
        return "Instance Creation";
    }
    
    TestResult execute(const TestVariation& variation) override {
        TestResult result;
        
        // Setup log capture
        setupLogCapture();
        
        // Extract options from variation
        bool enableValidation = getOption<bool>(variation.options, "enableValidation", true);
        std::string appName = getOption<std::string>(variation.options, "applicationName", "");
        std::string engineName = getOption<std::string>(variation.options, "engineName", "");
        
        // Handle special cases
        if (appName == "[LONG_STRING_1024]") {
            appName = std::string(1024, 'A');
        }
        
        // Create instance with variation options
        InstanceDesc desc;
        desc.enableValidation = enableValidation;
        desc.applicationName = appName;
        desc.engineName = engineName;
        
        auto instance = _factory->createInstance(desc);
        
        // Transfer captured logs to result
        transferLogsToResult(result);
        
        // Check return value expectation
        bool instanceCreated = (instance != nullptr);
        bool expectedNotNull = (variation.expectedBehavior.returnValue == "not_null");
        
        // Populate actual properties
        result.actualProperties["instanceCreated"] = instanceCreated;
        result.actualProperties["validationEnabled"] = enableValidation;
        
        // Determine pass/fail
        if (instanceCreated == expectedNotNull) {
            result.passed = true;
            result.actualBehavior = instanceCreated ? "Instance created" : "Instance not created (as expected)";
        } else {
            result.passed = false;
            result.actualBehavior = instanceCreated ? "Instance created unexpectedly" : "Failed to create instance";
            result.failureReason = instanceCreated 
                ? "Expected nullptr but got valid instance"
                : "Expected valid instance but got nullptr";
        }
        
        // Check additional properties if instance was created
        if (instance && variation.expectedBehavior.properties.size() > 0) {
            // Could check additional properties here if IInstance exposed them
            auto validationLayersActive = getOption<bool>(variation.expectedBehavior.properties, 
                                                          "validationLayersActive", false);
            if (validationLayersActive && !enableValidation) {
                result.passed = false;
                result.failureReason = "Validation layers expected but not enabled";
            }
        }
        
        return result;
    }
};

// Register the handler
REGISTER_TEST_HANDLER("Instance Creation", InstanceCreationHandler)

} // namespace pers::tests