#include "PlayerModel.h"
#include "../World/Chunk.h"
#include "../World/BlockType.h"
#include <raymath.h>
#include <rlgl.h>
#include <cmath>

namespace {

    const float UNIT = 1.8f / 32.0f;

    Color Shade(Color c, float k) {
        auto ch = [&](unsigned char v) {
            float f = (float)v * k;
            return (unsigned char)(f < 0.0f ? 0.0f : (f > 255.0f ? 255.0f : f));
            };
        return { ch(c.r), ch(c.g), ch(c.b), c.a };
    }

    void Box(Vector3 centre, Vector3 size, Color c) {
        rlSetTexture(rlGetTextureIdDefault());
        DrawCubeV(centre, size, c);
        rlSetTexture(0);
        DrawCubeWiresV(centre, size, Shade(c, 0.62f));
    }

    void PushLimb(Vector3 pivot, float angleDeg, Vector3 axis) {
        rlPushMatrix();
        rlTranslatef(pivot.x, pivot.y, pivot.z);
        rlRotatef(angleDeg, axis.x, axis.y, axis.z);
        rlTranslatef(-pivot.x, -pivot.y, -pivot.z);
    }

    void DrawBlockCube(int blockId, Vector3 centre, float size, Texture2D atlas) {
        if (blockId <= 0) return;

        float step = Chunk::TileStep();
        float u = 0.0f, v = 0.0f;
        Chunk::GetTextureUV((BlockType)blockId, 2, u, v);
        if (u < 0.0f) return;

        float h = size * 0.5f;
        Vector3 p = centre;

        rlSetTexture(atlas.id);
        rlBegin(RL_QUADS);
        rlColor4ub(255, 255, 255, 255);

        struct Face { Vector3 n; Vector3 a, b, c, d; float shade; };
        const Face faces[6] = {
            { {0,0,1},  {-h,-h, h}, { h,-h, h}, { h, h, h}, {-h, h, h}, 0.86f },
            { {0,0,-1}, { h,-h,-h}, {-h,-h,-h}, {-h, h,-h}, { h, h,-h}, 0.72f },
            { {1,0,0},  { h,-h, h}, { h,-h,-h}, { h, h,-h}, { h, h, h}, 0.80f },
            { {-1,0,0}, {-h,-h,-h}, {-h,-h, h}, {-h, h, h}, {-h, h,-h}, 0.66f },
            { {0,1,0},  {-h, h, h}, { h, h, h}, { h, h,-h}, {-h, h,-h}, 1.00f },
            { {0,-1,0}, {-h,-h,-h}, { h,-h,-h}, { h,-h, h}, {-h,-h, h}, 0.55f },
        };

        for (const Face& f : faces) {
            unsigned char s = (unsigned char)(255.0f * f.shade);
            rlColor4ub(s, s, s, 255);
            rlNormal3f(f.n.x, f.n.y, f.n.z);
            rlTexCoord2f(u, v + step);            rlVertex3f(p.x + f.a.x, p.y + f.a.y, p.z + f.a.z);
            rlTexCoord2f(u + step, v + step);     rlVertex3f(p.x + f.b.x, p.y + f.b.y, p.z + f.b.z);
            rlTexCoord2f(u + step, v);            rlVertex3f(p.x + f.c.x, p.y + f.c.y, p.z + f.c.z);
            rlTexCoord2f(u, v);                   rlVertex3f(p.x + f.d.x, p.y + f.d.y, p.z + f.d.z);
        }

        rlEnd();
        rlSetTexture(0);
    }

    void DrawBody(const PlayerPose& pose, const PlayerAppearance& look,
        int heldBlockId, Texture2D atlas) {
        const float U = UNIT;

        Color skin = look.skin;
        Color shirt = look.HasEquip(EquipSlot::Chest)
            ? PlayerModel::EquipTint(look.equipment[(int)EquipSlot::Chest]) : look.shirt;
        Color trousers = look.HasEquip(EquipSlot::Legs)
            ? PlayerModel::EquipTint(look.equipment[(int)EquipSlot::Legs]) : look.trousers;
        Color boots = look.HasEquip(EquipSlot::Feet)
            ? PlayerModel::EquipTint(look.equipment[(int)EquipSlot::Feet]) : look.boots;

        float legY = 6.0f * U;
        float bodyY = 18.0f * U;
        float headY = 28.0f * U;

        float swing = sinf(pose.limbSwing) * pose.swingAmount * 42.0f;
        float swingOpp = -swing;

        if (pose.sneaking) {
            bodyY -= 2.0f * U;
            headY -= 2.5f * U;
        }

        Vector3 hipL = { -2.0f * U, 12.0f * U, 0.0f };
        PushLimb(hipL, swing, { 1, 0, 0 });
        Box({ -2.0f * U, legY, 0.0f }, { 3.6f * U, 12.0f * U, 3.6f * U }, trousers);
        Box({ -2.0f * U, 1.4f * U, 0.3f * U }, { 3.8f * U, 2.8f * U, 4.4f * U }, boots);
        rlPopMatrix();

        Vector3 hipR = { 2.0f * U, 12.0f * U, 0.0f };
        PushLimb(hipR, swingOpp, { 1, 0, 0 });
        Box({ 2.0f * U, legY, 0.0f }, { 3.6f * U, 12.0f * U, 3.6f * U }, trousers);
        Box({ 2.0f * U, 1.4f * U, 0.3f * U }, { 3.8f * U, 2.8f * U, 4.4f * U }, boots);
        rlPopMatrix();

        Box({ 0.0f, bodyY, 0.0f }, { 8.0f * U, 12.0f * U, 4.2f * U }, shirt);

        if (look.HasCosmetic(CosmeticSlot::Cape)) {
            Color cape = PlayerModel::EquipTint(look.cosmetics[(int)CosmeticSlot::Cape]);
            float sway = sinf(pose.limbSwing * 0.5f) * pose.swingAmount * 8.0f;
            PushLimb({ 0.0f, 24.0f * U, -2.4f * U }, 6.0f + sway, { 1, 0, 0 });
            Box({ 0.0f, 17.0f * U, -3.0f * U }, { 8.4f * U, 14.0f * U, 0.7f * U }, cape);
            rlPopMatrix();
        }

        Vector3 shL = { -5.6f * U, 23.0f * U, 0.0f };
        PushLimb(shL, swingOpp * 0.85f, { 1, 0, 0 });
        Box({ -5.6f * U, 18.0f * U, 0.0f }, { 3.4f * U, 12.0f * U, 3.6f * U }, shirt);
        Box({ -5.6f * U, 11.6f * U, 0.0f }, { 3.5f * U, 2.6f * U, 3.7f * U }, skin);
        rlPopMatrix();

        Vector3 shR = { 5.6f * U, 23.0f * U, 0.0f };
        float armR = swing * 0.85f;
        if (heldBlockId > 0) armR -= 32.0f;
        PushLimb(shR, armR, { 1, 0, 0 });
        Box({ 5.6f * U, 18.0f * U, 0.0f }, { 3.4f * U, 12.0f * U, 3.6f * U }, shirt);
        Box({ 5.6f * U, 11.6f * U, 0.0f }, { 3.5f * U, 2.6f * U, 3.7f * U }, skin);
        if (heldBlockId > 0)
            DrawBlockCube(heldBlockId, { 5.6f * U, 9.0f * U, 1.6f * U }, 5.2f * U, atlas);
        rlPopMatrix();

        PushLimb({ 0.0f, headY, 0.0f }, -pose.pitch * 0.55f, { 1, 0, 0 });
        Box({ 0.0f, headY, 0.0f }, { 6.4f * U, 6.4f * U, 6.4f * U }, skin);
        Box({ 0.0f, headY + 2.6f * U, 0.0f }, { 6.6f * U, 1.6f * U, 6.6f * U }, look.hair);

        float eyeZ = 3.3f * U;
        Box({ -1.4f * U, headY + 0.6f * U, eyeZ }, { 1.1f * U, 1.1f * U, 0.2f * U }, { 30, 30, 38, 255 });
        Box({ 1.4f * U, headY + 0.6f * U, eyeZ }, { 1.1f * U, 1.1f * U, 0.2f * U }, { 30, 30, 38, 255 });

        if (look.HasEquip(EquipSlot::Head)) {
            Color helm = PlayerModel::EquipTint(look.equipment[(int)EquipSlot::Head]);
            Box({ 0.0f, headY + 0.4f * U, 0.0f }, { 7.2f * U, 7.2f * U, 7.2f * U },
                Color{ helm.r, helm.g, helm.b, 210 });
        }
        if (look.HasCosmetic(CosmeticSlot::Hat)) {
            Color hat = PlayerModel::EquipTint(look.cosmetics[(int)CosmeticSlot::Hat]);
            Box({ 0.0f, headY + 4.2f * U, 0.0f }, { 8.2f * U, 1.2f * U, 8.2f * U }, hat);
            Box({ 0.0f, headY + 5.6f * U, 0.0f }, { 5.4f * U, 2.6f * U, 5.4f * U }, Shade(hat, 0.88f));
        }
        rlPopMatrix();

        if (look.HasCosmetic(CosmeticSlot::Trinket)) {
            Color tr = PlayerModel::EquipTint(look.cosmetics[(int)CosmeticSlot::Trinket]);
            Box({ 0.0f, 23.4f * U, 2.4f * U }, { 1.6f * U, 1.6f * U, 0.9f * U }, tr);
        }
    }

}

namespace {
    RenderTexture2D g_portrait = { 0 };
    bool g_portraitValid = false;
}

namespace PlayerModel {

    void BuildPortrait(const PlayerAppearance& look, int heldBlockId,
        Texture2D atlas, int width, int height, float spinDegrees) {
        if (width < 16 || height < 16) { g_portraitValid = false; return; }

        if (g_portrait.id == 0 ||
            g_portrait.texture.width != width || g_portrait.texture.height != height) {
            if (g_portrait.id != 0) UnloadRenderTexture(g_portrait);
            g_portrait = LoadRenderTexture(width, height);
            if (g_portrait.id == 0) { g_portraitValid = false; return; }
            SetTextureFilter(g_portrait.texture, TEXTURE_FILTER_POINT);
        }

        Camera3D cam = { 0 };
        cam.position = { 0.0f, 1.02f, 3.15f };
        cam.target = { 0.0f, 0.98f, 0.0f };
        cam.up = { 0.0f, 1.0f, 0.0f };
        cam.fovy = 40.0f;
        cam.projection = CAMERA_PERSPECTIVE;

        PlayerPose pose;
        pose.feet = { 0.0f, 0.0f, 0.0f };
        pose.yaw = spinDegrees;
        pose.pitch = 0.0f;
        pose.limbSwing = 0.0f;
        pose.swingAmount = 0.0f;

        BeginTextureMode(g_portrait);
        ClearBackground(Color{ 16, 15, 13, 255 });
        BeginMode3D(cam);
        DrawBody(pose, look, heldBlockId, atlas);
        EndMode3D();
        EndTextureMode();

        g_portraitValid = true;
    }

    Texture2D PortraitTexture() { return g_portrait.texture; }
    bool PortraitReady() { return g_portraitValid && g_portrait.id != 0; }

    void ReleasePortrait() {
        if (g_portrait.id != 0) UnloadRenderTexture(g_portrait);
        g_portrait = { 0 };
        g_portraitValid = false;
    }

    void DrawIsoBlock(int blockId, Texture2D atlas, float cx, float cy, float r) {
        if (blockId <= 0 || atlas.id == 0 || r < 1.0f) return;

        float step = Chunk::TileStep();
        float uTop = 0.0f, vTop = 0.0f, uSide = 0.0f, vSide = 0.0f;
        Chunk::GetTextureUV((BlockType)blockId, 0, uTop, vTop);
        Chunk::GetTextureUV((BlockType)blockId, 2, uSide, vSide);
        if (uTop < 0.0f || uSide < 0.0f) return;

        float halfW = r;
        float halfH = r * 0.5f;
        float bodyH = r * 1.05f;

        Vector2 t = { cx, cy - bodyH * 0.5f - halfH };
        Vector2 rr = { cx + halfW, cy - bodyH * 0.5f };
        Vector2 b = { cx, cy - bodyH * 0.5f + halfH };
        Vector2 l = { cx - halfW, cy - bodyH * 0.5f };

        Vector2 bl = { l.x, l.y + bodyH };
        Vector2 bb = { b.x, b.y + bodyH };
        Vector2 br = { rr.x, rr.y + bodyH };

        auto face = [&](Vector2 a, Vector2 b2, Vector2 c, Vector2 d,
            float tu, float tv, float shade) {
                rlSetTexture(atlas.id);
                rlBegin(RL_QUADS);
                unsigned char s = (unsigned char)(255.0f * shade);
                rlColor4ub(s, s, s, 255);
                rlTexCoord2f(tu, tv);               rlVertex2f(a.x, a.y);
                rlTexCoord2f(tu, tv + step);        rlVertex2f(b2.x, b2.y);
                rlTexCoord2f(tu + step, tv + step); rlVertex2f(c.x, c.y);
                rlTexCoord2f(tu + step, tv);        rlVertex2f(d.x, d.y);
                rlEnd();
                rlSetTexture(0);
            };

        face(l, b, rr, t, uTop, vTop, 1.0f);
        face(l, bl, bb, b, uSide, vSide, 0.72f);
        face(b, bb, br, rr, uSide, vSide, 0.55f);
    }

    Color EquipTint(int itemId) {
        switch ((BlockType)itemId) {
        case BlockType::NativeCopper:
        case BlockType::Malachite:      return { 184, 112, 62, 255 };
        case BlockType::NativeGold:     return { 214, 178, 74, 255 };
        case BlockType::NativeSilver:   return { 190, 194, 202, 255 };
        case BlockType::Hematite:
        case BlockType::Magnetite:      return { 128, 130, 138, 255 };
        case BlockType::Kimberlite:     return { 128, 206, 214, 255 };
        case BlockType::OakLeaves:
        case BlockType::JungleLeaves:   return { 78, 116, 54, 255 };
        case BlockType::OakPlanks:      return { 146, 108, 62, 255 };
        case BlockType::PlantFibre:     return { 168, 152, 96, 255 };
        case BlockType::Berries:        return { 172, 54, 66, 255 };
        case BlockType::Snow:           return { 226, 232, 238, 255 };
        case BlockType::Basalt:         return { 62, 62, 68, 255 };
        case BlockType::Marble:         return { 208, 204, 196, 255 };
        default:                         return { 150, 142, 130, 255 };
        }
    }

    const char* EquipSlotName(EquipSlot s) {
        switch (s) {
        case EquipSlot::Head:  return "Head";
        case EquipSlot::Chest: return "Chest";
        case EquipSlot::Legs:  return "Legs";
        case EquipSlot::Feet:  return "Feet";
        default:                return "";
        }
    }

    const char* CosmeticSlotName(CosmeticSlot s) {
        switch (s) {
        case CosmeticSlot::Hat:     return "Hat";
        case CosmeticSlot::Cape:    return "Cape";
        case CosmeticSlot::Trinket: return "Trinket";
        case CosmeticSlot::Aura:    return "Aura";
        default:                     return "";
        }
    }

    bool FitsEquipSlot(int itemId, EquipSlot s) {
        (void)s;
        return itemId > 0;
    }

    bool FitsCosmeticSlot(int itemId, CosmeticSlot s) {
        (void)s;
        return itemId > 0;
    }

    void Draw(const PlayerPose& pose, const PlayerAppearance& look,
        int heldBlockId, Texture2D atlas) {
        rlPushMatrix();
        rlTranslatef(pose.feet.x, pose.feet.y, pose.feet.z);
        rlRotatef(pose.yaw, 0.0f, 1.0f, 0.0f);
        DrawBody(pose, look, heldBlockId, atlas);
        rlPopMatrix();

        if (look.HasCosmetic(CosmeticSlot::Aura)) {
            Color aura = EquipTint(look.cosmetics[(int)CosmeticSlot::Aura]);
            float t = (float)GetTime();
            BeginBlendMode(BLEND_ADDITIVE);
            for (int i = 0; i < 7; i++) {
                float a = t * 1.3f + (float)i * (6.2831853f / 7.0f);
                float rr = 0.55f + sinf(t * 2.0f + (float)i) * 0.08f;
                Vector3 p = {
                    pose.feet.x + cosf(a) * rr,
                    pose.feet.y + 0.9f + sinf(t * 1.7f + (float)i * 0.9f) * 0.35f,
                    pose.feet.z + sinf(a) * rr
                };
                DrawCubeV(p, { 0.07f, 0.07f, 0.07f }, Color{ aura.r, aura.g, aura.b, 190 });
            }
            EndBlendMode();
        }
    }

    void DrawPortrait(const PlayerAppearance& look, int heldBlockId,
        Texture2D atlas, Rectangle box, float spinDegrees) {
        (void)atlas;
        if (box.width < 8.0f || box.height < 8.0f) return;

        DrawRectangleRec(box, Color{ 16, 15, 13, 255 });
        BeginScissorMode((int)box.x, (int)box.y, (int)box.width, (int)box.height);

        const float FIGURE_TOP = -5.0f;
        const float FIGURE_BOTTOM = 33.4f;
        const float FIGURE_SPAN = FIGURE_BOTTOM - FIGURE_TOP;

        float u = std::min(box.height / (FIGURE_SPAN + 3.0f), box.width / 17.0f);
        float cx = box.x + box.width * 0.5f;
        float top = box.y + (box.height - FIGURE_SPAN * u) * 0.5f - FIGURE_TOP * u;

        float turn = sinf(spinDegrees * DEG2RAD);
        float lean = turn * u * 0.9f;

        auto part = [&](float ox, float oy, float w, float h, Color c, float shade) {
            Rectangle r = { cx + ox * u, top + oy * u, w * u, h * u };
            DrawRectangleRec(r, Shade(c, shade));
            DrawRectangleLinesEx(r, 1.0f, Shade(c, shade * 0.62f));
            return r;
            };

        Color skin = look.skin;
        Color shirt = look.HasEquip(EquipSlot::Chest)
            ? EquipTint(look.equipment[(int)EquipSlot::Chest]) : look.shirt;
        Color trousers = look.HasEquip(EquipSlot::Legs)
            ? EquipTint(look.equipment[(int)EquipSlot::Legs]) : look.trousers;
        Color boots = look.HasEquip(EquipSlot::Feet)
            ? EquipTint(look.equipment[(int)EquipSlot::Feet]) : look.boots;

        if (look.HasCosmetic(CosmeticSlot::Cape)) {
            Color cape = EquipTint(look.cosmetics[(int)CosmeticSlot::Cape]);
            part(-5.9f + lean * 0.5f, 7.8f, 11.8f, 16.0f, cape, 0.68f);
            part(-5.9f + lean * 0.5f, 23.0f, 11.8f, 1.2f, cape, 0.5f);
        }

        part(-7.7f + lean, 8.4f, 3.6f, 11.4f, shirt, 0.80f);
        part(4.1f + lean, 8.4f, 3.6f, 11.4f, shirt, 0.92f);
        part(-7.7f + lean, 19.4f, 3.6f, 2.8f, skin, 0.84f);
        part(4.1f + lean, 19.4f, 3.6f, 2.8f, skin, 0.96f);

        part(-4.0f + lean * 0.6f, 8.4f, 8.0f, 12.0f, shirt, 1.0f);

        part(-4.0f + lean * 0.4f, 20.6f, 3.8f, 10.0f, trousers, 0.88f);
        part(0.2f + lean * 0.4f, 20.6f, 3.8f, 10.0f, trousers, 1.0f);
        part(-4.1f + lean * 0.4f, 30.2f, 4.0f, 2.8f, boots, 0.9f);
        part(0.1f + lean * 0.4f, 30.2f, 4.0f, 2.8f, boots, 1.0f);

        Rectangle head = part(-3.2f + lean, 0.0f, 6.4f, 6.4f, skin, 1.0f);
        part(-3.3f + lean, -0.4f, 6.6f, 1.8f, look.hair, 1.0f);

        float eyeY = 2.4f;
        part(-2.0f + lean, eyeY, 1.2f, 1.2f, Color{ 32, 30, 38, 255 }, 1.0f);
        part(0.8f + lean, eyeY, 1.2f, 1.2f, Color{ 32, 30, 38, 255 }, 1.0f);

        if (look.HasEquip(EquipSlot::Head)) {
            Color helm = EquipTint(look.equipment[(int)EquipSlot::Head]);
            Rectangle hr = { head.x - u * 0.5f, head.y - u * 0.5f,
                             head.width + u, head.height + u };
            DrawRectangleLinesEx(hr, std::max(2.0f, u * 0.7f), helm);
        }

        if (look.HasCosmetic(CosmeticSlot::Hat)) {
            Color hat = EquipTint(look.cosmetics[(int)CosmeticSlot::Hat]);
            part(-4.6f + lean, -2.2f, 9.2f, 1.4f, hat, 1.0f);
            part(-2.6f + lean, -4.4f, 5.2f, 2.4f, hat, 0.86f);
        }

        if (look.HasCosmetic(CosmeticSlot::Trinket)) {
            Color tr = EquipTint(look.cosmetics[(int)CosmeticSlot::Trinket]);
            part(-0.9f + lean * 0.6f, 11.0f, 1.8f, 1.8f, tr, 1.0f);
        }

        if (heldBlockId > 0 && atlas.id != 0)
            DrawIsoBlock(heldBlockId, atlas,
                cx + (4.4f + lean) * u, top + 23.4f * u, u * 3.1f);

        if (look.HasCosmetic(CosmeticSlot::Aura)) {
            Color aura = EquipTint(look.cosmetics[(int)CosmeticSlot::Aura]);
            float t = (float)GetTime();
            for (int i = 0; i < 8; i++) {
                float a = t * 1.4f + (float)i * (6.2831853f / 8.0f);
                float rr = box.width * 0.32f + sinf(t * 2.1f + (float)i) * u * 0.8f;
                DrawRectangle((int)(cx + cosf(a) * rr),
                    (int)(top + 16.0f * u + sinf(a) * rr * 0.55f),
                    (int)std::max(2.0f, u * 0.6f), (int)std::max(2.0f, u * 0.6f),
                    Color{ aura.r, aura.g, aura.b, 220 });
            }
        }

        EndScissorMode();
    }

}
