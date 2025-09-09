#pragma once

#include "pers/graphics/IResourceFactory.h"
#include <webgpu/webgpu.h>
#include <memory>

namespace pers {

// Forward declaration
class WebGPULogicalDevice;

namespace webgpu {

class WebGPUResourceFactory final : public IResourceFactory {
public:
    explicit WebGPUResourceFactory(const std::shared_ptr<WebGPULogicalDevice>& logicalDevice);
    ~WebGPUResourceFactory() override;
    
    // IResourceFactory interface
    std::shared_ptr<IBuffer> createBuffer(const BufferDesc& desc) override;
    std::shared_ptr<ITexture> createTexture(const TextureDesc& desc) override;
    std::shared_ptr<ITextureView> createTextureView(
        const std::shared_ptr<ITexture>& texture,
        const TextureViewDesc& desc) override;
    std::shared_ptr<ISampler> createSampler(const SamplerDesc& desc) override;
    std::shared_ptr<IShaderModule> createShaderModule(const ShaderModuleDesc& desc) override;
    std::shared_ptr<IRenderPipeline> createRenderPipeline(const RenderPipelineDesc& desc) override;
    
    // Framebuffer creation methods
    std::shared_ptr<ISurfaceFramebuffer> createSurfaceFramebuffer(
        const NativeSurfaceHandle& surface,
        uint32_t width,
        uint32_t height,
        TextureFormat format = TextureFormat::BGRA8Unorm) override;
    
    std::shared_ptr<IFramebuffer> createOffscreenFramebuffer(
        const OffscreenFramebufferDesc& desc) override;
    
    std::shared_ptr<IFramebuffer> createDepthOnlyFramebuffer(
        uint32_t width,
        uint32_t height,
        TextureFormat format = TextureFormat::Depth24Plus,
        uint32_t sampleCount = 1,
        TextureUsage usage = TextureUsage::RenderAttachment | TextureUsage::TextureBinding) override;
    
private:
    std::weak_ptr<WebGPULogicalDevice> _logicalDevice;
};

} // namespace webgpu
} // namespace pers