#include "pers/graphics/backends/webgpu/WebGPULogicalDevice.h"
#include "pers/graphics/backends/webgpu/WebGPUQueue.h"
#include "pers/utils/NotImplemented.h"
#include "pers/utils/Logger.h"
#include <webgpu.h>
#include <iostream>

namespace pers {

WebGPULogicalDevice::WebGPULogicalDevice(WGPUDevice device, WGPUAdapter adapter)
    : _device(device), _adapter(adapter) {
    
    if (_device) {
        wgpuDeviceAddRef(_device);
        Logger::Instance().Log(LogLevel::Info, "WebGPULogicalDevice", "Created with device", PERS_SOURCE_LOC);
        
        // Create default queue immediately
        if (!createDefaultQueue()) {
            Logger::Instance().Log(LogLevel::Error, "WebGPULogicalDevice", "Failed to create default queue", PERS_SOURCE_LOC);
            // Device is still valid but queue creation failed
            // Caller should check if queue is available
        }
    } else {
        Logger::Instance().Log(LogLevel::Error, "WebGPULogicalDevice", "Created with null device!", PERS_SOURCE_LOC);
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
        Logger::Instance().Log(LogLevel::Info, "WebGPULogicalDevice", "Default queue created", PERS_SOURCE_LOC);
        return true;
    } else {
        Logger::Instance().Log(LogLevel::Error, "WebGPULogicalDevice", "Failed to get default queue", PERS_SOURCE_LOC);
        return false;
    }
}

std::shared_ptr<IQueue> WebGPULogicalDevice::getQueue() const {
    return _defaultQueue;
}

std::shared_ptr<IResourceFactory> WebGPULogicalDevice::getResourceFactory() const {
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

std::shared_ptr<ICommandEncoder> WebGPULogicalDevice::createCommandEncoder() {
    
    NotImplemented::Log(
        "WebGPULogicalDevice::createCommandEncoder",
        "Create WebGPUCommandEncoder from device",
        PERS_SOURCE_LOC
    );
    
    // TODO: Implementation steps:
    // 1. Create WGPUCommandEncoderDescriptor
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
    
    // WebGPU doesn't have a direct waitIdle equivalent like Vulkan's vkDeviceWaitIdle
    // We need to poll the device to process callbacks and wait for queue completion
    
    // Process any pending callbacks
    // Note: wgpuDevicePoll is the standard WebGPU way to process callbacks
    // but wgpu-native uses a different approach
    
    // For wgpu-native, we wait for the queue to complete all work
    if (_defaultQueue) {
        _defaultQueue->waitIdle();
    }
    
    // TODO: Investigate if wgpu-native has a device poll mechanism
    // Standard WebGPU would use: wgpuDevicePoll(_device, true, nullptr);
}

NativeDeviceHandle WebGPULogicalDevice::getNativeDeviceHandle() const {
    return NativeDeviceHandle::fromBackend(_device);
}

} // namespace pers