#pragma once

#include <cstdint>
#include <string>

namespace pers {

/**
 * @brief Window descriptor for window creation
 */
struct WindowDesc {
    uint32_t width = 800;
    uint32_t height = 600;
    std::string title = "Pers Engine";
};

/**
 * @brief Window mode enumeration
 */
enum class WindowMode {
    Windowed,
    Fullscreen,
    BorderlessFullscreen
};

/**
 * @brief Window state flags
 */
enum class WindowState {
    Normal,
    Minimized,
    Maximized,
    Hidden
};

} // namespace pers