#include <pers/graphics/core.h>
#include <sstream>

namespace pers {
namespace graphics {

std::string Version::toString() const {
    std::stringstream ss;
    ss << major << "." << minor << "." << patch;
    return ss.str();
}

Version getVersion() {
    return Version{0, 1, 0};
}

bool initialize() {
    // Graphics system initialization
    // TODO: Initialize WebGPU/Dawn when available
    return true;
}

void shutdown() {
    // Graphics system shutdown
    // TODO: Cleanup WebGPU/Dawn resources
}

glm::mat4 createPerspectiveMatrix(float fov, float aspect, float near, float far) {
    return glm::perspective(glm::radians(fov), aspect, near, far);
}

} // namespace graphics
} // namespace pers