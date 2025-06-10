#include "Graphics/Graphics.h"
#include "ResourceManager.h"

// モデルリソース読み込み
std::shared_ptr<ModelResource> ResourceManager::LoadModelResource(const char* filename, float scale)
{
    // モデルを検索
    int result;
    for (auto& model : models)
    {
        result = strcmp(model.first.c_str(), filename);
        if (result == 0)
        {
            if (model.second.lock())
            {
                return model.second.lock();
            }
            else
            {
                models.erase(model.first);
                break;
            }
        }
    }

    // 新規モデルリソース作成＆読み込み
    std::shared_ptr<ModelResource> model;
    model = std::make_shared<ModelResource>();
    model->Load(Graphics::Instance().GetDevice(), filename, 60);

    // マップに登録
    models.insert(std::make_pair(std::string(filename), std::weak_ptr<ModelResource>(model)));

    return model;
}

void ResourceManager::Clear()
{
    models.clear();
}