#include "request_adapter_handler.h"
#include <pers/graphics/IPhysicalDevice.h>
#include <pers/graphics/backends/IGraphicsBackendFactory.h>

namespace pers::tests {

bool RequestAdapterHandler::initializeInstance() {
        if (_instance) {
            return true;
        }
        
        InstanceDesc desc;
        desc.applicationName = "Adapter Test";
        desc.engineName = "Pers Graphics Engine";
        desc.enableValidation = false;
        _instance = _factory->createInstance(desc);
        return _instance != nullptr;
    }

RequestAdapterHandler::RequestAdapterHandler() 
    : _factory(std::make_shared<WebGPUBackendFactory>()) {
}

std::string RequestAdapterHandler::getTestType() const {
    return "Request Adapter";
}

TestResult RequestAdapterHandler::execute(const TestVariation& variation) {
        TestResult result;
        
        // Setup log capture
        setupLogCapture();
        
        // Initialize instance if needed
        if (!initializeInstance()) {
            result.passed = false;
            result.actualBehavior = "Instance initialization failed";
            result.failureReason = "Failed to create instance for adapter test";
            result.actualProperties["instanceCreated"] = false;
            return result;
        }
        
        // Extract options
        std::string powerPref = getOption<std::string>(variation.options, "powerPreference", "Undefined");
        bool forceFallback = getOption<bool>(variation.options, "forceFallback", false);
        
        // Convert string to enum
        PowerPreference preference = PowerPreference::Default;
        if (powerPref == "HighPerformance") {
            preference = PowerPreference::HighPerformance;
        } else if (powerPref == "LowPower") {
            preference = PowerPreference::LowPower;
        }
        
        // Request adapter with options
        PhysicalDeviceOptions options;
        options.powerPreference = preference;
        options.forceFallbackAdapter = forceFallback;
        
        auto adapter = _instance->requestPhysicalDevice(options);
        bool adapterFound = (adapter != nullptr);
        
        // Transfer captured logs to result
        transferLogsToResult(result);
        
        // Populate actual properties
        result.actualProperties["adapterFound"] = adapterFound;
        result.actualProperties["adapterCount"] = adapterFound ? 1 : 0;
        result.actualProperties["powerPreference"] = powerPref;
        result.actualProperties["forceFallback"] = forceFallback;
        
        if (adapterFound && adapter) {
            auto capabilities = adapter->getCapabilities();
            
            result.actualProperties["deviceName"] = capabilities.deviceName;
            result.actualProperties["dedicatedVideoMemory"] = capabilities.dedicatedVideoMemory;
            result.actualProperties["supportsCompute"] = capabilities.supportsCompute;
            
            // Check if it's a discrete GPU (has dedicated video memory)
            bool isDiscrete = capabilities.dedicatedVideoMemory > 0;
            result.actualProperties["isDiscreteGPU"] = isDiscrete;
            
            // Set return value
            result.actualProperties["returnValue"] = std::string("not_null");
        } else {
            result.actualProperties["returnValue"] = std::string("nullptr");
        }
        
        // Check expected return value
        bool expectedNotNull = (variation.expectedBehavior.returnValue == "not_null");
        bool expectedNull = (variation.expectedBehavior.returnValue == "nullptr" || 
                            variation.expectedBehavior.returnValue == "null");
        
        // Determine pass/fail based on return value comparison
        if (expectedNotNull) {
            result.passed = adapterFound;
            result.actualBehavior = adapterFound ? "Adapter found" : "No adapter found";
            if (!result.passed) {
                result.failureReason = "Expected adapter but none found";
            }
        } else if (expectedNull) {
            result.passed = !adapterFound;
            result.actualBehavior = adapterFound ? "Adapter found" : "No adapter found";  
            if (!result.passed) {
                result.failureReason = "Expected no adapter but found one";
            }
        } else {
            // Unknown expected value
            result.passed = false;
            result.actualBehavior = "Unknown expected value: " + variation.expectedBehavior.returnValue;
            result.failureReason = "Test configuration error";
        }
        
        return result;
}

} // namespace pers::tests