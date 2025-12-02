#include <algorithm>
#include "Graphics/Graphics.h"
#include "Math/Mathf.h"
#include "Character/player.h"
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

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#define new ::new(_NORMAL_BLOCK, __FILE__, __LINE__)

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
	model->SetAdRoughness(0.0f);

	sword = std::make_unique<EnemySword>(device, "Data/Model/Weapon/Sword/Sword.gltf");

	radius = 0.5f;
	height = 1.0f;
	IsGameClear = false;

	initAnimSpeed();

	// ビヘイビアツリー設定
	behaviorData = std::make_unique<BehaviorData<EnemyBoss>>();
	aiTree = new BehaviorTree<EnemyBoss>();

	// BehaviorTreeのルートノードを追加
	aiTree->AddNode("", "Root", 0, BehaviorTree<EnemyBoss>::SelectRule::Priority, nullptr, nullptr);
	{
		aiTree->AddNode("Root", "Battle", 1, BehaviorTree<EnemyBoss>::SelectRule::Priority, new BattleJudgment(this), nullptr);
		aiTree->AddNode("Battle", "Dead", 1, BehaviorTree<EnemyBoss>::SelectRule::Non, new DeadJudgment(this), new DeadAction(this));
		aiTree->AddNode("Battle", "Damage", 2, BehaviorTree<EnemyBoss>::SelectRule::Non, new AnyDamageJudgment(this), new UnifiedDamageAction(this));
		aiTree->AddNode("Battle", "Pursuit", 3, BehaviorTree<EnemyBoss>::SelectRule::Non, nullptr, new PursuitAction(this));
	}
	{
		aiTree->AddNode("Battle", "Attack", 2, BehaviorTree<EnemyBoss>::SelectRule::Random, new AttackJudgment(this), nullptr);
		aiTree->AddNode("Attack", "SlashCombo", 1, BehaviorTree<EnemyBoss>::SelectRule::Non, nullptr, new SlashCombo1Action(this));
		aiTree->AddNode("Attack", "TeleportCombo", 2, BehaviorTree<EnemyBoss>::SelectRule::Non, nullptr, new TeleportCombo(this));
		aiTree->AddNode("Attack", "TeleportAssault", 3, BehaviorTree<EnemyBoss>::SelectRule::Non, nullptr, new TelePortAssault(this));

	}
	{
		aiTree->AddNode("Battle", "LongRange", 2, BehaviorTree<EnemyBoss>::SelectRule::Random, new LongRangeJudgment(this), nullptr);
		aiTree->AddNode("LongRange", "DashSlash", 1, BehaviorTree<EnemyBoss>::SelectRule::Non, nullptr, new DashSlashAction(this));
		{
			aiTree->AddNode("LongRange", "DashSlashCombo", 2, BehaviorTree<EnemyBoss>::SelectRule::Sequence, nullptr, nullptr);
			aiTree->AddNode("DashSlashCombo", "DashSlash", 2, BehaviorTree<EnemyBoss>::SelectRule::Non, nullptr, new DashSlashAction(this));
			aiTree->AddNode("DashSlashCombo", "DashSlash", 2, BehaviorTree<EnemyBoss>::SelectRule::Non, nullptr, new DashSlashAction(this));
			aiTree->AddNode("DashSlashCombo", "DashSlash", 2, BehaviorTree<EnemyBoss>::SelectRule::Non, nullptr, new DashSlashAction(this));
		}
		aiTree->AddNode("LongRange", "TripleTeleport", 2, BehaviorTree<EnemyBoss>::SelectRule::Non, nullptr, new TripleTeleportAction(this));
		aiTree->AddNode("LongRange", "TeleportAssault", 2, BehaviorTree<EnemyBoss>::SelectRule::Non, nullptr, new TelePortAssault(this));
		{
			aiTree->AddNode("LongRange", "TripleTeleportAssault", 2, BehaviorTree<EnemyBoss>::SelectRule::Sequence, nullptr, nullptr);
			aiTree->AddNode("TripleTeleportAssault", "TeleportAssault", 2, BehaviorTree<EnemyBoss>::SelectRule::Non, nullptr, new TelePortAssault(this));
			aiTree->AddNode("TripleTeleportAssault", "TeleportAssault", 2, BehaviorTree<EnemyBoss>::SelectRule::Non, nullptr, new TelePortAssault(this));
			aiTree->AddNode("TripleTeleportAssault", "TeleportAssault", 2, BehaviorTree<EnemyBoss>::SelectRule::Non, nullptr, new TelePortAssault(this));
		}
	}
	{
		aiTree->AddNode("Root", "Scout", 2, BehaviorTree<EnemyBoss>::SelectRule::Priority, nullptr, nullptr);
		aiTree->AddNode("Scout", "Idle", 2, BehaviorTree<EnemyBoss>::SelectRule::Non, nullptr, new IdleAction(this));
	}

	// 衝突判定用のノードを設定
	nodeHitSpheres =
	{
		{"pelvis", nodeRadius[0]},
		{"spine_01", nodeRadius[1]},
		{"spine_02", nodeRadius[2]},
		{"neck_01", nodeRadius[3]},
		{"head", nodeRadius[4]},
		{"calf_l", nodeRadius[5]},
		{"foot_l", nodeRadius[6]},
		{"calf_r", nodeRadius[7]},
		{"foot_r", nodeRadius[8]},
	};

	SetPosition(DirectX::XMFLOAT3(-1.0f, -2.4f, 6.5f));
	SetAngle(DirectX::XMFLOAT3(0.0f, DirectX::XMConvertToRadians(180.0f), 0.0f));
	SetTerritory(GetPosition(), 10.0f);

	SetMaxHealth(75000);
	SetHealth(75000);

	SetRandomTargetPosition();

	const std::vector<ModelResource::Animation>& animations = model->GetResource()->GetAnimations();
	for (int i = 0; i < animations.size(); i++)
	{
		const AnimationConfig* config = AnimationConfigLoader::GetConfig("EnemyBoss", i);
		if (config != nullptr)
		{
			model->SetAnimationConfig(*config);
		}
	}

	areaSize = 21.7f;
}

// デストラクタ
EnemyBoss::~EnemyBoss()
{
	delete aiTree;
}

void EnemyBoss::UpdateEnemySpecific(float elapsedTime)
{
	int currentIndex = model->GetCurrentAnimationIndex();
	if (currentIndex >= 0) {
		const AnimationConfig* config = model->GetAnimationConfig("EnemyBoss", currentIndex);
		float animationSeconds = model->GetCurrentAnimationSeconds();
		float secondsLength = model->GetAnimationLength(currentIndex);
		float t = animationSeconds / secondsLength;
		t = std::clamp(t, 0.0f, 1.0f);  // 念のため 0.0～1.0 にクランプ
		float speed = model->EvaluateSpeed(config->speedCurve, t);
		model->SetAnimationSpeed(speed);
	}

	// 現在実行されているノードが無ければ
	if (activeNode == nullptr)
	{
		// 次に実行するノードを推論する。
		activeNode = aiTree->ActiveNodeInference(behaviorData.get());
	}
	// 現在実行するノードがあれば
	if (activeNode != nullptr)
	{
		// ビヘイビアツリーからノードを実行。
		activeNode = aiTree->Run(activeNode, behaviorData.get(), elapsedTime);
	}

	// アタッチメント
	sword->Attach("weapon_r", model.get());

	SetWarpPosition();
	SetKnockBackPosition();

	sword->Update(elapsedTime);

	KeepAreaLimit(position);
}

// エディター用更新処理
void EnemyBoss::EditUpdate(float elapsedTime)
{
	int currentIndex = model->GetCurrentAnimationIndex();
	AnimationConfig* config = model->GetAnimationConfig("EnemyBoss", currentIndex);
	if (currentIndex >= 0) {
		
		float animationSeconds = model->GetCurrentAnimationSeconds();
		float secondsLength = model->GetAnimationLength(currentIndex);
		float t = animationSeconds / secondsLength;
		t = std::clamp(t, 0.0f, 1.0f);  // 念のため 0.0～1.0 にクランプ
		float speed = model->EvaluateSpeed(config->speedCurve, t);

		model->SetAnimationSpeed(speed);
	}
	sword->AttackAnimationCollision(model.get(), config, this);

	sword->Attach("weapon_r", model.get());

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
	modelRenderer->Draw(shaderId, model);
	sword->Render(rc, shaderId);
	modelRenderer->Render(rc);
}

void EnemyBoss::ShadowRender(const RenderContext& rc, ShadowMap* shadowMap)
{
	shadowMap->Draw(rc, model.get());
	shadowMap->Draw(rc, sword->GetModel().get());
}

// 死亡した時に呼ばれる
void EnemyBoss::OnDead()
{
	
}

void EnemyBoss::DrawDebugPrimitive()
{
	if (drawCollisionPrimitive)
	{
		// 基底クラスのデバッグプリミティブ描画
		Enemy::DrawDebugPrimitive();

		ShapeRenderer* debugRenderer = Graphics::Instance().GetShapeRenderer();

		debugRenderer->DrawSphere(WarpPosition[0], radius, DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f));
		debugRenderer->DrawSphere(WarpPosition[1], radius, DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f));
		debugRenderer->DrawSphere(WarpPosition[2], radius, DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f));

		debugRenderer->DrawSphere(KnockBackPosition[0], radius, DirectX::XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f));
		debugRenderer->DrawSphere(KnockBackPosition[1], radius, DirectX::XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f));
		debugRenderer->DrawSphere(KnockBackPosition[2], radius, DirectX::XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f));
		debugRenderer->DrawSphere(KnockBackPosition[3], radius, DirectX::XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f));

		// 縄張り範囲をデバッグ円柱描画
		debugRenderer->DrawCylinder(territoryOrigin, territoryRange, 1.0f, DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f));

		// ターゲット位置をデバッグ球描画
		debugRenderer->DrawSphere(targetPosition, radius, DirectX::XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f));

		// 索敵範囲をデバッグ円柱描画
		debugRenderer->DrawCylinder(position, searchRange, 1.0f, DirectX::XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f));
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
void EnemyBoss::SetMovement(DirectX::XMFLOAT3& Vec, float speedRate)
{
	Move(Vec.x, Vec.z, moveSpeed * speedRate);
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
	if (length < 0.001f)return;

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
	if (length < 0.001f) return false; // ゼロベクトルの場合は向ききっていないと判定
	vx /= length;
	vz /= length;

	// 自身の前方向ベクトルとターゲット方向ベクトルの内積を計算
	float dot = frontX * vx + frontZ * vz;

	// 内積から角度を計算（ラジアン）
	float angleDifference = acosf(std::clamp(dot, -1.0f, 1.0f));

	// 許容誤差（例: 10度 = 10 * π / 180）
	const float tolerance = DirectX::XMConvertToRadians(10.0f);

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

		ImGui::InputInt("Health", &health);
		ImGui::InputInt("RevengeValue", &revengeValue);

		ImGui::DragFloat("moveSpeed", &moveSpeed, 0.01f);

		ImGui::DragFloat("Gravity", &gravity);

		ImGui::Checkbox("IsGround", &isGround);
		bool isAnyDamage = IsAnyDamage();
		ImGui::Checkbox("DamageReaction", &isAnyDamage);

		ImGui::Checkbox("ActionFlag", &actionFlag);

		ImGui::Text(u8"Behavior　%s", str.c_str());

		ImGui::Checkbox(u8"当たり判定描画フラグ", &drawCollisionPrimitive);

		Enemy::DrawDebugGUI();

		// 当たり判定調整
		for (size_t i = 0; i < nodeHitSpheres.size(); ++i)
		{
			ImGui::PushID(static_cast<int>(i)); // 各ノードごとにIDをプッシュして重複を防ぐ

			ImGui::Text(nodeHitSpheres[i].nodeName);

			// 半径の編集
			if (ImGui::DragFloat("Radius", &nodeHitSpheres[i].radius, 0.01f, 0.0f, 10.0f))
			{
				nodeRadius[i] = nodeHitSpheres[i].radius; // nodeRadius 配列も更新
			}

			ImGui::PopID();
		}
		ImGui::Separator();

		model->DebugGui(u8"Enemy");
		sword->DrawDebugImGUi();
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

	// ワープ位置を設定
	// 位置0: プレイヤー方向 + 右側
	WarpPosition[0] = DirectX::SimpleMath::Vector3{
		position.x + toPlayer.x * 5.0f + rightVec.x * 7.0f,
		position.y,
		position.z + toPlayer.z * 5.0f + rightVec.z * 7.0f
	};

	// 位置1: プレイヤー方向 + 左側
	WarpPosition[1] = DirectX::SimpleMath::Vector3{
		position.x + toPlayer.x * 5.0f + leftVec.x * 7.0f,
		position.y,
		position.z + toPlayer.z * 5.0f + leftVec.z * 7.0f
	};

	// 位置2: プレイヤー方向の前方
	WarpPosition[2] = DirectX::SimpleMath::Vector3{
		position.x + toPlayer.x * 10.0f,
		position.y,
		position.z + toPlayer.z * 10.0f
	};
}

void EnemyBoss::SetKnockBackPosition()
{
	DirectX::SimpleMath::Vector3 vec;
	vec = CharacterForward(angle);
	KnockBackPosition[0] = DirectX::SimpleMath::Vector3{
		position.x + vec.x * 2.0f,
		position.y,
		position.z + vec.z * 2.0f };
	KnockBackPosition[1] = DirectX::SimpleMath::Vector3{
		position.x + vec.x * 5.0f,
		position.y,
		position.z + vec.z * 5.0f };
	KnockBackPosition[2] = DirectX::SimpleMath::Vector3{
		position.x + vec.x * 3.0f,
		position.y + 3.0f,
		position.z + vec.z * 3.0f };
	KnockBackPosition[3] = DirectX::SimpleMath::Vector3{
		position.x + vec.x * 1.5f,
		-1.4f,
		position.z + vec.z * 1.5f };
}
