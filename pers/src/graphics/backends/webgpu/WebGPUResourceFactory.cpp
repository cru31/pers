#include "pers/graphics/backends/webgpu/WebGPUResourceFactory.h"
#include "pers/graphics/backends/webgpu/buffers/WebGPUBuffer.h"
#include "pers/graphics/backends/webgpu/WebGPUTexture.h"
#include "pers/graphics/backends/webgpu/buffers/WebGPUMappableBuffer.h"
#include "pers/graphics/backends/webgpu/WebGPUTextureView.h"
#include "pers/graphics/backends/webgpu/WebGPUShaderModule.h"
#include "pers/graphics/backends/webgpu/WebGPURenderPipeline.h"
#include "pers/graphics/backends/webgpu/WebGPULogicalDevice.h"
#include "pers/graphics/backends/webgpu/WebGPUConverters.h"
#include "pers/utils/Logger.h"
#include <webgpu/webgpu.h>
#include <cstring>  // for memcpy

namespace pers {

WebGPUResourceFactory::WebGPUResourceFactory(const std::shared_ptr<WebGPULogicalDevice>& logicalDevice) 
    : _logicalDevice(logicalDevice) {
    if (logicalDevice) {
        LOG_INFO("WebGPUResourceFactory",
            "Created resource factory");
    }
}

WebGPUResourceFactory::~WebGPUResourceFactory() {
    // No need to release device, shared_ptr handles it
}

std::shared_ptr<INativeBuffer> WebGPUResourceFactory::createBuffer(const BufferDesc& desc) const {
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
    return std::make_shared<WebGPUBuffer>(wgpuDevice, desc);
}

std::shared_ptr<ITexture> WebGPUResourceFactory::createTexture(const TextureDesc& desc) const {
    auto device = _logicalDevice.lock();
    if (!device) {
        LOG_ERROR("WebGPUResourceFactory", "Cannot create texture without device");
        return nullptr;
    }
    
    WGPUDevice wgpuDevice = device->getNativeDeviceHandle().as<WGPUDevice>();
    
    // Convert texture descriptor
    WGPUTextureDescriptor textureDesc = {};
    textureDesc.label = WGPUStringView{desc.label.data(), desc.label.length()};
    textureDesc.size.width = desc.width;
    textureDesc.size.height = desc.height;
    textureDesc.size.depthOrArrayLayers = desc.depthOrArrayLayers;
    textureDesc.mipLevelCount = desc.mipLevelCount;
    textureDesc.sampleCount = desc.sampleCount;
    textureDesc.dimension = WebGPUConverters::convertTextureDimension(desc.dimension);
    textureDesc.format = WebGPUConverters::convertTextureFormat(desc.format);
    textureDesc.usage = WebGPUConverters::convertTextureUsage(desc.usage);
    
    // Create the texture
    WGPUTexture wgpuTexture = wgpuDeviceCreateTexture(wgpuDevice, &textureDesc);
    if (!wgpuTexture) {
        LOG_ERROR("WebGPUResourceFactory", "Failed to create texture: " + desc.label);
        return nullptr;
    }
    
    return std::make_shared<WebGPUTexture>(
        wgpuTexture,
        desc.width,
        desc.height,
        desc.depthOrArrayLayers,
        desc.format,
        desc.usage,
        desc.dimension
    );
}

std::shared_ptr<ITextureView> WebGPUResourceFactory::createTextureView(
    const std::shared_ptr<ITexture>& texture,
    const TextureViewDesc& desc) const {
    if (!texture) {
        LOG_ERROR("WebGPUResourceFactory", "Cannot create texture view from null texture");
        return nullptr;
    }
    
    auto webgpuTexture = std::dynamic_pointer_cast<WebGPUTexture>(texture);
    if (!webgpuTexture) {
        LOG_ERROR("WebGPUResourceFactory", "Texture is not a WebGPU texture");
        return nullptr;
    }
    
    WGPUTexture wgpuTexture = webgpuTexture->getWGPUTexture();
    
    // Create texture view descriptor
    WGPUTextureViewDescriptor viewDesc = {};
    viewDesc.label = WGPUStringView{desc.label.data(), desc.label.length()};
    viewDesc.format = WebGPUConverters::convertTextureFormat(desc.format);
    viewDesc.dimension = WebGPUConverters::convertTextureViewDimension(desc.dimension);
    viewDesc.baseMipLevel = desc.baseMipLevel;
    viewDesc.mipLevelCount = desc.mipLevelCount;
    viewDesc.baseArrayLayer = desc.baseArrayLayer;
    viewDesc.arrayLayerCount = desc.arrayLayerCount;
    viewDesc.aspect = WebGPUConverters::convertTextureAspect(desc.aspect);
    
    // Create the texture view
    WGPUTextureView wgpuView = wgpuTextureCreateView(wgpuTexture, &viewDesc);
    if (!wgpuView) {
        LOG_ERROR("WebGPUResourceFactory", "Failed to create texture view: " + desc.label);
        return nullptr;
    }
    
    // Use the texture's actual dimensions for the view
    return std::make_shared<WebGPUTextureView>(
        wgpuView,
        webgpuTexture->getWidth(),
        webgpuTexture->getHeight(),
        webgpuTexture->getFormat(),
        false  // Not a swap chain texture
    );
}

std::shared_ptr<ISampler> WebGPUResourceFactory::createSampler(const SamplerDesc& desc) const {
    TODO_OR_DIE("WebGPUResourceFactory::createSampler", 
                   "Implement WebGPUSampler");
    return nullptr;
}

std::shared_ptr<IShaderModule> WebGPUResourceFactory::createShaderModule(const ShaderModuleDesc& desc) const {
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

std::shared_ptr<IRenderPipeline> WebGPUResourceFactory::createRenderPipeline(const RenderPipelineDesc& desc) const {
    auto device = _logicalDevice.lock();
    if (!device) {
        LOG_ERROR("WebGPUResourceFactory",
            "Cannot create render pipeline without device");
        return nullptr;
    }
    
    WGPUDevice wgpuDevice = device->getNativeDeviceHandle().as<WGPUDevice>();
    return std::make_shared<WebGPURenderPipeline>(desc, wgpuDevice);
}

std::shared_ptr<INativeBuffer> WebGPUResourceFactory::createInitializableDeviceBuffer(
    const BufferDesc& desc,
    const void* initialData,
    size_t dataSize) const {
    
    auto device = _logicalDevice.lock();
    if (!device) {
        LOG_ERROR("WebGPUResourceFactory",
            "Cannot create buffer without device");
        return nullptr;
    }
    
    if (!initialData || dataSize == 0) {
        LOG_ERROR("WebGPUResourceFactory",
            "Invalid initial data or size");
        return nullptr;
    }
    
    if (dataSize > desc.size) {
        LOG_ERROR("WebGPUResourceFactory",
            "Data size exceeds buffer size");
        return nullptr;
    }
    
    // Create buffer with mappedAtCreation for synchronous data write
    BufferDesc syncDesc = desc;
    syncDesc.mappedAtCreation = true;
    // Add CopySrc flag to satisfy validation when using mappedAtCreation
    syncDesc.usage |= BufferUsage::CopySrc;
    
    WGPUDevice wgpuDevice = device->getNativeDeviceHandle().as<WGPUDevice>();
    auto buffer = std::make_shared<WebGPUBuffer>(wgpuDevice, syncDesc);
    if (!buffer || !buffer->isValid()) {
        LOG_ERROR("WebGPUResourceFactory",
            "Failed to create buffer");
        return nullptr;
    }
    
    // Write data synchronously using mapped memory
    void* mappedData = buffer->getMappedDataAtCreation();
    if (mappedData) {
        memcpy(mappedData, initialData, dataSize);
        buffer->unmapAtCreation();
    } else {
        LOG_ERROR("WebGPUResourceFactory",
            "Failed to get mapped data");
        return nullptr;
    }
    
    return buffer;
}

std::shared_ptr<INativeMappableBuffer> WebGPUResourceFactory::createMappableBuffer(const BufferDesc& desc) const {

    auto device = _logicalDevice.lock();
    if (!device) {
        LOG_ERROR("WebGPUResourceFactory",
            "Cannot create mappable buffer without device");
        return nullptr;
    }
    
    // Validate buffer size - WebGPU requires size > 0
    if (desc.size == 0) {
        LOG_WARNING("WebGPUResourceFactory",
            "Cannot create mappable buffer with size 0 - WebGPU requires size > 0");
        return nullptr;
    }
    
    WGPUDevice wgpuDevice = device->getNativeDeviceHandle().as<WGPUDevice>();
    return std::make_shared<WebGPUMappableBuffer>(wgpuDevice, desc);
}

} // namespace pers
