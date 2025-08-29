#pragma once

namespace pers {

/**
 * Linux native window handle
 * Supports both X11 and Wayland
 */
struct NativeWindowHandle {
    enum LinuxWindowSystem {
        Unknown = 0,
        X11,
        Wayland
    };
    
    LinuxWindowSystem type = Unknown;
    void* display = nullptr;  // Display* (X11) or wl_display* (Wayland)
    void* window = nullptr;   // Window (X11) or wl_surface* (Wayland)
    
    static NativeWindowHandle CreateX11(void* display, void* window) {
        NativeWindowHandle handle;
        handle.type = X11;
        handle.display = display;
        handle.window = window;
        return handle;
    }
    
    static NativeWindowHandle CreateWayland(void* display, void* surface) {
        NativeWindowHandle handle;
        handle.type = Wayland;
        handle.display = display;
        handle.window = surface;
        return handle;
    }
};

} // namespace pers