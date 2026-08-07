#pragma once
#include <DirectXMath.h>

struct GpuParticleData {
    DirectX::XMFLOAT3 position; // 位置
    float lifeTime;             // 残り寿命 (0以下で非アクティブ)
    DirectX::XMFLOAT3 velocity; // 速度
    float maxLifeTime;          // 初期寿命
    DirectX::XMFLOAT4 color;    // 色
    float size;                 // サイズ
    DirectX::XMFLOAT3 padding;  // 16バイトアライメント用のパディング
};