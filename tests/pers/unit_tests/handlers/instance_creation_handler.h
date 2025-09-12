#pragma once
#include "../test_handler_base.h"
#include <pers/graphics/backends/webgpu/WebGPUInstanceFactory.h>

namespace pers::tests {

class InstanceCreationHandler : public TestHandlerBase {
private:
    std::shared_ptr<WebGPUInstanceFactory> _factory;
    
public:
    InstanceCreationHandler();
    std::string getTestType() const override;
    TestResult execute(const TestVariation& variation) override;
};

} // namespace pers::tests