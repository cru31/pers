#pragma once

#include "pers/graphics/IQueue.h"
#include <webgpu/webgpu.h>
#include <memory>
#include <vector>

namespace pers {

/**
 * @brief WebGPU implementation of IQueue
 */
class WebGPUQueue : public IQueue {
public:
    /**
     * @brief Constructor
     * @param queue WebGPU queue handle
     */
    explicit WebGPUQueue(WGPUQueue queue);
    ~WebGPUQueue() override;
    
    // IQueue interface implementation
    bool submit(const std::vector<std::shared_ptr<ICommandBuffer>>& commandBuffers) override;
    bool submit(std::shared_ptr<ICommandBuffer> commandBuffer) override;
    bool submitBatch(const std::vector<std::shared_ptr<ICommandBuffer>>& commandBuffers) override;
    bool writeBuffer(const BufferWriteDesc& desc) override;
    bool writeTexture(std::shared_ptr<ITexture> texture, 
                     const void* data, 
                     uint64_t dataSize,
                     uint32_t mipLevel = 0) override;
    bool waitIdle() override;
    
    /**
     * @brief Get native queue handle
     * @return Native handle to the queue
     */
    NativeQueueHandle getNativeQueueHandle() const override;
    
    /**
     * @brief Write buffer data (WebGPU specific helper)
     * @param buffer Buffer handle
     * @param offset Offset in buffer
     * @param data Data pointer
     * @param size Size of data
     */
    void writeBuffer(WGPUBuffer buffer, uint64_t offset, const void* data, uint64_t size);
    
private:
    WGPUQueue _queue = nullptr;
};

} // namespace pers