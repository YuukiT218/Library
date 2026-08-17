#include "BattleUI.h"
#include "Graphics/Graphics.h"
#include "Graphics/GpuResourceUtils.h"
#include "Character/Player/Player.h"
#include "Character/Enemy/EnemyBoss.h"
#include "Camera/CameraParam.h"
#include <cmath>
#include <algorithm>

void BattleUI::Initialize()
{
    ID3D11Device* device = Graphics::Instance().GetDevice();

    // ---------------------------------------------------
    //  スプライト読み込み
    // ---------------------------------------------------
    lockOnPoint = std::make_unique<Sprite>(device, "Data/Sprite/Lockon.png");

    // プレイヤー
    spritePlayerHP_Back = std::make_unique<Sprite>(device, "Data/Sprite/HPGauge.png");
    spritePlayerHP_Fill = std::make_unique<Sprite>(device, "Data/Sprite/HPBar.png");
    // マスク画像 (Spriteとしてロードするが、GetSRV()でテクスチャとして使う)
    spritePlayerHP_Mask = std::make_unique<Sprite>(device, "Data/Sprite/HPBarMask.png");

    // ゲージに埋め込むキャラクターの立ち絵
    portraitNormal = std::make_unique<Sprite>(device, "Data/Sprite/portrait_kohaku_01.png");
    portraitDamage = std::make_unique<Sprite>(device, "Data/Sprite/portrait_kohaku_06.png");

    // ボス
    spriteBossHP_Back = std::make_unique<Sprite>(device, "Data/Sprite/BossHPGauge.png");
    spriteBossHP_Fill = std::make_unique<Sprite>(device, "Data/Sprite/BossHPBar.png");
    spriteBossStock_Back = std::make_unique<Sprite>(device, "Data/Sprite/HPStockGauge.png");
    spriteBossStock_Fill = std::make_unique<Sprite>(device, "Data/Sprite/HPStockBar.png");

    // 操作説明
    padInstructionUI = std::make_unique<Sprite>(device, "Data/Sprite/PadInst.png");
    keyMouInstructionUI = std::make_unique<Sprite>(device, "Data/Sprite/KeyMouInst.png");

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
            float speed = (std::max)(maxHP * damageGaugeDrainRate, (tracker.displayHP - currentHP) * 3.0f);
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
    isController = Input::Instance().GetIsLastGamePad();

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
    lockonEnemy = CameraParam::Instance().GetLockOnEnemy();

    if (lockonEnemy)
    {
        // ターゲット位置（spine_03）の取得
        DirectX::XMFLOAT3 lockonEnemyPosition = lockonEnemy->GetPosition();
        Model* model = lockonEnemy->GetModel();
        if (model)
        {
            Model::Node* node = model->FindNode("spine_03");
            if (node)
            {
                lockonEnemyPosition = { node->worldTransform._41, node->worldTransform._42, node->worldTransform._43 };
            }
        }

        // 距離計算
        DirectX::XMFLOAT3 playerPos = Player::Instance().GetPosition();
        DirectX::XMVECTOR pPos = DirectX::XMLoadFloat3(&playerPos);
        DirectX::XMVECTOR ePos = DirectX::XMLoadFloat3(&lockonEnemyPosition);
        DirectX::XMVECTOR distVec = DirectX::XMVectorSubtract(pPos, ePos);
        float distance = DirectX::XMVectorGetX(DirectX::XMVector3Length(distVec));

        // アングル回転
        lockonAngle += elapsedTime * 90.0f;

        // 距離に基づいてスケールを変更 (元仕様)
        if (distance <= 5.0f)
        {
            lockonScale.x = 40.0f; // 距離が5以下なら最大スケール
            lockonScale.y = 40.0f;
        }
        else if (distance <= 30.0f)
        {
            // 距離が5〜30の間でスケールを線形補間
            lockonScale.x = 40.0f - ((distance - 5.0f) / 25.0f) * (40.0f - 30.0f);
            lockonScale.y = 40.0f - ((distance - 5.0f) / 25.0f) * (40.0f - 30.0f);
        }
        else
        {
            lockonScale.x = 30.0f; // 距離が30を超えるなら最小スケール
            lockonScale.y = 30.0f;
        }
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
        float w = spritePlayerHP_Back->GetTextureWidth() * playerGaugeScale;
        float h = spritePlayerHP_Back->GetTextureHeight() * playerGaugeScale;

        // 減少中のダメージ量を表す割合
        float damageRatio = 0.0f;
        if (maxHP > 0.0f) damageRatio = playerHPTracker.displayHP / maxHP;

        // 背景
        spritePlayerHP_Back->Render(dc, px, py, 0, w, h, 0, 1, 1, 1, 1.0f);

        w = spritePlayerHP_Fill->GetTextureWidth() * playerGaugeScale;
        h = spritePlayerHP_Fill->GetTextureHeight() * playerGaugeScale;

        ID3D11ShaderResourceView* maskSRV = spritePlayerHP_Mask->GetSRV();
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

            spritePlayerHP_Fill->Render(dc, px + 1, py + 3, 0, w, h, 0, 1, 1, 1, 1.0f,
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

            spritePlayerHP_Fill->Render(dc, px+1, py+3, 0, w, h, 0, 1, 1, 1, 1.0f,
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
        if (bossMaxHP > 100.0f) maxStockCount = static_cast<int>((bossMaxHP - 0.1f) / 100.0f);

        int currentStockCount = 0;
        if (bossCurrentHP > 100.0f) currentStockCount = static_cast<int>((bossCurrentHP - 0.1f) / 100.0f);

        // 現在のバーの端数 (0-100)
        float currentBarVal = std::fmod(bossCurrentHP, 100.0f);
        if (currentBarVal == 0.0f && bossCurrentHP > 0.0f) currentBarVal = 100.0f;
        int emptyCount = maxStockCount - currentStockCount;

        // 最大ストック数分ループして描画
        for (int i = 0; i < maxStockCount; ++i)
        {
            float sx = bossStockStartPos.x + (i * bossStockOffset);
            float sy = bossStockStartPos.y;

            float sw = spriteBossStock_Back->GetTextureWidth() * bossStockScale;
            float sh = spriteBossStock_Back->GetTextureHeight() * bossStockScale;
            spriteBossStock_Back->Render(dc, sx, sy, 0, sw, sh, 0, 1, 1, 1, 1.0f);

            // i=0(一番左) < emptyCount なら描画しない
            if (i >= emptyCount)
            {
                sw = spriteBossStock_Fill->GetTextureWidth() * bossStockScale;
                sh = spriteBossStock_Fill->GetTextureHeight() * bossStockScale;
                spriteBossStock_Fill->Render(dc, sx + 2.5f, sy + 2.5f, 0, sw, sh, 0, 1, 1, 1, 1.0f);
            }
        }

        // メインHPバー
        float bx = bossGaugePos.x;
        float by = bossGaugePos.y;
        float bw = spriteBossHP_Back->GetTextureWidth() * bossGaugeScale;
        float bh = spriteBossHP_Back->GetTextureHeight() * bossGaugeScale;

        spriteBossHP_Back->Render(dc, bx, by, 0, bw, bh, 0, 1, 1, 1, 1.0f);

        bw = spriteBossHP_Fill->GetTextureWidth() * bossGaugeScale;
        bh = spriteBossHP_Fill->GetTextureHeight() * bossGaugeScale;
        float fillRatio = currentBarVal / 100.0f;
        float fillWidth = bw * fillRatio;

        // ダメージ量バー（現在のバーより長い分だけが見えるようにする）
        // ストックをまたいだ場合はバーいっぱいまで伸ばす
        float barBase = bossCurrentHP - currentBarVal;
        float damageBarVal = bossHPTracker.displayHP - barBase;
        if (damageBarVal > 100.0f) damageBarVal = 100.0f;
        float damageRatio = damageBarVal / 100.0f;

        if (damageRatio > fillRatio)
        {
            GaugeConstants cb{};
            cb.hpRatio = 1.0f;
            cb.tintStrength = 1.0f;
            cb.useMask = 0.0f;  // ボスのバーは矩形なのでマスクは使わない
            cb.tintColor = damageGaugeColor;
            dc->UpdateSubresource(gaugeConstantBuffer.Get(), 0, nullptr, &cb, 0, 0);
            dc->PSSetConstantBuffers(0, 1, gaugeConstantBuffer.GetAddressOf());

            spriteBossHP_Fill->Render(dc,
                bx + 2, by + 2, 0,
                bw * damageRatio, bh,
                0, 0,
                spriteBossHP_Fill->GetTextureWidth() * damageRatio,
                spriteBossHP_Fill->GetTextureHeight(),
                0, 1, 1, 1, 1.0f,
                nullptr,
                gaugePixelShader.Get());
        }

        if (fillWidth > 0.0f)
        {
            spriteBossHP_Fill->Render(dc,
                bx+2, by+2, 0,
                fillWidth, bh,
                0, 0,
                spriteBossHP_Fill->GetTextureWidth() * fillRatio,
                spriteBossHP_Fill->GetTextureHeight(),
                0, 1, 1, 1, 1.0f);
        }
    }

    // =================================================================
    //  操作説明UI
    // =================================================================
    {
        if (isController)
            padInstructionUI->Render(dc, 0, 0, 0, 1920, 1080, 0, 1, 1, 1, 1.0f);
        else
            keyMouInstructionUI->Render(dc, 0, 0, 0, 1920, 1080, 0, 1, 1, 1, 1.0f);
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
                launcher->Render(dc, 0, 0, 0, 1920, 1080, 0, 1, 1, 1, 1.0f);
            else
                launcherPC->Render(dc, 0, 0, 0, 1920, 1080, 0, 1, 1, 1, 1.0f);
        }

        // Counter描画 (ガードカウンター待機状態)
        if (player.GetPlayerIsCounter())
        {
            if (isController)
                counter->Render(dc, 0, 0, 0, 1920, 1080, 0, 1, 1, 1, 1.0f);
            else
                counterPC->Render(dc, 0, 0, 0, 1920, 1080, 0, 1, 1, 1, 1.0f);
        }
    }

    // =================================================================
    // ロックオンカーソル
    // =================================================================
    if (CameraParam::Instance().GetIsLockOn())
    {
        lockonEnemy = CameraParam::Instance().GetLockOnEnemy();

        if (lockonEnemy)
        {
            // 敵の位置取得 (Updateで更新されたscaleを使用)
            DirectX::XMFLOAT3 targetPos = lockonEnemy->GetPosition();
            Model* model = lockonEnemy->GetModel();
            if (model)
            {
                Model::Node* node = model->FindNode("spine_03");
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
                    screenPos.x - lockonScale.x * 0.5f,
                    screenPos.y - lockonScale.y * 0.5f,
                    0,
                    lockonScale.x, lockonScale.y,
                    0, 0, 256, 256, // 元のテクスチャ切り抜き指定
                    lockonAngle,
                    1, 1, 1, 0.7f); // 元の色・透明度
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
    cb.edgeSoftness = (std::max)(portraitEdgeSoftness, 0.01f);
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
        dy += cosf(phase * 1.37f) * amplitude * 0.6f;
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
            ImGui::DragFloat2("Scale", &lockonScale.x, 1.0f);
        }

        if (ImGui::CollapsingHeader("Instruction UI", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::DragFloat2("Position", &instructionPos.x, 1.0f);
            ImGui::DragFloat("Scale", &instructionScale, 0.01f, 0.1f, 5.0f);
        }
    }
    ImGui::End();
}