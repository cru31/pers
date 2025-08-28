# Pers Graphics Engine - Coding Conventions

## 1. Naming Conventions

### 1.1 Basic Rules

| Element | Convention | Example |
|---------|-----------|---------|
| **Member Variables** | Underscore(_) prefix + camelCase | `_window`, `_renderer`, `_isRunning` |
| **Member Functions** | camelCase (lowercase start) | `initialize()`, `processEvents()`, `shouldClose()` |
| **Free Functions** | camelCase (lowercase start) | `createInstance()`, `loadShader()` |
| **Local Variables** | camelCase | `deltaTime`, `currentFrame` |
| **Constants** | UPPER_SNAKE_CASE | `MAX_ENTITIES`, `DEFAULT_WIDTH` |
| **Classes/Structs** | PascalCase | `EventDispatcher`, `ColorCubeApp` |
| **Interfaces** | I prefix + PascalCase | `ISurfaceProvider`, `ILogicalDevice` |
| **Namespaces** | lowercase | `pers`, `renderer`, `events` |
| **Enums** | PascalCase | `GraphicsBackendType`, `PresentMode` |
| **Enum Values** | PascalCase | `HighPerformance`, `Immediate` |

### 1.2 Example Code

```cpp
namespace pers {
namespace renderer {

// Interface: I prefix + PascalCase
class ISurfaceProvider {
private:
    // Member variables: _ prefix + camelCase
    uint32_t _width;
    uint32_t _height;
    bool _isInitialized;
    
public:
    // Member functions: camelCase
    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
    virtual bool shouldClose() const = 0;
    virtual void getSurfaceSize(uint32_t& width, uint32_t& height) const = 0;
};

// Class: PascalCase
class GLFWSurfaceProvider final : public ISurfaceProvider {
private:
    GLFWwindow* _window = nullptr;
    
    // Constants: UPPER_SNAKE_CASE
    static constexpr uint32_t DEFAULT_WIDTH = 1280;
    static constexpr uint32_t DEFAULT_HEIGHT = 720;
    
public:
    // Member functions: camelCase
    bool initialize() override {
        // Local variables: camelCase
        int windowWidth = DEFAULT_WIDTH;
        int windowHeight = DEFAULT_HEIGHT;
        
        return true;
    }
};

// Enum: PascalCase
enum class GraphicsBackendType {
    WebGPU,      // Values: PascalCase
    Vulkan,
    Metal,
    D3D12
};

// Free function: camelCase
std::shared_ptr<ISurfaceProvider> createSurfaceProvider(GraphicsBackendType type);

} // namespace renderer
} // namespace pers
```

## 2. Class Design Principles

### 2.1 Basic Rules

#### Virtual Destructor Rule
```cpp
// Classes that may be inherited
class BaseClass {
public:
    virtual ~BaseClass() = default;  // Required: virtual destructor
};

// Classes that won't be inherited
class ConcreteClass final {  // Explicit final keyword
public:
    ~ConcreteClass() = default;  // virtual not needed
};
```

#### Rule of Five
```cpp
class ResourceOwner {
private:
    Resource* _resource;
    
public:
    // If you define destructor, define all five
    ~ResourceOwner() { delete _resource; }
    
    // Copy constructor
    ResourceOwner(const ResourceOwner& other) = delete;  // or implement
    
    // Copy assignment operator
    ResourceOwner& operator=(const ResourceOwner& other) = delete;
    
    // Move constructor
    ResourceOwner(ResourceOwner&& other) noexcept;
    
    // Move assignment operator
    ResourceOwner& operator=(ResourceOwner&& other) noexcept;
};
```

#### Override Keyword
```cpp
class Derived : public Base {
public:
    // Required: use override keyword
    void virtualMethod() override;
    
    // Compile error: prevents typos
    void virtalMethod() override;  // Error!
};
```

#### Explicit Constructors
```cpp
class Widget {
public:
    // Single parameter constructors must be explicit
    explicit Widget(int value);
    
    // Multiple parameters are optional
    Widget(int width, int height);
};
```

#### Const-Correctness
```cpp
class Container {
private:
    std::vector<int> _data;
    
public:
    // Const methods
    size_t size() const { return _data.size(); }
    
    // Const parameters
    void process(const std::string& input) const;
    
    // Const return values (pointers/references)
    const std::vector<int>& getData() const { return _data; }
};
```

## 3. Resource Management

### 3.1 Pointer Usage Rules

#### Prefer Smart Pointers
```cpp
// Good: use smart pointers
std::shared_ptr<IDevice> _device;
std::unique_ptr<CommandBuffer> _commandBuffer;

// Avoid: raw pointers
IDevice* device;  // Only for special cases (C API, performance critical)
```

#### Parameter Passing
```cpp
class Renderer {
public:
    // Pass shared_ptr by const reference
    void setDevice(const std::shared_ptr<IDevice>& device);
    
    // Pass by value only when transferring ownership
    void takeOwnership(std::shared_ptr<Resource> resource);
    
    // Pass regular objects by const reference
    void setViewMatrix(const glm::mat4& view);
    
    // Pass primitive types by value
    void setFrameCount(uint32_t count);
};
```

### 3.2 RAII Pattern
```cpp
class ScopedRenderPass {
private:
    ICommandEncoder* _encoder;
    
public:
    // Acquire in constructor
    explicit ScopedRenderPass(ICommandEncoder* encoder) 
        : _encoder(encoder) {
        _encoder->beginRenderPass();
    }
    
    // Release in destructor (automatic)
    ~ScopedRenderPass() {
        _encoder->endRenderPass();
    }
    
    // Prevent copying
    ScopedRenderPass(const ScopedRenderPass&) = delete;
    ScopedRenderPass& operator=(const ScopedRenderPass&) = delete;
};

// Usage
{
    ScopedRenderPass pass(encoder);
    // ... rendering commands ...
}  // Automatically calls endRenderPass()
```

## 4. Code Structure

### 4.1 Header File Structure
```cpp
#pragma once  // or include guards

// System headers
#include <memory>
#include <vector>

// External libraries
#include <glm/glm.hpp>

// Project headers
#include "pers/renderer/Types.h"

namespace pers {
namespace renderer {

// Forward declarations
class IDevice;
struct RenderPassDesc;

// Class definition
class ClassName {
    // Order: public -> protected -> private
public:
    // Constructors/destructor
    ClassName();
    ~ClassName();
    
    // Public methods
    void publicMethod();
    
protected:
    // Protected members
    void protectedMethod();
    
private:
    // Private members
    void privateMethod();
    
    // Member variables at the end
    std::shared_ptr<IDevice> _device;
};

} // namespace renderer
} // namespace pers
```

### 4.2 Source File Structure
```cpp
#include "ClassName.h"

// Additional headers
#include <iostream>

namespace pers {
namespace renderer {

// Anonymous namespace (file local)
namespace {
    constexpr uint32_t BUFFER_SIZE = 1024;
    
    void helperFunction() {
        // ...
    }
}

// Constructor/destructor
ClassName::ClassName() 
    : _device(nullptr) {
    // Initialization
}

ClassName::~ClassName() {
    // Cleanup
}

// Public methods
void ClassName::publicMethod() {
    // Implementation
}

// Protected methods
void ClassName::protectedMethod() {
    // Implementation
}

// Private methods
void ClassName::privateMethod() {
    // Implementation
}

} // namespace renderer
} // namespace pers
```

## 5. Comments and Documentation

### 5.1 Comment Style
```cpp
// Single line comment: brief description

/*
 * Multi-line comment:
 * For complex logic or algorithm explanations
 */

/**
 * @brief Doxygen-style documentation
 * @param device Device to use
 * @return true on success
 */
bool initialize(const std::shared_ptr<IDevice>& device);
```

### 5.2 TODO Comments
```cpp
// TODO: Needs optimization
// FIXME: Fix memory leak
// NOTE: Important note
// HACK: Temporary solution (needs improvement)
```

## 6. Error Handling

### 6.1 Exception Usage
```cpp
void criticalFunction() {
    if (!condition) {
        throw std::runtime_error("Detailed error message");
    }
}
```

### 6.2 Error Codes
```cpp
enum class ErrorCode {
    Success = 0,
    InvalidParameter,
    DeviceLost,
    OutOfMemory
};

ErrorCode initialize() {
    if (!checkCondition()) {
        return ErrorCode::InvalidParameter;
    }
    return ErrorCode::Success;
}
```

## 7. Performance Considerations

### 7.1 Inline Functions
```cpp
class Vector3 {
public:
    // Simple getters inline in header
    float x() const { return _x; }
    
    // Complex logic in cpp file
    float length() const;
};
```

### 7.2 Move Semantics
```cpp
class Buffer {
public:
    // Move constructor
    Buffer(Buffer&& other) noexcept
        : _data(std::move(other._data)) {
    }
    
    // Move assignment
    Buffer& operator=(Buffer&& other) noexcept {
        if (this != &other) {
            _data = std::move(other._data);
        }
        return *this;
    }
};
```

## 8. Platform Independence

### 8.1 Conditional Compilation
```cpp
#ifdef _WIN32
    // Windows specific
    #include <windows.h>
#elif __APPLE__
    // macOS specific
    #include <Metal/Metal.h>
#elif __linux__
    // Linux specific
    #include <X11/Xlib.h>
#endif
```

### 8.2 Type Definitions
```cpp
// Platform-independent types
using WindowHandle = void*;
using SurfaceHandle = void*;
```

## 9. Forbidden Practices

### 9.1 Absolutely Forbidden
- `using namespace std;` (in global scope)
- `#define` macros (use constexpr instead)
- C-style casts (use static_cast, etc.)
- `goto` statements
- Global variables (except singleton pattern)

### 9.2 Should Avoid
- Deep nesting (more than 3 levels)
- Long functions (more than 50 lines)
- Magic numbers (define constants)
- Complex conditionals (extract to functions)

## 10. Code Examples

### Complete Class Example
```cpp
// ISurfaceProvider.h
#pragma once

#include <cstdint>
#include <memory>

namespace pers {
namespace renderer {

/**
 * @brief Surface provider interface for window system integration
 */
class ISurfaceProvider {
public:
    virtual ~ISurfaceProvider() = default;
    
    /**
     * @brief Initialize the surface provider
     * @return true on success, false on failure
     */
    virtual bool initialize() = 0;
    
    /**
     * @brief Shutdown the surface provider
     */
    virtual void shutdown() = 0;
    
    /**
     * @brief Check if the surface should close
     * @return true if close requested
     */
    virtual bool shouldClose() const = 0;
    
    /**
     * @brief Process window system events
     */
    virtual void pollEvents() = 0;
    
    /**
     * @brief Get the native window handle
     * @return Platform-specific window handle
     */
    virtual void* getNativeWindowHandle() const = 0;
    
    /**
     * @brief Get the surface dimensions
     * @param[out] width Surface width in pixels
     * @param[out] height Surface height in pixels
     */
    virtual void getSurfaceSize(uint32_t& width, uint32_t& height) const = 0;
};

/**
 * @brief Create a surface provider for the specified backend
 * @param type Graphics backend type
 * @return Surface provider instance
 */
std::shared_ptr<ISurfaceProvider> createSurfaceProvider(GraphicsBackendType type);

} // namespace renderer
} // namespace pers
```

---

This document defines the coding conventions for the Pers Graphics Engine.
All contributors must follow these rules.