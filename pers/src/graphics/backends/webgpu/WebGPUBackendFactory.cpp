#include "pers/graphics/backends/webgpu/WebGPUBackendFactory.h"
#include "pers/graphics/backends/webgpu/WebGPUInstance.h"
#include <iostream>

namespace pers {

std::shared_ptr<IInstance> WebGPUBackendFactory::createInstance(
    const InstanceDesc& desc) {
    
    std::cout << "[WebGPUBackendFactory] Creating WebGPU instance..." << std::endl;
    
    auto instance = std::make_shared<WebGPUInstance>();
    if (!instance->initialize(desc)) {
        std::cerr << "[WebGPUBackendFactory] Failed to initialize WebGPU instance" << std::endl;
        return nullptr;
    }
    
    return instance;
}

const std::string& WebGPUBackendFactory::getBackendName() const {
    static const std::string name = "WebGPU";
    return name;
}

} // namespace pers