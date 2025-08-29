#pragma once

namespace pers {

/**
 * macOS native window handle
 */
struct NativeWindowHandle {
    void* metalLayer = nullptr;  // CAMetalLayer*
    
    static NativeWindowHandle Create(void* metalLayer) {
        NativeWindowHandle handle;
        handle.metalLayer = metalLayer;
        return handle;
    }
};

} // namespace pers