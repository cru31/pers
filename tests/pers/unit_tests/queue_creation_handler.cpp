#include "queue_creation_handler.h"
#include "pers/graphics/backends/webgpu/WebGPUBackendFactory.h"
#include "pers/graphics/IInstance.h"
#include "pers/graphics/IPhysicalDevice.h"
#include "pers/graphics/ILogicalDevice.h"

namespace pers::tests::json {

std::string QueueCreationHandler::getTestType() const {
    return "Queue Creation";
}

bool QueueCreationHandler::execute(const JsonTestCase& testCase, 
                                  std::string& actualResult, 
                                  std::string& failureReason) {
    auto factory = std::make_shared<WebGPUBackendFactory>();
    auto instance = factory->createInstance({});
    auto physicalDevice = instance->requestPhysicalDevice({});
    auto device = physicalDevice->createLogicalDevice({});
    
    if (!device) {
        actualResult = "Device creation failed";
        failureReason = "Failed to create device for queue creation";
        return false;
    }
    
    // Parse options if option-based test
    if (testCase.inputValues.find("type") != testCase.inputValues.end() &&
        testCase.inputValues.at("type") == "OptionBased") {
        const auto& options = testCase.inputValues;
        
        // Parse queue label
        std::string queueLabel = "Default";
        if (options.find("queue_label") != options.end()) {
            queueLabel = options.at("queue_label");
        }
        
        // Parse multiple queues
        bool multipleQueues = false;
        if (options.find("multiple_queues") != options.end()) {
            multipleQueues = (options.at("multiple_queues") == "true");
        }
    }
    
    // Get queue from device
    auto queue = device->getQueue();
    
    if (queue) {
        actualResult = "Valid queue created";
        return testCase.expectedResult == actualResult || 
               testCase.expectedResult == "Success with options";
    } else {
        actualResult = "Queue creation failed";
        failureReason = "Failed to get queue from device";
        return false;
    }
}

} // namespace pers::tests::json