# Pers Graphics Engine V2 - Interface Definitions

## Core Interfaces

### IInstance
```cpp
namespace pers::graphics {

interface IInstance {
    virtual ~IInstance() = default;
    
    // Adapter management
    virtual std::vector<std::shared_ptr<IAdapter>> enumerateAdapters() = 0;
    virtual std::shared_ptr<IAdapter> requestAdapter(const AdapterOptions& options) = 0;
    
    // Surface creation (platform-specific)
    virtual std::shared_ptr<ISurface> createSurface(void* windowHandle) = 0;
    
    // Information query
    virtual Version getVersion() const = 0;
    virtual std::vector<std::string> getExtensions() const = 0;
};

} // namespace pers::graphics
```

### IAdapter
```cpp
interface IAdapter {
    virtual ~IAdapter() = default;
    
    // Device creation
    virtual std::shared_ptr<IDevice> createDevice(const DeviceDescriptor& desc) = 0;
    
    // Adapter information
    virtual AdapterProperties getProperties() const = 0;
    virtual AdapterLimits getLimits() const = 0;
    virtual std::vector<DeviceFeature> getSupportedFeatures() const = 0;
    
    // Performance information
    virtual bool isDiscreteGPU() const = 0;
    virtual size_t getVideoMemorySize() const = 0;
};
```

### IDevice
```cpp
interface IDevice {
    virtual ~IDevice() = default;
    
    // Core component access - using shared_ptr for consistency
    virtual std::shared_ptr<IQueue> getQueue() = 0;
    virtual std::shared_ptr<IResourceFactory> getResourceFactory() = 0;
    
    // Command creation
    virtual std::shared_ptr<ICommandEncoder> createCommandEncoder(const CommandEncoderDesc& desc = {}) = 0;
    
    // SwapChain creation
    virtual std::shared_ptr<ISwapChain> createSwapChain(
        const std::shared_ptr<ISurface>& surface, 
        const SwapChainDesc& desc) = 0;
    
    // Device information
    virtual DeviceLimits getLimits() const = 0;
    virtual std::vector<DeviceFeature> getEnabledFeatures() const = 0;
    
    // Synchronization
    virtual void waitIdle() = 0;
};
```

## Resource Factory Interface

### IResourceFactory
```cpp
interface IResourceFactory {
    virtual ~IResourceFactory() = default;
    
    // Buffer creation
    virtual std::shared_ptr<IVertexBuffer> createVertexBuffer(const VertexBufferDesc& desc) = 0;
    virtual std::shared_ptr<IIndexBuffer> createIndexBuffer(const IndexBufferDesc& desc) = 0;
    virtual std::shared_ptr<IUniformBuffer> createUniformBuffer(const UniformBufferDesc& desc) = 0;
    virtual std::shared_ptr<IStorageBuffer> createStorageBuffer(const StorageBufferDesc& desc) = 0;
    virtual std::shared_ptr<IIndirectBuffer> createIndirectBuffer(const IndirectBufferDesc& desc) = 0;
    
    // Texture creation
    virtual std::shared_ptr<ITexture2D> createTexture2D(const Texture2DDesc& desc) = 0;
    virtual std::shared_ptr<ITexture3D> createTexture3D(const Texture3DDesc& desc) = 0;
    virtual std::shared_ptr<ITextureCube> createTextureCube(const TextureCubeDesc& desc) = 0;
    virtual std::shared_ptr<ITextureArray> createTextureArray(const TextureArrayDesc& desc) = 0;
    virtual std::shared_ptr<IRenderTexture> createRenderTexture(const RenderTextureDesc& desc) = 0;
    
    // View creation
    virtual std::shared_ptr<ITextureView> createTextureView(
        const std::shared_ptr<ITexture>& texture,
        const TextureViewDesc& desc) = 0;
    
    // Sampler creation
    virtual std::shared_ptr<ISampler> createSampler(const SamplerDesc& desc) = 0;
    
    // Shader creation
    virtual std::shared_ptr<IShaderModule> createShaderModule(const ShaderModuleDesc& desc) = 0;
    
    // Pipeline creation
    virtual std::shared_ptr<IGraphicsPipeline> createGraphicsPipeline(const GraphicsPipelineDesc& desc) = 0;
    virtual std::shared_ptr<IComputePipeline> createComputePipeline(const ComputePipelineDesc& desc) = 0;
    
    // Layout creation
    virtual std::shared_ptr<IPipelineLayout> createPipelineLayout(const PipelineLayoutDesc& desc) = 0;
    virtual std::shared_ptr<IBindGroupLayout> createBindGroupLayout(const BindGroupLayoutDesc& desc) = 0;
    
    // BindGroup creation
    virtual std::shared_ptr<IBindGroup> createBindGroup(const BindGroupDesc& desc) = 0;
    
    // Query creation
    virtual std::shared_ptr<IQuerySet> createQuerySet(const QuerySetDesc& desc) = 0;
};
```

## Buffer Interfaces

### IBuffer (Base)
```cpp
interface IBuffer {
    virtual ~IBuffer() = default;
    
    virtual size_t getSize() const = 0;
    virtual BufferState getState() const = 0;
    
    // Async mapping (WebGPU pattern)
    virtual void mapAsync(
        MapMode mode,
        size_t offset,
        size_t size,
        std::function<void(MappingResult)> callback) = 0;
    virtual void* getMappedRange(size_t offset = 0, size_t size = 0) = 0;
    virtual void unmap() = 0;
    virtual bool isMapped() const = 0;
};
```

### IVertexBuffer
```cpp
interface IVertexBuffer : public IBuffer {
    virtual size_t getVertexCount() const = 0;
    virtual size_t getStride() const = 0;
    virtual const VertexLayout& getLayout() const = 0;
};
```

### IIndexBuffer
```cpp
interface IIndexBuffer : public IBuffer {
    virtual size_t getIndexCount() const = 0;
    virtual IndexFormat getFormat() const = 0;
};
```

### IUniformBuffer
```cpp
interface IUniformBuffer : public IBuffer {
    virtual void update(const void* data, size_t offset = 0, size_t size = 0) = 0;
    virtual bool isDynamic() const = 0;
    virtual size_t getAlignment() const = 0;
};
```

### IStorageBuffer
```cpp
interface IStorageBuffer : public IBuffer {
    virtual bool isReadOnly() const = 0;
};
```

### IIndirectBuffer
```cpp
interface IIndirectBuffer : public IBuffer {
    virtual uint32_t getDrawCount() const = 0;
    virtual IndirectCommandType getCommandType() const = 0;
};
```

## Texture Interfaces

### ITexture (Base)
```cpp
interface ITexture {
    virtual ~ITexture() = default;
    
    virtual TextureFormat getFormat() const = 0;
    virtual TextureUsage getUsage() const = 0;
    virtual uint32_t getMipLevelCount() const = 0;
    virtual uint32_t getSampleCount() const = 0;
};
```

### ITexture2D
```cpp
interface ITexture2D : public ITexture {
    virtual uint32_t getWidth() const = 0;
    virtual uint32_t getHeight() const = 0;
};
```

### ITexture3D
```cpp
interface ITexture3D : public ITexture {
    virtual uint32_t getWidth() const = 0;
    virtual uint32_t getHeight() const = 0;
    virtual uint32_t getDepth() const = 0;
};
```

### ITextureCube
```cpp
interface ITextureCube : public ITexture {
    virtual uint32_t getSize() const = 0;  // Cubemap is square
};
```

### ITextureArray
```cpp
interface ITextureArray : public ITexture {
    virtual uint32_t getWidth() const = 0;
    virtual uint32_t getHeight() const = 0;
    virtual uint32_t getArrayLayers() const = 0;
};
```

### IRenderTexture
```cpp
interface IRenderTexture : public ITexture2D {
    virtual bool hasDepthStencil() const = 0;
    virtual std::shared_ptr<ITexture2D> getColorTexture() = 0;
    virtual std::shared_ptr<ITexture2D> getDepthStencilTexture() = 0;
};
```

## Pipeline Interfaces

### IPipeline (Base)
```cpp
interface IPipeline {
    virtual ~IPipeline() = default;
    
D:\cru31.dev\pers_graphics_engine\docs\renderer-core-design\r    virtual std::shared_ptr<IPipelineLayout> getLayout() const = 0;
};
```

### IGraphicsPipeline
```cpp
interface IGraphicsPipeline : public IPipeline {
    virtual PrimitiveTopology getPrimitiveTopology() const = 0;
    virtual const RasterizationState& getRasterizationState() const = 0;
    virtual const DepthStencilState& getDepthStencilState() const = 0;
    virtual const BlendState& getBlendState() const = 0;
    virtual const VertexLayout& getVertexLayout() const = 0;
};
```

### IComputePipeline
```cpp
interface IComputePipeline : public IPipeline {
    virtual WorkgroupSize getWorkgroupSize() const = 0;
};
```

## Command Interfaces

### Core Principle: Command Recording
All GPU commands are **recorded** into command buffers, clearly distinguished by `cmd` prefix.
- `cmdXXX()`: Commands recorded into GPU command buffer
- No prefix: Immediate CPU-side operations

### ICommandEncoder
```cpp
#include <span>

interface ICommandEncoder {
    virtual ~ICommandEncoder() = default;
    
    // Begin render/compute pass (immediate - returns encoder object)
    virtual std::shared_ptr<IRenderPassEncoder> beginRenderPass(const RenderPassDesc& desc) = 0;
    virtual std::shared_ptr<IComputePassEncoder> beginComputePass(const ComputePassDesc& desc = {}) = 0;
    
    // Copy commands (recorded outside pass)
    virtual void cmdCopyBufferToBuffer(
        const std::shared_ptr<IBuffer>& source,
        size_t sourceOffset,
        const std::shared_ptr<IBuffer>& destination,
        size_t destinationOffset,
        size_t size) = 0;
        
    virtual void cmdCopyBufferToTexture(
        const std::shared_ptr<IBuffer>& source,
        const std::shared_ptr<ITexture>& destination,
        const BufferTextureCopy& copy) = 0;
        
    virtual void cmdCopyTextureToBuffer(
        const std::shared_ptr<ITexture>& source,
        const std::shared_ptr<IBuffer>& destination,
        const TextureBufferCopy& copy) = 0;
        
    virtual void cmdCopyTextureToTexture(
        const std::shared_ptr<ITexture>& source,
        const std::shared_ptr<ITexture>& destination,
        const TextureTextureCopy& copy) = 0;
    
    // Debug markers (recorded)
    virtual void cmdPushDebugGroup(const std::string& label) = 0;
    virtual void cmdPopDebugGroup() = 0;
    virtual void cmdInsertDebugMarker(const std::string& label) = 0;
    
    // Finish recording (immediate - creates CommandBuffer)
    virtual std::shared_ptr<ICommandBuffer> finish() = 0;
    
    // State query (immediate)
    virtual bool isRecording() const = 0;
};
```

### IRenderPassEncoder
```cpp
interface IRenderPassEncoder {
    virtual ~IRenderPassEncoder() = default;
    
    // Pipeline state (recorded)
    virtual void cmdSetGraphicsPipeline(const std::shared_ptr<IGraphicsPipeline>& pipeline) = 0;
    
    // Vertex input (recorded)
    virtual void cmdSetVertexBuffer(uint32_t slot, const std::shared_ptr<IVertexBuffer>& buffer, size_t offset = 0) = 0;
    virtual void cmdSetIndexBuffer(const std::shared_ptr<IIndexBuffer>& buffer, size_t offset = 0) = 0;
    
    // Resource binding (recorded)
    virtual void cmdSetBindGroup(
        uint32_t index, 
        const std::shared_ptr<IBindGroup>& bindGroup,
        std::span<const uint32_t> dynamicOffsets = {}) = 0;
    virtual void cmdSetUniformBuffer(uint32_t index, const std::shared_ptr<IUniformBuffer>& buffer, size_t offset = 0) = 0;
    
    // Render state (recorded)
    virtual void cmdSetViewport(const Viewport& viewport) = 0;
    virtual void cmdSetScissorRect(const Rect& rect) = 0;
    virtual void cmdSetStencilReference(uint32_t reference) = 0;
    virtual void cmdSetBlendConstants(const Color& color) = 0;
    
    // Draw commands (recorded)
    virtual void cmdDraw(uint32_t vertexCount, uint32_t instanceCount = 1, 
                     uint32_t firstVertex = 0, uint32_t firstInstance = 0) = 0;
                     
    virtual void cmdDrawIndexed(uint32_t indexCount, uint32_t instanceCount = 1,
                            uint32_t firstIndex = 0, int32_t baseVertex = 0, 
                            uint32_t firstInstance = 0) = 0;
                            
    virtual void cmdDrawIndirect(const std::shared_ptr<IIndirectBuffer>& buffer, size_t offset = 0) = 0;
    virtual void cmdDrawIndexedIndirect(const std::shared_ptr<IIndirectBuffer>& buffer, size_t offset = 0) = 0;
    virtual void cmdDrawIndirectCount(
        const std::shared_ptr<IIndirectBuffer>& buffer,
        size_t offset,
        const std::shared_ptr<IBuffer>& countBuffer,
        size_t countOffset,
        uint32_t maxDrawCount) = 0;
    
    // Execute bundles (recorded)
    virtual void cmdExecuteBundles(const std::vector<std::shared_ptr<IRenderBundle>>& bundles) = 0;
    
    // Queries (recorded)
    virtual void cmdBeginQuery(const std::shared_ptr<IQuerySet>& querySet, uint32_t index) = 0;
    virtual void cmdEndQuery(const std::shared_ptr<IQuerySet>& querySet, uint32_t index) = 0;
    
    // End pass (immediate - closes encoder)
    virtual void end() = 0;
    
    // State query (immediate)
    virtual bool isActive() const = 0;
};
```

### IComputePassEncoder
```cpp
interface IComputePassEncoder {
    virtual ~IComputePassEncoder() = default;
    
    // Pipeline state (recorded)
    virtual void cmdSetComputePipeline(const std::shared_ptr<IComputePipeline>& pipeline) = 0;
    
    // Resource binding (recorded)
    virtual void cmdSetBindGroup(
        uint32_t index, 
        const std::shared_ptr<IBindGroup>& bindGroup,
        std::span<const uint32_t> dynamicOffsets = {}) = 0;
    virtual void cmdSetStorageBuffer(
        uint32_t index, 
        const std::shared_ptr<IStorageBuffer>& buffer,
        size_t offset = 0) = 0;
    
    // Dispatch commands (recorded)
    virtual void cmdDispatch(uint32_t workgroupsX, uint32_t workgroupsY = 1, uint32_t workgroupsZ = 1) = 0;
    virtual void cmdDispatchIndirect(const std::shared_ptr<IIndirectBuffer>& buffer, size_t offset = 0) = 0;
    
    // Query (recorded)
    virtual void cmdWriteTimestamp(const std::shared_ptr<IQuerySet>& querySet, uint32_t index) = 0;
    
    // End pass (immediate)
    virtual void end() = 0;
    
    // State query (immediate)
    virtual bool isActive() const = 0;
};
```

### IRenderBundleEncoder
```cpp
interface IRenderBundleEncoder {
    virtual ~IRenderBundleEncoder() = default;
    
    // Similar to RenderPass but limited
    virtual void cmdSetGraphicsPipeline(const std::shared_ptr<IGraphicsPipeline>& pipeline) = 0;
    virtual void cmdSetVertexBuffer(uint32_t slot, const std::shared_ptr<IVertexBuffer>& buffer, size_t offset = 0) = 0;
    virtual void cmdSetIndexBuffer(const std::shared_ptr<IIndexBuffer>& buffer, size_t offset = 0) = 0;
    virtual void cmdSetBindGroup(uint32_t index, const std::shared_ptr<IBindGroup>& bindGroup) = 0;
    
    virtual void cmdDraw(uint32_t vertexCount, uint32_t instanceCount = 1) = 0;
    virtual void cmdDrawIndexed(uint32_t indexCount, uint32_t instanceCount = 1) = 0;
    
    // Finish bundle (immediate)
    virtual std::shared_ptr<IRenderBundle> finish(const RenderBundleDesc& desc = {}) = 0;
};
```

### IRenderBundle
```cpp
interface IRenderBundle {
    virtual ~IRenderBundle() = default;
    
    // Bundle is opaque - no methods needed
};
```

## Queue & Synchronization

### IQueue
```cpp
interface IQueue {
    virtual ~IQueue() = default;
    
    virtual void submit(const std::shared_ptr<ICommandBuffer>& commandBuffer) = 0;
    virtual void submit(const std::vector<std::shared_ptr<ICommandBuffer>>& commandBuffers) = 0;
    
    // Direct data write (immediate)
    virtual void writeBuffer(const std::shared_ptr<IBuffer>& buffer, size_t offset, 
                           const void* data, size_t size) = 0;
    virtual void writeTexture(const std::shared_ptr<ITexture>& texture,
                            const TextureData& data, const TextureRegion& region) = 0;
    
    // Synchronization (immediate)
    virtual void waitIdle() = 0;
    virtual void onSubmittedWorkDone(std::function<void()> callback) = 0;
    
    // State query (immediate)
    virtual bool isIdle() const = 0;
};
```

### IQuerySet
```cpp
interface IQuerySet {
    virtual ~IQuerySet() = default;
    
    virtual QueryType GetType() const = 0;
    virtual uint32_t GetCount() const = 0;
    
    virtual void resolve(uint32_t firstQuery, uint32_t queryCount,
                        const std::shared_ptr<IBuffer>& destination,
                        size_t destinationOffset) = 0;
};
```

## Presentation Interfaces

### ISurface
```cpp
interface ISurface {
    virtual ~ISurface() = default;
    
    virtual void Configure(const SurfaceConfiguration& config) = 0;
    virtual SurfaceCapabilities GetCapabilities(const std::shared_ptr<IAdapter>& adapter) const = 0;
    virtual std::pair<uint32_t, uint32_t> GetSize() const = 0;
};
```

### ISwapChain
```cpp
interface ISwapChain {
    virtual ~ISwapChain() = default;
    
    virtual std::shared_ptr<ITextureView> getCurrentTextureView() = 0;
    virtual void present() = 0;
    
    virtual TextureFormat GetFormat() const = 0;
    virtual PresentMode GetPresentMode() const = 0;
    virtual std::pair<uint32_t, uint32_t> GetSize() const = 0;
    
    virtual void Resize(uint32_t width, uint32_t height) = 0;
};
```

---

이 인터페이스들은 **타입 안정성**, **명확한 책임**, **백엔드 독립성**을 보장합니다.