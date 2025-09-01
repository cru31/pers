#include "pers/graphics/backends/webgpu/WebGPUResourceFactory.h"
#include "pers/graphics/backends/webgpu/WebGPUBuffer.h"
#include "pers/graphics/backends/webgpu/WebGPUShaderModule.h"
#include "pers/graphics/backends/webgpu/WebGPURenderPipeline.h"
#include "pers/graphics/IRenderPipeline.h"
#include "pers/utils/Logger.h"
#include "pers/utils/TodoOrDie.h"
#include <webgpu/webgpu.h>

namespace pers {
namespace webgpu {

class WebGPUResourceFactory::Impl {
public:
    explicit Impl(WGPUDevice device) 
        : _device(device) {
        if (_device) {
            wgpuDeviceAddRef(_device);
            Logger::Instance().Log(LogLevel::Info, "WebGPUResourceFactory",
                "Created resource factory", PERS_SOURCE_LOC);
        }
    }
    
    ~Impl() {
        if (_device) {
            wgpuDeviceRelease(_device);
        }
    }
    
    WGPUDevice _device;
};

WebGPUResourceFactory::WebGPUResourceFactory(void* device)
    : _impl(std::make_unique<Impl>(static_cast<WGPUDevice>(device))) {
}

WebGPUResourceFactory::~WebGPUResourceFactory() = default;

std::shared_ptr<IBuffer> WebGPUResourceFactory::createBuffer(const BufferDesc& desc) {
    if (!_impl->_device) {
        Logger::Instance().Log(LogLevel::Error, "WebGPUResourceFactory",
            "Cannot create buffer without device", PERS_SOURCE_LOC);
        return nullptr;
    }
    
    return std::make_shared<WebGPUBuffer>(desc, _impl->_device);
}

std::shared_ptr<ITexture> WebGPUResourceFactory::createTexture(const TextureDesc& desc) {
    TodoOrDie::Log("WebGPUResourceFactory::createTexture - Implement WebGPUTexture");
    return nullptr;
}

std::shared_ptr<ITextureView> WebGPUResourceFactory::createTextureView(
    std::shared_ptr<ITexture> texture,
    const TextureViewDesc& desc) {
    TodoOrDie::Log("WebGPUResourceFactory::createTextureView - Implement WebGPUTextureView");
    return nullptr;
}

std::shared_ptr<ISampler> WebGPUResourceFactory::createSampler(const SamplerDesc& desc) {
    TodoOrDie::Log("WebGPUResourceFactory::createSampler - Implement WebGPUSampler");
    return nullptr;
}

std::shared_ptr<IShaderModule> WebGPUResourceFactory::createShaderModule(const ShaderModuleDesc& desc) {
    if (!_impl->_device) {
        Logger::Instance().Log(LogLevel::Error, "WebGPUResourceFactory",
            "Cannot create shader module without device", PERS_SOURCE_LOC);
        return nullptr;
    }
    
    auto shader = std::make_shared<WebGPUShaderModule>(desc);
    
    // Create the actual WebGPU shader module
    shader->createShaderModule(_impl->_device);
    
    if (!shader->isValid()) {
        Logger::Instance().LogFormat(LogLevel::Error, "WebGPUResourceFactory",
            PERS_SOURCE_LOC, "Failed to create shader module: %s", desc.debugName.c_str());
        return nullptr;
    }
    
    return shader;
}

std::shared_ptr<IRenderPipeline> WebGPUResourceFactory::createRenderPipeline(const RenderPipelineDesc& desc) {
    if (!_impl->_device) {
        Logger::Instance().Log(LogLevel::Error, "WebGPUResourceFactory",
            "Cannot create render pipeline without device", PERS_SOURCE_LOC);
        return nullptr;
    }
    
    return std::make_shared<WebGPURenderPipeline>(desc, _impl->_device);
}

NativeResourceFactoryHandle WebGPUResourceFactory::getNativeFactoryHandle() const {
    return NativeResourceFactoryHandle{_impl->_device};
}

} // namespace webgpu
} // namespace pers