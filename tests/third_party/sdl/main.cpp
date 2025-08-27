#include <SDL3/SDL.h>
#include <iostream>
#include <cstdlib>

int main(int argc, char* argv[]) {
    std::cout << "=== SDL3 Basic Test ===" << std::endl;
    
    // Initialize SDL
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
        return -1;
    }
    std::cout << "SDL3 initialized successfully" << std::endl;
    
    // Get SDL version
    int version = SDL_GetVersion();
    int major = SDL_VERSIONNUM_MAJOR(version);
    int minor = SDL_VERSIONNUM_MINOR(version);
    int patch = SDL_VERSIONNUM_MICRO(version);
    
    std::cout << "SDL3 version: " 
              << major << "." 
              << minor << "." 
              << patch << std::endl;
    
    // Check if running in CI environment
    const bool isCI = std::getenv("CI") != nullptr;
    
    // Create a window (skip in CI if no display)
    SDL_Window* window = nullptr;
    if (!isCI) {
        window = SDL_CreateWindow("SDL3 Test", 640, 480, SDL_WINDOW_HIDDEN);
        if (window) {
            std::cout << "SDL3 window created successfully" << std::endl;
            SDL_DestroyWindow(window);
        } else {
            std::cout << "SDL3 window creation skipped (headless environment)" << std::endl;
        }
    } else {
        std::cout << "CI mode: Skipping window creation" << std::endl;
    }
    
    // Get video driver info
    int numVideoDrivers = SDL_GetNumVideoDrivers();
    std::cout << "Available video drivers: " << numVideoDrivers << std::endl;
    for (int i = 0; i < numVideoDrivers; ++i) {
        const char* driverName = SDL_GetVideoDriver(i);
        std::cout << "  - " << driverName << std::endl;
    }
    
    const char* currentDriver = SDL_GetCurrentVideoDriver();
    if (currentDriver) {
        std::cout << "Current video driver: " << currentDriver << std::endl;
    }
    
    // Cleanup
    SDL_Quit();
    std::cout << "SDL3 quit successfully" << std::endl;
    std::cout << "=== SDL3 Test Passed ===" << std::endl;
    
    return 0;
}