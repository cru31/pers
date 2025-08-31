#include "pers/graphics/backends/webgpu/WebGPUBackendFactory.h"
#include "pers/graphics/backends/webgpu/WebGPUInstance.h"
#include "pers/utils/Logger.h"

namespace pers {

std::shared_ptr<IInstance> WebGPUBackendFactory::createInstance(
    const InstanceDesc& desc) {
    
    pers::Logger::Instance().Log(pers::LogLevel::Info, "WebGPUBackendFactory", "Creating WebGPU instance...", PERS_SOURCE_LOC);
    
    auto instance = std::make_shared<WebGPUInstance>();
    if (!instance->initialize(desc)) {
        pers::Logger::Instance().Log(pers::LogLevel::Error, "WebGPUBackendFactory", "Failed to initialize WebGPU instance", PERS_SOURCE_LOC);
        return nullptr;
    }
    
    return instance;
}

const std::string& WebGPUBackendFactory::getBackendName() const {
    static const std::string name = "WebGPU";
    return name;
}

} // namespace pers