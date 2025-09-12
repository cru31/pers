#include "pers/graphics/backends/webgpu/WebGPUInstanceFactory.h"
#include "pers/graphics/backends/webgpu/WebGPUInstance.h"
#include "pers/utils/Logger.h"

namespace pers {

std::shared_ptr<IInstance> WebGPUInstanceFactory::createInstance(
    const InstanceDesc& desc) {
    
    LOG_INFO("WebGPUInstanceFactory", "Creating WebGPU instance...");
    
    auto instance = std::make_shared<WebGPUInstance>();
    if (!instance->initialize(desc)) {
        LOG_ERROR("WebGPUInstanceFactory", "Failed to initialize WebGPU instance");
        return nullptr;
    }
    
    return instance;
}

const std::string& WebGPUInstanceFactory::getBackendName() const {
    static const std::string name = "WebGPU";
    return name;
}

} // namespace pers