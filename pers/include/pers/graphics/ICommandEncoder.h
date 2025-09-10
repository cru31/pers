#pragma once

#include <memory>
#include "pers/graphics/RenderPassTypes.h"
#include "pers/graphics/buffers/BufferTypes.h"

namespace pers {

// Forward declarations
class ICommandBuffer;
class IRenderPassEncoder;

namespace graphics {
class IBuffer;
class DeviceBuffer;
class ImmediateStagingBuffer;
class DeferredStagingBuffer;
}

/**
 * @brief Command encoder interface for recording GPU commands
 * 
 * Encodes GPU commands into a command buffer that can be submitted to a queue.
 * Based on WebGPU CommandEncoder concept but abstracted for cross-platform use.
 */
class ICommandEncoder {
public:
    virtual ~ICommandEncoder() = default;
    
    /**
     * @brief Begin a render pass
     * @param desc Render pass descriptor
     * @return Render pass encoder for recording draw commands
     */
    virtual std::shared_ptr<IRenderPassEncoder> beginRenderPass(const RenderPassDesc& desc) = 0;
    
    /**
     * @brief Upload data from staging buffer to device buffer
     * @param stagingBuffer Source staging buffer with CPU data
     * @param deviceBuffer Destination GPU buffer
     * @param copyDesc Copy parameters (offsets and size)
     * @return true if command was successfully encoded, false otherwise
     */
    virtual bool uploadToDeviceBuffer(const std::shared_ptr<graphics::ImmediateStagingBuffer>& stagingBuffer,
                                      const std::shared_ptr<graphics::DeviceBuffer>& deviceBuffer,
                                      const graphics::BufferCopyDesc& copyDesc) = 0;
    
    /**
     * @brief Download data from device buffer to readback buffer
     * @param deviceBuffer Source GPU buffer
     * @param readbackBuffer Destination buffer for CPU readback
     * @param copyDesc Copy parameters (offsets and size)
     * @return true if command was successfully encoded, false otherwise
     */
    virtual bool downloadFromDeviceBuffer(const std::shared_ptr<graphics::DeviceBuffer>& deviceBuffer,
                                          const std::shared_ptr<graphics::DeferredStagingBuffer>& readbackBuffer,
                                          const graphics::BufferCopyDesc& copyDesc) = 0;
    
    /**
     * @brief Copy data between device buffers (GPU to GPU)
     * @param source Source GPU buffer
     * @param destination Destination GPU buffer
     * @param copyDesc Copy parameters (offsets and size)
     * @return true if command was successfully encoded, false otherwise
     */
    virtual bool copyDeviceToDevice(const std::shared_ptr<graphics::DeviceBuffer>& source,
                                    const std::shared_ptr<graphics::DeviceBuffer>& destination,
                                    const graphics::BufferCopyDesc& copyDesc) = 0;
    
    /**
     * @brief Finish recording and create command buffer
     * @return Command buffer ready for submission
     */
    virtual std::shared_ptr<ICommandBuffer> finish() = 0;
    
    /**
     * @brief Get native encoder handle for backend-specific operations
     * @return Native encoder handle (WGPUCommandEncoder for WebGPU)
     */
    virtual NativeEncoderHandle getNativeEncoderHandle() const = 0;
};

} // namespace pers