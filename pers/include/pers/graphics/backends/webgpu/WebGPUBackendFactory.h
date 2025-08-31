#pragma once

#include "pers/graphics/backends/IGraphicsBackendFactory.h"

namespace pers {

/**
 * @brief WebGPU backend factory implementation
 */
class WebGPUBackendFactory : public IGraphicsBackendFactory {
public:
    WebGPUBackendFactory() = default;
    ~WebGPUBackendFactory() override = default;
    
    /**
     * @brief Create a WebGPU instance
     * @param desc Instance descriptor
     * @return Shared pointer to WebGPU instance
     */
    std::shared_ptr<IInstance> createInstance(
        const InstanceDesc& desc) override;
    
    /**
     * @brief Get the backend name
     * @return "WebGPU"
     */
    const std::string& getBackendName() const override;
};

} // namespace pers