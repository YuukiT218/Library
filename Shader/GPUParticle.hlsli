struct Particle
{
    float3 position;
    float lifeTime;
    float3 velocity;
    float maxLifeTime;
    float4 color;
    float size;
    uint behaviorType;
    float2 padding;
};

// Compute Shader用の定数バッファ (b0)
cbuffer CbGpuParticleUpdate : register(b0)
{
    float deltaTime;
    float totalTime;
    int isRespawn;
    float padding;
    float3 gravity;
    float padding2;
};

// 描画(GS)用のカメラ定数バッファ (b1)
cbuffer CbCamera : register(b1)
{
    row_major float4x4 view;
    row_major float4x4 projection;
    float3 cameraPosition;
};