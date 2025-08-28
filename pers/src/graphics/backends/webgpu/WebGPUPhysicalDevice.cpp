#include "graphics/backends/webgpu/WebGPUPhysicalDevice.h"
#include "pers/utils/NotImplemented.h"
#include <webgpu.h>
#include <iostream>
#include <cstring>
#include <vector>

namespace pers {

WebGPUPhysicalDevice::WebGPUPhysicalDevice(WGPUAdapter adapter)
    : _adapter(adapter) {
    
    if (_adapter) {
        wgpuAdapterAddRef(_adapter);
        queryAdapterInfo(); // Query info once during construction
        std::cout << "[WebGPUPhysicalDevice] Created with adapter" << std::endl;
    }
}

WebGPUPhysicalDevice::~WebGPUPhysicalDevice() {
    if (_adapter) {
        wgpuAdapterInfoFreeMembers(_adapterInfo);
        wgpuAdapterRelease(_adapter);
        _adapter = nullptr;
    }
}

void WebGPUPhysicalDevice::queryAdapterInfo() {
    if (!_adapter) {
        return;
    }
    
    WGPUStatus status = wgpuAdapterGetInfo(_adapter, &_adapterInfo);
    if (status != WGPUStatus_Success) {
        std::cerr << "[WebGPUPhysicalDevice] Failed to get adapter info" << std::endl;
    }
}

PhysicalDeviceCapabilities WebGPUPhysicalDevice::getCapabilities() const {
    PhysicalDeviceCapabilities caps = {};
    
    if (!_adapter) {
        return caps;
    }
    
    // Fill basic info from adapter (already queried in constructor)
    // WGPUStringView has data and length members
    if (_adapterInfo.device.data) {
        caps.deviceName = std::string(_adapterInfo.device.data, _adapterInfo.device.length);
    }
    if (_adapterInfo.description.data) {
        caps.driverInfo = std::string(_adapterInfo.description.data, _adapterInfo.description.length);
    }
    
    // Query limits
    WGPULimits limits = {};
    WGPUStatus status = wgpuAdapterGetLimits(_adapter, &limits);
    if (status == WGPUStatus_Success) {
        caps.maxTextureSize2D = limits.maxTextureDimension2D;
        caps.maxTextureSize3D = limits.maxTextureDimension3D;
        caps.maxTextureLayers = limits.maxTextureArrayLayers;
    }
    
    // Query features
    WGPUSupportedFeatures supportedFeatures = {};
    wgpuAdapterGetFeatures(_adapter, &supportedFeatures);
    
    if (supportedFeatures.featureCount > 0 && supportedFeatures.features) {
        for (size_t i = 0; i < supportedFeatures.featureCount; ++i) {
            WGPUFeatureName feature = supportedFeatures.features[i];
            // Check for compute shader support
            // Note: All WebGPU adapters support compute shaders by default
            caps.supportsCompute = true;
            
            // Check for other features if needed
            if (feature == WGPUFeatureName_ShaderF16) {
                // F16 support detected
            }
        }
    } else {
        // WebGPU always supports compute
        caps.supportsCompute = true;
    }
    
    // Clean up features
    wgpuSupportedFeaturesFreeMembers(supportedFeatures);
    
    // WebGPU doesn't directly expose memory info
    // These would need platform-specific queries
    
    return caps;
}

std::vector<QueueFamily> WebGPUPhysicalDevice::getQueueFamilies() const {
    std::vector<QueueFamily> families;
    
    // WebGPU has a simplified queue model
    // All queues support all operations
    QueueFamily family;
    family.index = 0;
    family.queueCount = 1;
    family.supportsGraphics = true;
    family.supportsCompute = true;
    family.supportsTransfer = true;
    family.supportsSparse = false; // WebGPU doesn't support sparse resources
    
    families.push_back(family);
    
    return families;
}

bool WebGPUPhysicalDevice::supportsSurface(void* surface) const {
    if (!_adapter || !surface) {
        return false;
    }
    
    // WebGPU doesn't have a direct way to query surface support
    // Adapters requested with a compatible surface should support it
    // For now, we assume the adapter supports the surface
    return true;
}

std::shared_ptr<ILogicalDevice> WebGPUPhysicalDevice::createLogicalDevice(
    const LogicalDeviceDesc& desc) {
    
    NotImplemented::Log(
        "WebGPUPhysicalDevice::createLogicalDevice",
        "Create WebGPU device from adapter with requested features and limits",
        PERS_SOURCE_LOC
    );
    
    // TODO: Implementation steps:
    // 1. Build WGPUDeviceDescriptor from LogicalDeviceDesc
    // 2. Set up required features and limits
    // 3. Call wgpuAdapterRequestDevice with callback
    // 4. Wait for device callback
    // 5. Create and return WebGPULogicalDevice wrapping the device
    
    return nullptr;
}

void* WebGPUPhysicalDevice::getNativeHandle() const {
    return _adapter;
}

} // namespace pers