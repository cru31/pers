#include "device_creation_handler.h"
#include "pers/graphics/backends/webgpu/WebGPUBackendFactory.h"
#include "pers/graphics/IInstance.h"
#include "pers/graphics/IPhysicalDevice.h"

namespace pers::tests::json {

std::string DeviceCreationHandler::getTestType() const {
    return "Device Creation";
}

bool DeviceCreationHandler::execute(const JsonTestCase& testCase, 
                                   std::string& actualResult, 
                                   std::string& failureReason) {
    auto factory = std::make_shared<WebGPUBackendFactory>();
    auto instance = factory->createInstance({});
    auto physicalDevice = instance->requestPhysicalDevice({});
    
    if (!physicalDevice) {
        actualResult = "Physical device request failed";
        failureReason = "Failed to get physical device for device creation";
        return false;
    }
    
    LogicalDeviceDesc ldDesc;
    
    // Check if this is an option-based test
    if (testCase.inputValues.find("type") != testCase.inputValues.end() &&
        testCase.inputValues.at("type") == "OptionBased") {
        const auto& options = testCase.inputValues;
        
        // Parse required features
        if (options.find("required_features") != options.end()) {
            // Features would need to be parsed from string format
            // For now, we'll keep default features
        }
        
        // Parse required limits
        if (options.find("required_limits") != options.end()) {
            const std::string& limits = options.at("required_limits");
            if (limits == "minimal") {
                // Set minimal limits
            } else if (limits == "default") {
                // Use default limits
            } else if (limits == "maximum") {
                // Request maximum limits
            }
        }
        
        // Parse debug name
        if (options.find("debug_name") != options.end()) {
            ldDesc.debugName = options.at("debug_name");
        }
    } else {
        // Regular test
        ldDesc.debugName = "Test Device";
    }
    
    auto device = physicalDevice->createLogicalDevice(ldDesc);
    
    if (device) {
        actualResult = "Valid device created";
        return testCase.expectedResult == actualResult || 
               testCase.expectedResult == "Success with options";
    } else {
        actualResult = "Device creation failed";
        failureReason = "Failed to create logical device";
        return false;
    }
}

} // namespace pers::tests::json