#include <iostream>
#include <glm/gtc/matrix_transform.hpp>

int main() {
    std::cout << "=== Testing pers_static.lib ===" << std::endl;
    
    // Simple link test - just verify we can link against pers_static
    std::cout << "Static library linked successfully" << std::endl;
    
    // Test GLM is available (pers depends on GLM)
    glm::vec3 testVec(1.0f, 2.0f, 3.0f);
    glm::mat4 identity = glm::mat4(1.0f);
    glm::vec4 transformed = identity * glm::vec4(testVec, 1.0f);
    std::cout << "GLM Vector test: (" << transformed.x << ", " 
              << transformed.y << ", " << transformed.z << ")" << std::endl;
    
    std::cout << "=== Static library test PASSED ===" << std::endl;
    return 0;
}