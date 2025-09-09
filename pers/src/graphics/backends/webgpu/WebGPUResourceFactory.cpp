#include "pers/graphics/backends/webgpu/WebGPUResourceFactory.h"
#include "pers/graphics/backends/webgpu/WebGPUBuffer.h"
#include "pers/graphics/backends/webgpu/WebGPUShaderModule.h"
#include "pers/graphics/backends/webgpu/WebGPURenderPipeline.h"
#include "pers/graphics/backends/webgpu/WebGPULogicalDevice.h"
#include "pers/graphics/backends/webgpu/WebGPUFramebufferFactory.h"
#include "pers/graphics/backends/webgpu/WebGPUOffscreenFramebuffer.h"
#include "pers/utils/Logger.h"
#include <webgpu/webgpu.h>

namespace pers {
namespace webgpu {

WebGPUResourceFactory::WebGPUResourceFactory(const std::weak_ptr<WebGPULogicalDevice>& logicalDevice) 
    : _logicalDevice(logicalDevice) {
    if (!_logicalDevice.expired()) {
        LOG_INFO("WebGPUResourceFactory",
            "Created resource factory");
    }
}

WebGPUResourceFactory::~WebGPUResourceFactory() {
    // No need to release device, shared_ptr handles it
}

std::shared_ptr<IBuffer> WebGPUResourceFactory::createBuffer(const BufferDesc& desc) {
    auto device = _logicalDevice.lock();
    if (!device) {
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
    
    WGPUDevice wgpuDevice = device->getNativeDeviceHandle().as<WGPUDevice>();
    return std::make_shared<WebGPUBuffer>(desc, wgpuDevice);
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
    auto device = _logicalDevice.lock();
    if (!device) {
        LOG_ERROR("WebGPUResourceFactory",
            "Cannot create shader module without device");
        return nullptr;
    }
    
    auto shader = std::make_shared<WebGPUShaderModule>(desc);
    
    // Create the actual WebGPU shader module
    WGPUDevice wgpuDevice = device->getNativeDeviceHandle().as<WGPUDevice>();
    shader->createShaderModule(wgpuDevice);
    
    if (!shader->isValid()) {
        Logger::Instance().LogFormat(LogLevel::Error, "WebGPUResourceFactory",
            PERS_SOURCE_LOC, "Failed to create shader module: %s", desc.debugName.c_str());
        return nullptr;
    }
    
    return shader;
}

std::shared_ptr<IRenderPipeline> WebGPUResourceFactory::createRenderPipeline(const RenderPipelineDesc& desc) {
    auto device = _logicalDevice.lock();
    if (!device) {
        LOG_ERROR("WebGPUResourceFactory",
            "Cannot create render pipeline without device");
        return nullptr;
    }
    
    WGPUDevice wgpuDevice = device->getNativeDeviceHandle().as<WGPUDevice>();
    return std::make_shared<WebGPURenderPipeline>(desc, wgpuDevice);
}

std::shared_ptr<ISurfaceFramebuffer> WebGPUResourceFactory::createSurfaceFramebuffer(
    const NativeSurfaceHandle& surface,
    uint32_t width,
    uint32_t height,
    TextureFormat format) {
    
    auto device = _logicalDevice.lock();
    if (!device) {
        LOG_ERROR("WebGPUResourceFactory",
            "Cannot create surface framebuffer without device");
        return nullptr;
    }
    
    WGPUSurface wgpuSurface = surface.as<WGPUSurface>();
    return WebGPUFramebufferFactory::createSurfaceFramebuffer(
        device, wgpuSurface, width, height, format);
}

std::shared_ptr<IFramebuffer> WebGPUResourceFactory::createOffscreenFramebuffer(
    const OffscreenFramebufferDesc& desc) {
    
    auto device = _logicalDevice.lock();
    if (!device) {
        LOG_ERROR("WebGPUResourceFactory",
            "Cannot create offscreen framebuffer without device");
        return nullptr;
    }
    
    // Convert from IResourceFactory desc to WebGPU desc
    OffscreenFramebufferConfig config;
    config.width = desc.width;
    config.height = desc.height;
    config.sampleCount = desc.sampleCount;
    config.colorFormats = desc.colorFormats;
    config.depthFormat = desc.depthFormat;
    config.colorUsage = desc.colorUsage;
    config.depthUsage = desc.depthUsage;
    
    return WebGPUFramebufferFactory::createOffscreenFramebuffer(device, config);
}

std::shared_ptr<IFramebuffer> WebGPUResourceFactory::createDepthOnlyFramebuffer(
    uint32_t width,
    uint32_t height,
    TextureFormat format,
    uint32_t sampleCount,
    TextureUsage usage) {
    
    auto device = _logicalDevice.lock();
    if (!device) {
        LOG_ERROR("WebGPUResourceFactory",
            "Cannot create depth-only framebuffer without device");
        return nullptr;
    }
    
    return WebGPUFramebufferFactory::createDepthOnlyFramebuffer(
        device, width, height, format, sampleCount, usage);
}

} // namespace webgpu
} // namespace pers