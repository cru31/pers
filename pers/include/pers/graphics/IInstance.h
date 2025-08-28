#pragma once

#include <memory>
#include <string>

namespace pers {

// Forward declarations
class IPhysicalDevice;

/**
 * @brief Physical device power preference
 */
enum class PowerPreference {
    LowPower,
    HighPerformance,
    Default
};

/**
 * @brief Physical device request options
 */
struct PhysicalDeviceOptions {
    PowerPreference powerPreference = PowerPreference::Default;
    bool forceFallbackAdapter = false;
};

/**
 * @brief Graphics instance interface
 * 
 * The instance is the entry point for the graphics API.
 * It's responsible for creating physical devices and surfaces.
 * Backend-agnostic interface.
 */
class IInstance {
public:
    virtual ~IInstance() = default;
    
    /**
     * @brief Request a physical device (adapter)
     * @param options Physical device options
     * @return Shared pointer to physical device or nullptr if failed
     */
    virtual std::shared_ptr<IPhysicalDevice> requestPhysicalDevice(
        const PhysicalDeviceOptions& options) = 0;
    
    /**
     * @brief Create a surface from a native window handle
     * @param windowHandle Native window handle (GLFWwindow*, HWND, etc.)
     * @return Opaque surface handle or nullptr if failed
     */
    virtual void* createSurface(void* windowHandle) = 0;
};

} // namespace pers