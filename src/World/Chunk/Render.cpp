#include "../Chunk.h"
#include <raymath.h>
#include "rlgl.h"

void Chunk::UploadOne(Mesh& outMesh, Model& outModel, std::atomic<bool>& flag,
    std::vector<float>& vtx, std::vector<float>& tex, std::vector<float>& nrm,
    std::vector<unsigned char>& col, int count, Texture2D tx, Shader* sh) {
    (void)outMesh;

    if (count <= 0) {
        if (flag) { UnloadModel(outModel); outModel = { 0 }; flag = false; }
        return;
    }

    Mesh m = { 0 };
    m.vertexCount = count;
    m.triangleCount = count / 3;
    m.vertices = (float*)MemAlloc(count * 3 * sizeof(float));
    m.texcoords = (float*)MemAlloc(count * 2 * sizeof(float));
    m.normals = (float*)MemAlloc(count * 3 * sizeof(float));
    m.colors = (unsigned char*)MemAlloc(count * 4 * sizeof(unsigned char));
    memcpy(m.vertices, vtx.data(), vtx.size() * sizeof(float));
    memcpy(m.texcoords, tex.data(), tex.size() * sizeof(float));
    memcpy(m.normals, nrm.data(), nrm.size() * sizeof(float));
    memcpy(m.colors, col.data(), col.size() * sizeof(unsigned char));
    UploadMesh(&m, false);

    Model nm = LoadModelFromMesh(m);
    nm.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = tx;
    if (sh && sh->id > 0) nm.materials[0].shader = *sh;

    if (flag) UnloadModel(outModel);
    outModel = nm;
    flag = true;

    vtx.clear(); vtx.shrink_to_fit();
    tex.clear(); tex.shrink_to_fit();
    nrm.clear(); nrm.shrink_to_fit();
    col.clear(); col.shrink_to_fit();
}

void Chunk::UploadMeshGPU() {
    std::lock_guard<std::mutex> lock(chunkMutex);

    UploadOne(mesh, model, hasMesh, vertices, texcoords, normals, colors,
        vertexCount, atlasTexture, &fogShader);

    UploadOne(meshTransp, modelTransp, hasMeshTransp, verticesTransp, texcoordsTransp,
        normalsTransp, colorsTransp, vertexCountTransp, atlasTexture, &fogShader);

    UploadOne(meshWater, modelWater, hasMeshWater, verticesWater, texcoordsWater,
        normalsWater, colorsWater, vertexCountWater, waterAtlas, &waterShader);

    state = 3;
}

void Chunk::Draw() {
    if (hasMesh) {
        Vector3 pos = { (float)chunkX * 16, 0, (float)chunkZ * 16 };
        DrawModel(model, pos, 1.0f, WHITE);
    }
}

void Chunk::DrawTranslucent() {
    if (hasMeshTransp) {
        Vector3 pos = { (float)chunkX * 16, 0, (float)chunkZ * 16 };
        DrawModel(modelTransp, pos, 1.0f, WHITE);
    }
}

void Chunk::DrawWater() {
    if (hasMeshWater) {
        Vector3 pos = { (float)chunkX * 16, 0, (float)chunkZ * 16 };
        DrawModel(modelWater, pos, 1.0f, WHITE);
    }
}

