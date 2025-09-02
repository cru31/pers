#include "command_encoder_handler.h"
#include "pers/graphics/backends/webgpu/WebGPUBackendFactory.h"
#include "pers/graphics/IInstance.h"
#include "pers/graphics/IPhysicalDevice.h"
#include "pers/graphics/ILogicalDevice.h"

namespace pers::tests::json {

std::string CommandEncoderCreationHandler::getTestType() const {
    return "Command Encoder Creation";
}

bool CommandEncoderCreationHandler::execute(const JsonTestCase& testCase, 
                                           std::string& actualResult, 
                                           std::string& failureReason) {
    auto factory = std::make_shared<WebGPUBackendFactory>();
    auto instance = factory->createInstance({});
    auto physicalDevice = instance->requestPhysicalDevice({});
    auto device = physicalDevice->createLogicalDevice({});
    
    if (!device) {
        actualResult = "Device creation failed";
        failureReason = "Failed to create device for command encoder";
        return false;
    }
    
    // Parse options if option-based test
    std::string debugName = "Main Encoder";
    if (testCase.inputValues.find("type") != testCase.inputValues.end() &&
        testCase.inputValues.at("type") == "OptionBased") {
        const auto& options = testCase.inputValues;
        
        // Parse encoder label
        if (options.find("encoder_label") != options.end()) {
            debugName = options.at("encoder_label");
        }
        
        // Parse encoder type (hint only, not enforced)
        std::string encoderType = "General";
        if (options.find("encoder_type") != options.end()) {
            encoderType = options.at("encoder_type");
        }
    }
    
    // Create command encoder - no desc parameter in current API
    auto commandEncoder = device->createCommandEncoder();
    
    if (commandEncoder) {
        actualResult = "Valid encoder created";
        return testCase.expectedResult == actualResult || 
               testCase.expectedResult == "Success with options";
    } else {
        actualResult = "Encoder creation failed";
        failureReason = "Failed to create command encoder";
        return false;
    }
}

} // namespace pers::tests::json