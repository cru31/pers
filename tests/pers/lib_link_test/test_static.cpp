#include <pers/graphics/core.h>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>

int main() {
    std::cout << "=== Testing pers_static.lib ===" << std::endl;
    
    // Test version
    auto version = pers::graphics::getVersion();
    std::cout << "Pers Engine Version: " << version.toString() << std::endl;
    
    // Test initialization
    if (pers::graphics::initialize()) {
        std::cout << "Graphics system initialized successfully" << std::endl;
    } else {
        std::cerr << "Failed to initialize graphics system" << std::endl;
        return 1;
    }
    
    // Test GLM integration
    glm::mat4 projMatrix = pers::graphics::createPerspectiveMatrix(
        45.0f,  // FOV
        16.0f/9.0f,  // Aspect ratio
        0.1f,   // Near
        100.0f  // Far
    );
    
    std::cout << "Created perspective matrix:" << std::endl;
    std::cout << "  [" << projMatrix[0][0] << ", " << projMatrix[1][0] 
              << ", " << projMatrix[2][0] << ", " << projMatrix[3][0] << "]" << std::endl;
    
    // Test GLM directly
    glm::vec3 testVec(1.0f, 2.0f, 3.0f);
    glm::mat4 identity = glm::mat4(1.0f);
    glm::vec4 transformed = identity * glm::vec4(testVec, 1.0f);
    std::cout << "GLM Vector test: (" << transformed.x << ", " 
              << transformed.y << ", " << transformed.z << ")" << std::endl;
    
    // Shutdown
    pers::graphics::shutdown();
    std::cout << "Graphics system shutdown complete" << std::endl;
    
    std::cout << "=== Static library test PASSED ===" << std::endl;
    return 0;
}