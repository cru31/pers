#include "graphics/backends/webgpu/WebGPULogicalDevice.h"
#include "graphics/backends/webgpu/WebGPUQueue.h"
#include "pers/utils/NotImplemented.h"
#include "pers/utils/Logger.h"
#include <iostream>
#include <cassert>

namespace pers {

WebGPULogicalDevice::WebGPULogicalDevice(WGPUDevice device, WGPUAdapter adapter)
    : _device(device), _adapter(adapter) {
    
    if (_device) {
        wgpuDeviceAddRef(_device);
        Logger::info("[WebGPULogicalDevice] Created with device");
        
        // Create default queue immediately
        if (!createDefaultQueue()) {
            Logger::error("[WebGPULogicalDevice] Failed to create default queue");
            // Device is still valid but queue creation failed
            // Caller should check if queue is available
        }
    } else {
        Logger::error("[WebGPULogicalDevice] Created with null device!");
    }
    
    if (_adapter) {
        wgpuAdapterAddRef(_adapter);
    }
}

WebGPULogicalDevice::~WebGPULogicalDevice() {
    _defaultQueue.reset();
    
    if (_device) {
        wgpuDeviceRelease(_device);
        _device = nullptr;
    }
    
    if (_adapter) {
        wgpuAdapterRelease(_adapter);
        _adapter = nullptr;
    }
}

bool WebGPULogicalDevice::createDefaultQueue() {
    if (!_device) {
        return false;
    }
    
    // WebGPU devices have a default queue
    WGPUQueue queue = wgpuDeviceGetQueue(_device);
    if (queue) {
        _defaultQueue = std::make_shared<WebGPUQueue>(queue);
        Logger::info("[WebGPULogicalDevice] Default queue created");
        return true;
    } else {
        Logger::error("[WebGPULogicalDevice] Failed to get default queue");
        return false;
    }
}

std::shared_ptr<IQueue> WebGPULogicalDevice::getQueue(uint32_t queueFamilyIndex, uint32_t queueIndex) {
    // WebGPU only has one queue
    if (queueFamilyIndex != 0 || queueIndex != 0) {
        Logger::error("[WebGPULogicalDevice] Invalid queue indices: family={}, index={}", 
                     queueFamilyIndex, queueIndex);
        return nullptr;
    }
    
    return _defaultQueue;
}

std::shared_ptr<IResourceFactory> WebGPULogicalDevice::getResourceFactory() {
    NotImplemented::Log(
        "WebGPULogicalDevice::getResourceFactory",
        "Create WebGPUResourceFactory for buffer/texture/shader creation",
        PERS_SOURCE_LOC
    );
    
    // TODO: Implementation steps:
    // 1. Create WebGPUResourceFactory class
    // 2. Pass _device to factory
    // 3. Return shared_ptr to factory instance
    
    return nullptr;
}

std::shared_ptr<ICommandEncoder> WebGPULogicalDevice::createCommandEncoder(
    const CommandEncoderDesc& desc) {
    
    NotImplemented::Log(
        "WebGPULogicalDevice::createCommandEncoder",
        "Create WebGPUCommandEncoder from device",
        PERS_SOURCE_LOC
    );
    
    // TODO: Implementation steps:
    // 1. Create WGPUCommandEncoderDescriptor from desc
    // 2. Call wgpuDeviceCreateCommandEncoder
    // 3. Wrap in WebGPUCommandEncoder class
    // 4. Return shared_ptr
    
    return nullptr;
}

std::shared_ptr<ISwapChain> WebGPULogicalDevice::createSwapChain(
    const NativeSurfaceHandle& surface,
    const SwapChainDesc& desc) {
    
    NotImplemented::Log(
        "WebGPULogicalDevice::createSwapChain",
        "Create WebGPU swap chain for surface presentation",
        PERS_SOURCE_LOC
    );
    
    // TODO: Implementation steps:
    // 1. Create WGPUSwapChainDescriptor from desc
    // 2. Configure format, usage, present mode
    // 3. Call wgpuDeviceCreateSwapChain
    // 4. Wrap in WebGPUSwapChain class
    // 5. Return shared_ptr
    
    return nullptr;
}

void WebGPULogicalDevice::waitIdle() {
    if (!_device) {
        return;
    }
    
    // WebGPU doesn't have a direct waitIdle equivalent
    // We can tick the device to process all pending operations
    wgpuDeviceTick(_device);
    
    // If we have a queue, wait for it to be idle
    if (_defaultQueue) {
        _defaultQueue->waitIdle();
    }
}

NativeDeviceHandle WebGPULogicalDevice::getNativeDeviceHandle() const {
    return NativeDeviceHandle::fromBackend(_device);
}

} // namespace pers