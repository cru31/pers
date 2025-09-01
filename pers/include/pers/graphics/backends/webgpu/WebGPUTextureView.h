#pragma once

#include "pers/graphics/ITextureView.h"
#include <webgpu/webgpu.h>

namespace pers {

/**
 * @brief WebGPU implementation of ITextureView
 */
class WebGPUTextureView : public ITextureView {
public:
    /**
     * @brief Construct texture view wrapper
     * @param textureView Native WebGPU texture view (ownership NOT transferred)
     * @param width Width of the texture
     * @param height Height of the texture
     * @param format Texture format
     */
    WebGPUTextureView(WGPUTextureView textureView,
                      uint32_t width,
                      uint32_t height,
                      TextureFormat format);
    
    ~WebGPUTextureView() override;
    
    // ITextureView interface
    NativeTextureViewHandle getNativeTextureViewHandle() const override;
    void getDimensions(uint32_t& width, uint32_t& height) const override;
    TextureFormat getFormat() const override;
    
private:
    WGPUTextureView _textureView;
    uint32_t _width;
    uint32_t _height;
    TextureFormat _format;
    bool _ownsHandle; // Whether we should release the handle on destruction
};

} // namespace pers