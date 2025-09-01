#pragma once

#include "pers/graphics/IRenderPipeline.h"
#include <webgpu/webgpu.h>
#include <string>

namespace pers {
namespace webgpu {

class WebGPURenderPipeline final : public IRenderPipeline {
public:
    WebGPURenderPipeline(const RenderPipelineDesc& desc, WGPUDevice device);
    ~WebGPURenderPipeline() override;
    
    // IRenderPipeline interface
    const std::string& getDebugName() const override;
    bool isValid() const override;
    
    // WebGPU specific - internal use only
    WGPURenderPipeline getNativeHandle() const;
    
private:
    std::string _debugName;
    WGPURenderPipeline _pipeline = nullptr;
};

} // namespace webgpu
} // namespace pers