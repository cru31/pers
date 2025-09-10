#include "pers/graphics/backends/webgpu/WebGPUTexture.h"
#include "pers/utils/Logger.h"

namespace pers {

WebGPUTexture::WebGPUTexture(WGPUTexture texture,
                             uint32_t width,
                             uint32_t height,
                             uint32_t depth,
                             TextureFormat format,
                             TextureUsage usage,
                             TextureDimension dimension)
    : _texture(texture)
    , _width(width)
    , _height(height)
    , _depth(depth)
    , _mipLevelCount(1)
    , _arrayLayerCount(1)
    , _format(format)
    , _usage(usage)
    , _dimension(dimension) {
    
    if (!_texture) {
        LOG_ERROR("WebGPUTexture", "Invalid texture handle");
    }
}

WebGPUTexture::~WebGPUTexture() {
    if (_texture) {
        wgpuTextureRelease(_texture);
        _texture = nullptr;
    }
}

uint32_t WebGPUTexture::getWidth() const {
    return _width;
}

uint32_t WebGPUTexture::getHeight() const {
    return _height;
}

uint32_t WebGPUTexture::getDepthOrArrayLayers() const {
    return _depth;
}

uint32_t WebGPUTexture::getMipLevelCount() const {
    return _mipLevelCount;
}

uint32_t WebGPUTexture::getSampleCount() const {
    return 1; // TODO: Add sample count support
}

TextureFormat WebGPUTexture::getFormat() const {
    return _format;
}

TextureUsage WebGPUTexture::getUsage() const {
    return _usage;
}

TextureDimension WebGPUTexture::getDimension() const {
    return _dimension;
}

NativeTextureHandle WebGPUTexture::getNativeTextureHandle() const {
    return NativeTextureHandle(_texture);
}

} // namespace pers