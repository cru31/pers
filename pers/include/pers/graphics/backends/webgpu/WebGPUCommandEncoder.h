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
    
    bool uploadToDeviceBuffer(const std::shared_ptr<ImmediateStagingBuffer>& stagingBuffer,
                             const std::shared_ptr<DeviceBuffer>& deviceBuffer,
                             const BufferCopyDesc& copyDesc) override;
    
    bool downloadFromDeviceBuffer(const std::shared_ptr<DeviceBuffer>& deviceBuffer,
                                 const std::shared_ptr<DeferredStagingBuffer>& readbackBuffer,
                                 const BufferCopyDesc& copyDesc) override;

    bool downloadFromDeviceBuffer(const std::shared_ptr<ImmediateDeviceBuffer>& deviceBuffer,
                                 const std::shared_ptr<DeferredStagingBuffer>& readbackBuffer,
                                 const BufferCopyDesc& copyDesc) override;
    bool copyDeviceToDevice(const std::shared_ptr<DeviceBuffer>& source,
                           const std::shared_ptr<DeviceBuffer>& destination,
                           const BufferCopyDesc& copyDesc) override;
    
    std::shared_ptr<ICommandBuffer> finish() override;
    NativeEncoderHandle getNativeEncoderHandle() const override;
    
private:
    // Internal buffer copy implementation
    bool copyBufferToBuffer(const std::shared_ptr<IBuffer>& source,
                           const std::shared_ptr<IBuffer>& destination,
                           const BufferCopyDesc& copyDesc);
    
    WGPUCommandEncoder _encoder = nullptr;
    bool _finished = false;
};

} // namespace pers