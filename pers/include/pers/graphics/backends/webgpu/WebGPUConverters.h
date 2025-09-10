#pragma once

#include "pers/graphics/GraphicsFormats.h"
#include "pers/graphics/GraphicsTypes.h"
#include "pers/graphics/SwapChainTypes.h"
#include <webgpu/webgpu.h>

namespace pers {

/**
 * @brief Utility class for converting between Pers types and WebGPU types
 * 
 * Static helper functions for type conversions. No hardcoding,
 * all conversions based on input parameters.
 */
class WebGPUConverters {
public:
    // Texture format conversions
    static WGPUTextureFormat convertTextureFormat(TextureFormat format);
    static TextureFormat convertFromWGPUTextureFormat(WGPUTextureFormat format);
    
    // Present mode conversions
    static WGPUPresentMode convertPresentMode(PresentMode mode);
    static PresentMode convertFromWGPUPresentMode(WGPUPresentMode mode);
    
    // Composite alpha mode conversions
    static WGPUCompositeAlphaMode convertCompositeAlphaMode(CompositeAlphaMode mode);
    static CompositeAlphaMode convertFromWGPUCompositeAlphaMode(WGPUCompositeAlphaMode mode);
    
    // Load/Store operations
    static WGPULoadOp convertLoadOp(LoadOp op);
    static WGPUStoreOp convertStoreOp(StoreOp op);
    
    // Compare functions
    static WGPUCompareFunction convertCompareFunction(CompareFunction func);
    
    // Texture usage flags
    static WGPUTextureUsage convertTextureUsage(TextureUsage usage);
    
    // Buffer usage flags
    static WGPUBufferUsage convertBufferUsage(BufferUsage usage);
    
    // Texture dimension
    static WGPUTextureDimension convertTextureDimension(TextureDimension dimension);
    static WGPUTextureViewDimension convertTextureViewDimension(TextureViewDimension dimension);
    
    // Texture aspect
    static WGPUTextureAspect convertTextureAspect(TextureAspect aspect);
    
private:
    // Static utility class, no instantiation
    WebGPUConverters() = delete;
    ~WebGPUConverters() = delete;
};

} // namespace pers