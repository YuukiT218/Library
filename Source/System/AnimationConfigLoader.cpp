#include "AnimationConfigLoader.h"
#include <fstream>

std::unordered_map<std::string, std::vector<AnimationConfig>> AnimationConfigLoader::configMap;

#include <filesystem>
namespace fs = std::filesystem;

void AnimationConfigLoader::LoadAllConfigs()
{
    configMap.clear();

    const std::string folderPath = "Data/Json";
    const std::string prefix = "AnimationConfigs_";

    for (const auto& entry : fs::directory_iterator(folderPath)) {
        const auto& path = entry.path();
        const std::string filename = path.filename().string();

        if (path.extension() == ".json" &&
            filename.find(prefix) == 0) // 先頭が "AnimationConfigs_" かどうか
        {
            std::ifstream file(path);
            if (!file) continue;

            json root;
            file >> root;

            for (auto& [name, configsJson] : root.items()) {
                std::vector<AnimationConfig> configs;
                for (const auto& configJson : configsJson) {
                    configs.push_back(configJson.get<AnimationConfig>());
                }
                configMap[name] = std::move(configs);
            }
        }
    }
}

AnimationConfig* AnimationConfigLoader::GetConfig(const std::string& characterName, int animationIndex)
{
    auto it = configMap.find(characterName);
    if (it != configMap.end()) {
        for (auto& config : it->second) {
            if (config.animationIndex == animationIndex) {
                return &config;
            }
        }
    }
    return nullptr;
}
