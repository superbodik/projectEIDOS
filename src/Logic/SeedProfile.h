#pragma once
#include "EidosTypes.h"
#include <vector>

struct SeedProfile {
    float rawStats[(int)StatType::_COUNT];

    SeedProfile() {
        for (int i = 0; i < (int)StatType::_COUNT; ++i) rawStats[i] = 10.0f;
    }

    void set(StatType type, float val) {
        rawStats[(int)type] = val;
    }

    float get(StatType type) const {
        return rawStats[(int)type];
    }
};