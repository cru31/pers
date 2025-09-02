#include "shader_handler.h"
#include "pers/graphics/backends/webgpu/WebGPUBackendFactory.h"
#include "pers/graphics/IInstance.h"
#include "pers/graphics/IPhysicalDevice.h"
#include "pers/graphics/ILogicalDevice.h"
#include "pers/graphics/IResourceFactory.h"
#include "pers/graphics/IShaderModule.h"

namespace pers::tests::json {

std::string ShaderModuleCreationHandler::getTestType() const {
    return "Shader Module Creation";
}

bool ShaderModuleCreationHandler::execute(const JsonTestCase& testCase, 
                                         std::string& actualResult, 
                                         std::string& failureReason) {
    auto factory = std::make_shared<WebGPUBackendFactory>();
    auto instance = factory->createInstance({});
    auto physicalDevice = instance->requestPhysicalDevice({});
    auto device = physicalDevice->createLogicalDevice({});
    
    if (!device) {
        actualResult = "Device creation failed";
        failureReason = "Failed to create device for shader test";
        return false;
    }
    
    auto resourceFactory = device->getResourceFactory();
    if (!resourceFactory) {
        actualResult = "No resource factory";
        failureReason = "getResourceFactory() returned nullptr";
        return false;
    }
    
    // Simple vertex shader code
    const char* vertexShaderCode = R"(
        @vertex
        fn main(@builtin(vertex_index) vertexIndex: u32) -> @builtin(position) vec4<f32> {
            var pos = array<vec2<f32>, 3>(
                vec2<f32>( 0.0,  0.5),
                vec2<f32>(-0.5, -0.5),
                vec2<f32>( 0.5, -0.5)
            );
            return vec4<f32>(pos[vertexIndex], 0.0, 1.0);
        }
    )";
    
    ShaderModuleDesc shaderDesc;
    shaderDesc.code = vertexShaderCode;
    shaderDesc.stage = ShaderStage::Vertex;
    shaderDesc.debugName = "Test Vertex Shader";
    
    auto shaderModule = resourceFactory->createShaderModule(shaderDesc);
    
    if (shaderModule) {
        actualResult = "Valid shader";
        return testCase.expectedResult == actualResult;
    } else {
        actualResult = "Shader creation failed";
        failureReason = "createShaderModule() returned nullptr";
        return false;
    }
}

} // namespace pers::tests::json