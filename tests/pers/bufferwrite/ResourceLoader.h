#pragma once

#include <memory>
#include <vector>
#include <string>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <glm/common.hpp>

namespace pers {
    class IBuffer;
    class ILogicalDevice;
    class IQueue;
}

struct MeshData {
    std::vector<float> vertices;  // Interleaved vertex data
    std::vector<uint32_t> indices;
    
    // Vertex attributes info
    size_t vertexStride = 0;  // Bytes per vertex
    size_t positionOffset = 0;
    size_t normalOffset = 0;
    size_t texCoordOffset = 0;
    
    bool hasNormals = false;
    bool hasTexCoords = false;
    
    // Bounding box
    glm::vec3 minBounds;
    glm::vec3 maxBounds;
};

class ResourceLoader {
public:
    ResourceLoader() = default;
    ~ResourceLoader() = default;
    
    // Load mesh from file (supports .obj, .ply, etc. via Assimp)
    static bool loadMesh(const std::string& filepath, MeshData& outMesh);
    
    // Load Stanford Bunny (downloads if necessary)
    static bool loadStanfordBunny(MeshData& outMesh);
    
    // Create GPU buffers from mesh data using staging buffers
    static bool createGPUBuffers(
        const MeshData& mesh,
        const std::shared_ptr<pers::ILogicalDevice>& device,
        const std::shared_ptr<pers::IQueue>& queue,
        std::shared_ptr<pers::IBuffer>& outVertexBuffer,
        std::shared_ptr<pers::IBuffer>& outIndexBuffer
    );
    
private:
    // Helper to normalize mesh to fit in [-1, 1] cube
    static void normalizeMesh(MeshData& mesh);
    
    // Download file from URL
    static bool downloadFile(const std::string& url, const std::string& outputPath);
    
    // Check if file exists
    static bool fileExists(const std::string& path);
};