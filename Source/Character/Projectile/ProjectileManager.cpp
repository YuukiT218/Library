#include "ProjectileManager.h"
#include "Character/Player/Player.h"


#include <stdlib.h>



Projectile* ProjectileManager::Launch(ProjectileInfo info)
{
    std::shared_ptr<Effect> effect = GetEffect(info.effectPath);

    // 生成
    auto newProjectile = std::make_unique<Projectile>(info, effect);
    Projectile* ptr = newProjectile.get(); // 生ポインタを取得

    projectiles.emplace_back(std::move(newProjectile));

    return ptr; // アクション側で制御するために返す
}

void ProjectileManager::Update(float elapsedTime)
{
    // プレイヤーとの当たり判定用
    Player* player = &Player::Instance();

    auto it = projectiles.begin();
    while (it != projectiles.end())
    {
        bool isAlive = (*it)->Update(elapsedTime);

        // ヒット判定
        if (isAlive && player)
        {
            if ((*it)->OnHit(player))
            {
                //isAlive = false; // ヒットしたら消滅
            }
        }

        if (!isAlive)
        {
            it = projectiles.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void ProjectileManager::Clear()
{
    projectiles.clear();
    effectCache.clear();
}

void ProjectileManager::DrawDebugPrimitive()
{
    for (auto& projectile : projectiles)
    {
    	projectile->DrawDebugPrimitive();
    }
}

std::shared_ptr<Effect> ProjectileManager::GetEffect(const char* filename)
{
    if (!filename) return nullptr;
    std::string path = filename;

    // キャッシュ確認
    auto it = effectCache.find(path);
    if (it != effectCache.end())
    {
        return it->second;
    }

    // 新規ロード
    auto newEffect = std::make_shared<Effect>(filename);
    effectCache[path] = newEffect;
    return newEffect;
}

// --- プリセット実装 ---
ProjectileInfo ProjectileManager::GetSlashWaveInfo()
{
    ProjectileInfo info;
    info.effectPath = "Data/Effect/SlashWave.efkefc";
    info.speed = 30.0f;
    info.lifeTime = 2.0f;
    info.radius = 1.0f;
    info.scale = 0.5f;
    info.damage = 3;
    info.invincibleTime = 0.25f;
    // 薄い緑のポイントライトを追従させる
    info.pointLightColor = { 0.55f, 1.0f, 0.65f, 0.25f };
    info.pointLightRange = 5.0f;
    return info;
}

ProjectileInfo ProjectileManager::GetLightPillarInfo()
{
    ProjectileInfo info;
    info.effectPath = "Data/Effect/LightPillar.efkefc";
    info.speed = 25.0f;
    info.lifeTime = 2.0f;
    info.radius = 1.0f;
    info.height = 30.0f;
    info.scale = 0.7f;
    info.damage = 5;
    info.invincibleTime = 0.5f;
    info.shape = ProjectileShape::Cylinder;
    // 薄い黄色のポイントライトを追従させる
    info.pointLightColor = { 1.0f, 0.97f, 0.6f, 0.3f };
    info.pointLightRange = 7.0f;
    return info;
}
