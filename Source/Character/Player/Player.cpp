#include "Player.h"
#include "States/PlayerState.h"
#include "States/PlayerAttackStates.h"
#include "States/PlayerGuardStates.h"
#include "States/PlayerMoveStates.h"
#include "States/PlayerDamageStates.h"
#include "System/HitStop.h"
#include <imgui.h>
#include "Character/Enemy/EnemyBoss.h"
#include "Math/Collision.h"
#include "Math/Mathf.h"
#include "System/AnimationConfigLoader.h"


#include <stdlib.h>

namespace
{
    // ステージの高さに合わせた初期配置
    const DirectX::XMFLOAT3 SPAWN_POSITION = { -1.0f, -1.432f, -15.0f };

    // 体力
    constexpr int MAX_HEALTH = 300;

    // 移動可能範囲の半径
    constexpr float AREA_LIMIT_RADIUS = 21.75f;

    // 使用するアセット
    constexpr const char* SWORD_MODEL_PATH = "Data/Model/Weapon/Katana/GreenKatana.gltf";
    constexpr const char* GUARD_EFFECT_PATH = "Data/Effect/Guard.efkefc";
    constexpr const char* DEATH_EFFECT_PATH = "Data/Effect/Death.efkefc";

    // 刀を持たせるノード名
    constexpr const char* SWORD_ATTACH_NODE_NAME = "Character1_RightHand";

    // カメラ前方向のXZ成分がこれ以下なら、真上・真下を向いているとみなす
    constexpr float CAMERA_FRONT_XZ_MIN_LENGTH = 0.01f;

    // カメラ上方向のXZ成分をゼロとみなす閾値
    constexpr float CAMERA_UP_XZ_MIN_LENGTH = 0.001f;

    // ロックオン旋回時の旋回速度倍率
    constexpr float LOCK_ON_TURN_SPEED_SCALE = 10.0f;

    // ターゲットとの距離の2乗がこれ以下なら重なっているとみなす
    constexpr float LOCK_ON_MIN_DISTANCE_SQ = 0.00001f;

    // これ以下の回転量は回転させない
    constexpr float LOCK_ON_MIN_ROTATION = 0.0001f;

    // ダメージ方向ベクトルが未設定とみなす長さ
    constexpr float DAMAGE_DIRECTION_MIN_LENGTH = 0.001f;

    // 打ち上げ時に敵を配置する前方距離
    constexpr float LAUNCH_KNOCKBACK_DISTANCE = 1.5f;

    // 移動範囲デバッグ表示用円柱の高さ
    constexpr float AREA_DEBUG_CYLINDER_HEIGHT = 5.0f;
}

static Player* instance = nullptr;

// インスタンス取得
Player& Player::Instance()
{
    return *instance;
}

// コンストラクタ
Player::Player(ID3D11Device* device, const char* filename, float scale)
{
    // インスタンスポインタ設定
    instance = this;

    model = std::make_shared<Model>(device, filename, scale);
    model->SetAdMetalness(1.0f);
    model->SetAdRoughness(1.0f);

    // ステージの高さに合わせる
    position = SPAWN_POSITION;

    // アニメーションスピード設定
    InitAnimSpeed();

    sword = std::make_unique<Sword>(device, SWORD_MODEL_PATH);
    guardEffect = std::make_unique<Effect>(GUARD_EFFECT_PATH);
    deathEffect = std::make_shared<Effect>(DEATH_EFFECT_PATH);

    // プレイヤーの最大体力と体力設定
    maxHealth = MAX_HEALTH;
    health = maxHealth;

    // ステートマシーンの生成
    states[static_cast<int>(PlayerStateId::Idle)] = std::make_unique<PlayerIdleState>(this);
    states[static_cast<int>(PlayerStateId::Walk)] = std::make_unique<PlayerWalkState>(this);
    states[static_cast<int>(PlayerStateId::Run)] = std::make_unique<PlayerRunState>(this);
    states[static_cast<int>(PlayerStateId::Jump)] = std::make_unique<PlayerJumpState>(this);
    states[static_cast<int>(PlayerStateId::Fall)] = std::make_unique<PlayerFallState>(this);
    states[static_cast<int>(PlayerStateId::Dodge)] = std::make_unique<PlayerDodgeState>(this);
    states[static_cast<int>(PlayerStateId::Combo1)] = std::make_unique<PlayerCombo1State>(this);
    states[static_cast<int>(PlayerStateId::Combo2)] = std::make_unique<PlayerCombo2State>(this);
    states[static_cast<int>(PlayerStateId::Combo3)] = std::make_unique<PlayerCombo3State>(this);
    states[static_cast<int>(PlayerStateId::Combo4)] = std::make_unique<PlayerCombo4State>(this);
    states[static_cast<int>(PlayerStateId::Combo5)] = std::make_unique<PlayerCombo5State>(this);
    states[static_cast<int>(PlayerStateId::Heavy1)] = std::make_unique<PlayerHeavyAttack1State>(this);
    states[static_cast<int>(PlayerStateId::GuardIdle)] = std::make_unique<PlayerGuardIdle>(this);
    states[static_cast<int>(PlayerStateId::GuardHit)] = std::make_unique<PlayerGuardHit>(this);
    states[static_cast<int>(PlayerStateId::GuardCounter)] = std::make_unique<PlayerGuardCounter>(this);
    states[static_cast<int>(PlayerStateId::Damage)] = std::make_unique<PlayerDamageState>(this);
    states[static_cast<int>(PlayerStateId::Dead)] = std::make_unique<PlayerDeadState>(this);

    // アイドルステートから開始
    ChangeState(PlayerStateId::Idle);

    // 生成直後はブレンド元の姿勢がバインドポーズ(Tポーズ)なので、
    // そのままだとイントロ演出中にTポーズから待機モーションへの補間が見えてしまう。
    // ブレンドを打ち切って、最初のフレームから待機姿勢にする。
    model->CancelAnimationBlend();

    // アニメーションごとのパラメーター設定
    const std::vector<ModelResource::Animation>& animations = model->GetResource()->GetAnimations();
    for (int i = 0; i < animations.size(); i++)
    {
        const AnimationConfig* config = AnimationConfigLoader::GetConfig("Player", i);
        if (config != nullptr)
            model->SetAnimationConfig(*config);
    }

    // プレイヤーの範囲制限
    areaSize = AREA_LIMIT_RADIUS;
}

Player::~Player()
{
}


void Player::Update(float elapsedTime)
{
    if (currentStateId != PlayerStateId::EnumCount)
    {
#if _DEBUG
        // エディタの Game View にフォーカスがあるときは、
        // ImGui がマウスを掴んでいても操作を通す。
        // （全画面ドックスペースではカーソルが常に ImGui 上にあるため）
        ImGuiIO& io = ImGui::GetIO();
        if (inputForced || (!io.WantCaptureMouse && !io.WantCaptureKeyboard)) {
            GetState(currentStateId).Update(elapsedTime);
        }
#else
        GetState(currentStateId).Update(elapsedTime);
#endif
    }

    int currentIndex = this->model->GetCurrentAnimationIndex();
    const AnimationConfig* config = model->GetAnimationConfig("Player", currentIndex);
    float animationSeconds = model->GetCurrentAnimationSeconds();
    float secondsLength = model->GetAnimationLength(currentIndex);
    float t = animationSeconds / secondsLength;
    t = std::clamp(t, 0.0f, 1.0f);  // 念のため 0.0～1.0 にクランプ
    float speed = model->EvaluateSpeed(config->speedCurve, t);

    model->SetAnimationSpeed(speed);

	// 速力処理更新
    UpdateVelocity(elapsedTime);

    // 無敵時間更新
    UpdateInvincibleTimer(elapsedTime);

    // プレイヤーとエネミーの衝突処理
    CollisionPlayerVsEnemies();

    // アタッチメント
    sword->Attach(SWORD_ATTACH_NODE_NAME, model.get());

    // オブジェクト行列を更新
    UpdateTransform();

    SetKnockbackPosition();

    // アニメーション更新
	model->UpdateAnimation(elapsedTime, this);

    // モデル行列更新
    model->UpdateTransform(transform);

    sword->Update(elapsedTime);

    // 範囲制限
    KeepAreaLimit(position);
}

void Player::EditUpdate(float elapsedTime)
{
    int currentIndex = this->model->GetCurrentAnimationIndex();
    AnimationConfig* config = model->GetAnimationConfig("Player", currentIndex);
    float animationSeconds = model->GetCurrentAnimationSeconds();
    float secondsLength = model->GetAnimationLength(currentIndex);
    float t = animationSeconds / secondsLength;
    t = std::clamp(t, 0.0f, 1.0f);  // 念のため 0.0～1.0 にクランプ
    float speed = model->EvaluateSpeed(config->speedCurve, t);

    model->SetAnimationSpeed(speed);

    sword->AttackAnimationCollision(model.get(), config, this);

	// 速力処理更新
    UpdateVelocity(elapsedTime);

    // プレイヤーとエネミーの衝突処理
    CollisionPlayerVsEnemies();

    // アタッチメント
    sword->Attach(SWORD_ATTACH_NODE_NAME, model.get());

    // オブジェクト行列を更新
    UpdateTransform();

    // アニメーション更新
    model->UpdateAnimation(elapsedTime, this);

    // モデル行列更新
    model->UpdateTransform(transform);

    sword->Update(elapsedTime);

    // 範囲制限
    KeepAreaLimit(position);
}

// スティック入力値から移動ベクトルを取得
DirectX::XMFLOAT3 Player::GetMoveVec() const
{
    // 入力情報を取得
    GamePad& gamePad = Input::Instance().GetGamePad();
    float ax = gamePad.GetAxisLX();
    float ay = gamePad.GetAxisLY();

    // カメラ方向とスティックの入力値によって進行方向を計算する
    Camera& camera = Camera::Instance();
    const DirectX::XMFLOAT3& cameraRight = camera.GetRight();
    const DirectX::XMFLOAT3& cameraFront = camera.GetFront();

    // カメラ右方向ベクトルをXZ単位ベクトルに変換
    float cameraRightX = cameraRight.x;
    float cameraRightZ = cameraRight.z;
    float cameraRightLength = sqrtf(cameraRightX * cameraRightX + cameraRightZ * cameraRightZ);
    if (cameraRightLength > 0.0f)
    {
        // 単位ベクトル化
        cameraRightX /= cameraRightLength;
        cameraRightZ /= cameraRightLength;
    }

    // カメラ前方向ベクトルをXZ単位ベクトルに変換
    float cameraFrontX = cameraFront.x;
    float cameraFrontZ = cameraFront.z;
    float cameraFrontLength = sqrtf(cameraFrontX * cameraFrontX + cameraFrontZ * cameraFrontZ);

    // 閾値を少し大きめに設定して、完全に0でなくても垂直に近いなら回避処理を行う
    if (cameraFrontLength > CAMERA_FRONT_XZ_MIN_LENGTH)
    {
        // 通常時：単位ベクトル化
        cameraFrontX /= cameraFrontLength;
        cameraFrontZ /= cameraFrontLength;
    }
    else
    {
        // カメラがほぼ真上・真下を向いている場合
        // FrontベクトルのXZ成分が消失するため、代わりにUpベクトル（カメラの上方向）を利用する

        // 真下を向いている時、画面の上方向（スティック上）は、カメラのUpベクトル（頭の向き）と一致するため
        const DirectX::XMFLOAT3& cameraUp = camera.GetUp();
        float cameraUpX = cameraUp.x;
        float cameraUpZ = cameraUp.z;
        float cameraUpLength = sqrtf(cameraUpX * cameraUpX + cameraUpZ * cameraUpZ);

        if (cameraUpLength > CAMERA_UP_XZ_MIN_LENGTH)
        {
            cameraFrontX = cameraUpX / cameraUpLength;
            cameraFrontZ = cameraUpZ / cameraUpLength;

            // もし真上を見上げている場合(Front.y > 0)、Upベクトルは画面下方向（背中側）を向くことが多いので反転させる
            // (真下を見ているときは Front.y < 0 なのでそのまま使用)
            if (cameraFront.y > 0.0f)
            {
                cameraFrontX = -cameraFrontX;
                cameraFrontZ = -cameraFrontZ;
            }
        }
    }

    // スティックの水平入力値をカメラ右方向に反映し、
    // スティックの垂直入力値をカメラ前方方向に反映し、
    // 進行ベクトルを計算する
    DirectX::XMFLOAT3 vec;
    vec.x = ax * cameraRightX + ay * cameraFrontX;
    vec.z = ax * cameraRightZ + ay * cameraFrontZ;
    // Y軸方向には移動しない
    vec.y = 0.0f;

    return vec;
}

// 前方向の移動値
void Player::ForwardMove(float speed, float elapsedTime)
{
    // 前ベクトルを計算
    float forwardX = sinf(angle.y);
    float forwardZ = cosf(angle.y);

    // 前方向に移動
    position.x += forwardX * speed * elapsedTime;
    position.z += forwardZ * speed * elapsedTime;
}

// 後ろ方向の移動値
void Player::BackMove(float speed, float elapsedTime)
{
    // 後ろベクトルを計算
    float backX = -sinf(angle.y);
    float backZ = -cosf(angle.y);

    // 後ろ方向に移動
    position.x += backX * speed * elapsedTime;
    position.z += backZ * speed * elapsedTime;
}

// プレイヤーとエネミーとの衝突処理
void Player::CollisionPlayerVsEnemies()
{
    // プレイヤーが回避中なら処理を抜ける
    if (isRolling) return;

    if (invincibleTimer > 0.0f) return;

    if (isParry) return;

    for (auto& playerHitSphere : nodeHitSpheres)
    {
        Model::Node* playerNode = model->FindNode(playerHitSphere.nodeName);

        // ノード位置取得
        DirectX::XMFLOAT3 playerNodePosition;
        playerNodePosition = { playerNode->worldTransform._41, playerNode->worldTransform._42, playerNode->worldTransform._43 };

		EnemyBoss& boss = EnemyBoss::Instance();

        std::vector<NodeHitSphere> enemyNode = boss.GetNodeHitSpheres();
        for (auto& enemyHitSphere : enemyNode)
        {
            Model* enemyModel = boss.GetModel();
            Model::Node* enemyNode = enemyModel->FindNode(enemyHitSphere.nodeName);

            // ノード位置取得
            DirectX::XMFLOAT3 enemyNodePosition;
            enemyNodePosition = { enemyNode->worldTransform._41, enemyNode->worldTransform._42, enemyNode->worldTransform._43 };

            DirectX::XMFLOAT3 outPosition, hitPosition;
            if (!boss.IsTeleporting())
            {
	            if (Collision::IntersectSphereVsSphere(
				   enemyNodePosition,
				   enemyHitSphere.radius,
				   playerNodePosition,
				   playerHitSphere.radius,
				   outPosition,
				   hitPosition))
	            {
	            	DirectX::XMVECTOR Move;
	            	DirectX::XMVECTOR PlayerNodePosition = DirectX::XMLoadFloat3(&playerNodePosition);
	            	DirectX::XMVECTOR OutPosition = DirectX::XMLoadFloat3(&outPosition);

	            	Move = DirectX::XMVectorSubtract(OutPosition, PlayerNodePosition);
	            	Move = DirectX::XMVectorSetY(Move, 0.0f);
	            	DirectX::XMVECTOR PlayerPosition = DirectX::XMLoadFloat3(&this->position);
	            	PlayerPosition = DirectX::XMVectorAdd(PlayerPosition, Move);

	            	DirectX::XMStoreFloat3(&this->position, PlayerPosition);
	            	break;
	            }
            }
        }
    }
}

// ステート切り替え
void Player::ChangeState(PlayerStateId stateId)
{
    if (currentStateId != stateId)
    {
        lastStateId = currentStateId;
        if (lastStateId != PlayerStateId::EnumCount)
        {
            GetState(lastStateId).Exit();
        }

        currentStateId = stateId;
        GetState(currentStateId).Enter();
    }
}

// 移動処理
void Player::PlayerMove(float elapsedTime, float moveRate, float turnRate)
{
    // 進行ベクトル取得
    DirectX::XMFLOAT3 moveVec = GetMoveVec();

    // 移動処理
    Move(moveVec.x * moveRate, moveVec.z * moveRate, moveSpeed * moveRate);

    // 旋回処理
    Turn(elapsedTime, moveVec.x * turnRate, moveVec.z * turnRate, turnSpeed * turnRate);
}

void Player::PlayerTurn(float elapsedTime, float turnRate)
{
    // 進行ベクトル取得
    DirectX::XMFLOAT3 moveVec = GetMoveVec();

    // 旋回処理
    Turn(elapsedTime, moveVec.x * turnRate, moveVec.z * turnRate, turnSpeed * turnRate);
}

void Player::PlayerJump(float speed)
{
    if (IsGround())
    {
        Jump(speed);
    }
}

// ロックオン時敵の方を向く処理
bool Player::LockOnTurnToEnemy(float elapsedTime)
{
    // すでに向き終わっている場合は何もしない
    if (isTurnCompleted)
    {
        return true;
    }

    float turn = turnSpeed * LOCK_ON_TURN_SPEED_SCALE * elapsedTime;
    EnemyBoss& boss = EnemyBoss::Instance();

    // ターゲットに向く処理
    DirectX::XMVECTOR Position = DirectX::XMLoadFloat3(&position);
    DirectX::XMVECTOR Target = DirectX::XMLoadFloat3(&boss.GetPosition());
    DirectX::XMVECTOR Vec = DirectX::XMVectorSubtract(Target, Position);

    // ゼロベクトルでないなら回転処理
    DirectX::XMVECTOR LengthSq = DirectX::XMVector3LengthSq(Vec);
    float lengthSq;
    DirectX::XMStoreFloat(&lengthSq, LengthSq);

    if (lengthSq > LOCK_ON_MIN_DISTANCE_SQ)
    {
        // ターゲットまでのベクトルを単位ベクトル化
        Vec = DirectX::XMVector3Normalize(Vec);

        DirectX::XMFLOAT3 direction;
        direction.x = sinf(angle.y);
        direction.y = 0.0f;
        direction.z = cosf(angle.y);

        // 向いている方向ベクトルを算出
        DirectX::XMVECTOR Direction = DirectX::XMLoadFloat3(&direction);

        // 向いている方向とターゲットまでのベクトル内積(角度)を算出
        DirectX::XMVECTOR Dot = DirectX::XMVector3Dot(Direction, Vec);

        float dot;
        DirectX::XMStoreFloat(&dot, Dot);

        // 角度差をチェック
        // 内積から角度差を計算（ラジアン）
        float angleDifference = acosf(std::clamp(dot, -1.0f, 1.0f));

        // 閾値以内なら完了とみなす
        if (angleDifference <= turnCompletedThreshold)
        {
            isTurnCompleted = true;
            return true;
        }

        // 2つの単位ベクトルの角度が小さいほど1.0に近づくという性質を利用して回転速度を調整する
        float rot = 1.0f - dot;
        if (rot > turn)
        {
            rot = turn;
        }

        // 回転処理があるなら回転処理をする
        if (fabsf(rot) > LOCK_ON_MIN_ROTATION)
        {
            // 回転軸を算出
            DirectX::XMVECTOR Axis = DirectX::XMVector3Cross(Direction, Vec);

            // 回転軸と回転量から回転行列を算出
            DirectX::XMMATRIX Rotation = DirectX::XMMatrixRotationAxis(Axis, rot);

            // 現在の行列回転させる
            DirectX::XMMATRIX Transform = DirectX::XMLoadFloat4x4(&transform);
            Transform = DirectX::XMMatrixMultiply(Transform, Rotation);

            // Transformから前方ベクトルを取得（Z軸方向）
            DirectX::XMVECTOR forward = DirectX::XMVector3Normalize(Transform.r[2]);

            // Y軸角度（Yaw）を算出（XとZを使う）
            float angleY = atan2f(DirectX::XMVectorGetX(forward), DirectX::XMVectorGetZ(forward));

            // 現在の角度を取得し、Yを更新して設定
            angle.y = angleY;
        }
    }
    else
    {
        // ターゲットが非常に近い場合は完了とみなす
        isTurnCompleted = true;
        return true;
    }

    return false;  // まだ向き終わっていない
}

// 移動設定
void Player::SetMovement(DirectX::XMFLOAT3& vec, float moveRate)
{
    Move(vec.x * moveRate, vec.z * moveRate, moveSpeed * moveRate);
}

void Player::SetDamageDirection(const DirectX::XMFLOAT3& attackerPos)
{
    damageDirection.x = attackerPos.x - position.x;
    damageDirection.y = 0.0f;
    damageDirection.z = attackerPos.z - position.z;

    // 正規化
    float length = sqrtf(damageDirection.x * damageDirection.x + damageDirection.z * damageDirection.z);
    if (length > 0.0f)
    {
        damageDirection.x /= length;
        damageDirection.z /= length;
    }
}

DirectX::XMFLOAT3 Player::CalculateKnockbackPosition(float power)
{
    DirectX::XMFLOAT3 knockbackPos = position;

    float dirLength = sqrtf(damageDirection.x * damageDirection.x + damageDirection.z * damageDirection.z);

    if (dirLength > DAMAGE_DIRECTION_MIN_LENGTH)
    {
        // ダメージを受けた方向の逆方向（後方）に移動
        knockbackPos.x = position.x + damageDirection.x * power;
        knockbackPos.z = position.z + damageDirection.z * power;
    }
    else
    {
        // ダメージ方向が設定されていない場合は現在の向きの後方
        DirectX::XMFLOAT3 backVec = CharacterBack(angle);
        knockbackPos.x = position.x + backVec.x * power;
        knockbackPos.z = position.z + backVec.z * power;
    }

    knockbackPos.y = position.y;
    return knockbackPos;
}

// 着地した時に呼ばれる
void Player::OnLanding()
{
    // 今回は使わない
}

// ダメージを受けた時に呼ばれる
void Player::OnDamaged()
{
    // ダメージステートへ遷移
    ChangeState(PlayerStateId::Damage);
}

// 死亡した時に呼ばれる
void Player::OnDead()
{
    // 死亡ステートへ遷移
    ChangeState(PlayerStateId::Dead);
}

// 移動入力処理
float Player::InputMove(float elapsedTime)
{
    // 進行ベクトル取得
    DirectX::XMFLOAT3 moveVec = GetMoveVec();

    // 移動処理
    Move(moveVec.x, moveVec.z, moveSpeed);

    // 旋回処理
    Turn(elapsedTime, moveVec.x, moveVec.z, turnSpeed);

    // 進行ベクトルがゼロベクトルではない場合は入力された
    return sqrtf(moveVecX * moveVecX + moveVecZ * moveVecZ);
}

// ジャンプ入力処理
bool Player::InputJump()
{
    GamePad& gamePad = Input::Instance().GetGamePad();
    if (gamePad.GetButtonDown() & GamePad::BTN_A)
    {
        // ジャンプ回数制限
        if (jumpCount != jumpLimit)
        {
            // ジャンプ
            jumpCount++;
            Jump(jumpSpeed);

            // ジャンプ入力した
            return true;
        }
    }

    return false;
}

// ノックバック位置設定
void Player::SetKnockbackPosition()
{
    DirectX::SimpleMath::Vector3 vec = CharacterForward(angle);
	knockbackPosition = DirectX::SimpleMath::Vector3{
		position.x + vec.x * knockbackDistance,
		position.y,
		position.z + vec.z * knockbackDistance
	};
    lightKnockbackPosition = DirectX::SimpleMath::Vector3{
        position.x + vec.x * lightKnockbackDistance,
        position.y,
        position.z + vec.z * lightKnockbackDistance
    };
    heavyKnockbackPosition = DirectX::SimpleMath::Vector3{
        position.x + vec.x * heavyKnockbackDistance,
        position.y,
        position.z + vec.z * heavyKnockbackDistance
    };
    launchKnockbackPosition = DirectX::SimpleMath::Vector3{
        position.x + vec.x * LAUNCH_KNOCKBACK_DISTANCE,
        position.y + launchKnockbackRise,
        position.z + vec.z * LAUNCH_KNOCKBACK_DISTANCE
    };
}

// 描画処理
void Player::Render(const RenderContext& rc, ShaderId shaderId)
{
    ModelRenderer* modelRenderer = Graphics::Instance().GetModelRenderer();
    modelRenderer->Draw(shaderId, model);
    sword->Render(rc, ShaderId::PBR);
    modelRenderer->Render(rc);
}

void Player::ShadowRender(const RenderContext& rc, ShadowMap* shadowMap)
{
    shadowMap->Draw(rc, model.get());
    shadowMap->Draw(rc, sword->GetModel().get());
}

// デバッグプリミティブ描画
void Player::DrawDebugPrimitive()
{
    ShapeRenderer* shapeRenderer = Graphics::Instance().GetShapeRenderer();

    if (drawCollisionPrimitive)
	{
		// 衝突判定用のデバック球を描画
    	shapeRenderer->DrawSphere(position, radius, DirectX::XMFLOAT4(0, 0, 0, 1));

    	shapeRenderer->DrawSphere(knockbackPosition, radius, DirectX::XMFLOAT4(1, 0, 0, 1));
    	shapeRenderer->DrawSphere(lightKnockbackPosition, radius, DirectX::XMFLOAT4(1, 0, 0, 1));
    	shapeRenderer->DrawSphere(heavyKnockbackPosition, radius, DirectX::XMFLOAT4(1, 0, 0, 1));
    	shapeRenderer->DrawSphere(launchKnockbackPosition, radius, DirectX::XMFLOAT4(1, 0, 0, 1));

    	// 衝突判定用のデバック円柱を描画
    	shapeRenderer->DrawCylinder(position, radius, height, DirectX::XMFLOAT4(0, 0, 0, 1));

    	shapeRenderer->DrawCylinder(areaCenter, areaSize, AREA_DEBUG_CYLINDER_HEIGHT, { 0.0f, 0.0f, 0.0f, 1.0f });
	}

    // 全身に当たり判定を付与する
    AddCollisionSpheres(model, nodeHitSpheres);
}

// デバッグ用GUI描画
void Player::DrawDebugGUI()
{
    if (ImGui::Begin("Player", nullptr, ImGuiWindowFlags_None))
    {
        // 位置
        ImGui::DragFloat3("Position", &position.x, 0.10f, -1000, 1000);

        if (ImGui::CollapsingHeader(u8"敵ノックバック、テレポート位置設定", ImGuiTreeNodeFlags_DefaultOpen))
        {
	        ImGui::DragFloat("KnockbackPosition", &knockbackDistance, 0.01f, 0, 5.0f);
        	ImGui::DragFloat("LightKnockbackPosition", &lightKnockbackDistance, 0.01f, 0, 5.0f);
        	ImGui::DragFloat("HeavyKnockbackPosition", &heavyKnockbackDistance, 0.01f, 0, 10.0f);
        	ImGui::DragFloat("LaunchKnockbackPosition", &launchKnockbackRise, 0.01f, 0, 5.0f);
        }

        if (ImGui::CollapsingHeader(u8"ノックバック設定", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::DragFloat(u8"通常ノックバック強度", &normalKnockbackPower, 0.01f, 0.0f, 10.0f);
            ImGui::DragFloat(u8"軽ノックバック強度", &lightKnockbackPower, 0.01f, 0.0f, 10.0f);
            ImGui::DragFloat(u8"重ノックバック強度", &heavyKnockbackPower, 0.01f, 0.0f, 15.0f);
            ImGui::DragFloat(u8"打ち上げ高度", &launchKnockbackHeight, 0.01f, 0.0f, 10.0f);
            ImGui::DragFloat(u8"叩き落としノックバック強度", &knockdownKnockbackPower, 0.01f, 0.0f, 10.0f);

            ImGui::Separator();
            ImGui::Text(u8"ダメージ方向: (%.2f, %.2f, %.2f)", damageDirection.x, damageDirection.y, damageDirection.z);

            // テスト用ボタン
            if (ImGui::Button(u8"通常ダメージテスト"))
            {
                SetDamageDirection(EnemyBoss::Instance().GetPosition());
                SetDamage(true);
            }
            ImGui::SameLine();
            if (ImGui::Button(u8"軽ノックバックテスト"))
            {
                SetDamageDirection(EnemyBoss::Instance().GetPosition());
                SetLightDamage(true);
            }
            if (ImGui::Button(u8"重ノックバックテスト"))
            {
                SetDamageDirection(EnemyBoss::Instance().GetPosition());
                SetHeavyDamage(true);
            }
            ImGui::SameLine();
            if (ImGui::Button(u8"打ち上げテスト"))
            {
                SetDamageDirection(EnemyBoss::Instance().GetPosition());
                SetLaunchDamage(true);
            }
            if (ImGui::Button(u8"叩き落としテスト"))
            {
                SetDamageDirection(EnemyBoss::Instance().GetPosition());
                SetKnockdownDamage(true);
            }
        }

        // 回転
        ImGui::DragFloat3("angle", &angle.x, 0.01f);

        ImGui::DragFloat(u8"無敵時間", &invincibleTimer);

        // スケール
        ImGui::DragFloat3("Scale", &scale.x, 0.001f);

        //体力と最大体力を表示を表示
        ImGui::Text(u8"体力 %zu", health);  // 体力
        ImGui::Text(u8"最大体力 %zu", maxHealth);  // 最大体力

        int state = static_cast<int>(currentStateId);
        ImGui::DragInt("State", &state);

        for (int i = 0; i < static_cast<int>(PlayerStateId::EnumCount); ++i)
        {
            // デバッグ用GUI描画
            states[i]->DrawDebugGUI();
        }
        
        ImGui::Checkbox(u8"当たり判定描画フラグ", &drawCollisionPrimitive);

        if (ImGui::CollapsingHeader("Parameter"))
        {
            // 各パラメータ
            {
                ImGui::DragInt(u8"プレイヤー体力", &health);
                ImGui::DragFloat3(u8"プレイヤー速度", &velocity.x, 0.1f);
                ImGui::DragFloat(u8"プレイヤー重力", &gravity, 0.01f);
                ImGui::Checkbox(u8"プレイヤーが地面に着地しているか", &isGround);
                ImGui::DragFloat(u8"プレイヤー無敵時間", &invincibleTimer, 0.1f);
                ImGui::DragFloat(u8"プレイヤー摩擦力", &friction, 0.01f);
                ImGui::DragFloat(u8"プレイヤー加速度", &acceleration, 0.1f);
                ImGui::DragFloat(u8"プレイヤー最大移動速度", &maxSpeed, 0.1f);
                ImGui::DragFloat(u8"プレイヤーX方向ベクトル", &moveVecX, 0.1f);
                ImGui::DragFloat(u8"プレイヤーZ方向ベクトル", &moveVecZ, 0.1f);
                ImGui::DragFloat(u8"プレイヤー空中摩擦力", &airControl, 0.1f);
                ImGui::DragFloat(u8"プレイヤー登れる段差", &stepOffset, 0.1f);
                ImGui::DragFloat(u8"プレイヤー登れる角度", &slopeRate, 0.1f);
                ImGui::DragFloat3(u8"移動エリア中心点", &areaCenter.x, 0.01f, 0.0f);
                ImGui::DragFloat(u8"移動エリア範囲", &areaSize, 0.01f, 0.0f);
            }
            ImGui::Separator();

            // プレイヤー当たり判定調整
            for (size_t i = 0; i < nodeHitSpheres.size(); ++i)
            {
                ImGui::PushID(static_cast<int>(i)); // 各ノードごとにIDをプッシュして重複を防ぐ

                ImGui::Text(nodeHitSpheres[i].nodeName);

                // 半径の編集
                ImGui::DragFloat("Radius", &nodeHitSpheres[i].radius, 0.01f, 0.0f, 10.0f);

                ImGui::PopID();
            }
            ImGui::Separator();

            // プレイヤーモデルのパラメータ調整
            model->DebugGUI(u8"Player");
        }

        ImGui::Separator();
		sword->DrawDebugGUI();

    }
    ImGui::End();
}
