#pragma once
#include "../test_handler_base.h"
#include <pers/graphics/backends/webgpu/WebGPUBackendFactory.h>
#include <pers/graphics/IInstance.h>
#include <pers/graphics/IPhysicalDevice.h>

namespace pers::tests {

class DeviceCreationHandler : public TestHandlerBase {
private:
    std::shared_ptr<WebGPUBackendFactory> _factory;
    std::shared_ptr<IInstance> _instance;
    std::shared_ptr<IPhysicalDevice> _adapter;
    
    bool initializeAdapter();
    
public:
    DeviceCreationHandler();
    std::string getTestType() const override;
    TestResult execute(const TestVariation& variation) override;
};

} // namespace pers::tests