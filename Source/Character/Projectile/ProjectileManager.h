#pragma once
#include <vector>
#include <memory>
#include <map>
#include <string>
#include "Projectile.h"

class ProjectileManager
{
public:
    static ProjectileManager& Instance()
    {
        static ProjectileManager instance;
        return instance;
    }
    // コピー禁止
    ProjectileManager(const ProjectileManager&) = delete;
    void operator=(const ProjectileManager&) = delete;

    // 発射 (プリセット情報を受け取る)
    Projectile* Launch(ProjectileInfo info);

    // 更新
    void Update(float elapsedTime);

    // クリア
    void Clear();

    void DrawDebugPrimitive();

    // --- プリセット取得関数 ---
    static ProjectileInfo GetSlashWaveInfo(); // 斬撃波
    static ProjectileInfo GetLightPillarInfo();  // 光柱

private:
    ProjectileManager() {}
    ~ProjectileManager() {}

    // エフェクト読み込み・キャッシュ取得
    std::shared_ptr<Effect> GetEffect(const char* filename);

    std::vector<std::unique_ptr<Projectile>> projectiles;
    std::map<std::string, std::shared_ptr<Effect>> effectCache;
};