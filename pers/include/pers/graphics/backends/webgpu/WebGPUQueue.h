#pragma once

#include "pers/graphics/IQueue.h"
#include <webgpu.h>
#include <memory>

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
    
    /**
     * @brief Submit command buffer to queue
     * @param commandBuffer Command buffer to submit
     * @return true if submission succeeded
     */
    bool submit(std::shared_ptr<ICommandBuffer> commandBuffer) override;
    
    /**
     * @brief Submit multiple command buffers
     * @param commandBuffers Array of command buffers
     * @param count Number of command buffers
     * @return true if submission succeeded
     */
    bool submitBatch(std::shared_ptr<ICommandBuffer>* commandBuffers, uint32_t count) override;
    
    /**
     * @brief Wait for queue to become idle
     */
    void waitIdle() override;
    
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