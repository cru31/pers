#pragma once

#include "pers/graphics/ICommandEncoder.h"
#include <webgpu.h>
#include <memory>

namespace pers {

/**
 * @brief WebGPU implementation of ICommandEncoder
 */
class WebGPUCommandEncoder : public ICommandEncoder {
public:
    /**
     * @brief Constructor
     * @param encoder WebGPU command encoder handle
     */
    explicit WebGPUCommandEncoder(WGPUCommandEncoder encoder);
    ~WebGPUCommandEncoder() override;
    
    // ICommandEncoder interface implementation
    std::shared_ptr<IRenderPassEncoder> beginRenderPass(const RenderPassDesc& desc) override;
    std::shared_ptr<ICommandBuffer> finish() override;
    NativeEncoderHandle getNativeEncoderHandle() const override;
    
private:
    WGPUCommandEncoder _encoder = nullptr;
    bool _finished = false;
};

} // namespace pers