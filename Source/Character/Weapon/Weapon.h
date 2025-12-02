#pragma once

#include "Character/Character.h"
#include "Graphics/Graphics.h"
#include "Graphics/Shader.h"

class Weapon
{
public:
	Weapon() {}
	virtual ~Weapon() {}

	// 更新処理
	virtual void Update(float elapsedTime) {}

	// 描画処理
	virtual void Render(const RenderContext& rc, ShaderId shaderId) {}

	//モデル用ゲッター
	std::shared_ptr<Model> GetModel() const { return model; }

	//アタッチ
	void Attach(std::string nodeName, Model* character);

	//トレイル用アップデート
	void TrailUpdate(float elapsedTime);

	//ノードとエネミーの衝突処理
	void CollisionNodeVsEnemies(float nodeRadius, int AttackDamage, float invicibleTime, float leftVibrate, float rightVibrate, float hitStopTime, float hitStopSpeed);
	void CollisionNodeVsCharacter(float nodeRadius, AnimationConfig* config, AnimationAttribute* activeAttribute, Character* character);

	//アニメーションの攻撃当たり判定をつける
	void AttackAnimationCollision(Model* character, float animTimeMin, float animTimeMax, int AttackDamage, float invicibleTime, float leftVibrate, float rightVibrate, float hitStopTime, float hitStopSpeed);
	void AttackAnimationCollision(Model* model, AnimationConfig* config, Character* character);
protected:
	//トレイルの描画
	void TrailRender(const RenderContext& rc);


	//トレイル用デバッグImGUI
	void DrawDebugTrailGui();

protected:
	DirectX::XMFLOAT3 position = { 0, 0, 0 };
	DirectX::XMFLOAT3 angle = { 0, 0, 0 };
	DirectX::XMFLOAT3 scale = { 1, 1, 1 };
	DirectX::XMFLOAT4X4 transform = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
	std::shared_ptr<Model> model;
	DirectX::XMFLOAT3 weaponHitOffset[5];
	DirectX::XMFLOAT3 weaponHitPosition[5];
	//Model::EmissiveColors colors;//発光色

	//武器トレイル関係
	static const int MAX_POLYGON = 6 * 2;//何フレーム文を保存して描くか

	DirectX::XMFLOAT3					trailPositions[2][MAX_POLYGON];//トレイル用頂点ポジション
	DirectX::XMFLOAT3					trailoffset[2] =	//トレイル補正用 [0]根本 [1]先端
	{
		{0.0f, 0.0f, 0.5f},
		{0.0f, 0.0f, 1.5f},
	};

	DirectX::XMFLOAT4 TipBegin = {};//剣周辺色
	DirectX::XMFLOAT4 TipEnd = {};//消滅するトレイルの色

	DirectX::XMFLOAT4 RootBegin = {};//剣周辺色
	DirectX::XMFLOAT4 RootEnd = {};//消滅するトレイルの色

	float Colorscale{};//先端の色を濃くするためBeginにのみｘ

	float dissolve{};//ディゾルブ

	DirectX::XMFLOAT4 pointColor{};
	float attenuation{};


	bool IsAttack{};//トレイルの描画を判定

	bool IsParry = false;
	float ParryTime = 0.0f;
#define MAXPARRYTIME 0.4f
	float MaxParryTime = 0.0f;
	float MaxCollTime = 0.5f;//クールタイム
	float collTime;

	int hitSphereIndex = -1;
	float hitSphereRadius = 0.1f;

	std::shared_ptr<Effect> attackHitEffect = nullptr;
	Effekseer::Handle attackHitEffectHandle;

	/*std::shared_ptr<Effect> parryEffect = nullptr;
	Effekseer::Handle ParryEffectHandle;*/
};