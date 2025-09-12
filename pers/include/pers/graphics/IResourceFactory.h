#pragma once

#include <memory>
#include <string>
#include <vector>
#include <cstdint>
#include "pers/graphics/GraphicsTypes.h"
#include "pers/graphics/GraphicsFormats.h"
#include "pers/graphics/IShaderModule.h"
#include "pers/graphics/buffers/IBuffer.h"  // Include for BufferDesc
#include "pers/graphics/IRenderPipeline.h"  // Include for RenderPipelineDesc
#include "pers/graphics/ITexture.h"  // Include for TextureDesc

namespace pers {

// Forward declarations
class ITexture;
class ITextureView;
class ISampler;
class IRenderPipeline;
class IComputePipeline;
class IBindGroupLayout;
class IBindGroup;
class IPipelineLayout;
class IFramebuffer;
class IMappableBuffer;

/**
 * @brief Texture view descriptor
 */
struct TextureViewDesc {
    TextureFormat format = TextureFormat::Undefined;
    TextureViewDimension dimension = TextureViewDimension::D2;
    uint32_t baseMipLevel = 0;
    uint32_t mipLevelCount = 1;
    uint32_t baseArrayLayer = 0;
    uint32_t arrayLayerCount = 1;
    TextureAspect aspect = TextureAspect::All;
    std::string label;
};

/**
 * @brief Sampler descriptor
 */
struct SamplerDesc {
    FilterMode magFilter = FilterMode::Linear;
    FilterMode minFilter = FilterMode::Linear;
    FilterMode mipmapFilter = FilterMode::Linear;
    AddressMode addressModeU = AddressMode::ClampToEdge;
    AddressMode addressModeV = AddressMode::ClampToEdge;
    AddressMode addressModeW = AddressMode::ClampToEdge;
    float lodMinClamp = 0.0f;
    float lodMaxClamp = 1000.0f;
    CompareFunction compare = CompareFunction::Undefined;
    uint16_t maxAnisotropy = 1;
    std::string label;
};

// ShaderModuleDesc is defined in IShaderModule.h

/**
 * @brief Resource factory interface for creating GPU resources
 * 
 * Factory for creating buffers, textures, shaders, and other GPU resources.
 * All resource creation goes through this interface.
 */
class IResourceFactory {
public:
    virtual ~IResourceFactory() = default;
    
    /**
     * @brief Create a buffer
     * @param desc Buffer descriptor
     * @return Shared pointer to buffer or nullptr if failed
     */
    virtual std::shared_ptr<IBuffer> createBuffer(const BufferDesc& desc) const = 0;
    /**
     * @brief Create a buffer with initial data written synchronously
     * Uses mappedAtCreation internally for efficient initialization
     * @param desc Buffer description
     * @param initialData Pointer to initial data
     * @param dataSize Size of initial data in bytes
     * @return Created buffer with data already uploaded
     */
    virtual std::shared_ptr<IBuffer> createInitializableDeviceBuffer(
        const BufferDesc& desc,
        const void* initialData,
        size_t dataSize) const = 0;
    
    /**
     * @brief Create a texture
     * @param desc Texture descriptor
     * @return Shared pointer to texture or nullptr if failed
     */
    virtual std::shared_ptr<ITexture> createTexture(const TextureDesc& desc) const = 0;
    
    /**
     * @brief Create a texture view
     * @param texture Source texture
     * @param desc Texture view descriptor
     * @return Shared pointer to texture view or nullptr if failed
     */
    virtual std::shared_ptr<ITextureView> createTextureView(
        const std::shared_ptr<ITexture>& texture,
        const TextureViewDesc& desc) const = 0;
    
    /**
     * @brief Create a sampler
     * @param desc Sampler descriptor
     * @return Shared pointer to sampler or nullptr if failed
     */
    virtual std::shared_ptr<ISampler> createSampler(const SamplerDesc& desc) const = 0;
    
    /**
     * @brief Create a shader module
     * @param desc Shader module descriptor
     * @return Shared pointer to shader module or nullptr if failed
     */
    virtual std::shared_ptr<IShaderModule> createShaderModule(const ShaderModuleDesc& desc) const = 0;
    
    /**
     * @brief Create a render pipeline
     * @param desc Render pipeline descriptor
     * @return Shared pointer to render pipeline or nullptr if failed
     */
    virtual std::shared_ptr<IRenderPipeline> createRenderPipeline(const RenderPipelineDesc& desc) const = 0;
    /**
     * @brief Create a mappable buffer for CPU-GPU data transfer
     * @param desc Buffer descriptor
     * @return Shared pointer to mappable buffer or nullptr if failed
     */
    virtual std::shared_ptr<IMappableBuffer> createMappableBuffer(const BufferDesc& desc) const = 0;
    
};

} // namespace pers