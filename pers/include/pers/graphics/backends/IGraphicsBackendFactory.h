#pragma once

#include <memory>
#include <string>
#include <vector>

namespace pers {

class IInstance;

/**
 * @brief Instance descriptor for creating graphics instance
 * 
 * Contains parameters common to all graphics APIs:
 * - WebGPU: Used for instance creation and adapter request
 * - Vulkan: Used for VkApplicationInfo and instance layers
 * - Metal: Used for device selection and debugging
 * - D3D12: Used for debug layer and adapter selection
 */
struct InstanceDesc {
    // Application info (used by all APIs for debugging/profiling)
    std::string applicationName = "Pers Application";
    uint32_t applicationVersion = 1;
    std::string engineName = "Pers Graphics Engine";
    uint32_t engineVersion = 1;
    
    // Debugging and validation
    bool enableValidation = true;        // Vulkan validation layers, D3D12 debug layer, WebGPU validation
    bool enableGPUBasedValidation = false; // GPU-assisted validation (Vulkan, D3D12)
    bool enableSynchronizationValidation = false; // Thread safety validation
    
    // Performance and features
    bool preferHighPerformanceGPU = true; // Prefer discrete GPU over integrated
    bool allowSoftwareRenderer = false;   // Allow CPU fallback (SwiftShader, WARP, etc.)
    
    // Required extensions/features (API-specific, but common patterns)
    std::vector<std::string> requiredExtensions;  // Instance extensions for Vulkan
    std::vector<std::string> optionalExtensions;  // Nice-to-have extensions
    
    // API version hints (for backends that support multiple versions)
    uint32_t apiVersionMajor = 0; // 0 = use latest available
    uint32_t apiVersionMinor = 0;
    uint32_t apiVersionPatch = 0;
};

/**
 * @brief Graphics backend factory interface
 * 
 * Each graphics backend implements this interface.
 * The factory is responsible for creating backend-specific instances.
 */
class IGraphicsBackendFactory {
public:
    virtual ~IGraphicsBackendFactory() = default;
    
    /**
     * @brief Create a graphics instance
     * @param desc Instance descriptor
     * @return Shared pointer to instance or nullptr if failed
     */
    virtual std::shared_ptr<IInstance> createInstance(
        const InstanceDesc& desc) = 0;
    
    /**
     * @brief Get the backend name for debugging
     * @return Backend name string reference
     */
    virtual const std::string& getBackendName() const = 0;
};

} // namespace pers