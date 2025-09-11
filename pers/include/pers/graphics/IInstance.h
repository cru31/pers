#pragma once

#include <memory>
#include <string>
#include "pers/graphics/GraphicsTypes.h"

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
    
    /**
     * @brief Optional: Request an adapter that can render to this surface
     * 
     * ## Use Cases:
     * 
     * ### 1. Standard Rendering Pipeline (Most Common)
     * ```cpp
     * // Step 1: Create surface from window
     * auto surface = instance->createSurface(window);
     * 
     * // Step 2: Request adapter compatible with the surface
     * PhysicalDeviceOptions options;
     * options.compatibleSurface = surface;  //  Ensures GPU can render to this window
     * auto adapter = instance->requestPhysicalDevice(options);
     * 
     * // Step 3: Create device and swap chain
     * auto device = adapter->createLogicalDevice({});
     * auto swapChain = device->createSwapChain(surface, {});
     * ```
     * 
     * ### 2. Headless/Offscreen Rendering
     * ```cpp
     * PhysicalDeviceOptions options;
     * // Don't set compatibleSurface - no window output needed
     * options.powerPreference = PowerPreference::HighPerformance;
     * auto adapter = instance->requestPhysicalDevice(options);
     * ```
     * 
     * ### 3. Multi-Window Applications
     * ```cpp
     * for (auto& window : windows) {
     *     auto surface = instance->createSurface(window);
     *     
     *     PhysicalDeviceOptions options;
     *     options.compatibleSurface = surface;
     *     // Each window gets the most suitable GPU
     *     auto adapter = instance->requestPhysicalDevice(options);
     * }
     * ```
     * 
     * ## Why This Is Important:
     * 
     * In multi-GPU systems (e.g., laptop with Intel iGPU + NVIDIA dGPU):
     * - Different GPUs may be connected to different displays
     * - Rendering on wrong GPU requires expensive cross-GPU copies
     * - WebGPU automatically selects the optimal GPU for the given surface
     * 
     * Example scenario:
     * - Laptop screen connected to Intel iGPU
     * - External monitor connected to NVIDIA dGPU
     * - compatibleSurface ensures the right GPU is selected for each display
     * 
     * @note Leave empty (nullptr) for offscreen rendering or when surface compatibility doesn't matter
     */
    NativeSurfaceHandle compatibleSurface;
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
     * @return Native surface handle or nullptr if failed
     */
    virtual NativeSurfaceHandle createSurface(void* windowHandle) = 0;
};

} // namespace pers