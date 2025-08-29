#pragma once

/**
 * Platform-agnostic native window handle
 * Automatically includes the correct platform-specific implementation
 */

#ifdef _WIN32
    #include "pers/core/platform/win32/NativeWindowHandleWin32.h"
#elif defined(__APPLE__)
    #include "pers/core/platform/macos/NativeWindowHandleMacOS.h"
#elif defined(__linux__)
    #include "pers/core/platform/linux/NativeWindowHandleLinux.h"
#else
    #error "Unsupported platform"
#endif