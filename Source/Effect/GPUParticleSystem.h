#pragma once
#include <d3d11.h>
#include <wrl.h>
#include <DirectXMath.h>
#include "Graphics/RenderContext.h"

struct GpuParticleData {
    DirectX::XMFLOAT3 position;
    float lifeTime;
    DirectX::XMFLOAT3 velocity;
    float maxLifeTime;
    DirectX::XMFLOAT4 color;
    float size;
    UINT behaviorType = 1;
    DirectX::XMFLOAT2 padding;
};

// Compute Shaderに毎フレーム送る定数バッファ
struct CbGpuParticleUpdate {
    float deltaTime;
    float totalTime;
    int isRespawn;
    float padding; 
    DirectX::XMFLOAT3 gravity;
    float padding2;            
};

struct CbCamera {
    DirectX::XMFLOAT4X4 view;
    DirectX::XMFLOAT4X4 projection;
    DirectX::XMFLOAT3   cameraPosition;
    float               padding; 
};

class GpuParticleSystem {
public:
    GpuParticleSystem() = default;
    ~GpuParticleSystem() = default;

    HRESULT Initialize(ID3D11Device* device, int maxParticles);

    // コンピュートシェーダーによる更新
    void Update(ID3D11DeviceContext* context, float deltaTime);

    // 描画 (VS -> GS -> PS)
    void Render(ID3D11DeviceContext* context, const RenderContext& rc);

    void Emit(ID3D11DeviceContext* context, const DirectX::XMFLOAT3& position,
        const DirectX::XMFLOAT3& velocity, const DirectX::XMFLOAT4& color,
        float size, float lifeTime, UINT behaviorType);

    void SetRespawnEnable(bool flag) { respawnEnable = flag; }
private:
    int maxParticles = 0;
    int emitIndex = 0;
    float totalTime = 0.0f;
    bool respawnEnable = false;

    // バッファとビュー
    Microsoft::WRL::ComPtr<ID3D11Buffer> particleBuffer;
    Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> particleUAV;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> particleSRV;

    // 定数バッファ (時間や重力などをCSに送る用)
    Microsoft::WRL::ComPtr<ID3D11Buffer> cbUpdate;
    Microsoft::WRL::ComPtr<ID3D11Buffer> cbCamera;

    // シェーダー
    Microsoft::WRL::ComPtr<ID3D11ComputeShader> computeShader;
    Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader;
    Microsoft::WRL::ComPtr<ID3D11GeometryShader> geometryShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader;

    // 半透明描画などのためのステート
    Microsoft::WRL::ComPtr<ID3D11BlendState>          blendStateAdd;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState>   depthStencilState;

    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>  texture;       // パーティクルの画像
    Microsoft::WRL::ComPtr<ID3D11SamplerState>        samplerState;  // テクスチャのサンプラー
};