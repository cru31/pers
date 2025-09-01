#pragma once

#include "pers/graphics/IResourceFactory.h"
#include <webgpu/webgpu.h>
#include <memory>

namespace pers {
namespace webgpu {

class WebGPUResourceFactory final : public IResourceFactory {
public:
    explicit WebGPUResourceFactory(WGPUDevice device);
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
    
private:
    WGPUDevice _device = nullptr;
};

} // namespace webgpu
} // namespace pers