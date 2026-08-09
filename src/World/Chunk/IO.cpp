#include "../Chunk.h"
#include <fstream>
#include <iostream>
#include <cstring>

bool Chunk::SaveToFile(const std::string& worldPath) {
    if (!isModified) return true;

    std::string file = worldPath + "/chunk_" + std::to_string(chunkX) + "_" + std::to_string(chunkZ) + ".bin";
    std::ofstream out(file, std::ios::binary);
    if (!out) return false;

    out.write((char*)blocks, CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z * sizeof(BlockType));
    if (out.fail() || out.bad()) {
        out.close();
        return false;
    }
    out.close();
    if (out.fail()) return false;

    isModified = false;
    return true;
}

bool Chunk::LoadFromFile(const std::string& worldPath) {
    std::string file = worldPath + "/chunk_" + std::to_string(chunkX) + "_" + std::to_string(chunkZ) + ".bin";
    std::ifstream in(file, std::ios::binary);
    if (!in) return false;

    in.read((char*)blocks, CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z * sizeof(BlockType));
    if (in.fail() || in.bad()) {
        in.close();
        return false;
    }

    if (in.gcount() != (std::streamsize)(CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z * sizeof(BlockType))) {
        in.close();
        return false;
    }
    in.close();

    maxY = 0;
    for (int y = 0; y < CHUNK_SIZE_Y; y++) {
        for (int z = 0; z < CHUNK_SIZE_Z; z++) {
            for (int x = 0; x < CHUNK_SIZE_X; x++) {
                if (blocks[x + CHUNK_SIZE_X * (z + CHUNK_SIZE_Z * y)] != BlockType::Air) {
                    if (y > maxY) maxY = y;
                }
            }
        }
    }
    maxY += 30;
    if (maxY >= CHUNK_SIZE_Y) maxY = CHUNK_SIZE_Y - 1;

    state = 1;
    isModified = false;
    return true;
}

