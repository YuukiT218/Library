#include "GpuParticle.hlsli"

// 読み取り専用バッファ (SRV)
StructuredBuffer<Particle> ParticleBuffer : register(t0);

struct VS_OUT
{
    float3 pos : POSITION;
    float4 color : COLOR;
    float size : SIZE;
    float lifeTime : LIFETIME;
};

// SV_VertexID は「今何番目の頂点を処理しているか」を自動で教えてくれるシステム変数です
VS_OUT main(uint id : SV_VertexID)
{
    VS_OUT vout;
    Particle p = ParticleBuffer[id];
    
    // 取得したデータをそのままジオメトリシェーダーに横流しします
    vout.pos = p.position;
    vout.color = p.color;
    vout.size = p.size;
    vout.lifeTime = p.lifeTime;
    
    return vout;
}