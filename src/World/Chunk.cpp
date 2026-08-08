#include "Chunk.h"
#include <vector>
#include <fstream>
#include <iostream>
#include <raymath.h>
#include <algorithm>
#include <queue>
#include <cstring>
#include "rlgl.h"

Texture2D Chunk::atlasTexture = { 0 };
Texture2D Chunk::waterAtlas = { 0 };
Shader Chunk::fogShader = { 0 };
Shader Chunk::waterShader = { 0 };

void Chunk::LoadWaterAtlas() {
    if (waterAtlas.id != 0) return;

    const int T = WATER_TILE;
    const int F = WATER_FRAMES;
    Image img = GenImageColor(T, T * F, { 0, 0, 0, 0 });

    struct Ripple { float fx, fy, ft, amp, phase; };
    const Ripple waves[] = {
        { 1.0f,  0.0f,  1.0f, 1.00f, 0.0f },
        { 0.0f,  1.0f, -1.0f, 0.85f, 1.7f },
        { 1.0f,  1.0f,  2.0f, 0.55f, 3.1f },
        { 2.0f, -1.0f, -1.0f, 0.40f, 5.2f },
        {-1.0f,  2.0f,  1.0f, 0.32f, 2.4f },
        { 2.0f,  2.0f, -2.0f, 0.22f, 4.8f },
        { 3.0f, -2.0f,  1.0f, 0.16f, 0.9f },
    };

    const float TAU = 6.28318530718f;
    float ampSum = 0.0f;
    for (const Ripple& w : waves) ampSum += w.amp;

    const Color deep = { 26, 62, 92, 206 };
    const Color mid = { 40, 92, 130, 200 };
    const Color shallow = { 66, 128, 162, 192 };
    const Color crest = { 122, 176, 198, 188 };

    auto mixCol = [](Color a, Color b, float t) {
        return Color{
            (unsigned char)(a.r + (b.r - a.r) * t),
            (unsigned char)(a.g + (b.g - a.g) * t),
            (unsigned char)(a.b + (b.b - a.b) * t),
            (unsigned char)(a.a + (b.a - a.a) * t) };
        };

    for (int f = 0; f < F; f++) {
        float tNorm = (float)f / (float)F;
        for (int y = 0; y < T; y++) {
            for (int x = 0; x < T; x++) {
                float u = (float)x / (float)T;
                float v = (float)y / (float)T;

                float h = 0.0f;
                for (const Ripple& w : waves)
                    h += w.amp * sinf(TAU * (w.fx * u + w.fy * v + w.ft * tNorm) + w.phase);
                h = h / ampSum;

                float n = h * 0.5f + 0.5f;
                n = std::clamp(n, 0.0f, 1.0f);

                Color c;
                if (n < 0.40f)      c = mixCol(deep, mid, n / 0.40f);
                else if (n < 0.72f) c = mixCol(mid, shallow, (n - 0.40f) / 0.32f);
                else                c = mixCol(shallow, crest, (n - 0.72f) / 0.28f);

                float sparkle = h;
                for (const Ripple& w : waves)
                    sparkle += 0.35f * w.amp * sinf(TAU * (w.fx * 2.0f * u + w.fy * 2.0f * v + w.ft * 2.0f * tNorm) + w.phase * 1.7f);
                if (sparkle > 1.45f) {
                    c.r = (unsigned char)std::min(255, c.r + 55);
                    c.g = (unsigned char)std::min(255, c.g + 45);
                    c.b = (unsigned char)std::min(255, c.b + 30);
                }

                ImageDrawPixel(&img, x, f * T + y, c);
            }
        }
    }

    waterAtlas = LoadTextureFromImage(img);
    SetTextureFilter(waterAtlas, TEXTURE_FILTER_POINT);
    SetTextureWrap(waterAtlas, TEXTURE_WRAP_REPEAT);
    UnloadImage(img);
}

Image Chunk::BuildAtlasImage() {
    const int T = 16;
    const int G = 16;
    const int SZ = G * T;

    Image img = GenImageColor(SZ, SZ, { 0, 0, 0, 0 });

    auto Hash01 = [](int hx, int hy, unsigned int s) {
        unsigned int h = (unsigned int)hx * 374761393u + (unsigned int)hy * 668265263u + s * 1442695041u;
        h = (h ^ (h >> 13)) * 1274126177u;
        return (float)((h ^ (h >> 16)) & 0xFFFFu) / 65535.0f;
        };

    auto Vnoise = [&](float fx, float fy, int cells, unsigned int s) {
        float gx = fx * cells, gy = fy * cells;
        int ix = (int)gx, iy = (int)gy;
        float tx = gx - ix; tx = tx * tx * (3.0f - 2.0f * tx);
        float ty = gy - iy; ty = ty * ty * (3.0f - 2.0f * ty);
        int x1 = (ix + 1) % cells, y1 = (iy + 1) % cells;
        float a = Hash01(ix, iy, s), b = Hash01(x1, iy, s);
        float c2 = Hash01(ix, y1, s), d = Hash01(x1, y1, s);
        float ab = a + (b - a) * tx, cd = c2 + (d - c2) * tx;
        return ab + (cd - ab) * ty;
        };

    auto Fbm = [&](float fx, float fy, unsigned int s) {
        return Vnoise(fx, fy, 2, s) * 0.46f + Vnoise(fx, fy, 4, s + 7u) * 0.28f +
            Vnoise(fx, fy, 8, s + 19u) * 0.17f + Vnoise(fx, fy, 16, s + 41u) * 0.09f;
        };

    auto MixC = [](Color a, Color b, float t) {
        t = Clamp(t, 0.0f, 1.0f);
        return Color{
            (unsigned char)((float)a.r + ((float)b.r - (float)a.r) * t),
            (unsigned char)((float)a.g + ((float)b.g - (float)a.g) * t),
            (unsigned char)((float)a.b + ((float)b.b - (float)a.b) * t),
            (unsigned char)((float)a.a + ((float)b.a - (float)a.a) * t) };
        };

    auto Tint = [](Color c, float k) {
        return Color{
            (unsigned char)Clamp((float)c.r * (1.0f + k), 0.0f, 255.0f),
            (unsigned char)Clamp((float)c.g * (1.0f + k), 0.0f, 255.0f),
            (unsigned char)Clamp((float)c.b * (1.0f + k), 0.0f, 255.0f), c.a };
        };

    auto PutPx = [&](int px, int py, Color base, float k, unsigned char a) {
        ImageDrawPixel(&img, px, py, {
            (unsigned char)Clamp((float)base.r + k, 0.0f, 255.0f),
            (unsigned char)Clamp((float)base.g + k, 0.0f, 255.0f),
            (unsigned char)Clamp((float)base.b + k, 0.0f, 255.0f), a });
        };

    auto PutWarm = [&](int px, int py, Color base, float warm, unsigned char a) {
        ImageDrawPixel(&img, px, py, {
            (unsigned char)Clamp((float)base.r + warm, 0.0f, 255.0f),
            (unsigned char)Clamp((float)base.g + warm * 0.25f, 0.0f, 255.0f),
            (unsigned char)Clamp((float)base.b - warm * 0.7f, 0.0f, 255.0f), a });
        };

    auto Natural = [](Color c, float desat, float darken) {
        float lum = 0.299f * c.r + 0.587f * c.g + 0.114f * c.b;
        auto mix = [&](unsigned char v) {
            float f = ((float)v * (1.0f - desat) + lum * desat) * (1.0f - darken);
            return (unsigned char)Clamp(f, 0.0f, 255.0f);
            };
        return Color{ mix(c.r), mix(c.g), mix(c.b), c.a };
        };

    auto OutlineTile = [&](int c, int r, float strength) {
        std::vector<Color> src((size_t)T * T);
        for (int y = 0; y < T; y++)
            for (int x = 0; x < T; x++)
                src[(size_t)y * T + x] = GetImageColor(img, c * T + x, r * T + y);

        for (int y = 0; y < T; y++) for (int x = 0; x < T; x++) {
            Color px = src[(size_t)y * T + x];
            if (px.a < 40) continue;

            bool edge = false;
            const int dx[4] = { 1, -1, 0, 0 };
            const int dy[4] = { 0, 0, 1, -1 };
            for (int k = 0; k < 4 && !edge; k++) {
                int nx = x + dx[k], ny = y + dy[k];
                if (nx < 0 || nx >= T || ny < 0 || ny >= T) { edge = true; break; }
                if (src[(size_t)ny * T + nx].a < 40) edge = true;
            }
            if (!edge) continue;

            bool lit = (x > 0 && y > 0 && src[(size_t)(y - 1) * T + x].a < 40);
            float k = lit ? -strength * 0.35f : strength;
            Color out = {
                (unsigned char)Clamp((float)px.r * (1.0f - k), 0.0f, 255.0f),
                (unsigned char)Clamp((float)px.g * (1.0f - k), 0.0f, 255.0f),
                (unsigned char)Clamp((float)px.b * (1.0f - k), 0.0f, 255.0f),
                px.a };
            ImageDrawPixel(&img, c * T + x, r * T + y, out);
        }
        };

    auto Relief = [&](int x, int y, float height) {
        (void)x; (void)y;
        return (height - 0.5f) * 7.0f;
        };

    auto Solid = [&](int c, int r, Color base, int noise) {
        unsigned int s = (unsigned int)(c * 31 + r * 17 + 1);
        float amp = Clamp((float)noise / 24.0f, 0.20f, 1.2f);

        Color dark = Tint(base, -0.13f * amp);
        Color lite = Tint(base, 0.11f * amp);

        for (int y = 0; y < T; y++) for (int x = 0; x < T; x++) {
            float fx = (x + 0.5f) / T, fy = (y + 0.5f) / T;

            float patch = Vnoise(fx, fy, 4, s);
            float t2 = Clamp(patch, 0.0f, 1.0f);

            Color col;
            if (t2 < 0.42f)      col = dark;
            else if (t2 > 0.62f) col = lite;
            else                 col = base;

            if (Hash01(x, y, s + 91u) > 0.90f) col = Tint(col, 0.05f);

            PutWarm(c * T + x, r * T + y, col, 0.0f, base.a);
        }
        };

    auto Ore = [&](int c, int r, Color stone, Color ore) {
        unsigned int s = (unsigned int)(c * 31 + r * 17 + 101);
        Color sDark = Tint(stone, -0.24f);
        Color sLite = Tint(stone, 0.16f);
        Color oDark = Tint(ore, -0.34f);
        Color oLite = Tint(ore, 0.22f);
        for (int y = 0; y < T; y++) for (int x = 0; x < T; x++) {
            float fx = (x + 0.5f) / T, fy = (y + 0.5f) / T;
            float rock = Clamp(Fbm(fx, fy, s) * 0.75f + Hash01(x, y, s + 5u) * 0.25f, 0.0f, 1.0f);
            Color base = (rock < 0.5f) ? MixC(sDark, stone, rock * 2.0f)
                : MixC(stone, sLite, (rock - 0.5f) * 2.0f);

            float vein = Vnoise(fx, fy, 3, s + 33u) * 0.62f + Vnoise(fx, fy, 6, s + 57u) * 0.38f;
            bool onVein = false;
            if (vein > 0.56f) {
                float core = Clamp((vein - 0.56f) / 0.30f, 0.0f, 1.0f);
                Color mineral = MixC(oDark, ore, Clamp(core * 1.6f, 0.0f, 1.0f));
                if (core > 0.72f) mineral = MixC(ore, oLite, (core - 0.72f) / 0.28f);
                base = MixC(base, mineral, Clamp(core * 2.2f, 0.0f, 1.0f));
                onVein = (core > 0.35f);
            }

            float shade = Relief(x, y, rock) + (onVein ? 5.0f : 0.0f);
            base = {
                (unsigned char)Clamp((float)base.r + shade, 0.0f, 255.0f),
                (unsigned char)Clamp((float)base.g + shade, 0.0f, 255.0f),
                (unsigned char)Clamp((float)base.b + shade, 0.0f, 255.0f), 255 };

            PutWarm(c * T + x, r * T + y, base, (Hash01(x, y, s + 211u) - 0.5f) * 4.0f, 255);
        }
        };

    auto Striped = [&](int c, int r, Color a, Color b, int h) {
        unsigned int s = (unsigned int)(c * 31 + r * 17 + 201);
        Color pale = Tint(a, 0.14f);
        float period = (float)std::max(2, h) * 1.35f;
        for (int y = 0; y < T; y++) for (int x = 0; x < T; x++) {
            float fx = (x + 0.5f) / T, fy = (y + 0.5f) / T;
            float warp = (Vnoise(fx, fy, 3, s) - 0.5f) * 3.4f + (Vnoise(fx, fy, 6, s + 3u) - 0.5f) * 1.6f;
            float band = sinf(((float)y + warp) * 6.28318f / period) * 0.5f + 0.5f;
            band = band * band;
            Color col = MixC(a, b, band * 0.85f);

            float fleck = Vnoise(fx * 3.0f, fy, 8, s + 61u);
            if (fleck > 0.74f) col = MixC(col, pale, (fleck - 0.74f) * 2.4f);

            float knot = Vnoise(fx, fy, 2, s + 97u);
            if (knot > 0.88f) col = MixC(col, Tint(b, -0.30f), (knot - 0.88f) * 5.0f);

            float n = (Hash01(x, y, s + 5u) - 0.5f) * 9.0f;
            PutPx(c * T + x, r * T + y, col, n, 255);
        }
        };

    auto LogTop = [&](int c, int r, Color wood, Color ring) {
        unsigned int s = (unsigned int)(c * 31 + r * 17 + 301);
        for (int y = 0; y < T; y++) for (int x = 0; x < T; x++) {
            float fx = (x + 0.5f) / T, fy = (y + 0.5f) / T;
            float dx = x - 7.5f, dy = y - 7.5f;
            float d = sqrtf(dx * dx + dy * dy) + (Vnoise(fx, fy, 4, s) - 0.5f) * 2.2f;
            float ringT = sinf(d * 2.05f) * 0.5f + 0.5f;
            Color col = MixC(wood, ring, ringT * ringT * 0.8f);
            if (d < 1.5f) col = MixC(col, Tint(ring, -0.22f), (1.5f - d) / 1.5f);
            float n = (Hash01(x, y, s + 9u) - 0.5f) * 12.0f;
            PutPx(c * T + x, r * T + y, col, n, 255);
        }
        };

    auto GrassSide = [&](int c, int r) {
        unsigned int s = 401u;
        const Color soil = { 92, 71, 51, 255 };
        const Color soilDark = { 70, 54, 39, 255 };
        const Color turf = { 86, 108, 50, 255 };
        const Color turfDark = { 62, 80, 38, 255 };
        for (int y = 0; y < T; y++) for (int x = 0; x < T; x++) {
            float fx = (x + 0.5f) / T, fy = (y + 0.5f) / T;
            float edge = 3.0f + Vnoise(fx, 0.31f, 8, s) * 3.4f;
            float blade = Vnoise(fx, 0.77f, 16, s + 13u);
            if (blade > 0.62f) edge += 1.6f;

            float m = Fbm(fx, fy, s + 3u);
            Color col;
            if ((float)y < edge) col = MixC(turfDark, turf, m);
            else {
                float below = Clamp(((float)y - edge) / 4.0f, 0.0f, 1.0f);
                col = MixC(soilDark, soil, m);
                col = MixC(MixC(turfDark, col, 0.55f), col, below);
            }
            PutWarm(c * T + x, r * T + y, col, (Hash01(x, y, s + 71u) - 0.5f) * 5.0f, 255);
        }
        };

    auto Leaves = [&](int c, int r, Color base) {
        unsigned int s = (unsigned int)(c * 31 + r * 17 + 501);
        Color deep = Tint(base, -0.38f);
        Color lit = Tint(base, 0.24f);
        for (int y = 0; y < T; y++) for (int x = 0; x < T; x++) {
            float fx = (x + 0.5f) / T, fy = (y + 0.5f) / T;
            float m = Vnoise(fx, fy, 4, s) * 0.6f + Vnoise(fx, fy, 8, s + 3u) * 0.4f;
            float dith = Hash01(x, y, s + 77u);
            if (m < 0.36f || dith < 0.10f) {
                ImageDrawPixel(&img, c * T + x, r * T + y, { 0,0,0,0 });
                continue;
            }
            float depth = Clamp((m - 0.36f) / 0.64f, 0.0f, 1.0f);
            Color col = (depth < 0.55f) ? MixC(deep, base, depth / 0.55f)
                : MixC(base, lit, (depth - 0.55f) / 0.45f);
            float n = (Hash01(x, y, s + 11u) - 0.5f) * 13.0f;
            PutPx(c * T + x, r * T + y, col, n, 255);
        }
        };

    auto Blade = [&](int c, int r, float bx, float baseY, float topY, float bend,
        Color lo, Color hi, unsigned int s) {
            float len = baseY - topY;
            if (len < 1.0f) return;
            for (float t = 0.0f; t <= 1.0f; t += 0.06f) {
                float yy = baseY - len * t;
                float xx = bx + bend * t * t;
                float w = (1.0f - t) * 1.35f + 0.35f;
                int px0 = (int)floorf(xx - w * 0.5f + 0.5f);
                int px1 = (int)floorf(xx + w * 0.5f + 0.5f);
                for (int px = px0; px <= px1; px++) {
                    int py = (int)yy;
                    if (px < 0 || px >= T || py < 0 || py >= T) continue;
                    Color col = MixC(lo, hi, t * 0.85f + Hash01(px, py, s) * 0.15f);
                    ImageDrawPixel(&img, c * T + px, r * T + py, col);
                }
            }
        };

    auto GrassTuft = [&](int c, int r, Color lo, Color hi) {
        unsigned int s = (unsigned int)(c * 31 + r * 17 + 601);
        const float bases[7] = { 4.0f, 6.0f, 7.5f, 9.0f, 10.5f, 12.0f, 5.0f };
        for (int i = 0; i < 7; i++) {
            float h = 4.0f + Hash01(i, 1, s) * 8.0f;
            float bend = (bases[i] - 8.0f) * 0.42f + (Hash01(i, 2, s) - 0.5f) * 2.0f;
            Blade(c, r, bases[i], 15.0f, 15.0f - h, bend, lo, hi, s + (unsigned int)i);
        }
        };

    auto FlowerTex = [&](int c, int r, Color petal, Color core, Color stem) {
        unsigned int s = (unsigned int)(c * 31 + r * 17 + 701);
        Color stemDark = Tint(stem, -0.30f);
        for (int y = 7; y < T; y++) {
            int px = 7 + (int)((float)(y - 7) * -0.06f);
            ImageDrawPixel(&img, c * T + px, r * T + y, MixC(stem, stemDark, (float)(y - 7) / 9.0f));
            ImageDrawPixel(&img, c * T + px + 1, r * T + y, stemDark);
        }
        for (int i = 0; i < 3; i++) {
            int ly = 9 + i * 2;
            int dir = (i % 2 == 0) ? -1 : 1;
            for (int k = 1; k <= 3; k++) {
                int px = 7 + dir * k + (dir > 0 ? 1 : 0);
                int py = ly - k / 2;
                if (px < 0 || px >= T || py < 0 || py >= T) continue;
                ImageDrawPixel(&img, c * T + px, r * T + py, (k < 3) ? stem : stemDark);
            }
        }
        Color petalDark = Tint(petal, -0.28f);
        for (int i = 0; i < 6; i++) {
            float a = 6.2831853f * (float)i / 6.0f + 0.4f;
            float pxf = 7.5f + cosf(a) * 2.7f;
            float pyf = 4.2f + sinf(a) * 2.3f;
            for (int dy = -1; dy <= 1; dy++) for (int dx = -1; dx <= 1; dx++) {
                if (abs(dx) + abs(dy) > 1) continue;
                int px = (int)pxf + dx, py = (int)pyf + dy;
                if (px < 0 || px >= T || py < 0 || py >= T) continue;
                ImageDrawPixel(&img, c * T + px, r * T + py,
                    (dx == 0 && dy == 0) ? petal : MixC(petal, petalDark, 0.55f));
            }
        }
        for (int dy = -1; dy <= 1; dy++) for (int dx = -1; dx <= 1; dx++) {
            if (abs(dx) + abs(dy) > 1) continue;
            ImageDrawPixel(&img, c * T + 7 + dx, r * T + 4 + dy, core);
        }
        (void)s;
        };

    auto MushroomTex = [&](int c, int r, Color cap, Color gill) {
        unsigned int s = (unsigned int)(c * 31 + r * 17 + 801);
        Color capDark = Tint(cap, -0.32f), capLite = Tint(cap, 0.20f);
        for (int y = 9; y < 15; y++) for (int x = 6; x < 10; x++) {
            Color col = (x == 6 || x == 9) ? Tint(gill, -0.22f) : gill;
            PutPx(c * T + x, r * T + y, col, (Hash01(x, y, s) - 0.5f) * 10.0f, 255);
        }
        for (int y = 3; y < 10; y++) {
            float t = (float)(y - 3) / 6.0f;
            float halfW = 1.5f + 4.6f * sinf(t * 1.55f);
            for (int x = 0; x < T; x++) {
                float d = fabsf((float)x - 7.5f);
                if (d > halfW) continue;
                Color col;
                if (y >= 9) col = Tint(gill, -0.30f);
                else {
                    float shade = 1.0f - d / (halfW + 0.5f);
                    col = MixC(capDark, capLite, shade * 0.85f + Hash01(x, y, s + 3u) * 0.15f);
                    col = MixC(col, cap, 0.35f);
                }
                ImageDrawPixel(&img, c * T + x, r * T + y, col);
            }
        }
        };

    auto DeadBushTex = [&](int c, int r) {
        unsigned int s = 901u;
        const Color twig = { 118, 96, 58, 255 };
        const Color twigDark = { 88, 70, 42, 255 };
        for (int y = 6; y < T; y++)
            ImageDrawPixel(&img, c * T + 7, r * T + y, MixC(twig, twigDark, (float)(y - 6) / 10.0f));
        struct Br { int x, y, dx, len; };
        const Br brs[6] = { {7,11,-1,5}, {7,9,1,4}, {7,7,-1,4}, {7,12,1,3}, {7,6,1,3}, {7,10,-1,3} };
        for (const Br& b : brs) {
            for (int k = 1; k <= b.len; k++) {
                int px = b.x + b.dx * k;
                int py = b.y - k + (k > 2 ? 1 : 0);
                if (px < 0 || px >= T || py < 0 || py >= T) continue;
                ImageDrawPixel(&img, c * T + px, r * T + py,
                    (Hash01(px, py, s) > 0.5f) ? twig : twigDark);
            }
        }
        };

    auto BirchLog = [&](int c, int r) {
        unsigned int s = 701u;
        const Color bark = { 198, 194, 182, 255 };
        const Color barkDark = { 168, 163, 150, 255 };
        const Color mark = { 58, 52, 44, 255 };
        for (int y = 0; y < T; y++) for (int x = 0; x < T; x++) {
            float fx = (x + 0.5f) / T, fy = (y + 0.5f) / T;
            Color col = MixC(barkDark, bark, Fbm(fx, fy, s + 21u));
            bool dash = (y % 5 == 2 || y % 5 == 3) && (Hash01(x / 2, y / 5, s) > 0.52f);
            if (dash) col = MixC(col, mark, 0.72f + Hash01(x, y, s + 3u) * 0.28f);
            PutPx(c * T + x, r * T + y, col, (Hash01(x, y, s + 5u) - 0.5f) * 8.0f, 255);
        }
        };

    auto Lava = [&](int c, int r) {
        unsigned int s = 801u;
        const Color crust = { 62, 26, 18, 255 };
        const Color glow = { 176, 62, 16, 255 };
        const Color core = { 240, 154, 42, 255 };
        for (int y = 0; y < T; y++) for (int x = 0; x < T; x++) {
            float fx = (x + 0.5f) / T, fy = (y + 0.5f) / T;
            float m = Fbm(fx, fy, s);
            float hot = Clamp((m - 0.30f) / 0.55f, 0.0f, 1.0f);
            Color col = (hot < 0.55f) ? MixC(crust, glow, hot / 0.55f)
                : MixC(glow, core, (hot - 0.55f) / 0.45f);
            PutPx(c * T + x, r * T + y, col, (Hash01(x, y, s + 7u) - 0.5f) * 10.0f, 255);
        }
        };

    auto IceTex = [&](int c, int r, Color base, unsigned char alpha, float crackAmp) {
        unsigned int s = (unsigned int)(c * 8 + r + 1001);
        for (int y = 0; y < T; y++) for (int x = 0; x < T; x++) {
            float fx = (x + 0.5f) / T, fy = (y + 0.5f) / T;
            float n = Fbm(fx, fy, s) * 2.0f - 1.0f;
            float cr1 = 1.0f - fabsf(Vnoise(fx, fy, 4, s + 31u) * 2.0f - 1.0f);
            float cr2 = 1.0f - fabsf(Vnoise(fx, fy, 8, s + 67u) * 2.0f - 1.0f);
            float crack = std::max(powf(cr1, 8.0f), powf(cr2, 10.0f) * 0.8f) * crackAmp;
            PutPx(c * T + x, r * T + y, base, n * 10.0f + crack * 70.0f, alpha);
        }
        };

    auto PlanksTex = [&](int c, int r) {
        unsigned int s = 1101u;
        for (int y = 0; y < T; y++) for (int x = 0; x < T; x++) {
            int board = x / 4;
            bool gap = (x % 4 == 0);
            bool nail = (!gap && (y == 2 || y == 13) && (x % 4 == 2));
            Color base = gap ? Color{ 96, 62, 30, 255 } : Color{ 156, 110, 58, 255 };
            if (nail) base = { 72, 52, 36, 255 };
            float n = (Hash01(x + board * 7, y, s) - 0.5f) * 18.0f;
            float grain = (Hash01(board, y / 2, s + 9u) - 0.5f) * 14.0f;
            PutPx(c * T + x, r * T + y, base, n + grain, 255);
        }
        };

    auto FernTex = [&](int c, int r) {
        unsigned int s = 1201u;
        const Color frondLit = { 74, 106, 46, 255 };
        const Color frondDim = { 48, 76, 34, 255 };

        auto blade = [&](float bx, float by, float dirX, float len, float droop) {
            for (float t = 0.0f; t < len; t += 0.5f) {
                float u = t / len;
                int px = (int)(bx + dirX * t);
                int py = (int)(by + droop * u * u * len * 0.55f - t * 0.12f);
                if (px < 0 || px >= T || py < 0 || py >= T) continue;
                Color col = MixC(frondLit, frondDim, u * 0.8f);
                ImageDrawPixel(&img, c * T + px, r * T + py, col);
                if (u > 0.15f && u < 0.9f && ((int)(t * 2.0f) % 2 == 0)) {
                    int qy = py - 1;
                    if (qy >= 0) ImageDrawPixel(&img, c * T + px, r * T + qy, MixC(col, frondDim, 0.4f));
                }
            }
            };

        for (int y = 7; y < T; y++) {
            float t = (float)(y - 7) / 9.0f;
            ImageDrawPixel(&img, c * T + 7, r * T + y, MixC(frondDim, Color{ 62, 50, 30, 255 }, t));
            ImageDrawPixel(&img, c * T + 8, r * T + y, MixC(frondDim, Color{ 52, 42, 26, 255 }, t));
        }

        for (int i = 0; i < 5; i++) {
            float by = 9.0f - (float)i * 1.9f;
            float len = 6.2f - (float)i * 0.75f;
            float droop = 0.30f + Hash01(i, 3, s) * 0.22f;
            blade(7.0f, by, -1.0f, len, droop);
            blade(8.0f, by + 0.6f, 1.0f, len, droop);
        }
        };

    auto ReedTex = [&](int c, int r) {
        unsigned int s = 1301u;
        const int stems[5] = { 2, 5, 8, 11, 14 };
        for (int k = 0; k < 5; k++) {
            int sx = stems[k];
            int top = 1 + (int)(Hash01(k, 0, s) * 5.0f);
            float lean = (Hash01(k, 9, s) - 0.5f) * 1.6f;
            Color stalk = (Hash01(k, 4, s) > 0.5f) ? Color{ 92, 124, 56, 255 } : Color{ 74, 106, 48, 255 };

            for (int y = top; y < T; y++) {
                float u = (float)(y - top) / (float)(T - top);
                int px = sx + (int)(lean * (1.0f - u) * (1.0f - u) * 2.0f);
                if (px < 0 || px >= T) continue;
                Color col = MixC(Tint(stalk, 0.10f), Tint(stalk, -0.26f), u);
                ImageDrawPixel(&img, c * T + px, r * T + y, col);
                if (Hash01(px, y, s + 5u) > 0.72f && px + 1 < T)
                    ImageDrawPixel(&img, c * T + px + 1, r * T + y, Tint(col, -0.20f));
            }

            int hx = sx + (int)(lean * 2.0f);
            for (int y = top; y < top + 4 && y < T; y++) {
                if (hx < 0 || hx >= T) continue;
                ImageDrawPixel(&img, c * T + hx, r * T + y, { 118, 88, 52, 255 });
                if (hx + 1 < T) ImageDrawPixel(&img, c * T + hx + 1, r * T + y, { 96, 70, 42, 255 });
            }
        }
        };

    auto MineralTex = [&](int c, int r, Color host, Color mineral, float density) {
        unsigned int s = (unsigned int)(c * 31 + r * 17 + 1501);
        Color hDark = Tint(host, -0.26f), hLite = Tint(host, 0.18f);
        Color mDark = Tint(mineral, -0.32f), mLite = Tint(mineral, 0.24f);
        for (int y = 0; y < T; y++) for (int x = 0; x < T; x++) {
            float fx = (x + 0.5f) / T, fy = (y + 0.5f) / T;
            float rock = Clamp(Fbm(fx, fy, s) * 0.72f + Hash01(x, y, s + 7u) * 0.28f, 0.0f, 1.0f);
            Color base = (rock < 0.5f) ? MixC(hDark, host, rock * 2.0f)
                : MixC(host, hLite, (rock - 0.5f) * 2.0f);
            float grain = Vnoise(fx, fy, 4, s + 41u) * 0.6f + Vnoise(fx, fy, 8, s + 83u) * 0.4f;
            if (grain > 1.0f - density) {
                float k = Clamp((grain - (1.0f - density)) / density, 0.0f, 1.0f);
                Color spec = (k > 0.7f) ? MixC(mineral, mLite, (k - 0.7f) / 0.3f) : MixC(mDark, mineral, k / 0.7f);
                base = MixC(base, spec, Clamp(k * 1.8f, 0.0f, 1.0f));
            }
            PutWarm(c * T + x, r * T + y, base, (Hash01(x, y, s + 211u) - 0.5f) * 4.0f, 255);
        }
        };

    auto TorchTex = [&](int c, int r) {
        unsigned int s = 1401u;
        for (int y = 4; y < T; y++) for (int x = 6; x < 10; x++) {
            float u = (float)(y - 4) / 12.0f;
            Color wood = (x == 6 || x == 9) ? Color{ 78, 54, 32, 255 } : Color{ 112, 78, 44, 255 };
            PutPx(c * T + x, r * T + y, MixC(wood, Tint(wood, -0.35f), u),
                (Hash01(x, y, s) - 0.5f) * 12.0f, 255);
        }
        for (int y = 0; y < 5; y++) for (int x = 5; x < 11; x++) {
            float dx = x - 7.5f, dy = y - 2.6f;
            float d = sqrtf(dx * dx + dy * dy * 1.6f) / 3.2f;
            if (d > 1.0f + (Hash01(x, y, s + 3u) - 0.5f) * 0.35f) continue;
            Color flame = (d < 0.38f) ? Color{ 255, 246, 206, 255 }
                : (d < 0.72f) ? Color{ 248, 186, 62, 255 } : Color{ 214, 104, 30, 255 };
            ImageDrawPixel(&img, c * T + x, r * T + y, flame);
        }
        };

    auto Sapling = [&](int c, int r) {
        unsigned int s = 901u;
        for (int y = 9; y < 15; y++) {
            ImageDrawPixel(&img, c * T + 7, r * T + y, { 96, 62, 30, 255 });
            ImageDrawPixel(&img, c * T + 8, r * T + y, { 82, 52, 24, 255 });
        }
        for (int y = 2; y < 11; y++) for (int x = 3; x < 13; x++) {
            float dx = x - 7.5f, dy = y - 6.0f;
            float d = sqrtf(dx * dx + dy * dy);
            if (d < 3.6f + (Hash01(x, y, s) - 0.5f) * 2.2f) {
                Color g = (Hash01(x, y, s + 3u) > 0.5f) ? Color{ 62, 142, 46, 255 } : Color{ 44, 116, 34, 255 };
                ImageDrawPixel(&img, c * T + x, r * T + y, g);
            }
        }
        };

    auto BushTex = [&](int c, int r, bool ripe) {
        unsigned int s = 1601u;
        const Color leafLit = { 74, 118, 48, 255 };
        const Color leafDim = { 44, 82, 34, 255 };
        const Color twig = { 82, 62, 38, 255 };

        for (int y = 8; y < T; y++) {
            int px = 7 + (int)((Hash01(0, y, s) - 0.5f) * 1.6f);
            PutPx(c * T + px, r * T + y, twig, (Hash01(px, y, s + 2u) - 0.5f) * 10.0f, 255);
            if (px + 1 < T)
                PutPx(c * T + px + 1, r * T + y, Tint(twig, -0.22f), 0.0f, 255);
        }

        for (int k = 0; k < 3; k++) {
            int bx = 4 + k * 4;
            for (int y = 3; y < 13; y++) for (int x = bx - 3; x <= bx + 3; x++) {
                if (x < 0 || x >= T) continue;
                float dx = (float)(x - bx), dy = (float)(y - 7) * 1.25f;
                float d = sqrtf(dx * dx + dy * dy);
                float edge = 3.1f + (Hash01(x, y, s + k * 13u) - 0.5f) * 1.9f;
                if (d > edge) continue;
                float u = Clamp(d / edge, 0.0f, 1.0f);
                Color g = MixC(leafLit, leafDim, u * 0.85f + Hash01(x, y, s + 5u) * 0.15f);
                PutPx(c * T + x, r * T + y, g, (Hash01(x, y, s + 9u) - 0.5f) * 8.0f, 255);
            }
        }

        if (!ripe) return;

        const int bpx[7] = { 4, 7, 11, 5, 9, 12, 8 };
        const int bpy[7] = { 5, 4, 6, 9, 8, 10, 11 };
        for (int i = 0; i < 7; i++) {
            int x = bpx[i], y = bpy[i];
            Color deep = { 152, 34, 52, 255 };
            Color lit = { 206, 62, 74, 255 };
            PutPx(c * T + x, r * T + y, lit, 0.0f, 255);
            if (x + 1 < T) PutPx(c * T + x + 1, r * T + y, deep, 0.0f, 255);
            if (y + 1 < T) PutPx(c * T + x, r * T + y + 1, deep, 0.0f, 255);
            if (x + 1 < T && y + 1 < T)
                PutPx(c * T + x + 1, r * T + y + 1, Tint(deep, -0.28f), 0.0f, 255);
        }
        };

    auto BerriesTex = [&](int c, int r) {
        unsigned int s = 1607u;
        const int bx[6] = { 4, 8, 12, 6, 10, 8 };
        const int by[6] = { 5, 4, 6, 9, 10, 13 };
        for (int i = 0; i < 6; i++) {
            for (int y = -2; y <= 2; y++) for (int x = -2; x <= 2; x++) {
                float d = sqrtf((float)(x * x + y * y));
                if (d > 2.2f) continue;
                int px = bx[i] + x, py = by[i] + y;
                if (px < 0 || px >= T || py < 0 || py >= T) continue;
                float u = Clamp(d / 2.2f, 0.0f, 1.0f);
                Color col = MixC(Color{ 214, 74, 84, 255 }, Color{ 128, 26, 44, 255 }, u);
                if (x <= -1 && y <= -1) col = MixC(col, Color{ 246, 168, 172, 255 }, 0.55f);
                PutPx(c * T + px, r * T + py, col, (Hash01(px, py, s) - 0.5f) * 6.0f, 255);
            }
        }
        };

    auto AcornTex = [&](int c, int r) {
        unsigned int s = 1613u;
        for (int y = 4; y < 13; y++) for (int x = 5; x < 12; x++) {
            float dx = (float)(x - 8) / 3.2f, dy = (float)(y - 9) / 4.2f;
            if (dx * dx + dy * dy > 1.0f) continue;
            float u = Clamp((float)(y - 4) / 9.0f, 0.0f, 1.0f);
            Color nut = MixC(Color{ 188, 142, 78, 255 }, Color{ 126, 86, 44, 255 }, u);
            if (x <= 6) nut = MixC(nut, Color{ 214, 178, 118, 255 }, 0.4f);
            PutPx(c * T + x, r * T + y, nut, (Hash01(x, y, s) - 0.5f) * 8.0f, 255);
        }
        for (int y = 2; y < 6; y++) for (int x = 5; x < 12; x++) {
            float dx = (float)(x - 8) / 3.4f;
            if (dx * dx > 1.0f) continue;
            Color cap = MixC(Color{ 96, 66, 36, 255 }, Color{ 62, 42, 24, 255 },
                (float)(y - 2) / 4.0f);
            if (Hash01(x, y, s + 3u) > 0.6f) cap = Tint(cap, 0.16f);
            PutPx(c * T + x, r * T + y, cap, 0.0f, 255);
        }
        for (int y = 0; y < 3; y++) PutPx(c * T + 8, r * T + y, { 74, 52, 30, 255 }, 0.0f, 255);
        };

    auto GrubTex = [&](int c, int r) {
        unsigned int s = 1619u;
        auto grub = [&](int ox, int oy, float bend) {
            for (int i = 0; i < 9; i++) {
                float t = (float)i / 8.0f;
                int px = ox + i;
                int py = oy + (int)(sinf(t * 3.14159f) * bend);
                float rad = 1.6f - fabsf(t - 0.45f) * 1.3f;
                for (int dy = -2; dy <= 2; dy++) {
                    if (fabsf((float)dy) > rad) continue;
                    int qx = px, qy = py + dy;
                    if (qx < 0 || qx >= T || qy < 0 || qy >= T) continue;
                    Color body = MixC(Color{ 236, 224, 190, 255 }, Color{ 190, 172, 132, 255 },
                        fabsf((float)dy) / 2.0f);
                    if (i % 2 == 0) body = Tint(body, -0.10f);
                    if (i == 0) body = Color{ 138, 96, 62, 255 };
                    PutPx(c * T + qx, r * T + qy, body, (Hash01(qx, qy, s) - 0.5f) * 6.0f, 255);
                }
            }
            };
        grub(3, 5, 2.2f);
        grub(4, 11, -1.8f);
        };

    auto EggTex = [&](int c, int r) {
        unsigned int s = 1627u;
        for (int y = 2; y < 15; y++) for (int x = 4; x < 13; x++) {
            float dx = (float)(x - 8) / 4.0f;
            float dy = (float)(y - 9) / 6.0f;
            if (dx * dx + dy * dy > 1.0f) continue;
            float shade = Clamp(((float)(x - 4) + (float)(y - 2)) / 20.0f, 0.0f, 1.0f);
            Color shell = MixC(Color{ 238, 230, 208, 255 }, Color{ 186, 174, 148, 255 }, shade);
            if (Hash01(x, y, s + 11u) > 0.86f) shell = MixC(shell, Color{ 138, 112, 82, 255 }, 0.6f);
            PutPx(c * T + x, r * T + y, shell, (Hash01(x, y, s) - 0.5f) * 5.0f, 255);
        }
        };

    auto FibreTex = [&](int c, int r) {
        unsigned int s = 1631u;
        for (int k = 0; k < 7; k++) {
            float bx = 2.0f + (float)k * 1.9f;
            float lean = (Hash01(k, 1, s) - 0.5f) * 3.4f;
            for (int y = 3; y < 14; y++) {
                float u = (float)(y - 3) / 11.0f;
                int px = (int)(bx + lean * u);
                if (px < 0 || px >= T) continue;
                Color fib = MixC(Color{ 186, 170, 108, 255 }, Color{ 132, 116, 68, 255 },
                    u * 0.7f + Hash01(px, y, s + 3u) * 0.3f);
                PutPx(c * T + px, r * T + y, fib, (Hash01(px, y, s) - 0.5f) * 7.0f, 255);
            }
        }
        };

    auto NuggetTex = [&](int c, int r, Color metal, Color hi) {
        unsigned int s = (unsigned int)(1700 + c * 37 + r * 11);
        const int nx[5] = { 5, 9, 7, 11, 4 };
        const int ny[5] = { 6, 5, 10, 11, 12 };
        const float nr[5] = { 2.6f, 2.1f, 2.4f, 1.7f, 1.5f };

        for (int i = 0; i < 5; i++) {
            for (int y = -4; y <= 4; y++) for (int x = -4; x <= 4; x++) {
                float d = sqrtf((float)(x * x + y * y));
                float edge = nr[i] + (Hash01(nx[i] + x, ny[i] + y, s) - 0.5f) * 1.1f;
                if (d > edge) continue;
                int px = nx[i] + x, py = ny[i] + y;
                if (px < 0 || px >= T || py < 0 || py >= T) continue;

                float u = Clamp(d / std::max(0.4f, edge), 0.0f, 1.0f);
                Color col = MixC(hi, metal, u * 0.85f);
                if (x <= -1 && y <= -1) col = MixC(col, hi, 0.5f);
                if (u > 0.82f) col = Tint(col, -0.35f);
                PutWarm(c * T + px, r * T + py, col,
                    (Hash01(px, py, s + 3u) - 0.5f) * 8.0f, 255);
            }
        }
        };

    Solid(0, 0, { 92, 118, 56, 255 }, 16);
    Solid(1, 0, { 102, 78, 56, 255 }, 14);
    Solid(2, 0, { 126, 124, 121, 255 }, 14);
    Solid(3, 0, { 200, 186, 150, 255 }, 12);
    Solid(4, 0, { 162, 96, 58, 255 }, 14);
    Solid(5, 0, { 124, 118, 109, 255 }, 18);
    Solid(6, 0, { 106, 104, 101, 255 }, 16);
    Solid(7, 0, { 36,35,38,255 }, 10);

    Striped(0, 1, { 104,76,46,255 }, { 78,55,32,255 }, 3);
    LogTop(1, 1, { 122,96,60,255 }, { 88,64,38,255 });
    Leaves(2, 1, { 62,92,44,255 });
    Striped(3, 1, { 66,50,34,255 }, { 46,34,22,255 }, 3);
    Leaves(4, 1, { 44,72,46,255 });
    BirchLog(5, 1);
    Leaves(6, 1, { 104,132,72,255 });
    Leaves(7, 1, { 94,114,56,255 });

    Solid(0, 2, { 28,100,200,100 }, 15);
    Lava(1, 2);
    Solid(2, 2, { 196,216,228,96 }, 6);
    IceTex(3, 2, { 150,186,208,255 }, 176, 1.0f);
    Solid(4, 2, { 226,230,236,255 }, 8);
    IceTex(5, 2, { 128,166,192,255 }, 236, 1.2f);
    Solid(6, 2, { 70,56,42,255 }, 18);
    Solid(7, 2, { 124,130,138,255 }, 14);

    Ore(0, 3, { 152,124,112,255 }, { 176,146,130,255 });
    Solid(1, 3, { 44,44,48,255 }, 12);
    Ore(2, 3, { 62,62,66,255 }, { 84,86,90,255 });
    Solid(3, 3, { 132,132,136,255 }, 16);
    Solid(4, 3, { 154,104,92,255 }, 18);
    Solid(5, 3, { 112,114,104,255 }, 15);
    Ore(6, 3, { 214,210,204,255 }, { 182,178,184,255 });
    Solid(7, 3, { 110,86,60,255 }, 22);

    Solid(0, 4, { 192,188,168,255 }, 12);
    Striped(1, 4, { 186,166,122,255 }, { 162,142,102,255 }, 5);
    Striped(2, 4, { 154,92,64,255 }, { 130,72,48,255 }, 5);
    Striped(3, 4, { 78,78,84,255 }, { 60,60,66,255 }, 3);
    Solid(4, 4, { 214,213,205,255 }, 13);
    Solid(5, 4, { 184,184,176,255 }, 12);
    Ore(6, 4, { 134,118,98,255 }, { 100,86,70,255 });
    Solid(7, 4, { 96,92,84,255 }, 15);
    Striped(8, 4, { 150,138,120,255 }, { 131,120,103,255 }, 4);
    BushTex(9, 4, false);
    BushTex(10, 4, true);
    BerriesTex(11, 4);
    AcornTex(12, 4);
    GrubTex(13, 4);
    EggTex(14, 4);
    FibreTex(15, 4);
    NuggetTex(0, 8, { 168, 96, 48, 255 }, { 232, 158, 92, 255 });
    NuggetTex(1, 8, { 158, 160, 164, 255 }, { 222, 224, 228, 255 });
    NuggetTex(2, 8, { 178, 182, 190, 255 }, { 244, 246, 250, 255 });
    NuggetTex(3, 8, { 196, 160, 56, 255 }, { 252, 224, 128, 255 });

    auto LumpTex = [&](int c, int r, Color base) {
        unsigned int s = (unsigned int)(1810 + c * 13 + r * 7);
        for (int y = 4; y < 14; y++) for (int x = 3; x < 13; x++) {
            float dx = (float)(x - 8) / 4.6f, dy = (float)(y - 9) / 4.6f;
            float d = sqrtf(dx * dx + dy * dy);
            if (d > 1.0f + (Hash01(x, y, s) - 0.5f) * 0.30f) continue;
            Color col = MixC(Tint(base, 0.16f), Tint(base, -0.24f), Clamp(d, 0.0f, 1.0f));
            PutWarm(c * T + x, r * T + y, col, (Hash01(x, y, s + 5u) - 0.5f) * 9.0f, 255);
        }
        };

    auto VesselTex = [&](int c, int r, Color base, bool fired) {
        unsigned int s = (unsigned int)(1830 + c * 17 + r * 3);
        Color body = fired ? Tint(base, -0.10f) : base;
        for (int y = 3; y < 14; y++) {
            float t = (float)(y - 3) / 11.0f;
            int half = (int)(2.6f + t * 3.2f);
            for (int x = 8 - half; x <= 7 + half; x++) {
                if (x < 0 || x >= T) continue;
                Color col = MixC(Tint(body, 0.18f), Tint(body, -0.26f),
                    Clamp(fabsf((float)x - 7.5f) / (float)(half + 1), 0.0f, 1.0f));
                if (y < 5) col = Tint(col, -0.30f);
                PutWarm(c * T + x, r * T + y, col, (Hash01(x, y, s) - 0.5f) * 7.0f, 255);
            }
        }
        };

    auto MouldTex = [&](int c, int r, Color base, bool fired) {
        unsigned int s = (unsigned int)(1850 + c * 19 + r * 5);
        Color body = fired ? Tint(base, -0.08f) : base;
        for (int y = 4; y < 13; y++) for (int x = 2; x < 14; x++) {
            Color col = MixC(Tint(body, 0.14f), Tint(body, -0.22f),
                Clamp((float)(y - 4) / 9.0f, 0.0f, 1.0f));
            bool cavity = (y >= 7 && y <= 9 && x >= 4 && x <= 11);
            if (cavity) col = Tint(col, -0.52f);
            PutWarm(c * T + x, r * T + y, col, (Hash01(x, y, s) - 0.5f) * 6.0f, 255);
        }
        };

    auto TongsTex = [&](int c, int r) {
        unsigned int s = 1870u;
        for (int i = 0; i < 11; i++) {
            int y = 3 + i;
            int lx = 6 - (i < 5 ? i / 2 : 2);
            int rx = 9 + (i < 5 ? i / 2 : 2);
            for (int k = 0; k < 2; k++) {
                int px = (k == 0) ? lx : rx;
                if (px < 0 || px >= T) continue;
                Color wood = MixC(Color{ 122, 86, 50, 255 }, Color{ 82, 58, 34, 255 },
                    (float)i / 11.0f);
                PutWarm(c * T + px, r * T + y, wood, (Hash01(px, y, s) - 0.5f) * 8.0f, 255);
            }
        }
        for (int x = 6; x <= 9; x++)
            PutWarm(c * T + x, r * T + 13, { 96, 68, 40, 255 }, 0.0f, 255);
        };

    LumpTex(4, 8, { 128, 106, 88, 255 });
    VesselTex(5, 8, { 132, 104, 86, 255 }, false);
    VesselTex(6, 8, { 156, 96, 66, 255 }, true);
    MouldTex(7, 8, { 132, 104, 86, 255 }, false);
    MouldTex(8, 8, { 156, 96, 66, 255 }, true);
    TongsTex(9, 8);

    auto PitTex = [&](int c, int r, int stage) {
        unsigned int s = (unsigned int)(1890 + stage * 29);
        for (int y = 0; y < T; y++) for (int x = 0; x < T; x++) {
            float fx = (x + 0.5f) / T, fy = (y + 0.5f) / T;
            float n = Fbm(fx, fy, s);
            Color soil = MixC(Color{ 58, 46, 34, 255 }, Color{ 84, 66, 48, 255 }, n);
            PutWarm(c * T + x, r * T + y, soil, (Hash01(x, y, s) - 0.5f) * 8.0f, 255);
        }

        for (int i = 0; i < 5; i++) {
            int bx = 2 + i * 3;
            for (int y = 5; y < 12; y++) {
                int px = bx + ((y - 5) / 3);
                if (px < 0 || px >= T) continue;
                Color wood = (i % 2 == 0) ? Color{ 96, 68, 40, 255 }
                                          : Color{ 74, 52, 32, 255 };
                if (stage == 2) wood = Tint(wood, -0.45f);
                PutWarm(c * T + px, r * T + y, wood, (Hash01(px, y, s + 7u) - 0.5f) * 9.0f, 255);
            }
        }

        if (stage == 0) return;

        for (int y = 3; y < 14; y++) for (int x = 3; x < 13; x++) {
            float dx = (float)(x - 8) / 4.4f, dy = (float)(y - 9) / 5.0f;
            float d = sqrtf(dx * dx + dy * dy);
            float edge = 1.0f + (Hash01(x, y, s + 11u) - 0.5f) * 0.42f;
            if (d > edge) continue;

            Color hot;
            if (stage == 1) {
                hot = (d < 0.34f) ? Color{ 255, 244, 198, 255 }
                    : (d < 0.68f) ? Color{ 246, 176, 56, 255 }
                    : Color{ 208, 92, 28, 255 };
            }
            else {
                if (Hash01(x, y, s + 17u) < 0.55f) continue;
                hot = (d < 0.5f) ? Color{ 214, 96, 34, 255 } : Color{ 132, 48, 22, 255 };
            }
            ImageDrawPixel(&img, c * T + x, r * T + y, hot);
        }
        };

    auto AshTex = [&](int c, int r) {
        unsigned int s = 1930u;
        for (int y = 0; y < T; y++) for (int x = 0; x < T; x++) {
            float fx = (x + 0.5f) / T, fy = (y + 0.5f) / T;
            float n = Fbm(fx, fy, s);
            Color a = MixC(Color{ 96, 92, 88, 255 }, Color{ 158, 154, 148, 255 }, n);
            if (Hash01(x, y, s + 5u) > 0.93f) a = Tint(a, -0.42f);
            PutWarm(c * T + x, r * T + y, a, (Hash01(x, y, s + 3u) - 0.5f) * 7.0f, 255);
        }
        };

    auto CattailTex = [&](int c, int r) {
        unsigned int s = 1950u;
        const int stems[4] = { 3, 7, 10, 13 };
        for (int k = 0; k < 4; k++) {
            int sx = stems[k];
            int top = 1 + (int)(Hash01(k, 1, s) * 3.0f);
            float lean = (Hash01(k, 5, s) - 0.5f) * 1.4f;
            for (int y = top; y < T; y++) {
                float u = (float)(y - top) / (float)(T - top);
                int px = sx + (int)(lean * (1.0f - u) * 2.0f);
                if (px < 0 || px >= T) continue;
                Color stalk = MixC(Natural(Color{ 104, 138, 62, 255 }, 0.34f, 0.10f),
                    Natural(Color{ 66, 96, 44, 255 }, 0.30f, 0.14f), u);
                PutWarm(c * T + px, r * T + y, stalk, (Hash01(px, y, s) - 0.5f) * 7.0f, 255);
                if (px + 1 < T)
                    PutWarm(c * T + px + 1, r * T + y, Tint(stalk, -0.26f), 0.0f, 255);
            }
            if (k % 2 == 0) {
                int hx = sx + (int)(lean * 2.0f);
                for (int y = top + 1; y < top + 6 && y < T; y++) {
                    for (int dx = -1; dx <= 2; dx++) {
                        int px = hx + dx;
                        if (px < 0 || px >= T) continue;
                        Color head = MixC(Natural(Color{ 122, 82, 44, 255 }, 0.26f, 0.10f),
                            Natural(Color{ 82, 54, 30, 255 }, 0.22f, 0.12f),
                            (float)(y - top) / 6.0f);
                        PutWarm(c * T + px, r * T + y, head, 0.0f, 255);
                    }
                }
            }
        }
        };

    auto LilyTex = [&](int c, int r) {
        unsigned int s = 1960u;
        for (int y = 1; y < 15; y++) for (int x = 1; x < 15; x++) {
            float dx = (float)(x - 8) / 6.6f, dy = (float)(y - 8) / 6.6f;
            float d = sqrtf(dx * dx + dy * dy);
            if (d > 1.0f + (Hash01(x, y, s) - 0.5f) * 0.16f) continue;
            float ang = atan2f((float)(y - 8), (float)(x - 8));
            if (ang > 0.55f && ang < 1.15f && d > 0.35f) continue;
            Color leaf = MixC(Natural(Color{ 74, 122, 52, 255 }, 0.36f, 0.12f),
                              Natural(Color{ 44, 84, 38, 255 }, 0.32f, 0.16f), d * 0.9f);
            if (d < 0.25f) leaf = Tint(leaf, 0.16f);
            PutWarm(c * T + x, r * T + y, leaf, (Hash01(x, y, s + 3u) - 0.5f) * 8.0f, 255);
        }
        };

    auto CranberryTex = [&](int c, int r) {
        unsigned int s = 1970u;
        for (int k = 0; k < 4; k++) {
            int bx = 3 + k * 3;
            for (int y = 6; y < T; y++) {
                int px = bx + ((y - 6) / 4);
                if (px < 0 || px >= T) continue;
                PutWarm(c * T + px, r * T + y, Natural(Color{ 72, 92, 48, 255 }, 0.32f, 0.14f),
                    (Hash01(px, y, s) - 0.5f) * 7.0f, 255);
            }
        }
        for (int y = 3; y < 12; y++) for (int x = 2; x < 14; x++) {
            if (Hash01(x, y, s + 4u) < 0.62f) continue;
            Color g = (Hash01(x, y, s + 8u) > 0.5f)
                ? Natural(Color{ 84, 116, 52, 255 }, 0.34f, 0.12f)
                : Natural(Color{ 62, 94, 44, 255 }, 0.30f, 0.16f);
            PutWarm(c * T + x, r * T + y, g, 0.0f, 255);
        }
        const int bpx[5] = { 4, 8, 11, 6, 12 };
        const int bpy[5] = { 6, 5, 8, 10, 11 };
        for (int i = 0; i < 5; i++) {
            for (int dy = 0; dy <= 1; dy++) for (int dx = 0; dx <= 1; dx++) {
                int px = bpx[i] + dx, py = bpy[i] + dy;
                if (px < 0 || px >= T || py < 0 || py >= T) continue;
                Color berry = (dx == 0 && dy == 0)
                    ? Natural(Color{ 208, 58, 62, 255 }, 0.20f, 0.10f)
                    : Natural(Color{ 150, 32, 40, 255 }, 0.18f, 0.20f);
                ImageDrawPixel(&img, c * T + px, r * T + py, berry);
            }
        }
        };

    auto ToadstoolTex = [&](int c, int r) {
        unsigned int s = 1980u;
        for (int y = 8; y < 14; y++) for (int x = 7; x < 10; x++)
            PutWarm(c * T + x, r * T + y, Natural(Color{ 214, 206, 190, 255 }, 0.20f, 0.14f),
                (Hash01(x, y, s) - 0.5f) * 6.0f, 255);
        for (int y = 3; y < 9; y++) for (int x = 3; x < 14; x++) {
            float dx = (float)(x - 8) / 5.2f, dy = (float)(y - 8) / 5.4f;
            if (dx * dx + dy * dy > 1.0f) continue;
            Color cap = MixC(Natural(Color{ 186, 74, 56, 255 }, 0.24f, 0.12f),
                Natural(Color{ 126, 44, 34, 255 }, 0.20f, 0.16f),
                Clamp((float)(8 - y) / 6.0f, 0.0f, 1.0f));
            if (Hash01(x, y, s + 6u) > 0.84f) cap = Color{ 232, 226, 212, 255 };
            PutWarm(c * T + x, r * T + y, cap, 0.0f, 255);
        }
        };

    auto CloverTex = [&](int c, int r) {
        unsigned int s = 1990u;
        for (int k = 0; k < 14; k++) {
            int cx2 = 2 + (int)(Hash01(k, 0, s) * 12.0f);
            int cy2 = 4 + (int)(Hash01(k, 1, s) * 9.0f);

            for (int y = cy2 + 2; y < T; y++) {
                if (cx2 < 0 || cx2 >= T) break;
                PutWarm(c * T + cx2, r * T + y,
                    Natural(Color{ 74, 104, 46, 255 }, 0.30f, 0.20f), 0.0f, 255);
            }

            for (int i = 0; i < 3; i++) {
                float a = (float)i * 2.094f + Hash01(k, 2, s) * 3.0f;
                int lx = cx2 + (int)(cosf(a) * 1.9f);
                int ly = cy2 + (int)(sinf(a) * 1.9f);
                for (int dy = 0; dy <= 1; dy++) for (int dx = 0; dx <= 1; dx++) {
                    int px = lx + dx, py = ly + dy;
                    if (px < 0 || px >= T || py < 0 || py >= T) continue;
                    Color g = (dx == 0 && dy == 0)
                        ? Natural(Color{ 104, 146, 60, 255 }, 0.32f, 0.08f)
                        : Natural(Color{ 72, 110, 48, 255 }, 0.30f, 0.16f);
                    PutWarm(c * T + px, r * T + py, g,
                        (Hash01(px, py, s + 5u) - 0.5f) * 6.0f, 255);
                }
            }
        }
        };

    auto DuneGrassTex = [&](int c, int r) {
        unsigned int s = 2000u;
        for (int k = 0; k < 13; k++) {
            float bx = 0.8f + (float)k * 1.25f;
            float lean = (Hash01(k, 3, s) - 0.5f) * 5.0f;
            int top = 2 + (int)(Hash01(k, 7, s) * 6.0f);
            int wide = (Hash01(k, 11, s) > 0.55f) ? 1 : 0;

            for (int y = top; y < T; y++) {
                float u = (float)(y - top) / (float)std::max(1, T - top);
                int px = (int)(bx + lean * (1.0f - u) * (1.0f - u));
                for (int w = 0; w <= wide; w++) {
                    int qx = px + w;
                    if (qx < 0 || qx >= T) continue;
                    Color g = MixC(Natural(Color{ 198, 186, 124, 255 }, 0.22f, 0.06f),
                        Natural(Color{ 124, 118, 72, 255 }, 0.20f, 0.18f), u);
                    if (w == 1) g = Tint(g, -0.22f);
                    PutWarm(c * T + qx, r * T + y, g,
                        (Hash01(qx, y, s) - 0.5f) * 7.0f, 255);
                }
            }
        }
        };

    CattailTex(14, 8);
    LilyTex(15, 8);
    CranberryTex(0, 9);
    ToadstoolTex(1, 9);
    CloverTex(2, 9);
    DuneGrassTex(3, 9);

    OutlineTile(14, 8, 0.34f);
    OutlineTile(15, 8, 0.30f);
    OutlineTile(0, 9, 0.34f);
    OutlineTile(1, 9, 0.30f);
    OutlineTile(2, 9, 0.34f);
    OutlineTile(3, 9, 0.32f);

    OutlineTile(0, 7, 0.34f);
    OutlineTile(1, 7, 0.32f);
    OutlineTile(2, 7, 0.32f);
    OutlineTile(3, 7, 0.30f);
    OutlineTile(4, 7, 0.34f);
    OutlineTile(5, 7, 0.30f);
    OutlineTile(6, 7, 0.32f);
    OutlineTile(8, 1, 0.30f);
    OutlineTile(9, 0, 0.34f);
    OutlineTile(10, 0, 0.32f);
    OutlineTile(11, 0, 0.26f);
    OutlineTile(9, 4, 0.32f);
    OutlineTile(10, 4, 0.32f);
    OutlineTile(11, 4, 0.30f);
    OutlineTile(12, 4, 0.28f);
    OutlineTile(13, 4, 0.30f);
    OutlineTile(14, 4, 0.28f);
    OutlineTile(15, 4, 0.30f);
    OutlineTile(0, 8, 0.28f);
    OutlineTile(1, 8, 0.28f);
    OutlineTile(2, 8, 0.28f);
    OutlineTile(3, 8, 0.28f);
    OutlineTile(4, 8, 0.28f);
    OutlineTile(5, 8, 0.28f);
    OutlineTile(6, 8, 0.28f);
    OutlineTile(7, 8, 0.28f);
    OutlineTile(8, 8, 0.28f);
    OutlineTile(9, 8, 0.30f);
    Striped(4, 9, { 92, 74, 52, 255 }, { 66, 52, 36, 255 }, 3);
    Leaves(5, 9, { 96, 122, 62, 255 });
    Striped(6, 9, { 74, 56, 38, 255 }, { 52, 40, 26, 255 }, 3);
    Leaves(7, 9, { 38, 74, 46, 255 });

    PitTex(10, 8, 0);
    PitTex(11, 8, 1);
    PitTex(12, 8, 2);
    AshTex(13, 8);

    Solid(0, 5, { 206,202,194,255 }, 16);
    Striped(1, 5, { 78,82,88,255 }, { 62,66,72,255 }, 2);
    Ore(2, 5, { 114,110,104,255 }, { 150,145,138,255 });
    Striped(3, 5, { 126,118,112,255 }, { 148,140,132,255 }, 5);
    Striped(4, 5, { 104,102,100,255 }, { 120,118,116,255 }, 3);
    Solid(5, 5, { 148,140,124,255 }, 14);
    Solid(6, 5, { 58,46,34,255 }, 16);
    Leaves(7, 5, { 46,80,40,255 });

    Ore(0, 6, { 110,108,105,255 }, { 30,29,32,255 });
    Ore(1, 6, { 110,108,105,255 }, { 138,82,58,255 });
    Ore(2, 6, { 110,108,105,255 }, { 186,158,66,255 });
    Ore(3, 6, { 110,108,105,255 }, { 150,104,62,255 });
    Ore(4, 6, { 52,52,58,255 }, { 96,150,168,255 });
    Striped(5, 6, { 94,62,38,255 }, { 70,44,26,255 }, 3);
    Striped(6, 6, { 104,76,46,255 }, { 80,55,32,255 }, 3);
    GrassSide(7, 6);

    GrassTuft(0, 7, { 58, 84, 40, 255 }, { 108, 138, 62, 255 });
    FlowerTex(1, 7, { 156, 52, 46, 255 }, { 208, 176, 88, 255 }, { 62, 90, 44, 255 });
    FlowerTex(2, 7, { 198, 178, 68, 255 }, { 224, 208, 128, 255 }, { 62, 90, 44, 255 });
    Solid(3, 7, { 58,96,52,255 }, 18);
    DeadBushTex(4, 7);
    MushroomTex(5, 7, { 132, 88, 52, 255 }, { 206, 196, 176, 255 });
    Sapling(6, 7);
    Solid(7, 7, { 186,120,42,255 }, 18);
    MushroomTex(8, 1, { 168, 56, 48, 255 }, { 214, 206, 190, 255 });

    PlanksTex(8, 0);
    FernTex(9, 0);
    ReedTex(10, 0);
    TorchTex(11, 0);
    MineralTex(12, 0, { 68, 66, 70, 255 }, { 118, 116, 122, 255 }, 0.35f);
    MineralTex(13, 0, { 116, 114, 110, 255 }, { 166, 160, 150, 255 }, 0.28f);
    MineralTex(14, 0, { 112, 116, 118, 255 }, { 158, 168, 172, 255 }, 0.26f);

    // Строка 10: подобранные камешки руд. Раньше золото и серебро делили
    // одну плитку, а медь, железо и алмаз брали цвет с чужой жилы —
    // у каждой руды теперь своя честная текстура
    Solid(0, 10, { 56, 58, 64, 255 }, 12);
    Solid(1, 10, { 44, 36, 32, 255 }, 10);
    Solid(2, 10, { 132, 96, 48, 255 }, 16);
    Solid(3, 10, { 158, 74, 40, 255 }, 16);
    Solid(4, 10, { 224, 182, 58, 255 }, 8);
    Solid(5, 10, { 196, 110, 62, 255 }, 10);
    Solid(6, 10, { 224, 232, 238, 255 }, 6);
    Solid(7, 10, { 202, 206, 212, 255 }, 8);
    Solid(8, 10, { 222, 220, 206, 255 }, 10);
    Solid(9, 10, { 60, 48, 42, 255 }, 12);
    Solid(10, 10, { 148, 100, 194, 255 }, 10);
    Solid(11, 10, { 136, 122, 94, 255 }, 14);
    Solid(12, 10, { 212, 138, 130, 255 }, 8);
    Solid(13, 10, { 42, 48, 36, 255 }, 10);
    Solid(14, 10, { 110, 114, 120, 255 }, 10);

    // Строка 11: жилы новых руд в материнской породе
    Ore(0, 11, { 176, 168, 150, 255 }, { 222, 220, 206, 255 });
    Ore(1, 11, { 176, 168, 150, 255 }, { 148, 100, 194, 255 });
    Ore(2, 11, { 150, 138, 112, 255 }, { 136, 122, 94, 255 });
    Ore(3, 11, { 150, 138, 112, 255 }, { 212, 138, 130, 255 });
    Ore(4, 11, { 150, 146, 140, 255 }, { 60, 48, 42, 255 });
    Ore(5, 11, { 96, 94, 92, 255 }, { 42, 48, 36, 255 });

    return img;
}

void Chunk::LoadAtlas() {
    if (atlasTexture.id != 0) return;
    Image img = BuildAtlasImage();
    atlasTexture = LoadTextureFromImage(img);
    SetTextureFilter(atlasTexture, TEXTURE_FILTER_POINT);
    UnloadImage(img);
}

Chunk::Chunk(int x, int z) : chunkX(x), chunkZ(z) {
    blocks = new BlockType[CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z]();
    lightMap = new unsigned char[CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z]();
    mesh = { 0 }; model = { 0 };
    meshTransp = { 0 }; modelTransp = { 0 };
    meshWater = { 0 }; modelWater = { 0 };
}

Chunk::~Chunk() {
    std::lock_guard<std::mutex> lock(chunkMutex);
    if (hasMesh) UnloadModel(model);
    if (hasMeshTransp) UnloadModel(modelTransp);
    if (hasMeshWater) UnloadModel(modelWater);
    delete[] blocks;
    delete[] lightMap;
}

void Chunk::GenerateTerrain(WorldGenerator& gen) {
    maxY = 0;
    for (int x = 0; x < CHUNK_SIZE_X; x++) {
        for (int z = 0; z < CHUNK_SIZE_Z; z++) {
            int wx = chunkX * CHUNK_SIZE_X + x;
            int wz = chunkZ * CHUNK_SIZE_Z + z;

            ColumnInfo col = gen.GetColumnInfo(wx, wz);

            int topY = std::max(col.height, col.river.fallTop);
            if (col.height < WorldGenerator::SEA_LEVEL)
                topY = std::max(topY, WorldGenerator::SEA_LEVEL);
            topY = std::max(topY, col.river.waterLevel);
            if (topY > maxY) maxY = topY;

            int ceiling = std::max(col.height + 24, WorldGenerator::SEA_LEVEL);
            ceiling = std::max(ceiling, col.river.fallTop);
            ceiling = std::max(ceiling, col.river.waterLevel);
            if (ceiling > CHUNK_SIZE_Y - 1) ceiling = CHUNK_SIZE_Y - 1;

            for (int y = 0; y <= ceiling; y++) {
                blocks[x + CHUNK_SIZE_X * (z + CHUNK_SIZE_Z * y)] =
                    gen.GetBlockFast(wx, y, wz, col);
            }
            for (int y = ceiling + 1; y < CHUNK_SIZE_Y; y++) {
                blocks[x + CHUNK_SIZE_X * (z + CHUNK_SIZE_Z * y)] = BlockType::Air;
            }
        }
    }
    maxY = std::min(maxY + 30, CHUNK_SIZE_Y - 1);
    lightPasses = 0;
    state = 1;
}

unsigned char Chunk::GetLight(int x, int y, int z) {
    int cx = std::clamp(x, 0, 15);
    int cy = std::clamp(y, 0, 255);
    int cz = std::clamp(z, 0, 15);
    return lightMap[cx + CHUNK_SIZE_X * (cz + CHUNK_SIZE_Z * cy)];
}

void Chunk::SetLight(int x, int y, int z, unsigned char level) {
    if (x >= 0 && x < CHUNK_SIZE_X && z >= 0 && z < CHUNK_SIZE_Z && y >= 0 && y < CHUNK_SIZE_Y) {
        lightMap[x + CHUNK_SIZE_X * (z + CHUNK_SIZE_Z * y)] = level;
    }
}

BlockType Chunk::GetBlockSafe(int lx, int ly, int lz, Chunk** neighbors) {
    if (ly < 0 || ly >= CHUNK_SIZE_Y) return BlockType::Air;

    int cx = 0, cz = 0;
    if (lx < 0) cx = -1; else if (lx >= CHUNK_SIZE_X) cx = 1;
    if (lz < 0) cz = -1; else if (lz >= CHUNK_SIZE_Z) cz = 1;

    if (cx == 0 && cz == 0) return blocks[lx + CHUNK_SIZE_X * (lz + CHUNK_SIZE_Z * ly)];

    Chunk* n = neighbors[(cx + 1) + (cz + 1) * 3];
    if (n) {
        int nx = (lx % CHUNK_SIZE_X + CHUNK_SIZE_X) % CHUNK_SIZE_X;
        int nz = (lz % CHUNK_SIZE_Z + CHUNK_SIZE_Z) % CHUNK_SIZE_Z;
        return n->GetBlock(nx, ly, nz);
    }
    return BlockType::Air;
}

unsigned char Chunk::GetLightSafe(int lx, int ly, int lz, Chunk** neighbors) {
    if (ly < 0 || ly >= CHUNK_SIZE_Y) return 15;

    int cx = 0, cz = 0;
    if (lx < 0) cx = -1; else if (lx >= CHUNK_SIZE_X) cx = 1;
    if (lz < 0) cz = -1; else if (lz >= CHUNK_SIZE_Z) cz = 1;

    if (cx == 0 && cz == 0) return lightMap[lx + CHUNK_SIZE_X * (lz + CHUNK_SIZE_Z * ly)];

    Chunk* n = neighbors[(cx + 1) + (cz + 1) * 3];
    if (n) {
        int nx = (lx % CHUNK_SIZE_X + CHUNK_SIZE_X) % CHUNK_SIZE_X;
        int nz = (lz % CHUNK_SIZE_Z + CHUNK_SIZE_Z) % CHUNK_SIZE_Z;
        return n->GetLight(nx, ly, nz);
    }
    return 15;
}

int Chunk::GetLightEmission(BlockType t) {
    switch (t) {
    case BlockType::Torch:      return 14;
    case BlockType::Lava:
    case BlockType::LavaSource: return 12;
    default:                    return 0;
    }
}

bool Chunk::IsLightPassable(BlockType t) {
    return t == BlockType::Air || t == BlockType::Glass || t == BlockType::OakSapling ||
        t == BlockType::Fern || t == BlockType::Reed || t == BlockType::Rose ||
        t == BlockType::BerryBush || t == BlockType::BerryBushRipe ||
        t == BlockType::Cattail || t == BlockType::LilyPad ||
            t == BlockType::CranberryBush || t == BlockType::Toadstool ||
            t == BlockType::Clover || t == BlockType::DuneGrass ||
        t == BlockType::Dandelion || t == BlockType::TallGrass || t == BlockType::Water ||
        t == BlockType::Torch ||
        t == BlockType::DeadBush || t == BlockType::BrownMushroom || t == BlockType::RedMushroom ||
        t == BlockType::OakLeaves || t == BlockType::SpruceLeaves || t == BlockType::BirchLeaves ||
        t == BlockType::AcaciaLeaves || t == BlockType::JungleLeaves;
}

bool Chunk::CalculateBasicSunlight(Chunk** neighbors) {
    const int N = CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z;
    std::vector<unsigned char> tSky(N, 0), tBlk(N, 0);
    std::queue<int> lightQ, blockQ;

    auto isLightPassable = [](BlockType t) { return IsLightPassable(t); };

    auto dimmer = [](BlockType t) -> unsigned char {
        if (t == BlockType::Water || t == BlockType::OakLeaves || t == BlockType::SpruceLeaves ||
            t == BlockType::BirchLeaves || t == BlockType::AcaciaLeaves || t == BlockType::JungleLeaves)
            return 2;
        return 1;
        };

    for (int x = 0; x < CHUNK_SIZE_X; x++) {
        for (int z = 0; z < CHUNK_SIZE_Z; z++) {
            unsigned char currentLight = 15;
            for (int y = CHUNK_SIZE_Y - 1; y >= 0; y--) {
                int idx = x + CHUNK_SIZE_X * (z + CHUNK_SIZE_Z * y);
                BlockType t = blocks[idx];

                if (!isLightPassable(t)) currentLight = 0;
                else if (dimmer(t) == 2) {
                    if (currentLight > 2) currentLight -= 2; else currentLight = 0;
                }

                tSky[idx] = currentLight;
                if (currentLight > 0 && y <= maxY) {
                    lightQ.push(x); lightQ.push(y); lightQ.push(z);
                }

                int emit = GetLightEmission(t);
                if (emit > 0 && y <= maxY) {
                    tBlk[idx] = (unsigned char)emit;
                    blockQ.push(x); blockQ.push(y); blockQ.push(z);
                }
            }
        }
    }

    Chunk* nXneg = neighbors[0 + 1 * 3]; Chunk* nXpos = neighbors[2 + 1 * 3];
    Chunk* nZneg = neighbors[1 + 0 * 3]; Chunk* nZpos = neighbors[1 + 2 * 3];

    auto borderLight = [&](Chunk* n, int nx, int ny, int nz, bool wantSky) -> int {
        if (!n || n->dirty) return wantSky ? 15 : 0;
        BlockType t = n->GetBlock(nx, ny, nz);
        if (!IsLightPassable(t)) return 0;
        unsigned char packed = n->GetLight(nx, ny, nz);
        return wantSky ? SkyLightOf(packed) : BlockLightOf(packed);
        };

    for (int y = 0; y <= maxY; y++) {
        for (int x = 0; x < CHUNK_SIZE_X; x++) {
            for (int z = 0; z < CHUNK_SIZE_Z; z++) {
                if (x != 0 && x != CHUNK_SIZE_X - 1 && z != 0 && z != CHUNK_SIZE_Z - 1) continue;
                int idx = x + CHUNK_SIZE_X * (z + CHUNK_SIZE_Z * y);
                BlockType t = blocks[idx];
                if (!isLightPassable(t)) continue;

                unsigned char drop = dimmer(t);

                for (int pass = 0; pass < 2; pass++) {
                    bool wantSky = (pass == 0);
                    int best = 0;
                    if (x == 0) best = std::max(best, borderLight(nXneg, 15, y, z, wantSky));
                    if (x == CHUNK_SIZE_X - 1) best = std::max(best, borderLight(nXpos, 0, y, z, wantSky));
                    if (z == 0) best = std::max(best, borderLight(nZneg, x, y, 15, wantSky));
                    if (z == CHUNK_SIZE_Z - 1) best = std::max(best, borderLight(nZpos, x, y, 0, wantSky));

                    if (best <= drop) continue;
                    unsigned char newL = (unsigned char)(best - drop);
                    std::vector<unsigned char>& target = wantSky ? tSky : tBlk;
                    if (newL > target[idx]) {
                        target[idx] = newL;
                        std::queue<int>& q = wantSky ? lightQ : blockQ;
                        q.push(x); q.push(y); q.push(z);
                    }
                }
            }
        }
    }

    const int dirs[6][3] = { {1,0,0}, {-1,0,0}, {0,1,0}, {0,-1,0}, {0,0,1}, {0,0,-1} };

    auto flood = [&](std::queue<int>& q, std::vector<unsigned char>& field) {
        while (!q.empty()) {
            int qx = q.front(); q.pop();
            int qy = q.front(); q.pop();
            int qz = q.front(); q.pop();
            unsigned char lightLvl = field[qx + CHUNK_SIZE_X * (qz + CHUNK_SIZE_Z * qy)];

            for (int i = 0; i < 6; i++) {
                int nx = qx + dirs[i][0], ny = qy + dirs[i][1], nz = qz + dirs[i][2];
                if (nx < 0 || nx >= CHUNK_SIZE_X || ny < 0 || ny > maxY || nz < 0 || nz >= CHUNK_SIZE_Z) continue;
                int nIdx = nx + CHUNK_SIZE_X * (nz + CHUNK_SIZE_Z * ny);
                BlockType nt = blocks[nIdx];
                if (!isLightPassable(nt)) continue;

                unsigned char drop = dimmer(nt);
                if (lightLvl <= drop) continue;
                unsigned char newLight = lightLvl - drop;
                if (newLight > field[nIdx]) {
                    field[nIdx] = newLight;
                    q.push(nx); q.push(ny); q.push(nz);
                }
            }
        }
        };

    flood(lightQ, tSky);
    flood(blockQ, tBlk);

    std::vector<unsigned char> packed(N);
    for (int i = 0; i < N; i++)
        packed[i] = (unsigned char)((tSky[i] << 4) | (tBlk[i] & 0x0F));

    bool lightChanged = false;
    {
        std::lock_guard<std::mutex> lock(chunkMutex);
        if (memcmp(lightMap, packed.data(), packed.size()) != 0) {
            lightChanged = true;
            memcpy(lightMap, packed.data(), packed.size());
        }
    }
    return lightChanged;
}

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
        type != BlockType::UraniumPebble;
}

void Chunk::GetTextureUV(BlockType type, int faceDir, float& u, float& v) {
    int col = 7, row = 7;

    switch (type) {
    case BlockType::Grass:
        if (faceDir == 0) { col = 0; row = 0; }
        else if (faceDir == 1) { col = 1; row = 0; }
        else { col = 7; row = 6; }
        break;

    case BlockType::Dirt:             col = 1; row = 0; break;
    case BlockType::CoarseDirt:       col = 7; row = 3; break;
    case BlockType::Mud:              col = 6; row = 2; break;
    case BlockType::Clay:             col = 7; row = 2; break;
    case BlockType::Sand:             col = 3; row = 0; break;
    case BlockType::RedSand:          col = 4; row = 0; break;
    case BlockType::Gravel:           col = 5; row = 0; break;
    case BlockType::Cobblestone:      col = 6; row = 0; break;
    case BlockType::Bedrock:          col = 7; row = 0; break;
    case BlockType::Stone:            col = 2; row = 0; break;
    case BlockType::Silt:             col = 5; row = 5; break;
    case BlockType::Peat:             col = 6; row = 5; break;

    case BlockType::Water:
    case BlockType::WaterSource:      col = 0; row = 2; break;
    case BlockType::Lava:
    case BlockType::LavaSource:       col = 1; row = 2; break;
    case BlockType::Glass:            col = 2; row = 2; break;

    case BlockType::Snow:             col = 4; row = 2; break;
    case BlockType::Ice:              col = 3; row = 2; break;
    case BlockType::PackedIce:        col = 5; row = 2; break;

    case BlockType::OakLog:
    case BlockType::BirchLog:
        col = (faceDir == 0 || faceDir == 1) ? 1 : 0; row = 1; break;
    case BlockType::SpruceLog:
        col = (faceDir == 0 || faceDir == 1) ? 1 : 3; row = 1; break;
    case BlockType::AcaciaLog:
        col = 5; row = 6; break;
    case BlockType::JungleLog:
        col = 6; row = 6; break;

    case BlockType::OakLeaves:        col = 2; row = 1; break;
    case BlockType::SpruceLeaves:     col = 4; row = 1; break;
    case BlockType::BirchLeaves:      col = 6; row = 1; break;
    case BlockType::AcaciaLeaves:     col = 7; row = 1; break;
    case BlockType::JungleLeaves:     col = 7; row = 5; break;

    case BlockType::Granite:          col = 0; row = 3; break;
    case BlockType::Basalt:           col = 1; row = 3; break;
    case BlockType::Gabbro:           col = 2; row = 3; break;
    case BlockType::Andesite:
    case BlockType::Diorite:          col = 3; row = 3; break;
    case BlockType::Rhyolite:         col = 4; row = 3; break;
    case BlockType::Dacite:           col = 5; row = 3; break;
    case BlockType::Marble:           col = 6; row = 3; break;

    case BlockType::Limestone:        col = 0; row = 4; break;
    case BlockType::Sandstone:        col = 1; row = 4; break;
    case BlockType::RedSandstone:     col = 2; row = 4; break;
    case BlockType::Shale:            col = 3; row = 4; break;
    case BlockType::Chalk:            col = 4; row = 4; break;
    case BlockType::Dolomite:         col = 5; row = 4; break;
    case BlockType::Conglomerate:     col = 6; row = 4; break;
    case BlockType::Chert:            col = 7; row = 4; break;
    case BlockType::Claystone:        col = 8; row = 4; break;

    case BlockType::Quartzite:        col = 0; row = 5; break;
    case BlockType::Slate:            col = 1; row = 5; break;
    case BlockType::Schist:           col = 2; row = 5; break;
    case BlockType::Gneiss:           col = 3; row = 5; break;
    case BlockType::Phyllite:         col = 4; row = 5; break;

    case BlockType::BituminousCoal:
    case BlockType::Lignite:          col = 0; row = 6; break;
    case BlockType::Hematite:
    case BlockType::Magnetite:
    case BlockType::Limonite:
    case BlockType::Galena:           col = 1; row = 6; break;
    case BlockType::NativeGold:
    case BlockType::NativeSilver:     col = 2; row = 6; break;
    case BlockType::NativeCopper:
    case BlockType::Malachite:
    case BlockType::Tetrahedrite:
    case BlockType::Cassiterite:
    case BlockType::Sphalerite:       col = 3; row = 6; break;
    case BlockType::Kimberlite:
    case BlockType::Bismuthinite:     col = 4; row = 6; break;

    case BlockType::TallGrass:        col = 0; row = 7; break;
    case BlockType::OakSapling:       col = 6; row = 7; break;
    case BlockType::OakPlanks:        col = 8; row = 0; break;
    case BlockType::Fern:             col = 9; row = 0; break;
    case BlockType::Reed:             col = 10; row = 0; break;
    case BlockType::Torch:            col = 11; row = 0; break;
    case BlockType::BerryBush:        col = 9; row = 4; break;
    case BlockType::BerryBushRipe:    col = 10; row = 4; break;
    case BlockType::Berries:          col = 11; row = 4; break;
    case BlockType::Acorn:            col = 12; row = 4; break;
    case BlockType::Grubs:            col = 13; row = 4; break;
    case BlockType::BirdEgg:          col = 14; row = 4; break;
    case BlockType::PlantFibre:       col = 15; row = 4; break;
    case BlockType::CopperNugget:     col = 0; row = 8; break;
    case BlockType::TinNugget:        col = 1; row = 8; break;
    case BlockType::SilverNugget:     col = 2; row = 8; break;
    case BlockType::GoldNugget:       col = 3; row = 8; break;
    case BlockType::ClayLump:         col = 4; row = 8; break;
    case BlockType::UnfiredCrucible:  col = 5; row = 8; break;
    case BlockType::Crucible:         col = 6; row = 8; break;
    case BlockType::UnfiredPickMould: col = 7; row = 8; break;
    case BlockType::PickMould:        col = 8; row = 8; break;
    case BlockType::Tongs:            col = 9; row = 8; break;
    case BlockType::FirePit:          col = 10; row = 8; break;
    case BlockType::FirePitLit:       col = 11; row = 8; break;
    case BlockType::FirePitEmbers:    col = 12; row = 8; break;
    case BlockType::Ash:              col = 13; row = 8; break;
    case BlockType::Cattail:          col = 14; row = 8; break;
    case BlockType::LilyPad:          col = 15; row = 8; break;
    case BlockType::CranberryBush:    col = 0; row = 9; break;
    case BlockType::Toadstool:        col = 1; row = 9; break;
    case BlockType::Clover:           col = 2; row = 9; break;
    case BlockType::DuneGrass:        col = 3; row = 9; break;
    case BlockType::WillowLog:        col = 4; row = 9; break;
    case BlockType::WillowLeaves:     col = 5; row = 9; break;
    case BlockType::FirLog:           col = 6; row = 9; break;
    case BlockType::FirLeaves:        col = 7; row = 9; break;
    case BlockType::Rose:             col = 1; row = 7; break;
    case BlockType::RedMushroom:      col = 8; row = 1; break;
    case BlockType::Dandelion:        col = 2; row = 7; break;
    case BlockType::Cactus:           col = 3; row = 7; break;
    case BlockType::DeadBush:         col = 4; row = 7; break;
    case BlockType::BrownMushroom:    col = 5; row = 7; break;
    case BlockType::Pumpkin:
    case BlockType::Melon:            col = 7; row = 7; break;

    case BlockType::StonePebble:      col = 2; row = 0; break;
    case BlockType::GranitePebble:     col = 0; row = 3; break;
    case BlockType::BasaltPebble:      col = 1; row = 3; break;
    case BlockType::LimestonePebble:   col = 0; row = 4; break;
    case BlockType::SandstonePebble:   col = 1; row = 4; break;
    case BlockType::CoalPebble:       col = 0; row = 6; break;

    // Свои плитки на строке 10 — раньше несколько руд ссылались на
    // чужие текстуры (золото выглядело как серебро) или на пустые
    // ячейки атласа (кремень, олово, цинк были невидимы)
    case BlockType::FlintPebble:      col = 0; row = 10; break;
    case BlockType::TinPebble:        col = 1; row = 10; break;
    case BlockType::ZincPebble:       col = 2; row = 10; break;
    case BlockType::IronPebble:       col = 3; row = 10; break;
    case BlockType::GoldPebble:       col = 4; row = 10; break;
    case BlockType::CopperPebble:     col = 5; row = 10; break;
    case BlockType::DiamondPebble:    col = 6; row = 10; break;
    case BlockType::SilverPebble:     col = 7; row = 10; break;
    case BlockType::BaritePebble:     col = 8; row = 10; break;
    case BlockType::TungstenPebble:   col = 9; row = 10; break;
    case BlockType::FluoritePebble:   col = 10; row = 10; break;
    case BlockType::PhosphoritePebble: col = 11; row = 10; break;
    case BlockType::PotashPebble:     col = 12; row = 10; break;
    case BlockType::UraniumPebble:    col = 13; row = 10; break;
    case BlockType::LeadPebble:       col = 14; row = 10; break;

    case BlockType::Barite:      col = 0; row = 11; break;
    case BlockType::Fluorite:    col = 1; row = 11; break;
    case BlockType::Phosphorite: col = 2; row = 11; break;
    case BlockType::Sylvite:     col = 3; row = 11; break;
    case BlockType::Wolframite:  col = 4; row = 11; break;
    case BlockType::Uraninite:   col = 5; row = 11; break;

    default: col = 7; row = 7; break;
    }

    u = (float)col * 0.0625f;
    v = (float)row * 0.0625f;
}

void Chunk::GetFaceUVRect(BlockType type, int faceDir, float& uMin, float& vMin, float& uMax, float& vMax) {
    if (IsWaterBlock(type)) {
        const float frameH = 1.0f / (float)WATER_FRAMES;
        const float inset = 0.5f / (float)(WATER_TILE * WATER_FRAMES);
        uMin = 0.0f;
        uMax = 1.0f;
        vMin = inset;
        vMax = frameH - inset;
        return;
    }

    float texU, texV;
    GetTextureUV(type, faceDir, texU, texV);
    const float step = 0.0625f;
    const float offset = 0.001f;
    uMin = texU + offset; uMax = texU + step - offset;
    vMin = texV + offset; vMax = texV + step - offset;
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
            t == BlockType::UraniumPebble;
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
            t == BlockType::UraniumPebble;
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

BlockType Chunk::GetBlock(int x, int y, int z) {
    if (x < 0 || x >= CHUNK_SIZE_X || z < 0 || z >= CHUNK_SIZE_Z || y < 0 || y >= CHUNK_SIZE_Y) return BlockType::Air;
    return blocks[x + CHUNK_SIZE_X * (z + CHUNK_SIZE_Z * y)];
}

void Chunk::SetBlock(int x, int y, int z, int type) {
    std::lock_guard<std::mutex> lock(chunkMutex);
    if (x >= 0 && x < CHUNK_SIZE_X && z >= 0 && z < CHUNK_SIZE_Z && y >= 0 && y < CHUNK_SIZE_Y) {
        blocks[x + CHUNK_SIZE_X * (z + CHUNK_SIZE_Z * y)] = (BlockType)type;
        isModified = true;
        lightPasses = 0;
        if (type != 0 && y > maxY - 5) {
            maxY = y + 10;
            if (maxY >= CHUNK_SIZE_Y) maxY = CHUNK_SIZE_Y - 1;
        }
    }
}

void Chunk::SetBlockRaw(int x, int y, int z, int type) {
    blocks[x + CHUNK_SIZE_X * (z + CHUNK_SIZE_Z * y)] = (BlockType)type;
    isModified = true;
    if (type != 0 && y > maxY - 5) {
        maxY = y + 10;
        if (maxY >= CHUNK_SIZE_Y) maxY = CHUNK_SIZE_Y - 1;
    }
}

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
