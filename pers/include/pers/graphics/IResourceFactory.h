#pragma once

#include <memory>
#include <string>
#include <vector>
#include <cstdint>
#include "pers/graphics/GraphicsTypes.h"

namespace pers {

// Forward declarations
class IBuffer;
class ITexture;
class ITextureView;
class ISampler;
class IShaderModule;
class IRenderPipeline;
class IComputePipeline;
class IBindGroupLayout;
class IBindGroup;
class IPipelineLayout;

/**
 * @brief Buffer descriptor for creation
 */
struct BufferDesc {
    uint64_t size = 0;
    BufferUsage usage = BufferUsage::None;
    bool mappedAtCreation = false;
    std::string label;
};

/**
 * @brief Texture descriptor for creation
 */
struct TextureDesc {
    uint32_t width = 1;
    uint32_t height = 1;
    uint32_t depth = 1;
    uint32_t mipLevelCount = 1;
    uint32_t sampleCount = 1;
    TextureDimension dimension = TextureDimension::D2;
    TextureFormat format = TextureFormat::RGBA8Unorm;
    TextureUsage usage = TextureUsage::None;
    std::string label;
};

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

/**
 * @brief Shader module descriptor
 */
struct ShaderModuleDesc {
    std::vector<uint8_t> code;  // SPIR-V or WGSL code
    ShaderStage stage = ShaderStage::Vertex;
    std::string entryPoint = "main";
    std::string label;
};

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
    virtual std::shared_ptr<IBuffer> createBuffer(const BufferDesc& desc) = 0;
    
    /**
     * @brief Create a texture
     * @param desc Texture descriptor
     * @return Shared pointer to texture or nullptr if failed
     */
    virtual std::shared_ptr<ITexture> createTexture(const TextureDesc& desc) = 0;
    
    /**
     * @brief Create a texture view
     * @param texture Source texture
     * @param desc Texture view descriptor
     * @return Shared pointer to texture view or nullptr if failed
     */
    virtual std::shared_ptr<ITextureView> createTextureView(
        std::shared_ptr<ITexture> texture,
        const TextureViewDesc& desc) = 0;
    
    /**
     * @brief Create a sampler
     * @param desc Sampler descriptor
     * @return Shared pointer to sampler or nullptr if failed
     */
    virtual std::shared_ptr<ISampler> createSampler(const SamplerDesc& desc) = 0;
    
    /**
     * @brief Create a shader module
     * @param desc Shader module descriptor
     * @return Shared pointer to shader module or nullptr if failed
     */
    virtual std::shared_ptr<IShaderModule> createShaderModule(const ShaderModuleDesc& desc) = 0;
    
    /**
     * @brief Get native factory handle for backend-specific operations
     * @return Native factory handle (implementation-specific)
     */
    virtual NativeResourceFactoryHandle getNativeFactoryHandle() const = 0;
};

} // namespace pers