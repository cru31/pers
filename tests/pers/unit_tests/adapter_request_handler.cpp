#include "adapter_request_handler.h"
#include "pers/graphics/backends/webgpu/WebGPUBackendFactory.h"
#include "pers/graphics/IInstance.h"

namespace pers::tests::json {

std::string AdapterRequestHandler::getTestType() const {
    return "Adapter Request";
}

bool AdapterRequestHandler::execute(const JsonTestCase& testCase, 
                                   std::string& actualResult, 
                                   std::string& failureReason) {
    auto factory = std::make_shared<WebGPUBackendFactory>();
    auto instance = factory->createInstance({});
    
    if (!instance) {
        actualResult = "Instance creation failed";
        failureReason = "Failed to create instance for adapter request";
        return false;
    }
    
    PhysicalDeviceOptions pdOptions;
    
    // Check if this is an option-based test
    if (testCase.inputValues.find("type") != testCase.inputValues.end() &&
        testCase.inputValues.at("type") == "OptionBased") {
        const auto& options = testCase.inputValues;
        
        // Parse power preference
        if (options.find("power_preference") != options.end()) {
            const std::string& pref = options.at("power_preference");
            if (pref == "LowPower") {
                pdOptions.powerPreference = PowerPreference::LowPower;
            } else if (pref == "HighPerformance") {
                pdOptions.powerPreference = PowerPreference::HighPerformance;
            } else {
                pdOptions.powerPreference = PowerPreference::Default;
            }
        }
        
        // Parse force fallback
        if (options.find("force_fallback") != options.end()) {
            pdOptions.forceFallbackAdapter = (options.at("force_fallback") == "true");
        }
    } else {
        // Regular test
        pdOptions.powerPreference = PowerPreference::HighPerformance;
    }
    
    auto physicalDevice = instance->requestPhysicalDevice(pdOptions);
    
    if (physicalDevice) {
        actualResult = "At least 1 adapter found";
        return testCase.expectedResult == actualResult || 
               testCase.expectedResult == "Success with options";
    } else {
        actualResult = "No adapter found";
        failureReason = "Failed to request physical device";
        return false;
    }
}

} // namespace pers::tests::json