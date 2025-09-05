#include "pers/graphics/GraphicsEnumStrings.h"
#include <sstream>

namespace pers {

std::string GraphicsEnumStrings::toString(TextureFormat format) {
    switch (format) {
        case TextureFormat::Undefined: return "Undefined";
        
        // 8-bit formats
        case TextureFormat::R8Unorm: return "R8Unorm";
        case TextureFormat::R8Snorm: return "R8Snorm";
        case TextureFormat::R8Uint: return "R8Uint";
        case TextureFormat::R8Sint: return "R8Sint";
        
        // 16-bit formats
        case TextureFormat::R16Uint: return "R16Uint";
        case TextureFormat::R16Sint: return "R16Sint";
        case TextureFormat::R16Float: return "R16Float";
        case TextureFormat::RG8Unorm: return "RG8Unorm";
        case TextureFormat::RG8Snorm: return "RG8Snorm";
        case TextureFormat::RG8Uint: return "RG8Uint";
        case TextureFormat::RG8Sint: return "RG8Sint";
        
        // 32-bit formats
        case TextureFormat::R32Float: return "R32Float";
        case TextureFormat::R32Uint: return "R32Uint";
        case TextureFormat::R32Sint: return "R32Sint";
        case TextureFormat::RG16Uint: return "RG16Uint";
        case TextureFormat::RG16Sint: return "RG16Sint";
        case TextureFormat::RG16Float: return "RG16Float";
        case TextureFormat::RGBA8Unorm: return "RGBA8Unorm";
        case TextureFormat::RGBA8UnormSrgb: return "RGBA8UnormSrgb";
        case TextureFormat::RGBA8Snorm: return "RGBA8Snorm";
        case TextureFormat::RGBA8Uint: return "RGBA8Uint";
        case TextureFormat::RGBA8Sint: return "RGBA8Sint";
        case TextureFormat::BGRA8Unorm: return "BGRA8Unorm";
        case TextureFormat::BGRA8UnormSrgb: return "BGRA8UnormSrgb";
        
        // 64-bit formats
        case TextureFormat::RG32Float: return "RG32Float";
        case TextureFormat::RG32Uint: return "RG32Uint";
        case TextureFormat::RG32Sint: return "RG32Sint";
        case TextureFormat::RGBA16Uint: return "RGBA16Uint";
        case TextureFormat::RGBA16Sint: return "RGBA16Sint";
        case TextureFormat::RGBA16Float: return "RGBA16Float";
        
        // 128-bit formats
        case TextureFormat::RGBA32Float: return "RGBA32Float";
        case TextureFormat::RGBA32Uint: return "RGBA32Uint";
        case TextureFormat::RGBA32Sint: return "RGBA32Sint";
        
        // Depth/stencil formats
        case TextureFormat::Depth16Unorm: return "Depth16Unorm";
        case TextureFormat::Depth24Plus: return "Depth24Plus";
        case TextureFormat::Depth24PlusStencil8: return "Depth24PlusStencil8";
        case TextureFormat::Depth32Float: return "Depth32Float";
        
        default: return "Unknown(" + std::to_string(static_cast<int>(format)) + ")";
    }
}

std::string GraphicsEnumStrings::toString(PresentMode mode) {
    switch (mode) {
        case PresentMode::Immediate: return "Immediate";
        case PresentMode::Mailbox: return "Mailbox";
        case PresentMode::Fifo: return "Fifo";
        case PresentMode::FifoRelaxed: return "FifoRelaxed";
        default: return "Unknown(" + std::to_string(static_cast<int>(mode)) + ")";
    }
}

std::string GraphicsEnumStrings::toString(CompositeAlphaMode mode) {
    switch (mode) {
        case CompositeAlphaMode::Opaque: return "Opaque";
        case CompositeAlphaMode::PreMultiplied: return "PreMultiplied";
        case CompositeAlphaMode::PostMultiplied: return "PostMultiplied";
        case CompositeAlphaMode::Inherit: return "Inherit";
        default: return "Unknown(" + std::to_string(static_cast<int>(mode)) + ")";
    }
}

std::string GraphicsEnumStrings::toString(DeviceFeature feature) {
    switch (feature) {
        case DeviceFeature::DepthClipControl: return "DepthClipControl";
        case DeviceFeature::Depth32FloatStencil8: return "Depth32FloatStencil8";
        case DeviceFeature::TimestampQuery: return "TimestampQuery";
        case DeviceFeature::PipelineStatisticsQuery: return "PipelineStatisticsQuery";
        case DeviceFeature::TextureCompressionBC: return "TextureCompressionBC";
        case DeviceFeature::TextureCompressionETC2: return "TextureCompressionETC2";
        case DeviceFeature::TextureCompressionASTC: return "TextureCompressionASTC";
        case DeviceFeature::IndirectFirstInstance: return "IndirectFirstInstance";
        case DeviceFeature::ShaderF16: return "ShaderF16";
        case DeviceFeature::RG11B10UfloatRenderable: return "RG11B10UfloatRenderable";
        case DeviceFeature::BGRA8UnormStorage: return "BGRA8UnormStorage";
        case DeviceFeature::Float32Filterable: return "Float32Filterable";
        default: return "Unknown(" + std::to_string(static_cast<int>(feature)) + ")";
    }
}

std::string GraphicsEnumStrings::toString(TextureUsage usage) {
    if (usage == TextureUsage::None) {
        return "None";
    }
    
    std::stringstream ss;
    bool first = true;
    
    auto addFlag = [&](TextureUsage flag, const char* name) {
        if ((usage & flag) == flag) {
            if (!first) ss << " | ";
            ss << name;
            first = false;
        }
    };
    
    addFlag(TextureUsage::CopySrc, "CopySrc");
    addFlag(TextureUsage::CopyDst, "CopyDst");
    addFlag(TextureUsage::TextureBinding, "TextureBinding");
    addFlag(TextureUsage::StorageBinding, "StorageBinding");
    addFlag(TextureUsage::RenderAttachment, "RenderAttachment");
    
    return ss.str();
}

} // namespace pers