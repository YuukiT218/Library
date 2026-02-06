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
        float hpRatio;
        float padding[3];
    };

    // --- スプライト ---

    // ロックオン
    std::unique_ptr<Sprite> lockonpoint;

    // プレイヤー用
    std::unique_ptr<Sprite> spritePlayerHP_Back;   // HPGauge.png
    std::unique_ptr<Sprite> spritePlayerHP_Fill;   // HPBar.png
    std::unique_ptr<Sprite> spritePlayerHP_Mask;   // HPBarMask.png (テクスチャとして使用)

    // ボス用
    std::unique_ptr<Sprite> spriteBossHP_Back;     // BossHPGauge.png
    std::unique_ptr<Sprite> spriteBossHP_Fill;     // BossHPBar.png
    std::unique_ptr<Sprite> spriteBossStock_Back;  // HPStockGauge.png
    std::unique_ptr<Sprite> spriteBossStock_Fill;  // HPStockBar.png

    std::unique_ptr<Sprite> padInstructionUI;
    std::unique_ptr<Sprite> keyMouInstructionUI;

    // --- シェーダー関連 ---
    Microsoft::WRL::ComPtr<ID3D11Buffer> gaugeConstantBuffer;       // 定数バッファ
    Microsoft::WRL::ComPtr<ID3D11PixelShader> gaugePixelShader;     // マスク用PS

    // プレイヤーパラメータ
    DirectX::XMFLOAT2 playerGaugePos = { 1455.0f, 800.0f }; // 位置
    float playerGaugeScale = 1.1f;                         // スケール

    // ボスパラメータ
    DirectX::XMFLOAT2 bossGaugePos = { 1360.0f, 50.0f };     // メインゲージ位置
    float bossGaugeScale = 1.1f;                           // スケール

    // ボスストックパラメータ
    DirectX::XMFLOAT2 bossStockStartPos = { 1660.0f, 85.0f };// ストック開始位置
    float bossStockOffset = 30.0f;                         // アイコンの間隔
    float bossStockScale = 1.1f;                           // ストックスケール

    // ロックオンパラメータ
    Enemy* lockonEnemy = nullptr;
    float lockonAngle = 0.0f;
    DirectX::XMFLOAT2 lockonScale = { 40.0f, 40.0f }; // 動的に変化

    DirectX::XMFLOAT2 instructionPos = { 1455.0f, 800.0f }; // 位置
    float instructionScale = 1.1f;                         // スケール
};