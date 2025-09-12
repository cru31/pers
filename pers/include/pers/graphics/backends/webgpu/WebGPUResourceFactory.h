#pragma once

#include "pers/graphics/IResourceFactory.h"
#include <webgpu/webgpu.h>
#include <memory>

namespace pers {

// Forward declaration
class WebGPULogicalDevice;

class WebGPUResourceFactory final : public IResourceFactory {
public:
    explicit WebGPUResourceFactory(const std::shared_ptr<WebGPULogicalDevice>& logicalDevice);
    ~WebGPUResourceFactory() override;
    
    // IResourceFactory interface
    std::shared_ptr<IBuffer> createBuffer(const BufferDesc& desc) const override;
    std::shared_ptr<IBuffer> createInitializableDeviceBuffer(
        const BufferDesc& desc,
        const void* initialData,
        size_t dataSize) const override;
    std::shared_ptr<ITexture> createTexture(const TextureDesc& desc) const override;
    std::shared_ptr<ITextureView> createTextureView(
        const std::shared_ptr<ITexture>& texture,
        const TextureViewDesc& desc) const override;
    std::shared_ptr<ISampler> createSampler(const SamplerDesc& desc) const override;
    std::shared_ptr<IShaderModule> createShaderModule(const ShaderModuleDesc& desc) const override;
    std::shared_ptr<IRenderPipeline> createRenderPipeline(const RenderPipelineDesc& desc) const override;
    std::shared_ptr<IMappableBuffer> createMappableBuffer(const BufferDesc& desc) const override;
    
    
private:
    std::weak_ptr<WebGPULogicalDevice> _logicalDevice;
};

} // namespace pers