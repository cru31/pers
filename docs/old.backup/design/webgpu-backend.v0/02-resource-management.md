# WebGPU Resource Management

## 1. Buffer Management

### Buffer Types and Usage
Following WebGPU's explicit usage model from LearnWebGPU:

```cpp
namespace prism::webgpu {

enum class BufferUsage : uint32_t {
    MapRead      = WGPUBufferUsage_MapRead,
    MapWrite     = WGPUBufferUsage_MapWrite,
    CopySrc      = WGPUBufferUsage_CopySrc,
    CopyDst      = WGPUBufferUsage_CopyDst,
    Index        = WGPUBufferUsage_Index,
    Vertex       = WGPUBufferUsage_Vertex,
    Uniform      = WGPUBufferUsage_Uniform,
    Storage      = WGPUBufferUsage_Storage,
    Indirect     = WGPUBufferUsage_Indirect,
    QueryResolve = WGPUBufferUsage_QueryResolve
};

class Buffer {
private:
    WGPUBuffer handle;
    WGPUDevice device;
    size_t size;
    BufferUsage usage;
    bool mapped = false;
    
public:
    static BufferPtr Create(Device& device, const BufferDescriptor& desc) {
        WGPUBufferDescriptor wgpuDesc = {};
        wgpuDesc.label = desc.label.c_str();
        wgpuDesc.size = desc.size;
        wgpuDesc.usage = static_cast<WGPUBufferUsageFlags>(desc.usage);
        wgpuDesc.mappedAtCreation = desc.mappedAtCreation;
        
        WGPUBuffer buffer = wgpuDeviceCreateBuffer(device.GetHandle(), &wgpuDesc);
        return std::make_shared<Buffer>(device, buffer, desc);
    }
    
    void MapAsync(MapMode mode, size_t offset, size_t size,
                  std::function<void(BufferMapStatus)> callback) {
        wgpuBufferMapAsync(
            handle,
            static_cast<WGPUMapModeFlags>(mode),
            offset,
            size,
            [](WGPUBufferMapAsyncStatus status, void* userdata) {
                auto* cb = static_cast<std::function<void(BufferMapStatus)>*>(userdata);
                (*cb)(static_cast<BufferMapStatus>(status));
                delete cb;
            },
            new std::function<void(BufferMapStatus)>(callback)
        );
    }
    
    void* GetMappedRange(size_t offset = 0, size_t size = WGPU_WHOLE_MAP_SIZE) {
        return wgpuBufferGetMappedRange(handle, offset, size);
    }
    
    void Unmap() {
        wgpuBufferUnmap(handle);
        mapped = false;
    }
};

}
```

### Dynamic Buffer Management
Ring buffer for per-frame dynamic data:

```cpp
class DynamicBufferAllocator {
private:
    struct Frame {
        BufferPtr buffer;
        size_t currentOffset = 0;
        void* mappedData = nullptr;
    };
    
    static constexpr size_t FRAME_COUNT = 3;
    static constexpr size_t BUFFER_SIZE = 16 * 1024 * 1024; // 16MB per frame
    static constexpr size_t ALIGNMENT = 256; // WebGPU uniform buffer alignment
    
    Frame frames[FRAME_COUNT];
    uint32_t currentFrame = 0;
    
public:
    struct Allocation {
        BufferPtr buffer;
        size_t offset;
        size_t size;
        void* mappedPtr;
    };
    
    Allocation Allocate(size_t size) {
        size = AlignUp(size, ALIGNMENT);
        Frame& frame = frames[currentFrame];
        
        if (frame.currentOffset + size > BUFFER_SIZE) {
            // Out of space in current frame
            throw std::runtime_error("Dynamic buffer overflow");
        }
        
        Allocation alloc;
        alloc.buffer = frame.buffer;
        alloc.offset = frame.currentOffset;
        alloc.size = size;
        alloc.mappedPtr = static_cast<uint8_t*>(frame.mappedData) + frame.currentOffset;
        
        frame.currentOffset += size;
        return alloc;
    }
    
    void NextFrame() {
        currentFrame = (currentFrame + 1) % FRAME_COUNT;
        frames[currentFrame].currentOffset = 0;
    }
};
```

### Staging Buffer Pool
For efficient GPU uploads:

```cpp
class StagingBufferManager {
private:
    struct StagingBuffer {
        BufferPtr buffer;
        size_t size;
        bool inUse = false;
        uint32_t lastUsedFrame = 0;
    };
    
    std::vector<StagingBuffer> pool;
    uint32_t currentFrame = 0;
    
public:
    BufferPtr AcquireBuffer(size_t size) {
        // Try to find existing buffer
        for (auto& staging : pool) {
            if (!staging.inUse && staging.size >= size) {
                staging.inUse = true;
                staging.lastUsedFrame = currentFrame;
                return staging.buffer;
            }
        }
        
        // Create new staging buffer
        BufferDescriptor desc;
        desc.size = AlignUp(size, 65536); // 64KB minimum
        desc.usage = BufferUsage::MapWrite | BufferUsage::CopySrc;
        desc.mappedAtCreation = false;
        
        auto buffer = Buffer::Create(device, desc);
        pool.push_back({buffer, desc.size, true, currentFrame});
        return buffer;
    }
    
    void ReleaseBuffer(BufferPtr buffer) {
        for (auto& staging : pool) {
            if (staging.buffer == buffer) {
                staging.inUse = false;
                break;
            }
        }
    }
    
    void GarbageCollect() {
        // Remove buffers not used for 10 frames
        pool.erase(
            std::remove_if(pool.begin(), pool.end(),
                [this](const StagingBuffer& s) {
                    return !s.inUse && (currentFrame - s.lastUsedFrame) > 10;
                }),
            pool.end()
        );
    }
};
```

## 2. Texture Management

### Texture Creation and Loading
```cpp
class Texture {
private:
    WGPUTexture handle;
    WGPUTextureView defaultView;
    WGPUExtent3D size;
    WGPUTextureFormat format;
    uint32_t mipLevelCount;
    uint32_t sampleCount;
    
public:
    static TexturePtr Create(Device& device, const TextureDescriptor& desc) {
        WGPUTextureDescriptor wgpuDesc = {};
        wgpuDesc.label = desc.label.c_str();
        wgpuDesc.size = {desc.width, desc.height, desc.depthOrArrayLayers};
        wgpuDesc.mipLevelCount = desc.mipLevelCount;
        wgpuDesc.sampleCount = desc.sampleCount;
        wgpuDesc.dimension = static_cast<WGPUTextureDimension>(desc.dimension);
        wgpuDesc.format = static_cast<WGPUTextureFormat>(desc.format);
        wgpuDesc.usage = static_cast<WGPUTextureUsageFlags>(desc.usage);
        
        WGPUTexture texture = wgpuDeviceCreateTexture(device.GetHandle(), &wgpuDesc);
        
        // Create default view
        WGPUTextureView view = wgpuTextureCreateView(texture, nullptr);
        
        return std::make_shared<Texture>(texture, view, desc);
    }
    
    void Upload(CommandEncoder& encoder, const void* data, size_t dataSize,
                uint32_t mipLevel = 0, uint32_t arrayLayer = 0) {
        // Create staging buffer
        auto staging = stagingManager.AcquireBuffer(dataSize);
        
        // Map and copy data
        staging->MapAsync(MapMode::Write, 0, dataSize,
            [data, dataSize, staging](BufferMapStatus status) {
                if (status == BufferMapStatus::Success) {
                    void* mapped = staging->GetMappedRange(0, dataSize);
                    memcpy(mapped, data, dataSize);
                    staging->Unmap();
                }
            });
        
        // Copy from staging to texture
        WGPUImageCopyBuffer source = {};
        source.buffer = staging->GetHandle();
        source.layout.bytesPerRow = CalculateBytesPerRow(format, size.width);
        source.layout.rowsPerImage = size.height;
        
        WGPUImageCopyTexture destination = {};
        destination.texture = handle;
        destination.mipLevel = mipLevel;
        destination.origin = {0, 0, arrayLayer};
        
        wgpuCommandEncoderCopyBufferToTexture(
            encoder.GetHandle(),
            &source,
            &destination,
            &size
        );
    }
};
```

### Texture Cache
```cpp
class TextureCache {
private:
    struct TextureEntry {
        TexturePtr texture;
        std::string path;
        uint32_t refCount = 1;
        uint32_t lastAccessFrame = 0;
        size_t memorySize = 0;
    };
    
    std::unordered_map<std::string, TextureEntry> cache;
    size_t totalMemoryUsage = 0;
    size_t memoryBudget = 512 * 1024 * 1024; // 512MB
    
public:
    TexturePtr LoadTexture(const std::string& path, bool generateMips = true) {
        // Check cache
        auto it = cache.find(path);
        if (it != cache.end()) {
            it->second.refCount++;
            it->second.lastAccessFrame = GetCurrentFrame();
            return it->second.texture;
        }
        
        // Load from file
        ImageData imageData = LoadImageFromFile(path);
        
        // Create texture
        TextureDescriptor desc;
        desc.width = imageData.width;
        desc.height = imageData.height;
        desc.format = DetermineFormat(imageData);
        desc.mipLevelCount = generateMips ? CalculateMipLevels(desc.width, desc.height) : 1;
        desc.usage = TextureUsage::TextureBinding | TextureUsage::CopyDst;
        
        auto texture = Texture::Create(device, desc);
        
        // Upload data
        texture->Upload(encoder, imageData.pixels.data(), imageData.pixels.size());
        
        // Generate mipmaps if requested
        if (generateMips) {
            GenerateMipmaps(texture);
        }
        
        // Add to cache
        TextureEntry entry;
        entry.texture = texture;
        entry.path = path;
        entry.memorySize = CalculateTextureMemorySize(desc);
        entry.lastAccessFrame = GetCurrentFrame();
        
        cache[path] = entry;
        totalMemoryUsage += entry.memorySize;
        
        // Evict if over budget
        EvictLRU();
        
        return texture;
    }
    
private:
    void EvictLRU() {
        while (totalMemoryUsage > memoryBudget) {
            // Find least recently used texture with refCount == 0
            auto oldest = cache.end();
            uint32_t oldestFrame = UINT32_MAX;
            
            for (auto it = cache.begin(); it != cache.end(); ++it) {
                if (it->second.refCount == 0 && it->second.lastAccessFrame < oldestFrame) {
                    oldest = it;
                    oldestFrame = it->second.lastAccessFrame;
                }
            }
            
            if (oldest != cache.end()) {
                totalMemoryUsage -= oldest->second.memorySize;
                cache.erase(oldest);
            } else {
                break; // No textures can be evicted
            }
        }
    }
};
```

## 3. Bind Group Management

### Bind Group Layout Cache
```cpp
class BindGroupLayoutCache {
private:
    struct LayoutKey {
        std::vector<WGPUBindGroupLayoutEntry> entries;
        
        bool operator==(const LayoutKey& other) const {
            return entries == other.entries;
        }
    };
    
    struct LayoutKeyHash {
        size_t operator()(const LayoutKey& key) const {
            size_t hash = 0;
            for (const auto& entry : key.entries) {
                hash ^= std::hash<uint32_t>{}(entry.binding);
                hash ^= std::hash<uint32_t>{}(entry.visibility);
                hash ^= std::hash<uint32_t>{}(static_cast<uint32_t>(entry.buffer.type));
            }
            return hash;
        }
    };
    
    std::unordered_map<LayoutKey, WGPUBindGroupLayout, LayoutKeyHash> cache;
    
public:
    WGPUBindGroupLayout GetOrCreate(Device& device, const std::vector<WGPUBindGroupLayoutEntry>& entries) {
        LayoutKey key{entries};
        
        auto it = cache.find(key);
        if (it != cache.end()) {
            return it->second;
        }
        
        WGPUBindGroupLayoutDescriptor desc = {};
        desc.entryCount = entries.size();
        desc.entries = entries.data();
        
        WGPUBindGroupLayout layout = wgpuDeviceCreateBindGroupLayout(device.GetHandle(), &desc);
        cache[key] = layout;
        
        return layout;
    }
};
```

### Bind Group Pool
```cpp
class BindGroupPool {
private:
    struct BindGroupEntry {
        WGPUBindGroup bindGroup;
        uint64_t hash;
        uint32_t frameLastUsed;
        bool inUse;
    };
    
    std::vector<BindGroupEntry> pool;
    uint32_t currentFrame = 0;
    
public:
    WGPUBindGroup GetOrCreate(Device& device, 
                              WGPUBindGroupLayout layout,
                              const std::vector<WGPUBindGroupEntry>& entries) {
        uint64_t hash = ComputeBindGroupHash(entries);
        
        // Try to find existing bind group
        for (auto& entry : pool) {
            if (!entry.inUse && entry.hash == hash) {
                entry.inUse = true;
                entry.frameLastUsed = currentFrame;
                return entry.bindGroup;
            }
        }
        
        // Create new bind group
        WGPUBindGroupDescriptor desc = {};
        desc.layout = layout;
        desc.entryCount = entries.size();
        desc.entries = entries.data();
        
        WGPUBindGroup bindGroup = wgpuDeviceCreateBindGroup(device.GetHandle(), &desc);
        
        pool.push_back({bindGroup, hash, currentFrame, true});
        return bindGroup;
    }
    
    void ReleaseAll() {
        for (auto& entry : pool) {
            entry.inUse = false;
        }
    }
    
    void GarbageCollect() {
        // Remove bind groups not used for 10 frames
        pool.erase(
            std::remove_if(pool.begin(), pool.end(),
                [this](const BindGroupEntry& e) {
                    return !e.inUse && (currentFrame - e.frameLastUsed) > 10;
                }),
            pool.end()
        );
    }
    
private:
    uint64_t ComputeBindGroupHash(const std::vector<WGPUBindGroupEntry>& entries) {
        uint64_t hash = 0;
        for (const auto& entry : entries) {
            hash ^= std::hash<uint32_t>{}(entry.binding);
            hash ^= std::hash<void*>{}(entry.buffer);
            hash ^= std::hash<void*>{}(entry.textureView);
            hash ^= std::hash<void*>{}(entry.sampler);
        }
        return hash;
    }
};
```

## 4. Pipeline Management

### Pipeline State Cache
```cpp
class PipelineCache {
private:
    struct PipelineKey {
        std::string vertexShader;
        std::string fragmentShader;
        WGPUVertexState vertexState;
        WGPUPrimitiveState primitiveState;
        WGPUDepthStencilState depthStencilState;
        WGPUMultisampleState multisampleState;
        std::vector<WGPUColorTargetState> colorTargets;
        
        bool operator==(const PipelineKey& other) const;
        size_t Hash() const;
    };
    
    std::unordered_map<size_t, WGPURenderPipeline> renderPipelines;
    std::unordered_map<size_t, WGPUComputePipeline> computePipelines;
    
public:
    WGPURenderPipeline GetOrCreateRenderPipeline(
        Device& device,
        const RenderPipelineDescriptor& desc) {
        
        PipelineKey key = CreateKey(desc);
        size_t hash = key.Hash();
        
        auto it = renderPipelines.find(hash);
        if (it != renderPipelines.end()) {
            return it->second;
        }
        
        // Create pipeline
        WGPURenderPipelineDescriptor wgpuDesc = ConvertDescriptor(desc);
        WGPURenderPipeline pipeline = wgpuDeviceCreateRenderPipeline(
            device.GetHandle(), &wgpuDesc);
        
        renderPipelines[hash] = pipeline;
        return pipeline;
    }
    
    WGPUComputePipeline GetOrCreateComputePipeline(
        Device& device,
        const ComputePipelineDescriptor& desc) {
        
        size_t hash = ComputeHash(desc);
        
        auto it = computePipelines.find(hash);
        if (it != computePipelines.end()) {
            return it->second;
        }
        
        WGPUComputePipelineDescriptor wgpuDesc = ConvertDescriptor(desc);
        WGPUComputePipeline pipeline = wgpuDeviceCreateComputePipeline(
            device.GetHandle(), &wgpuDesc);
        
        computePipelines[hash] = pipeline;
        return pipeline;
    }
};
```

## 5. Memory Allocation Strategy

### Memory Heaps
```cpp
class MemoryAllocator {
private:
    struct MemoryHeap {
        BufferPtr buffer;
        size_t size;
        size_t used;
        std::vector<std::pair<size_t, size_t>> freeBlocks; // offset, size
    };
    
    std::vector<MemoryHeap> heaps;
    static constexpr size_t HEAP_SIZE = 256 * 1024 * 1024; // 256MB heaps
    
public:
    struct Allocation {
        BufferPtr buffer;
        size_t offset;
        size_t size;
    };
    
    Allocation Allocate(size_t size, BufferUsage usage) {
        // Find heap with enough space
        for (auto& heap : heaps) {
            if (heap.buffer->GetUsage() == usage) {
                auto block = FindFreeBlock(heap, size);
                if (block.first != SIZE_MAX) {
                    Allocation alloc;
                    alloc.buffer = heap.buffer;
                    alloc.offset = block.first;
                    alloc.size = size;
                    return alloc;
                }
            }
        }
        
        // Create new heap
        BufferDescriptor desc;
        desc.size = HEAP_SIZE;
        desc.usage = usage;
        
        MemoryHeap newHeap;
        newHeap.buffer = Buffer::Create(device, desc);
        newHeap.size = HEAP_SIZE;
        newHeap.used = size;
        newHeap.freeBlocks.push_back({size, HEAP_SIZE - size});
        
        heaps.push_back(newHeap);
        
        Allocation alloc;
        alloc.buffer = newHeap.buffer;
        alloc.offset = 0;
        alloc.size = size;
        return alloc;
    }
    
    void Free(const Allocation& alloc) {
        for (auto& heap : heaps) {
            if (heap.buffer == alloc.buffer) {
                // Add to free list and coalesce
                heap.freeBlocks.push_back({alloc.offset, alloc.size});
                CoalesceFreeBlocks(heap);
                heap.used -= alloc.size;
                break;
            }
        }
    }
    
private:
    std::pair<size_t, size_t> FindFreeBlock(MemoryHeap& heap, size_t size) {
        for (auto it = heap.freeBlocks.begin(); it != heap.freeBlocks.end(); ++it) {
            if (it->second >= size) {
                size_t offset = it->first;
                it->first += size;
                it->second -= size;
                if (it->second == 0) {
                    heap.freeBlocks.erase(it);
                }
                return {offset, size};
            }
        }
        return {SIZE_MAX, 0};
    }
    
    void CoalesceFreeBlocks(MemoryHeap& heap) {
        std::sort(heap.freeBlocks.begin(), heap.freeBlocks.end());
        
        for (size_t i = 0; i < heap.freeBlocks.size() - 1; ) {
            if (heap.freeBlocks[i].first + heap.freeBlocks[i].second == 
                heap.freeBlocks[i + 1].first) {
                heap.freeBlocks[i].second += heap.freeBlocks[i + 1].second;
                heap.freeBlocks.erase(heap.freeBlocks.begin() + i + 1);
            } else {
                ++i;
            }
        }
    }
};
```

## 6. Resource Lifecycle Management

### Reference Counting
```cpp
template<typename T>
class ResourceHandle {
private:
    std::shared_ptr<T> resource;
    std::atomic<uint32_t>* refCount;
    
public:
    ResourceHandle() : resource(nullptr), refCount(nullptr) {}
    
    explicit ResourceHandle(std::shared_ptr<T> res) 
        : resource(res), refCount(new std::atomic<uint32_t>(1)) {}
    
    ResourceHandle(const ResourceHandle& other) 
        : resource(other.resource), refCount(other.refCount) {
        if (refCount) {
            refCount->fetch_add(1);
        }
    }
    
    ~ResourceHandle() {
        Release();
    }
    
    void Release() {
        if (refCount && refCount->fetch_sub(1) == 1) {
            delete refCount;
            resource.reset();
        }
    }
    
    T* operator->() const { return resource.get(); }
    T& operator*() const { return *resource; }
    bool IsValid() const { return resource != nullptr; }
};
```

### Deferred Destruction
```cpp
class DeferredDestructionQueue {
private:
    struct Entry {
        std::function<void()> destructor;
        uint32_t frameSubmitted;
    };
    
    std::queue<Entry> destructionQueue;
    uint32_t currentFrame = 0;
    static constexpr uint32_t FRAMES_IN_FLIGHT = 3;
    
public:
    template<typename T>
    void QueueDestruction(T* resource) {
        destructionQueue.push({
            [resource]() { delete resource; },
            currentFrame
        });
    }
    
    void ProcessDestructions() {
        while (!destructionQueue.empty()) {
            const Entry& entry = destructionQueue.front();
            if (currentFrame - entry.frameSubmitted >= FRAMES_IN_FLIGHT) {
                entry.destructor();
                destructionQueue.pop();
            } else {
                break; // Still in use by GPU
            }
        }
    }
    
    void NextFrame() {
        currentFrame++;
        ProcessDestructions();
    }
};
```