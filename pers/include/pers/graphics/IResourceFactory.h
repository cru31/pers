#pragma once

#include <memory>
#include <string>
#include <vector>
#include <cstdint>
#include "pers/graphics/GraphicsTypes.h"
#include "pers/graphics/GraphicsFormats.h"
#include "pers/graphics/IShaderModule.h"
#include "pers/graphics/IBuffer.h"  // Include for BufferDesc
#include "pers/graphics/IRenderPipeline.h"  // Include for RenderPipelineDesc

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
class ISurfaceFramebuffer;
class IFramebuffer;

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
 * @brief Offscreen framebuffer descriptor
 */
struct OffscreenFramebufferDesc {
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t sampleCount = 1;  // 1 for no MSAA, 2/4/8 for MSAA
    std::vector<TextureFormat> colorFormats;  // Up to 8 for MRT
    TextureFormat depthFormat = TextureFormat::Undefined;  // Optional depth
    TextureUsage colorUsage = TextureUsage::RenderAttachment | TextureUsage::TextureBinding;
    TextureUsage depthUsage = TextureUsage::RenderAttachment;
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
        const std::shared_ptr<ITexture>& texture,
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
     * @brief Create a render pipeline
     * @param desc Render pipeline descriptor
     * @return Shared pointer to render pipeline or nullptr if failed
     */
    virtual std::shared_ptr<IRenderPipeline> createRenderPipeline(const RenderPipelineDesc& desc) = 0;
    
    /**
     * @brief Create a surface framebuffer for a swap chain surface
     * @param surface Native surface handle
     * @param width Initial width
     * @param height Initial height
     * @param format Surface format (typically BGRA8Unorm)
     * @return Shared pointer to surface framebuffer or nullptr if failed
     */
    virtual std::shared_ptr<ISurfaceFramebuffer> createSurfaceFramebuffer(
        const NativeSurfaceHandle& surface,
        uint32_t width,
        uint32_t height,
        TextureFormat format = TextureFormat::BGRA8Unorm) = 0;
    
    /**
     * @brief Create an offscreen framebuffer
     * @param desc Offscreen framebuffer descriptor
     * @return Shared pointer to framebuffer or nullptr if failed
     */
    virtual std::shared_ptr<IFramebuffer> createOffscreenFramebuffer(
        const OffscreenFramebufferDesc& desc) = 0;
    
    /**
     * @brief Create a depth-only framebuffer
     * @param width Width
     * @param height Height
     * @param format Depth format
     * @param sampleCount Sample count (1 for no MSAA)
     * @param usage Texture usage flags (externally configurable)
     * @return Shared pointer to framebuffer or nullptr if failed
     */
    virtual std::shared_ptr<IFramebuffer> createDepthOnlyFramebuffer(
        uint32_t width,
        uint32_t height,
        TextureFormat format = TextureFormat::Depth24Plus,
        uint32_t sampleCount = 1,
        TextureUsage usage = TextureUsage::RenderAttachment | TextureUsage::TextureBinding) = 0;
};

} // namespace pers