#pragma once

#include <memory>
#include <string>

class IWindow;

class IWindowFactory {
public:
    virtual ~IWindowFactory() = default;
    
    // Create a window with specified parameters
    virtual std::unique_ptr<IWindow> createWindow(int width, int height, const std::string& title) const = 0;
    
    // Get factory type name
    virtual const char* getFactoryName() const = 0;
};