#pragma once 

#pragma once

enum class StatType {
	Might,     
	Cunning,  
	Endurance,  
	Focus,      
	Social,     
	Intellect,  
	Luck,       
	Mastery,    
	_COUNT      
};


inline const char* StatsType(StatType stat) {
	switch (stat) {
	case StatType::Might: return "Might";
	case StatType::Cunning: return "Cunning";
	case StatType::Endurance: return "Endurance";
	case StatType::Social: return "Endurance";
	case StatType::Focus: return "Focus";
	case StatType::Intellect: return "Intellect";
	case StatType::Luck: return "Luck";
	case StatType::Mastery: return "Mastery";
	default: return "Unknown";
	}
};
