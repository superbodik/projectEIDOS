#include "Chunk.h"
#include <vector>
#include <cstring>
#include "WorldGenerator.h"
#include <cmath>
#include <fstream>
#include <filesystem> 

namespace fs = std::filesystem;

Chunk::Chunk(int x, int z) : chunkX(x), chunkZ(z) {
    std::memset(blocks, 0, sizeof(blocks));
}

Chunk::~Chunk() {
    if (hasModel) UnloadModel(model);
}

// === СОХРАНЕНИЕ И ЗАГРУЗКА ===

bool Chunk::SaveToFile(const std::string& worldPath) {
    if (!isModified) return true;
    std::string chunksDir = worldPath + "/chunks";
    if (!fs::exists(chunksDir)) fs::create_directories(chunksDir);
    std::string filename = chunksDir + "/c_" + std::to_string(chunkX) + "_" + std::to_string(chunkZ) + ".dat";
    std::ofstream out(filename, std::ios::binary);
    if (!out.is_open()) return false;
    out.write((char*)blocks, sizeof(blocks));
    out.close();
    return true;
}

bool Chunk::LoadFromFile(const std::string& worldPath) {
    std::string filename = worldPath + "/chunks/c_" + std::to_string(chunkX) + "_" + std::to_string(chunkZ) + ".dat";
    if (!fs::exists(filename)) return false;
    std::ifstream in(filename, std::ios::binary);
    if (!in.is_open()) return false;
    in.read((char*)blocks, sizeof(blocks));
    in.close();
    isModified = false;
    state = 1;
    return true;
}

// === ГЕНЕРАЦИЯ ЛАНДШАФТА ===

void Chunk::GenerateTerrain(WorldGenerator& gen) {
    int startX = chunkX * CHUNK_SIZE_X;
    int startZ = chunkZ * CHUNK_SIZE_Z;

    // 1. Основной рельеф
    for (int lx = 0; lx < CHUNK_SIZE_X; lx++) {
        for (int lz = 0; lz < CHUNK_SIZE_Z; lz++) {
            int worldX = startX + lx;
            int worldZ = startZ + lz;
            for (int ly = 0; ly < CHUNK_SIZE_Y; ly++) {
                blocks[lx][ly][lz] = gen.GetBlock(worldX, ly, worldZ);
            }
        }
    }

    // 2. Декорации (Деревья, Кактусы, Трава)
    // Сканируем шире границ чанка, чтобы деревья с соседних чанков могли зайти к нам
    for (int lx = -3; lx < CHUNK_SIZE_X + 3; lx++) {
        for (int lz = -3; lz < CHUNK_SIZE_Z + 3; lz++) {
            int worldX = startX + lx;
            int worldZ = startZ + lz;
            int groundY = gen.GetHeight(worldX, worldZ);

            if (groundY >= CHUNK_SIZE_Y - 10) continue;

            BlockType surface = gen.GetBlock(worldX, groundY, worldZ);
            if (surface == BlockType::Water) continue;

            float rng = std::abs(gen.Noise2D((float)worldX * 123.45f, (float)worldZ * 678.91f));
            float temp = gen.GetTemperature(worldX, worldZ);

            if (surface == BlockType::Grass) {
                // Деревья (редко)
                if (rng > 0.985f) {
                    std::vector<WorldGenerator::TreeBlock> tree;
                    if (temp < 0.45f) tree = gen.GetSpruceAt(worldX, groundY + 1, worldZ);
                    else tree = gen.GetTreeAt(worldX, groundY + 1, worldZ);

                    for (const auto& b : tree) {
                        int localX = b.x - startX;
                        int localZ = b.z - startZ;
                        if (IsValid(localX, b.y, localZ)) {
                            BlockType current = blocks[localX][b.y][localZ];
                            // Заменяем только воздух или траву
                            if (current == BlockType::Air || current == BlockType::TallGrass) {
                                blocks[localX][b.y][localZ] = b.type;
                            }
                        }
                    }
                }
                // Высокая трава (часто)
                else if (rng > 0.7f && IsValid(lx, groundY + 1, lz)) {
                    if (blocks[lx][groundY + 1][lz] == BlockType::Air) {
                        blocks[lx][groundY + 1][lz] = BlockType::TallGrass;
                    }
                }
            }
            else if (surface == BlockType::Sand) {
                // Кактусы
                if (rng > 0.99f && temp > 0.8f) {
                    auto cactus = gen.GetCactusAt(worldX, groundY + 1, worldZ);
                    for (const auto& b : cactus) {
                        int localX = b.x - startX;
                        int localZ = b.z - startZ;
                        if (IsValid(localX, b.y, localZ)) {
                            if (blocks[localX][b.y][localZ] == BlockType::Air) {
                                blocks[localX][b.y][localZ] = b.type;
                            }
                        }
                    }
                }
            }
        }
    }
    state = 1;
    isModified = false;
}

// === ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ===

bool Chunk::IsBlockOpaque(BlockType type) {
    if (type == BlockType::Air) return false;
    if (type == BlockType::Water || type == BlockType::WaterSource) return false;
    if (type == BlockType::TallGrass || type == BlockType::Cactus) return false;
    if (type == BlockType::OakLeaves || type == BlockType::SpruceLeaves) return false;
    if (type == BlockType::Ice) return false;
    return true;
}

bool Chunk::ShouldDrawFace(BlockType current, BlockType neighbor) {
    if (neighbor == BlockType::Air || neighbor == BlockType::TallGrass) return true;
    if (current == neighbor) return false; // Оптимизация: одинаковые блоки сливаются

    // ЛОГИКА ВОДЫ
    if (current == BlockType::Water || current == BlockType::WaterSource) {
        // Если сосед тоже вода - не рисуем грань (слияние)
        if (neighbor == BlockType::Water || neighbor == BlockType::WaterSource) return false;

        // Если сосед твердый блок - не рисуем (грань скрыта землей)
        if (IsBlockOpaque(neighbor)) return false;

        return true;
    }

    // ЛОГИКА ТВЕРДЫХ БЛОКОВ
    // Если сосед прозрачный (но не воздух) - рисуем грань
    if (!IsBlockOpaque(neighbor)) return true;

    return false;
}

// === ПОСТРОЕНИЕ МЕША ===

void Chunk::BuildMeshCPU(WorldGenerator& gen) {
    std::vector<float> tempVerts;
    std::vector<unsigned char> tempColors;

    // Резервируем память для ускорения
    tempVerts.reserve(24000);
    tempColors.reserve(32000);

    int worldStartX = chunkX * CHUNK_SIZE_X;
    int worldStartZ = chunkZ * CHUNK_SIZE_Z;

    for (int x = 0; x < CHUNK_SIZE_X; x++) {
        for (int y = 0; y < CHUNK_SIZE_Y; y++) {
            for (int z = 0; z < CHUNK_SIZE_Z; z++) {
                BlockType type = blocks[x][y][z];
                if (type == BlockType::Air) continue;

                // Определение цвета
                Color baseCol = MAGENTA;
                switch (type) {
                case BlockType::Grass: baseCol = Color{ 54, 150, 54, 255 }; break;
                case BlockType::Dirt:  baseCol = Color{ 121, 85, 58, 255 }; break;
                case BlockType::Water: baseCol = Color{ 20, 90, 200, 160 }; break; // Прозрачная вода
                case BlockType::Sand:  baseCol = Color{ 238, 214, 175, 255 }; break;
                case BlockType::RedSand: baseCol = Color{ 210, 120, 50, 255 }; break;
                case BlockType::Snow:  baseCol = WHITE; break;
                case BlockType::Ice:   baseCol = Color{ 150, 200, 255, 180 }; break;
                case BlockType::OakLog: baseCol = Color{ 101, 67, 33, 255 }; break;
                case BlockType::OakLeaves: baseCol = Color{ 50, 205, 50, 255 }; break;
                case BlockType::SpruceLog: baseCol = Color{ 60, 40, 20, 255 }; break;
                case BlockType::SpruceLeaves: baseCol = Color{ 30, 80, 50, 255 }; break;
                case BlockType::Granite: baseCol = Color{ 150, 110, 100, 255 }; break;
                case BlockType::Basalt: baseCol = Color{ 50, 50, 55, 255 }; break;
                case BlockType::Limestone: baseCol = Color{ 200, 200, 190, 255 }; break;
                case BlockType::Cactus: baseCol = Color{ 20, 180, 20, 255 }; break;
                case BlockType::TallGrass: baseCol = Color{ 100, 220, 0, 255 }; break;
                case BlockType::Bedrock: baseCol = BLACK; break;
                    // ... Добавь остальные свои блоки ...
                default: baseCol = GRAY; break;
                }

                float wx = (float)x; float wy = (float)y; float wz = (float)z;

                // Уровень воды (0.88 если сверху воздух)
                float topY = 1.0f;
                if (type == BlockType::Water) {
                    BlockType up = (y < CHUNK_SIZE_Y - 1) ? blocks[x][y + 1][z] : BlockType::Air;
                    if (up == BlockType::Air || up == BlockType::TallGrass) topY = 0.88f;
                }

                auto addVert = [&](float ox, float oy, float oz, float shadow) {
                    tempVerts.push_back(wx + ox); tempVerts.push_back(wy + oy); tempVerts.push_back(wz + oz);
                    unsigned char r = (unsigned char)(baseCol.r * shadow);
                    unsigned char g = (unsigned char)(baseCol.g * shadow);
                    unsigned char b = (unsigned char)(baseCol.b * shadow);
                    tempColors.push_back(r); tempColors.push_back(g); tempColors.push_back(b); tempColors.push_back(baseCol.a);
                    };

                BlockType neighbor;

                // --- ГИБРИДНАЯ ЛОГИКА ГРАНИЦ (Фикс дыр и сетки воды) ---

                // UP
                neighbor = (y < CHUNK_SIZE_Y - 1) ? blocks[x][y + 1][z] : BlockType::Air;
                if (ShouldDrawFace(type, neighbor)) {
                    addVert(0, topY, 0, 1.0f); addVert(0, topY, 1, 1.0f); addVert(1, topY, 1, 1.0f);
                    addVert(0, topY, 0, 1.0f); addVert(1, topY, 1, 1.0f); addVert(1, topY, 0, 1.0f);
                }

                // DOWN
                neighbor = (y > 0) ? blocks[x][y - 1][z] : BlockType::Air;
                if (ShouldDrawFace(type, neighbor)) {
                    addVert(0, 0, 0, 0.5f); addVert(1, 0, 1, 0.5f); addVert(0, 0, 1, 0.5f);
                    addVert(0, 0, 0, 0.5f); addVert(1, 0, 0, 0.5f); addVert(1, 0, 1, 0.5f);
                }

                // FRONT (+Z)
                if (z < CHUNK_SIZE_Z - 1) neighbor = blocks[x][y][z + 1];
                else {
                    if (type == BlockType::Water) neighbor = gen.GetBlock(worldStartX + x, y, worldStartZ + z + 1);
                    else neighbor = BlockType::Air; // Твердые блоки всегда рисуют стенку на границе -> НЕТ ДЫР
                }
                if (ShouldDrawFace(type, neighbor)) {
                    addVert(0, 0, 1, 0.8f); addVert(1, topY, 1, 0.8f); addVert(1, 0, 1, 0.8f);
                    addVert(0, 0, 1, 0.8f); addVert(0, topY, 1, 0.8f); addVert(1, topY, 1, 0.8f);
                }

                // BACK (-Z)
                if (z > 0) neighbor = blocks[x][y][z - 1];
                else {
                    if (type == BlockType::Water) neighbor = gen.GetBlock(worldStartX + x, y, worldStartZ + z - 1);
                    else neighbor = BlockType::Air;
                }
                if (ShouldDrawFace(type, neighbor)) {
                    addVert(0, 0, 0, 0.8f); addVert(1, 0, 0, 0.8f); addVert(1, topY, 0, 0.8f);
                    addVert(0, 0, 0, 0.8f); addVert(1, topY, 0, 0.8f); addVert(0, topY, 0, 0.8f);
                }

                // RIGHT (+X)
                if (x < CHUNK_SIZE_X - 1) neighbor = blocks[x + 1][y][z];
                else {
                    if (type == BlockType::Water) neighbor = gen.GetBlock(worldStartX + x + 1, y, worldStartZ + z);
                    else neighbor = BlockType::Air;
                }
                if (ShouldDrawFace(type, neighbor)) {
                    addVert(1, 0, 0, 0.6f); addVert(1, topY, 0, 0.6f); addVert(1, topY, 1, 0.6f);
                    addVert(1, 0, 0, 0.6f); addVert(1, topY, 1, 0.6f); addVert(1, 0, 1, 0.6f);
                }

                // LEFT (-X)
                if (x > 0) neighbor = blocks[x - 1][y][z];
                else {
                    if (type == BlockType::Water) neighbor = gen.GetBlock(worldStartX + x - 1, y, worldStartZ + z);
                    else neighbor = BlockType::Air;
                }
                if (ShouldDrawFace(type, neighbor)) {
                    addVert(0, 0, 0, 0.6f); addVert(0, topY, 1, 0.6f); addVert(0, topY, 0, 0.6f);
                    addVert(0, 0, 0, 0.6f); addVert(0, 0, 1, 0.6f); addVert(0, topY, 1, 0.6f);
                }
            }
        }
    }

    {
        std::lock_guard<std::mutex> lock(meshMutex);
        bufferVertices = std::move(tempVerts);
        bufferColors = std::move(tempColors);
    }
    state = 2;
    shouldRender = false;
}

void Chunk::UploadMeshGPU() {
    if (state != 2) return;
    std::vector<float> verts; std::vector<unsigned char> cols;
    {
        std::lock_guard<std::mutex> lock(meshMutex);
        if (bufferVertices.empty()) { state = 3; return; }
        verts = std::move(bufferVertices); cols = std::move(bufferColors);
    }
    Mesh mesh = { 0 };
    mesh.vertexCount = (int)(verts.size() / 3);
    mesh.triangleCount = (int)(mesh.vertexCount / 3);
    mesh.vertices = (float*)MemAlloc((unsigned int)(verts.size() * sizeof(float)));
    memcpy(mesh.vertices, verts.data(), verts.size() * sizeof(float));
    mesh.colors = (unsigned char*)MemAlloc((unsigned int)(cols.size() * sizeof(unsigned char)));
    memcpy(mesh.colors, cols.data(), cols.size() * sizeof(unsigned char));
    UploadMesh(&mesh, false);
    Model newModel = LoadModelFromMesh(mesh);
    if (hasModel) UnloadModel(model);
    model = newModel; hasModel = true;
    state = 3;
}

bool Chunk::IsValid(int x, int y, int z) const {
    return x >= 0 && x < CHUNK_SIZE_X && y >= 0 && y < CHUNK_SIZE_Y && z >= 0 && z < CHUNK_SIZE_Z;
}

BlockType Chunk::GetBlock(int x, int y, int z) const {
    if (!IsValid(x, y, z)) return BlockType::Air;
    return blocks[x][y][z];
}

void Chunk::SetBlockRaw(int bx, int by, int bz, int type) {
    if (IsValid(bx, by, bz)) { blocks[bx][by][bz] = (BlockType)type; isModified = true; }
}

void Chunk::SetBlock(int bx, int by, int bz, int type) {
    if (IsValid(bx, by, bz)) { blocks[bx][by][bz] = (BlockType)type; isModified = true; shouldRender = true; }
}

void Chunk::Draw() {
    if (hasModel) {
        Vector3 pos = { (float)chunkX * CHUNK_SIZE_X, 0.0f, (float)chunkZ * CHUNK_SIZE_Z };
        DrawModel(model, pos, 1.0f, WHITE);
    }
}