#include "player.h"
#include "PlayerState.h"
//#include "System/HitStop.h"
#include <imgui.h>
#include "Model/ResourceManager.h"
#include "Scene/SceneManager.h"
//#include "Enemy/EnemyManager.h"
#include "Character/Enemy/SilverDragonkin.h"
#include "Math/Collision.h"
#include "Math/Mathf.h"
#include "System/AnimationConfigLoader.h"
#include "Camera/CameraParam.h"

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
    position.x = -1.0f;
    //position.y = -2.4f;
    position.z = -15.0f;

    // アニメーションスピード設定
    initAnimSpeed();

    sword = std::make_unique<Sword>(device, "Data/Model/Weapon/Staff.gltf");

    // プレイヤーの最大体力と体力設定
    maxHealth = 70;
    health = maxHealth;

    //healthBar = std::make_unique<Sprite>(device);

    // ステートマシーンの生成
    states[static_cast<int>(PlayerStateId::Idle)] = std::make_unique<PlayerIdleState>(this);
    states[static_cast<int>(PlayerStateId::Walk)] = std::make_unique<PlayerWalkState>(this);
    states[static_cast<int>(PlayerStateId::Run)] = std::make_unique<PlayerRunState>(this);
    states[static_cast<int>(PlayerStateId::Jump)] = std::make_unique<PlayerJumpState>(this);
    states[static_cast<int>(PlayerStateId::Fall)] = std::make_unique<PlayerFallState>(this);
    states[static_cast<int>(PlayerStateId::Dodge)] = std::make_unique<PlayerDodgeState>(this);
    states[static_cast<int>(PlayerStateId::DodgeAttack)] = std::make_unique<PlayerDodgeAttackState>(this);
    states[static_cast<int>(PlayerStateId::Combo1)] = std::make_unique<PlayerCombo1State>(this);
    states[static_cast<int>(PlayerStateId::Combo2)] = std::make_unique<PlayerCombo2State>(this);
    states[static_cast<int>(PlayerStateId::Combo3)] = std::make_unique<PlayerCombo3State>(this);
    states[static_cast<int>(PlayerStateId::Combo4)] = std::make_unique<PlayerCombo4State>(this);
    states[static_cast<int>(PlayerStateId::Heavy1)] = std::make_unique<PlayerHeavyAttack1State>(this);
    states[static_cast<int>(PlayerStateId::Heavy2)] = std::make_unique<PlayerHeavyAttack2State>(this);
    states[static_cast<int>(PlayerStateId::GuardIdle)] = std::make_unique<PlayerGuardIdle>(this);
    states[static_cast<int>(PlayerStateId::GuardWalk)] = std::make_unique<PlayerGuardWalk>(this);
    states[static_cast<int>(PlayerStateId::GuardHit)] = std::make_unique<PlayerGuardHit>(this);
    states[static_cast<int>(PlayerStateId::GuardParry)] = std::make_unique<PlayerGuardParry>(this);
    states[static_cast<int>(PlayerStateId::Damage)] = std::make_unique<PlayerDamageState>(this);
    states[static_cast<int>(PlayerStateId::Dead)] = std::make_unique<PlayerDeadState>(this);

    // アイドルステートから開始
    ChangeState(PlayerStateId::Idle);

    // アニメーションごとのパラメーター設定
    const std::vector<ModelResource::Animation>& animations = model->GetResource()->GetAnimations();
    for (int i = 0; i < animations.size(); i++)
    {
        const AnimationConfig* config = AnimationConfigLoader::GetConfig("Player", i);
        if (config != nullptr)
            model->SetAnimationConfig(*config);
    }

    // プレイヤーの範囲制限
    areaSize = 29.5f;
}

Player::~Player()
{
    // 今は何もしない
}

#include <DirectXMath.h>
#include <random>
#include <chrono>

// 名前空間の省略
using namespace DirectX;

// ランダムカラーを返す関数
XMFLOAT4 GetRandomColorEvery1Seconds() {
    // 静的に乱数エンジンと分布を初期化（1回だけ初期化される）
    static std::mt19937 rng(static_cast<unsigned int>(
        std::chrono::steady_clock::now().time_since_epoch().count()));
    static std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    // 現在の時間を取得
    static auto lastUpdate = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();

    // 経過時間が3秒以上かチェック
    static XMFLOAT4 currentColor = { dist(rng), dist(rng), dist(rng), 1.0f };
    if (std::chrono::duration_cast<std::chrono::seconds>(now - lastUpdate).count() >= 1) {
        // 新しいランダムカラーを生成
        currentColor = { dist(rng), dist(rng), dist(rng), 1.0f };
        lastUpdate = now;
    }

    return currentColor;
}

// ランダムカラーを生成する関数
XMFLOAT4 GenerateRandomColor() {
    static std::mt19937 rng(static_cast<unsigned int>(
        std::chrono::steady_clock::now().time_since_epoch().count()));
    static std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    return XMFLOAT4(dist(rng), dist(rng), dist(rng), 1.0f); // RGBA
}

// 滑らかに変化するカラーを返す関数
XMFLOAT4 GetSmoothChangingColor() {
    // 静的な変数で前回のカラー、次のカラー、時間管理
    static XMFLOAT4 currentColor = GenerateRandomColor();
    static XMFLOAT4 nextColor = GenerateRandomColor();
    static auto lastUpdate = std::chrono::steady_clock::now();

    // 現在の時間を取得
    auto now = std::chrono::steady_clock::now();
    float elapsedTime = std::chrono::duration<float>(now - lastUpdate).count();

    // ラープの進行度（0.0～1.0）
    const float transitionTime = 2400.0f; // 3秒で次の色へ移行
    float t = elapsedTime / transitionTime;

    if (t >= 1.0f) {
        // 次のカラーに完全に到達した場合、新しいランダムカラーを生成
        currentColor = nextColor;
        nextColor = GenerateRandomColor();
        lastUpdate = now;
        t = 0.0f; // ラープ進行度をリセット
    }

    // XMVECTOR に変換してラープ計算
    XMVECTOR currentVec = XMLoadFloat4(&currentColor);
    XMVECTOR nextVec = XMLoadFloat4(&nextColor);
    XMVECTOR lerpedVec = XMVectorLerp(currentVec, nextVec, t);

    // 結果を XMFLOAT4 に変換して返す
    XMFLOAT4 smoothColor;
    XMStoreFloat4(&smoothColor, lerpedVec);
    return smoothColor;
}


void Player::Update(float elapsedTime)
{
    if (currentStateID != PlayerStateId::EnumCount)
    {
#if _DEBUG
        /*ImGuiIO& io = ImGui::GetIO();
        if (!io.WantCaptureMouse && !io.WantCaptureKeyboard) {*/
            GetState(currentStateID).Update(elapsedTime);
        //}
#else
        GetState(currentStateID).Update(elapsedTime);
#endif
    }

	// 速力処理更新
    UpdateVelocity(elapsedTime);

    // 無敵時間更新
    UpdateInvincibleTimer(elapsedTime);

    // プレイヤーとエネミーの衝突処理
    CollisionPlayerVsEnemies();

    // アタッチメント
    sword->Attach("Character1_RightHand", model.get());

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

    // カメラ方向とステックの入力値によって進行方向を計算する
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
    if (cameraFrontLength > 0.0f)
    {
        // 単位ベクトル化
        cameraFrontX /= cameraFrontLength;
        cameraFrontZ /= cameraFrontLength;
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
    float BackX = -sinf(angle.y);
    float BackZ = -cosf(angle.y);

    // 前方向に移動
    position.x += BackX * speed * elapsedTime;
    position.z += BackZ * speed * elapsedTime;
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

        //// 指定のノードと全ての敵を総当たりで衝突処理
        //EnemyManager& enemyManager = EnemyManager::Instance();
		SilverDragonkin& dragonkin = SilverDragonkin::Instance();

        //// 全てのプレイヤー攻撃判定と全ての敵の総当たりで衝突処理
        //int enemyCount = enemyManager.GetEnemyCount();
        //for (int i = 0; i < enemyCount; ++i)
        //{
        //    Enemy* enemy = enemyManager.GetEnemy(i);

        std::vector<NodeHitSphere> enemyNode = dragonkin.GetNodeHitSpheres();
        for (auto& enemyHitSphere : enemyNode)
        {
            Model* enemyModel = dragonkin.GetModel();
            Model::Node* enemyNode = enemyModel->FindNode(enemyHitSphere.nodeName);

            // ノード位置取得
            DirectX::XMFLOAT3 enemyNodePosition;
            enemyNodePosition = { enemyNode->worldTransform._41, enemyNode->worldTransform._42, enemyNode->worldTransform._43 };

            DirectX::XMFLOAT3 outPosition, hitPosition;
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

                //複数の判定と押し出ししてしまうと、何重にも位置が加算されて吹っ飛ぶので
                //最初にあたった判定のみ動作させる　
                // （ここは移動幅が大きいところで処理するように改良した方がいいかも）
                break;
            }
        }
        //}
    }
}

// ステート切り替え
void Player::ChangeState(PlayerStateId stateId)
{
    if (currentStateID != stateId)
    {
        lastStateID = currentStateID;
        if (lastStateID != PlayerStateId::EnumCount)
        {
            GetState(lastStateID).Exit();
        }

        currentStateID = stateId;
        GetState(currentStateID).Enter();
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

void Player::PlayerJump(float speed)
{
    if (IsGround())
    {
        Jump(speed);
    }
}

// ロックオン時敵の方を向く処理
void Player::LockOnTurnToEnemy(float elapsedTime)
{
    float turn = turnSpeed * elapsedTime;
    SilverDragonkin& dragon = SilverDragonkin::Instance();

    //if (dragon == nullptr) return;

    // ターゲットに向く処理
    DirectX::XMVECTOR Position = DirectX::XMLoadFloat3(&position);
    DirectX::XMVECTOR Target = DirectX::XMLoadFloat3(&dragon.GetPosition());
    DirectX::XMVECTOR Vec = DirectX::XMVectorSubtract(Target, Position);

    // ゼロベクトルでないなら回転処理
    DirectX::XMVECTOR LengthSq = DirectX::XMVector3LengthSq(Vec);
    float lengthSq;
    DirectX::XMStoreFloat(&lengthSq, LengthSq);
    if (lengthSq > 0.00001f)
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

        // 2つの単位ベクトルの角度が小さいほど1.0に近づくという性質を利用して回転速度を調整する
        float rot = 1.0f - dot;
        if (rot > turnSpeed)
        {
            rot = turnSpeed;
        }

        // 回転処理があるなら回転処理をする
        if (fabsf(rot) > 0.0001f)
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
}

// 移動設定
void Player::SetMovement(DirectX::XMFLOAT3& Vec, float moveRate)
{
    Move(Vec.x * moveRate, Vec.z * moveRate, moveSpeed * moveRate);
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

// 体力ゲージ表示
void Player::DisplayHealthBar()
{
    //// 体力ゲージ追加
    //DirectX::XMVECTOR positionVec = DirectX::XMLoadFloat3(&position);

    //// スクリーンサイズ取得
    //float screenWidth = Graphics::Instance().GetScreenWidth();
    //float screenHeight = Graphics::Instance().GetScreenHeight();

    //// 各行列を取得
    //DirectX::XMMATRIX View = DirectX::XMLoadFloat4x4(&Camera::Instance().GetView());
    //DirectX::XMMATRIX Projection = DirectX::XMLoadFloat4x4(&Camera::Instance().GetProjection());
    //DirectX::XMMATRIX World = DirectX::XMMatrixIdentity();

    //DirectX::XMVECTOR ScreenPosition;

    //// ワールド座標からスクリーン座標へ変換
    //ScreenPosition = DirectX::XMVector3Project(
    //    positionVec,
    //    0.0f,
    //    0.0f,
    //    screenWidth,
    //    screenHeight,
    //    0.0f,
    //    1.0f,
    //    Projection,
    //    View,
    //    World
    //);

    //// スクリーン座標
    //DirectX::XMFLOAT2 screenPosition;
    //DirectX::XMStoreFloat2(&screenPosition, ScreenPosition);

    //// ゲージ描画
    //const float gaugeWidth = 30.0f;
    //const float gaugeHeight = 5.0f;

    //ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();

    //healthBar->Render(dc,
    //    screenPosition.x - gaugeWidth,
    //    screenPosition.y - gaugeHeight,
    //    0.0f,
    //    gaugeWidth,
    //    gaugeHeight,
    //    0.0f,
    //    1.0f,
    //    0.0f,
    //    0.0f,
    //    1.0f
    //);
}

// 描画処理
void Player::Render(const RenderContext& rc, ShaderId shaderId)
{
    ModelRenderer* modelRenderer = Graphics::Instance().GetModelRenderer();
    modelRenderer->Draw(shaderId, model);
    modelRenderer->Draw(shaderId, sword->GetModel());
    modelRenderer->Render(rc);

    // 体力ゲージ表示
    //DisplayHealthBar();
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

    if (isCollisionRender)
    {
        // 衝突判定用のデバック球を描画
        shapeRenderer->DrawSphere(position, radius, DirectX::XMFLOAT4(0, 0, 0, 1));

        // 衝突判定用のデバック円柱を描画
        shapeRenderer->DrawCylinder(position, radius, height, DirectX::XMFLOAT4(0, 0, 0, 1));

        shapeRenderer->DrawCylinder(areaCenter, areaSize, 5.0f, { 0.0, 0.0f, 0.0f, 1.0f });

        // 全身に当たり判定を付与する
        AddCollisionSpheres(model, nodeHitSpheres);
    }
}

// デバッグ用GUI描画
void Player::DrawDebugGUI()
{
    if (ImGui::Begin("Player", nullptr, ImGuiWindowFlags_None))
    {
        // 位置
        ImGui::DragFloat3("Position", &position.x, 0.10f, -1000, 1000);

        // 回転
        DirectX::XMFLOAT3 a;
        a.x = DirectX::XMConvertToDegrees(angle.x);
        a.y = DirectX::XMConvertToDegrees(angle.y);
        a.z = DirectX::XMConvertToDegrees(angle.z);
        ImGui::DragFloat3("angle", &angle.x, 0.01f);

        ImGui::DragFloat(u8"無敵時間", &invincibleTimer);

        // スケール
        ImGui::DragFloat3("Scale", &scale.x, 0.001f);

        //体力と最大体力を表示を表示
        ImGui::Text(u8"体力 %zu", health);  // 体力
        ImGui::Text(u8"最大体力 %zu", maxHealth);  // 最大体力

        //model->DrawGui();

        int state = static_cast<int>(currentStateID);
        ImGui::DragInt("State", &state);

        for (int i = 0; i < static_cast<int>(PlayerStateId::EnumCount); ++i)
        {
            // デバッグ用GUI描画
            states[i]->DrawDebugGUI();
        }
        //model->DrawGui();
        ImGui::Checkbox(u8"当たり判定描画フラグ", &isCollisionRender);

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
                if (ImGui::DragFloat("Radius", &nodeHitSpheres[i].radius, 0.01f, 0.0f, 10.0f))
                {
                    nodeRadius[i] = nodeHitSpheres[i].radius; // nodeRadius 配列も更新
                }

                ImGui::PopID();
            }
            ImGui::Separator();

            // プレイヤーモデルのパラメータ調整
            model->DebugGui(u8"Player");
        }

        ImGui::Separator();
		sword->DrawDebugImGUi();

    }
    ImGui::End();
}
