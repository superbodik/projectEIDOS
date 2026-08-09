#include "../Chunk.h"

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

    case BlockType::StonePickHead:   col = 6; row = 11; break;
    case BlockType::StoneAxeHead:    col = 7; row = 11; break;
    case BlockType::StoneShovelHead: col = 8; row = 11; break;
    case BlockType::StoneHoeHead:    col = 9; row = 11; break;
    case BlockType::StoneKnifeBlade: col = 10; row = 11; break;

    case BlockType::StonePickaxe: col = 11; row = 11; break;
    case BlockType::StoneAxe:     col = 12; row = 11; break;
    case BlockType::StoneShovel:  col = 13; row = 11; break;
    case BlockType::StoneHoe:     col = 14; row = 11; break;
    case BlockType::StoneKnife:   col = 15; row = 11; break;

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

