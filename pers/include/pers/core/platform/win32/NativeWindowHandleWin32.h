#pragma once

namespace pers {

/**
 * Windows native window handle
 */
struct NativeWindowHandle {
    void* hwnd = nullptr;  // HWND
    
    static NativeWindowHandle Create(void* hwnd) {
        NativeWindowHandle handle;
        handle.hwnd = hwnd;
        return handle;
    }
};

} // namespace pers