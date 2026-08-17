#pragma once
#include <DirectXMath.h>
#include <Effekseer.h>
#include <memory>
#include "Character/Character.h"
#include "Effect/Effect.h"

enum class ProjectileShape
{
	Sphere,
    Cylinder
};

// 動きのタイプ
enum class MovementType
{
	Linear,        // 直進
    Stationary,    // 停止
    Spiral         // 回転拡散
};

// 発射物の設定情報
struct ProjectileInfo
{
    // 基本設定
    const char* effectPath;             // エフェクトファイルのパス
    float speed = 10.0f;                // 速度
    float lifeTime = 2.0f;              // 生存時間
    float scale = 1.0f;                 // エフェクトのスケール
    int damage = 10;                    // ダメージ量
    float invincibleTime = 0.5f;

    // 当たり判定設定
    ProjectileShape shape = ProjectileShape::Sphere; // 当たり判定の形状 
    float radius = 0.5f;                             // 当たり判定半径
    float height = 1.0f;                             // 高さ(円柱のみ使用)

    // 動作パラメーター
    DirectX::XMFLOAT3 spawnPosition = { 0,0,0 }; // 出現位置
    DirectX::XMFLOAT3 direction = { 0,0,1 };     // 進行方向（正規化ベクトル）
    Character* owner = nullptr;                         // 発射したキャラクター
    MovementType moveType = MovementType::Linear;

    // 発射物に追従させるポイントライト
    // rgbが光の色、wが明るさ。範囲か明るさが0なら光らせない
    DirectX::XMFLOAT4 pointLightColor = { 0.0f, 0.0f, 0.0f, 0.0f };
    float pointLightRange = 0.0f;

    // Spiral用パラメーター
    DirectX::XMFLOAT3 centerPosition = { 0,0,0 }; // 回転中心
    float currentAngle = 0.0f;                           // 現在の角度(ラジアン)
    float angularSpeed = 0.0f;                           // 回転速度
    float radialSpeed = 0.0f;                            // 外側に広がる速度
    float currentRadius = 0.0f;                          // 現在の半径
};

class Projectile
{
public:
    Projectile(const ProjectileInfo& info, std::shared_ptr<Effect> effectResource);
    ~Projectile();

    // エフェクトのハンドルとポイントライトを所有しているためコピーを禁止する
    Projectile(const Projectile&) = delete;
    Projectile& operator=(const Projectile&) = delete;

    // 更新処理 (falseを返すと消滅)
    bool Update(float elapsedTime);

    // 当たり判定処理
    bool OnHit(Character* target);

    // 当たり判定描画
    void DrawDebugPrimitive();

    // 指定座標に向かって発射 (ベクトル計算をここで行う)
    void FireAt(const DirectX::XMFLOAT3& targetPos, float newSpeed, bool bakeY);

    // 螺旋拡散モードへ切り替え
    void StartSpiral(float angularSpd, float radialSpd);

    // 生存確認用
    bool IsActive() const { return ageTimer < info.lifeTime; }

    // 座標を強制的に設定する（追従処理用）
    void SetPosition(const DirectX::XMFLOAT3& pos) { position = pos; }

    // 
    const ProjectileInfo& GetInfo() const { return info; }
private:
    ProjectileInfo info;
    DirectX::XMFLOAT3 position;     // 現在位置
    DirectX::XMFLOAT3 velocity;     // 速度ベクトル
    DirectX::XMFLOAT3 rotation;     // 現在の回転（ラジアン）
    float ageTimer = 0.0f;          // 経過時間

    // エフェクト関連
    std::shared_ptr<Effect> effect; // エフェクトリソース
    Effekseer::Handle effectHandle = -1; // 再生ハンドル

    // 確保したポイントライトの番号（-1なら未使用）
    int pointLightIndex = -1;

    // ポイントライトの位置・明るさを更新する
    void UpdatePointLight();

    // 内部関数：方向ベクトルから回転角度を計算
    void CalculateRotationFromVelocity(bool bakeY);

    // 螺旋移動の更新処理
    void UpdateSpiral(float elapsedTime);
};