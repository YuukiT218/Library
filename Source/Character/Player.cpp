#include "player.h"
//#include "System/HitStop.h"
#include <imgui.h>
#include "Model/ResourceManager.h"
#include "Scene/SceneManager.h"
//#include "Enemy/EnemyManager.h"
#include "Math/Collision.h"
#include "Math/Mathf.h"
//#include "System/AnimationConfigLoader.h"
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
    /*model->SetAdMetalness(1.0f);
    model->SetAdRoughness(0.0f);*/

    // ステージの高さに合わせる
    /*position.y = -2.7f;
    position.z = -10.0f;*/

    // アニメーションスピード設定
    initAnimSpeed();

    sword = std::make_unique<Sword>(device, "Data/Model/Weapon/Staff.glb");

    // プレイヤーの最大体力と体力設定
    maxHealth = 70;
    health = maxHealth;

    //healthBar = std::make_unique<Sprite>(device);

    // ステートマシーンの生成
    states[static_cast<int>(PlayerStateId::Idle)] = std::make_unique<PlayerIdleState>(this);
    states[static_cast<int>(PlayerStateId::Walk)] = std::make_unique<PlayerWalkState>(this);
    states[static_cast<int>(PlayerStateId::Run)] = std::make_unique<PlayerRunState>(this);
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
    /*for (int i = 0; i < animations.size(); i++)
    {
        const AnimationConfig* config = AnimationConfigLoader::GetConfig("Player", i);
        if (config != nullptr)
            model->SetAnimationConfig(*config);
    }*/

    // プレイヤーの範囲制限
    areaSize = 54.64f;
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

    //int currentIndex = this->model->GetCurrentAnimationIndex();
    ////const AnimationConfig* config = model->GetAnimationConfig("Player", currentIndex);
    //float animationSeconds = model->GetCurrentAnimationSeconds();
    //float secondsLength = model->GetAnimationLength(currentIndex);
    //float t = animationSeconds / secondsLength;
    //t = std::clamp(t, 0.0f, 1.0f);  // 念のため 0.0～1.0 にクランプ
    //float speed = model->EvaluateSpeed(config->speedCurve, t);

    //model->SetAnimationSpeed(speed);

    // 速力処理更新
    UpdateVelocity(elapsedTime);

    // 無敵時間更新
    UpdateInvincibleTimer(elapsedTime);

    // プレイヤーとエネミーの衝突処理
    CollisionPlayerVsEnemies();

    // アタッチメント
    sword->Attach("Character1_LeftHand", model.get());

    // オブジェクト行列を更新
    UpdateTransform();

    // アニメーション更新
    model->UpdateAnimation(elapsedTime);

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

        //// 全てのプレイヤー攻撃判定と全ての敵の総当たりで衝突処理
        //int enemyCount = enemyManager.GetEnemyCount();
        //for (int i = 0; i < enemyCount; ++i)
        //{
        //    Enemy* enemy = enemyManager.GetEnemy(i);

        //    std::vector<NodeHitSphere> enemyNode = enemy->GetNodeHitSpheres();
        //    for (auto& enemyHitSphere : enemyNode)
        //    {
        //        Model* enemyModel = enemy->GetModel();
        //        Model::Node* enemyNode = enemyModel->FindNode(enemyHitSphere.nodeName);

        //        // ノード位置取得
        //        DirectX::XMFLOAT3 enemyNodePosition;
        //        enemyNodePosition = { enemyNode->worldTransform._41, enemyNode->worldTransform._42, enemyNode->worldTransform._43 };

        //        DirectX::XMFLOAT3 outPosition, hitPosition;
        //        if (Collision::IntersectSphereVsSphere(
        //            enemyNodePosition,
        //            enemyHitSphere.radius,
        //            playerNodePosition,
        //            playerHitSphere.radius,
        //            outPosition,
        //            hitPosition))
        //        {
        //            DirectX::XMVECTOR Move;
        //            DirectX::XMVECTOR PlayerNodePosition = DirectX::XMLoadFloat3(&playerNodePosition);
        //            DirectX::XMVECTOR OutPosition = DirectX::XMLoadFloat3(&outPosition);

        //            Move = DirectX::XMVectorSubtract(OutPosition, PlayerNodePosition);
        //            Move = DirectX::XMVectorSetY(Move, 0.0f);
        //            DirectX::XMVECTOR PlayerPosition = DirectX::XMLoadFloat3(&this->position);
        //            PlayerPosition = DirectX::XMVectorAdd(PlayerPosition, Move);

        //            DirectX::XMStoreFloat3(&this->position, PlayerPosition);

        //            //複数の判定と押し出ししてしまうと、何重にも位置が加算されて吹っ飛ぶので
        //            //最初にあたった判定のみ動作させる　
        //            // （ここは移動幅が大きいところで処理するように改良した方がいいかも）
        //            break;
        //        }
        //    }
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

    /*if (CameraParam::Instance().GetIsLockOn())
    {
        LockOnTurnToEnemy(elapsedTime);
    }*/
    //else
    {
        // 旋回処理
        Turn(elapsedTime, moveVec.x * turnRate, moveVec.z * turnRate, turnSpeed * turnRate);
    }
}

// ロックオン時敵の方を向く処理
void Player::LockOnTurnToEnemy(float elapsedTime)
{
    //float turn = turnSpeed * elapsedTime;
    ////ElderDragon* dragon = EnemyManager::Instance().FindElderDragon();

    //if (dragon == nullptr) return;

    //// ターゲットに向く処理
    //DirectX::XMVECTOR Position = DirectX::XMLoadFloat3(&position);
    //DirectX::XMVECTOR Target = DirectX::XMLoadFloat3(&dragon->GetPosition());
    //DirectX::XMVECTOR Vec = DirectX::XMVectorSubtract(Target, Position);

    //// ゼロベクトルでないなら回転処理
    //DirectX::XMVECTOR LengthSq = DirectX::XMVector3LengthSq(Vec);
    //float lengthSq;
    //DirectX::XMStoreFloat(&lengthSq, LengthSq);
    //if (lengthSq > 0.00001f)
    //{
    //    // ターゲットまでのベクトルを単位ベクトル化
    //    Vec = DirectX::XMVector3Normalize(Vec);

    //    DirectX::XMFLOAT3 direction;
    //    direction.x = sinf(angle.y);
    //    direction.y = 0.0f;
    //    direction.z = cosf(angle.y);

    //    // 向いている方向ベクトルを算出
    //    DirectX::XMVECTOR Direction = DirectX::XMLoadFloat3(&direction);

    //    // 向いている方向とターゲットまでのベクトル内積(角度)を算出
    //    DirectX::XMVECTOR Dot = DirectX::XMVector3Dot(Direction, Vec);

    //    float dot;
    //    DirectX::XMStoreFloat(&dot, Dot);

    //    // 2つの単位ベクトルの角度が小さいほど1.0に近づくという性質を利用して回転速度を調整する
    //    float rot = 1.0f - dot;
    //    if (rot > turnSpeed)
    //    {
    //        rot = turnSpeed;
    //    }

    //    // 回転処理があるなら回転処理をする
    //    if (fabsf(rot) > 0.0001f)
    //    {
    //        // 回転軸を算出
    //        DirectX::XMVECTOR Axis = DirectX::XMVector3Cross(Direction, Vec);

    //        // 回転軸と回転量から回転行列を算出
    //        DirectX::XMMATRIX Rotation = DirectX::XMMatrixRotationAxis(Axis, rot);

    //        // 現在の行列回転させる
    //        DirectX::XMMATRIX Transform = DirectX::XMLoadFloat4x4(&transform);
    //        Transform = DirectX::XMMatrixMultiply(Transform, Rotation);

    //        // Transformから前方ベクトルを取得（Z軸方向）
    //        DirectX::XMVECTOR forward = DirectX::XMVector3Normalize(Transform.r[2]);

    //        // Y軸角度（Yaw）を算出（XとZを使う）
    //        float angleY = atan2f(DirectX::XMVectorGetX(forward), DirectX::XMVectorGetZ(forward));

    //        // 現在の角度を取得し、Yを更新して設定
    //        angle.y = angleY;
    //    }
    //}
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
    modelRenderer->Draw(ShaderId::Lambert, model);
    modelRenderer->Draw(ShaderId::Lambert, std::shared_ptr<Model>(sword->GetModel(), [](Model*) {}));
    modelRenderer->Render(rc);

    // 体力ゲージ表示
    //DisplayHealthBar();
}

//void Player::ShadowRender(const RenderContext& rc, ShadowMap* shadowMap)
//{
//    shadowMap->Draw(rc, model.get());
//    shadowMap->Draw(rc, weaponRight->GetModel());
//    shadowMap->Draw(rc, leftShield->GetModel());
//}
//
//void Player::SetShadowMap(ShadowMap* shadowMap)
//{
//    shadowMap->SetShadowModel(model.get());
//    shadowMap->SetShadowModel(weaponRight->GetModel());
//    shadowMap->SetShadowModel(leftShield->GetModel());
//}

// デバッグプリミティブ描画
void Player::DrawDebugPrimitive()
{
    ShapeRenderer* shapeRenderer = Graphics::Instance().GetShapeRenderer();

    if (isCollisionRender)
    {
        // 衝突判定用のデバック球を描画
        //debugRenderer->DrawSphere(position, radius, DirectX::XMFLOAT4(0, 0, 0, 1));

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
        ImGui::DragFloat3("Position", &position.x, 0.10f, -10, 10);

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
                ImGui::DragFloat(u8"プレイヤー最大移動速度", &maxMoveSpeed, 0.1f);
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

//-------------------------------------------------------------
// ステート基盤
//-------------------------------------------------------------
// コンストラクタ
PlayerState::PlayerState(Player* player)
    : player(player)
{
}

// ステート切り替え
void PlayerState::ChangeState(PlayerStateId stateId)
{
    player->ChangeState(stateId);
}

// コンボ入力
PlayerState::InputComboType PlayerState::InputCombo()
{
    GamePad& gamepad = Input::Instance().GetGamePad();
    const Mouse& mouse = Input::Instance().GetMouse();

    if (gamepad.GetButtonDown() & GamePad::BTN_Y || (gamepad.GetButton() & GamePad::BTN_LEFT_THUMB && mouse.GetButtonDown() & Mouse::BTN_LEFT)) return InputComboType::Heavy;
    if (gamepad.GetButtonDown() & GamePad::BTN_X || mouse.GetButtonDown() & Mouse::BTN_LEFT) return InputComboType::Light;
    return InputComboType::None;
}

// 回避入力
bool PlayerState::InputDodge() const
{
    const GamePad& gamepad = Input::Instance().GetGamePad();

    if (gamepad.GetButtonDown() & GamePad::BTN_A)
    {
        return true;
    }

    return false;
}

// 歩き移動処理
bool PlayerState::InputWalkMove() const
{
    const GamePad& gamepad = Input::Instance().GetGamePad();

    return gamepad.GetLAxisPower() > 0.2f && gamepad.GetLAxisPower() < 0.5f;
}

// 走り移動入力
bool PlayerState::InputRunMove() const
{
    const GamePad& gamepad = Input::Instance().GetGamePad();

    return gamepad.GetLAxisPower() > 0.5f;
}

// ガード入力
bool PlayerState::InputGuard() const
{
    const GamePad& gamepad = Input::Instance().GetGamePad();
    const Mouse& mouse = Input::Instance().GetMouse();

    // LBが押されている間
    if (gamepad.GetButton() & GamePad::BTN_LEFT_SHOULDER || mouse.GetButton() & Mouse::BTN_RIGHT)
    {
        return true;
    }

    return false;
}

// パリィ入力
bool PlayerState::InputGuardParry() const
{
    const GamePad& gamepad = Input::Instance().GetGamePad();
    const Mouse& mouse = Input::Instance().GetMouse();

    // Bボタンが押された瞬間
    if (gamepad.GetButtonDown() & GamePad::BTN_B || mouse.GetButtonDown() & Mouse::BTN_LEFT)
    {
        return true;
    }

    return false;
}

// ロックオンしている場合はストレイフ
void PlayerState::LockOnStrafe(int rightIndex, int leftIndex, int frontIndex, int backIndex)
{
    if (CameraParam::Instance().GetIsLockOn())
    {
        const GamePad& gamepad = Input::Instance().GetGamePad();
        if (gamepad.GetLAxisPower() > 0.1f)
        {
            float axisX = gamepad.GetAxisLX();
            float axisY = gamepad.GetAxisLY();

            int newAnimationIndex = -1;

            // 右前
            if (axisY > 0.1f && axisX > 0.1f)
                newAnimationIndex = rightIndex;
            // 左前
            else if (axisY > 0.1f && axisX < -0.1f)
                newAnimationIndex = leftIndex;
            // 右後ろ
            else if (axisY < -0.1f && axisX > 0.1f)
                newAnimationIndex = rightIndex;
            // 左後ろ
            else if (axisY < -0.1f && axisX < -0.1f)
                newAnimationIndex = leftIndex;
            // 前
            else if (axisY > 0.1f)
                newAnimationIndex = frontIndex;
            // 後ろ
            else if (axisY < -0.1f)
                newAnimationIndex = backIndex;
            // 右
            else if (axisX > 0.1f)
                newAnimationIndex = rightIndex;
            // 左
            else if (axisX < -0.1f)
                newAnimationIndex = leftIndex;

            if (newAnimationIndex != -1 &&
                newAnimationIndex != player->GetPlayerModel()->GetCurrentAnimationIndex())
            {
                player->GetPlayerModel()->PlayAnimation(newAnimationIndex, true, 0.1f);
            }
        }
    }
    else
    {
        if (frontIndex != player->GetPlayerModel()->GetCurrentAnimationIndex())
        {
            player->GetPlayerModel()->PlayAnimation(frontIndex, true, 0.1f);
        }
    }
}

//-------------------------------------------------------------
// 待機ステート
//-------------------------------------------------------------
// コンストラクタ
PlayerIdleState::PlayerIdleState(Player* player)
    : PlayerState(player)
{
    idleAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("Idle");
}

// 開始処理
void PlayerIdleState::Enter()
{
    player->GetPlayerModel()->PlayAnimation(idleAnimationIndex, true, 0.3f);
}

// 更新処理
void PlayerIdleState::Update(float elapsedTime)
{
    if (CameraParam::Instance().GetIsLockOn())
    {
        player->LockOnTurnToEnemy(elapsedTime);
    }

    // コンボ1ステートに遷移
    if (InputCombo() == InputComboType::Light)
    {
        ChangeState(PlayerStateId::Combo1);
    }
    // 強攻撃1ステートに遷移
    else if (InputCombo() == InputComboType::Heavy)
    {
        ChangeState(PlayerStateId::Heavy1);
    }
    // 回避ステートに遷移
    else if (InputDodge())
    {
        ChangeState(PlayerStateId::Dodge);
    }
    // 走りステートに遷移
    else if (InputRunMove())
    {
        ChangeState(PlayerStateId::Run);
    }
    // 歩きステートに遷移
    else if (InputWalkMove())
    {
        ChangeState(PlayerStateId::Walk);
    }
    // ガード待機に遷移
    else if (InputGuard())
    {
        ChangeState(PlayerStateId::GuardIdle);
    }

    //player->GetPlayerModel()->SetBaseAnimationSpeed(idleAnimationSpeed);
}

// デバッグ用GUI描画
void PlayerIdleState::DrawDebugGUI()
{
    ImGui::Separator();

    if (ImGui::TreeNode(u8"待機"))
    {
        //ImGui::Text(player->GetPlayerModel()->GetAnimationName(idleAnimationIndex));
        //ImGui::SameLine();
        //ImGui::Text(u8"終了フレーム:%.3f", player->GetPlayerModel()->GetAnimationLength(idleAnimationIndex));
        ImGui::DragFloat(u8"アニメーションスピード", &idleAnimationSpeed, 0.01f, 0.0f, 5.0f);
        ImGui::TreePop();
    }
}

//-------------------------------------------------------------
// 歩きステート
//-------------------------------------------------------------
// コンストラクタ
PlayerWalkState::PlayerWalkState(Player* player)
    : PlayerState(player)
{
    walkFrontAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("WalkForwardInPlace");
    walkBackAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("WalkForwardInPlace");
    walkRightAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("WalkForwardInPlace");
    walkLeftAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("WalkForwardInPlace");
}

// 開始処理
void PlayerWalkState::Enter()
{
}

// 更新処理
void PlayerWalkState::Update(float elapsedTime)
{
    LockOnStrafe(walkRightAnimationIndex, walkLeftAnimationIndex, walkFrontAnimationIndex, walkBackAnimationIndex);

    player->PlayerMove(elapsedTime, walkAnimationMoveRate);

    float speed = DirectX::XMVectorGetX(XMVector3LengthSq(DirectX::XMLoadFloat3(&player->GetMoveVec())));

    float t = std::clamp(speed, 0.0f, 1.0f);

    walkAnimationSpeed = Mathf::Lerp(0.2f, 0.8f, t);

    // コンボ1ステートに遷移
    if (InputCombo() == InputComboType::Light)
    {
        ChangeState(PlayerStateId::Combo1);
    }
    // 強攻撃1ステートに遷移
    else if (InputCombo() == InputComboType::Heavy)
    {
        ChangeState(PlayerStateId::Heavy1);
    }
    // 回避ステートに遷移
    else if (InputDodge())
    {
        ChangeState(PlayerStateId::Dodge);
    }
    // ガード待機ステート遷移
    else if (InputGuard())
    {
        ChangeState(PlayerStateId::GuardIdle);
    }
    // 走りステートに遷移
    else if (InputRunMove())
    {
        ChangeState(PlayerStateId::Run);
    }
    // アイドルステートに遷移
    else if (!InputWalkMove() && !InputRunMove())
    {
        ChangeState(PlayerStateId::Idle);
    }

    //player->GetPlayerModel()->SetBaseAnimationSpeed(walkAnimationSpeed);
}

// デバッグ用GUI描画
void PlayerWalkState::DrawDebugGUI()
{
    ImGui::Separator();

    if (ImGui::TreeNode(u8"歩き"))
    {
        //ImGui::Text(player->GetPlayerModel()->GetAnimationName(walkFrontAnimationIndex));
        //ImGui::SameLine();
        //ImGui::Text(u8"終了フレーム:%.3f", player->GetPlayerModel()->GetAnimationLength(walkFrontAnimationIndex));
        ImGui::DragFloat(u8"アニメーションスピード", &walkAnimationSpeed, 0.01f, 0.0f, 5.0f);
        ImGui::DragFloat(u8"移動率", &walkAnimationMoveRate, 0.01f, 0.0f, 5.0f);
        ImGui::TreePop();
    }
}

//-------------------------------------------------------------
// 走りステート
//-------------------------------------------------------------
// コンストラクタ
PlayerRunState::PlayerRunState(Player* player)
    : PlayerState(player)
{
    runAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("RunForwardInPlace");
    runFrontAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("WarriorRunForward");
    runBackAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("WarriorRunBackward");
    runRightAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("WarriorRunRightward");
    runLeftAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("WarriorRunLeftward");
}

// 開始処理
void PlayerRunState::Enter()
{
    if (!CameraParam::Instance().GetIsLockOn())
    {
        player->GetPlayerModel()->PlayAnimation(runAnimationIndex, true, 0.1f);
    }
}

// 更新処理
void PlayerRunState::Update(float elapsedTime)
{
    if (CameraParam::Instance().GetIsLockOn())
    {
        LockOnStrafe(runRightAnimationIndex, runLeftAnimationIndex, runFrontAnimationIndex, runBackAnimationIndex);
    }

    player->PlayerMove(elapsedTime, runAnimationMoveRate);

    // コンボ1ステートに遷移
    if (InputCombo() == InputComboType::Light)
    {
        ChangeState(PlayerStateId::Combo1);
    }
    // 強攻撃1ステートに遷移
    else if (InputCombo() == InputComboType::Heavy)
    {
        ChangeState(PlayerStateId::Heavy1);
    }
    // 回避ステートに遷移
    else if (InputDodge())
    {
        ChangeState(PlayerStateId::Dodge);
    }
    // ガード待機ステート遷移
    else if (InputGuard())
    {
        ChangeState(PlayerStateId::GuardIdle);
    }
    // 走りステートに遷移
    else if (InputWalkMove())
    {
        ChangeState(PlayerStateId::Walk);
    }
    // アイドルステートに遷移
    else if (!InputWalkMove() && !InputRunMove())
    {
        ChangeState(PlayerStateId::Idle);
    }

    //player->GetPlayerModel()->SetBaseAnimationSpeed(runAnimationSpeed);
}

// デバッグ用GUI描画
void PlayerRunState::DrawDebugGUI()
{
    ImGui::Separator();

    if (ImGui::TreeNode(u8"走り"))
    {
        //ImGui::Text(player->GetPlayerModel()->GetAnimationName(runFrontAnimationIndex));
        //ImGui::SameLine();
        //ImGui::Text(u8"終了フレーム:%.3f", player->GetPlayerModel()->GetAnimationLength(runFrontAnimationIndex));
        ImGui::DragFloat(u8"アニメーションスピード", &runAnimationSpeed, 0.01f, 0.0f, 5.0f);
        ImGui::DragFloat(u8"移動率", &runAnimationMoveRate, 0.01f, 0.0f, 5.0f);
        ImGui::TreePop();
    }
}

//-------------------------------------------------------------
// 回避ステート
//-------------------------------------------------------------
// コンストラクタ
PlayerDodgeState::PlayerDodgeState(Player* player)
    : PlayerState(player)
{
    dodgeBackAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("WarriorDodge");
    rollingFrontAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("WarriorEvadeForward");
    rollingBackAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("WarriorEvadeBackward");
    rollingRightAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("WarriorEvadeRight");
    rollingLeftAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("WarriorEvadeLeft");
    rollingFrontRightAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("WarriorEvadeForwardRight");
    rollingFrontLeftAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("WarriorEvadeForwardLeft");
    rollingBackRightAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("WarriorEvadeBackwardRight");
    rollingBackLeftAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("WarriorEvadeBackwardLeft");
}

// 開始処理
void PlayerDodgeState::Enter()
{
    const Camera& camera = Camera::Instance();
    const GamePad& gamepad = Input::Instance().GetGamePad();

    DirectX::XMVECTOR Vec;
    float moveRate;
    if (gamepad.GetLAxisPower() > 0.1f)
    {
        // 入力方向へ回避
        float axisX = gamepad.GetAxisLX();
        float axisY = gamepad.GetAxisLY();

        // ワールド進行方向を取得
        Vec = DirectX::XMLoadFloat3(&player->ComputeWorldVec(camera, axisX, axisY));
        Vec = DirectX::XMVector3Normalize(Vec);

        if (CameraParam::Instance().GetIsLockOn())
        {
            // 右前
            if (axisY > 0.1f && axisX > 0.1f)
                player->GetPlayerModel()->PlayAnimation(rollingFrontRightAnimationIndex, false, 0.1f);
            // 左前
            else if (axisY > 0.1f && axisX < -0.1f)
                player->GetPlayerModel()->PlayAnimation(rollingFrontLeftAnimationIndex, false, 0.1f);
            // 右後ろ
            else if (axisY < -0.1f && axisX > 0.1f)
                player->GetPlayerModel()->PlayAnimation(rollingBackRightAnimationIndex, false, 0.1f);
            // 左後ろ
            else if (axisY < -0.1f && axisX < -0.1f)
                player->GetPlayerModel()->PlayAnimation(rollingBackLeftAnimationIndex, false, 0.1f);
            // 前
            else if (axisY > 0.1f)
                player->GetPlayerModel()->PlayAnimation(rollingFrontAnimationIndex, false, 0.1f);
            // 後ろ
            else if (axisY < -0.1f)
                player->GetPlayerModel()->PlayAnimation(rollingBackAnimationIndex, false, 0.1f);
            // 右
            else if (axisX > 0.1f)
                player->GetPlayerModel()->PlayAnimation(rollingRightAnimationIndex, false, 0.1f);
            // 左
            else if (axisX < -0.1f)
                player->GetPlayerModel()->PlayAnimation(rollingLeftAnimationIndex, false, 0.1f);
        }
        else
        {
            player->GetPlayerModel()->PlayAnimation(rollingFrontAnimationIndex, false, 0.1f);
        }

        timer = rollingFrontAnimationTime;

        // 回避
        moveRate = rollingFrontMovePow;
    }
    else
    {
        Vec = DirectX::XMLoadFloat3(&player->CharacterBack(player->GetAngle()));
        Vec = DirectX::XMVector3Normalize(Vec);

        player->GetPlayerModel()->PlayAnimation(dodgeBackAnimationIndex, false, 0.1f);

        timer = dodgeBackAnimationTime;

        isDodgeBack = true;

        // 回避
        moveRate = dodgeBackMovePow;
    }

    player->SetPlayerRolling(true);

    nextShiftReady = false;

    DirectX::XMFLOAT3 vec;
    DirectX::XMStoreFloat3(&vec, Vec);

    player->SetMovement(vec, moveRate);
}

// 更新処理
void PlayerDodgeState::Update(float elapsedTime)
{
    timer -= elapsedTime;
    if (timer <= 0.3f)
    {
        // 走りステート遷移
        if (InputRunMove())
        {
            ChangeState(PlayerStateId::Run);
        }
        // 歩きステート遷移
        else if (InputWalkMove())
        {
            ChangeState(PlayerStateId::Walk);
        }
        // 待機ステート遷移
        else
        {
            ChangeState(PlayerStateId::Idle);
        }
    }

    if (isDodgeBack)
    {
        if (InputCombo() != InputComboType::None)
        {
            nextShiftReady = true;
        }

        if (nextShiftReady)
        {
            if (timer <= 0.4f)
            {
                isDodgeBack = false;

                ChangeState(PlayerStateId::DodgeAttack);
            }
        }
    }

    //player->GetPlayerModel()->SetBaseAnimationSpeed(dodgeBackAnimationSpeed);
}

// 終了処理
void PlayerDodgeState::Exit()
{
    player->SetPlayerRolling(false);
}

// デバッグ用GUI描画
void PlayerDodgeState::DrawDebugGUI()
{
    ImGui::Separator();

    if (ImGui::TreeNode(u8"バックステップ回避"))
    {
        //ImGui::Text(player->GetPlayerModel()->GetAnimationName(dodgeBackAnimationIndex));
        //ImGui::SameLine();
        //ImGui::Text(u8"終了フレーム:%.3f", player->GetPlayerModel()->GetAnimationLength(dodgeBackAnimationIndex));
        //ImGui::DragFloat(u8"アニメーションスピード", &dodgeBackAnimationSpeed, 0.01f, 0.0f, 5.0f);
        ImGui::DragFloat(u8"アニメーション遷移時間", &dodgeBackAnimationTime, 0.01f, 0.0f, 5.0f);
        ImGui::DragInt(u8"移動距離", &dodgeBackMovePow);
        ImGui::TreePop();
    }

    ImGui::Separator();

    if (ImGui::TreeNode(u8"ローリング回避"))
    {
        //ImGui::Text(player->GetPlayerModel()->GetAnimationName(rollingFrontAnimationIndex));
        //ImGui::SameLine();
        //ImGui::Text(u8"終了フレーム:%.3f", player->GetPlayerModel()->GetAnimationLength(rollingFrontAnimationIndex));
        //ImGui::DragFloat(u8"アニメーションスピード", &dodgeBackAnimationSpeed, 0.01f, 0.0f, 5.0f);
        ImGui::DragFloat(u8"アニメーション遷移時間", &rollingFrontAnimationTime, 0.01f, 0.0f, 5.0f);
        ImGui::DragInt(u8"移動距離", &rollingFrontMovePow);
        ImGui::TreePop();
    }
}

//-------------------------------------------------------------
// 回避攻撃ステート
//-------------------------------------------------------------
// コンストラクタ
PlayerDodgeAttackState::PlayerDodgeAttackState(Player* player)
    : PlayerState(player)
{
    dodgeAttackAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("WarriorAttackDodge");
}

// 開始処理
void PlayerDodgeAttackState::Enter()
{
    DirectX::XMVECTOR Vec;

    player->GetPlayerModel()->PlayAnimation(dodgeAttackAnimationIndex, false, 0.1f);
    timer = 0.95f;

    Vec = DirectX::XMLoadFloat3(&player->CharacterForward(player->GetAngle()));
    Vec = DirectX::XMVector3Normalize(Vec);

    // 回避
    Vec = DirectX::XMVectorScale(Vec, 20.0f);

    DirectX::XMFLOAT3 vec;
    DirectX::XMStoreFloat3(&vec, Vec);

    player->SetMovement(vec, 2.0f);
}

// 更新処理
void PlayerDodgeAttackState::Update(float elapsedTime)
{
    /*int index = player->GetPlayerModel()->GetCurrentAnimationIndex();
    AnimationConfig* config = player->GetPlayerModel()->GetAnimationConfig("Player", index);
    player->GetWeaponRight()->AttackAnimationCollision(player->GetModel(), config);*/

    timer -= elapsedTime;
    if (timer <= 0.0f)
    {
        // 走りステート遷移
        if (InputRunMove())
        {
            ChangeState(PlayerStateId::Run);
        }
        // 歩きステート遷移
        else if (InputWalkMove())
        {
            ChangeState(PlayerStateId::Walk);
        }
        // 待機ステート遷移
        else
        {
            ChangeState(PlayerStateId::Idle);
        }
    }
}

//-------------------------------------------------------------
// コンボステート
//-------------------------------------------------------------
// コンストラクタ
PlayerComboState::PlayerComboState(Player* player)
    : PlayerState(player)
{
}

// 開始処理
void PlayerComboState::Enter()
{
    forwarded = false;
    nextShiftReady = false;
    player->GetPlayerModel()->PlayAnimation(comboAnimationIndex, false, 0.1f);
}

// 終了処理
void PlayerComboState::Exit()
{
    nextShiftReady = false;
}

// 更新処理
void PlayerComboState::Update(float elapsedTime)
{
    float frame = player->GetPlayerModel()->GetCurrentAnimationSeconds();
    int index = player->GetPlayerModel()->GetCurrentAnimationIndex();

    //AnimationConfig* config = player->GetPlayerModel()->GetAnimationConfig("Player", index);

    if (CameraParam::Instance().GetIsLockOn())
    {
        player->LockOnTurnToEnemy(elapsedTime);
    }

    // 指定フレームを超えたら前進処理
    //if (!forwarded)
    //{
    //    if (frame >= config->attackParam.forwardFrame)
    //    {
    //        const float aimRange = 7.0f;
    //        float rate = 1.0f; // デフォルトの移動倍率

    //        DirectX::XMVECTOR playerPos = DirectX::XMLoadFloat3(&player->GetPosition());
    //        Enemy* enemy = player->GetLockOnEnemy();

    //        if (enemy)
    //        {
    //            DirectX::XMVECTOR enemyPos = DirectX::XMLoadFloat3(&enemy->GetPosition());
    //            DirectX::XMVECTOR diffVec = DirectX::XMVectorSubtract(enemyPos, playerPos);

    //            float distSq;
    //            DirectX::XMStoreFloat(&distSq, DirectX::XMVector3LengthSq(diffVec));

    //            // 敵との距離が範囲内なら距離に応じて前進力を調整
    //            if (distSq < aimRange * aimRange)
    //            {
    //                float dist = sqrtf(distSq);
    //                rate = dist / aimRange;  // 0～1の範囲でスケール
    //            }
    //        }

    //        DirectX::XMFLOAT3 front = player->CharacterForward(player->GetAngle());
    //        DirectX::XMFLOAT3 velocity = {
    //            front.x * config->attackParam.forwardPower * rate,
    //            front.y * config->attackParam.forwardPower * rate,
    //            front.z * config->attackParam.forwardPower * rate
    //        };
    //        player->SetVelocity(velocity);

    //        forwarded = true;
    //    }
    //}

    //if (comboAnimationIndex == player->GetPlayerModel()->GetAnimationIndex("WarriorAttackCombo3"))
    //{
    //    player->GetLeftShield()->AttackAnimationCollision(player->GetModel(), config);
    //}
    //else
    //{
    //    player->GetWeaponRight()->AttackAnimationCollision(player->GetModel(), config);
    //}

    InputComboType input = InputCombo();

    if (!nextShiftReady)
    {
        if (frame >= poseFrame)
        {
            // 走りステート遷移
            if (InputRunMove())
            {
                ChangeState(PlayerStateId::Run);
            }
            //player->GetPlayerModel()->SetBaseAnimationSpeed(comboPoseSpeed);
        }
        else
        {
            //player->GetPlayerModel()->SetBaseAnimationSpeed(comboAttackSpeed);
        }
    }

    // 次のコンボステート処理
    if (inputToNextState.count(input))
    {

        // 先行入力処理
        if (input != InputComboType::None)
        {
            if (frame <= nextShiftFrame)
            {
                nextShiftReady = true;
                nextInput = input;
            }            
        }
    }

    // 次のコンボステートへ遷移
    if (nextShiftReady)
    {
        if (frame >= nextShiftFrame)
        {
            ChangeState(inputToNextState[nextInput]);
        }
    }

    // 終了後のステート遷移
    if (!player->GetPlayerModel()->IsPlayAnimation())
    {
        ChangeState(PlayerStateId::Idle);
    }
}

//-------------------------------------------------------------
// コンボ1ステート
//-------------------------------------------------------------
// コンストラクタ
PlayerCombo1State::PlayerCombo1State(Player* player)
    : PlayerComboState(player)
{
    comboAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("Combo1");

    nextShiftFrame = 0.53f;
    poseFrame = 0.53f;
    endFrame = 0.816f;
    comboAttackSpeed = 1.3f;
    comboPoseSpeed = 0.5f;

    forwardFrame = 0.15f;
    forwardPower = 12.0f;
    moveRate = 0.5f;
    turnRate = 0.5f;

    // 攻撃判定必要変数
    attackCollisionStartFrame = 0.26f;
    attackCollisionEndFrame = 0.49f;
    attackDamage = 10.0f;
    invisibleTime = 0.5f;

    // コントローラーの振動変数
    attackLeftVibrate = 1.0f;
    attackRightVibrate = 1.0f;

    // 攻撃時ヒットストップ変数
    attackHitStopTime = 0.0f;
    attackHitStopSpeed = 0.1f;

    inputToNextState[InputComboType::Light] = PlayerStateId::Combo2;
}

// デバッグ用GUI描画
void PlayerCombo1State::DrawDebugGUI()
{
    ImGui::Separator();

    if (ImGui::TreeNode(u8"コンボ1"))
    {
        //ImGui::Text(player->GetPlayerModel()->GetAnimationName(comboAnimationIndex));
        //ImGui::SameLine();
        //ImGui::Text(u8"終了フレーム:%.3f", player->GetPlayerModel()->GetAnimationLength(comboAnimationIndex));
        ImGui::Checkbox(u8"先行入力しているか", &nextShiftReady);
        ImGui::DragFloat(u8"攻撃スピード", &comboAttackSpeed, 0.01f, 0.0f, 5.0f);
        ImGui::DragFloat(u8"構えスピード", &comboPoseSpeed, 0.01f, 0.0f, 5.0f);
        ImGui::DragFloat(u8"先行入力受付開始フレーム", &nextShiftFrame, 0.01f, 0.0f);
        ImGui::DragFloat(u8"先行入力受付終了フレーム", &endFrame, 0.01f, 0.0f);
        ImGui::DragFloat(u8"前移動開始フレーム", &forwardFrame, 0.01f, 0.0f);
        ImGui::DragFloat(u8"前移動値", &forwardPower, 0.01f, 0.0f);
        ImGui::DragFloat(u8"コントローラーの左振動の強さ", &attackLeftVibrate, 0.01f, 0.0f);
        ImGui::DragFloat(u8"コントローラーの右振動の強さ", &attackRightVibrate, 0.01f, 0.0f);
        ImGui::DragFloat(u8"攻撃がヒットした時に何秒間止めるか", &attackHitStopTime, 0.01f, 0.0f);
        ImGui::DragFloat(u8"攻撃がヒットした時にどれぐらいの速さにするか", &attackHitStopSpeed, 0.01f, 0.0f);
        ImGui::TreePop();
    }
}

//-------------------------------------------------------------
// コンボ2ステート
//-------------------------------------------------------------
// コンストラクタ
PlayerCombo2State::PlayerCombo2State(Player* player)
    : PlayerComboState(player)
{
    inputToNextState[InputComboType::Light] = PlayerStateId::Combo3;
    inputToNextState[InputComboType::Heavy] = PlayerStateId::Heavy1;
    nextShiftFrame = 0.5f;
    poseFrame = 0.5f;
    endFrame = 0.816f;
    comboAttackSpeed = 1.5f;
    comboPoseSpeed = 1.0f;
    comboAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("Combo2");

    forwardFrame = 0.0f;
    forwardPower = 15.0f;
    moveRate = 0.5f;
    turnRate = 0.5f;

    // 攻撃判定必要変数
    attackCollisionStartFrame = 0.28f;
    attackCollisionEndFrame = 0.5f;
    attackDamage = 10.0f;
    invisibleTime = 0.5f;

    // コントローラーの振動変数
    attackLeftVibrate = 1.0f;
    attackRightVibrate = 1.0f;

    // 攻撃時ヒットストップ変数
    attackHitStopTime = 0.0f;
    attackHitStopSpeed = 0.1f;
}

// デバッグ用GUI描画
void PlayerCombo2State::DrawDebugGUI()
{
    ImGui::Separator();

    if (ImGui::TreeNode(u8"コンボ２"))
    {
        //ImGui::Text(player->GetPlayerModel()->GetAnimationName(comboAnimationIndex));
        //ImGui::SameLine();
        //ImGui::Text(u8"終了フレーム:%.3f", player->GetPlayerModel()->GetAnimationLength(comboAnimationIndex));
        ImGui::Checkbox(u8"先行入力しているか", &nextShiftReady);
        ImGui::DragFloat(u8"攻撃スピード", &comboAttackSpeed, 0.01f, 0.0f, 5.0f);
        ImGui::DragFloat(u8"構えスピード", &comboPoseSpeed, 0.01f, 0.0f, 5.0f);
        ImGui::DragFloat(u8"先行入力受付開始フレーム", &nextShiftFrame, 0.01f, 0.0f);
        ImGui::DragFloat(u8"先行入力受付終了フレーム", &endFrame, 0.01f, 0.0f);
        ImGui::DragFloat(u8"前移動開始フレーム", &forwardFrame, 0.01f, 0.0f);
        ImGui::DragFloat(u8"前移動値", &forwardPower, 0.01f, 0.0f);
        ImGui::DragFloat(u8"コントローラーの左振動の強さ", &attackLeftVibrate, 0.01f, 0.0f);
        ImGui::DragFloat(u8"コントローラーの右振動の強さ", &attackRightVibrate, 0.01f, 0.0f);
        ImGui::DragFloat(u8"攻撃がヒットした時に何秒間止めるか", &attackHitStopTime, 0.01f, 0.0f);
        ImGui::DragFloat(u8"攻撃がヒットした時にどれぐらいの速さにするか", &attackHitStopSpeed, 0.01f, 0.0f);
        ImGui::TreePop();
    }
}

//-------------------------------------------------------------
// コンボ3ステート
//-------------------------------------------------------------
// コンストラクタ
PlayerCombo3State::PlayerCombo3State(Player* player)
    : PlayerComboState(player)
{
    inputToNextState[InputComboType::Light] = PlayerStateId::Combo4;
    //inputToNextState[InputComboType::Heavy] = PlayerStateId::Heavy2;
    nextShiftFrame = 0.63f;
    poseFrame = 0.63f;
    endFrame = 0.9f;
    comboAttackSpeed = 1.0f;
    comboPoseSpeed = 1.0f;
    comboAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("Combo3");

    forwardFrame = 0.46f;
    forwardPower = 13.0f;
    moveRate = 0.5f;
    turnRate = 0.5f;

    // 攻撃判定必要変数
    attackCollisionStartFrame = 0.45f;
    attackCollisionEndFrame = 0.65f;
    attackDamage = 10.0f;
    invisibleTime = 0.5f;

    // コントローラーの振動変数
    attackLeftVibrate = 1.0f;
    attackRightVibrate = 1.0f;

    // 攻撃時ヒットストップ変数
    attackHitStopTime = 0.0f;
    attackHitStopSpeed = 0.1f;
}

// デバッグ用GUI描画
void PlayerCombo3State::DrawDebugGUI()
{
    ImGui::Separator();

    if (ImGui::TreeNode(u8"コンボ3"))
    {
        //ImGui::Text(player->GetPlayerModel()->GetAnimationName(comboAnimationIndex));
        //ImGui::SameLine();
        //ImGui::Text(u8"終了フレーム:%.3f", player->GetPlayerModel()->GetAnimationLength(comboAnimationIndex));
        ImGui::Checkbox(u8"先行入力しているか", &nextShiftReady);
        ImGui::DragFloat(u8"攻撃スピード", &comboAttackSpeed, 0.01f, 0.0f, 5.0f);
        ImGui::DragFloat(u8"構えスピード", &comboPoseSpeed, 0.01f, 0.0f, 5.0f);
        ImGui::DragFloat(u8"先行入力受付開始フレーム", &nextShiftFrame, 0.01f, 0.0f);
        ImGui::DragFloat(u8"先行入力受付終了フレーム", &endFrame, 0.01f, 0.0f);
        ImGui::DragFloat(u8"前移動開始フレーム", &forwardFrame, 0.01f, 0.0f);
        ImGui::DragFloat(u8"前移動値", &forwardPower, 0.01f, 0.0f);
        ImGui::DragFloat(u8"コントローラーの左振動の強さ", &attackLeftVibrate, 0.01f, 0.0f);
        ImGui::DragFloat(u8"コントローラーの右振動の強さ", &attackRightVibrate, 0.01f, 0.0f);
        ImGui::DragFloat(u8"攻撃がヒットした時に何秒間止めるか", &attackHitStopTime, 0.01f, 0.0f);
        ImGui::DragFloat(u8"攻撃がヒットした時にどれぐらいの速さにするか", &attackHitStopSpeed, 0.01f, 0.0f);
        ImGui::TreePop();
    }
}

//-------------------------------------------------------------
// コンボ4ステート
//-------------------------------------------------------------
// コンストラクタ
PlayerCombo4State::PlayerCombo4State(Player* player)
    : PlayerComboState(player)
{
    inputToNextState[InputComboType::Light] = PlayerStateId::Combo1;
    inputToNextState[InputComboType::Heavy] = PlayerStateId::Heavy1;
    nextShiftFrame = 0.7f;
    poseFrame = 0.7f;
    endFrame = 1.016f;
    comboAttackSpeed = 1.0f;
    comboPoseSpeed = 1.0f;
    comboAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("Combo4");

    forwardFrame = 0.23f;
    forwardPower = 20.0f;
    moveRate = 0.5f;
    turnRate = 0.5f;

    // 攻撃判定必要変数
    attackCollisionStartFrame = 0.37f;
    attackCollisionEndFrame = 0.65f;
    attackDamage = 10.0f;
    invisibleTime = 0.5f;

    // コントローラーの振動変数
    attackLeftVibrate = 1.0f;
    attackRightVibrate = 1.0f;

    // 攻撃時ヒットストップ変数
    attackHitStopTime = 0.0f;
    attackHitStopSpeed = 0.1f;
}

// デバッグ用GUI描画
void PlayerCombo4State::DrawDebugGUI()
{
    ImGui::Separator();

    if (ImGui::TreeNode(u8"コンボ4"))
    {
        //ImGui::Text(player->GetPlayerModel()->GetAnimationName(comboAnimationIndex));
        //ImGui::SameLine();
        //ImGui::Text(u8"終了フレーム:%.3f", player->GetPlayerModel()->GetAnimationLength(comboAnimationIndex));
        ImGui::Checkbox(u8"先行入力しているか", &nextShiftReady);
        ImGui::DragFloat(u8"攻撃スピード", &comboAttackSpeed, 0.01f, 0.0f, 5.0f);
        ImGui::DragFloat(u8"構えスピード", &comboPoseSpeed, 0.01f, 0.0f, 5.0f);
        ImGui::DragFloat(u8"先行入力受付開始フレーム", &nextShiftFrame, 0.01f, 0.0f);
        ImGui::DragFloat(u8"先行入力受付終了フレーム", &endFrame, 0.01f, 0.0f);
        ImGui::DragFloat(u8"前移動開始フレーム", &forwardFrame, 0.01f, 0.0f);
        ImGui::DragFloat(u8"前移動値", &forwardPower, 0.01f, 0.0f);
        ImGui::DragFloat(u8"コントローラーの左振動の強さ", &attackLeftVibrate, 0.01f, 0.0f);
        ImGui::DragFloat(u8"コントローラーの右振動の強さ", &attackRightVibrate, 0.01f, 0.0f);
        ImGui::DragFloat(u8"攻撃がヒットした時に何秒間止めるか", &attackHitStopTime, 0.01f, 0.0f);
        ImGui::DragFloat(u8"攻撃がヒットした時にどれぐらいの速さにするか", &attackHitStopSpeed, 0.01f, 0.0f);
        ImGui::TreePop();
    }
}

//-------------------------------------------------------------
// 強攻撃1ステート
//-------------------------------------------------------------
// コンストラクタ
PlayerHeavyAttack1State::PlayerHeavyAttack1State(Player* player)
    : PlayerComboState(player)
{
    inputToNextState[InputComboType::Heavy] = PlayerStateId::Heavy2;
    nextShiftFrame = 0.4f;
    poseFrame = 0.4f;
    endFrame = 0.749f;
    comboAttackSpeed = 1.0f;
    comboPoseSpeed = 1.0f;
    comboAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("WarriorAttackHeavy1");

    forwardFrame = 0.0f;
    forwardPower = 17.0f;
    moveRate = 0.5f;
    turnRate = 0.5f;

    // 攻撃判定必要変数
    attackCollisionStartFrame = 0.13f;
    attackCollisionEndFrame = 0.25f;
    attackDamage = 10.0f;
    invisibleTime = 0.5f;

    // コントローラーの振動変数
    attackLeftVibrate = 1.0f;
    attackRightVibrate = 1.0f;

    // 攻撃時ヒットストップ変数
    attackHitStopTime = 0.0f;
    attackHitStopSpeed = 0.1f;
}

void PlayerHeavyAttack1State::DrawDebugGUI()
{
    ImGui::Separator();

    if (ImGui::TreeNode(u8"強攻撃1"))
    {
        //ImGui::Text(player->GetPlayerModel()->GetAnimationName(comboAnimationIndex));
        //ImGui::SameLine();
        //ImGui::Text(u8"終了フレーム:%.3f", player->GetPlayerModel()->GetAnimationLength(comboAnimationIndex));
        ImGui::Checkbox(u8"先行入力しているか", &nextShiftReady);
        ImGui::DragFloat(u8"攻撃スピード", &comboAttackSpeed, 0.01f, 0.0f, 5.0f);
        ImGui::DragFloat(u8"構えスピード", &comboPoseSpeed, 0.01f, 0.0f, 5.0f);
        ImGui::DragFloat(u8"先行入力受付開始フレーム", &nextShiftFrame, 0.01f, 0.0f);
        ImGui::DragFloat(u8"先行入力受付終了フレーム", &endFrame, 0.01f, 0.0f);
        ImGui::DragFloat(u8"前移動開始フレーム", &forwardFrame, 0.01f, 0.0f);
        ImGui::DragFloat(u8"前移動値", &forwardPower, 0.01f, 0.0f);
        ImGui::DragFloat(u8"コントローラーの左振動の強さ", &attackLeftVibrate, 0.01f, 0.0f);
        ImGui::DragFloat(u8"コントローラーの右振動の強さ", &attackRightVibrate, 0.01f, 0.0f);
        ImGui::DragFloat(u8"攻撃がヒットした時に何秒間止めるか", &attackHitStopTime, 0.01f, 0.0f);
        ImGui::DragFloat(u8"攻撃がヒットした時にどれぐらいの速さにするか", &attackHitStopSpeed, 0.01f, 0.0f);
        ImGui::TreePop();
    }
}

//-------------------------------------------------------------
// 強攻撃2ステート
//-------------------------------------------------------------
// コンストラクタ
PlayerHeavyAttack2State::PlayerHeavyAttack2State(Player* player)
    : PlayerComboState(player)
{
    poseFrame = 0.55f;
    endFrame = 0.816f;
    comboAttackSpeed = 1.0f;
    comboPoseSpeed = 1.0f;
    comboAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("WarriorAttackHeavy2");

    forwardFrame = 0.0f;
    forwardPower = 13.0f;
    moveRate = 0.5f;
    turnRate = 0.5f;

    // 攻撃判定必要変数
    attackCollisionStartFrame = 0.23f;
    attackCollisionEndFrame = 0.45f;
    attackDamage = 10.0f;
    invisibleTime = 0.5f;

    // コントローラーの振動変数
    attackLeftVibrate = 1.0f;
    attackRightVibrate = 1.0f;

    // 攻撃時ヒットストップ変数
    attackHitStopTime = 0.0f;
    attackHitStopSpeed = 0.1f;
}

void PlayerHeavyAttack2State::DrawDebugGUI()
{
    ImGui::Separator();

    if (ImGui::TreeNode(u8"強攻撃2"))
    {
        //ImGui::Text(player->GetPlayerModel()->GetAnimationName(comboAnimationIndex));
        //ImGui::SameLine();
        //ImGui::Text(u8"終了フレーム:%.3f", player->GetPlayerModel()->GetAnimationLength(comboAnimationIndex));
        ImGui::Checkbox(u8"先行入力しているか", &nextShiftReady);
        ImGui::DragFloat(u8"攻撃スピード", &comboAttackSpeed, 0.01f, 0.0f, 5.0f);
        ImGui::DragFloat(u8"構えスピード", &comboPoseSpeed, 0.01f, 0.0f, 5.0f);
        ImGui::DragFloat(u8"先行入力受付開始フレーム", &nextShiftFrame, 0.01f, 0.0f);
        ImGui::DragFloat(u8"先行入力受付終了フレーム", &endFrame, 0.01f, 0.0f);
        ImGui::DragFloat(u8"前移動開始フレーム", &forwardFrame, 0.01f, 0.0f);
        ImGui::DragFloat(u8"前移動値", &forwardPower, 0.01f, 0.0f);
        ImGui::DragFloat(u8"コントローラーの左振動の強さ", &attackLeftVibrate, 0.01f, 0.0f);
        ImGui::DragFloat(u8"コントローラーの右振動の強さ", &attackRightVibrate, 0.01f, 0.0f);
        ImGui::DragFloat(u8"攻撃がヒットした時に何秒間止めるか", &attackHitStopTime, 0.01f, 0.0f);
        ImGui::DragFloat(u8"攻撃がヒットした時にどれぐらいの速さにするか", &attackHitStopSpeed, 0.01f, 0.0f);
        ImGui::TreePop();
    }
}

//-------------------------------------------------------------
// ガード待機ステート
//-------------------------------------------------------------
// コンストラクタ
PlayerGuardIdle::PlayerGuardIdle(Player* player)
    : PlayerState(player)
{
    guardIdleAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("WarriorGuard");
}

// 開始処理
void PlayerGuardIdle::Enter()
{
    player->GetPlayerModel()->PlayAnimation(guardIdleAnimationIndex, true, 0.1f);

    player->SetPlayerGuard(true);
}

// 更新処理
void PlayerGuardIdle::Update(float elapsedTime)
{
    if (CameraParam::Instance().GetIsLockOn())
    {
        player->LockOnTurnToEnemy(elapsedTime);
    }

    // 待機ステート遷移
    if (!InputGuard())
    {
        ChangeState(PlayerStateId::Idle);
    }
    // ガードパリィステート遷移
    else if (InputGuard() && InputGuardParry())
    {
        ChangeState(PlayerStateId::GuardParry);
    }
    // ガード歩きステート遷移
    else if (InputGuard() && (InputWalkMove() || InputRunMove()))
    {
        ChangeState(PlayerStateId::GuardWalk);
    }
    // 回避ステート遷移
    else if (InputDodge())
    {
        ChangeState(PlayerStateId::Dodge);
    }

    //player->GetPlayerModel()->SetBaseAnimationSpeed(guardIdleAnimationSpeed);
}

// 終了処理
void PlayerGuardIdle::Exit()
{
    player->SetPlayerGuard(false);
}

// デバッグ用GUI描画
void PlayerGuardIdle::DrawDebugGUI()
{
    ImGui::Separator();

    if (ImGui::TreeNode(u8"ガード待機"))
    {
        //ImGui::Text(player->GetPlayerModel()->GetAnimationName(guardIdleAnimationIndex));
        //ImGui::SameLine();
        //ImGui::Text(u8"終了フレーム:%.3f", player->GetPlayerModel()->GetAnimationLength(guardIdleAnimationIndex));
        ImGui::DragFloat(u8"アニメーションスピード", &guardIdleAnimationSpeed, 0.01f, 0.0f, 5.0f);
        ImGui::TreePop();
    }
}

//-------------------------------------------------------------
// ガード歩きステート
//-------------------------------------------------------------
// コンストラクタ
PlayerGuardWalk::PlayerGuardWalk(Player* player)
    : PlayerState(player)
{
    guardFrontWalkAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("WarriorGuardWalkForward");
    guardBackWalkAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("WarriorGuardWalkBackward");
    guardRightWalkAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("WarriorGuardWalkRightward");
    guardLeftWalkAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("WarriorGuardWalkLeftward");
}

// 開始処理
void PlayerGuardWalk::Enter()
{
    player->SetPlayerGuard(true);
}

// 更新処理
void PlayerGuardWalk::Update(float elapsedTime)
{
    LockOnStrafe(guardRightWalkAnimationIndex, guardLeftWalkAnimationIndex, guardFrontWalkAnimationIndex, guardBackWalkAnimationIndex);

    player->PlayerMove(elapsedTime, guardWalkAnimationMoveRate);

    // 待機ステート遷移
    if (!InputGuard())
    {
        ChangeState(PlayerStateId::Idle);
    }
    // ガードパリィステート遷移
    else if (InputGuard() && InputGuardParry())
    {
        ChangeState(PlayerStateId::GuardParry);
    }
    // ガード待機ステート遷移
    else if (InputGuard() && !InputWalkMove() && !InputRunMove())
    {
        ChangeState(PlayerStateId::GuardIdle);
    }
    // 回避ステート遷移
    else if (InputDodge())
    {
        ChangeState(PlayerStateId::Dodge);
    }

    //player->GetPlayerModel()->SetBaseAnimationSpeed(guardWalkAnimationSpeed);
}

// 終了処理
void PlayerGuardWalk::Exit()
{
    player->SetPlayerGuard(false);
}

// デバッグ用GUI描画
void PlayerGuardWalk::DrawDebugGUI()
{
    ImGui::Separator();

    if (ImGui::TreeNode(u8"ガード歩き"))
    {
        //ImGui::Text(player->GetPlayerModel()->GetAnimationName(guardFrontWalkAnimationIndex));
        //ImGui::SameLine();
        //ImGui::Text(u8"終了フレーム:%.3f", player->GetPlayerModel()->GetAnimationLength(guardFrontWalkAnimationIndex));
        ImGui::DragFloat(u8"ガード歩き移動率", &guardWalkAnimationMoveRate, 0.01f, 0.0f);
        ImGui::DragFloat(u8"アニメーションスピード", &guardWalkAnimationSpeed, 0.01f, 0.0f, 5.0f);
        ImGui::TreePop();
    }
}

//-------------------------------------------------------------
// ガードヒットステート
//-------------------------------------------------------------
// コンストラクタ
PlayerGuardHit::PlayerGuardHit(Player* player)
    : PlayerState(player)
{
    guardHitAnimationIndex = player->GetModel()->GetAnimationIndex("WarriorGuardHit2");
}

void PlayerGuardHit::Enter()
{
    player->GetModel()->PlayAnimation(guardHitAnimationIndex, false, 0.1f);

    player->SetPlayerGuard(true);

    GamePad& gamepad = Input::Instance().GetGamePad();

    gamepad.Vibrate(0.7f, 0.7f);

    timer = 0.567f;
}

void PlayerGuardHit::Update(float elapsedTime)
{
    GamePad& gamepad = Input::Instance().GetGamePad();

    timer -= elapsedTime;

    if (timer <= 0.0f)
    {
        // コンボ1ステートに遷移
        if (InputCombo() == InputComboType::Light)
        {
            ChangeState(PlayerStateId::Combo1);
        }
        // 強攻撃1ステートに遷移
        else if (InputCombo() == InputComboType::Heavy)
        {
            ChangeState(PlayerStateId::Heavy1);
        }
        // 回避ステートに遷移
        else if (InputDodge())
        {
            ChangeState(PlayerStateId::Dodge);
        }
        // ガード待機ステート遷移
        else if (InputGuard())
        {
            ChangeState(PlayerStateId::GuardIdle);
        }
        // 走りステートに遷移
        else if (InputRunMove())
        {
            ChangeState(PlayerStateId::Run);
        }
        // 歩きステートに遷移
        else if (InputWalkMove())
        {
            ChangeState(PlayerStateId::Walk);
        }
        // アイドルステートに遷移
        else if (!InputWalkMove() && !InputRunMove())
        {
            ChangeState(PlayerStateId::Idle);
        }
    }
    else if (timer <= 0.4f)
    {
        gamepad.Vibrate(0.0f, 0.0f);
    }
}

// 終了処理
void PlayerGuardHit::Exit()
{
    player->SetPlayerGuard(false);
}

//-------------------------------------------------------------
// ガードパリィステート
//-------------------------------------------------------------
// コンストラクタ
PlayerGuardParry::PlayerGuardParry(Player* player)
    : PlayerState(player)
{
    guardParryAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("WarriorParry");
}

void PlayerGuardParry::Enter()
{
    player->GetPlayerModel()->PlayAnimation(guardParryAnimationIndex, false, 0.1f);
    timer = 0.5f;

    player->SetPlayerParry(true);
}

void PlayerGuardParry::Update(float elapsedTime)
{
    int index = player->GetPlayerModel()->GetCurrentAnimationIndex();
    //AnimationConfig* config = player->GetPlayerModel()->GetAnimationConfig("Player", index);
    //player->GetLeftShield()->ParryAnimationCollision(player->GetModel(), config);

    timer -= elapsedTime;
    if (timer <= 0.0f)
    {
        player->SetPlayerParry(false);

        // 待機ステート遷移
        if (!InputGuard())
        {
            ChangeState(PlayerStateId::Idle);
        }
        // ガード歩きステート遷移
        else if (InputGuard() && (InputWalkMove() || InputRunMove()))
        {
            ChangeState(PlayerStateId::GuardWalk);
        }
        // ガード待機ステート遷移
        else if (InputGuard())
        {
            ChangeState(PlayerStateId::GuardIdle);
        }
    }

    //  player->GetPlayerModel()->SetBaseAnimationSpeed(guardParryAnimationSpeed);
}

// デバッグ用GUI描画
void PlayerGuardParry::DrawDebugGUI()
{
    ImGui::Separator();

    if (ImGui::TreeNode(u8"ガードパリィ"))
    {
        //ImGui::Text(player->GetPlayerModel()->GetAnimationName(guardParryAnimationIndex));
        //ImGui::SameLine();
        //ImGui::Text(u8"終了フレーム:%.3f", player->GetPlayerModel()->GetAnimationLength(guardParryAnimationIndex));
        ImGui::DragFloat(u8"アニメーションスピード", &guardParryAnimationSpeed, 0.01f, 0.0f, 5.0f);
        bool parryFlag = player->GetPlayerIsParry();
        ImGui::Checkbox(u8"パリィ判定があるか", &parryFlag);
        ImGui::TreePop();
    }
}

//-------------------------------------------------------------
// ダメージステート
//-------------------------------------------------------------
// コンストラクタ
PlayerDamageState::PlayerDamageState(Player* player)
    : PlayerState(player)
{
    damageAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("WarriorHit2");
}

// 開始処理
void PlayerDamageState::Enter()
{
    player->GetModel()->PlayAnimation(damageAnimationIndex, false, 0.1f);

    GamePad& gamepad = Input::Instance().GetGamePad();

    gamepad.Vibrate(0.3f, 0.3f);
}

// 更新処理
void PlayerDamageState::Update(float elapsedTime)
{
    GamePad& gamepad = Input::Instance().GetGamePad();

    if (!player->GetModel()->IsPlayAnimation())
    {
        gamepad.Vibrate(0.0f, 0.0f);

        // コンボ1ステートに遷移
        if (InputCombo() == InputComboType::Light)
        {
            ChangeState(PlayerStateId::Combo1);
        }
        // 強攻撃1ステートに遷移
        else if (InputCombo() == InputComboType::Heavy)
        {
            ChangeState(PlayerStateId::Heavy1);
        }
        // 回避ステートに遷移
        else if (InputDodge())
        {
            ChangeState(PlayerStateId::Dodge);
        }
        // ガード待機ステート遷移
        else if (InputGuard())
        {
            ChangeState(PlayerStateId::GuardIdle);
        }
        // 走りステートに遷移
        else if (InputRunMove())
        {
            ChangeState(PlayerStateId::Run);
        }
        // 歩きステートに遷移
        else if (InputWalkMove())
        {
            ChangeState(PlayerStateId::Walk);
        }
        // アイドルステートに遷移
        else if (!InputWalkMove() && !InputRunMove())
        {
            ChangeState(PlayerStateId::Idle);
        }
    }
}

//-------------------------------------------------------------
// 死亡ステートステート
//-------------------------------------------------------------
// コンストラクタ
PlayerDeadState::PlayerDeadState(Player* player)
    : PlayerState(player)
{
    deadAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("WarriorDeath");
}

// 開始処理
void PlayerDeadState::Enter()
{
    GamePad& gamePad = Input::Instance().GetGamePad();
    gamePad.Vibrate(0.0f, 0.0f);

    player->GetModel()->PlayAnimation(deadAnimationIndex, false, 0.1f);

    player->SetDeathFlag(true);
}

// 更新処理
void PlayerDeadState::Update(float elapsedTime)
{
    GamePad& gamePad = Input::Instance().GetGamePad();
    Mouse& mouse = Input::Instance().GetMouse();

    if (gamePad.GetButtonDown() & gamePad.BTN_DOWN)
    {
        player->currentSelection = Player::DeathMenuOption::Continue;
    }
    else if (gamePad.GetButtonDown() & gamePad.BTN_UP)
    {
        player->currentSelection = Player::DeathMenuOption::Exit;
    }

    if (gamePad.GetButtonDown() & gamePad.BTN_A || mouse.GetButtonDown() & mouse.BTN_LEFT)
    {
        switch (player->currentSelection)
        {
        case Player::DeathMenuOption::Continue:
            player->SetDeathFlag(false);
            player->SetHealth(player->GetMaxHealth());
            ChangeState(PlayerStateId::Idle);
            break;

        case Player::DeathMenuOption::Exit:
            if (player->onExitToTitle)
            {
                player->onExitToTitle();
            }
            break;
        default:
            break;
        }
    }
}
