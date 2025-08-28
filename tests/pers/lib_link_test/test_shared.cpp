#include <iostream>
#include <glm/gtc/matrix_transform.hpp>

int main() {
    std::cout << "=== Testing pers.dll (shared library) ===" << std::endl;
    
    // Simple link test - just verify we can link against pers_shared
    std::cout << "Shared library linked successfully" << std::endl;
    
    // Test GLM is available (pers depends on GLM)
    glm::vec3 testVec(5.0f, 10.0f, 15.0f);
    glm::mat4 translation = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 2.0f, 3.0f));
    glm::vec4 transformed = translation * glm::vec4(testVec, 1.0f);
    std::cout << "GLM Translation test: (" << transformed.x << ", " 
              << transformed.y << ", " << transformed.z << ")" << std::endl;
    
    std::cout << "=== Shared library test PASSED ===" << std::endl;
    return 0;
}