#pragma once
#include <vector>
#include <mutex>
#include <raylib.h>
#include "../World/BlockType.h"

class WorldGenerator; 

class Chunk {
public:
    static const int CHUNK_SIZE_X = 16;
    static const int CHUNK_SIZE_Y = 256;
    static const int CHUNK_SIZE_Z = 16;

    int chunkX, chunkZ;
    BlockType blocks[CHUNK_SIZE_X][CHUNK_SIZE_Y][CHUNK_SIZE_Z];

    int state = 0;
    bool isModified = false;
    bool shouldRender = true;
    bool isBusy = false;

    Model model;
    bool hasModel = false;

    Chunk(int x, int z);
    ~Chunk();

    void GenerateTerrain(WorldGenerator& gen);

    void BuildMeshCPU(WorldGenerator& gen);

    void UploadMeshGPU();
    void Draw();

    bool SaveToFile(const std::string& worldPath);
    bool LoadFromFile(const std::string& worldPath);

    BlockType GetBlock(int x, int y, int z) const;
    void SetBlock(int bx, int by, int bz, int type);
    void SetBlockRaw(int bx, int by, int bz, int type);

private:
    std::mutex meshMutex;
    std::vector<float> bufferVertices;
    std::vector<unsigned char> bufferColors;

    bool IsValid(int x, int y, int z) const;
    bool IsBlockOpaque(BlockType type);
    bool ShouldDrawFace(BlockType current, BlockType neighbor);
};