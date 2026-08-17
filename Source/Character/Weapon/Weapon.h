#pragma once

#include "Character/Character.h"
#include "Graphics/Graphics.h"
#include "Graphics/Shader.h"
#include "System/Audio/AudioSource.h"

class Weapon
{
public:
	Weapon() {}
	virtual ~Weapon();

	// 更新処理
	virtual void Update(float elapsedTime) {}

	// 描画処理
	virtual void Render(const RenderContext& rc, ShaderId shaderId) {}

	// モデル用ゲッター
	std::shared_ptr<Model> GetModel() const { return model; }

	// アタッチ
	void Attach(std::string nodeName, Model* character);

	// トレイル用アップデート
	void TrailUpdate(float elapsedTime);

	// ノードとエネミーの衝突処理
	void CollisionNodeVsEnemies(float nodeRadius, int attackDamage, float invincibleTime, float leftVibrate, float rightVibrate, float hitStopTime, float hitStopSpeed);
	void CollisionNodeVsCharacter(float nodeRadius, AnimationConfig* config, AnimationAttribute* activeAttribute, Character* character);

	// アニメーションの攻撃当たり判定をつける
	void AttackAnimationCollision(Model* character, float animTimeMin, float animTimeMax, int attackDamage, float invincibleTime, float leftVibrate, float rightVibrate, float hitStopTime, float hitStopSpeed);
	void AttackAnimationCollision(Model* model, AnimationConfig* config, Character* character);

	// テレポートエフェクト設定
	void SetTeleportEffect(bool enable, float progress, float time);
	void ClearTeleportEffect();
	bool HasTeleportEffect() const { return hasTeleportEffect; }
	float GetTeleportProgress() const { return teleportProgress; }
	float GetTeleportTime() const { return teleportTime; }
	void ResetAttackState();
public:
	// 刃に沿って並べる当たり判定球の数
	static constexpr int HIT_SPHERE_COUNT = 5;

protected:
	//トレイルの描画
	void TrailRender(const RenderContext& rc);

	//トレイル用デバッグImGUI
	void DrawDebugTrailGUI();

	// 刃に沿った当たり判定球の初期配置（派生クラス共通）
	void SetupBladeHitSpheres(float sphereRadius);

	// ヒットエフェクトと打撃音の読み込み（派生クラス共通）
	void LoadCommonResources();

	// 武器共通のデバッグGUI（位置・回転・スケール・当たり判定球）
	void DrawCommonDebugGUI();

protected:
	DirectX::XMFLOAT3 position = { 0, 0, 0 };
	DirectX::XMFLOAT3 angle = { 0, 0, 0 };
	DirectX::XMFLOAT3 scale = { 1, 1, 1 };
	DirectX::XMFLOAT4X4 transform = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
	std::shared_ptr<Model> model;
	DirectX::XMFLOAT3 weaponHitOffset[HIT_SPHERE_COUNT];
	DirectX::XMFLOAT3 weaponHitPosition[HIT_SPHERE_COUNT];
	Model::EmissiveColors colors;//発光色

	// テレポートエフェクト用
	bool hasTeleportEffect = false;
	float teleportProgress = 0.0f;
	float teleportTime = 0.0f;

	// 武器トレイル関係
	static const int MAX_POLYGON = 6 * 2;//何フレーム文を保存して描くか
	DirectX::XMFLOAT3					trailPositions[2][MAX_POLYGON];//トレイル用頂点ポジション
	DirectX::XMFLOAT3					trailOffset[2] =	//トレイル補正用 [0]根本 [1]先端
	{
		{0.0f, 0.0f, 0.5f},
		{0.0f, 0.0f, 1.5f},
	};
	DirectX::XMFLOAT4 tipBegin = {};//剣周辺色
	DirectX::XMFLOAT4 tipEnd = {};//消滅するトレイルの色
	DirectX::XMFLOAT4 rootBegin = {};//剣周辺色
	DirectX::XMFLOAT4 rootEnd = {};//消滅するトレイルの色

	float colorScale{};//先端の色を濃くするためBeginにのみｘ

	float dissolve{};//ディゾルブ

	DirectX::XMFLOAT4 pointColor{};
	float attenuation{};

	bool isAttack{};//トレイルの描画を判定

	bool isParry = false;
	float parryTime = 0.0f;
	float maxParryTime = 0.0f;
	float maxCoolTime = 0.5f;//クールタイム
	float coolTime = 0.0f;

	int hitSphereIndex = -1;
	float hitSphereRadius = 0.1f;

	std::shared_ptr<Effect> attackHitEffect = nullptr;
	Effekseer::Handle attackHitEffectHandle;

	AudioSource* lightSE = nullptr;
	AudioSource* mediumSE = nullptr;
	AudioSource* heavySE = nullptr;
};
