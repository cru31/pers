#pragma once

#include <memory>
#include <string>

namespace pers {

enum class ShaderStage : uint32_t {
    None = 0,
    Vertex = 1,
    Fragment = 2,
    Compute = 4
};

struct ShaderModuleDesc {
    std::string code;
    ShaderStage stage = ShaderStage::None;  // Auto-detect from code if None
    std::string entryPoint = "main";         // Smart default
    std::string debugName;                   // Optional
};

class IShaderModule {
public:
    virtual ~IShaderModule() = default;
    
    virtual ShaderStage getStage() const = 0;
    virtual const std::string& getEntryPoint() const = 0;
    virtual const std::string& getDebugName() const = 0;
    virtual bool isValid() const = 0;
};

} // namespace pers