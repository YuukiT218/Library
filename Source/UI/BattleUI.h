#pragma once

#include <memory>
#include <d3d11.h>
#include <DirectXMath.h>
#include <wrl.h>

#include "Sprite/Sprite.h"

// 前方宣言
class Player;
class EnemyBoss;
class Enemy;

class BattleUI
{
private:
    BattleUI() {};
    ~BattleUI() {};

public:
    // シングルトン
    static BattleUI& Instance()
    {
        static BattleUI instance;
        return instance;
    }

    // 初期化
    void Initialize();

    // 更新（アニメーション用などに使用可能）
    void Update(float elapsedTime);

    // 描画
    void Render(float elapsedTime);

    void DrawDebugGUI();

    DirectX::XMFLOAT2 ConvertWorldToScreen(
        const DirectX::XMFLOAT3& worldPosition,
        const DirectX::XMMATRIX& view,
        const DirectX::XMMATRIX& projection,
        float screenWidth,
        float screenHeight);

private:
    // シェーダー用定数バッファ構造体
    struct GaugeConstants
    {
        float hpRatio;                  // HP割合
        float tintStrength;             // 色の塗り替え強度
        float useMask;                  // マスクによる切り抜きを行うか
        float padding;
        DirectX::XMFLOAT4 tintColor;    // 塗り替える色
    };

    // ポートレート用定数バッファ構造体
    struct PortraitConstants
    {
        DirectX::XMFLOAT2 circleCenter; // ゲージのグレー円の中心(スクリーン座標)
        float circleRadius;             // グレー円の半径(スクリーン座標)
        float edgeSoftness;             // 縁のぼかし幅

        DirectX::XMFLOAT2 headCenter;   // 頭の突き抜けを許可する楕円の中心
        DirectX::XMFLOAT2 headRadius;   // 頭の突き抜けを許可する楕円の半径
    };

    // HPの遅延表示（受けたダメージ量の可視化）用
    struct HPTracker
    {
        float currentHP = -1.0f;    // 前フレームのHP
        float displayHP = -1.0f;    // 遅れて減少する表示用HP
        float holdTimer = 0.0f;     // 減少開始までの待ち時間
        float damageTimer = 0.0f;   // 被弾リアクションの残り時間
    };

    // HPの遅延表示の更新
    void UpdateHPTracker(HPTracker& tracker, float currentHP, float maxHP, float elapsedTime);

    // プレイヤーの立ち絵をゲージの円に切り抜いて描画
    void RenderPlayerPortrait(ID3D11DeviceContext* dc);

    // --- スプライト ---

    // ロックオン
    std::unique_ptr<Sprite> lockOnPoint;

    // プレイヤー用
    std::unique_ptr<Sprite> spritePlayerHPBack;   // HPGauge.png
    std::unique_ptr<Sprite> spritePlayerHPFill;   // HPBar.png
    std::unique_ptr<Sprite> spritePlayerHPMask;   // HPBarMask.png (テクスチャとして使用)
    std::unique_ptr<Sprite> portraitNormal;        // portrait_kohaku_01.png (平常時)
    std::unique_ptr<Sprite> portraitDamage;        // portrait_kohaku_06.png (被弾時)
    std::unique_ptr<Sprite> counter;
    std::unique_ptr<Sprite> launcher;
    std::unique_ptr<Sprite> counterPC;
    std::unique_ptr<Sprite> launcherPC;

    // ボス用
    std::unique_ptr<Sprite> spriteBossHPBack;     // BossHPGauge.png
    std::unique_ptr<Sprite> spriteBossHPFill;     // BossHPBar.png
    std::unique_ptr<Sprite> spriteBossStockBack;  // HPStockGauge.png
    std::unique_ptr<Sprite> spriteBossStockFill;  // HPStockBar.png

    // 操作説明
    std::unique_ptr<Sprite> padInstructionUI;
    std::unique_ptr<Sprite> keyMouseInstructionUI;

    // --- シェーダー関連 ---
    Microsoft::WRL::ComPtr<ID3D11Buffer> gaugeConstantBuffer;       // 定数バッファ
    Microsoft::WRL::ComPtr<ID3D11PixelShader> gaugePixelShader;     // マスク用PS
    Microsoft::WRL::ComPtr<ID3D11Buffer> portraitConstantBuffer;    // ポートレート用定数バッファ
    Microsoft::WRL::ComPtr<ID3D11PixelShader> portraitPixelShader;  // ポートレート切り抜き用PS

    // プレイヤーパラメータ
    DirectX::XMFLOAT2 playerGaugePos = { 1455.0f, 800.0f }; // 位置
    float playerGaugeScale = 1.1f;                         // スケール

    // --- ポートレート（HPGauge.pngのテクセル基準。ゲージのスケールに追従する） ---
    DirectX::XMFLOAT2 portraitCircleCenter = { 284.5f, 92.5f }; // グレー円の中心
    float portraitCircleRadius = 55.0f;                         // グレー円の半径
    float portraitHeadOffsetY = -22.0f;                         // 頭用楕円の中心オフセット
    DirectX::XMFLOAT2 portraitHeadRadius = { 55.0f, 72.0f };    // 頭用楕円の半径
    float portraitEdgeSoftness = 1.5f;                          // 切り抜きの縁のぼかし幅

    // 立ち絵から顔まわりを切り抜く矩形 (x, y, w, h)
    DirectX::XMFLOAT4 portraitSrcRect = { 783.0f, 180.0f, 1000.0f, 1150.0f };
    float portraitScale = 0.1318f;                              // 立ち絵の縮小率
    DirectX::XMFLOAT2 portraitOffset = { -65.9f, -79.1f };      // 円中心から立ち絵左上へのオフセット

    // 被弾リアクション
    float portraitDamageTime = 0.45f;       // リアクションの継続時間
    float portraitShakeAmplitude = 6.0f;    // 振動の大きさ
    float portraitShakeSpeed = 55.0f;       // 振動の速さ
    DirectX::XMFLOAT3 portraitDamageColor = { 1.0f, 0.25f, 0.2f }; // 被弾時の色

    // --- ダメージ量の可視化 ---
    HPTracker playerHPTracker;
    HPTracker bossHPTracker;
    float damageGaugeHoldTime = 0.35f;      // 減り始めるまでの待ち時間
    float damageGaugeDrainRate = 0.6f;      // 最大HPに対する毎秒の減少割合
    DirectX::XMFLOAT4 damageGaugeColor = { 1.0f, 0.2f, 0.15f, 1.0f }; // ダメージ量表示の色

    // ボスパラメータ
    DirectX::XMFLOAT2 bossGaugePos = { 1360.0f, 50.0f };     // メインゲージ位置
    float bossGaugeScale = 1.1f;                           // スケール

    // ボスストックパラメータ
    DirectX::XMFLOAT2 bossStockStartPos = { 1600.0f, 85.0f };// ストック開始位置
    float bossStockOffset = 30.0f;                         // アイコンの間隔
    float bossStockScale = 1.1f;                           // ストックスケール

    // ロックオンパラメータ
    Enemy* lockOnEnemy = nullptr;
    float lockOnAngle = 0.0f;
    DirectX::XMFLOAT2 lockOnScale = { 40.0f, 40.0f }; // 動的に変化

    DirectX::XMFLOAT2 instructionPos = { 1455.0f, 800.0f }; // 位置
    float instructionScale = 1.1f;                         // スケール

    bool isController = false;
};