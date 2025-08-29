#include "pers/graphics/backends/webgpu/WebGPUQueue.h"
#include "pers/graphics/ICommandBuffer.h"
#include "pers/utils/NotImplemented.h"
#include "pers/utils/Logger.h"
#include <vector>

namespace pers {

WebGPUQueue::WebGPUQueue(WGPUQueue queue)
    : _queue(queue) {
    
    if (_queue) {
        wgpuQueueAddRef(_queue);
        Logger::info("[WebGPUQueue] Created with queue");
    } else {
        Logger::error("[WebGPUQueue] Created with null queue!");
    }
}

WebGPUQueue::~WebGPUQueue() {
    if (_queue) {
        wgpuQueueRelease(_queue);
        _queue = nullptr;
    }
}

bool WebGPUQueue::submit(std::shared_ptr<ICommandBuffer> commandBuffer) {
    if (!_queue) {
        Logger::error("[WebGPUQueue] Cannot submit: queue is null");
        return false;
    }
    
    if (!commandBuffer) {
        Logger::error("[WebGPUQueue] Cannot submit null command buffer");
        return false;
    }
    
    // Get native command buffer handle
    NativeCommandBufferHandle nativeHandle = commandBuffer->getNativeCommandBufferHandle();
    if (!nativeHandle.isValid()) {
        Logger::error("[WebGPUQueue] Command buffer has invalid native handle");
        return false;
    }
    
    // Convert to WebGPU command buffer
    WGPUCommandBuffer wgpuCmdBuffer = nativeHandle.as<WGPUCommandBuffer>();
    
    // Submit to queue
    wgpuQueueSubmit(_queue, 1, &wgpuCmdBuffer);
    
    Logger::debug("[WebGPUQueue] Command buffer submitted");
    return true;
}

bool WebGPUQueue::submitBatch(std::shared_ptr<ICommandBuffer>* commandBuffers, uint32_t count) {
    if (!_queue) {
        Logger::error("[WebGPUQueue] Cannot submit: queue is null");
        return false;
    }
    
    if (!commandBuffers || count == 0) {
        Logger::error("[WebGPUQueue] Invalid command buffer batch");
        return false;
    }
    
    // Collect native handles
    std::vector<WGPUCommandBuffer> wgpuBuffers;
    wgpuBuffers.reserve(count);
    
    for (uint32_t i = 0; i < count; ++i) {
        if (!commandBuffers[i]) {
            Logger::error("[WebGPUQueue] Null command buffer at index {}", i);
            return false;
        }
        
        NativeCommandBufferHandle nativeHandle = commandBuffers[i]->getNativeCommandBufferHandle();
        if (!nativeHandle.isValid()) {
            Logger::error("[WebGPUQueue] Invalid native handle at index {}", i);
            return false;
        }
        
        wgpuBuffers.push_back(nativeHandle.as<WGPUCommandBuffer>());
    }
    
    // Submit batch to queue
    wgpuQueueSubmit(_queue, count, wgpuBuffers.data());
    
    Logger::debug("[WebGPUQueue] {} command buffers submitted", count);
    return true;
}

void WebGPUQueue::waitIdle() {
    if (!_queue) {
        return;
    }
    
    // WebGPU doesn't have a direct waitIdle for queues
    // We use onSubmittedWorkDone callback to wait
    
    NotImplemented::Log(
        "WebGPUQueue::waitIdle",
        "Implement queue synchronization with onSubmittedWorkDone",
        PERS_SOURCE_LOC
    );
    
    // TODO: Implementation steps:
    // 1. Set up a callback with wgpuQueueOnSubmittedWorkDone
    // 2. Use a condition variable or event to wait
    // 3. Signal when callback is invoked
}

NativeQueueHandle WebGPUQueue::getNativeQueueHandle() const {
    return NativeQueueHandle::fromBackend(_queue);
}

void WebGPUQueue::writeBuffer(WGPUBuffer buffer, uint64_t offset, const void* data, uint64_t size) {
    if (!_queue) {
        Logger::error("[WebGPUQueue] Cannot write buffer: queue is null");
        return;
    }
    
    if (!buffer || !data || size == 0) {
        Logger::error("[WebGPUQueue] Invalid buffer write parameters");
        return;
    }
    
    wgpuQueueWriteBuffer(_queue, buffer, offset, data, size);
    Logger::debug("[WebGPUQueue] Wrote {} bytes to buffer at offset {}", size, offset);
}

} // namespace pers