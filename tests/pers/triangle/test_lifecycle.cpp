#include "PersTriangleApp.h"
#include "GLFWWindow.h"
#include "pers/graphics/backends/webgpu/WebGPUInstanceFactory.h"
#include <iostream>
#include <memory>

int main() {
    std::cout << "=== Lifecycle Test Start ===" << std::endl;
    
    // Create the graphics backend factory
    auto factory = std::make_shared<pers::WebGPUInstanceFactory>();
    std::cout << "[main] Created factory: " << factory->getBackendName() << std::endl;
    
    // Create the window
    auto window = std::make_unique<GLFWWindow>();
    if (!window->create(800, 600, "Lifecycle Test")) {
        std::cerr << "Failed to create window" << std::endl;
        return -1;
    }
    std::cout << "[main] Created window" << std::endl;
    
    // Create app scope
    {
        PersTriangleApp app;
        std::cout << "[main] Created app" << std::endl;
        
        if (!app.initialize(std::move(window), factory)) {
            std::cerr << "Failed to initialize app" << std::endl;
            return -1;
        }
        std::cout << "[main] App initialized" << std::endl;
        
        // Immediately exit to see cleanup
        std::cout << "\n=== Starting Cleanup ===" << std::endl;
    } // App destructor called here
    std::cout << "[main] App destroyed" << std::endl;
    
    // Factory still alive
    std::cout << "[main] Factory still alive: " << factory.use_count() << " references" << std::endl;
    factory.reset();
    std::cout << "[main] Factory released" << std::endl;
    
    std::cout << "=== Lifecycle Test End ===" << std::endl;
    return 0;
}