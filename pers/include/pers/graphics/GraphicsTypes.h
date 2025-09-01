#pragma once

#include <cstddef>
#include <cstdint>

namespace pers {

/**
 * @brief Texture dimension enumeration
 */
enum class TextureDimension {
    D1,
    D2,
    D3
};

/**
 * @brief Texture view dimension enumeration
 */
enum class TextureViewDimension {
    D1,
    D2,
    D2Array,
    Cube,
    CubeArray,
    D3
};

/**
 * @brief Texture usage flags
 */
enum class TextureUsage : uint32_t {
    None = 0,
    CopySrc = 1 << 0,
    CopyDst = 1 << 1,
    TextureBinding = 1 << 2,
    StorageBinding = 1 << 3,
    RenderAttachment = 1 << 4
};

/**
 * @brief Filter mode for texture sampling
 */
enum class FilterMode {
    Nearest,
    Linear
};

/**
 * @brief Address mode for texture sampling
 */
enum class AddressMode {
    Repeat,
    MirrorRepeat,
    ClampToEdge,
    ClampToBorder
};

/**
 * @brief Load operation for render pass attachments
 */
enum class LoadOp {
    Clear,
    Load,
    Undefined
};

/**
 * @brief Store operation for render pass attachments  
 */
enum class StoreOp {
    Store,
    Discard
};

/**
 * @brief Color write mask flags
 */
enum class ColorWriteMask : uint32_t {
    None = 0,
    Red = 1 << 0,
    Green = 1 << 1,
    Blue = 1 << 2,
    Alpha = 1 << 3,
    All = Red | Green | Blue | Alpha
};

/**
 * @brief Handle type enumeration for type safety
 */
enum class HandleType {
    Adapter,
    Device,
    Queue,
    Surface,
    SwapChain,
    CommandBuffer,
    CommandEncoder,
    RenderPass,
    TextureView,
    Buffer,
    Texture,
    Sampler,
    Pipeline,
    Shader,
    BindGroup,
    BindGroupLayout,
    PipelineLayout
};

/**
 * @brief Type-safe handle wrapper
 * 
 * Provides compile-time type safety for native handles while maintaining
 * zero runtime overhead. Each handle type is distinct at compile time,
 * preventing accidental mixing of different handle types.
 */
template<HandleType Type>
struct TypedHandle {
private:
    void* _ptr = nullptr;
    
public:
    // Default constructor - creates null handle
    TypedHandle() = default;
    
    // Explicit constructor from void pointer
    explicit TypedHandle(void* p) : _ptr(p) {}
    
    // Nullptr constructor
    TypedHandle(std::nullptr_t) : _ptr(nullptr) {}
    
    // Copy constructor and assignment
    TypedHandle(const TypedHandle&) = default;
    TypedHandle& operator=(const TypedHandle&) = default;
    
    // Move constructor and assignment
    TypedHandle(TypedHandle&&) = default;
    TypedHandle& operator=(TypedHandle&&) = default;
    
    // Comparison operators
    bool operator==(const TypedHandle& other) const { return _ptr == other._ptr; }
    bool operator!=(const TypedHandle& other) const { return _ptr != other._ptr; }
    bool operator==(std::nullptr_t) const { return _ptr == nullptr; }
    bool operator!=(std::nullptr_t) const { return _ptr != nullptr; }
    
    // Check if handle is valid
    bool isValid() const { return _ptr != nullptr; }
    explicit operator bool() const { return isValid(); }
    
    // Get raw pointer (only for backend implementation use)
    void* getRaw() const { return _ptr; }
    
    // Cast to backend-specific type (only for backend implementation use)
    template<typename T>
    T as() const { return static_cast<T>(_ptr); }
    
    // Create from backend-specific type
    template<typename T>
    static TypedHandle fromBackend(T backend) {
        return TypedHandle(static_cast<void*>(backend));
    }
};

/**
 * @brief Native graphics API handle types
 * 
 * These are strongly-typed handles that wrap backend-specific objects.
 * Each handle type is distinct at compile time, preventing accidental
 * type confusion.
 */

// Device and adapter handles
using NativeAdapterHandle = TypedHandle<HandleType::Adapter>;           // WGPUAdapter for WebGPU
using NativeDeviceHandle = TypedHandle<HandleType::Device>;             // WGPUDevice for WebGPU

// Command and queue handles  
using NativeQueueHandle = TypedHandle<HandleType::Queue>;               // WGPUQueue for WebGPU
using NativeCommandBufferHandle = TypedHandle<HandleType::CommandBuffer>; // WGPUCommandBuffer for WebGPU
using NativeEncoderHandle = TypedHandle<HandleType::CommandEncoder>;    // WGPUCommandEncoder for WebGPU
using NativeRenderPassHandle = TypedHandle<HandleType::RenderPass>;     // WGPURenderPassEncoder for WebGPU

// Resource handles
using NativeSwapChainHandle = TypedHandle<HandleType::SwapChain>;       // Implementation-specific
using NativeTextureViewHandle = TypedHandle<HandleType::TextureView>;   // WGPUTextureView for WebGPU
using NativeBufferHandle = TypedHandle<HandleType::Buffer>;             // WGPUBuffer for WebGPU
using NativeTextureHandle = TypedHandle<HandleType::Texture>;           // WGPUTexture for WebGPU
using NativeSamplerHandle = TypedHandle<HandleType::Sampler>;           // WGPUSampler for WebGPU

// Pipeline and shader handles
using NativePipelineHandle = TypedHandle<HandleType::Pipeline>;         // WGPURenderPipeline for WebGPU
using NativeShaderHandle = TypedHandle<HandleType::Shader>;             // WGPUShaderModule for WebGPU
using NativeBindGroupHandle = TypedHandle<HandleType::BindGroup>;       // WGPUBindGroup for WebGPU
using NativeBindGroupLayoutHandle = TypedHandle<HandleType::BindGroupLayout>; // WGPUBindGroupLayout for WebGPU
using NativePipelineLayoutHandle = TypedHandle<HandleType::PipelineLayout>;   // WGPUPipelineLayout for WebGPU

// Surface handle
using NativeSurfaceHandle = TypedHandle<HandleType::Surface>;           // WGPUSurface for WebGPU

// Render pass encoder handle (alias for consistency)
using NativeRenderPassEncoderHandle = NativeRenderPassHandle;           // WGPURenderPassEncoder for WebGPU

} // namespace pers