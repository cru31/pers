#pragma once

#include "pers/graphics/IRenderPipeline.h"
#include <memory>

namespace pers {
namespace webgpu {

class WebGPURenderPipeline final : public IRenderPipeline {
public:
    WebGPURenderPipeline(const RenderPipelineDesc& desc, void* device);
    ~WebGPURenderPipeline() override;
    
    // IRenderPipeline interface
    const std::string& getDebugName() const override;
    bool isValid() const override;
    
    // WebGPU specific - internal use only
    void* getNativeHandle() const;
    
private:
    class Impl;
    std::unique_ptr<Impl> _impl;
};

} // namespace webgpu
} // namespace pers