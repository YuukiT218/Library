#pragma once

#include <string>
#include <vector>
#include <DirectXMath.h>
#include <wrl.h>
#include <d3d11.h>
#include "ModelResource.h"
#include <unordered_map>
#include <imgui.h>

#include "nlohmann/json.hpp"

using json = nlohmann::json;

// ラッパー構造体
struct Float3 {
	DirectX::XMFLOAT3 value;

	Float3() : value(0, 0, 0) {}
	Float3(float x, float y, float z) : value(x, y, z) {}
	Float3(const DirectX::XMFLOAT3& v) : value(v) {}
	operator DirectX::XMFLOAT3() const { return value; }

	float& x() { return value.x; }
	float& y() { return value.y; }
	float& z() { return value.z; }

	const float& x() const { return value.x; }
	const float& y() const { return value.y; }
	const float& z() const { return value.z; }
};

// JSON変換
inline void to_json(json& j, const Float3& v) {
	j = json::array({ v.value.x, v.value.y, v.value.z });
}

inline void from_json(const json& j, Float3& v) {
	float x = j.at(0).get<float>();
	float y = j.at(1).get<float>();
	float z = j.at(2).get<float>();
	v.value = DirectX::XMFLOAT3(x, y, z);
}

struct CameraKeyframe {
	float time;
	float range;
	Float3 eyeOffset;
	Float3 targetOffset;
	float savedYaw = 0.0f;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CameraKeyframe, time, range, eyeOffset, targetOffset, savedYaw)

enum class EventType {
	Camera,
	Effect,
	// 必要に応じて追加
};
// 変換用マップ
NLOHMANN_JSON_SERIALIZE_ENUM(EventType, {
	{EventType::Camera, "Camera"},
	{EventType::Effect, "Effect"}
	})

enum class AnimationFlag
{
	None,
	Attack,
	Invincible,
	Guard,
	SuperArmor
};

// enum <-> string 変換のための定義
NLOHMANN_JSON_SERIALIZE_ENUM(AnimationFlag, {
	{AnimationFlag::None, "None"},
	{AnimationFlag::Attack, "Attack"},
	{AnimationFlag::Invincible, "Invincible"},
	{AnimationFlag::Guard, "Guard"},
	{AnimationFlag::SuperArmor, "SuperArmor"}
	})

	struct Keyframe {
	float time;       // 0.0～1.0
	float value;      // 0.0～3.0
	float handleOffsetX = 0.05f;  // ハンドルのオフセット（自動）
	float handleOffsetY = 0.0f;
	float inTangent;
	float outTangent;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Keyframe, time, value, handleOffsetX, handleOffsetY, inTangent, outTangent)

enum class KnockbackType
{
	None,
	Light,
	Heavy,
	Launch,
	KnockDown
};

// enum <-> string 変換のための定義
NLOHMANN_JSON_SERIALIZE_ENUM(KnockbackType, {
	{KnockbackType::None, "None"},
	{KnockbackType::Light, "Light"},
	{KnockbackType::Heavy, "Heavy"},
	{KnockbackType::Launch, "Launch"},
	{KnockbackType::KnockDown, "KnockDown"}
	})

struct AttackAnimParam
{
	// 敵との距離の移動値と回転値
	float moveRate = 1.0f;
	float turnRate = 1.0f;

	//敵との距離に応じて移動値を調整
	float forwardPower = 0.0f;
	float forwardFrame = 0.0f;
	bool forwarded = false;

	// 攻撃判定必要変数
	int   attackDamage = 1.0f;
	float invisibleTime = 0.5f;

	// コントローラーの振動変数
	float attackLeftVibrate = 1.0f;
	float attackRightVibrate = 1.0f;

	// 攻撃時ヒットストップ変数
	float attackHitStopTime = 1.0f;
	float attackHitStopSpeed = 0.1f;

	// ノックバックの種類
	KnockbackType knockbackType = KnockbackType::None;

	// リベンジ値蓄積量
	int revengeValue = 1.0f;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AttackAnimParam,
	moveRate, turnRate,
	forwardPower, forwardFrame, forwarded,
	attackDamage, invisibleTime,
	attackLeftVibrate, attackRightVibrate,
	attackHitStopTime, attackHitStopSpeed,
	knockbackType, revengeValue)

struct AnimationAttribute
{
	AnimationFlag flag;  // 属性の種類（攻撃、無敵、パリィなど）
	float startTime;     // 開始時間（秒）
	float endTime;       // 終了時間（秒）

	AttackAnimParam attackParam;

	// 任意：指定時間にこの属性が有効かどうか
	bool IsActive(float currentTime) const
	{
		return currentTime >= startTime && currentTime <= endTime;
	}
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AnimationAttribute, flag, startTime, endTime, attackParam)

struct AnimationEvent
{
	float timeInSeconds;
	float timeOutSeconds;
	EventType eventType; //カメラやエフェクトなど
	std::string eventName; //EventCamera1などイベントタイプの中で何をするかを判別

	bool IsActive(float currentTime) const
	{
		return currentTime >= timeInSeconds && currentTime <= timeOutSeconds;
	}
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AnimationEvent, timeInSeconds, timeOutSeconds, eventType, eventName)

struct AnimationConfig
{
	std::string characterName;
	int animationIndex;
	std::vector<Keyframe> speedCurve;
	std::vector<AnimationEvent> events;
	std::vector<AnimationAttribute> attributes;
	std::vector<CameraKeyframe> cameraKeyframes;

	float advanceInputStartFrame = 0.0f;	//先行入力受付開始フレーム
	float advanceInputEndFrame = 0.0f;		//先行終了受付終了フレーム
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AnimationConfig,
	characterName,
	animationIndex,
	speedCurve,
	events,
	attributes,
	cameraKeyframes,
	advanceInputStartFrame,
	advanceInputEndFrame)

class Character;

class Model
{
public:
	Model(ID3D11Device* device, const char* filename, float scale);

	static const std::vector<D3D11_INPUT_ELEMENT_DESC> InputElementDescs;

	struct Node
	{
		std::string			name;
		int					parentIndex = -1;
		DirectX::XMFLOAT3	position = { 0, 0, 0 };
		DirectX::XMFLOAT4	rotation = { 0, 0, 0, 1 };
		DirectX::XMFLOAT3	scale = { 1, 1, 1 };

		DirectX::XMFLOAT4X4	localTransform = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
		DirectX::XMFLOAT4X4	globalTransform = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
		DirectX::XMFLOAT4X4	worldTransform = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };

		Node*				parent = nullptr; 
		std::vector<Node*>	children;

		template<class Archive>
		void serialize(Archive& archive);
	};

	struct NodePose
	{
		DirectX::XMFLOAT3	position = { 0, 0, 0 };
		DirectX::XMFLOAT4	rotation = { 0, 0, 0, 1 };
		DirectX::XMFLOAT3	scale = { 1, 1, 1 };
	};
	std::vector<NodePose> nodePose; // ノードポーズキャッシュ

	struct  EmissiveColors
	{
		float emissiveFactor = 1.0f;//発光度
		DirectX::XMFLOAT4 adjustColor{ 1.0f,1.0f,1.0f,1.0f };//色の調整
	};

	struct  DissolveConstants
	{
		float emissivedissolve = -0.1f;//エミッシブテクスチャ用ディゾルブ
		float dissolve = -0.1f;	//ディゾルブ
		float alphaFactor = 1.0f;//アルファ値調整
		DirectX::XMFLOAT4 OverwriteColor = { 1.0f,1.0f,1.0f,1.0f };//モデルの色を変化
	};

	struct RimLightConstants
	{
		float rimPower = 1.0f;
		float rimIntensity = 0.0f;
		DirectX::XMFLOAT4 rimColor = { 0.0f,0.0f,1.0f, 1.0f };
	};
	RimLightConstants rimLightConstants;

	void DrawGui();

	// ノードデータ取得
	const std::vector<Node>& GetNodes() const { return nodes; }
	std::vector<Node>& GetNodes() { return nodes; }

	// ルートノード取得
	Node* GetRootNode() { return nodes.data(); }

	// ノード検索
	Node* FindNode(const char* name);

	// トランスフォーム更新処理
	void UpdateTransform(const DirectX::XMFLOAT4X4& worldTransform);

	// 移動値取得
	const DirectX::XMFLOAT3& GetMove() const { return move; }
	void ClearMove() { move = { 0, 0, 0 }; }

	// アニメーション再生
	void PlayAnimation(int index, bool loop, float blendSeconds = 0);

	// ルートモーション再生
	void PlayRootMotion(int index, bool loop, bool bakeY, float blendSeconds = 0, const char* rootName = nullptr);

	// アニメーション再生中か
	bool IsPlayAnimation() const;

	// ルートモーション再生中か
	bool IsPlayRootMotion() const { return isRootMotion; }

	// 現在のアニメーション再生時間取得
	float GetCurrentAnimationSeconds() const { return currentAnimationSeconds; }
	void SetCurrentAnimationSeconds(float seconds) { currentAnimationSeconds = seconds; }

	// アニメーションインデックス取得
	int GetAnimationIndex(const char* name) const;
	int GetCurrentAnimationIndex() { return currentAnimationIndex; }

	// アニメーション名取得
	const char* GetAnimationName(int animationIndex) const;

	// アニメーション再生時間取得
	float GetAnimationLength(int animationIndex) const;

	// アニメーションスピード設定
	void SetAnimationSpeed(float animationSpeed) { this->animationSpeed = animationSpeed; }

	// アニメーション更新処理
	void UpdateAnimation(float elapsedTime, Character* character);

	// アニメーション計算処理
	void ComputeAnimation(float elapsedTime);
	void ComputeRootAnimation(float elapsedTime, Character* character);

	// アニメーション一時停止、再開
	void PauseAnimation(bool animationPause) { this->animationPause = animationPause; }

	// ブレンディング計算処理
	void ComputeBlending(float elapsedTime);

	// リソース取得
	const ModelResource* GetResource() const { return resource.get(); }

	// アニメーション計算
	void ComputeAnimation(int animationIndex, int nodeIndex, float time, NodePose& nodePose) const;
	void ComputeAnimation(int animationIndex, float time, std::vector<NodePose>& nodePoses) const;

	// ノードポーズ設定
	void SetNodePoses(const std::vector<NodePose>& nodePoses);

	// ノードポーズ取得
	void GetNodePoses(std::vector<NodePose>& nodePoses) const;

	//金属質感補正値取得
	void SetAdMetalness(const float metalness) { adjustMetalness = metalness; }
	//質感粗さ補正値取得
	void SetAdRoughness(const float roughness) { adjustRoughness = roughness; }

	//金属質感補正値取得
	float GetAdMetalness() const { return adjustMetalness; }
	//質感粗さ補正値取得
	float GetAdRoughness() const { return adjustRoughness;; }

	//ディゾルブ
	void SetEmissiveDissolve(const float emidissolve) { this->dissolveConstants.emissivedissolve = emidissolve; }
	void SetEmissiveConstants(const DissolveConstants dissolveConstants) { this->dissolveConstants = dissolveConstants; }
	DissolveConstants GetEmissiveConstants() const { return dissolveConstants; }

	//エミッシブ色設定
	void SetEmissiveColors(const EmissiveColors colors) { emissive = colors; }

	//エミッシブ色取得
	EmissiveColors GetEmissiveColors() const { return emissive; }

	//質感調整用ImGui
	void DebugGui(const char* name);

	// アニメーション設定の保存
	void SetAnimationConfig(const AnimationConfig& config)
	{
		// 一致するものがあれば上書き
		for (auto& existing : animationConfigs) {
			if (existing.characterName == config.characterName && existing.animationIndex == config.animationIndex) {
				existing = config;
				return;
			}
		}
		// なければ追加
		animationConfigs.push_back(config);
	}

	// アニメーション設定取得
	AnimationConfig* GetAnimationConfig(const std::string& characterName, int animationIndex)
	{
		for (auto& config : animationConfigs)
		{
			if (config.characterName == characterName && config.animationIndex == animationIndex)
			{
				return &config;
			}
		}

		// 見つからなかった場合、新しいAnimationConfigを追加して返す
		AnimationConfig defaultConfig;
		defaultConfig.characterName = characterName;
		defaultConfig.animationIndex = animationIndex;
		defaultConfig.speedCurve = { { 0.0f, 1.0f }, { 1.0f, 1.0f } };
		defaultConfig.events = {};
		defaultConfig.attributes = {};  // 空の配列
		defaultConfig.cameraKeyframes = {};

		animationConfigs.push_back(defaultConfig);
		return &animationConfigs.back();
	}

	// 
	float EvaluateSpeed(const std::vector<Keyframe>& curve, float t)
	{
		if (curve.empty())
			return 1.0f;

		// 端端は定値クロップ
		if (t <= curve.front().time)  return curve.front().value;
		if (t >= curve.back().time)   return curve.back().value;

		// 該当区間を探す
		for (size_t i = 0; i + 1 < curve.size(); ++i)
		{
			const auto& k0 = curve[i];
			const auto& k1 = curve[i + 1];
			if (t < k0.time || t > k1.time) continue;

			// 正規化パラメータ
			float dt = k1.time - k0.time;
			float u = (t - k0.time) / dt;

			// Hermite 基底
			float u2 = u * u, u3 = u2 * u;
			float h00 = 2 * u3 - 3 * u2 + 1;
			float h10 = u3 - 2 * u2 + u;
			float h01 = -2 * u3 + 3 * u2;
			float h11 = u3 - u2;

			// タンジェントをスケーリング（時間スケール分をかける）
			float m0 = k0.outTangent * dt;
			float m1 = k1.inTangent * dt;

			// 補間値を返す
			return h00 * k0.value
				+ h10 * m0
				+ h01 * k1.value
				+ h11 * m1;
		}

		// 万一見つからなかったら
		return 1.0f;
	}

private:
	//model事の質感補正値
	float adjustMetalness = 0; //  金属質調整
	float adjustRoughness = 0; //  粗さ調整
	//model事の発光補正値
	EmissiveColors emissive;
	//ディゾルブ
	DissolveConstants dissolveConstants;
	
	int currentAnimationIndex = -1;
	float currentAnimationSeconds = 0;
	float oldAnimationSeconds = 0;
	float animationSpeed = 1.0f;
	float baseAnimationSpeed = 1.0f;
	float scaling = 1.0f;

	bool animationPlaying = false;
	bool animationLoop = false;
	bool isRootMotion = false;
	bool bakeMoveY = false;

	float currentAnimationBlendSeconds = 0.0f;
	float animationBlendSecondsLength = -1.0f;
	bool animationBlending = false;
	bool animationPause = false;

	const char* filename = nullptr;
	const char* rootNodeName = nullptr;

private:
	std::shared_ptr<ModelResource> resource;
	std::vector<Node>		nodes;
	std::vector<NodePose> nodePoses;
	std::vector<std::string>	nodeNames; // ノード名キャッシュ
	NodePose beginPose, oldPose, newPose;
	NodePose endPose;
	std::vector<AnimationConfig> animationConfigs;
protected:
	DirectX::XMFLOAT3 move = { 0,0,0 };
};