#pragma once

#include <memory>
#include <string>

namespace pers {
namespace graphics {
namespace webgpu {

struct WebGPUConfig {
    bool enableValidation = true;
    std::string applicationName = "Pers Graphics Engine";
};

class WebGPUContext {
public:
    WebGPUContext();
    ~WebGPUContext();
    
    bool initialize(const WebGPUConfig& config = {});
    void shutdown();
    bool isInitialized() const;
    
    // Test function to verify WebGPU linkage
    std::string getAdapterInfo() const;
    
private:
    class Impl;
    std::unique_ptr<Impl> _impl;
};

// Simple test function to verify WebGPU is linked correctly
bool testWebGPUAvailable();

} // namespace webgpu
} // namespace graphics
} // namespace pers