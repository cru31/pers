#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/version.h>
#include <iostream>
#include <sstream>

int main(int argc, char* argv[]) {
    std::cout << "=== Assimp Basic Test ===" << std::endl;
    
    // Get Assimp version
    std::cout << "Assimp version: " 
              << aiGetVersionMajor() << "."
              << aiGetVersionMinor() << "."
              << aiGetVersionPatch() << std::endl;
    
    std::cout << "Assimp revision: " << aiGetVersionRevision() << std::endl;
    std::cout << "Compile flags: " << aiGetCompileFlags() << std::endl;
    
    // Create an importer instance
    Assimp::Importer importer;
    std::cout << "Assimp Importer created successfully" << std::endl;
    
    // Check supported import formats
    std::string extensions;
    importer.GetExtensionList(extensions);
    std::cout << "Supported import formats: " << extensions << std::endl;
    
    // Test with a simple in-memory OBJ file
    const std::string simpleObjData = 
        "# Simple triangle OBJ\n"
        "v 0.0 0.0 0.0\n"
        "v 1.0 0.0 0.0\n"
        "v 0.5 1.0 0.0\n"
        "f 1 2 3\n";
    
    // Load from memory
    const aiScene* scene = importer.ReadFileFromMemory(
        simpleObjData.c_str(), 
        simpleObjData.length(),
        aiProcess_Triangulate | aiProcess_ValidateDataStructure,
        ".obj"
    );
    
    if (scene == nullptr) {
        std::cerr << "Failed to load test scene: " << importer.GetErrorString() << std::endl;
        return -1;
    }
    
    std::cout << "Test scene loaded successfully" << std::endl;
    
    // Print scene info
    std::cout << "Scene statistics:" << std::endl;
    std::cout << "  Meshes: " << scene->mNumMeshes << std::endl;
    std::cout << "  Materials: " << scene->mNumMaterials << std::endl;
    std::cout << "  Textures: " << scene->mNumTextures << std::endl;
    std::cout << "  Animations: " << scene->mNumAnimations << std::endl;
    std::cout << "  Cameras: " << scene->mNumCameras << std::endl;
    std::cout << "  Lights: " << scene->mNumLights << std::endl;
    
    // Check the mesh
    if (scene->mNumMeshes > 0) {
        const aiMesh* mesh = scene->mMeshes[0];
        std::cout << "First mesh statistics:" << std::endl;
        std::cout << "  Vertices: " << mesh->mNumVertices << std::endl;
        std::cout << "  Faces: " << mesh->mNumFaces << std::endl;
        std::cout << "  Has normals: " << (mesh->HasNormals() ? "yes" : "no") << std::endl;
        std::cout << "  Has tangents: " << (mesh->HasTangentsAndBitangents() ? "yes" : "no") << std::endl;
        std::cout << "  UV channels: " << mesh->GetNumUVChannels() << std::endl;
        std::cout << "  Color channels: " << mesh->GetNumColorChannels() << std::endl;
        
        // Verify triangle data
        if (mesh->mNumVertices == 3 && mesh->mNumFaces == 1) {
            std::cout << "Triangle mesh validated successfully" << std::endl;
        }
    }
    
    // Test post-processing flags info
    std::cout << "\nAvailable post-processing flags:" << std::endl;
    std::cout << "  aiProcess_Triangulate: " << aiProcess_Triangulate << std::endl;
    std::cout << "  aiProcess_GenNormals: " << aiProcess_GenNormals << std::endl;
    std::cout << "  aiProcess_CalcTangentSpace: " << aiProcess_CalcTangentSpace << std::endl;
    std::cout << "  aiProcess_OptimizeMeshes: " << aiProcess_OptimizeMeshes << std::endl;
    
    // Importer automatically cleans up the scene
    std::cout << "Scene cleanup handled by Importer" << std::endl;
    
    std::cout << "=== Assimp Test Passed ===" << std::endl;
    
    return 0;
}