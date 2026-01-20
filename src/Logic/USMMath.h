#pragma once
#include "EidosTypes.h"
#include <cmath>
#include <algorithm> 
class USMMath {
public:
    static float Convert(StatType type, float X, float W) {
        float safeX = std::max(0.1f, X);
        float safeW = std::max(0.1f, W);

        switch (type) {
        case StatType::Might:     return X * (1.0f + W / 100.0f);
        case StatType::Cunning:   return X / (X + W);
        case StatType::Endurance: return X * (W / 50.0f);
        case StatType::Focus:     return (X * 5.0f) + (W * 2.0f);
        case StatType::Social:    return (X / safeW) * 100.0f;
        case StatType::Intellect: return 1.0f / (1.0f + safeW / safeX);
        case StatType::Luck:      return X * (1.0f / (safeW * 10.0f));
        case StatType::Mastery:   return (X * 0.05f) + (W / 10.0f);
        default: return 0.0f;
        }
    }
};