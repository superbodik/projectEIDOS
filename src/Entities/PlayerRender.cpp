#include "../Core/EidosEngine.h"
#include "../World/Chunk.h"
#include "PlayerModel.h"
#include <raymath.h>
#include <rlgl.h>
#include <cmath>
#include <algorithm>

namespace {

    void HandBox(Vector3 centre, Vector3 size, Color c) {
        DrawCubeV(centre, size, c);
        DrawCubeWiresV(centre, size, Color{
            (unsigned char)(c.r * 0.6f), (unsigned char)(c.g * 0.6f),
            (unsigned char)(c.b * 0.6f), c.a });
    }

    void HandBlock(int blockId, Vector3 centre, float size, Matrix basis) {
        float step = Chunk::TileStep();
        float u = 0.0f, v = 0.0f;
        Chunk::GetTextureUV((BlockType)blockId, 2, u, v);
        if (u < 0.0f) return;

        float h = size * 0.5f;

        rlSetTexture(Chunk::atlasTexture.id);
        rlBegin(RL_QUADS);

        struct Face { Vector3 n; Vector3 a, b, c, d; float shade; };
        const Face faces[6] = {
            { {0,0,1},  {-h,-h, h}, { h,-h, h}, { h, h, h}, {-h, h, h}, 0.88f },
            { {0,0,-1}, { h,-h,-h}, {-h,-h,-h}, {-h, h,-h}, { h, h,-h}, 0.70f },
            { {1,0,0},  { h,-h, h}, { h,-h,-h}, { h, h,-h}, { h, h, h}, 0.82f },
            { {-1,0,0}, {-h,-h,-h}, {-h,-h, h}, {-h, h, h}, {-h, h,-h}, 0.66f },
            { {0,1,0},  {-h, h, h}, { h, h, h}, { h, h,-h}, {-h, h,-h}, 1.00f },
            { {0,-1,0}, {-h,-h,-h}, { h,-h,-h}, { h,-h, h}, {-h,-h, h}, 0.56f },
        };

        for (const Face& f : faces) {
            unsigned char s = (unsigned char)(255.0f * f.shade);
            rlColor4ub(s, s, s, 255);
            Vector3 a = Vector3Add(centre, Vector3Transform(f.a, basis));
            Vector3 b = Vector3Add(centre, Vector3Transform(f.b, basis));
            Vector3 c = Vector3Add(centre, Vector3Transform(f.c, basis));
            Vector3 d = Vector3Add(centre, Vector3Transform(f.d, basis));
            rlTexCoord2f(u, v + step);        rlVertex3f(a.x, a.y, a.z);
            rlTexCoord2f(u + step, v + step); rlVertex3f(b.x, b.y, b.z);
            rlTexCoord2f(u + step, v);        rlVertex3f(c.x, c.y, c.z);
            rlTexCoord2f(u, v);               rlVertex3f(d.x, d.y, d.z);
        }

        rlEnd();
        rlSetTexture(0);
    }

    bool IsCubeItem(int id) {
        if (id >= 100 && id <= 109) return true;
        if (id >= 5 && id <= 69) return true;
        if (id >= 120 && id <= 122) return true;
        if (id == 124) return true;
        return false;
    }

    void HandSprite(int blockId, Vector3 centre, float size, float thickness,
        Vector3 right, Vector3 up, Vector3 fwd) {
        float step = Chunk::TileStep();
        float u = 0.0f, v = 0.0f;
        Chunk::GetTextureUV((BlockType)blockId, 2, u, v);
        if (u < 0.0f) return;

        const float TILT = 22.0f * DEG2RAD;
        Vector3 planeU = Vector3Add(Vector3Scale(right, cosf(TILT)),
            Vector3Scale(fwd, sinf(TILT)));
        Vector3 normal = Vector3Add(Vector3Scale(right, -sinf(TILT)),
            Vector3Scale(fwd, cosf(TILT)));
        Vector3 planeV = up;

        float h = size * 0.5f;
        const int LAYERS = 9;

        rlSetTexture(Chunk::atlasTexture.id);
        rlBegin(RL_QUADS);

        for (int i = 0; i < LAYERS; i++) {
            float d = ((float)i / (float)(LAYERS - 1) - 0.5f) * thickness;
            float shade = 0.68f + 0.32f * (1.0f - fabsf((float)i / (LAYERS - 1) - 0.5f) * 2.0f);
            unsigned char s = (unsigned char)(255.0f * shade);
            rlColor4ub(s, s, s, 255);

            Vector3 o = Vector3Add(centre, Vector3Scale(normal, d));
            Vector3 a = Vector3Add(o, Vector3Add(Vector3Scale(planeU, -h), Vector3Scale(planeV, -h)));
            Vector3 b = Vector3Add(o, Vector3Add(Vector3Scale(planeU, h), Vector3Scale(planeV, -h)));
            Vector3 c = Vector3Add(o, Vector3Add(Vector3Scale(planeU, h), Vector3Scale(planeV, h)));
            Vector3 dd = Vector3Add(o, Vector3Add(Vector3Scale(planeU, -h), Vector3Scale(planeV, h)));

            rlTexCoord2f(u, v + step);        rlVertex3f(a.x, a.y, a.z);
            rlTexCoord2f(u + step, v + step); rlVertex3f(b.x, b.y, b.z);
            rlTexCoord2f(u + step, v);        rlVertex3f(c.x, c.y, c.z);
            rlTexCoord2f(u, v);               rlVertex3f(dd.x, dd.y, dd.z);
        }

        rlEnd();
        rlSetTexture(0);
    }

}

void EidosEngine::DrawFirstPersonHands(int heldBlockId) {
    const Camera3D& cam = player.camera;

    Vector3 fwd = Vector3Normalize(Vector3Subtract(cam.target, cam.position));
    Vector3 right = Vector3Normalize(Vector3CrossProduct(fwd, Vector3{ 0, 1, 0 }));
    Vector3 up = Vector3Normalize(Vector3CrossProduct(right, fwd));

    float bob = sinf(player.limbSwing) * player.swingAmount * 0.022f;
    float bobSide = cosf(player.limbSwing * 0.5f) * player.swingAmount * 0.016f;

    float punch = player.handSwing;
    float punchDrop = sinf(punch * 3.14159f) * 0.13f;
    float punchFwd = sinf(punch * 3.14159f) * 0.10f;

    Matrix basis = {
        right.x, up.x, -fwd.x, 0.0f,
        right.y, up.y, -fwd.y, 0.0f,
        right.z, up.z, -fwd.z, 0.0f,
        0.0f,    0.0f, 0.0f,   1.0f
    };

    auto place = [&](float rx, float uy, float fz) {
        Vector3 p = cam.position;
        p = Vector3Add(p, Vector3Scale(right, rx + bobSide));
        p = Vector3Add(p, Vector3Scale(up, uy + bob - punchDrop));
        p = Vector3Add(p, Vector3Scale(fwd, fz + punchFwd));
        return p;
        };

    rlDisableDepthTest();

    Color skin = player.appearance.skin;
    Color sleeve = player.appearance.HasEquip(EquipSlot::Chest)
        ? PlayerModel::EquipTint(player.appearance.equipment[(int)EquipSlot::Chest])
        : player.appearance.shirt;

    auto limb = [&](float rx, float uy, float fz, Vector3 size, Color c) {
        Vector3 p = place(rx, uy, fz);
        Vector3 half = Vector3Scale(size, 0.5f);
        Vector3 ex = Vector3Scale(right, half.x);
        Vector3 ey = Vector3Scale(up, half.y);
        Vector3 ez = Vector3Scale(fwd, half.z);
        Vector3 corners[8];
        for (int i = 0; i < 8; i++) {
            float sx = (i & 1) ? 1.0f : -1.0f;
            float sy = (i & 2) ? 1.0f : -1.0f;
            float sz = (i & 4) ? 1.0f : -1.0f;
            corners[i] = Vector3Add(p, Vector3Add(Vector3Add(
                Vector3Scale(ex, sx), Vector3Scale(ey, sy)), Vector3Scale(ez, sz)));
        }
        static const int QUADS[6][4] = {
            {0,1,3,2}, {4,6,7,5}, {0,2,6,4}, {1,5,7,3}, {2,3,7,6}, {0,4,5,1}
        };
        static const float SHADE[6] = { 0.70f, 0.86f, 0.62f, 0.92f, 1.0f, 0.55f };
        rlSetTexture(rlGetTextureIdDefault());
        rlBegin(RL_QUADS);
        for (int f = 0; f < 6; f++) {
            rlColor4ub((unsigned char)(c.r * SHADE[f]), (unsigned char)(c.g * SHADE[f]),
                (unsigned char)(c.b * SHADE[f]), 255);
            rlNormal3f(0.0f, 1.0f, 0.0f);
            for (int k = 0; k < 4; k++) {
                Vector3 v = corners[QUADS[f][k]];
                rlTexCoord2f(0.0f, 0.0f);
                rlVertex3f(v.x, v.y, v.z);
            }
        }
        rlEnd();
        rlSetTexture(0);
        };

    const float ARM = 0.052f;

    if (heldBlockId > 0) {
        if (IsCubeItem(heldBlockId)) {
            HandBlock(heldBlockId, place(0.140f, -0.135f, 0.400f), 0.088f, basis);
        }
        else {
            HandSprite(heldBlockId, place(0.132f, -0.118f, 0.395f),
                0.132f, 0.020f, right, up, fwd);
        }
        limb(0.150f, -0.190f, 0.320f, { ARM, ARM, 0.150f }, skin);
        limb(0.158f, -0.215f, 0.235f, { ARM * 1.15f, ARM * 1.15f, 0.115f }, sleeve);
    }
    else {
        limb(0.128f, -0.160f, 0.360f, { ARM * 1.12f, ARM * 1.12f, 0.070f }, skin);
        limb(0.140f, -0.196f, 0.268f, { ARM * 1.2f, ARM * 1.2f, 0.145f }, sleeve);
    }

    limb(-0.150f, -0.205f, 0.268f, { ARM * 1.2f, ARM * 1.2f, 0.145f }, sleeve);
    limb(-0.140f, -0.172f, 0.352f, { ARM * 1.12f, ARM * 1.12f, 0.062f }, skin);

    rlDrawRenderBatchActive();
    rlEnableDepthTest();
}

void EidosEngine::DrawPlayerNametag() {
    if (player.viewMode == Player::VIEW_FIRST) return;
    if (player.name.empty()) return;

    Vector3 head = { player.position.x, player.position.y + 2.28f, player.position.z };
    Vector2 scr = GetWorldToScreen(head, player.camera);

    if (scr.x < -400.0f || scr.y < -400.0f ||
        scr.x > GetScreenWidth() + 400.0f || scr.y > GetScreenHeight() + 400.0f) return;

    Vector3 toHead = Vector3Subtract(head, player.camera.position);
    Vector3 fwd = Vector3Normalize(Vector3Subtract(player.camera.target, player.camera.position));
    if (Vector3DotProduct(toHead, fwd) <= 0.0f) return;

    const char* text = player.name.c_str();
    int fontSize = 20;
    int w = MeasureText(text, fontSize);
    int px = (int)scr.x - w / 2;
    int py = (int)scr.y - fontSize - 4;

    DrawRectangle(px - 7, py - 4, w + 14, fontSize + 8, Color{ 12, 11, 9, 165 });
    DrawRectangleLines(px - 7, py - 4, w + 14, fontSize + 8, Color{ 217, 164, 65, 90 });
    DrawText(text, px, py, fontSize, Color{ 236, 231, 221, 255 });
}
