#pragma once
#include "../test_handler_base.h"
#include <pers/graphics/backends/webgpu/WebGPUBackendFactory.h>
#include <pers/graphics/IInstance.h>

namespace pers::tests {

class RequestAdapterHandler : public TestHandlerBase {
private:
    std::shared_ptr<WebGPUBackendFactory> _factory;
    std::shared_ptr<IInstance> _instance;
    
    bool initializeInstance();
    
public:
    RequestAdapterHandler();
    std::string getTestType() const override;
    TestResult execute(const TestVariation& variation) override;
};

} // namespace pers::tests