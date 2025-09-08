#include "pers/graphics/backends/webgpu/WebGPUTextureView.h"

namespace pers {

WebGPUTextureView::WebGPUTextureView(WGPUTextureView textureView,
                                     uint32_t width,
                                     uint32_t height,
                                     TextureFormat format,
                                     bool isSwapChainTexture)
    : _textureView(textureView)
    , _width(width)
    , _height(height)
    , _format(format)
    , _ownsHandle(false) // SwapChain manages the lifetime
    , _isSwapChainTexture(isSwapChainTexture) {
}

WebGPUTextureView::~WebGPUTextureView() {
    // We don't release the handle here because SwapChain manages it
    // The texture view from swap chain is released when presenting or getting next frame
}

NativeTextureViewHandle WebGPUTextureView::getNativeTextureViewHandle() const {
    return NativeTextureViewHandle(_textureView);
}

void WebGPUTextureView::getDimensions(uint32_t& width, uint32_t& height) const {
    width = _width;
    height = _height;
}

TextureFormat WebGPUTextureView::getFormat() const {
    return _format;
}

bool WebGPUTextureView::isSwapChainTexture() const {
    return _isSwapChainTexture;
}

} // namespace pers