#include <algorithm>
#include "Graphics/Graphics.h"
#include "Math/Mathf.h"
#include "Character/Player/Player.h"
#include "EnemyBoss.h"
#include "Model/ResourceManager.h"
#include "Scene/SceneManager.h"
#include "Character/Enemy/BehaviorTree/BehaviorTree.h"
#include "Character/Enemy/BehaviorTree/BehaviorData.h"
#include "Character/Enemy/BehaviorTree/NodeBase.h"
#include "Character/Enemy/BehaviorTree/JudgementDerived.h"
#include "Character/Enemy/BehaviorTree/ActionDerived.h"
#include "Character/Enemy/StateMachine/StateBase.h"
#include "Character/Enemy/StateMachine/SequenceState.h"
#include "Character/Enemy/StateMachine/RandomState.h"
#include "StateMachine/StateDerived.h"
#include "System/AnimationConfigLoader.h"
#include "Character/Enemy/BehaviorTree/EnemyBossBehavior.h"
#include "Script/LuaScriptSystem.h"

#include <stdlib.h>

namespace
{
	// 初期配置
	const DirectX::XMFLOAT3 SPAWN_POSITION = { -1.0f, -1.4f, 6.5f };
	constexpr float SPAWN_ANGLE_DEGREE = 180.0f;

	// 縄張りの広さと索敵範囲
	constexpr float TERRITORY_RANGE = 10.0f;
	constexpr float SEARCH_RANGE = 20.0f;

	// 体力
	constexpr int MAX_HEALTH = 800;

	// 当たり判定の大きさ
	constexpr float BODY_RADIUS = 0.5f;
	constexpr float BODY_HEIGHT = 1.0f;

	// 移動可能範囲の半径
	constexpr float AREA_LIMIT_RADIUS = 21.0f;

	// 使用するアセット
	constexpr const char* SWORD_MODEL_PATH = "Data/Model/Weapon/Katana/RedKatana.gltf";
	constexpr const char* LIGHT_BALL_EFFECT_PATH = "Data/Effect/LightBall.efkefc";
	constexpr const char* TELEPORT_EFFECT_PATH = "Data/Effect/Teleport.efkefc";
	constexpr const char* ATTACK_SIGN_EFFECT_PATH = "Data/Effect/AttackSign.efkefc";
	constexpr const char* MAGIC_CIRCLE_EFFECT_PATH = "Data/Effect/MagicCircle.efkefc";
	constexpr const char* DEATH_EFFECT_PATH = "Data/Effect/Death.efkefc";

	// 刀を持たせるノード名
	constexpr const char* SWORD_ATTACH_NODE_NAME = "middle_01_r";

	// 死亡時のアニメーションとルートモーションの基準ノード
	constexpr const char* DEATH_ANIMATION_NAME = "Hit_Large_Combat_Death_Seq_0";
	constexpr const char* ROOT_NODE_NAME = "root";
	constexpr float DEATH_BLEND_SECONDS = 0.1f;

	// アニメーションの長さをゼロとみなす閾値
	constexpr float MIN_ANIMATION_LENGTH = 0.0001f;

	// ベクトルをゼロベクトルとみなす長さ
	constexpr float DIRECTION_MIN_LENGTH = 0.001f;

	// ターゲットの方向を向ききったとみなす角度差
	constexpr float TURN_COMPLETE_TOLERANCE_DEGREE = 10.0f;

	// デバッグGUIから試すテレポートの消失時間
	constexpr float DEBUG_TELEPORT_FADE_OUT_SECONDS = 0.2f;

	// デバッグ表示する円柱の高さ
	constexpr float DEBUG_CYLINDER_HEIGHT = 1.0f;

	// ワープ候補位置の、プレイヤー方向と横方向へのオフセット
	// x成分とz成分で別の倍率を使っている
	struct WarpOffset
	{
		DirectX::XMFLOAT2 forward;  // プレイヤー方向へのオフセット (x, z)
		DirectX::XMFLOAT2 side;     // 横方向へのオフセット (x, z)
	};
	constexpr WarpOffset WARP_RIGHT_OFFSET = { { 5.0f, 3.0f }, { 10.0f, 4.0f } };
	constexpr WarpOffset WARP_LEFT_OFFSET = { { 5.0f, 5.0f }, { 10.0f, 7.0f } };
	constexpr WarpOffset WARP_FRONT_OFFSET = { { 10.0f, 10.0f }, { 0.0f, 0.0f } };
}

static EnemyBoss* instance = nullptr;

// インスタンス取得
EnemyBoss& EnemyBoss::Instance()
{
	return *instance;
}

// コンストラクタ
EnemyBoss::EnemyBoss(ID3D11Device* device, const char* filename, float scale)
{
	instance = this;

	model = std::make_shared<Model>(device, filename, scale);
	model->SetAdMetalness(1.0f);
	model->SetAdRoughness(1.0f);

	sword = std::make_unique<EnemySword>(device, SWORD_MODEL_PATH);
	lightBall = std::make_unique<Effect>(LIGHT_BALL_EFFECT_PATH);
	teleportEffect = std::make_unique<Effect>(TELEPORT_EFFECT_PATH);
	attackSign = std::make_unique<Effect>(ATTACK_SIGN_EFFECT_PATH);
	magicCircle = std::make_unique<Effect>(MAGIC_CIRCLE_EFFECT_PATH);
	deathEffect = std::make_shared<Effect>(DEATH_EFFECT_PATH);

	radius = BODY_RADIUS;
	height = BODY_HEIGHT;
	isGameClear = false;
	specialReady = true;

	InitAnimSpeed();

	// ビヘイビアツリー設定
	behaviorData = std::make_unique<BehaviorData<EnemyBoss>>();
	aiTree = new BehaviorTree<EnemyBoss>();

	// ツリーの構成は Data/Json/BehaviorTree_EnemyBoss.json にある。
	// エディタ（Behavior Tree ウィンドウ）で編集して、その場で組み直せる。
	LoadBehaviorTree();

	// 衝突判定用のノードを設定（ノード名と半径）
	nodeHitSpheres =
	{
		{"pelvis", 0.4f},
		{"spine_01", 0.3f},
		{"spine_02", 0.3f},
		{"neck_01", 0.3f},
		{"head", 0.25f},
		{"calf_l", 0.25f},
		{"foot_l", 0.25f},
		{"calf_r", 0.25f},
		{"foot_r", 0.25f},
	};

	SetPosition(SPAWN_POSITION);
	SetAngle(DirectX::XMFLOAT3(0.0f, DirectX::XMConvertToRadians(SPAWN_ANGLE_DEGREE), 0.0f));
	SetTerritory(GetPosition(), TERRITORY_RANGE);
	searchRange = SEARCH_RANGE;

	SetMaxHealth(MAX_HEALTH);
	SetHealth(MAX_HEALTH);

	SetRandomTargetPosition();

	const std::vector<ModelResource::Animation>& animations = model->GetResource()->GetAnimations();
	for (int i = 0; i < animations.size(); i++)
	{
		const AnimationConfig* config = AnimationConfigLoader::GetConfig(ANIMATION_CONFIG_OWNER, i);
		if (config != nullptr)
		{
			model->SetAnimationConfig(*config);
		}
	}

	areaSize = AREA_LIMIT_RADIUS;
}

// ビヘイビアツリー読み込み
void EnemyBoss::LoadBehaviorTree()
{
	EnemyBossBehavior::RegisterBehaviors();

	BehaviorTreeAsset asset;
	std::string error;

	if (!asset.Load(EnemyBossBehavior::GetTreeAssetPath(), error))
	{
		// まだ JSON が無い（初回起動）ので、これまでの構成を書き出しておく。
		// 次回からはこのファイルを編集すれば行動を差し替えられる。
		asset = EnemyBossBehavior::MakeDefaultTreeAsset();

		std::string saveError;
		asset.Save(EnemyBossBehavior::GetTreeAssetPath(), saveError);
	}

	std::vector<std::string> problems;
	if (!ApplyBehaviorAsset(asset, problems))
	{
		// JSON が壊れていてもボスが棒立ちにならないよう、既定の構成へ戻す
		LuaScriptSystem& lua = LuaScriptSystem::Instance();
		for (const std::string& problem : problems)
		{
			lua.Log(LuaScriptSystem::LogEntry::Level::Error, "BehaviorTree: " + problem);
		}

		std::vector<std::string> fallbackProblems;
		ApplyBehaviorAsset(EnemyBossBehavior::MakeDefaultTreeAsset(), fallbackProblems);
	}
}

// エディタで編集したアセットを反映する
bool EnemyBoss::ApplyBehaviorAsset(const BehaviorTreeAsset& asset, std::vector<std::string>& problems)
{
	if (aiTree == nullptr) aiTree = new BehaviorTree<EnemyBoss>();

	// 実行中ノードは作り直しで消えるので、先に手放しておく
	activeNode = nullptr;

	const bool built = EnemyBossBehavior::BuildTree(*aiTree, this, asset, problems);

	behaviorProblems = problems;

	if (built)
	{
		behaviorAsset = asset;
		EnemyBossBehavior::PreloadScripts(behaviorAsset);
	}

	ResetBehaviorState();
	return built;
}

// 実行中のノード ID を返す
int EnemyBoss::GetActiveNodeSourceId() const
{
	return activeNode != nullptr ? activeNode->GetSourceId() : -1;
}

// 実行中のノード名を返す
std::string EnemyBoss::GetActiveNodeName() const
{
	return activeNode != nullptr ? activeNode->GetName() : std::string();
}

// 推論をやり直す
void EnemyBoss::ResetBehaviorState()
{
	activeNode = nullptr;
	if (behaviorData != nullptr) behaviorData->Init();
}

// デストラクタ
EnemyBoss::~EnemyBoss()
{
	delete aiTree;
	afterimages.clear();
}

void EnemyBoss::UpdateEnemySpecific(float elapsedTime)
{
	int currentIndex = model->GetCurrentAnimationIndex();
	const ModelResource* resource = model->GetResource();

	if (resource != nullptr &&
		currentIndex >= 0 &&
		currentIndex < static_cast<int>(resource->GetAnimations().size()))
	{
		const AnimationConfig* config = model->GetAnimationConfig(ANIMATION_CONFIG_OWNER, currentIndex);
		float animationSeconds = model->GetCurrentAnimationSeconds();

		// 安全に長さを取得
		float secondsLength = model->GetAnimationLength(currentIndex);

		// ゼロ除算防止（長さが極端に短い、または0の場合は計算しない）
		if (secondsLength > MIN_ANIMATION_LENGTH)
		{
			float t = animationSeconds / secondsLength;
			t = std::clamp(t, 0.0f, 1.0f);  // 0.0～1.0 にクランプ
			float speed = model->EvaluateSpeed(config->speedCurve, t);
			model->SetAnimationSpeed(speed);
		}
	}

	// 反撃値チェック
	if (GetRevengeValue() > revengeTolerance)
	{
		SetRevenge(true);
		SetSuperArmor(true);
		ResetRevengeValue();
	}	

	// スーパーアーマー中は全てのダメージリアクションをキャンセルする
	if (isSuperArmor)
	{
		ResetDamage();
	}

	SetInvincible(isTeleporting);

	// 現在実行されているノードが無ければ
	if (actionFlag)
	{
		if (activeNode == nullptr)
		{
			sword->ResetAttackState();
			// 次に実行するノードを推論する。
			activeNode = aiTree->ActiveNodeInference(behaviorData.get());
		}
		// 現在実行するノードがあれば
		if (activeNode != nullptr)
		{
			if (activeNode->GetName() != "Damage" && GetRevengeValue() > 0)
			{
				SubRevengeValue();
			}
			// ビヘイビアツリーからノードを実行。
			activeNode = aiTree->Run(activeNode, behaviorData.get(), elapsedTime);
		}
	}

	// アタッチメント
	sword->Attach(SWORD_ATTACH_NODE_NAME, model.get());

	SetWarpPosition();

	sword->Update(elapsedTime);

	KeepAreaLimit(position);
}

// エディター用更新処理
void EnemyBoss::EditUpdate(float elapsedTime)
{
	int currentIndex = model->GetCurrentAnimationIndex();
	AnimationConfig* config = model->GetAnimationConfig(ANIMATION_CONFIG_OWNER, currentIndex);
	if (currentIndex >= 0) {
		
		float animationSeconds = model->GetCurrentAnimationSeconds();
		float secondsLength = model->GetAnimationLength(currentIndex);
		float t = animationSeconds / secondsLength;
		t = std::clamp(t, 0.0f, 1.0f);  // 念のため 0.0～1.0 にクランプ
		float speed = model->EvaluateSpeed(config->speedCurve, t);

		model->SetAnimationSpeed(speed);
	}
	sword->AttackAnimationCollision(model.get(), config, this);

	sword->Attach(SWORD_ATTACH_NODE_NAME, model.get());

	sword->Update(elapsedTime);

	// オブジェクト行列更新
	UpdateTransform();

	// モデルのアニメーション更新
	model->UpdateAnimation(elapsedTime, this);

	// モデル行列更新
	model->UpdateTransform(transform);
}

void EnemyBoss::Render(const RenderContext& rc, ShaderId shaderId)
{
	ModelRenderer* modelRenderer = Graphics::Instance().GetModelRenderer();

	// テレポートで置いていった体を、その場で粒子に分解しながら消す
	if (HasAfterimage())
	{
		const auto& afterimages = GetAfterimages(); // リストを取得
		for (const auto& afterimage : afterimages)
		{
			// 本体と武器を同じ削れ具合で描く
			for (const auto& part : afterimage.parts)
			{
				modelRenderer->DrawDisintegration(
					shaderId,
					part.model,
					part.nodes,
					afterimage.dissolve,
					GetTotalGameTime(),
					GetDisintegrationEdgeColor(),
					disintegrateEdgeWidth
				);
			}
		}
	}

	// キャラクター本体の描画
	TeleportPhase phase = GetTeleportPhase();

	// テレポート中（消失・移動・出現）は本体を描かず、残像の分解だけを見せる
	if (phase == TeleportPhase::None)
	{
		modelRenderer->Draw(shaderId, model);
		sword->Render(rc, shaderId);
	}
}

// 粒子分解に巻き込む追加モデル
std::vector<std::shared_ptr<Model>> EnemyBoss::GetAfterimageAttachments() const
{
	if (sword == nullptr) return {};

	return { sword->GetModel() };
}

void EnemyBoss::ShadowRender(const RenderContext& rc, ShadowMap* shadowMap)
{
	// テレポート中はRender()側で本体を描画していないため、影も落とさない
	// （落とすと、姿が無いのに影だけがステージ上を滑っていく状態になる）
	if (GetTeleportPhase() != TeleportPhase::None) return;

	shadowMap->Draw(rc, model.get());
	shadowMap->Draw(rc, sword->GetModel().get());
}

// 死亡した時に呼ばれる
void EnemyBoss::OnDead()
{
	SetDeathFlag(true);
	GetModel()->PlayRootMotion(GetModel()->GetAnimationIndex(DEATH_ANIMATION_NAME), false, true, DEATH_BLEND_SECONDS, ROOT_NODE_NAME);
}

void EnemyBoss::DrawDebugPrimitive()
{
	if (drawCollisionPrimitive)
	{
		// 基底クラスのデバッグプリミティブ描画
		Enemy::DrawDebugPrimitive();

		ShapeRenderer* debugRenderer = Graphics::Instance().GetShapeRenderer();

		for (const DirectX::SimpleMath::Vector3& warpPosition : warpPositions)
		{
			debugRenderer->DrawSphere(warpPosition, radius, DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f));
		}

		// 縄張り範囲をデバッグ円柱描画
		debugRenderer->DrawCylinder(territoryOrigin, territoryRange, DEBUG_CYLINDER_HEIGHT, DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f));

		// ターゲット位置をデバッグ球描画
		debugRenderer->DrawSphere(targetPosition, radius, DirectX::XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f));

		// 索敵範囲をデバッグ円柱描画
		debugRenderer->DrawCylinder(position, searchRange, DEBUG_CYLINDER_HEIGHT, DirectX::XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f));
	}

	AddCollisionSpheres(model, nodeHitSpheres);
}

// 縄張り設定
void EnemyBoss::SetTerritory(const DirectX::XMFLOAT3& origin, float range)
{
	territoryOrigin = origin;
	territoryRange = range;
}

// ターゲット位置をランダム設定
void EnemyBoss::SetRandomTargetPosition()
{
	float theta = Mathf::RandomRange(-DirectX::XM_PI, DirectX::XM_PI);
	float range = Mathf::RandomRange(0.0f, territoryRange);
	targetPosition.x = territoryOrigin.x + sinf(theta) * range;
	targetPosition.y = territoryOrigin.y;
	targetPosition.z = territoryOrigin.z + cosf(theta) * range;
}

// 移動設定
void EnemyBoss::SetMovement(DirectX::XMFLOAT3& vec, float speedRate)
{
	Move(vec.x, vec.z, moveSpeed * speedRate);
}

// 目的地点へ移動
void EnemyBoss::MoveToTarget(float elapsedTime, float speedRate)
{
	// ターゲット方向への進行ベクトルを算出
	float vx = targetPosition.x - position.x;
	float vz = targetPosition.z - position.z;
	float dist = sqrtf(vx * vx + vz * vz);
	vx /= dist;
	vz /= dist;

	// 移動処理
	Move(vx, vz, moveSpeed * speedRate);
	Turn(elapsedTime, vx, vz, turnSpeed);
}

// 旋回
void EnemyBoss::TurnToTarget(float elapsedTime, float speed)
{
	speed *= elapsedTime;

	float vx = targetPosition.x - position.x;
	float vz = targetPosition.z - position.z;

	// 進行ベクトルがゼロベクトルの場合は処理する必要なし
	float length = sqrtf(vx * vx + vz * vz);
	if (length < DIRECTION_MIN_LENGTH) return;

	// 進行ベクトルを単位ベクトル化
	vx /= length;
	vz /= length;

	// 自身の回転値から前方向を求める
	float frontX = sinf(angle.y);
	float frontZ = cosf(angle.y);

	// 回転角を求めるため、2つの単位ベクトルの内積を計算する
	float dot = frontX * vx + frontZ * vz;

	// 内積値は-1.0～1.0で表現されており、2つの単位ベクトルの角度が
	// 小さいほど1.0に近づくという性質を利用して回転速度を調整する
	float rot = 1.0f - dot;
	if (rot > speed) rot = speed;

	// 左右判定を行うために2つの単位ベクトルの外積を計算する
	float cross = (frontZ * vx) - (frontX * vz);

	// 2Dの外積値が正の場合か負の場合によって左右判定が行える
	// 左右判定を行うことによって左右回転を選択する
	if (health > 0.0f)
	{
		if (cross < 0.0f)
		{
			angle.y -= rot;
		}
		else
		{
			angle.y += rot;
		}
	}
}

bool EnemyBoss::SearchPlayer()
{
	// プレイヤーとの高低差を考慮して3Dで距離判定をする
	const DirectX::XMFLOAT3& playerPosition = Player::Instance().GetPosition();
	float vx = playerPosition.x - position.x;
	float vy = playerPosition.y - position.y;
	float vz = playerPosition.z - position.z;
	float dist = sqrtf(vx * vx + vy * vy + vz * vz);

	// 戦闘状態中は索敵を行わずに戦闘状態継続
	if (isBattle || health < maxHealth)
	{
		return true;
	}

	if (dist < searchRange)
	{
		float distXZ = sqrtf(vx * vx + vz * vz);
		// 単位ベクトル化
		vx /= distXZ;
		vz /= distXZ;

		// 方向ベクトル化
		float frontX = sinf(angle.y);
		float frontZ = cosf(angle.y);
		// 2つのベクトルの内積値で前後判定
		float dot = (frontX * vx) + (frontZ * vz);
		if (dot > 0.0f)
		{
			isBattle = true;
			return true;
		}
	}
	else if (dist > searchRange)
	{
		isBattle = false;
	}
	return false;
}

// プレイヤーの方向を向ききったか判定
bool EnemyBoss::IsTurnToTarget(float vx, float vz)
{
	// 自身の前方向ベクトルを計算
	float frontX = sinf(angle.y);
	float frontZ = cosf(angle.y);

	// ターゲット方向ベクトルを単位ベクトル化
	float length = sqrtf(vx * vx + vz * vz);
	if (length < DIRECTION_MIN_LENGTH) return false; // ゼロベクトルの場合は向ききっていないと判定
	vx /= length;
	vz /= length;

	// 自身の前方向ベクトルとターゲット方向ベクトルの内積を計算
	float dot = frontX * vx + frontZ * vz;

	// 内積から角度を計算（ラジアン）
	float angleDifference = acosf(std::clamp(dot, -1.0f, 1.0f));

	// 許容誤差
	const float tolerance = DirectX::XMConvertToRadians(TURN_COMPLETE_TOLERANCE_DEGREE);

	// 角度差が許容範囲内であれば向ききったと判定
	return angleDifference <= tolerance;
}

// デバッグエネミー情報表示
void EnemyBoss::DrawDebugGUI()
{
	std::string str = "";
	if (activeNode != nullptr)
	{
		str = activeNode->GetName();
	}
	//トランスフォーム
	if (ImGui::Begin("Enemy", nullptr, ImGuiWindowFlags_None))
	{
		// 位置
		ImGui::InputFloat3("Position", &position.x);
		// 回転
		DirectX::XMFLOAT3 a;
		a.x = DirectX::XMConvertToDegrees(angle.x);
		a.y = DirectX::XMConvertToDegrees(angle.y);
		a.z = DirectX::XMConvertToDegrees(angle.z);
		ImGui::InputFloat3("Angle", &a.x);
		angle.x = DirectX::XMConvertToRadians(a.x);
		angle.y = DirectX::XMConvertToRadians(a.y);
		angle.z = DirectX::XMConvertToRadians(a.z);
		// スケール
		ImGui::InputFloat3("Scale", &scale.x);

		ImGui::DragFloat("AttackRange", &attackRange, 0.1f);
		ImGui::DragFloat("TeleportOffset", &teleportOffset, 0.1f);

		ImGui::InputInt("Health", &health);
		ImGui::InputInt("RevengeValue", &revengeValue);

		ImGui::DragFloat("moveSpeed", &moveSpeed, 0.01f);

		ImGui::DragFloat("Gravity", &gravity);

		ImGui::Checkbox("IsGround", &isGround);
		bool isAnyDamage = IsAnyDamage();
		ImGui::Checkbox("DamageReaction", &isAnyDamage);
		ImGui::Checkbox("SuperArmor", &isSuperArmor);
		ImGui::Checkbox("RevengeState", &isRevenge);

		ImGui::Checkbox("ActionFlag", &actionFlag);

		ImGui::Text(u8"Behavior　%s", str.c_str());

		ImGui::Checkbox(u8"当たり判定描画フラグ", &drawCollisionPrimitive);

		if (ImGui::TreeNode("Teleport Effect"))
		{
			if (ImGui::Button("Test Teleport"))
			{
				DirectX::XMFLOAT3 targetPos = position;
				StartTeleport(targetPos, DEBUG_TELEPORT_FADE_OUT_SECONDS);
			}

			ImGui::TreePop();
		}

		Enemy::DrawDebugGUI();

		// 当たり判定調整
		for (size_t i = 0; i < nodeHitSpheres.size(); ++i)
		{
			ImGui::PushID(static_cast<int>(i)); // 各ノードごとにIDをプッシュして重複を防ぐ

			ImGui::Text(nodeHitSpheres[i].nodeName);

			// 半径の編集
			ImGui::DragFloat("Radius", &nodeHitSpheres[i].radius, 0.01f, 0.0f, 10.0f);

			ImGui::PopID();
		}
		ImGui::Separator();

		model->DebugGUI(u8"Enemy");
		sword->DrawDebugGUI();
	}
	ImGui::End();
	
}

void EnemyBoss::SetWarpPosition()
{
	// プレイヤーへの方向ベクトルを計算
	DirectX::SimpleMath::Vector3 toPlayer = DirectX::SimpleMath::Vector3{
		Player::Instance().GetPosition().x - position.x,
		0.0f,
		Player::Instance().GetPosition().z - position.z
	};

	// 正規化
	toPlayer.Normalize();

	// プレイヤー方向の左右ベクトルを計算（90度回転）
	DirectX::SimpleMath::Vector3 leftVec = DirectX::SimpleMath::Vector3{
		-toPlayer.z,
		0.0f,
		toPlayer.x
	};

	DirectX::SimpleMath::Vector3 rightVec = DirectX::SimpleMath::Vector3{
		toPlayer.z,
		0.0f,
		-toPlayer.x
	};

	// プレイヤー方向と横方向へのオフセットからワープ位置を求める
	auto computeWarpPosition = [&](const WarpOffset& offset, const DirectX::SimpleMath::Vector3& sideVec)
	{
		DirectX::SimpleMath::Vector3 warpPosition = DirectX::SimpleMath::Vector3{
			position.x + toPlayer.x * offset.forward.x + sideVec.x * offset.side.x,
			position.y,
			position.z + toPlayer.z * offset.forward.y + sideVec.z * offset.side.y
		};
		KeepAreaLimit(warpPosition);
		return warpPosition;
	};

	// 位置0: プレイヤー方向 + 右側
	warpPositions[0] = computeWarpPosition(WARP_RIGHT_OFFSET, rightVec);

	// 位置1: プレイヤー方向 + 左側
	warpPositions[1] = computeWarpPosition(WARP_LEFT_OFFSET, leftVec);

	// 位置2: プレイヤー方向の前方
	warpPositions[2] = computeWarpPosition(WARP_FRONT_OFFSET, rightVec);
}

void EnemyBoss::OnTeleportPhaseChanged(TeleportPhase newPhase)
{
	if (newPhase == TeleportPhase::FadeIn)
	{
		if (teleportEffect)
		{
			playedFadeEffect = true;
		}
	}
	else if (newPhase == TeleportPhase::None)
	{
		playedFadeEffect = false;
	}
}