#include "Projectile.h"
#include "Effect/EffectManager.h"
#include "Graphics/Graphics.h"
#include "Debug/ShapeRenderer.h"
#include "Character/Player/Player.h"
#include "Character/Enemy/EnemyBoss.h"
#include "Graphics/Light.h"
#include <cmath>
#include <algorithm>


#include <stdlib.h>

namespace
{
    // 光柱のエフェクトかどうかを判別する文字列
    constexpr const char* LIGHT_PILLAR_EFFECT_KEYWORD = "LightPillar";

    // 光柱の足元から1フレームに撒く火花の数
    constexpr int SPARK_EMIT_COUNT = 5;

    // 火花の飛び散る速さ・上昇速度・色・大きさ・寿命
    constexpr float SPARK_MIN_SPEED = 2.0f;
    constexpr float SPARK_SPEED_RANGE = 3.0f;
    constexpr float SPARK_RISE_SPEED = 2.0f;
    constexpr DirectX::XMFLOAT4 SPARK_COLOR = { 1.0f, 0.9f, 0.4f, 1.0f };  // ゴールド系
    constexpr float SPARK_MIN_SIZE = 0.1f;
    constexpr float SPARK_SIZE_RANGE = 0.2f;
    constexpr float SPARK_MIN_LIFE = 0.2f;
    constexpr float SPARK_LIFE_RANGE = 0.3f;

    // 方向ベクトルをゼロとみなす長さ
    constexpr float DIRECTION_MIN_LENGTH = 0.0001f;

    // 当たり判定で相手側に足す半径と、相手の体の高さ
    constexpr float TARGET_HIT_RADIUS_MARGIN = 0.3f;
    constexpr float TARGET_BODY_HEIGHT = 1.0f;

    // 0.0～1.0 の乱数
    float Random01()
    {
        return static_cast<float>(rand()) / RAND_MAX;
    }
}

Projectile::Projectile(const ProjectileInfo& info, std::shared_ptr<Effect> effectResource)
    : info(info), position(info.spawnPosition), effect(effectResource), ageTimer(0.0f)
{
    // 速度ベクトルを計算 (方向 * 速さ)
    velocity.x = info.direction.x * info.speed;
    velocity.y = info.direction.y * info.speed;
    velocity.z = info.direction.z * info.speed;

    // 進行方向から回転を計算
    CalculateRotationFromVelocity(true);

    // エフェクト再生
    if (effect)
    {
        // EffectクラスのPlayメソッドを使用
        effectHandle = effect->Play(position, info.scale, rotation);
    }

    // ポイントライトを使う設定なら1つ確保する
    if (info.pointLightRange > 0.0f && info.pointLightColor.w > 0.0f)
    {
        pointLightIndex = LightManager::Instance().AllocatePointLight();
        UpdatePointLight();
    }
}

Projectile::~Projectile()
{
    // エフェクト停止
    if (effect && effectHandle != -1)
    {
        effect->Stop(effectHandle);
    }

    // 確保したポイントライトを返却する
    if (pointLightIndex >= 0)
    {
        LightManager::Instance().FreePointLight(pointLightIndex);
        pointLightIndex = -1;
    }
}

// ポイントライトの位置・明るさを更新する
void Projectile::UpdatePointLight()
{
    if (pointLightIndex < 0) return;

    // 消える間際に急に暗くならないよう、寿命の終盤で明るさを落とす
    constexpr float FADE_START_RATE = 0.7f;

    float lifeRate = (info.lifeTime > 0.0f)
        ? std::clamp(ageTimer / info.lifeTime, 0.0f, 1.0f)
        : 1.0f;

    float fade = (lifeRate < FADE_START_RATE)
        ? 1.0f
        : 1.0f - (lifeRate - FADE_START_RATE) / (1.0f - FADE_START_RATE);

    float intensity = info.pointLightColor.w * fade;
    DirectX::XMFLOAT4 color =
    {
        info.pointLightColor.x * intensity,
        info.pointLightColor.y * intensity,
        info.pointLightColor.z * intensity,
        1.0f
    };

    LightManager::Instance().UpdatePointLight(
        pointLightIndex, position, info.pointLightRange, color);
}

bool Projectile::Update(float elapsedTime)
{
    // 寿命チェック
    ageTimer += elapsedTime;
    if (ageTimer >= info.lifeTime)
    {
        return false; // 消滅
    }

    // 動きのタイプによって分岐
    switch (info.moveType)
    {
    case MovementType::Stationary:
        // 動かない (エフェクトの更新のみ)
        break;

    case MovementType::Linear:
        position.x += velocity.x * elapsedTime;
        position.y += velocity.y * elapsedTime;
        position.z += velocity.z * elapsedTime;
        break;

    case MovementType::Spiral:
        UpdateSpiral(elapsedTime);
        break;
    }

    // エフェクトの位置・回転同期
    // ここでEffectクラス経由で毎フレーム更新
    if (effect && effectHandle != -1)
    {
        // 位置更新
        effect->SetPosition(effectHandle, position);

        // 回転更新 (もし誘導弾などで向きが変わる場合は重要)
        effect->SetRotation(effectHandle, rotation);
    }

    // ポイントライトもエフェクトに追従させる
    UpdatePointLight();

    if (std::string(info.effectPath).find(LIGHT_PILLAR_EFFECT_KEYWORD) != std::string::npos)
    {
        for (int i = 0; i < SPARK_EMIT_COUNT; ++i)
        {
            // ランダムな角度(0 ～ 2PI)
            float angle = Random01() * DirectX::XM_2PI;

            // ランダムな半径 (中心から info.radius の範囲)
            float r = Random01() * info.radius;

            DirectX::XMFLOAT3 emitPos = {
                position.x + cosf(angle) * r,
                position.y,
                position.z + sinf(angle) * r
            };

            // 飛び散る速度
            // XZ平面は角度(angle)の方向へ、Yは少し上へ向かわせる
            float speed = SPARK_MIN_SPEED + Random01() * SPARK_SPEED_RANGE; // 飛び散る勢い
            float vx = cosf(angle) * speed;
            float vy = Random01() * SPARK_RISE_SPEED; // 少し上へ
            float vz = sinf(angle) * speed;
            DirectX::XMFLOAT3 velocity = { vx, vy, vz };

            // サイズと寿命（短く弾けて消える）
            float size = SPARK_MIN_SIZE + Random01() * SPARK_SIZE_RANGE;
            float lifeTime = SPARK_MIN_LIFE + Random01() * SPARK_LIFE_RANGE;

            EffectManager::Instance().EmitGpuParticle(emitPos, velocity, SPARK_COLOR, size, lifeTime, GpuParticleBehavior::Float);
        }
    }

    return true; // 生存
}

// 螺旋・拡散移動の計算
void Projectile::UpdateSpiral(float elapsedTime)
{
    // 角度更新
    info.currentAngle += info.angularSpeed * elapsedTime;
    // 半径更新 (外へ広がる)
    info.currentRadius += info.radialSpeed * elapsedTime;

    // 新しい位置を計算 (XZ平面での回転)
    float newX = info.centerPosition.x + cosf(info.currentAngle) * info.currentRadius;
    float newZ = info.centerPosition.z + sinf(info.currentAngle) * info.currentRadius;

    // 速度ベクトルを逆算（当たり判定や回転計算のため）
    velocity.x = (newX - position.x) / elapsedTime;
    velocity.z = (newZ - position.z) / elapsedTime;
    velocity.y = 0.0f; // 必要なら高さ変動を入れる

    position.x = newX;
    position.z = newZ;

    CalculateRotationFromVelocity(true);
}

// ターゲットに向かって発射 (ベクトル計算)
void Projectile::FireAt(const DirectX::XMFLOAT3& targetPos, float newSpeed, bool bakeY)
{
    // 停止状態などを解除
    info.moveType = MovementType::Linear;
    info.speed = newSpeed;

    // ベクトル計算
    float dx = targetPos.x - position.x;
    float dy = targetPos.y - position.y;
    float dz = targetPos.z - position.z;

    float len = sqrtf(dx * dx + dy * dy + dz * dz);
    if (len > DIRECTION_MIN_LENGTH)
    {
        info.direction = { dx / len, dy / len, dz / len };
    }
    else
    {
        info.direction = { 0.0f, 0.0f, 1.0f };
    }

    // 速度更新
    velocity.x = info.direction.x * info.speed;
    if (!bakeY)
        velocity.y = info.direction.y * info.speed;
    velocity.z = info.direction.z * info.speed;

    CalculateRotationFromVelocity(bakeY);
}

void Projectile::StartSpiral(float angularSpeed, float radialSpeed)
{
    info.moveType = MovementType::Spiral;
    info.angularSpeed = angularSpeed;
    info.radialSpeed = radialSpeed;
    // 必要なら lifeTime を延長する処理を入れても良い
}

void Projectile::CalculateRotationFromVelocity(bool bakeY)
{
    // Y軸回転 (Yaw)
    rotation.y = atan2f(velocity.x, velocity.z);

    // X軸回転 (Pitch) - 上下方向の傾き
    if (!bakeY)
	{
		float horizontalLen = sqrtf(velocity.x * velocity.x + velocity.z * velocity.z);
    	rotation.x = -atan2f(velocity.y, horizontalLen);
	}

    // Z軸回転 (Roll)
    rotation.z = 0.0f;
}

bool Projectile::OnHit(Character* target)
{
    if (!target || target == info.owner || target->IsDeathFlag()) return false;

    // ターゲットの情報
    DirectX::XMFLOAT3 targetPosition = target->GetPosition();
    // 当たり判定半径 (自身の半径 + 相手側の余白)
    float hitRadius = info.radius + TARGET_HIT_RADIUS_MARGIN;
    float hitHeight = targetPosition.y + TARGET_BODY_HEIGHT;
    bool isHit = false;

    // ---------------------------------------------------------
    // 形状による判定分岐
    // ---------------------------------------------------------
    if (info.shape == ProjectileShape::Sphere)
    {
        // --- 球体判定 (Sphere) ---
        // 単純な3次元距離チェック
        float dx = position.x - targetPosition.x;
        float dy = position.y - (targetPosition.y + hitHeight * 0.5f); // 相手の中心付近を狙う補正
        float dz = position.z - targetPosition.z;
        float distSq = dx * dx + dy * dy + dz * dz;

        float combinedRadius = info.radius + hitRadius;
        if (distSq < combinedRadius * combinedRadius)
        {
            isHit = true;
        }
    }
    else if (info.shape == ProjectileShape::Cylinder)
    {
        // --- 円柱判定 (Cylinder) ---
        // 1. 水平方向(XZ)の距離チェック
        float dx = position.x - targetPosition.x;
        float dz = position.z - targetPosition.z;
        float distSqXZ = dx * dx + dz * dz;
        float combinedRadius = info.radius + hitRadius;

        if (distSqXZ < combinedRadius * combinedRadius)
        {
            // 2. 垂直方向(Y)の高さチェック
            // 弾の底面～上面 と キャラクターの底面～上面 が重なっているか
            float projBottom = position.y - (info.height * 0.5f); // 弾の下端
            float projTop = position.y + (info.height * 0.5f); // 弾の上端

            float charBottom = targetPosition.y;              // キャラの足元
            float charTop = targetPosition.y + hitHeight;    // キャラの頭頂

            // 「弾がキャラより完全に上にいる」または「弾がキャラより完全に下にいる」以外ならヒット
            if (projBottom < charTop && projTop > charBottom)
            {
                isHit = true;
            }
        }
    }

    if (isHit)
    {
        if (target == static_cast<Character*>(&Player::Instance()))
        {
	        bool isPlayerRolling = Player::Instance().IsRolling();
        	if (isPlayerRolling || Player::Instance().GetInvincibleTimer() > 0.0f)
        	{
        		return false;
        	}
        	else if (Player::Instance().IsGuard())
        	{
                Player::Instance().ChangeState(PlayerStateId::GuardHit);
                return false;
        	}
            else
            {
                Player::Instance().SetDamageDirection(EnemyBoss::Instance().GetPosition());
	            Player::Instance().SetLightDamage(true);
            }
        }
        else if (target == static_cast<Character*>(&EnemyBoss::Instance()))
        {
	        EnemyBoss::Instance().SetDamage(true);
        }
        target->ApplyDamage(info.damage, info.invincibleTime);
        return true;
    }

	return false;
}

void Projectile::DrawDebugPrimitive()
{
    ShapeRenderer* shapeRenderer = Graphics::Instance().GetShapeRenderer();
    if (info.shape == ProjectileShape::Sphere)
        shapeRenderer->DrawSphere(position, info.radius, DirectX::XMFLOAT4(1, 0, 0, 1));
    else if (info.shape == ProjectileShape::Cylinder)
        shapeRenderer->DrawCylinder(position, info.radius, info.height, DirectX::XMFLOAT4(1, 0, 0, 1));
}
