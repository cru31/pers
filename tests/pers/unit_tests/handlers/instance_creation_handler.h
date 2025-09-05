#pragma once
#include "../test_handler_base.h"
#include <pers/graphics/backends/webgpu/WebGPUBackendFactory.h>

namespace pers::tests {

class InstanceCreationHandler : public TestHandlerBase {
private:
    std::shared_ptr<WebGPUBackendFactory> _factory;
    
public:
    InstanceCreationHandler();
    std::string getTestType() const override;
    TestResult execute(const TestVariation& variation) override;
};

} // namespace pers::tests