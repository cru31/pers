#include "pers/graphics/backends/webgpu/WebGPUConverters.h"
#include "pers/utils/Logger.h"

namespace pers {

WGPUTextureFormat WebGPUConverters::convertTextureFormat(TextureFormat format) {
    switch (format) {
        // 8-bit formats
        case TextureFormat::R8Unorm: return WGPUTextureFormat_R8Unorm;
        case TextureFormat::R8Snorm: return WGPUTextureFormat_R8Snorm;
        case TextureFormat::R8Uint: return WGPUTextureFormat_R8Uint;
        case TextureFormat::R8Sint: return WGPUTextureFormat_R8Sint;
        
        // 16-bit formats
        case TextureFormat::R16Uint: return WGPUTextureFormat_R16Uint;
        case TextureFormat::R16Sint: return WGPUTextureFormat_R16Sint;
        case TextureFormat::R16Float: return WGPUTextureFormat_R16Float;
        case TextureFormat::RG8Unorm: return WGPUTextureFormat_RG8Unorm;
        case TextureFormat::RG8Snorm: return WGPUTextureFormat_RG8Snorm;
        case TextureFormat::RG8Uint: return WGPUTextureFormat_RG8Uint;
        case TextureFormat::RG8Sint: return WGPUTextureFormat_RG8Sint;
        
        // 32-bit formats
        case TextureFormat::R32Float: return WGPUTextureFormat_R32Float;
        case TextureFormat::R32Uint: return WGPUTextureFormat_R32Uint;
        case TextureFormat::R32Sint: return WGPUTextureFormat_R32Sint;
        case TextureFormat::RG16Uint: return WGPUTextureFormat_RG16Uint;
        case TextureFormat::RG16Sint: return WGPUTextureFormat_RG16Sint;
        case TextureFormat::RG16Float: return WGPUTextureFormat_RG16Float;
        case TextureFormat::RGBA8Unorm: return WGPUTextureFormat_RGBA8Unorm;
        case TextureFormat::RGBA8UnormSrgb: return WGPUTextureFormat_RGBA8UnormSrgb;
        case TextureFormat::RGBA8Snorm: return WGPUTextureFormat_RGBA8Snorm;
        case TextureFormat::RGBA8Uint: return WGPUTextureFormat_RGBA8Uint;
        case TextureFormat::RGBA8Sint: return WGPUTextureFormat_RGBA8Sint;
        case TextureFormat::BGRA8Unorm: return WGPUTextureFormat_BGRA8Unorm;
        case TextureFormat::BGRA8UnormSrgb: return WGPUTextureFormat_BGRA8UnormSrgb;
        
        // 64-bit formats
        case TextureFormat::RG32Float: return WGPUTextureFormat_RG32Float;
        case TextureFormat::RG32Uint: return WGPUTextureFormat_RG32Uint;
        case TextureFormat::RG32Sint: return WGPUTextureFormat_RG32Sint;
        case TextureFormat::RGBA16Uint: return WGPUTextureFormat_RGBA16Uint;
        case TextureFormat::RGBA16Sint: return WGPUTextureFormat_RGBA16Sint;
        case TextureFormat::RGBA16Float: return WGPUTextureFormat_RGBA16Float;
        
        // 128-bit formats
        case TextureFormat::RGBA32Float: return WGPUTextureFormat_RGBA32Float;
        case TextureFormat::RGBA32Uint: return WGPUTextureFormat_RGBA32Uint;
        case TextureFormat::RGBA32Sint: return WGPUTextureFormat_RGBA32Sint;
        
        // Depth/Stencil formats
        case TextureFormat::Depth16Unorm: return WGPUTextureFormat_Depth16Unorm;
        case TextureFormat::Depth24Plus: return WGPUTextureFormat_Depth24Plus;
        case TextureFormat::Depth24PlusStencil8: return WGPUTextureFormat_Depth24PlusStencil8;
        case TextureFormat::Depth32Float: return WGPUTextureFormat_Depth32Float;
        case TextureFormat::Depth32FloatStencil8: return WGPUTextureFormat_Depth32FloatStencil8;
        case TextureFormat::Stencil8: return WGPUTextureFormat_Stencil8;
        
        case TextureFormat::Undefined:
        default:
            LOG_WARNING("WebGPUConverters", 
                "Unknown texture format: " + std::to_string(static_cast<int>(format)));
            return WGPUTextureFormat_Undefined;
    }
}

TextureFormat WebGPUConverters::convertFromWGPUTextureFormat(WGPUTextureFormat format) {
    switch (format) {
        // 8-bit formats
        case WGPUTextureFormat_R8Unorm: return TextureFormat::R8Unorm;
        case WGPUTextureFormat_R8Snorm: return TextureFormat::R8Snorm;
        case WGPUTextureFormat_R8Uint: return TextureFormat::R8Uint;
        case WGPUTextureFormat_R8Sint: return TextureFormat::R8Sint;
        
        // 16-bit formats
        case WGPUTextureFormat_R16Uint: return TextureFormat::R16Uint;
        case WGPUTextureFormat_R16Sint: return TextureFormat::R16Sint;
        case WGPUTextureFormat_R16Float: return TextureFormat::R16Float;
        case WGPUTextureFormat_RG8Unorm: return TextureFormat::RG8Unorm;
        case WGPUTextureFormat_RG8Snorm: return TextureFormat::RG8Snorm;
        case WGPUTextureFormat_RG8Uint: return TextureFormat::RG8Uint;
        case WGPUTextureFormat_RG8Sint: return TextureFormat::RG8Sint;
        
        // 32-bit formats
        case WGPUTextureFormat_R32Float: return TextureFormat::R32Float;
        case WGPUTextureFormat_R32Uint: return TextureFormat::R32Uint;
        case WGPUTextureFormat_R32Sint: return TextureFormat::R32Sint;
        case WGPUTextureFormat_RG16Uint: return TextureFormat::RG16Uint;
        case WGPUTextureFormat_RG16Sint: return TextureFormat::RG16Sint;
        case WGPUTextureFormat_RG16Float: return TextureFormat::RG16Float;
        case WGPUTextureFormat_RGBA8Unorm: return TextureFormat::RGBA8Unorm;
        case WGPUTextureFormat_RGBA8UnormSrgb: return TextureFormat::RGBA8UnormSrgb;
        case WGPUTextureFormat_RGBA8Snorm: return TextureFormat::RGBA8Snorm;
        case WGPUTextureFormat_RGBA8Uint: return TextureFormat::RGBA8Uint;
        case WGPUTextureFormat_RGBA8Sint: return TextureFormat::RGBA8Sint;
        case WGPUTextureFormat_BGRA8Unorm: return TextureFormat::BGRA8Unorm;
        case WGPUTextureFormat_BGRA8UnormSrgb: return TextureFormat::BGRA8UnormSrgb;
        
        // 64-bit formats
        case WGPUTextureFormat_RG32Float: return TextureFormat::RG32Float;
        case WGPUTextureFormat_RG32Uint: return TextureFormat::RG32Uint;
        case WGPUTextureFormat_RG32Sint: return TextureFormat::RG32Sint;
        case WGPUTextureFormat_RGBA16Uint: return TextureFormat::RGBA16Uint;
        case WGPUTextureFormat_RGBA16Sint: return TextureFormat::RGBA16Sint;
        case WGPUTextureFormat_RGBA16Float: return TextureFormat::RGBA16Float;
        
        // 128-bit formats
        case WGPUTextureFormat_RGBA32Float: return TextureFormat::RGBA32Float;
        case WGPUTextureFormat_RGBA32Uint: return TextureFormat::RGBA32Uint;
        case WGPUTextureFormat_RGBA32Sint: return TextureFormat::RGBA32Sint;
        
        // Depth/Stencil formats
        case WGPUTextureFormat_Depth16Unorm: return TextureFormat::Depth16Unorm;
        case WGPUTextureFormat_Depth24Plus: return TextureFormat::Depth24Plus;
        case WGPUTextureFormat_Depth24PlusStencil8: return TextureFormat::Depth24PlusStencil8;
        case WGPUTextureFormat_Depth32Float: return TextureFormat::Depth32Float;
        case WGPUTextureFormat_Depth32FloatStencil8: return TextureFormat::Depth32FloatStencil8;
        case WGPUTextureFormat_Stencil8: return TextureFormat::Stencil8;
        
        case WGPUTextureFormat_Undefined:
        default:
            return TextureFormat::Undefined;
    }
}

WGPUPresentMode WebGPUConverters::convertPresentMode(PresentMode mode) {
    switch (mode) {
        case PresentMode::Immediate: return WGPUPresentMode_Immediate;
        case PresentMode::Mailbox: return WGPUPresentMode_Mailbox;
        case PresentMode::Fifo: return WGPUPresentMode_Fifo;
        case PresentMode::FifoRelaxed: return WGPUPresentMode_FifoRelaxed;
        default:
            LOG_WARNING("WebGPUConverters", 
                "Unknown present mode, defaulting to Fifo");
            return WGPUPresentMode_Fifo;
    }
}

PresentMode WebGPUConverters::convertFromWGPUPresentMode(WGPUPresentMode mode) {
    switch (mode) {
        case WGPUPresentMode_Immediate: return PresentMode::Immediate;
        case WGPUPresentMode_Mailbox: return PresentMode::Mailbox;
        case WGPUPresentMode_Fifo: return PresentMode::Fifo;
        case WGPUPresentMode_FifoRelaxed: return PresentMode::FifoRelaxed;
        default:
            return PresentMode::Fifo;
    }
}

WGPUCompositeAlphaMode WebGPUConverters::convertCompositeAlphaMode(CompositeAlphaMode mode) {
    switch (mode) {
        case CompositeAlphaMode::Auto: return WGPUCompositeAlphaMode_Auto;
        case CompositeAlphaMode::Opaque: return WGPUCompositeAlphaMode_Opaque;
        case CompositeAlphaMode::PreMultiplied: return WGPUCompositeAlphaMode_Premultiplied;
        case CompositeAlphaMode::PostMultiplied: return WGPUCompositeAlphaMode_Unpremultiplied;
        case CompositeAlphaMode::Inherit: return WGPUCompositeAlphaMode_Inherit;
        default:
            return WGPUCompositeAlphaMode_Auto;
    }
}

CompositeAlphaMode WebGPUConverters::convertFromWGPUCompositeAlphaMode(WGPUCompositeAlphaMode mode) {
    switch (mode) {
        case WGPUCompositeAlphaMode_Auto: return CompositeAlphaMode::Auto;
        case WGPUCompositeAlphaMode_Opaque: return CompositeAlphaMode::Opaque;
        case WGPUCompositeAlphaMode_Premultiplied: return CompositeAlphaMode::PreMultiplied;
        case WGPUCompositeAlphaMode_Unpremultiplied: return CompositeAlphaMode::PostMultiplied;
        case WGPUCompositeAlphaMode_Inherit: return CompositeAlphaMode::Inherit;
        default:
            return CompositeAlphaMode::Auto;
    }
}

WGPULoadOp WebGPUConverters::convertLoadOp(LoadOp op) {
    switch (op) {
        case LoadOp::Load: return WGPULoadOp_Load;
        case LoadOp::Clear: return WGPULoadOp_Clear;
        default:
            LOG_WARNING("WebGPUConverters", 
                "Unknown load op, defaulting to Clear");
            return WGPULoadOp_Clear;
    }
}

WGPUStoreOp WebGPUConverters::convertStoreOp(StoreOp op) {
    switch (op) {
        case StoreOp::Store: return WGPUStoreOp_Store;
        case StoreOp::Discard: return WGPUStoreOp_Discard;
        default:
            LOG_WARNING("WebGPUConverters", 
                "Unknown store op, defaulting to Store");
            return WGPUStoreOp_Store;
    }
}

WGPUCompareFunction WebGPUConverters::convertCompareFunction(CompareFunction func) {
    switch (func) {
        case CompareFunction::Never: return WGPUCompareFunction_Never;
        case CompareFunction::Less: return WGPUCompareFunction_Less;
        case CompareFunction::Equal: return WGPUCompareFunction_Equal;
        case CompareFunction::LessEqual: return WGPUCompareFunction_LessEqual;
        case CompareFunction::Greater: return WGPUCompareFunction_Greater;
        case CompareFunction::NotEqual: return WGPUCompareFunction_NotEqual;
        case CompareFunction::GreaterEqual: return WGPUCompareFunction_GreaterEqual;
        case CompareFunction::Always: return WGPUCompareFunction_Always;
        default:
            LOG_WARNING("WebGPUConverters", 
                "Unknown compare function, defaulting to Always");
            return WGPUCompareFunction_Always;
    }
}

WGPUTextureUsage WebGPUConverters::convertTextureUsage(TextureUsage usage) {
    WGPUTextureUsage flags = WGPUTextureUsage_None;
    
    if (static_cast<uint32_t>(usage) & static_cast<uint32_t>(TextureUsage::CopySrc)) {
        flags |= WGPUTextureUsage_CopySrc;
    }
    if (static_cast<uint32_t>(usage) & static_cast<uint32_t>(TextureUsage::CopyDst)) {
        flags |= WGPUTextureUsage_CopyDst;
    }
    if (static_cast<uint32_t>(usage) & static_cast<uint32_t>(TextureUsage::TextureBinding)) {
        flags |= WGPUTextureUsage_TextureBinding;
    }
    if (static_cast<uint32_t>(usage) & static_cast<uint32_t>(TextureUsage::StorageBinding)) {
        flags |= WGPUTextureUsage_StorageBinding;
    }
    if (static_cast<uint32_t>(usage) & static_cast<uint32_t>(TextureUsage::RenderAttachment)) {
        flags |= WGPUTextureUsage_RenderAttachment;
    }
    
    return flags;
}

WGPUBufferUsage WebGPUConverters::convertBufferUsage(BufferUsage usage) {
    WGPUBufferUsage flags = WGPUBufferUsage_None;
    
    if (static_cast<uint32_t>(usage) & static_cast<uint32_t>(BufferUsage::MapRead)) {
        flags |= WGPUBufferUsage_MapRead;
    }
    if (static_cast<uint32_t>(usage) & static_cast<uint32_t>(BufferUsage::MapWrite)) {
        flags |= WGPUBufferUsage_MapWrite;
    }
    if (static_cast<uint32_t>(usage) & static_cast<uint32_t>(BufferUsage::CopySrc)) {
        flags |= WGPUBufferUsage_CopySrc;
    }
    if (static_cast<uint32_t>(usage) & static_cast<uint32_t>(BufferUsage::CopyDst)) {
        flags |= WGPUBufferUsage_CopyDst;
    }
    if (static_cast<uint32_t>(usage) & static_cast<uint32_t>(BufferUsage::Index)) {
        flags |= WGPUBufferUsage_Index;
    }
    if (static_cast<uint32_t>(usage) & static_cast<uint32_t>(BufferUsage::Vertex)) {
        flags |= WGPUBufferUsage_Vertex;
    }
    if (static_cast<uint32_t>(usage) & static_cast<uint32_t>(BufferUsage::Uniform)) {
        flags |= WGPUBufferUsage_Uniform;
    }
    if (static_cast<uint32_t>(usage) & static_cast<uint32_t>(BufferUsage::Storage)) {
        flags |= WGPUBufferUsage_Storage;
    }
    if (static_cast<uint32_t>(usage) & static_cast<uint32_t>(BufferUsage::Indirect)) {
        flags |= WGPUBufferUsage_Indirect;
    }
    if (static_cast<uint32_t>(usage) & static_cast<uint32_t>(BufferUsage::QueryResolve)) {
        flags |= WGPUBufferUsage_QueryResolve;
    }
    
    return flags;
}

} // namespace pers