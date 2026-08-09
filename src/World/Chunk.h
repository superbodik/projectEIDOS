#pragma once
#include <raylib.h>
#include <vector>
#include <string>
#include <atomic>
#include <mutex>
#include "WorldGenerator.h"

class Chunk {
public:
    static const int CHUNK_SIZE_X = 16;
    static const int CHUNK_SIZE_Y = 256;
    static const int CHUNK_SIZE_Z = 16;

    static const int WATER_FRAMES = 32;
    static const int WATER_TILE = 16;

    static Texture2D atlasTexture;
    static Texture2D waterAtlas;
    static Shader fogShader;
    static Shader waterShader;
    static Image BuildAtlasImage();
    static void LoadAtlas();
    static void LoadWaterAtlas();

    static bool IsWaterBlock(BlockType t) {
        return t == BlockType::Water || t == BlockType::WaterSource;
    }

    int chunkX, chunkZ;
    int maxY = 0;

    std::atomic<int> state = 0;
    std::atomic<bool> isBusy{ false };
    std::atomic<bool> dirty{ false };
    std::atomic<bool> hasMesh{ false };
    std::atomic<bool> hasMeshTransp{ false };
    std::atomic<bool> hasMeshWater{ false };
    std::atomic<bool> shouldRender{ false };
    std::atomic<bool> isModified{ false };
    std::atomic<int> lightPasses{ 0 };

    Chunk(int x, int z);
    ~Chunk();

    void GenerateTerrain(WorldGenerator& gen);
    bool BuildMeshCPU(WorldGenerator& gen, Chunk** neighbors);

    void UploadMeshGPU();
    void Draw();
    void DrawTranslucent();
    void DrawWater();
    Model& GetModel() { return model; }

    BlockType GetBlock(int x, int y, int z);
    void SetBlock(int x, int y, int z, int type);
    void SetBlockRaw(int x, int y, int z, int type);

    unsigned char GetLight(int x, int y, int z);
    void SetLight(int x, int y, int z, unsigned char level);
    bool CalculateBasicSunlight(Chunk** neighbors);

    static void GetTextureUV(BlockType type, int faceDir, float& u, float& v);
    static float TileStep() { return 1.0f / 16.0f; }
    static int  GetLightEmission(BlockType t);
    static bool IsLightPassable(BlockType t);
    static unsigned char SkyLightOf(unsigned char packed) { return (unsigned char)(packed >> 4); }
    static unsigned char BlockLightOf(unsigned char packed) { return (unsigned char)(packed & 0x0F); }

    bool SaveToFile(const std::string& worldPath);
    bool LoadFromFile(const std::string& worldPath);

private:
    BlockType* blocks;
    unsigned char* lightMap;

    Mesh mesh;
    Model model;
    Mesh meshTransp;
    Model modelTransp;
    Mesh meshWater;
    Model modelWater;
    std::mutex chunkMutex;

    std::vector<float> vertices;
    std::vector<float> texcoords;
    std::vector<float> normals;
    std::vector<unsigned char> colors;
    int vertexCount = 0;

    std::vector<float> verticesTransp;
    std::vector<float> texcoordsTransp;
    std::vector<float> normalsTransp;
    std::vector<unsigned char> colorsTransp;
    int vertexCountTransp = 0;

    std::vector<float> verticesWater;
    std::vector<float> texcoordsWater;
    std::vector<float> normalsWater;
    std::vector<unsigned char> colorsWater;
    int vertexCountWater = 0;

    BlockType GetBlockSafe(int lx, int ly, int lz, Chunk** neighbors);
    unsigned char GetLightSafe(int lx, int ly, int lz, Chunk** neighbors);

    void AddFace(int x, int y, int z, int faceDir, BlockType type, Chunk** neighbors,
        std::vector<float>& tVerts, std::vector<float>& tTex, std::vector<float>& tNorms,
        std::vector<unsigned char>& tCols, int& tCount);

    void AddPlantFaces(int x, int y, int z, BlockType type, Chunk** neighbors,
        std::vector<float>& tVerts, std::vector<float>& tTex, std::vector<float>& tNorms,
        std::vector<unsigned char>& tCols, int& tCount);

    void AddPebbleFaces(int x, int y, int z, BlockType type, Chunk** neighbors,
        std::vector<float>& tVerts, std::vector<float>& tTex, std::vector<float>& tNorms,
        std::vector<unsigned char>& tCols, int& tCount);

    void AddStickFaces(int x, int y, int z, Chunk** neighbors,
        std::vector<float>& tVerts, std::vector<float>& tTex, std::vector<float>& tNorms,
        std::vector<unsigned char>& tCols, int& tCount);

    void AddTorchFaces(int x, int y, int z, Chunk** neighbors,
        std::vector<float>& tVerts, std::vector<float>& tTex, std::vector<float>& tNorms,
        std::vector<unsigned char>& tCols, int& tCount);

    void SampleLight(int x, int y, int z, Chunk** neighbors, float& r, float& g, float& b);

    bool IsSolid(int x, int y, int z, Chunk** neighbors);
    void GetFaceUVRect(BlockType type, int faceDir, float& uMin, float& vMin, float& uMax, float& vMax);
    void UploadOne(Mesh& outMesh, Model& outModel, std::atomic<bool>& flag,
        std::vector<float>& vtx, std::vector<float>& tex, std::vector<float>& nrm,
        std::vector<unsigned char>& col, int count, Texture2D tx, Shader* sh);
};
