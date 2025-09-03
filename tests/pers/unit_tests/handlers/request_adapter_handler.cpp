#include "../test_handler_base.h"
#include <pers/graphics/backends/webgpu/WebGPUBackendFactory.h>
#include <pers/graphics/IInstance.h>
#include <pers/graphics/IPhysicalDevice.h>
#include <pers/graphics/backends/IGraphicsBackendFactory.h>

namespace pers::tests {

class RequestAdapterHandler : public TestHandlerBase {
private:
    std::shared_ptr<WebGPUBackendFactory> _factory;
    std::shared_ptr<IInstance> _instance;
    
    bool initializeInstance() {
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
    
public:
    RequestAdapterHandler() 
        : _factory(std::make_shared<WebGPUBackendFactory>()) {
    }
    
    std::string getTestType() const override {
        return "Request Adapter";
    }
    
    TestResult execute(const TestVariation& variation) override {
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
        
        // Check expectations
        bool expectedFound = variation.expectedBehavior.properties.count("adapterFound")
            ? std::any_cast<bool>(variation.expectedBehavior.properties.at("adapterFound"))
            : true;
        
        bool expectedNotNull = (variation.expectedBehavior.returnValue == "not_null");
        
        // Validate properties if specified
        if (adapterFound && variation.expectedBehavior.properties.size() > 0) {
            // Check adapter type if specified
            if (variation.expectedBehavior.properties.count("adapterType")) {
                std::string expectedType = std::any_cast<std::string>(
                    variation.expectedBehavior.properties.at("adapterType"));
                
                if (expectedType == "DiscreteGPU") {
                    bool isDiscrete = std::any_cast<bool>(result.actualProperties["isDiscreteGPU"]);
                    if (!isDiscrete && preference == PowerPreference::HighPerformance) {
                        // High performance requested but didn't get discrete GPU
                        // This might still be OK on some systems
                        result.actualProperties["adapterTypeMatch"] = false;
                    } else {
                        result.actualProperties["adapterTypeMatch"] = true;
                    }
                }
            }
            
            // Check numeric conditions
            for (const auto& [property, condition] : variation.expectedBehavior.numericChecks) {
                if (property == "dedicatedVideoMemory") {
                    uint64_t videoMem = std::any_cast<uint64_t>(result.actualProperties["dedicatedVideoMemory"]);
                    if (!checkNumericCondition(condition, static_cast<double>(videoMem))) {
                        result.passed = false;
                        result.failureReason = "Video memory check failed: " + std::to_string(videoMem) + 
                                             " does not satisfy " + condition;
                        return result;
                    }
                }
            }
        }
        
        // Determine pass/fail
        result.passed = (adapterFound == expectedFound) && (adapterFound == expectedNotNull);
        
        if (result.passed) {
            result.actualBehavior = adapterFound 
                ? "Adapter found with requested preferences"
                : "No adapter found as expected";
        } else {
            result.actualBehavior = adapterFound
                ? "Adapter found unexpectedly"
                : "Failed to find adapter";
            if (result.failureReason.empty()) {
                result.failureReason = adapterFound
                    ? "Expected no adapter but found one"
                    : "Expected adapter but none found";
            }
        }
        
        return result;
    }
};

// Register the handler
REGISTER_TEST_HANDLER("Request Adapter", RequestAdapterHandler)

} // namespace pers::tests