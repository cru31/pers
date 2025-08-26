#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <string>

namespace pers {
namespace graphics {

// Version information
struct Version {
    int major;
    int minor;
    int patch;
    
    std::string toString() const;
};

// Get library version
Version getVersion();

// Initialize graphics system
bool initialize();

// Shutdown graphics system  
void shutdown();

// Math utilities using GLM (always available)
glm::mat4 createPerspectiveMatrix(float fov, float aspect, float near, float far);

} // namespace graphics
} // namespace pers