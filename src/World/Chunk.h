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

    static Texture2D atlasTexture;
    static void LoadAtlas();

    int chunkX, chunkZ;
    int maxY = 0;

    std::atomic<int> state = 0;
    std::atomic<bool> isBusy{ false };
    std::atomic<bool> dirty{ false };
    std::atomic<bool> hasMesh{ false };
    std::atomic<bool> shouldRender{ false };
    std::atomic<bool> isModified{ false };

    Chunk(int x, int z);
    ~Chunk();

    void GenerateTerrain(WorldGenerator& gen);
    bool BuildMeshCPU(WorldGenerator& gen, Chunk** neighbors);

    void UploadMeshGPU();
    void Draw();

    BlockType GetBlock(int x, int y, int z);
    void SetBlock(int x, int y, int z, int type);
    void SetBlockRaw(int x, int y, int z, int type);

    unsigned char GetLight(int x, int y, int z);
    void SetLight(int x, int y, int z, unsigned char level);
    bool CalculateBasicSunlight(Chunk** neighbors);

    bool SaveToFile(const std::string& worldPath);
    bool LoadFromFile(const std::string& worldPath);

private:
    BlockType* blocks;
    unsigned char* lightMap;

    Mesh mesh;
    Model model;
    std::mutex chunkMutex;

    std::vector<float> vertices;
    std::vector<float> texcoords;
    std::vector<unsigned char> colors;
    int vertexCount = 0;

    BlockType GetBlockSafe(int lx, int ly, int lz, Chunk** neighbors);
    unsigned char GetLightSafe(int lx, int ly, int lz, Chunk** neighbors);

    void AddFace(int x, int y, int z, int faceDir, BlockType type, Chunk** neighbors,
        std::vector<float>& tVerts, std::vector<float>& tTex, std::vector<unsigned char>& tCols, int& tCount);

    bool IsSolid(int x, int y, int z, Chunk** neighbors);
    void GetTextureUV(BlockType type, int faceDir, float& u, float& v);
};