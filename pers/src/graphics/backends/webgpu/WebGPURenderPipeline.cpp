#include "pers/graphics/backends/webgpu/WebGPURenderPipeline.h"
#include "pers/graphics/backends/webgpu/WebGPUShaderModule.h"
#include "pers/utils/Logger.h"
#include <webgpu/webgpu.h>
#include <vector>

namespace pers {
namespace webgpu {

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
        case TextureFormat::BGRA8Unorm: return WGPUTextureFormat_BGRA8Unorm;
        case TextureFormat::RGBA8Unorm: return WGPUTextureFormat_RGBA8Unorm;
        case TextureFormat::Depth24PlusStencil8: return WGPUTextureFormat_Depth24PlusStencil8;
        case TextureFormat::Depth32Float: return WGPUTextureFormat_Depth32Float;
        default: return WGPUTextureFormat_BGRA8Unorm;
    }
}

WebGPURenderPipeline::WebGPURenderPipeline(const RenderPipelineDesc& desc, WGPUDevice device) 
    : _debugName(desc.debugName.empty() ? "RenderPipeline" : desc.debugName)
    , _pipeline(nullptr) {
    
    if (!device || !desc.vertex || !desc.fragment) {
        Logger::Instance().Log(LogLevel::Error, "WebGPURenderPipeline",
            "Invalid parameters for pipeline creation", PERS_SOURCE_LOC);
        return;
    }
    
    // Get shader modules
    auto vertexModule = static_cast<WebGPUShaderModule*>(desc.vertex.get());
    auto fragmentModule = static_cast<WebGPUShaderModule*>(desc.fragment.get());
    
    WGPUShaderModule vertShader = vertexModule->getNativeHandle();
    WGPUShaderModule fragShader = fragmentModule->getNativeHandle();
    
    if (!vertShader || !fragShader) {
        Logger::Instance().Log(LogLevel::Error, "WebGPURenderPipeline",
            "Shader modules not ready", PERS_SOURCE_LOC);
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
    
    // Default color target if none specified
    if (colorTargets.empty()) {
        WGPUColorTargetState colorTarget = {};
        colorTarget.format = WGPUTextureFormat_BGRA8Unorm;
        colorTarget.writeMask = WGPUColorWriteMask_All;
        colorTargets.push_back(colorTarget);
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
    primitive.stripIndexFormat = desc.primitive.stripIndexFormat ? 
        WGPUIndexFormat_Uint16 : WGPUIndexFormat_Undefined;
    primitive.frontFace = convertFrontFace(desc.primitive.frontFace);
    primitive.cullMode = convertCullMode(desc.primitive.cullMode);
    
    // Depth stencil state
    WGPUDepthStencilState* depthStencilPtr = nullptr;
    WGPUDepthStencilState depthStencil = {};
    if (desc.depthStencil.depthWriteEnabled || 
        desc.depthStencil.depthCompare != CompareFunction::Undefined) {
        depthStencil.format = WGPUTextureFormat_Depth24PlusStencil8;
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

} // namespace webgpu
} // namespace pers