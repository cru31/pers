#pragma once

#include "pers/graphics/ITexture.h"
#include <webgpu/webgpu.h>
#include <memory>

namespace pers {

class WebGPUTexture final : public ITexture {
public:
    WebGPUTexture(WGPUTexture texture,
                  uint32_t width,
                  uint32_t height,
                  uint32_t depth,
                  TextureFormat format,
                  TextureUsage usage,
                  TextureDimension dimension);
    
    ~WebGPUTexture() override;
    
    // ITexture interface
    uint32_t getWidth() const override;
    uint32_t getHeight() const override;
    uint32_t getDepthOrArrayLayers() const override;
    uint32_t getMipLevelCount() const override;
    uint32_t getSampleCount() const override;
    TextureDimension getDimension() const override;
    TextureFormat getFormat() const override;
    TextureUsage getUsage() const override;
    NativeTextureHandle getNativeTextureHandle() const override;
    
    // WebGPU specific
    WGPUTexture getWGPUTexture() const { return _texture; }
    
private:
    WGPUTexture _texture;
    uint32_t _width;
    uint32_t _height;
    uint32_t _depth;
    uint32_t _mipLevelCount;
    uint32_t _arrayLayerCount;
    TextureFormat _format;
    TextureUsage _usage;
    TextureDimension _dimension;
};

} // namespace pers