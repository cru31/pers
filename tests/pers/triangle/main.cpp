#include "PersTriangleApp.h"
#include "GLFWWindowFactory.h"
#include "pers/graphics/backends/webgpu/WebGPUBackendFactory.h"
#include <iostream>
#include <memory>

int main() {
    // Create factories
    auto windowFactory = std::make_shared<GLFWWindowFactory>();
    std::cout << "Using window system: " << windowFactory->getFactoryName() << std::endl;
    
    auto graphicsFactory = std::make_shared<pers::WebGPUBackendFactory>();
    std::cout << "Using graphics backend: " << graphicsFactory->getBackendName() << std::endl;
    
    // Create app and pass the factories
    PersTriangleApp app;
    if (!app.initialize(windowFactory, graphicsFactory)) {
        std::cerr << "Failed to initialize app" << std::endl;
        return -1;
    }
    
    app.run();
    
    return 0;
}