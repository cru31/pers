#pragma once

#include <string>
#include "pers/graphics/GraphicsTypes.h"
#include "pers/graphics/GraphicsFormats.h"
#include "pers/graphics/SwapChainTypes.h"
#include "pers/graphics/IPhysicalDevice.h"

namespace pers {

/**
 * @brief Utility class for converting graphics enums to strings
 * 
 * This class provides static methods to convert various graphics enums
 * to their string representations for debugging and logging purposes.
 */
class GraphicsEnumStrings {
public:
    /**
     * @brief Convert TextureFormat enum to string
     * @param format The texture format to convert
     * @return String representation of the format
     */
    static std::string toString(TextureFormat format);
    
    /**
     * @brief Convert PresentMode enum to string
     * @param mode The present mode to convert
     * @return String representation of the mode
     */
    static std::string toString(PresentMode mode);
    
    /**
     * @brief Convert CompositeAlphaMode enum to string
     * @param mode The composite alpha mode to convert
     * @return String representation of the mode
     */
    static std::string toString(CompositeAlphaMode mode);
    
    /**
     * @brief Convert DeviceFeature enum to string
     * @param feature The device feature to convert
     * @return String representation of the feature
     */
    static std::string toString(DeviceFeature feature);
    
    /**
     * @brief Convert TextureUsage flags to string
     * @param usage The texture usage flags
     * @return String representation of the usage flags
     */
    static std::string toString(TextureUsage usage);
    
private:
    // Prevent instantiation
    GraphicsEnumStrings() = delete;
    ~GraphicsEnumStrings() = delete;
};

} // namespace pers