#pragma once

#include <memory>
#include <string>
#include <vector>
#include "pers/graphics/GraphicsTypes.h"
#include "pers/graphics/GraphicsFormats.h"

namespace pers {

// Forward declarations
class IShaderModule;

enum class PrimitiveTopology {
    PointList,
    LineList,
    LineStrip,
    TriangleList,
    TriangleStrip
};

enum class CullMode {
    None,
    Front,
    Back
};

enum class FrontFace {
    CCW,  // Counter-clockwise
    CW    // Clockwise
};

// VertexFormat is defined in GraphicsFormats.h

enum class VertexStepMode {
    Vertex,
    Instance
};

struct VertexAttribute {
    VertexFormat format = VertexFormat::Float32x3;
    uint64_t offset = 0;
    uint32_t shaderLocation = 0;
};

struct VertexBufferLayout {
    uint64_t arrayStride = 0;
    VertexStepMode stepMode = VertexStepMode::Vertex;
    std::vector<VertexAttribute> attributes;
};

struct PrimitiveState {
    PrimitiveTopology topology = PrimitiveTopology::TriangleList;
    IndexFormat stripIndexFormat = IndexFormat::Undefined;  // Index format for strip primitives
    FrontFace frontFace = FrontFace::CCW;
    CullMode cullMode = CullMode::None;
};

struct DepthStencilState {
    TextureFormat format = TextureFormat::Undefined;  // Depth format to use
    bool depthWriteEnabled = false;
    CompareFunction depthCompare = CompareFunction::Less;
    uint32_t stencilReadMask = 0xFFFFFFFF;
    uint32_t stencilWriteMask = 0xFFFFFFFF;
};

struct ColorTargetState {
    TextureFormat format = TextureFormat::BGRA8Unorm;
    ColorWriteMask writeMask = ColorWriteMask::All;
};

struct MultisampleState {
    uint32_t count = 1;
    uint32_t mask = 0xFFFFFFFF;
    bool alphaToCoverageEnabled = false;
};

struct RenderPipelineDesc {
    // Shaders (required)
    std::shared_ptr<IShaderModule> vertex;
    std::shared_ptr<IShaderModule> fragment;
    
    // Vertex state (optional - can be empty for vertex-less rendering)
    std::vector<VertexBufferLayout> vertexLayouts;
    
    // Pipeline states with smart defaults
    PrimitiveState primitive;
    DepthStencilState depthStencil;
    MultisampleState multisample;
    std::vector<ColorTargetState> colorTargets;
    
    // Optional debug name
    std::string debugName;
};

class IRenderPipeline {
public:
    virtual ~IRenderPipeline() = default;
    
    virtual const std::string& getDebugName() const = 0;
    virtual bool isValid() const = 0;
};

} // namespace pers