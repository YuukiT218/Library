#pragma once
#include "Model/Model.h"

class AnimationConfigLoader {
public:
    static void LoadAllConfigs(); // 最初に全データを読み込む
    static AnimationConfig* GetConfig(const std::string& characterName, int animationIndex);

private:
    static std::unordered_map<std::string, std::vector<AnimationConfig>> configMap;
};
