#include "pers/graphics/backends/webgpu/WebGPURenderPipeline.h"
#include "pers/graphics/backends/webgpu/WebGPUShaderModule.h"
#include "pers/utils/Logger.h"
#include <webgpu/webgpu.h>
#include <vector>

namespace pers {

// Convert our enums to WebGPU enums
static WGPUColorWriteMask convertColorWriteMask(ColorWriteMask mask) {
    uint32_t result = WGPUColorWriteMask_None;
    uint32_t maskValue = static_cast<uint32_t>(mask);
    
    if (mask == ColorWriteMask::None) {
        return WGPUColorWriteMask_None;
    }
    if (mask == ColorWriteMask::All) {
        return WGPUColorWriteMask_All;
    }
    
    // Convert individual flags - our enum values may not match WebGPU's
    if (maskValue & static_cast<uint32_t>(ColorWriteMask::Red)) {
        result |= WGPUColorWriteMask_Red;
    }
    if (maskValue & static_cast<uint32_t>(ColorWriteMask::Green)) {
        result |= WGPUColorWriteMask_Green;
    }
    if (maskValue & static_cast<uint32_t>(ColorWriteMask::Blue)) {
        result |= WGPUColorWriteMask_Blue;
    }
    if (maskValue & static_cast<uint32_t>(ColorWriteMask::Alpha)) {
        result |= WGPUColorWriteMask_Alpha;
    }
    
    return static_cast<WGPUColorWriteMask>(result);
}

static WGPUPrimitiveTopology convertTopology(PrimitiveTopology topology) {
    switch (topology) {
        case PrimitiveTopology::PointList: return WGPUPrimitiveTopology_PointList;
        case PrimitiveTopology::LineList: return WGPUPrimitiveTopology_LineList;
        case PrimitiveTopology::LineStrip: return WGPUPrimitiveTopology_LineStrip;
        case PrimitiveTopology::TriangleList: return WGPUPrimitiveTopology_TriangleList;
        case PrimitiveTopology::TriangleStrip: return WGPUPrimitiveTopology_TriangleStrip;
    }
    return WGPUPrimitiveTopology_TriangleList;
}

static WGPUCullMode convertCullMode(CullMode mode) {
    switch (mode) {
        case CullMode::None: return WGPUCullMode_None;
        case CullMode::Front: return WGPUCullMode_Front;
        case CullMode::Back: return WGPUCullMode_Back;
    }
    return WGPUCullMode_None;
}

static WGPUFrontFace convertFrontFace(FrontFace face) {
    switch (face) {
        case FrontFace::CCW: return WGPUFrontFace_CCW;
        case FrontFace::CW: return WGPUFrontFace_CW;
    }
    return WGPUFrontFace_CCW;
}

static WGPUVertexFormat convertVertexFormat(VertexFormat format) {
    switch (format) {
        case VertexFormat::Float32: return WGPUVertexFormat_Float32;
        case VertexFormat::Float32x2: return WGPUVertexFormat_Float32x2;
        case VertexFormat::Float32x3: return WGPUVertexFormat_Float32x3;
        case VertexFormat::Float32x4: return WGPUVertexFormat_Float32x4;
        case VertexFormat::Uint32: return WGPUVertexFormat_Uint32;
        case VertexFormat::Uint32x2: return WGPUVertexFormat_Uint32x2;
        case VertexFormat::Uint32x3: return WGPUVertexFormat_Uint32x3;
        case VertexFormat::Uint32x4: return WGPUVertexFormat_Uint32x4;
        case VertexFormat::Sint32: return WGPUVertexFormat_Sint32;
        case VertexFormat::Sint32x2: return WGPUVertexFormat_Sint32x2;
        case VertexFormat::Sint32x3: return WGPUVertexFormat_Sint32x3;
        case VertexFormat::Sint32x4: return WGPUVertexFormat_Sint32x4;
    }
    return WGPUVertexFormat_Float32x3;
}

static WGPUVertexStepMode convertStepMode(VertexStepMode mode) {
    switch (mode) {
        case VertexStepMode::Vertex: return WGPUVertexStepMode_Vertex;
        case VertexStepMode::Instance: return WGPUVertexStepMode_Instance;
    }
    return WGPUVertexStepMode_Vertex;
}

static WGPUIndexFormat convertIndexFormat(IndexFormat format) {
    switch (format) {
        case IndexFormat::Undefined: return WGPUIndexFormat_Undefined;
        case IndexFormat::Uint16: return WGPUIndexFormat_Uint16;
        case IndexFormat::Uint32: return WGPUIndexFormat_Uint32;
    }
    return WGPUIndexFormat_Undefined;
}

static WGPUCompareFunction convertCompareFunction(CompareFunction func) {
    switch (func) {
        case CompareFunction::Never: return WGPUCompareFunction_Never;
        case CompareFunction::Less: return WGPUCompareFunction_Less;
        case CompareFunction::Equal: return WGPUCompareFunction_Equal;
        case CompareFunction::LessEqual: return WGPUCompareFunction_LessEqual;
        case CompareFunction::Greater: return WGPUCompareFunction_Greater;
        case CompareFunction::NotEqual: return WGPUCompareFunction_NotEqual;
        case CompareFunction::GreaterEqual: return WGPUCompareFunction_GreaterEqual;
        case CompareFunction::Always: return WGPUCompareFunction_Always;
        case CompareFunction::Undefined: return WGPUCompareFunction_Undefined;
        default: 
            Logger::Instance().LogFormat(LogLevel::Warning, "WebGPURenderPipeline",
                PERS_SOURCE_LOC, "Unknown CompareFunction value: %d, using Always", 
                static_cast<int>(func));
            return WGPUCompareFunction_Always;  // Safe default
    }
}

static WGPUTextureFormat convertTextureFormat(TextureFormat format) {
    switch (format) {
        // Color formats
        case TextureFormat::R8Unorm: return WGPUTextureFormat_R8Unorm;
        case TextureFormat::R8Snorm: return WGPUTextureFormat_R8Snorm;
        case TextureFormat::R8Uint: return WGPUTextureFormat_R8Uint;
        case TextureFormat::R8Sint: return WGPUTextureFormat_R8Sint;
        case TextureFormat::R16Uint: return WGPUTextureFormat_R16Uint;
        case TextureFormat::R16Sint: return WGPUTextureFormat_R16Sint;
        case TextureFormat::R16Float: return WGPUTextureFormat_R16Float;
        case TextureFormat::RG8Unorm: return WGPUTextureFormat_RG8Unorm;
        case TextureFormat::RG8Snorm: return WGPUTextureFormat_RG8Snorm;
        case TextureFormat::RG8Uint: return WGPUTextureFormat_RG8Uint;
        case TextureFormat::RG8Sint: return WGPUTextureFormat_RG8Sint;
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
        case TextureFormat::RGB10A2Unorm: return WGPUTextureFormat_RGB10A2Unorm;
        case TextureFormat::RG32Float: return WGPUTextureFormat_RG32Float;
        case TextureFormat::RG32Uint: return WGPUTextureFormat_RG32Uint;
        case TextureFormat::RG32Sint: return WGPUTextureFormat_RG32Sint;
        case TextureFormat::RGBA16Uint: return WGPUTextureFormat_RGBA16Uint;
        case TextureFormat::RGBA16Sint: return WGPUTextureFormat_RGBA16Sint;
        case TextureFormat::RGBA16Float: return WGPUTextureFormat_RGBA16Float;
        case TextureFormat::RGBA32Float: return WGPUTextureFormat_RGBA32Float;
        case TextureFormat::RGBA32Uint: return WGPUTextureFormat_RGBA32Uint;
        case TextureFormat::RGBA32Sint: return WGPUTextureFormat_RGBA32Sint;
        
        // Depth/stencil formats
        case TextureFormat::Stencil8: return WGPUTextureFormat_Stencil8;
        case TextureFormat::Depth16Unorm: return WGPUTextureFormat_Depth16Unorm;
        case TextureFormat::Depth24Plus: return WGPUTextureFormat_Depth24Plus;
        case TextureFormat::Depth24PlusStencil8: return WGPUTextureFormat_Depth24PlusStencil8;
        case TextureFormat::Depth32Float: return WGPUTextureFormat_Depth32Float;
        case TextureFormat::Depth32FloatStencil8: return WGPUTextureFormat_Depth32FloatStencil8;
        
        // Compressed formats
        case TextureFormat::BC1RGBAUnorm: return WGPUTextureFormat_BC1RGBAUnorm;
        case TextureFormat::BC1RGBAUnormSrgb: return WGPUTextureFormat_BC1RGBAUnormSrgb;
        case TextureFormat::BC2RGBAUnorm: return WGPUTextureFormat_BC2RGBAUnorm;
        case TextureFormat::BC2RGBAUnormSrgb: return WGPUTextureFormat_BC2RGBAUnormSrgb;
        case TextureFormat::BC3RGBAUnorm: return WGPUTextureFormat_BC3RGBAUnorm;
        case TextureFormat::BC3RGBAUnormSrgb: return WGPUTextureFormat_BC3RGBAUnormSrgb;
        case TextureFormat::BC4RUnorm: return WGPUTextureFormat_BC4RUnorm;
        case TextureFormat::BC4RSnorm: return WGPUTextureFormat_BC4RSnorm;
        case TextureFormat::BC5RGUnorm: return WGPUTextureFormat_BC5RGUnorm;
        case TextureFormat::BC5RGSnorm: return WGPUTextureFormat_BC5RGSnorm;
        case TextureFormat::BC6HRGBUfloat: return WGPUTextureFormat_BC6HRGBUfloat;
        case TextureFormat::BC6HRGBFloat: return WGPUTextureFormat_BC6HRGBFloat;
        case TextureFormat::BC7RGBAUnorm: return WGPUTextureFormat_BC7RGBAUnorm;
        case TextureFormat::BC7RGBAUnormSrgb: return WGPUTextureFormat_BC7RGBAUnormSrgb;
        
        case TextureFormat::Undefined: return WGPUTextureFormat_Undefined;
        default: 
            Logger::Instance().LogFormat(LogLevel::Error, "WebGPURenderPipeline",
                PERS_SOURCE_LOC, "Unknown TextureFormat value: %d", 
                static_cast<int>(format));
            return WGPUTextureFormat_Undefined;
    }
}

WebGPURenderPipeline::WebGPURenderPipeline(const RenderPipelineDesc& desc, WGPUDevice device) 
    : _debugName(desc.debugName.empty() ? "RenderPipeline" : desc.debugName)
    , _pipeline(nullptr) {
    
    if (!device || !desc.vertex || !desc.fragment) {
        LOG_ERROR("WebGPURenderPipeline",
            "Invalid parameters for pipeline creation");
        return;
    }
    
    // Get shader modules
    auto vertexModule = static_cast<WebGPUShaderModule*>(desc.vertex.get());
    auto fragmentModule = static_cast<WebGPUShaderModule*>(desc.fragment.get());
    
    WGPUShaderModule vertShader = vertexModule->getNativeHandle();
    WGPUShaderModule fragShader = fragmentModule->getNativeHandle();
    
    if (!vertShader || !fragShader) {
        LOG_ERROR("WebGPURenderPipeline",
            "Shader modules not ready");
        return;
    }
    
    // Setup vertex state
    std::vector<WGPUVertexBufferLayout> vertexBuffers;
    std::vector<std::vector<WGPUVertexAttribute>> attributeArrays;
    
    for (const auto& layout : desc.vertexLayouts) {
        attributeArrays.emplace_back();
        auto& attrs = attributeArrays.back();
        
        for (const auto& attr : layout.attributes) {
            WGPUVertexAttribute wgpuAttr = {};
            wgpuAttr.format = convertVertexFormat(attr.format);
            wgpuAttr.offset = attr.offset;
            wgpuAttr.shaderLocation = attr.shaderLocation;
            attrs.push_back(wgpuAttr);
        }
        
        WGPUVertexBufferLayout buffer = {};
        buffer.arrayStride = layout.arrayStride;
        buffer.stepMode = convertStepMode(layout.stepMode);
        buffer.attributeCount = attrs.size();
        buffer.attributes = attrs.data();
        vertexBuffers.push_back(buffer);
    }
    
    // Vertex stage
    WGPUVertexState vertex = {};
    vertex.module = vertShader;
    std::string vertexEntryPoint = desc.vertex->getEntryPoint();
    vertex.entryPoint = WGPUStringView{vertexEntryPoint.data(), vertexEntryPoint.length()};
    vertex.bufferCount = vertexBuffers.size();
    vertex.buffers = vertexBuffers.empty() ? nullptr : vertexBuffers.data();
    
    // Fragment stage
    std::vector<WGPUColorTargetState> colorTargets;
    for (const auto& target : desc.colorTargets) {
        WGPUColorTargetState colorTarget = {};
        colorTarget.format = convertTextureFormat(target.format);
        colorTarget.writeMask = convertColorWriteMask(target.writeMask);
        colorTargets.push_back(colorTarget);
    }
    
    // No default color target - user must specify what they want
    if (colorTargets.empty()) {
        LOG_ERROR("WebGPURenderPipeline",
            "No color targets specified in RenderPipelineDesc");
        return;
    }
    
    WGPUFragmentState fragment = {};
    fragment.module = fragShader;
    std::string fragmentEntryPoint = desc.fragment->getEntryPoint();
    fragment.entryPoint = WGPUStringView{fragmentEntryPoint.data(), fragmentEntryPoint.length()};
    fragment.targetCount = colorTargets.size();
    fragment.targets = colorTargets.data();
    
    // Primitive state
    WGPUPrimitiveState primitive = {};
    primitive.topology = convertTopology(desc.primitive.topology);
    primitive.stripIndexFormat = convertIndexFormat(desc.primitive.stripIndexFormat);
    primitive.frontFace = convertFrontFace(desc.primitive.frontFace);
    primitive.cullMode = convertCullMode(desc.primitive.cullMode);
    
    // Depth stencil state
    WGPUDepthStencilState* depthStencilPtr = nullptr;
    WGPUDepthStencilState depthStencil = {};
    if (desc.depthStencil.format != TextureFormat::Undefined) {
        // Use the format specified by the user, not hardcoded!
        depthStencil.format = convertTextureFormat(desc.depthStencil.format);
        depthStencil.depthWriteEnabled = desc.depthStencil.depthWriteEnabled ? WGPUOptionalBool_True : WGPUOptionalBool_False;
        depthStencil.depthCompare = convertCompareFunction(desc.depthStencil.depthCompare);
        depthStencil.stencilReadMask = desc.depthStencil.stencilReadMask;
        depthStencil.stencilWriteMask = desc.depthStencil.stencilWriteMask;
        depthStencilPtr = &depthStencil;
    }
    
    // Multisample state
    WGPUMultisampleState multisample = {};
    multisample.count = desc.multisample.count;
    multisample.mask = desc.multisample.mask;
    multisample.alphaToCoverageEnabled = desc.multisample.alphaToCoverageEnabled;
    
    // Create pipeline
    WGPURenderPipelineDescriptor pipelineDesc = {};
    pipelineDesc.label = WGPUStringView{_debugName.data(), _debugName.length()};
    pipelineDesc.vertex = vertex;
    pipelineDesc.fragment = &fragment;
    pipelineDesc.primitive = primitive;
    pipelineDesc.depthStencil = depthStencilPtr;
    pipelineDesc.multisample = multisample;
    
    _pipeline = wgpuDeviceCreateRenderPipeline(device, &pipelineDesc);
    
    if (!_pipeline) {
        Logger::Instance().LogFormat(LogLevel::Error, "WebGPURenderPipeline",
            PERS_SOURCE_LOC, "Failed to create render pipeline: %s", _debugName.c_str());
    } else {
        Logger::Instance().LogFormat(LogLevel::Info, "WebGPURenderPipeline",
            PERS_SOURCE_LOC, "Created render pipeline: %s", _debugName.c_str());
    }
}

WebGPURenderPipeline::~WebGPURenderPipeline() {
    if (_pipeline) {
        wgpuRenderPipelineRelease(_pipeline);
    }
}

const std::string& WebGPURenderPipeline::getDebugName() const {
    return _debugName;
}

bool WebGPURenderPipeline::isValid() const {
    return _pipeline != nullptr;
}

WGPURenderPipeline WebGPURenderPipeline::getNativeHandle() const {
    return _pipeline;
}

} // namespace pers