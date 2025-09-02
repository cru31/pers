#include "webgpu_instance_handler.h"
#include "pers/graphics/backends/webgpu/WebGPUBackendFactory.h"
#include "pers/graphics/backends/IGraphicsBackendFactory.h"

namespace pers::tests::json {

std::string WebGPUInstanceCreationHandler::getTestType() const {
    return "WebGPU Instance Creation";
}

bool WebGPUInstanceCreationHandler::execute(const JsonTestCase& testCase, 
                                           std::string& actualResult, 
                                           std::string& failureReason) {
    auto factory = std::make_shared<WebGPUBackendFactory>();
    InstanceDesc desc;
    
    // Check if this is an option-based test
    if (testCase.inputValues.find("type") != testCase.inputValues.end() &&
        testCase.inputValues.at("type") == "OptionBased") {
        // Parse options
        const auto& options = testCase.inputValues;
        if (options.find("validation") != options.end()) {
            desc.enableValidation = (options.at("validation") == "true");
        }
        if (options.find("application_name") != options.end()) {
            desc.applicationName = options.at("application_name");
        }
        if (options.find("debug_mode") != options.end()) {
            // This would set debug mode if available in InstanceDesc
        }
        
        auto instance = factory->createInstance(desc);
        if (instance) {
            actualResult = "Valid instance created";
            return testCase.expectedResult == actualResult || 
                   testCase.expectedResult == "Success with options";
        } else {
            actualResult = "Failed to create instance";
            failureReason = "Instance creation returned nullptr with options";
            return false;
        }
    } else {
        // Regular test
        desc.enableValidation = true;
        desc.applicationName = "Unit Test";
        desc.engineName = "Pers Graphics Engine";
        
        auto instance = factory->createInstance(desc);
        
        if (instance) {
            actualResult = "Valid instance created";
            return testCase.expectedResult == actualResult;
        } else {
            actualResult = "Failed to create instance";
            failureReason = "Instance creation returned nullptr";
            return false;
        }
    }
}

} // namespace pers::tests::json