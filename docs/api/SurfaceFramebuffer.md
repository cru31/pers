# SurfaceFramebuffer API Documentation

## Overview
`SurfaceFramebuffer` is a concrete implementation of `ISurfaceFramebuffer` that manages the swap chain and provides a framebuffer interface for rendering to the screen.

## Class Hierarchy
```
IFramebuffer
  └── IResizableFramebuffer  
      └── ISurfaceFramebuffer
          └── SurfaceFramebuffer
```

## Key Features
- **Swap Chain Management**: Creates and manages the swap chain lifecycle
- **Depth Buffer Support**: Automatic depth buffer creation with configurable format
- **Acquire/Present Lifecycle**: Manages frame synchronization with the display
- **Create/Destroy Pattern**: Clean resource management with symmetric operations

## Constructor
```cpp
SurfaceFramebuffer(const std::shared_ptr<ILogicalDevice>& device)
```
- **device**: The logical device used for resource creation
- Only stores the device reference, no resources are created

## Resource Management

### create()
```cpp
bool create(const NativeSurfaceHandle& surface, const SwapChainDesc& desc)
```
Creates the swap chain and associated resources.

**Creation Order:**
1. Store dimensions and format from descriptor
2. Create swap chain through device
3. Create depth buffer if depth format is not Undefined

**Parameters:**
- `surface`: Native surface handle for the window
- `desc`: Swap chain configuration (size, format, present mode)

**Returns:** `true` if successful, `false` otherwise

### destroy()
```cpp
void destroy()
```
Destroys all resources in exact reverse order of creation.

**Destruction Order:**
1. Destroy depth framebuffer
2. Destroy swap chain  
3. Reset dimensions and format

**Important:** The destruction order is the exact reverse of creation order to ensure proper cleanup.

## Frame Lifecycle

### acquireNextImage()
```cpp
bool acquireNextImage()
```
Acquires the next available image from the swap chain for rendering.

**Behavior:**
- If an image is already acquired, it releases it first with a warning
- Gets the current texture view from the swap chain
- Sets internal state to track acquisition

**Returns:** `true` if successful, `false` if no image available

### present()
```cpp
void present()
```
Presents the rendered image to the screen.

**Behavior:**
- Calls present on the swap chain
- Releases the current color view
- Resets acquisition state

**Warning:** Only call after successful rendering to acquired image

### isReady()
```cpp
bool isReady() const
```
**Returns:** `true` if an image has been acquired and not yet presented

## Framebuffer Interface

### getColorAttachment()
```cpp
std::shared_ptr<ITextureView> getColorAttachment(uint32_t index) const
```
**Parameters:**
- `index`: Must be 0 (only one color attachment supported)

**Returns:** Current swap chain texture view if acquired, `nullptr` otherwise

### getDepthStencilAttachment()
```cpp
std::shared_ptr<ITextureView> getDepthStencilAttachment() const
```
**Returns:** Depth texture view if depth buffer exists, `nullptr` otherwise

### resize()
```cpp
bool resize(uint32_t width, uint32_t height)
```
Resizes the swap chain and recreates the depth buffer.

**Parameters:**
- `width`: New width in pixels
- `height`: New height in pixels

**Behavior:**
- Returns immediately if size hasn't changed
- Resizes swap chain
- Recreates depth buffer with new dimensions

## Depth Buffer Management

### setDepthFramebuffer()
```cpp
void setDepthFramebuffer(const std::shared_ptr<IFramebuffer>& depthFramebuffer)
```
Sets an external depth framebuffer instead of using the internally created one.

**Use Cases:**
- Sharing depth buffer between multiple surface framebuffers
- Custom depth buffer configuration
- Advanced rendering techniques requiring specific depth formats

**Warning:** Logs warning if dimensions don't match surface dimensions

## Internal State
- `_device`: Logical device for resource creation
- `_swapChain`: The WebGPU swap chain
- `_depthFramebuffer`: Depth buffer (OffscreenFramebuffer)
- `_currentColorView`: Currently acquired texture view
- `_width`, `_height`: Current dimensions
- `_format`: Color format (e.g., BGRA8Unorm)
- `_depthFormat`: Depth format (default: Depth24Plus)
- `_acquired`: Tracks if image is currently acquired

## Usage Example

```cpp
// Create surface framebuffer
auto surfaceFramebuffer = std::make_shared<SurfaceFramebuffer>(logicalDevice);

// Configure swap chain
SwapChainDesc desc;
desc.width = 800;
desc.height = 600;
desc.format = TextureFormat::BGRA8Unorm;
desc.presentMode = PresentMode::Fifo;

// Create swap chain
if (!surfaceFramebuffer->create(surface, desc)) {
    // Handle error
}

// Render loop
while (running) {
    // Acquire next image
    if (!surfaceFramebuffer->acquireNextImage()) {
        continue;
    }
    
    // Create render pass descriptor
    RenderPassDesc renderPassDesc;
    renderPassDesc.colorAttachments[0].view = surfaceFramebuffer->getColorAttachment(0);
    renderPassDesc.depthStencilAttachment.view = surfaceFramebuffer->getDepthStencilAttachment();
    
    // Render frame...
    
    // Present
    surfaceFramebuffer->present();
}

// Cleanup (automatic in destructor)
surfaceFramebuffer->destroy();
```

## Thread Safety
- Not thread-safe
- All methods must be called from the same thread
- Typically the main/render thread

## Error Handling
- Logs errors through Logger system
- Returns false/nullptr on failure
- Destructor ensures cleanup even if destroy() not called

## Platform Support
- WebGPU backend (current)
- Extensible to Vulkan, Metal, D3D12 through ILogicalDevice abstraction