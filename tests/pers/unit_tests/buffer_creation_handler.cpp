#include "buffer_creation_handler.h"
#include "pers/graphics/backends/webgpu/WebGPUBackendFactory.h"
#include "pers/graphics/IBuffer.h"
#include "pers/graphics/IInstance.h"
#include "pers/graphics/IPhysicalDevice.h"
#include "pers/graphics/ILogicalDevice.h"
#include "pers/graphics/IResourceFactory.h"

namespace pers::tests::json {

std::string BufferCreationHandler::getTestType() const {
    return "Buffer Creation";
}

bool BufferCreationHandler::execute(const JsonTestCase& testCase, 
                                   std::string& actualResult, 
                                   std::string& failureReason) {
    auto factory = std::make_shared<WebGPUBackendFactory>();
    auto instance = factory->createInstance({});
    if (!instance) {
        actualResult = "No instance";
        return false;
    }
    
    auto physicalDevice = instance->requestPhysicalDevice({});
    if (!physicalDevice) {
        actualResult = "No physical device";
        return false;
    }
    
    auto device = physicalDevice->createLogicalDevice({});
    if (!device) {
        actualResult = "No device";
        return false;
    }
    
    auto resourceFactory = device->getResourceFactory();
    if (!resourceFactory) {
        actualResult = "No resource factory";
        return false;
    }
    
    // Parse buffer options
    BufferDesc bufferDesc;
    
    // Check if this is an option-based test
    if (testCase.inputValues.find("type") != testCase.inputValues.end() &&
        testCase.inputValues.at("type") == "OptionBased") {
        const auto& options = testCase.inputValues;
        
        if (options.find("size") != options.end()) {
            bufferDesc.size = std::stoull(options.at("size"));
        }
        if (options.find("label") != options.end()) {
            bufferDesc.debugName = options.at("label");
        }
        if (options.find("usage") != options.end()) {
            // Parse usage flags
            std::string usage = options.at("usage");
            bufferDesc.usage = BufferUsage::None;
            if (usage.find("Vertex") != std::string::npos) {
                bufferDesc.usage = static_cast<BufferUsage>(
                    static_cast<uint32_t>(bufferDesc.usage) | static_cast<uint32_t>(BufferUsage::Vertex));
            }
            if (usage.find("Index") != std::string::npos) {
                bufferDesc.usage = static_cast<BufferUsage>(
                    static_cast<uint32_t>(bufferDesc.usage) | static_cast<uint32_t>(BufferUsage::Index));
            }
            if (usage.find("Uniform") != std::string::npos) {
                bufferDesc.usage = static_cast<BufferUsage>(
                    static_cast<uint32_t>(bufferDesc.usage) | static_cast<uint32_t>(BufferUsage::Uniform));
            }
            if (usage.find("Storage") != std::string::npos) {
                bufferDesc.usage = static_cast<BufferUsage>(
                    static_cast<uint32_t>(bufferDesc.usage) | static_cast<uint32_t>(BufferUsage::Storage));
            }
            if (usage.find("CopySrc") != std::string::npos) {
                bufferDesc.usage = static_cast<BufferUsage>(
                    static_cast<uint32_t>(bufferDesc.usage) | static_cast<uint32_t>(BufferUsage::CopySrc));
            }
            if (usage.find("CopyDst") != std::string::npos) {
                bufferDesc.usage = static_cast<BufferUsage>(
                    static_cast<uint32_t>(bufferDesc.usage) | static_cast<uint32_t>(BufferUsage::CopyDst));
            }
        }
    }
    
    // Special case for size 0
    if (bufferDesc.size == 0) {
        auto buffer = resourceFactory->createBuffer(bufferDesc);
        if (!buffer) {
            actualResult = "Returns nullptr";
            return testCase.expectedResult == actualResult;
        } else {
            actualResult = "Buffer created with 0 size";
            failureReason = "Should have returned nullptr for 0 size";
            return false;
        }
    }
    
    auto buffer = resourceFactory->createBuffer(bufferDesc);
    if (buffer) {
        actualResult = "Success with options";
        return testCase.expectedResult == actualResult || 
               testCase.expectedResult == "Valid buffer";
    } else {
        actualResult = "Failed to create buffer";
        failureReason = "Buffer creation failed";
        return false;
    }
}

std::string BufferSpecificTestHandler::getTestType() const {
    return "Buffer Creation 64KB";
}

bool BufferSpecificTestHandler::canHandle(const JsonTestCase& testCase) const {
    return testCase.testType == "Buffer Creation 64KB" || 
           testCase.testType == "Buffer Creation 0 Size";
}

bool BufferSpecificTestHandler::execute(const JsonTestCase& testCase, 
                                       std::string& actualResult, 
                                       std::string& failureReason) {
    auto factory = std::make_shared<WebGPUBackendFactory>();
    auto instance = factory->createInstance({});
    auto physicalDevice = instance->requestPhysicalDevice({});
    auto device = physicalDevice->createLogicalDevice({});
    
    if (!device) {
        actualResult = "Failed to create device";
        failureReason = "Device required for buffer test";
        return false;
    }
    
    auto resourceFactory = device->getResourceFactory();
    if (!resourceFactory) {
        actualResult = "No resource factory";
        failureReason = "getResourceFactory() returned nullptr";
        return false;
    }
    
    if (testCase.testType == "Buffer Creation 64KB") {
        BufferDesc desc;
        desc.size = 65536;
        desc.usage = BufferUsage::Storage;
        desc.debugName = "Test Buffer 64KB";
        
        auto buffer = resourceFactory->createBuffer(desc);
        if (buffer) {
            actualResult = "Valid buffer";
            return testCase.expectedResult == actualResult;
        } else {
            actualResult = "Buffer is nullptr";
            failureReason = "createBuffer() returned nullptr";
            return false;
        }
    }
    else if (testCase.testType == "Buffer Creation 0 Size") {
        BufferDesc desc;
        desc.size = 0;
        desc.usage = BufferUsage::Vertex;
        
        auto buffer = resourceFactory->createBuffer(desc);
        if (!buffer) {
            actualResult = "Returns nullptr";
            return testCase.expectedResult == actualResult;
        } else {
            actualResult = "Buffer created with 0 size";
            failureReason = "Should have returned nullptr for 0 size";
            return false;
        }
    }
    
    return false;
}

} // namespace pers::tests::json