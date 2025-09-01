#pragma once

#include "pers/graphics/IRenderPassEncoder.h"
#include <webgpu/webgpu.h>
#include <memory>

namespace pers {

/**
 * @brief WebGPU implementation of IRenderPassEncoder
 */
class WebGPURenderPassEncoder : public IRenderPassEncoder {
public:
    /**
     * @brief Constructor
     * @param encoder WebGPU render pass encoder handle
     */
    explicit WebGPURenderPassEncoder(WGPURenderPassEncoder encoder);
    ~WebGPURenderPassEncoder() override;
    
    // IRenderPassEncoder interface implementation
    void setPipeline(std::shared_ptr<IPipeline> pipeline) override;
    void setBindGroup(uint32_t index, std::shared_ptr<IBindGroup> bindGroup) override;
    void setVertexBuffer(uint32_t slot, std::shared_ptr<IBuffer> buffer, 
                        uint64_t offset = 0, uint64_t size = 0) override;
    void setIndexBuffer(std::shared_ptr<IBuffer> buffer, 
                       IndexFormat indexFormat,
                       uint64_t offset = 0, uint64_t size = 0) override;
    void draw(uint32_t vertexCount, uint32_t instanceCount = 1,
             uint32_t firstVertex = 0, uint32_t firstInstance = 0) override;
    void drawIndexed(uint32_t indexCount, uint32_t instanceCount = 1,
                    uint32_t firstIndex = 0, int32_t baseVertex = 0,
                    uint32_t firstInstance = 0) override;
    void end() override;
    NativeRenderPassEncoderHandle getNativeRenderPassEncoderHandle() const override;
    
private:
    WGPURenderPassEncoder _encoder = nullptr;
    bool _ended = false;
};

} // namespace pers