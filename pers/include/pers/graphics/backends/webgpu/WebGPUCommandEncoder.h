#pragma once

#include "pers/graphics/ICommandEncoder.h"
#include <webgpu/webgpu.h>
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
    
    bool uploadToDeviceBuffer(const std::shared_ptr<graphics::ImmediateStagingBuffer>& stagingBuffer,
                             const std::shared_ptr<graphics::DeviceBuffer>& deviceBuffer,
                             const graphics::BufferCopyDesc& copyDesc) override;
    
    bool downloadFromDeviceBuffer(const std::shared_ptr<graphics::DeviceBuffer>& deviceBuffer,
                                 const std::shared_ptr<graphics::DeferredStagingBuffer>& readbackBuffer,
                                 const graphics::BufferCopyDesc& copyDesc) override;
    
    bool copyDeviceToDevice(const std::shared_ptr<graphics::DeviceBuffer>& source,
                           const std::shared_ptr<graphics::DeviceBuffer>& destination,
                           const graphics::BufferCopyDesc& copyDesc) override;
    
    std::shared_ptr<ICommandBuffer> finish() override;
    NativeEncoderHandle getNativeEncoderHandle() const override;
    
private:
    // Internal buffer copy implementation
    bool copyBufferToBuffer(const std::shared_ptr<graphics::IBuffer>& source,
                           const std::shared_ptr<graphics::IBuffer>& destination,
                           const graphics::BufferCopyDesc& copyDesc);
    
    WGPUCommandEncoder _encoder = nullptr;
    bool _finished = false;
};

} // namespace pers