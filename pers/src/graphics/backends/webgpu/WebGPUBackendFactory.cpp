#include "pers/graphics/backends/webgpu/WebGPUBackendFactory.h"
#include "pers/graphics/backends/webgpu/WebGPUInstance.h"
#include "pers/utils/Logger.h"

namespace pers {

std::shared_ptr<IInstance> WebGPUBackendFactory::createInstance(
    const InstanceDesc& desc) {
    
    LOG_INFO("WebGPUBackendFactory", "Creating WebGPU instance...");
    
    // Test TodoOrDie for NYI categorization
    TODO_OR_DIE("WebGPUBackendFactory::createInstance", "Intentional TodoOrDie for testing NYI categorization");
    
    auto instance = std::make_shared<WebGPUInstance>();
    if (!instance->initialize(desc)) {
        LOG_ERROR("WebGPUBackendFactory", "Failed to initialize WebGPU instance");
        return nullptr;
    }
    
    return instance;
}

const std::string& WebGPUBackendFactory::getBackendName() const {
    static const std::string name = "WebGPU";
    return name;
}

} // namespace pers