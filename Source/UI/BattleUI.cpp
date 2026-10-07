#include "BattleUI.h"
#include "Graphics/Graphics.h"
#include "Graphics/GpuResourceUtils.h"
#include "Character/Player/Player.h"
#include "Character/Enemy/EnemyBoss.h"
#include "Camera/CameraParam.h"
#include "System/ScreenSize.h"
#include <cmath>
#include <algorithm>

namespace
{
    // 全画面の1枚絵UIを描く大きさ
    constexpr float FULL_SCREEN_WIDTH = static_cast<float>(ScreenSize::WIDTH);
    constexpr float FULL_SCREEN_HEIGHT = static_cast<float>(ScreenSize::HEIGHT);

    // ロックオンカーソルを表示するノード名
    constexpr const char* LOCK_ON_TARGET_NODE_NAME = "spine_03";

    // ロックオンカーソルの回転速度（度/秒）
    constexpr float LOCK_ON_ROTATE_SPEED = 90.0f;

    // ロックオンカーソルの大きさ（近距離で最大、遠距離で最小）
    constexpr float LOCK_ON_NEAR_DISTANCE = 5.0f;
    constexpr float LOCK_ON_FAR_DISTANCE = 30.0f;
    constexpr float LOCK_ON_MAX_SCALE = 40.0f;
    constexpr float LOCK_ON_MIN_SCALE = 30.0f;

    // ロックオンカーソル画像の切り抜きサイズと不透明度
    constexpr float LOCK_ON_TEXTURE_SIZE = 256.0f;
    constexpr float LOCK_ON_ALPHA = 0.7f;

    // 表示が遅れているダメージ量を、残差に比例して追いつかせる速さ
    constexpr float DAMAGE_GAUGE_CATCH_UP_RATE = 3.0f;

    // ボスのHPゲージ1本（ストック1つ）あたりのHP
    constexpr float BOSS_HP_PER_STOCK = 100.0f;

    // ちょうどストックの境目のHPを、前のストックとして数えるための補正
    constexpr float BOSS_STOCK_BOUNDARY_EPSILON = 0.1f;

    // 背景画像に対するゲージ本体の描画位置のずれ
    const DirectX::XMFLOAT2 PLAYER_GAUGE_FILL_OFFSET = { 1.0f, 3.0f };
    const DirectX::XMFLOAT2 BOSS_GAUGE_FILL_OFFSET = { 2.0f, 2.0f };
    constexpr float BOSS_STOCK_FILL_OFFSET = 2.5f;

    // 縁のぼかし幅の下限（0だとシェーダーで割り算できない）
    constexpr float MIN_EDGE_SOFTNESS = 0.01f;

    // 被弾時の立ち絵の揺れ（縦揺れの周期と大きさの倍率）
    constexpr float PORTRAIT_SHAKE_Y_FREQUENCY_SCALE = 1.37f;
    constexpr float PORTRAIT_SHAKE_Y_AMPLITUDE_SCALE = 0.6f;
}

void BattleUI::Initialize()
{
    ID3D11Device* device = Graphics::Instance().GetDevice();

    // ---------------------------------------------------
    //  スプライト読み込み
    // ---------------------------------------------------
    lockOnPoint = std::make_unique<Sprite>(device, "Data/Sprite/Lockon.png");

    // プレイヤー
    spritePlayerHPBack = std::make_unique<Sprite>(device, "Data/Sprite/HPGauge.png");
    spritePlayerHPFill = std::make_unique<Sprite>(device, "Data/Sprite/HPBar.png");
    // マスク画像 (Spriteとしてロードするが、GetSRV()でテクスチャとして使う)
    spritePlayerHPMask = std::make_unique<Sprite>(device, "Data/Sprite/HPBarMask.png");

    // ゲージに埋め込むキャラクターの立ち絵
    portraitNormal = std::make_unique<Sprite>(device, "Data/Sprite/portrait_kohaku_01.png");
    portraitDamage = std::make_unique<Sprite>(device, "Data/Sprite/portrait_kohaku_06.png");

    // ボス
    spriteBossHPBack = std::make_unique<Sprite>(device, "Data/Sprite/BossHPGauge.png");
    spriteBossHPFill = std::make_unique<Sprite>(device, "Data/Sprite/BossHPBar.png");
    spriteBossStockBack = std::make_unique<Sprite>(device, "Data/Sprite/HPStockGauge.png");
    spriteBossStockFill = std::make_unique<Sprite>(device, "Data/Sprite/HPStockBar.png");

    // 操作説明
    padInstructionUI = std::make_unique<Sprite>(device, "Data/Sprite/PadInst.png");
    keyMouseInstructionUI = std::make_unique<Sprite>(device, "Data/Sprite/KeyMouInst.png");

    // カウンターと打ち上げ攻撃が可能な状態を示す
    counter = std::make_unique<Sprite>(device, "Data/Sprite/Counter.png");
    counterPC = std::make_unique<Sprite>(device, "Data/Sprite/CounterPC.png");
    launcher = std::make_unique<Sprite>(device, "Data/Sprite/Launcher.png");
    launcherPC = std::make_unique<Sprite>(device, "Data/Sprite/LauncherPC.png");

    // ---------------------------------------------------
    //  シェーダーリソース作成
    // ---------------------------------------------------
    // 定数バッファ作成
    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(GaugeConstants);
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = 0;
    device->CreateBuffer(&bd, nullptr, gaugeConstantBuffer.GetAddressOf());

    // ポートレート用定数バッファ作成
    bd.ByteWidth = sizeof(PortraitConstants);
    device->CreateBuffer(&bd, nullptr, portraitConstantBuffer.GetAddressOf());

    // カスタムピクセルシェーダー読み込み
    GpuResourceUtils::LoadPixelShader(device, "Data/Shader/GaugePS.cso", gaugePixelShader.GetAddressOf());
    GpuResourceUtils::LoadPixelShader(device, "Data/Shader/PortraitPS.cso", portraitPixelShader.GetAddressOf());

    // HPの遅延表示をリセット（最初の更新で現在値に合わせる）
    playerHPTracker = HPTracker();
    bossHPTracker = HPTracker();
}

// HPの遅延表示の更新
void BattleUI::UpdateHPTracker(HPTracker& tracker, float currentHP, float maxHP, float elapsedTime)
{
    // 初回は現在のHPに合わせる
    if (tracker.currentHP < 0.0f)
    {
        tracker.currentHP = currentHP;
        tracker.displayHP = currentHP;
        return;
    }

    if (currentHP < tracker.currentHP)
    {
        // 被弾したので、しばらく減少量を表示したままにする
        tracker.holdTimer = damageGaugeHoldTime;
        tracker.damageTimer = portraitDamageTime;
    }
    else if (currentHP > tracker.currentHP)
    {
        // 回復時は即座に追従させる
        tracker.displayHP = currentHP;
        tracker.holdTimer = 0.0f;
    }
    tracker.currentHP = currentHP;

    if (tracker.displayHP > currentHP)
    {
        if (tracker.holdTimer > 0.0f)
        {
            tracker.holdTimer -= elapsedTime;
        }
        else
        {
            // 一定速度と、残差に比例した速度の速い方で追いつかせる
            float speed = (std::max)(maxHP * damageGaugeDrainRate, (tracker.displayHP - currentHP) * DAMAGE_GAUGE_CATCH_UP_RATE);
            tracker.displayHP -= speed * elapsedTime;
            if (tracker.displayHP < currentHP) tracker.displayHP = currentHP;
        }
    }
    else
    {
        tracker.displayHP = currentHP;
    }

    if (tracker.damageTimer > 0.0f)
    {
        tracker.damageTimer -= elapsedTime;
        if (tracker.damageTimer < 0.0f) tracker.damageTimer = 0.0f;
    }
}

void BattleUI::Update(float elapsedTime)
{
    isController = Input::Instance().IsLastGamePad();

    // HPの遅延表示（ダメージ量の可視化）の更新
    {
        Player& player = Player::Instance();
        UpdateHPTracker(playerHPTracker,
            static_cast<float>(player.GetHealth()),
            static_cast<float>(player.GetMaxHealth()),
            elapsedTime);

        EnemyBoss& boss = EnemyBoss::Instance();
        UpdateHPTracker(bossHPTracker,
            static_cast<float>(boss.GetHealth()),
            static_cast<float>(boss.GetMaxHealth()),
            elapsedTime);
    }

    // ロックオン状態の更新
    lockOnEnemy = CameraParam::Instance().GetLockOnEnemy();

    if (lockOnEnemy)
    {
        // ターゲット位置（spine_03）の取得
        DirectX::XMFLOAT3 lockOnEnemyPosition = lockOnEnemy->GetPosition();
        Model* model = lockOnEnemy->GetModel();
        if (model)
        {
            Model::Node* node = model->FindNode(LOCK_ON_TARGET_NODE_NAME);
            if (node)
            {
                lockOnEnemyPosition = { node->worldTransform._41, node->worldTransform._42, node->worldTransform._43 };
            }
        }

        // 距離計算
        DirectX::XMFLOAT3 playerPos = Player::Instance().GetPosition();
        DirectX::XMVECTOR pPos = DirectX::XMLoadFloat3(&playerPos);
        DirectX::XMVECTOR ePos = DirectX::XMLoadFloat3(&lockOnEnemyPosition);
        DirectX::XMVECTOR distVec = DirectX::XMVectorSubtract(pPos, ePos);
        float distance = DirectX::XMVectorGetX(DirectX::XMVector3Length(distVec));

        // アングル回転
        lockOnAngle += elapsedTime * LOCK_ON_ROTATE_SPEED;

        // 距離に基づいてスケールを変更（近いほど大きく、遠いほど小さく）
        float t = (distance - LOCK_ON_NEAR_DISTANCE) / (LOCK_ON_FAR_DISTANCE - LOCK_ON_NEAR_DISTANCE);
        t = std::clamp(t, 0.0f, 1.0f);
        float lockOnSize = LOCK_ON_MAX_SCALE - t * (LOCK_ON_MAX_SCALE - LOCK_ON_MIN_SCALE);
        lockOnScale.x = lockOnSize;
        lockOnScale.y = lockOnSize;
    }
}

void BattleUI::Render(float elapsedTime)
{
    // グラフィックスシステムとコンテキストの取得
    Graphics& graphics = Graphics::Instance();
    ID3D11DeviceContext* dc = graphics.GetDeviceContext();
    RenderState* renderState = graphics.GetRenderState();

    // ステート設定
    FLOAT blendFactor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    UINT sampleMask = 0xFFFFFFFF;
    dc->OMSetBlendState(renderState->GetBlendState(BlendState::Transparency), blendFactor, sampleMask);
    dc->OMSetDepthStencilState(renderState->GetDepthStencilState(DepthState::NoTestNoWrite), 0);
    dc->RSSetState(renderState->GetRasterizerState(RasterizerState::SolidCullNone));
    ID3D11SamplerState* samplers[] = { renderState->GetSamplerState(SamplerState::LinearClamp) };
    dc->PSSetSamplers(0, 1, samplers);

    // =================================================================
    // プレイヤーHPゲージ (円形・マスク処理)
    // =================================================================
    Player& player = Player::Instance();
    {
        float maxHP = static_cast<float>(player.GetMaxHealth());
        float currentHP = static_cast<float>(player.GetHealth());
        float ratio = 0.0f;
        if (maxHP > 0.0f) ratio = currentHP / maxHP;

        // ImGuiで調整可能な変数を使用
        float px = playerGaugePos.x;
        float py = playerGaugePos.y;
        float w = spritePlayerHPBack->GetTextureWidth() * playerGaugeScale;
        float h = spritePlayerHPBack->GetTextureHeight() * playerGaugeScale;

        // 減少中のダメージ量を表す割合
        float damageRatio = 0.0f;
        if (maxHP > 0.0f) damageRatio = playerHPTracker.displayHP / maxHP;

        // 背景
        spritePlayerHPBack->Render(dc, px, py, 0, w, h, 0, 1, 1, 1, 1.0f);

        w = spritePlayerHPFill->GetTextureWidth() * playerGaugeScale;
        h = spritePlayerHPFill->GetTextureHeight() * playerGaugeScale;

        ID3D11ShaderResourceView* maskSRV = spritePlayerHPMask->GetSRV();
        dc->PSSetShaderResources(1, 1, &maskSRV);
        dc->PSSetConstantBuffers(0, 1, gaugeConstantBuffer.GetAddressOf());

        // ダメージ量バー（緑バーより先に描いて、はみ出た分だけが見えるようにする）
        if (damageRatio > ratio)
        {
            GaugeConstants cb{};
            cb.hpRatio = damageRatio;
            cb.tintStrength = 1.0f;
            cb.useMask = 1.0f;
            cb.tintColor = damageGaugeColor;
            dc->UpdateSubresource(gaugeConstantBuffer.Get(), 0, nullptr, &cb, 0, 0);

            spritePlayerHPFill->Render(dc, px + PLAYER_GAUGE_FILL_OFFSET.x, py + PLAYER_GAUGE_FILL_OFFSET.y, 0, w, h, 0, 1, 1, 1, 1.0f,
                nullptr,
                gaugePixelShader.Get()
            );
        }

        // 緑バー
        if (currentHP > 0)
        {
            GaugeConstants cb{};
            cb.hpRatio = ratio;
            cb.tintStrength = 0.0f;
            cb.useMask = 1.0f;
            cb.tintColor = { 1.0f, 1.0f, 1.0f, 1.0f };
            dc->UpdateSubresource(gaugeConstantBuffer.Get(), 0, nullptr, &cb, 0, 0);

            spritePlayerHPFill->Render(dc, px + PLAYER_GAUGE_FILL_OFFSET.x, py + PLAYER_GAUGE_FILL_OFFSET.y, 0, w, h, 0, 1, 1, 1, 1.0f,
                nullptr,
                gaugePixelShader.Get()
            );
        }

        // マスクテクスチャの解除
        ID3D11ShaderResourceView* nullSRV = nullptr;
        dc->PSSetShaderResources(1, 1, &nullSRV);

        // キャラクターの立ち絵（ゲージに重ねるので最後に描画する）
        RenderPlayerPortrait(dc);
    }

    // =================================================================
    // ボスHPゲージ (ストック制)
    // =================================================================
    EnemyBoss& boss = EnemyBoss::Instance();
    if (boss.GetHealth() > 0)
    {
        float bossMaxHP = static_cast<float>(boss.GetMaxHealth());
        float bossCurrentHP = static_cast<float>(boss.GetHealth());

        // ストック計算
        int maxStockCount = 0;
        if (bossMaxHP > BOSS_HP_PER_STOCK) maxStockCount = static_cast<int>((bossMaxHP - BOSS_STOCK_BOUNDARY_EPSILON) / BOSS_HP_PER_STOCK);

        int currentStockCount = 0;
        if (bossCurrentHP > BOSS_HP_PER_STOCK) currentStockCount = static_cast<int>((bossCurrentHP - BOSS_STOCK_BOUNDARY_EPSILON) / BOSS_HP_PER_STOCK);

        // 現在のバーの端数 (0-100)
        float currentBarVal = std::fmod(bossCurrentHP, BOSS_HP_PER_STOCK);
        if (currentBarVal == 0.0f && bossCurrentHP > 0.0f) currentBarVal = BOSS_HP_PER_STOCK;
        int emptyCount = maxStockCount - currentStockCount;

        // 最大ストック数分ループして描画
        for (int i = 0; i < maxStockCount; ++i)
        {
            float sx = bossStockStartPos.x + (i * bossStockOffset);
            float sy = bossStockStartPos.y;

            float sw = spriteBossStockBack->GetTextureWidth() * bossStockScale;
            float sh = spriteBossStockBack->GetTextureHeight() * bossStockScale;
            spriteBossStockBack->Render(dc, sx, sy, 0, sw, sh, 0, 1, 1, 1, 1.0f);

            // i=0(一番左) < emptyCount なら描画しない
            if (i >= emptyCount)
            {
                sw = spriteBossStockFill->GetTextureWidth() * bossStockScale;
                sh = spriteBossStockFill->GetTextureHeight() * bossStockScale;
                spriteBossStockFill->Render(dc, sx + BOSS_STOCK_FILL_OFFSET, sy + BOSS_STOCK_FILL_OFFSET, 0, sw, sh, 0, 1, 1, 1, 1.0f);
            }
        }

        // メインHPバー
        float bx = bossGaugePos.x;
        float by = bossGaugePos.y;
        float bw = spriteBossHPBack->GetTextureWidth() * bossGaugeScale;
        float bh = spriteBossHPBack->GetTextureHeight() * bossGaugeScale;

        spriteBossHPBack->Render(dc, bx, by, 0, bw, bh, 0, 1, 1, 1, 1.0f);

        bw = spriteBossHPFill->GetTextureWidth() * bossGaugeScale;
        bh = spriteBossHPFill->GetTextureHeight() * bossGaugeScale;
        float fillRatio = currentBarVal / BOSS_HP_PER_STOCK;
        float fillWidth = bw * fillRatio;

        // ダメージ量バー（現在のバーより長い分だけが見えるようにする）
        // ストックをまたいだ場合はバーいっぱいまで伸ばす
        float barBase = bossCurrentHP - currentBarVal;
        float damageBarVal = bossHPTracker.displayHP - barBase;
        if (damageBarVal > BOSS_HP_PER_STOCK) damageBarVal = BOSS_HP_PER_STOCK;
        float damageRatio = damageBarVal / BOSS_HP_PER_STOCK;

        if (damageRatio > fillRatio)
        {
            GaugeConstants cb{};
            cb.hpRatio = 1.0f;
            cb.tintStrength = 1.0f;
            cb.useMask = 0.0f;  // ボスのバーは矩形なのでマスクは使わない
            cb.tintColor = damageGaugeColor;
            dc->UpdateSubresource(gaugeConstantBuffer.Get(), 0, nullptr, &cb, 0, 0);
            dc->PSSetConstantBuffers(0, 1, gaugeConstantBuffer.GetAddressOf());

            spriteBossHPFill->Render(dc,
                bx + BOSS_GAUGE_FILL_OFFSET.x, by + BOSS_GAUGE_FILL_OFFSET.y, 0,
                bw * damageRatio, bh,
                0, 0,
                spriteBossHPFill->GetTextureWidth() * damageRatio,
                spriteBossHPFill->GetTextureHeight(),
                0, 1, 1, 1, 1.0f,
                nullptr,
                gaugePixelShader.Get());
        }

        if (fillWidth > 0.0f)
        {
            spriteBossHPFill->Render(dc,
                bx + BOSS_GAUGE_FILL_OFFSET.x, by + BOSS_GAUGE_FILL_OFFSET.y, 0,
                fillWidth, bh,
                0, 0,
                spriteBossHPFill->GetTextureWidth() * fillRatio,
                spriteBossHPFill->GetTextureHeight(),
                0, 1, 1, 1, 1.0f);
        }
    }

    // =================================================================
    //  操作説明UI
    // =================================================================
    {
        if (isController)
            padInstructionUI->Render(dc, 0, 0, 0, FULL_SCREEN_WIDTH, FULL_SCREEN_HEIGHT, 0, 1, 1, 1, 1.0f);
        else
            keyMouseInstructionUI->Render(dc, 0, 0, 0, FULL_SCREEN_WIDTH, FULL_SCREEN_HEIGHT, 0, 1, 1, 1, 1.0f);
    }

    // =================================================================
    // アクションUI (Launcher / Counter)
    // =================================================================
    {
        // Launcher描画 (地上コンボ1段目 or 2段目の間)
        PlayerStateId state = player.GetCurrentStateId();
        if (player.IsGround() && (state == PlayerStateId::Combo1 || state == PlayerStateId::Combo2))
        {
            // 画面全体に描画
            if (isController)
                launcher->Render(dc, 0, 0, 0, FULL_SCREEN_WIDTH, FULL_SCREEN_HEIGHT, 0, 1, 1, 1, 1.0f);
            else
                launcherPC->Render(dc, 0, 0, 0, FULL_SCREEN_WIDTH, FULL_SCREEN_HEIGHT, 0, 1, 1, 1, 1.0f);
        }

        // Counter描画 (ガードカウンター待機状態)
        if (player.IsStandbyCounter())
        {
            if (isController)
                counter->Render(dc, 0, 0, 0, FULL_SCREEN_WIDTH, FULL_SCREEN_HEIGHT, 0, 1, 1, 1, 1.0f);
            else
                counterPC->Render(dc, 0, 0, 0, FULL_SCREEN_WIDTH, FULL_SCREEN_HEIGHT, 0, 1, 1, 1, 1.0f);
        }
    }

    // =================================================================
    // ロックオンカーソル
    // =================================================================
    if (CameraParam::Instance().IsLockOn())
    {
        lockOnEnemy = CameraParam::Instance().GetLockOnEnemy();

        if (lockOnEnemy)
        {
            // 敵の位置取得 (Updateで更新されたscaleを使用)
            DirectX::XMFLOAT3 targetPos = lockOnEnemy->GetPosition();
            Model* model = lockOnEnemy->GetModel();
            if (model)
            {
                Model::Node* node = model->FindNode(LOCK_ON_TARGET_NODE_NAME);
                if (node) targetPos = { node->worldTransform._41, node->worldTransform._42, node->worldTransform._43 };
            }

            // カメラ行列取得
            Camera& camera = Camera::Instance();
            DirectX::XMMATRIX view = DirectX::XMLoadFloat4x4(&camera.GetView());
            DirectX::XMMATRIX proj = DirectX::XMLoadFloat4x4(&camera.GetProjection());
            float sw = graphics.GetScreenWidth();
            float sh = graphics.GetScreenHeight();

            // 座標変換
            DirectX::XMFLOAT2 screenPos = ConvertWorldToScreen(targetPos, view, proj, sw, sh);

            DirectX::XMVECTOR vPos = DirectX::XMLoadFloat3(&targetPos);
            DirectX::XMVECTOR vScreen = DirectX::XMVector3Project(vPos, 0, 0, sw, sh, 0, 1, proj, view, DirectX::XMMatrixIdentity());
            DirectX::XMFLOAT3 vScreen3;
            DirectX::XMStoreFloat3(&vScreen3, vScreen);

            if (vScreen3.z < 1.0f)
            {
                lockOnPoint->Render(dc,
                    screenPos.x - lockOnScale.x * 0.5f,
                    screenPos.y - lockOnScale.y * 0.5f,
                    0,
                    lockOnScale.x, lockOnScale.y,
                    0, 0, LOCK_ON_TEXTURE_SIZE, LOCK_ON_TEXTURE_SIZE,
                    lockOnAngle,
                    1, 1, 1, LOCK_ON_ALPHA);
            }
        }
    }
}

// プレイヤーの立ち絵をゲージの円に切り抜いて描画する
void BattleUI::RenderPlayerPortrait(ID3D11DeviceContext* dc)
{
    // ゲージのグレー円をスクリーン座標へ変換
    float scale = playerGaugeScale;
    DirectX::XMFLOAT2 circleCenter =
    {
        playerGaugePos.x + portraitCircleCenter.x * scale,
        playerGaugePos.y + portraitCircleCenter.y * scale
    };
    float circleRadius = portraitCircleRadius * scale;

    // 被弾リアクションの進行度 (1.0 → 0.0)
    float damageRate = 0.0f;
    if (portraitDamageTime > 0.0f)
    {
        damageRate = playerHPTracker.damageTimer / portraitDamageTime;
        if (damageRate < 0.0f) damageRate = 0.0f;
        if (damageRate > 1.0f) damageRate = 1.0f;
    }
    bool isDamaged = damageRate > 0.0f;

    // 切り抜き範囲を定数バッファへ（円は振動させずに立ち絵だけを揺らす）
    PortraitConstants cb{};
    cb.circleCenter = circleCenter;
    cb.circleRadius = circleRadius;
    cb.edgeSoftness = (std::max)(portraitEdgeSoftness, MIN_EDGE_SOFTNESS);
    cb.headCenter = { circleCenter.x, circleCenter.y + portraitHeadOffsetY * scale };
    cb.headRadius = { portraitHeadRadius.x * scale, portraitHeadRadius.y * scale };
    dc->UpdateSubresource(portraitConstantBuffer.Get(), 0, nullptr, &cb, 0, 0);
    dc->PSSetConstantBuffers(0, 1, portraitConstantBuffer.GetAddressOf());

    // 描画位置（被弾中は振動させる）
    float dw = portraitSrcRect.z * portraitScale * scale;
    float dh = portraitSrcRect.w * portraitScale * scale;
    float dx = circleCenter.x + portraitOffset.x * scale;
    float dy = circleCenter.y + portraitOffset.y * scale;

    if (isDamaged)
    {
        float phase = playerHPTracker.damageTimer * portraitShakeSpeed;
        float amplitude = portraitShakeAmplitude * damageRate * scale;
        dx += sinf(phase) * amplitude;
        dy += cosf(phase * PORTRAIT_SHAKE_Y_FREQUENCY_SCALE) * amplitude * PORTRAIT_SHAKE_Y_AMPLITUDE_SCALE;
    }

    // 被弾中は赤くする
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
    if (isDamaged)
    {
        g = 1.0f + (portraitDamageColor.y - 1.0f) * damageRate;
        b = 1.0f + (portraitDamageColor.z - 1.0f) * damageRate;
        r = 1.0f + (portraitDamageColor.x - 1.0f) * damageRate;
    }

    const Sprite* portrait = isDamaged ? portraitDamage.get() : portraitNormal.get();
    if (portrait == nullptr) return;

    portrait->Render(dc,
        dx, dy, 0,
        dw, dh,
        portraitSrcRect.x, portraitSrcRect.y,
        portraitSrcRect.z, portraitSrcRect.w,
        0,
        r, g, b, 1.0f,
        nullptr,
        portraitPixelShader.Get());
}

// ワールド座標からスクリーン座標へ変換する関数
DirectX::XMFLOAT2 BattleUI::ConvertWorldToScreen(
    const DirectX::XMFLOAT3& worldPosition,
    const DirectX::XMMATRIX& view,
    const DirectX::XMMATRIX& projection,
    float screenWidth,
    float screenHeight)
{
    DirectX::XMVECTOR worldVec = DirectX::XMLoadFloat3(&worldPosition);

    DirectX::XMVECTOR screenVec = DirectX::XMVector3Project(
        worldVec,
        0.0f, 0.0f,
        screenWidth, screenHeight,
        0.0f, 1.0f,
        projection,
        view,
        DirectX::XMMatrixIdentity()
    );

    DirectX::XMFLOAT2 screenPosition;
    DirectX::XMStoreFloat2(&screenPosition, screenVec);

    return screenPosition;
}

void BattleUI::DrawDebugGUI()
{
    if (ImGui::Begin("Battle UI Config"))
    {
        if (ImGui::CollapsingHeader("Player UI", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::DragFloat2("PPosition", &playerGaugePos.x, 1.0f);
            ImGui::DragFloat("PScale", &playerGaugeScale, 0.01f, 0.1f, 5.0f);
        }

        if (ImGui::CollapsingHeader("Player Portrait", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::TextUnformatted("Clip Shape (HPGauge.png texel)");
            ImGui::DragFloat2("Circle Center", &portraitCircleCenter.x, 0.5f);
            ImGui::DragFloat("Circle Radius", &portraitCircleRadius, 0.5f, 1.0f, 400.0f);
            ImGui::DragFloat("Head Offset Y", &portraitHeadOffsetY, 0.5f);
            ImGui::DragFloat2("Head Radius", &portraitHeadRadius.x, 0.5f, 1.0f, 400.0f);
            ImGui::DragFloat("Edge Softness", &portraitEdgeSoftness, 0.1f, 0.01f, 20.0f);

            ImGui::TextUnformatted("Portrait");
            ImGui::DragFloat4("Src Rect", &portraitSrcRect.x, 1.0f);
            ImGui::DragFloat("Portrait Scale", &portraitScale, 0.001f, 0.001f, 2.0f);
            ImGui::DragFloat2("Portrait Offset", &portraitOffset.x, 0.5f);

            ImGui::TextUnformatted("Damage Reaction");
            ImGui::DragFloat("Damage Time", &portraitDamageTime, 0.01f, 0.0f, 3.0f);
            ImGui::DragFloat("Shake Amplitude", &portraitShakeAmplitude, 0.1f, 0.0f, 40.0f);
            ImGui::DragFloat("Shake Speed", &portraitShakeSpeed, 0.5f, 0.0f, 300.0f);
            ImGui::ColorEdit3("Damage Color", &portraitDamageColor.x);
        }

        if (ImGui::CollapsingHeader("Damage Gauge", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::DragFloat("Hold Time", &damageGaugeHoldTime, 0.01f, 0.0f, 3.0f);
            ImGui::DragFloat("Drain Rate", &damageGaugeDrainRate, 0.01f, 0.01f, 10.0f);
            ImGui::ColorEdit4("Damage Bar Color", &damageGaugeColor.x);
            ImGui::Text("Player %.1f -> %.1f", playerHPTracker.displayHP, playerHPTracker.currentHP);
            ImGui::Text("Boss   %.1f -> %.1f", bossHPTracker.displayHP, bossHPTracker.currentHP);
        }

        if (ImGui::CollapsingHeader("Boss Main Gauge", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::DragFloat2("BPosition", &bossGaugePos.x, 1.0f);
            ImGui::DragFloat("BScale", &bossGaugeScale, 0.01f, 0.1f, 5.0f);
        }

        if (ImGui::CollapsingHeader("Boss Stock UI", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::DragFloat2("Start Position", &bossStockStartPos.x, 1.0f);
            ImGui::DragFloat("Offset X", &bossStockOffset, 1.0f);
            ImGui::DragFloat("Scale", &bossStockScale, 0.01f, 0.1f, 5.0f);
        }

        if (ImGui::CollapsingHeader("LockOn UI"))
        {
            ImGui::DragFloat2("Scale", &lockOnScale.x, 1.0f);
        }

        if (ImGui::CollapsingHeader("Instruction UI", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::DragFloat2("Position", &instructionPos.x, 1.0f);
            ImGui::DragFloat("Scale", &instructionScale, 0.01f, 0.1f, 5.0f);
        }
    }
    ImGui::End();
}