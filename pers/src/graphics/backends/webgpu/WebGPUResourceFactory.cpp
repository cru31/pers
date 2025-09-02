#include "pers/graphics/backends/webgpu/WebGPUResourceFactory.h"
#include "pers/graphics/backends/webgpu/WebGPUBuffer.h"
#include "pers/graphics/backends/webgpu/WebGPUShaderModule.h"
#include "pers/graphics/backends/webgpu/WebGPURenderPipeline.h"
#include "pers/utils/Logger.h"
#include <webgpu/webgpu.h>

namespace pers {
namespace webgpu {

WebGPUResourceFactory::WebGPUResourceFactory(WGPUDevice device) 
    : _device(device) {
    if (_device) {
        wgpuDeviceAddRef(_device);
        LOG_INFO("WebGPUResourceFactory",
            "Created resource factory");
    }
}

WebGPUResourceFactory::~WebGPUResourceFactory() {
    if (_device) {
        wgpuDeviceRelease(_device);
    }
}

std::shared_ptr<IBuffer> WebGPUResourceFactory::createBuffer(const BufferDesc& desc) {
    if (!_device) {
        LOG_ERROR("WebGPUResourceFactory",
            "Cannot create buffer without device");
        return nullptr;
    }
    
    // Validate buffer size - WebGPU requires size > 0
    if (desc.size == 0) {
        LOG_WARNING("WebGPUResourceFactory",
            "Cannot create buffer with size 0 - WebGPU requires size > 0");
        return nullptr;
    }
    
    return std::make_shared<WebGPUBuffer>(desc, _device);
}

std::shared_ptr<ITexture> WebGPUResourceFactory::createTexture(const TextureDesc& desc) {
    TODO_OR_DIE("WebGPUResourceFactory::createTexture", 
                   "Implement WebGPUTexture");
    return nullptr;
}

std::shared_ptr<ITextureView> WebGPUResourceFactory::createTextureView(
    const std::shared_ptr<ITexture>& texture,
    const TextureViewDesc& desc) {
    TODO_OR_DIE("WebGPUResourceFactory::createTextureView", 
                   "Implement WebGPUTextureView");
    return nullptr;
}

std::shared_ptr<ISampler> WebGPUResourceFactory::createSampler(const SamplerDesc& desc) {
    TODO_OR_DIE("WebGPUResourceFactory::createSampler", 
                   "Implement WebGPUSampler");
    return nullptr;
}

std::shared_ptr<IShaderModule> WebGPUResourceFactory::createShaderModule(const ShaderModuleDesc& desc) {
    if (!_device) {
        LOG_ERROR("WebGPUResourceFactory",
            "Cannot create shader module without device");
        return nullptr;
    }
    
    auto shader = std::make_shared<WebGPUShaderModule>(desc);
    
    // Create the actual WebGPU shader module
    shader->createShaderModule(_device);
    
    if (!shader->isValid()) {
        Logger::Instance().LogFormat(LogLevel::Error, "WebGPUResourceFactory",
            PERS_SOURCE_LOC, "Failed to create shader module: %s", desc.debugName.c_str());
        return nullptr;
    }
    
    return shader;
}

std::shared_ptr<IRenderPipeline> WebGPUResourceFactory::createRenderPipeline(const RenderPipelineDesc& desc) {
    if (!_device) {
        LOG_ERROR("WebGPUResourceFactory",
            "Cannot create render pipeline without device");
        return nullptr;
    }
    
    return std::make_shared<WebGPURenderPipeline>(desc, _device);
}

} // namespace webgpu
} // namespace pers