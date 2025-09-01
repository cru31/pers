#pragma once

#include "pers/graphics/IShaderModule.h"
#include <memory>

namespace pers {
namespace webgpu {

class WebGPUShaderModule final : public IShaderModule {
public:
    WebGPUShaderModule(const ShaderModuleDesc& desc);
    ~WebGPUShaderModule() override;
    
    // IShaderModule interface
    ShaderStage getStage() const override;
    const std::string& getEntryPoint() const override;
    const std::string& getDebugName() const override;
    bool isValid() const override;
    
    // WebGPU specific - internal use only
    void* getNativeHandle() const;
    void createShaderModule(void* device);
    
private:
    class Impl;
    std::unique_ptr<Impl> _impl;
};

} // namespace webgpu
} // namespace pers