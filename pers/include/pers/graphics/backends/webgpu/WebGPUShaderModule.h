#pragma once

#include "pers/graphics/IShaderModule.h"
#include <webgpu/webgpu.h>
#include <string>

namespace pers {

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
    WGPUShaderModule getNativeHandle() const;
    void createShaderModule(WGPUDevice device);
    
private:
    ShaderStage _stage;
    std::string _entryPoint;
    std::string _debugName;
    std::string _code;  // Store code until device is available
    WGPUShaderModule _shaderModule = nullptr;
};

} // namespace pers