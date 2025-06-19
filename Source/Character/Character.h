#pragma once

#include <DirectXMath.h>
#include <memory>
#include <vector>
#include "Model/Model.h"
#include "Camera/Camera.h"
#include "Input/Input.h"
//#include "Effect/Effect.h"

// ノードの当たり判定構造体
struct NodeHitSphere
{
	const char* nodeName;
	float radius;
};

// キャラクター
class Character
{
public:
	Character() {}
	virtual ~Character() {}

	// 行列更新処理
	void UpdateTransform();

	// 行列更新処理
	void UpdateTransform(DirectX::XMFLOAT3 scale, DirectX::XMFLOAT3 angle, DirectX::XMFLOAT3 position, DirectX::XMFLOAT4X4* transform);

	// 位置取得
	const DirectX::XMFLOAT3& GetPosition() const { return position; }

	// 位置取得
	void SetPosition(const DirectX::XMFLOAT3& position) { this->position = position; }

	// 回転取得
	const DirectX::XMFLOAT3& GetAngle() const { return angle; }

	// 回転設定
	void SetAngle(const DirectX::XMFLOAT3& angle) { this->angle = angle; }

	// スケール取得
	const DirectX::XMFLOAT3& GetScale() const { return scale; }

	// スケール設定
	void SetScale(const DirectX::XMFLOAT3& scale) { this->scale = scale; }

	// 速力取得
	const DirectX::XMFLOAT3& GetVelocity() const { return velocity; }

	// 速力設定
	void SetVelocity(const DirectX::XMFLOAT3& velocity) { this->velocity = velocity; }

	// 半径取得
	float GetRadius() const { return radius; }

	// 入力値からワールドベクトルを取得
	DirectX::XMFLOAT3 ComputeWorldVec(const Camera& camera, float axisX, float axisY) const;

	// キャラクター前方向計算
	DirectX::XMFLOAT3 CharacterForward(DirectX::XMFLOAT3 angle);

	// キャラクター後ろ方向計算
	DirectX::XMFLOAT3 CharacterBack(DirectX::XMFLOAT3 angle);

	// キャラクター左方向計算
	DirectX::XMFLOAT3 CharacterLeft(DirectX::XMFLOAT3 angle);

	// キャラクター右方向計算
	DirectX::XMFLOAT3 CharacterRight(DirectX::XMFLOAT3 angle);

	// 地面に接しているか
	bool IsGround() const { return isGround; }

	// 高さ取得
	float GetHeight() const { return height; }

	// 行列取得
	DirectX::XMFLOAT4X4 GetTransform() const { return transform; }

	// 行列設定
	void SetTransform(DirectX::XMFLOAT4X4 transform) { this->transform = transform; }

	// 健康状態を取得
	int GetHealth() const { return health; }

	// 健康状態設定
	void SetHealth(float num) { health = num; }

	// 最大健康状態を取得
	int GetMaxHealth() const { return  maxHealth; }

	// 生存状態取得
	bool IsDeathFlag() { return deathFlag; }

	// 生存状態設定
	void SetDeathFlag(bool flag) { deathFlag = flag; }

	// ダメージを与える
	bool ApplyDamage(int damage, float invicibleTime, bool isState = true, DirectX::XMFLOAT3 HitPosition = {});

	// 衝撃を与える
	void AddImpulse(const DirectX::XMFLOAT3& impulse);

	virtual Model* GetModel() { return model.get(); }

	std::unordered_map<int, float> animSpeed;

	void initAnimSpeed();
protected:
	// 移動処理
	void Move(float vx, float vz, float speed);

	// 旋回処理
	void Turn(float elapsedTime, float vx, float vz, float speed);

	// ジャンプ処理
	void Jump(float speed);

	// 速力更新処理
	void UpdateVelocity(float elapsedTime);

	// 速力更新処理
	void UpdateVelocity(DirectX::XMFLOAT3* position, DirectX::XMFLOAT3* angle, DirectX::XMFLOAT3* velocity, float elapsedTime);

	// 着地した時に呼ばれる
	virtual void OnLanding() {}

	// ダメージを受けた時に呼ばれる
	virtual void OnDamaged() {}

	// 死亡した時に呼ばれる
	virtual void OnDead() {}

	// 無敵時間更新
	void UpdateInvincibleTimer(float elapsedTime);

	// 全身に当たり判定を付与する
	void AddCollisionSpheres(std::shared_ptr<Model> model, std::vector<NodeHitSphere> nodeHitSpheres);

	// エリア外に行けないようにする
	void KeepAreaLimit(DirectX::XMFLOAT3& position);

private:
	// 垂直速力更新処理
	void UpdateVerticalVelocity(float elapsedTime);

	// 垂直速力更新処理
	void UpdateVerticalVelocity(DirectX::XMFLOAT3* velocity, float elapsedTime);

	// 垂直移動更新処理
	void UpdateVerticalMove(float elapsedTime);

	// 垂直移動更新処理
	void UpdateVerticalMove(DirectX::XMFLOAT3* position, DirectX::XMFLOAT3* angle, DirectX::XMFLOAT3* velocity, float elapsedTime);

	// 水平速力更新処理
	void UpdateHorizontalVelocity(float elapsedTime);

	// 水平移動更新処理
	void UpdateHorizontalMove(float elapsedTime);

protected:
	DirectX::XMFLOAT3 position = { 0, 0, 0 };
	DirectX::XMFLOAT3 angle = { 0, 0, 0 };
	DirectX::XMFLOAT3 scale = { 1, 1, 1 };
	DirectX::XMFLOAT4X4 transform = {
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1
	};

	float radius = 0.5f;
	float gravity = -1.0f;
	DirectX::XMFLOAT3 velocity = { 0, 0, 0 };
	bool isGround = true;
	float height = 2.0f;
	int health = 1000000;
	int maxHealth = 1000000;
	bool deathFlag = false;	// キャラクターが死亡したらtrueになる変数
	float invincibleTimer = 1.0f;
	float friction = 0.5f;

	// 慣性移動
	float acceleration = 1.0f;
	float maxMoveSpeed = 5.0f;
	float moveVecX = 0.0f;
	float moveVecZ = 0.0f;

	// 空中制御
	float airControl = 0.3f;

	float stepOffset = 1.0f;
	float slopeRate = 1.0f;

	// キャラクターの行ける範囲を制限する変数
	DirectX::XMFLOAT3 areaCenter = { 0.0f, -2.7f, 0.0f };
	float areaSize = 10.0f;

	std::shared_ptr<Model> model;
};