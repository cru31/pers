#pragma once
#include "../test_handler_base.h"
#include <pers/graphics/backends/webgpu/WebGPUBackendFactory.h>
#include <pers/graphics/IInstance.h>
#include <pers/graphics/IPhysicalDevice.h>
#include <pers/graphics/ILogicalDevice.h>
#include <pers/graphics/ISwapChain.h>
#include <pers/core/platform/NativeWindowHandle.h>

// Forward declare GLFW types
struct GLFWwindow;

namespace pers::tests {

class SwapChainBuilderNegotiationHandler : public TestHandlerBase {
private:
    std::shared_ptr<WebGPUBackendFactory> _factory;
    std::shared_ptr<IInstance> _instance;
    std::shared_ptr<IPhysicalDevice> _physicalDevice;
    std::shared_ptr<ILogicalDevice> _logicalDevice;
    GLFWwindow* _window = nullptr;
    NativeSurfaceHandle _surface;
    std::shared_ptr<ISwapChain> _swapChain;
    
    bool initializeDevice();
    
    // Helper functions
    TextureFormat stringToTextureFormat(const std::string& str);
    PresentMode stringToPresentMode(const std::string& str);
    CompositeAlphaMode stringToAlphaMode(const std::string& str);
    
public:
    SwapChainBuilderNegotiationHandler();
    ~SwapChainBuilderNegotiationHandler();
    std::string getTestType() const override;
    TestResult execute(const TestVariation& variation) override;
};

} // namespace pers::tests