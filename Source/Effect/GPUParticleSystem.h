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
    int m_maxParticles = 0;
    int m_emitIndex = 0;
    float m_totalTime = 0.0f;
    bool respawnEnable = false;

    // バッファとビュー
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_particleBuffer;
    Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> m_particleUAV;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_particleSRV;

    // 定数バッファ (時間や重力などをCSに送る用)
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_cbUpdate;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_cbCamera;

    // シェーダー
    Microsoft::WRL::ComPtr<ID3D11ComputeShader> m_computeShader;
    Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vertexShader;
    Microsoft::WRL::ComPtr<ID3D11GeometryShader> m_geometryShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> m_pixelShader;

    // 半透明描画などのためのステート
    Microsoft::WRL::ComPtr<ID3D11BlendState>          m_blendStateAdd;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState>   m_depthStencilState;

    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>  m_texture;       // パーティクルの画像
    Microsoft::WRL::ComPtr<ID3D11SamplerState>        m_samplerState;  // テクスチャのサンプラー
};