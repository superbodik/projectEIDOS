#include "../Chunk.h"
#include <raymath.h>
#include <algorithm>
#include <cstring>
#include "rlgl.h"

bool Chunk::IsSolid(int x, int y, int z, Chunk** neighbors) {
    BlockType type = GetBlockSafe(x, y, z, neighbors);
    return type != BlockType::Air && type != BlockType::Water && type != BlockType::Glass &&
        type != BlockType::OakSapling && type != BlockType::Fern && type != BlockType::Reed &&
        type != BlockType::Torch &&
        type != BlockType::BerryBush && type != BlockType::BerryBushRipe &&
        type != BlockType::Cattail && type != BlockType::LilyPad &&
        type != BlockType::CranberryBush && type != BlockType::Toadstool &&
        type != BlockType::Clover && type != BlockType::DuneGrass &&
        type != BlockType::Rose && type != BlockType::Dandelion && type != BlockType::TallGrass &&
        type != BlockType::DeadBush && type != BlockType::BrownMushroom && type != BlockType::RedMushroom &&
        type != BlockType::OakLeaves && type != BlockType::SpruceLeaves && type != BlockType::BirchLeaves &&
        type != BlockType::AcaciaLeaves && type != BlockType::JungleLeaves &&
        type != BlockType::StonePebble && type != BlockType::CopperPebble && type != BlockType::IronPebble &&
        type != BlockType::CoalPebble && type != BlockType::GoldPebble && type != BlockType::DiamondPebble &&
        type != BlockType::FlintPebble &&
        type != BlockType::GranitePebble &&
        type != BlockType::BasaltPebble &&
        type != BlockType::LimestonePebble &&
        type != BlockType::SandstonePebble &&
        type != BlockType::TinPebble &&
        type != BlockType::SilverPebble &&
        type != BlockType::ZincPebble &&
        type != BlockType::LeadPebble && type != BlockType::BaritePebble &&
        type != BlockType::FluoritePebble && type != BlockType::PhosphoritePebble &&
        type != BlockType::PotashPebble && type != BlockType::TungstenPebble &&
        type != BlockType::UraniumPebble && type != BlockType::Stick;
}


bool Chunk::BuildMeshCPU(WorldGenerator& gen, Chunk** neighbors) {
    (void)gen;
    bool lightChanged = CalculateBasicSunlight(neighbors);

    std::vector<float> tVerts, tVertsT, tVertsW;
    std::vector<float> tTex, tTexT, tTexW;
    std::vector<float> tNorms, tNormsT, tNormsW;
    std::vector<unsigned char> tCols, tColsT, tColsW;
    int tCount = 0, tCountT = 0, tCountW = 0;

    tVerts.reserve(20000);  tVertsT.reserve(3000);  tVertsW.reserve(6000);
    tTex.reserve(14000);    tTexT.reserve(2000);    tTexW.reserve(4000);
    tNorms.reserve(20000);  tNormsT.reserve(3000);  tNormsW.reserve(6000);
    tCols.reserve(28000);   tColsT.reserve(4000);   tColsW.reserve(8000);

    auto isTransp = [](BlockType t) {
        return t == BlockType::Air || t == BlockType::Water || t == BlockType::WaterSource ||
            t == BlockType::Glass || t == BlockType::OakSapling ||
            t == BlockType::Fern || t == BlockType::Reed || t == BlockType::Torch ||
            t == BlockType::BerryBush || t == BlockType::BerryBushRipe ||
            t == BlockType::Cattail || t == BlockType::LilyPad ||
            t == BlockType::CranberryBush || t == BlockType::Toadstool ||
            t == BlockType::Clover || t == BlockType::DuneGrass ||
            t == BlockType::Rose || t == BlockType::Dandelion || t == BlockType::TallGrass ||
            t == BlockType::DeadBush || t == BlockType::BrownMushroom || t == BlockType::RedMushroom ||
            t == BlockType::OakLeaves || t == BlockType::SpruceLeaves || t == BlockType::BirchLeaves ||
            t == BlockType::AcaciaLeaves || t == BlockType::JungleLeaves || t == BlockType::SugarCane ||
            t == BlockType::StonePebble || t == BlockType::CopperPebble || t == BlockType::IronPebble ||
            t == BlockType::CoalPebble || t == BlockType::GoldPebble || t == BlockType::DiamondPebble ||
            t == BlockType::FlintPebble ||
            t == BlockType::GranitePebble ||
            t == BlockType::BasaltPebble ||
            t == BlockType::LimestonePebble ||
            t == BlockType::SandstonePebble ||
            t == BlockType::TinPebble ||
            t == BlockType::SilverPebble ||
            t == BlockType::ZincPebble ||
            t == BlockType::LeadPebble || t == BlockType::BaritePebble ||
            t == BlockType::FluoritePebble || t == BlockType::PhosphoritePebble ||
            t == BlockType::PotashPebble || t == BlockType::TungstenPebble ||
            t == BlockType::UraniumPebble || t == BlockType::Stick;
        };

    auto isBlend = [](BlockType t) {
        return t == BlockType::Glass || t == BlockType::Ice || t == BlockType::PackedIce;
        };

    auto isPlant = [](BlockType t) {
        return t == BlockType::Rose || t == BlockType::Dandelion || t == BlockType::TallGrass ||
            t == BlockType::DeadBush || t == BlockType::BrownMushroom || t == BlockType::RedMushroom ||
            t == BlockType::OakSapling || t == BlockType::Fern || t == BlockType::Reed ||
            t == BlockType::BerryBush || t == BlockType::BerryBushRipe ||
            t == BlockType::Cattail || t == BlockType::LilyPad ||
            t == BlockType::CranberryBush || t == BlockType::Toadstool ||
            t == BlockType::Clover || t == BlockType::DuneGrass;
        };

    auto isPebble = [](BlockType t) {
        return t == BlockType::StonePebble || t == BlockType::CopperPebble || t == BlockType::IronPebble ||
            t == BlockType::CoalPebble || t == BlockType::GoldPebble || t == BlockType::DiamondPebble ||
            t == BlockType::FlintPebble ||
            t == BlockType::GranitePebble ||
            t == BlockType::BasaltPebble ||
            t == BlockType::LimestonePebble ||
            t == BlockType::SandstonePebble ||
            t == BlockType::TinPebble ||
            t == BlockType::SilverPebble ||
            t == BlockType::ZincPebble ||
            t == BlockType::LeadPebble || t == BlockType::BaritePebble ||
            t == BlockType::FluoritePebble || t == BlockType::PhosphoritePebble ||
            t == BlockType::PotashPebble || t == BlockType::TungstenPebble ||
            t == BlockType::UraniumPebble || t == BlockType::Stick;
        };

    auto isLeaf = [](BlockType t) {
        return t == BlockType::OakLeaves || t == BlockType::SpruceLeaves ||
            t == BlockType::BirchLeaves || t == BlockType::AcaciaLeaves ||
            t == BlockType::JungleLeaves;
        };

    auto shouldAddFace = [&](BlockType neighbor, BlockType current) {
        if (!isTransp(neighbor)) return false;
        if (neighbor == current) return false;
        return true;
        };

    for (int x = 0; x < CHUNK_SIZE_X; x++) {
        for (int y = 0; y <= maxY; y++) {
            for (int z = 0; z < CHUNK_SIZE_Z; z++) {
                int idx = x + CHUNK_SIZE_X * (z + CHUNK_SIZE_Z * y);
                BlockType type = blocks[idx];
                if (type == BlockType::Air) continue;

                if (isPlant(type)) {
                    AddPlantFaces(x, y, z, type, neighbors, tVerts, tTex, tNorms, tCols, tCount);
                    continue;
                }

                if (isPebble(type)) {
                    AddPebbleFaces(x, y, z, type, neighbors, tVerts, tTex, tNorms, tCols, tCount);
                    continue;
                }

                if (type == BlockType::Torch) {
                    AddTorchFaces(x, y, z, neighbors, tVerts, tTex, tNorms, tCols, tCount);
                    continue;
                }

                bool isWater = IsWaterBlock(type);
                bool useBlend = isBlend(type);

                BlockType nyP = GetBlockSafe(x, y + 1, z, neighbors);
                BlockType nyN = GetBlockSafe(x, y - 1, z, neighbors);
                BlockType nzP = GetBlockSafe(x, y, z + 1, neighbors);
                BlockType nzN = GetBlockSafe(x, y, z - 1, neighbors);
                BlockType nxP = GetBlockSafe(x + 1, y, z, neighbors);
                BlockType nxN = GetBlockSafe(x - 1, y, z, neighbors);

                std::vector<float>& vb = isWater ? tVertsW : (useBlend ? tVertsT : tVerts);
                std::vector<float>& tb = isWater ? tTexW : (useBlend ? tTexT : tTex);
                std::vector<float>& nb = isWater ? tNormsW : (useBlend ? tNormsT : tNorms);
                std::vector<unsigned char>& cb = isWater ? tColsW : (useBlend ? tColsT : tCols);
                int& cnt = isWater ? tCountW : (useBlend ? tCountT : tCount);

                if (isLeaf(type)) {
                    if (isTransp(nyP)) AddFace(x, y, z, 0, type, neighbors, vb, tb, nb, cb, cnt);
                    if (isTransp(nyN) && !isLeaf(nyN)) AddFace(x, y, z, 1, type, neighbors, vb, tb, nb, cb, cnt);
                    if (isTransp(nzP)) AddFace(x, y, z, 2, type, neighbors, vb, tb, nb, cb, cnt);
                    if (isTransp(nzN) && !isLeaf(nzN)) AddFace(x, y, z, 3, type, neighbors, vb, tb, nb, cb, cnt);
                    if (isTransp(nxP)) AddFace(x, y, z, 4, type, neighbors, vb, tb, nb, cb, cnt);
                    if (isTransp(nxN) && !isLeaf(nxN)) AddFace(x, y, z, 5, type, neighbors, vb, tb, nb, cb, cnt);
                    continue;
                }

                if (shouldAddFace(nyP, type)) AddFace(x, y, z, 0, type, neighbors, vb, tb, nb, cb, cnt);
                if (shouldAddFace(nyN, type)) AddFace(x, y, z, 1, type, neighbors, vb, tb, nb, cb, cnt);
                if (shouldAddFace(nzP, type)) AddFace(x, y, z, 2, type, neighbors, vb, tb, nb, cb, cnt);
                if (shouldAddFace(nzN, type)) AddFace(x, y, z, 3, type, neighbors, vb, tb, nb, cb, cnt);
                if (shouldAddFace(nxP, type)) AddFace(x, y, z, 4, type, neighbors, vb, tb, nb, cb, cnt);
                if (shouldAddFace(nxN, type)) AddFace(x, y, z, 5, type, neighbors, vb, tb, nb, cb, cnt);
            }
        }
    }

    std::lock_guard<std::mutex> lock(chunkMutex);
    std::swap(vertices, tVerts);
    std::swap(texcoords, tTex);
    std::swap(normals, tNorms);
    std::swap(colors, tCols);
    vertexCount = tCount;

    std::swap(verticesTransp, tVertsT);
    std::swap(texcoordsTransp, tTexT);
    std::swap(normalsTransp, tNormsT);
    std::swap(colorsTransp, tColsT);
    vertexCountTransp = tCountT;

    std::swap(verticesWater, tVertsW);
    std::swap(texcoordsWater, tTexW);
    std::swap(normalsWater, tNormsW);
    std::swap(colorsWater, tColsW);
    vertexCountWater = tCountW;

    state = 2;

    return lightChanged;
}

void Chunk::AddFace(int x, int y, int z, int faceDir, BlockType type, Chunk** neighbors,
    std::vector<float>& tVerts, std::vector<float>& tTex, std::vector<float>& tNorms,
    std::vector<unsigned char>& tCols, int& tCount) {
    float uMin, vMin, uMax, vMax;
    GetFaceUVRect(type, faceDir, uMin, vMin, uMax, vMax);

    int lx = 0, ly = 0, lz = 0;
    if (faceDir == 0) ly = 1; else if (faceDir == 1) ly = -1;
    else if (faceDir == 2) lz = 1; else if (faceDir == 3) lz = -1;
    else if (faceDir == 4) lx = 1; else if (faceDir == 5) lx = -1;

    auto getS = [&](int dx, int dy, int dz) { return IsSolid(x + dx, y + dy, z + dz, neighbors); };
    auto getP = [&](int dx, int dy, int dz) { return GetLightSafe(x + dx, y + dy, z + dz, neighbors); };

    auto calcLightAO = [&](int side1X, int side1Y, int side1Z,
        int side2X, int side2Y, int side2Z,
        int cornX, int cornY, int cornZ,
        float& skyOut, float& blkOut, int& aoOut) {
            bool s1 = getS(side1X, side1Y, side1Z);
            bool s2 = getS(side2X, side2Y, side2Z);
            bool c = getS(cornX, cornY, cornZ);

            if (s1 && s2) aoOut = 0;
            else aoOut = 3 - (s1 + s2 + c);

            unsigned char p0 = getP(lx, ly, lz);
            unsigned char p1 = getP(side1X, side1Y, side1Z);
            unsigned char p2 = getP(side2X, side2Y, side2Z);
            unsigned char p3 = getP(cornX, cornY, cornZ);

            if (s1) p1 = p0;
            if (s2) p2 = p0;
            if (c)  p3 = p0;

            skyOut = (float)(SkyLightOf(p0) + SkyLightOf(p1) + SkyLightOf(p2) + SkyLightOf(p3)) / 4.0f;
            blkOut = (float)(BlockLightOf(p0) + BlockLightOf(p1) + BlockLightOf(p2) + BlockLightOf(p3)) / 4.0f;
        };

    float vSky[4], vBlk[4]; int ao[4];

    if (faceDir == 0) {
        calcLightAO(-1, 1, 0, 0, 1, -1, -1, 1, -1, vSky[0], vBlk[0], ao[0]);
        calcLightAO(-1, 1, 0, 0, 1, 1, -1, 1, 1, vSky[1], vBlk[1], ao[1]);
        calcLightAO(1, 1, 0, 0, 1, 1, 1, 1, 1, vSky[2], vBlk[2], ao[2]);
        calcLightAO(1, 1, 0, 0, 1, -1, 1, 1, -1, vSky[3], vBlk[3], ao[3]);
    }
    else if (faceDir == 1) {
        calcLightAO(-1, -1, 0, 0, -1, 1, -1, -1, 1, vSky[0], vBlk[0], ao[0]);
        calcLightAO(-1, -1, 0, 0, -1, -1, -1, -1, -1, vSky[1], vBlk[1], ao[1]);
        calcLightAO(1, -1, 0, 0, -1, -1, 1, -1, -1, vSky[2], vBlk[2], ao[2]);
        calcLightAO(1, -1, 0, 0, -1, 1, 1, -1, 1, vSky[3], vBlk[3], ao[3]);
    }
    else if (faceDir == 2) {
        calcLightAO(-1, 0, 1, 0, 1, 1, -1, 1, 1, vSky[0], vBlk[0], ao[0]);
        calcLightAO(-1, 0, 1, 0, -1, 1, -1, -1, 1, vSky[1], vBlk[1], ao[1]);
        calcLightAO(1, 0, 1, 0, -1, 1, 1, -1, 1, vSky[2], vBlk[2], ao[2]);
        calcLightAO(1, 0, 1, 0, 1, 1, 1, 1, 1, vSky[3], vBlk[3], ao[3]);
    }
    else if (faceDir == 3) {
        calcLightAO(1, 0, -1, 0, 1, -1, 1, 1, -1, vSky[0], vBlk[0], ao[0]);
        calcLightAO(1, 0, -1, 0, -1, -1, 1, -1, -1, vSky[1], vBlk[1], ao[1]);
        calcLightAO(-1, 0, -1, 0, -1, -1, -1, -1, -1, vSky[2], vBlk[2], ao[2]);
        calcLightAO(-1, 0, -1, 0, 1, -1, -1, 1, -1, vSky[3], vBlk[3], ao[3]);
    }
    else if (faceDir == 4) {
        calcLightAO(1, 0, 1, 1, 1, 0, 1, 1, 1, vSky[0], vBlk[0], ao[0]);
        calcLightAO(1, 0, 1, 1, -1, 0, 1, -1, 1, vSky[1], vBlk[1], ao[1]);
        calcLightAO(1, 0, -1, 1, -1, 0, 1, -1, -1, vSky[2], vBlk[2], ao[2]);
        calcLightAO(1, 0, -1, 1, 1, 0, 1, 1, -1, vSky[3], vBlk[3], ao[3]);
    }
    else if (faceDir == 5) {
        calcLightAO(-1, 0, -1, -1, 1, 0, -1, 1, -1, vSky[0], vBlk[0], ao[0]);
        calcLightAO(-1, 0, -1, -1, -1, 0, -1, -1, -1, vSky[1], vBlk[1], ao[1]);
        calcLightAO(-1, 0, 1, -1, -1, 0, -1, -1, 1, vSky[2], vBlk[2], ao[2]);
        calcLightAO(-1, 0, 1, -1, 1, 0, -1, 1, 1, vSky[3], vBlk[3], ao[3]);
    }

    unsigned char cR[4], cG[4], cB[4];
    for (int i = 0; i < 4; i++) {
        float aoVal = 0.45f + (float)ao[i] * 0.183f;
        float sky = vSky[i] / 15.0f;
        float blk = vBlk[i] / 15.0f;
        bool seeThrough = (type == BlockType::Glass || type == BlockType::Ice ||
            type == BlockType::PackedIce);
        float lum = std::max(seeThrough ? 0.34f : 0.12f, std::max(sky, blk));
        float shadeVal = 1.0f;
        if (faceDir == 1) shadeVal = 0.85f;
        else if (faceDir == 2 || faceDir == 3) shadeVal = 0.94f;
        else if (faceDir == 4 || faceDir == 5) shadeVal = 0.90f;

        float warm = std::clamp(blk - sky, 0.0f, 1.0f);
        float base = 255.0f * aoVal * lum * shadeVal;
        cR[i] = (unsigned char)std::min(255.0f, base * (1.0f + 0.22f * warm));
        cG[i] = (unsigned char)std::min(255.0f, base * (1.0f + 0.04f * warm));
        cB[i] = (unsigned char)std::min(255.0f, base * (1.0f - 0.26f * warm));
    }

    unsigned char windAlpha = 0;
    if (type == BlockType::OakLeaves || type == BlockType::SpruceLeaves ||
        type == BlockType::BirchLeaves || type == BlockType::AcaciaLeaves ||
        type == BlockType::JungleLeaves) {
        windAlpha = 100;
    }

    float nx = 0, ny = 0, nz = 0;
    if      (faceDir == 0) { nx = 0; ny = 1;  nz = 0; }
    else if (faceDir == 1) { nx = 0; ny = -1; nz = 0; }
    else if (faceDir == 2) { nx = 0; ny = 0;  nz = 1; }
    else if (faceDir == 3) { nx = 0; ny = 0;  nz = -1; }
    else if (faceDir == 4) { nx = 1; ny = 0;  nz = 0; }
    else if (faceDir == 5) { nx = -1;ny = 0;  nz = 0; }

    float bx = (float)x; float by = (float)y; float bz = (float)z;
    float v[4][3];
    if (faceDir == 0) { v[0][0] = bx;v[0][1] = by + 1;v[0][2] = bz;v[1][0] = bx;v[1][1] = by + 1;v[1][2] = bz + 1;v[2][0] = bx + 1;v[2][1] = by + 1;v[2][2] = bz + 1;v[3][0] = bx + 1;v[3][1] = by + 1;v[3][2] = bz; }
    else if (faceDir == 1) {
        v[0][0] = bx;   v[0][1] = by; v[0][2] = bz;
        v[1][0] = bx + 1; v[1][1] = by; v[1][2] = bz;
        v[2][0] = bx + 1; v[2][1] = by; v[2][2] = bz + 1;
        v[3][0] = bx;   v[3][1] = by; v[3][2] = bz + 1;
    }
    else if (faceDir == 2) { v[0][0] = bx;v[0][1] = by + 1;v[0][2] = bz + 1;v[1][0] = bx;v[1][1] = by;v[1][2] = bz + 1;v[2][0] = bx + 1;v[2][1] = by;v[2][2] = bz + 1;v[3][0] = bx + 1;v[3][1] = by + 1;v[3][2] = bz + 1; }
    else if (faceDir == 3) { v[0][0] = bx + 1;v[0][1] = by + 1;v[0][2] = bz;v[1][0] = bx + 1;v[1][1] = by;v[1][2] = bz;v[2][0] = bx;v[2][1] = by;v[2][2] = bz;v[3][0] = bx;v[3][1] = by + 1;v[3][2] = bz; }
    else if (faceDir == 4) { v[0][0] = bx + 1;v[0][1] = by + 1;v[0][2] = bz + 1;v[1][0] = bx + 1;v[1][1] = by;v[1][2] = bz + 1;v[2][0] = bx + 1;v[2][1] = by;v[2][2] = bz;v[3][0] = bx + 1;v[3][1] = by + 1;v[3][2] = bz; }
    else if (faceDir == 5) { v[0][0] = bx;v[0][1] = by + 1;v[0][2] = bz;v[1][0] = bx;v[1][1] = by;v[1][2] = bz;v[2][0] = bx;v[2][1] = by;v[2][2] = bz + 1;v[3][0] = bx;v[3][1] = by + 1;v[3][2] = bz + 1; }

    int tri1[3] = { 0, 1, 2 };
    for (int i : tri1) {
        tVerts.push_back(v[i][0]); tVerts.push_back(v[i][1]); tVerts.push_back(v[i][2]);
        tNorms.push_back(nx); tNorms.push_back(ny); tNorms.push_back(nz);
        tCols.push_back(cR[i]); tCols.push_back(cG[i]); tCols.push_back(cB[i]); tCols.push_back(windAlpha);
    }
    tTex.push_back(uMin); tTex.push_back(vMin);
    tTex.push_back(uMin); tTex.push_back(vMax);
    tTex.push_back(uMax); tTex.push_back(vMax);

    int tri2[3] = { 0, 2, 3 };
    for (int i : tri2) {
        tVerts.push_back(v[i][0]); tVerts.push_back(v[i][1]); tVerts.push_back(v[i][2]);
        tNorms.push_back(nx); tNorms.push_back(ny); tNorms.push_back(nz);
        tCols.push_back(cR[i]); tCols.push_back(cG[i]); tCols.push_back(cB[i]); tCols.push_back(windAlpha);
    }
    tTex.push_back(uMin); tTex.push_back(vMin);
    tTex.push_back(uMax); tTex.push_back(vMax);
    tTex.push_back(uMax); tTex.push_back(vMin);

    tCount += 6;
}

void Chunk::SampleLight(int x, int y, int z, Chunk** neighbors, float& r, float& g, float& b) {
    unsigned char p = GetLightSafe(x, y, z, neighbors);
    float sky = (float)SkyLightOf(p) / 15.0f;
    float blk = (float)BlockLightOf(p) / 15.0f;
    float lum = std::max(0.12f, std::max(sky, blk));
    float warm = std::clamp(blk - sky, 0.0f, 1.0f);
    r = std::min(1.0f, lum * (1.0f + 0.22f * warm));
    g = std::min(1.0f, lum * (1.0f + 0.04f * warm));
    b = std::min(1.0f, lum * (1.0f - 0.26f * warm));
}

void Chunk::AddTorchFaces(int x, int y, int z, Chunk** neighbors,
    std::vector<float>& tVerts, std::vector<float>& tTex, std::vector<float>& tNorms,
    std::vector<unsigned char>& tCols, int& tCount) {

    float uMin, vMin, uMax, vMax;
    GetFaceUVRect(BlockType::Torch, 0, uMin, vMin, uMax, vMax);

    const float cx = (float)x + 0.5f, cz = (float)z + 0.5f;
    const float hw = 0.065f;
    const float top = (float)y + 0.62f;
    const float bot = (float)y;

    float lr, lg, lb;
    SampleLight(x, y, z, neighbors, lr, lg, lb);
    unsigned char c = (unsigned char)(255.0f * std::max(0.85f, lr));
    unsigned char cg = (unsigned char)(255.0f * std::max(0.78f, lg));
    unsigned char cb = (unsigned char)(255.0f * std::max(0.62f, lb));

    auto quad = [&](float ax, float az, float bx2, float bz2) {
        float px[4] = { ax, bx2, bx2, ax };
        float py[4] = { top, top, bot, bot };
        float pz[4] = { az, bz2, bz2, az };
        float uu[4] = { uMin, uMax, uMax, uMin };
        float vv[4] = { vMin, vMin, vMax, vMax };
        int tri[6] = { 0, 3, 2, 0, 2, 1 };
        for (int i : tri) {
            tVerts.push_back(px[i]); tVerts.push_back(py[i]); tVerts.push_back(pz[i]);
            tNorms.push_back(0); tNorms.push_back(1); tNorms.push_back(0);
            tCols.push_back(c); tCols.push_back(cg); tCols.push_back(cb); tCols.push_back(0);
            tTex.push_back(uu[i]); tTex.push_back(vv[i]);
        }
        tCount += 6;
        };

    quad(cx - hw, cz - hw, cx + hw, cz - hw);
    quad(cx + hw, cz - hw, cx - hw, cz - hw);
    quad(cx - hw, cz + hw, cx + hw, cz + hw);
    quad(cx + hw, cz + hw, cx - hw, cz + hw);
    quad(cx - hw, cz - hw, cx - hw, cz + hw);
    quad(cx - hw, cz + hw, cx - hw, cz - hw);
    quad(cx + hw, cz - hw, cx + hw, cz + hw);
    quad(cx + hw, cz + hw, cx + hw, cz - hw);

    float tu = uMin + (uMax - uMin) * 0.30f, tu2 = uMin + (uMax - uMin) * 0.70f;
    float tv = vMin + (vMax - vMin) * 0.02f, tv2 = vMin + (vMax - vMin) * 0.30f;
    float qx[4] = { cx - hw, cx + hw, cx + hw, cx - hw };
    float qz[4] = { cz - hw, cz - hw, cz + hw, cz + hw };
    float qu[4] = { tu, tu2, tu2, tu };
    float qv[4] = { tv, tv, tv2, tv2 };
    int tri[6] = { 0, 1, 2, 0, 2, 3 };
    for (int i : tri) {
        tVerts.push_back(qx[i]); tVerts.push_back(top); tVerts.push_back(qz[i]);
        tNorms.push_back(0); tNorms.push_back(1); tNorms.push_back(0);
        tCols.push_back(c); tCols.push_back(cg); tCols.push_back(cb); tCols.push_back(0);
        tTex.push_back(qu[i]); tTex.push_back(qv[i]);
    }
    tCount += 6;
}

void Chunk::AddPebbleFaces(int x, int y, int z, BlockType type, Chunk** neighbors,
    std::vector<float>& tVerts, std::vector<float>& tTex, std::vector<float>& tNorms,
    std::vector<unsigned char>& tCols, int& tCount) {

    float texU, texV;
    GetTextureUV(type, 0, texU, texV);
    const float step = 0.0625f, offset = 0.0015f;
    const float uMin = texU + offset, uMax = texU + step - offset;
    const float vMin = texV + offset, vMax = texV + step - offset;

    int wx = chunkX * CHUNK_SIZE_X + x;
    int wz = chunkZ * CHUNK_SIZE_Z + z;
    unsigned int h = (unsigned int)(wx * 374761393 + wz * 668265263 + y * 1274126177);
    h = (h ^ (h >> 13)) * 1274126177u;
    float r0 = (float)((h >> 4) & 0xFF) / 255.0f;
    float r1 = (float)((h >> 12) & 0xFF) / 255.0f;
    float r2 = (float)((h >> 20) & 0xFF) / 255.0f;

    const float cx = (float)x + 0.35f + r0 * 0.30f;
    const float cz = (float)z + 0.35f + r1 * 0.30f;
    const float by = (float)y + 0.001f;
    const float rad = 0.115f + r2 * 0.075f;
    const float hgt = 0.075f + r0 * 0.070f;
    const float rot = r1 * 6.2831853f;

    float lr, lg, lb;
    SampleLight(x, y, z, neighbors, lr, lg, lb);

    const int SIDES = 6;
    float px[SIDES], pz[SIDES], tpx[SIDES], tpz[SIDES];
    for (int i = 0; i < SIDES; i++) {
        float a = rot + 6.2831853f * (float)i / (float)SIDES;
        float wob = 0.78f + 0.34f * (float)(((h >> (i * 3)) & 7) / 7.0f);
        px[i] = cx + cosf(a) * rad * wob;
        pz[i] = cz + sinf(a) * rad * wob;
        tpx[i] = cx + cosf(a) * rad * wob * 0.55f;
        tpz[i] = cz + sinf(a) * rad * wob * 0.55f;
    }

    auto push = [&](float vx, float vy, float vz, float u, float v,
        float nx, float ny, float nz, float shade) {
            tVerts.push_back(vx); tVerts.push_back(vy); tVerts.push_back(vz);
            tNorms.push_back(nx); tNorms.push_back(ny); tNorms.push_back(nz);
            tCols.push_back((unsigned char)(255.0f * lr * shade));
            tCols.push_back((unsigned char)(255.0f * lg * shade));
            tCols.push_back((unsigned char)(255.0f * lb * shade));
            tCols.push_back(0);
            tTex.push_back(u); tTex.push_back(v);
        };

    const float uMid = (uMin + uMax) * 0.5f;
    const float vMid = (vMin + vMax) * 0.5f;

    for (int i = 0; i < SIDES; i++) {
        int j = (i + 1) % SIDES;
        float mx = (px[i] + px[j]) * 0.5f - cx;
        float mz = (pz[i] + pz[j]) * 0.5f - cz;
        float len = sqrtf(mx * mx + mz * mz);
        if (len < 0.0001f) len = 1.0f;
        float nx = mx / len, nz = mz / len;
        float shade = 0.72f + 0.16f * (nx * 0.6f + 0.4f);

        push(px[i], by, pz[i], uMin, vMax, nx, 0.25f, nz, shade);
        push(px[j], by, pz[j], uMax, vMax, nx, 0.25f, nz, shade);
        push(tpx[j], by + hgt, tpz[j], uMax, vMid, nx, 0.25f, nz, shade * 1.12f);

        push(px[i], by, pz[i], uMin, vMax, nx, 0.25f, nz, shade);
        push(tpx[j], by + hgt, tpz[j], uMax, vMid, nx, 0.25f, nz, shade * 1.12f);
        push(tpx[i], by + hgt, tpz[i], uMin, vMid, nx, 0.25f, nz, shade * 1.12f);
        tCount += 6;
    }

    for (int i = 0; i < SIDES; i++) {
        int j = (i + 1) % SIDES;
        push(cx, by + hgt * 1.06f, cz, uMid, vMid, 0, 1, 0, 1.0f);
        push(tpx[i], by + hgt, tpz[i], uMin, vMin, 0, 1, 0, 1.0f);
        push(tpx[j], by + hgt, tpz[j], uMax, vMin, 0, 1, 0, 1.0f);
        tCount += 3;
    }
}

void Chunk::AddPlantFaces(int x, int y, int z, BlockType type, Chunk** neighbors,
    std::vector<float>& tVerts, std::vector<float>& tTex, std::vector<float>& tNorms,
    std::vector<unsigned char>& tCols, int& tCount) {

    float texU, texV;
    GetTextureUV(type, 0, texU, texV);
    float step = 0.0625f; float offset = 0.001f;
    float uMin = texU + offset; float uMax = texU + step - offset;
    float vMin = texV + offset; float vMax = texV + step - offset;

    float bx = (float)x; float by = (float)y; float bz = (float)z;

    float lr, lg, lb;
    SampleLight(x, y, z, neighbors, lr, lg, lb);
    unsigned char c = (unsigned char)(255.0f * lr);
    unsigned char cg = (unsigned char)(255.0f * lg);
    unsigned char cb = (unsigned char)(255.0f * lb);

    float v1[4][3] = {
        {bx, by + 1, bz}, {bx, by, bz}, {bx + 1, by, bz + 1}, {bx + 1, by + 1, bz + 1}
    };

    float v2[4][3] = {
        {bx + 1, by + 1, bz}, {bx + 1, by, bz}, {bx, by, bz + 1}, {bx, by + 1, bz + 1}
    };

    auto pushQuad = [&](float v[4][3], bool reverse) {
        int tri1[3];
        int tri2[3];

        if (reverse) {
            tri1[0] = 0; tri1[1] = 2; tri1[2] = 1;
            tri2[0] = 0; tri2[1] = 3; tri2[2] = 2;
        }
        else {
            tri1[0] = 0; tri1[1] = 1; tri1[2] = 2;
            tri2[0] = 0; tri2[1] = 2; tri2[2] = 3;
        }

        for (int i : tri1) {
            tVerts.push_back(v[i][0]); tVerts.push_back(v[i][1]); tVerts.push_back(v[i][2]);
            tNorms.push_back(0); tNorms.push_back(1); tNorms.push_back(0);
            tCols.push_back(c); tCols.push_back(cg); tCols.push_back(cb); tCols.push_back(255);
        }
        if (!reverse) { tTex.push_back(uMin); tTex.push_back(vMin); tTex.push_back(uMin); tTex.push_back(vMax); tTex.push_back(uMax); tTex.push_back(vMax); }
        else { tTex.push_back(uMax); tTex.push_back(vMin); tTex.push_back(uMin); tTex.push_back(vMax); tTex.push_back(uMin); tTex.push_back(vMin); }

        for (int i : tri2) {
            tVerts.push_back(v[i][0]); tVerts.push_back(v[i][1]); tVerts.push_back(v[i][2]);
            tNorms.push_back(0); tNorms.push_back(1); tNorms.push_back(0);
            tCols.push_back(c); tCols.push_back(cg); tCols.push_back(cb); tCols.push_back(255);
        }
        if (!reverse) { tTex.push_back(uMin); tTex.push_back(vMin); tTex.push_back(uMax); tTex.push_back(vMax); tTex.push_back(uMax); tTex.push_back(vMin); }
        else { tTex.push_back(uMax); tTex.push_back(vMin); tTex.push_back(uMax); tTex.push_back(vMax); tTex.push_back(uMin); tTex.push_back(vMin); }

        tCount += 6;
        };

    pushQuad(v1, false);
    pushQuad(v1, true);
    pushQuad(v2, false);
    pushQuad(v2, true);
}

