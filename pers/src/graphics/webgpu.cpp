#include <pers/graphics/webgpu.h>
#include <iostream>

// wgpu-native C API
extern "C" {
    // Forward declarations for wgpu-native C API
    typedef struct WGPUInstanceImpl* WGPUInstance;
    typedef struct WGPUAdapterImpl* WGPUAdapter;
    
    WGPUInstance wgpuCreateInstance(const void* descriptor);
    void wgpuInstanceRelease(WGPUInstance instance);
}

namespace pers {
namespace graphics {
namespace webgpu {

class WebGPUContext::Impl {
public:
    bool initialized = false;
    WGPUInstance instance = nullptr;
    WGPUAdapter adapter = nullptr;
};

WebGPUContext::WebGPUContext() : _impl(std::make_unique<Impl>()) {
}

WebGPUContext::~WebGPUContext() {
    shutdown();
}

bool WebGPUContext::initialize(const WebGPUConfig& config) {
    if (_impl->initialized) {
        return true;
    }
    
    // Create WebGPU instance
    _impl->instance = wgpuCreateInstance(nullptr);
    if (!_impl->instance) {
        std::cerr << "Failed to create WebGPU instance" << std::endl;
        return false;
    }
    
    _impl->initialized = true;
    std::cout << "WebGPU context initialized: " << config.applicationName << std::endl;
    return true;
}

void WebGPUContext::shutdown() {
    if (!_impl->initialized) {
        return;
    }
    
    if (_impl->instance) {
        wgpuInstanceRelease(_impl->instance);
        _impl->instance = nullptr;
    }
    
    _impl->initialized = false;
    std::cout << "WebGPU context shutdown" << std::endl;
}

bool WebGPUContext::isInitialized() const {
    return _impl->initialized;
}

std::string WebGPUContext::getAdapterInfo() const {
    if (_impl->initialized && _impl->adapter) {
        return "WebGPU Adapter (wgpu-native)";
    }
    return _impl->initialized ? "WebGPU Available but no adapter" : "WebGPU Not Initialized";
}

bool testWebGPUAvailable() {
    return true;  // WebGPU is always available (required dependency)
}

} // namespace webgpu
} // namespace graphics
} // namespace pers