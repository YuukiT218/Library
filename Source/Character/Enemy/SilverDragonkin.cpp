#include <algorithm>
#include "Graphics/Graphics.h"
#include "Math/Mathf.h"
#include "Character/player.h"
#include "SilverDragonkin.h"
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

static SilverDragonkin* instance = nullptr;

// インスタンス取得
SilverDragonkin& SilverDragonkin::Instance()
{
	return *instance;
}

// コンストラクタ
SilverDragonkin::SilverDragonkin(ID3D11Device* device, const char* filename, float scale)
{
	instance = this;

	model = std::make_shared<Model>(device, filename, scale);

	radius = 0.5f;
	height = 1.0f;
	IsGameClear = false;

	initAnimSpeed();

	// ビヘイビアツリー設定
	behaviorData = std::make_unique<BehaviorData<SilverDragonkin>>();
	aiTree = std::make_unique<BehaviorTree<SilverDragonkin>>();

	// BehaviorTreeのルートノードを追加
	aiTree->AddNode("", "Root", 0, BehaviorTree<SilverDragonkin>::SelectRule::Priority, nullptr, nullptr);
	aiTree->AddNode("Root", "Battle", 1, BehaviorTree<SilverDragonkin>::SelectRule::Priority, new BattleJudgment(this), nullptr);
	aiTree->AddNode("Root", "Scout", 2, BehaviorTree<SilverDragonkin>::SelectRule::Priority, nullptr, nullptr);

	//aiTree->AddNode("Battle", "Roar", 1, BehaviorTree<SilverDragonkin>::SelectRule::Non, new RoarJudgment(this), new RoarAction(this));
	aiTree->AddNode("Battle", "Dead", 1, BehaviorTree<SilverDragonkin>::SelectRule::Non, new DeadJudgment(this), new DeadAction(this));
	aiTree->AddNode("Battle", "Damage", 2, BehaviorTree<SilverDragonkin>::SelectRule::Non, new DamageJudgment(this), new DamageAction(this));
	aiTree->AddNode("Battle", "Attack", 2, BehaviorTree<SilverDragonkin>::SelectRule::Random, new AttackJudgment(this), nullptr);
	//aiTree->AddNode("Attack", "AttackState", 2, BehaviorTree<SilverDragonkin>::SelectRule::Non, nullptr, new StateMachineAction(this, rootState.get()), rootState.get());
	aiTree->AddNode("Attack", "Scratch", 1, BehaviorTree<SilverDragonkin>::SelectRule::Non, new FineJudgment(this), new ScratchAction(this));
	aiTree->AddNode("Attack", "Slap", 2, BehaviorTree<SilverDragonkin>::SelectRule::Non, new FineJudgment(this), new SlapAction(this));
	//aiTree->AddNode("Attack", "Spin", 3, BehaviorTree<SilverDragonkin>::SelectRule::Non, new FineJudgment(this), new SpinAction(this));
	//aiTree->AddNode("Attack", "Tackle", 4, BehaviorTree<SilverDragonkin>::SelectRule::Non, nullptr, new TackleAction(this));
	//aiTree->AddNode("Attack", "Sidestep", 5, BehaviorTree<SilverDragonkin>::SelectRule::Non, nullptr, new SidestepAction(this));
	//aiTree->AddNode("Attack", "SlapScratch", 1, BehaviorTree<SilverDragonkin>::SelectRule::Non, new DyingJudgment(this), new ScratchSlapAction(this));
	//aiTree->AddNode("Attack", "SlapSpin", 2, BehaviorTree<SilverDragonkin>::SelectRule::Non, new DyingJudgment(this), new SlapSpinAction(this));
	//aiTree->AddNode("Attack", "SpinSpin", 3, BehaviorTree<SilverDragonkin>::SelectRule::Non, new DyingJudgment(this), new SpinSpinAction(this));
	//aiTree->AddNode("Attack", "SpinTackle", 4, BehaviorTree<SilverDragonkin>::SelectRule::Non, new DyingJudgment(this), new SpinTackleAction(this));
	aiTree->AddNode("Battle", "LongRange", 2, BehaviorTree<SilverDragonkin>::SelectRule::Random, new LongRangeJudgment(this), nullptr);
	aiTree->AddNode("LongRange", "TripleTeleport", 1, BehaviorTree<SilverDragonkin>::SelectRule::Non, nullptr, new TripleTeleportAction(this));
	//aiTree->AddNode("LongRange", "BreathAttack", 1, BehaviorTree<SilverDragonkin>::SelectRule::Non, nullptr, new BreathAction(this));
	//aiTree->AddNode("LongRange", "BreathAttackSweeping", 2, BehaviorTree<SilverDragonkin>::SelectRule::Non, nullptr, new BreathSweepingAction(this));
	//aiTree->AddNode("LongRange", "Tackle", 3, BehaviorTree<SilverDragonkin>::SelectRule::Non, new TackleJudgment(this), new TackleAction(this));
	aiTree->AddNode("LongRange", "Pursuit", 4, BehaviorTree<SilverDragonkin>::SelectRule::Non, nullptr, new PursuitAction(this));
	aiTree->AddNode("Battle", "Pursuit", 3, BehaviorTree<SilverDragonkin>::SelectRule::Non, nullptr, new PursuitAction(this));


	aiTree->AddNode("Scout", "Wander", 1, BehaviorTree<SilverDragonkin>::SelectRule::Non, new WanderJudgment(this), new WanderAction(this));
	aiTree->AddNode("Scout", "Idle", 2, BehaviorTree<SilverDragonkin>::SelectRule::Non, nullptr, new IdleAction(this));

	// 衝突判定用のノードを設定
	nodeHitSpheres =
	{
		{"Pelvis", nodeRadius[0]},
		{"spine_01", nodeRadius[1]},
		{"spine_02", nodeRadius[2]},
		{"neck_01", nodeRadius[3]},
		{"head", nodeRadius[4]},
		{"calf_l", nodeRadius[5]},
		{"Foot_L", nodeRadius[6]},
		{"calf_r", nodeRadius[7]},
		{"Foot_R", nodeRadius[8]},
	};

	// 攻撃判定用のノードを設定
	// 右前腕
	rightArmNodeHitSpheres =
	{
		/*{"Bip001_R_Hand", nodeRadiusAttack[0]},
		{"Bip001_R_Forearm", nodeRadiusAttack[1]},
		{"Bip001_R_Finger2", nodeRadiusAttack[2]},*/
	};
	// 尻尾
	tailNodeHitSpheres =
	{
		/*{"Bone001", nodeRadiusAttack[21]},
		{"Bone002", nodeRadiusAttack[22]},
		{"Bone003", nodeRadiusAttack[23]},
		{"Bone004", nodeRadiusAttack[24]},
		{"Bone005", nodeRadiusAttack[25]},
		{"Bone006", nodeRadiusAttack[26]},
		{"Bone007", nodeRadiusAttack[27]},
		{"Bone008", nodeRadiusAttack[28]},*/
	};
	// タックル用
	tackleHitSpheres =
	{
		/*{"Bip001_R_Hand", nodeRadiusAttack[0]},
		{"Bip001_R_Forearm", nodeRadiusAttack[1]},
		{"Bip001_R_Finger2", nodeRadiusAttack[2]},
		{"Bip001_L_Hand", nodeRadiusAttack[3]},
		{"Bip001_L_Forearm", nodeRadiusAttack[4]},
		{"Bip001_L_Finger2", nodeRadiusAttack[5]},
		{"Bip001_R_Thigh", nodeRadiusAttack[6]},
		{"Bip001_R_Calf", nodeRadiusAttack[7]},
		{"Bip001_R_HorseLink", nodeRadiusAttack[8]},
		{"Bip001_R_Foot", nodeRadiusAttack[9]},
		{"Bip001_L_Thigh", nodeRadiusAttack[10]},
		{"Bip001_L_Calf", nodeRadiusAttack[11]},
		{"Bip001_L_HorseLink", nodeRadiusAttack[12]},
		{"Bip001_L_Foot", nodeRadiusAttack[13]},
		{"Bip001_Pelvis", nodeRadiusAttack[14]},
		{"Bip001_Spine", nodeRadiusAttack[15]},
		{"Bip001_Spine1", nodeRadiusAttack[16]},
		{"Bip001_Spine2", nodeRadiusAttack[17]},
		{"Point013", nodeRadiusAttack[18]},
		{"Point002", nodeRadiusAttack[19]},
		{"Point007", nodeRadiusAttack[20]},
		{"Bone001", nodeRadiusAttack[21]},
		{"Bone002", nodeRadiusAttack[22]},
		{"Bone003", nodeRadiusAttack[23]},
		{"Bone004", nodeRadiusAttack[24]},
		{"Bone005", nodeRadiusAttack[25]},
		{"Bone006", nodeRadiusAttack[26]},
		{"Bone007", nodeRadiusAttack[27]},
		{"Bone008", nodeRadiusAttack[28]},*/
	};

	
	SetPosition(DirectX::XMFLOAT3(-1.0f, -3.8f, 40.0f));
	SetAngle(DirectX::XMFLOAT3(0.0f, DirectX::XMConvertToRadians(180.0f), 0.0f));
	SetTerritory(GetPosition(), 10.0f);

	SetMaxHealth(75000);
	SetHealth(75000);

	SetRandomTargetPosition();

	const std::vector<ModelResource::Animation>& animations = model->GetResource()->GetAnimations();
	/*for (int i = 0; i < animations.size(); i++)
	{
		const AnimationConfig* config = AnimationConfigLoader::GetConfig("SilverDragonkin", i);
		if (config != nullptr)
		{
			model->SetAnimationConfig(*config);
		}
	}*/

	areaSize = 44.2f;
}

// デストラクタ
SilverDragonkin::~SilverDragonkin()
{

}

void SilverDragonkin::UpdateEnemySpecific(float elapsedTime)
{
	int currentIndex = model->GetCurrentAnimationIndex();
	if (currentIndex >= 0) {
		//const AnimationConfig* config = model->GetAnimationConfig("SilverDragonkin", currentIndex);
		float animationSeconds = model->GetCurrentAnimationSeconds();
		float secondsLength = model->GetAnimationLength(currentIndex);
		float t = animationSeconds / secondsLength;
		t = std::clamp(t, 0.0f, 1.0f);  // 念のため 0.0～1.0 にクランプ
		//float speed = model->EvaluateSpeed(config->speedCurve, t);

		//model->SetAnimationSpeed(speed);

		//for (const auto& event : config->events)
		//{
		//	if (animationSeconds >= event.timeInSeconds)
		//	{
		//		static Effekseer::Handle handle = -1; // エフェクトのハンドルを保持
		//		switch (event.eventType)
		//		{
		//		case EventType::Effect:
		//			if (event.eventName == "Attack")
		//			{
		//				// ブレスエフェクト再生
		//				{
		//					Model::Node* modelNode = model->FindNode("Bone048");
		//					DirectX::XMFLOAT3 pos = {
		//						modelNode->worldTransform._41,
		//						modelNode->worldTransform._42,
		//						modelNode->worldTransform._43
		//					};

		//					if (handle == -1)
		//					{
		//						handle = attackTelegraphEffect->Play(pos);
		//						attackTelegraphEffect->SetPosition(handle, pos);
		//					}
		//				}
		//			}
		//			break;
		//		}


		//		if (!event.IsActive(animationSeconds))
		//		{
		//			// エフェクト停止
		//			if (handle != -1)
		//			{
		//				attackTelegraphEffect->Stop(handle);
		//				handle = -1;
		//			}
		//		}
		//	}
		//}
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

	// オブジェクト行列更新
	UpdateTransform();

	// モデル行列更新
	model->UpdateTransform(transform);

	//ポイントライトの更新
	/*for (int i = 0; i < AnimPointLights.size(); i++)
	{
		AnimPointLights.at(i).Update(elapsedTime);
	}*/

	KeepAreaLimit(position);
}

// エディター用更新処理
void SilverDragonkin::EditUpdate(float elapsedTime)
{
	//int currentIndex = model->GetCurrentAnimationIndex();
	//if (currentIndex >= 0) {
	//	const AnimationConfig* config = model->GetAnimationConfig("SilverDragonkin", currentIndex);
	//	float animationSeconds = model->GetCurrentAnimationSeconds();
	//	float secondsLength = model->GetAnimationLength(currentIndex);
	//	float t = animationSeconds / secondsLength;
	//	t = std::clamp(t, 0.0f, 1.0f);  // 念のため 0.0～1.0 にクランプ
	//	float speed = model->EvaluateSpeed(config->speedCurve, t);

	//	model->SetAnimationSpeed(speed);

	//	for (const auto& event : config->events)
	//	{
	//		if (animationSeconds >= event.timeInSeconds)
	//		{
	//			static Effekseer::Handle handle = -1; // エフェクトのハンドルを保持
	//			static Effekseer::Handle handle1 = -1; // エフェクトのハンドルを保持
	//			switch (event.eventType)
	//			{
	//			case EventType::Effect:
	//				if (event.eventName == "Breath")
	//				{
	//					// ブレスエフェクト再生
	//					{
	//						Model::Node* modelNode = model->FindNode("Bone052");
	//						DirectX::XMFLOAT3 pos = {
	//							modelNode->worldTransform._41,
	//							modelNode->worldTransform._42,
	//							modelNode->worldTransform._43
	//						};
	//						DirectX::XMFLOAT3 Angle = {
	//							angle.z,
	//							angle.y + DirectX::XM_PI,
	//							angle.x
	//						};

	//						if (handle == -1)
	//							handle = breathEffect->Play(pos);

	//						breathEffect->SetRotation(handle, Angle);
	//						breathEffect->SetPosition(handle, pos);
	//						breathEffect->SetTargetPosition(handle, Player::Instance().GetPosition());
	//					}
	//				}
	//				if (event.eventName == "Attack")
	//				{
	//					{
	//						if (handle1 == -1)
	//						{
	//							Model::Node* modelNode = model->FindNode("Bone048");
	//							DirectX::XMFLOAT3 pos = {
	//								modelNode->worldTransform._41,
	//								modelNode->worldTransform._42,
	//								modelNode->worldTransform._43
	//							};
	//							//attackTelegraphEffect->SetPosition(handle1, pos);
	//							handle1 = attackTelegraphEffect->Play(pos);
	//						}
	//					}
	//				}
	//				break;

	//			case EventType::Camera:
	//				// カメラ制御などをここに追加
	//				break;
	//			}


	//			if (!event.IsActive(animationSeconds))
	//			{
	//				// エフェクト停止
	//				if (handle != -1)
	//				{
	//					breathEffect->Stop(handle);
	//					handle = -1;
	//				}
	//				if (handle1 != -1)
	//				{
	//					attackTelegraphEffect->Stop(handle1);
	//					handle1 = -1;
	//				}
	//			}
	//		}
	//	}

	//}
	//// オブジェクト行列更新
	//UpdateTransform();

	//// モデルのアニメーション更新
	//model->UpdateAnimation(elapsedTime);

	//// モデル行列更新
	//model->UpdateTransform(transform);
}

void SilverDragonkin::Render(const RenderContext& rc, ShaderId shaderId)
{
	ModelRenderer* modelRenderer = Graphics::Instance().GetModelRenderer();
	modelRenderer->Draw(shaderId, model);
	modelRenderer->Render(rc);
}

void SilverDragonkin::ShadowRender(const RenderContext& rc, ShadowMap* shadowMap)
{
	shadowMap->Draw(rc, model.get());
}

// 死亡した時に呼ばれる
void SilverDragonkin::OnDead()
{
	model->PlayAnimation(static_cast<int>(EnemyAnimation::Die), false, GetBlendSeconds());
	SetDeathFlag(true);
}

void SilverDragonkin::DrawDebugPrimitive()
{
	// 基底クラスのデバッグプリミティブ描画
	Enemy::DrawDebugPrimitive();

	ShapeRenderer* debugRenderer = Graphics::Instance().GetShapeRenderer();

	// 縄張り範囲をデバッグ円柱描画
	debugRenderer->DrawCylinder(territoryOrigin, territoryRange, 1.0f, DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f));

	// ターゲット位置をデバッグ球描画
	debugRenderer->DrawSphere(targetPosition, radius, DirectX::XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f));

	// 索敵範囲をデバッグ円柱描画
	debugRenderer->DrawCylinder(position, searchRange, 1.0f, DirectX::XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f));

	AddCollisionSpheres(model, nodeHitSpheres);
}

// 縄張り設定
void SilverDragonkin::SetTerritory(const DirectX::XMFLOAT3& origin, float range)
{
	territoryOrigin = origin;
	territoryRange = range;
}
// ターゲット位置をランダム設定
void SilverDragonkin::SetRandomTargetPosition()
{
	float theta = Mathf::RandomRange(-DirectX::XM_PI, DirectX::XM_PI);
	float range = Mathf::RandomRange(0.0f, territoryRange);
	targetPosition.x = territoryOrigin.x + sinf(theta) * range;
	targetPosition.y = territoryOrigin.y;
	targetPosition.z = territoryOrigin.z + cosf(theta) * range;
}

// 移動設定
void SilverDragonkin::SetMovement(DirectX::XMFLOAT3& Vec, float speedRate)
{
	Move(Vec.x, Vec.z, moveSpeed * speedRate);
}

// 目的地点へ移動
void SilverDragonkin::MoveToTarget(float elapsedTime, float speedRate)
{
	// ターゲット方向への進行ベクトルを算出
	float vx = targetPosition.x - position.x;
	float vz = targetPosition.z - position.z;
	float dist = sqrtf(vx * vx + vz * vz);
	vx /= dist;
	vz /= dist;

	// 移動処理
	Move(vx, vz, moveSpeed * speedRate);
	Turn(elapsedTime, vx, vz, turnSpeed * speedRate);
}

// 旋回
void SilverDragonkin::TurnToTarget(float elapsedTime, float vx, float vz, float speed)
{
	speed *= elapsedTime;

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
	if (rot > speed)rot = speed;

	// 左右判定を行うために2つの単位ベクトルの外積を計算する
	float cross = (frontZ * vx) - (frontX * vz);

	// 2Dの外積値が正の場合か負の場合によって左右判定が行える
	// 左右判定を行うことによって左右回転を選択する
	if (health > 0.0f)
	{
		if (cross < 0.0f)
		{
			/*if (!isTurnAnimation)
			{
				model->PlayAnimation(static_cast<int>(EnemyAnimation::TurnLeft), false, blendSeconds);
				isTurnAnimation = true;
			}*/
			angle.y -= rot;
		}
		else
		{
			/*if (!isTurnAnimation)
			{
				model->PlayAnimation(static_cast<int>(EnemyAnimation::TurnRight), false, blendSeconds);
				isTurnAnimation = true;
			}*/
			angle.y += rot;
		}
	}
}

bool SilverDragonkin::SearchPlayer()
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
bool SilverDragonkin::IsTurnToTarget(float vx, float vz)
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
void SilverDragonkin::DrawDebugGUI()
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

		ImGui::Checkbox("DamageReaction", &isDamage);

		ImGui::Text(u8"Behavior　%s", str.c_str());

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

		ShowDragonLightEditor();
		model->DebugGui(u8"Enemy");
	}
	ImGui::End();
	
}

void SilverDragonkin::ShowDragonLightEditor()
{
	//if (ImGui::CollapsingHeader("Dragon Light", ImGuiTreeNodeFlags_Framed))
	//{
	//	if (ImGui::Button(u8"ドラゴンライト生成", { 200,50 }))
	//	{
	//		AnimatedLight animlight{};
	//		//int index = LightManager::Instance().AllocatePointLight();
	//		int index = 30 + AnimPointLights.size();
	//		if (AnimPointLights.size() != 10)
	//		{
	//			animlight.index = index;
	//			//LightManager::Instance().SetPointLight(animlight.baseLight, index); // 初期ライト登録（任意）
	//			AnimPointLights.push_back(animlight);
	//		}
	//		else
	//		{
	//			// 最大数に達していたらここに警告などを入れてもOK
	//			ImGui::OpenPopup(u8"ライト生成失敗");
	//		}
	//	}

	//	// 実際にポップアップ内容を描画するコード（※毎フレーム必要）
	//	if (ImGui::BeginPopup(u8"ライト生成失敗"))
	//	{
	//		ImGui::Text(u8"ライト数が上限に達しています！");
	//		if (ImGui::Button("OK"))
	//		{
	//			ImGui::CloseCurrentPopup();
	//		}
	//		ImGui::EndPopup();
	//	}

	//	for (int i = 0; i < AnimPointLights.size(); i++)
	//	{

	//		ImGui::PushID(i);  // ここでIDスコープを変える
	//		if (ImGui::CollapsingHeader("Anim Light", ImGuiTreeNodeFlags_Framed))
	//		{
	//			float pos[4] = { AnimPointLights.at(i).baseLight.position.x, AnimPointLights.at(i).baseLight.position.y,
	//				AnimPointLights.at(i).baseLight.position.z, AnimPointLights.at(i).baseLight.position.w };
	//			float col[4] = { AnimPointLights.at(i).baseLight.color.x, AnimPointLights.at(i).baseLight.color.y,
	//				AnimPointLights.at(i).baseLight.color.z, AnimPointLights.at(i).baseLight.color.w };

	//			float targetIntensity = AnimPointLights.at(i).targetIntensity;

	//			ImGui::Separator();
	//			ImGui::Text("Light");
	//			if (ImGui::DragFloat3("Position", pos, 0.1f)) {
	//				AnimPointLights.at(i).baseLight.position = { pos[0], pos[1], pos[2], AnimPointLights.at(i).baseLight.position.w };
	//			}
	//			if (ImGui::DragFloat("Attenuation", &targetIntensity, 0.1f, 0.0f, 100.0f))
	//			{
	//				AnimPointLights.at(i).FadeIn(targetIntensity);
	//			}
	//			if (ImGui::ColorEdit3("Color", col)) {
	//				AnimPointLights.at(i).baseLight.color = { col[0], col[1], col[2], 1.0f };
	//			}

	//			static char nodeName[64] = "Head";  // デフォルトノード名
	//			int& selectedNodeIndex = AnimPointLights[i].selectedNodeIndex;
	//			static std::vector<std::string>NodeNameList = model->GetNodeList();

	//			if (ImGui::BeginCombo(u8"ライトアタッチノード", NodeNameList[selectedNodeIndex].c_str()))
	//			{
	//				for (int i = 0; i < NodeNameList.size(); ++i)
	//				{
	//					bool isSelected = (selectedNodeIndex == i);
	//					if (ImGui::Selectable(NodeNameList[i].c_str(), isSelected))
	//					{
	//						selectedNodeIndex = i;
	//					}
	//					if (isSelected)
	//						ImGui::SetItemDefaultFocus();
	//				}
	//				ImGui::EndCombo();
	//			}

	//			const std::string& selectedNodeName = NodeNameList[selectedNodeIndex];
	//			Model::Node* targetNode = model->FindNode(selectedNodeName.c_str());

	//			// チェックボックスの描画（戻り値は変更されたときだけtrueになる）
	//			ImGui::Checkbox(u8"ポイントライトをアタッチ", &AnimPointLights[i].attachToNode);

	//			AnimPointLights[i].selectedNodeIndex = selectedNodeIndex;

	//			// Attachがtrueの間は、毎フレームこの処理を通る
	//			if (AnimPointLights[i].attachToNode)
	//			{
	//				if (targetNode)
	//				{

	//					// ローカル座標（targetNode->position）をワールド座標に変換する
	//					DirectX::XMVECTOR localPos = XMLoadFloat3(&targetNode->position);      // ローカル座標
	//					DirectX::XMMATRIX worldMatrix = DirectX::XMLoadFloat4x4(&targetNode->worldTransform);          // ワールド行列（自作関数か保持してる行列）

	//					DirectX::XMVECTOR worldPos = XMVector3Transform(localPos, worldMatrix); // ワールド座標に変換
	//					DirectX::XMFLOAT3 worldPosition;
	//					DirectX::XMStoreFloat3(&worldPosition, worldPos);

	//					ImGui::Text(u8"ノードポジション: X = %.2f, Y = %.2f, Z = %.2f", worldPosition.x, worldPosition.y, worldPosition.z);
	//					ImGui::Text(u8"ベースポジション: X = %.2f, Y = %.2f, Z = %.2f", AnimPointLights.at(i).basePosition.x, AnimPointLights.at(i).basePosition.y, AnimPointLights.at(i).basePosition.z);
	//					AnimPointLights.at(i).basePosition = worldPosition;
	//				}
	//				else
	//				{
	//					ImGui::OpenPopup(u8"ノードの検索失敗");
	//				}
	//			}

	//			// ポップアップ描画
	//			if (ImGui::BeginPopup(u8"ノードの検索失敗"))
	//			{
	//				ImGui::Text(u8"ノードが存在していません！");
	//				if (ImGui::Button("OK"))
	//				{
	//					ImGui::CloseCurrentPopup();
	//				}
	//				ImGui::EndPopup();
	//			}


	//		}
	//		ImGui::PopID();  // ← 必ず PushID と対にする！
	//	}
	//	//	// 羽ライト（インデックス 0）
	//	//	{

	//	//		float pos[4] = { light.position.x, light.position.y, light.position.z, light.position.w };
	//	//		float col[4] = { light.color.x, light.color.y, light.color.z, light.color.w };

	//	//		ImGui::Text("Wing Light");
	//	//		if (ImGui::DragFloat3("Wing Pos", pos, 0.1f)) {
	//	//			light.position = { pos[0], pos[1], pos[2], light.position.w };
	//	//		}
	//	//		if (ImGui::DragFloat("Wing Attenuation", &pos[3], 0.1f, 0.1f, 100.0f)) {
	//	//			light.position.w = pos[3];
	//	//		}
	//	//		if (ImGui::ColorEdit3("Wing Color", col)) {
	//	//			light.color = { col[0], col[1], col[2], 1.0f };
	//	//		}

	//	//		LightManager::Instance().SetPointLight(light, index);
	//	//	}

	//	//	// 口ライト（インデックス 1）
	//	//	{
	//	//		int index = 11;
	//	//		PointLight light = LightManager::Instance().GetPointLight(index);

	//	//		float pos[4] = { light.position.x, light.position.y, light.position.z, light.position.w };
	//	//		float col[4] = { light.color.x, light.color.y, light.color.z, light.color.w };

	//	//		ImGui::Separator();
	//	//		ImGui::Text("Mouth Light");
	//	//		if (ImGui::DragFloat3("Mouth Pos", pos, 0.1f)) {
	//	//			light.position = { pos[0], pos[1], pos[2], light.position.w };
	//	//		}
	//	//		if (ImGui::DragFloat("Mouth Attenuation", &pos[3], 0.1f, 0.1f, 100.0f)) {
	//	//			light.position.w = pos[3];
	//	//		}
	//	//		if (ImGui::ColorEdit3("Mouth Color", col)) {
	//	//			light.color = { col[0], col[1], col[2], 1.0f };
	//	//		}

	//	//		LightManager::Instance().SetPointLight(light, index);
	//	//	}

	//}
}