#include "queue_operations_handler.h"
#include "pers/graphics/backends/webgpu/WebGPUBackendFactory.h"
#include "pers/graphics/IInstance.h"
#include "pers/graphics/IPhysicalDevice.h"
#include "pers/graphics/ILogicalDevice.h"
#include "pers/graphics/IQueue.h"
#include "pers/graphics/ICommandEncoder.h"
#include "pers/graphics/ICommandBuffer.h"

namespace pers::tests::json {

std::string QueueSubmitEmptyHandler::getTestType() const {
    return "Queue Submit Empty";
}

bool QueueSubmitEmptyHandler::execute(const JsonTestCase& testCase, 
                                     std::string& actualResult, 
                                     std::string& failureReason) {
    auto factory = std::make_shared<WebGPUBackendFactory>();
    auto instance = factory->createInstance({});
    auto physicalDevice = instance->requestPhysicalDevice({});
    auto device = physicalDevice->createLogicalDevice({});
    
    if (!device) {
        actualResult = "Device creation failed";
        failureReason = "Failed to create device for queue test";
        return false;
    }
    
    auto queue = device->getQueue();
    if (!queue) {
        actualResult = "Queue creation failed";
        failureReason = "getQueue() returned nullptr";
        return false;
    }
    
    // Create empty command encoder and finish it immediately
    auto commandEncoder = device->createCommandEncoder();
    if (!commandEncoder) {
        actualResult = "Command encoder creation failed";
        failureReason = "createCommandEncoder() returned nullptr";
        return false;
    }
    
    // Finish without any commands
    auto commandBuffer = commandEncoder->finish();
    if (!commandBuffer) {
        actualResult = "Command buffer creation failed";
        failureReason = "finish() returned nullptr";
        return false;
    }
    
    // Submit empty command buffer
    bool submitResult = queue->submit(commandBuffer);
    
    if (submitResult) {
        actualResult = "Returns success";
        return testCase.expectedResult == actualResult;
    } else {
        actualResult = "Submit failed";
        failureReason = "queue->submit() returned false";
        return false;
    }
}

} // namespace pers::tests::json