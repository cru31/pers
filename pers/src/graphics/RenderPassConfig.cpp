#include "pers/graphics/RenderPassConfig.h"
#include "pers/graphics/IFramebuffer.h"
#include "pers/utils/Logger.h"

namespace pers {

void RenderPassConfig::addColorAttachment(const ColorConfig& config) {
    _colorConfigs.push_back(config);
}

void RenderPassConfig::setDepthStencilConfig(const DepthStencilConfig& config) {
    _depthStencilConfig = std::make_shared<DepthStencilConfig>(config);
}

void RenderPassConfig::setLabel(const std::string& label) {
    _label = label;
}

bool RenderPassConfig::hasDepthStencil() const {
    return _depthStencilConfig != nullptr;
}

uint32_t RenderPassConfig::getColorAttachmentCount() const {
    return static_cast<uint32_t>(_colorConfigs.size());
}

RenderPassDesc RenderPassConfig::makeDescriptor(const std::shared_ptr<IFramebuffer>& framebuffer) const {
    RenderPassDesc desc;
    desc.label = _label;
    
    if (!framebuffer) {
        LOG_ERROR("RenderPassConfig", "Cannot create descriptor with null framebuffer");
        return desc;
    }
    
    // Apply color attachment configurations to framebuffer's color attachments
    for (uint32_t i = 0; i < _colorConfigs.size(); ++i) {
        auto view = framebuffer->getColorAttachment(i);
        if (!view) {
            LOG_WARNING("RenderPassConfig", 
                "Framebuffer missing color attachment at index " + std::to_string(i));
            continue;
        }
        
        RenderPassColorAttachment colorAttachment;
        colorAttachment.view = view;
        colorAttachment.loadOp = _colorConfigs[i].loadOp;
        colorAttachment.storeOp = _colorConfigs[i].storeOp;
        colorAttachment.clearColor = _colorConfigs[i].clearColor;
        desc.colorAttachments.push_back(colorAttachment);
    }
    
    // Apply depth/stencil configuration if present
    if (_depthStencilConfig && framebuffer->hasDepthStencilAttachment()) {
        auto depthView = framebuffer->getDepthStencilAttachment();
        if (depthView) {
            auto depthAttachment = std::make_shared<RenderPassDepthStencilAttachment>();
            depthAttachment->view = depthView;
            depthAttachment->depthLoadOp = _depthStencilConfig->depthLoadOp;
            depthAttachment->depthStoreOp = _depthStencilConfig->depthStoreOp;
            depthAttachment->depthClearValue = _depthStencilConfig->depthClearValue;
            depthAttachment->depthReadOnly = _depthStencilConfig->depthReadOnly;
            depthAttachment->stencilLoadOp = _depthStencilConfig->stencilLoadOp;
            depthAttachment->stencilStoreOp = _depthStencilConfig->stencilStoreOp;
            depthAttachment->stencilClearValue = _depthStencilConfig->stencilClearValue;
            depthAttachment->stencilReadOnly = _depthStencilConfig->stencilReadOnly;
            desc.depthStencilAttachment = depthAttachment;
        }
    }
    
    return desc;
}

} // namespace pers