#include "ResourceLoader.h"
#include "pers/graphics/ILogicalDevice.h"
#include "pers/graphics/IResourceFactory.h"
#include "pers/graphics/IQueue.h"
#include "pers/graphics/ICommandEncoder.h"
#include "pers/graphics/buffers/IBuffer.h"
#include "pers/graphics/buffers/ImmediateStagingBuffer.h"
#include "pers/graphics/buffers/DeviceBuffer.h"
#include "pers/graphics/buffers/DeviceBufferUsage.h"
#include "pers/utils/Logger.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <limits>

#ifdef _WIN32
#define NOMINMAX  // Prevent Windows.h from defining min/max macros
#include <windows.h>
#include <urlmon.h>
#pragma comment(lib, "urlmon.lib")
#endif

bool ResourceLoader::loadMesh(const std::string& filepath, MeshData& outMesh) {
    LOG_INFO("ResourceLoader", "Loading mesh from: " + filepath);
    
    // Check if file exists
    if (!std::filesystem::exists(filepath)) {
        LOG_ERROR("ResourceLoader", "File does not exist: " + filepath);
        return false;
    }
    
    // Use Assimp to load the mesh
    Assimp::Importer importer;
    
    // Configure import options
    unsigned int flags = 
        aiProcess_Triangulate |               // Convert to triangles
        aiProcess_JoinIdenticalVertices |     // Optimize vertices
        aiProcess_GenSmoothNormals |                // Generate normals if missing
        aiProcess_CalcTangentSpace |          // Calculate tangents
        aiProcess_OptimizeMeshes |            // Optimize mesh
        aiProcess_PreTransformVertices;       // Apply node transformations
    
    const aiScene* scene = importer.ReadFile(filepath, flags);
    
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        LOG_ERROR("ResourceLoader", "Assimp error: " + std::string(importer.GetErrorString()));
        return false;
    }
    
    // Clear output mesh
    outMesh.vertices.clear();
    outMesh.indices.clear();
    
    // Initialize bounds
    outMesh.minBounds = glm::vec3(std::numeric_limits<float>::max());
    outMesh.maxBounds = glm::vec3(std::numeric_limits<float>::lowest());
    
    // Process all meshes in the scene
    uint32_t baseVertex = 0;
    
    for (unsigned int m = 0; m < scene->mNumMeshes; m++) {
        const aiMesh* mesh = scene->mMeshes[m];
        
        // Check what attributes are available
        bool hasNormals = mesh->HasNormals();
        bool hasTexCoords = mesh->HasTextureCoords(0);
        
        // Set mesh data info (use first mesh's attributes for consistency)
        if (m == 0) {
            outMesh.hasNormals = hasNormals;
            outMesh.hasTexCoords = hasTexCoords;
            
            // Calculate stride and offsets
            outMesh.positionOffset = 0;
            outMesh.vertexStride = 3 * sizeof(float);  // Position (x, y, z)
            
            if (hasNormals) {
                outMesh.normalOffset = outMesh.vertexStride;
                outMesh.vertexStride += 3 * sizeof(float);  // Normal (nx, ny, nz)
            }
            
            if (hasTexCoords) {
                outMesh.texCoordOffset = outMesh.vertexStride;
                outMesh.vertexStride += 2 * sizeof(float);  // TexCoord (u, v)
            }
        }
        
        // Process vertices
        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            // Position
            glm::vec3 position(
                mesh->mVertices[i].x,
                mesh->mVertices[i].y,
                mesh->mVertices[i].z
            );
            
            outMesh.vertices.push_back(position.x);
            outMesh.vertices.push_back(position.y);
            outMesh.vertices.push_back(position.z);
            
            // Update bounds
            outMesh.minBounds = glm::min(outMesh.minBounds, position);
            outMesh.maxBounds = glm::max(outMesh.maxBounds, position);
            
            // Normal
            if (hasNormals) {
                outMesh.vertices.push_back(mesh->mNormals[i].x);
                outMesh.vertices.push_back(mesh->mNormals[i].y);
                outMesh.vertices.push_back(mesh->mNormals[i].z);
            }
            
            // Texture coordinates
            if (hasTexCoords) {
                outMesh.vertices.push_back(mesh->mTextureCoords[0][i].x);
                outMesh.vertices.push_back(mesh->mTextureCoords[0][i].y);
            }
        }
        
        // Process indices
        for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
            const aiFace& face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++) {
                outMesh.indices.push_back(baseVertex + face.mIndices[j]);
            }
        }
        
        baseVertex += mesh->mNumVertices;
    }
    
    LOG_INFO("ResourceLoader", 
        "Loaded mesh with " + std::to_string(outMesh.vertices.size() / (outMesh.vertexStride / sizeof(float))) + 
        " vertices and " + std::to_string(outMesh.indices.size() / 3) + " triangles");
    
    // Normalize the mesh to fit in [-1, 1] cube
    normalizeMesh(outMesh);
    
    return true;
}

bool ResourceLoader::loadStanfordBunny(MeshData& outMesh) {
    // Try multiple paths for the bunny model
    std::vector<std::string> possiblePaths = {
        "resources/bunny.obj",
        "resources/bunny.ply",
        "resources/stanford_bunny.obj",
        "resources/stanford_bunny.ply",
        "../resources/bunny.obj",
        "../resources/bunny.ply",
        "D:/cru31.dev/pers_work/pers_repo/tests/pers/bufferwrite/resources/bunny.ply",
        "D:/cru31.dev/pers_work/pers_repo/tests/pers/bufferwrite/resources/bunny.obj"
    };
    
    // Try to find existing bunny file
    for (const auto& path : possiblePaths) {
        if (fileExists(path)) {
            LOG_INFO("ResourceLoader", "Found Stanford Bunny at: " + path);
            return loadMesh(path, outMesh);
        }
    }
    
    // If not found, try to download it
    LOG_INFO("ResourceLoader", "Stanford Bunny not found locally, attempting to download...");
    
    // Create resources directory if it doesn't exist
    std::filesystem::create_directories("resources");
    
    // Stanford Bunny PLY format URL (from Stanford 3D Scanning Repository)
    std::string url = "http://graphics.stanford.edu/pub/3Dscanrep/bunny/reconstruction/bun_zipper_res2.ply";
    std::string outputPath = "resources/bunny.ply";
    
    if (downloadFile(url, outputPath)) {
        LOG_INFO("ResourceLoader", "Successfully downloaded Stanford Bunny");
        return loadMesh(outputPath, outMesh);
    }
    
    // If download failed, create a simple procedural bunny-like shape
    LOG_WARNING("ResourceLoader", "Could not download Stanford Bunny, creating procedural approximation");
    
    // Create a simple sphere as fallback
    const int segments = 20;
    const int rings = 20;
    
    outMesh.vertices.clear();
    outMesh.indices.clear();
    outMesh.hasNormals = true;
    outMesh.hasTexCoords = false;
    outMesh.vertexStride = 6 * sizeof(float);  // position + normal
    outMesh.positionOffset = 0;
    outMesh.normalOffset = 3 * sizeof(float);
    
    // Generate sphere vertices
    for (int ring = 0; ring <= rings; ring++) {
        float phi = float(ring) / float(rings) * 3.14159265f;
        float y = cos(phi);
        float r = sin(phi);
        
        for (int segment = 0; segment <= segments; segment++) {
            float theta = float(segment) / float(segments) * 2.0f * 3.14159265f;
            float x = r * cos(theta);
            float z = r * sin(theta);
            
            // Add some deformation to make it more bunny-like
            float deform = 1.0f;
            if (y > 0.3f) {  // Head area
                deform = 0.8f;
            } else if (y < -0.3f) {  // Bottom
                deform = 1.2f;
            }
            
            x *= deform;
            z *= deform;
            
            // Position
            outMesh.vertices.push_back(x * 0.5f);
            outMesh.vertices.push_back(y * 0.6f);
            outMesh.vertices.push_back(z * 0.5f);
            
            // Normal (for sphere, normal = normalized position)
            float len = sqrt(x*x + y*y + z*z);
            outMesh.vertices.push_back(x / len);
            outMesh.vertices.push_back(y / len);
            outMesh.vertices.push_back(z / len);
        }
    }
    
    // Generate sphere indices
    for (int ring = 0; ring < rings; ring++) {
        for (int segment = 0; segment < segments; segment++) {
            int current = ring * (segments + 1) + segment;
            int next = current + segments + 1;
            
            // Triangle 1
            outMesh.indices.push_back(current);
            outMesh.indices.push_back(next);
            outMesh.indices.push_back(current + 1);
            
            // Triangle 2
            outMesh.indices.push_back(current + 1);
            outMesh.indices.push_back(next);
            outMesh.indices.push_back(next + 1);
        }
    }
    
    // Set bounds
    outMesh.minBounds = glm::vec3(-0.5f, -0.6f, -0.5f);
    outMesh.maxBounds = glm::vec3(0.5f, 0.6f, 0.5f);
    
    LOG_INFO("ResourceLoader", "Created procedural mesh with " + 
        std::to_string(outMesh.vertices.size() / 6) + " vertices");
    
    return true;
}

bool ResourceLoader::createGPUBuffers(
    const MeshData& mesh,
    const std::shared_ptr<pers::ILogicalDevice>& device,
    const std::shared_ptr<pers::IQueue>& queue,
    std::shared_ptr<pers::IBuffer>& outVertexBuffer,
    std::shared_ptr<pers::IBuffer>& outIndexBuffer) {
    
    if (!device || !queue) {
        LOG_ERROR("ResourceLoader", "Invalid device or queue");
        return false;
    }
    
    // Get resource factory
    const auto& factory = device->getResourceFactory();
    if (!factory) {
        LOG_ERROR("ResourceLoader", "Failed to get resource factory");
        return false;
    }
    
    // Create vertex buffer using staging buffer
    size_t vertexDataSize = mesh.vertices.size() * sizeof(float);
    
    if (vertexDataSize > 0) {
        // Create staging buffer for vertices
        //pers::BufferDesc stagingDesc;
        //stagingDesc.size = vertexDataSize;
        //stagingDesc.usage = pers::BufferUsage::CopySrc | pers::BufferUsage::MapWrite;
        //stagingDesc.debugName = "BunnyVertexStagingBuffer";
        
        auto stagingBuffer = std::make_shared<pers::ImmediateStagingBuffer>();
        if (!stagingBuffer->create(vertexDataSize, device, "BunnyVertexStagingBuffer")) {
            LOG_ERROR("ResourceLoader", "Failed to create vertex staging buffer");
            return false;
        }
        
        // Write vertex data
        if (!stagingBuffer->writeBytes(mesh.vertices.data(), vertexDataSize, 0)) {
            LOG_ERROR("ResourceLoader", "Failed to write vertex data to staging buffer");
            return false;
        }
        
        stagingBuffer->finalize();
        
        // Create device buffer for vertices
        pers::BufferDesc deviceBufferDesc;
        deviceBufferDesc.size = vertexDataSize;
        deviceBufferDesc.usage = pers::BufferUsage::Vertex | pers::BufferUsage::CopyDst;
        deviceBufferDesc.debugName = "BunnyVertexBuffer";
        
        auto deviceBuffer = std::make_shared<pers::DeviceBuffer>();
        if (!deviceBuffer->create(vertexDataSize, pers::DeviceBufferUsage::Vertex, device, "BunnyVertexBuffer")) {
            LOG_ERROR("ResourceLoader", "Failed to create vertex device buffer");
            return false;
        }
        
        // Copy from staging to device
        auto commandEncoder = device->createCommandEncoder();
        if (!commandEncoder) {
            LOG_ERROR("ResourceLoader", "Failed to create command encoder for vertex upload");
            return false;
        }
        
        pers::BufferCopyDesc copyDesc;
        copyDesc.srcOffset = 0;
        copyDesc.dstOffset = 0;
        copyDesc.size = vertexDataSize;
        
        if (!commandEncoder->uploadToDeviceBuffer(stagingBuffer, deviceBuffer, copyDesc)) {
            LOG_ERROR("ResourceLoader", "Failed to copy vertex data to device buffer");
            return false;
        }
        
        auto commandBuffer = commandEncoder->finish();
        if (!commandBuffer) {
            LOG_ERROR("ResourceLoader", "Failed to finish vertex upload command");
            return false;
        }
        
        queue->submit(commandBuffer);
        outVertexBuffer = deviceBuffer;
    }
    
    // Create index buffer using staging buffer
    size_t indexDataSize = mesh.indices.size() * sizeof(uint32_t);
    
    if (indexDataSize > 0) {
        // Create staging buffer for indices
        pers::BufferDesc stagingDesc;
        stagingDesc.size = indexDataSize;
        stagingDesc.usage = pers::BufferUsage::CopySrc | pers::BufferUsage::MapWrite;
        stagingDesc.debugName = "BunnyIndexStagingBuffer";
        
        auto stagingBuffer = std::make_shared<pers::ImmediateStagingBuffer>();
        if (!stagingBuffer->create(indexDataSize, device, "BunnyIndexStagingBuffer")) {
            LOG_ERROR("ResourceLoader", "Failed to create index staging buffer");
            return false;
        }
        
        // Write index data
        if (!stagingBuffer->writeBytes(mesh.indices.data(), indexDataSize, 0)) {
            LOG_ERROR("ResourceLoader", "Failed to write index data to staging buffer");
            return false;
        }
        
        stagingBuffer->finalize();
        
        // Create device buffer for indices
        pers::BufferDesc deviceBufferDesc;
        deviceBufferDesc.size = indexDataSize;
        deviceBufferDesc.usage = pers::BufferUsage::Index | pers::BufferUsage::CopyDst;
        deviceBufferDesc.debugName = "BunnyIndexBuffer";
        
        auto deviceBuffer = std::make_shared<pers::DeviceBuffer>();
        if (!deviceBuffer->create(indexDataSize, pers::DeviceBufferUsage::Index, device, "BunnyIndexBuffer")) {
            LOG_ERROR("ResourceLoader", "Failed to create index device buffer");
            return false;
        }
        
        // Copy from staging to device
        auto commandEncoder = device->createCommandEncoder();
        if (!commandEncoder) {
            LOG_ERROR("ResourceLoader", "Failed to create command encoder for index upload");
            return false;
        }
        
        pers::BufferCopyDesc copyDesc;
        copyDesc.srcOffset = 0;
        copyDesc.dstOffset = 0;
        copyDesc.size = indexDataSize;
        
        if (!commandEncoder->uploadToDeviceBuffer(stagingBuffer, deviceBuffer, copyDesc)) {
            LOG_ERROR("ResourceLoader", "Failed to copy index data to device buffer");
            return false;
        }
        
        auto commandBuffer = commandEncoder->finish();
        if (!commandBuffer) {
            LOG_ERROR("ResourceLoader", "Failed to finish index upload command");
            return false;
        }
        
        queue->submit(commandBuffer);
        outIndexBuffer = deviceBuffer;
    }
    
    LOG_INFO("ResourceLoader", "Successfully created GPU buffers");
    return true;
}

void ResourceLoader::normalizeMesh(MeshData& mesh) {
    // Calculate center and scale
    glm::vec3 center = (mesh.minBounds + mesh.maxBounds) * 0.5f;
    glm::vec3 size = mesh.maxBounds - mesh.minBounds;
    float maxDim = (std::max)({size.x, size.y, size.z});
    
    if (maxDim <= 0.0f) {
        LOG_WARNING("ResourceLoader", "Invalid mesh bounds, skipping normalization");
        return;
    }
    
    float scale = 2.0f / maxDim;  // Scale to fit in [-1, 1] cube
    
    // Apply transformation to all vertices
    size_t floatsPerVertex = mesh.vertexStride / sizeof(float);
    size_t vertexCount = mesh.vertices.size() / floatsPerVertex;
    
    for (size_t i = 0; i < vertexCount; i++) {
        size_t posIndex = i * floatsPerVertex;
        
        // Transform position
        mesh.vertices[posIndex + 0] = (mesh.vertices[posIndex + 0] - center.x) * scale;
        mesh.vertices[posIndex + 1] = (mesh.vertices[posIndex + 1] - center.y) * scale;
        mesh.vertices[posIndex + 2] = (mesh.vertices[posIndex + 2] - center.z) * scale;
    }
    
    // Update bounds
    mesh.minBounds = (mesh.minBounds - center) * scale;
    mesh.maxBounds = (mesh.maxBounds - center) * scale;
    
    LOG_INFO("ResourceLoader", "Normalized mesh to fit in [-1, 1] cube");
}

bool ResourceLoader::downloadFile(const std::string& url, const std::string& outputPath) {
#ifdef _WIN32
    // Use Windows URLDownloadToFile
    HRESULT hr = URLDownloadToFileA(NULL, url.c_str(), outputPath.c_str(), 0, NULL);
    if (SUCCEEDED(hr)) {
        return true;
    }
    LOG_ERROR("ResourceLoader", "Failed to download file from: " + url);
#else
    // Use curl on Linux/Mac
    std::string command = "curl -L -o \"" + outputPath + "\" \"" + url + "\"";
    int result = system(command.c_str());
    if (result == 0) {
        return true;
    }
    LOG_ERROR("ResourceLoader", "Failed to download file using curl");
#endif
    return false;
}

bool ResourceLoader::fileExists(const std::string& path) {
    return std::filesystem::exists(path);
}