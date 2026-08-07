#include "GpuParticle.hlsli"

RWStructuredBuffer<Particle> ParticleBuffer : register(u0);

float rand(float2 seed)
{
    return frac(sin(dot(seed, float2(12.9898, 78.233))) * 43758.5453);
}

[numthreads(256, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint index = DTid.x;

    Particle p = ParticleBuffer[index];

    if (p.lifeTime > 0.0f)
    {
        p.lifeTime -= deltaTime;
        // --- 挙動タイプによる分岐 ---
        if (p.behaviorType == 0)
        {
        // 【Type 0: 物理スパーク (剣のヒットなど)】
        // 重力の影響を受け、放物線を描いて落ちる
            p.velocity += gravity * deltaTime;
            p.position += p.velocity * deltaTime;
        
        }
        else if (p.behaviorType == 1)
        {
        // 【Type 1: マジックチャージ / アンチグラビティ】
        // 空気抵抗で急減速しつつ、ゆっくり上に昇る
            p.velocity *= exp(-5.0f * deltaTime); // 強い空気抵抗
            p.position += p.velocity * deltaTime;
            p.position.y += 1.5f * deltaTime; // フワッと浮き上がる
        
        }
        else if (p.behaviorType == 2)
        {
        // 【Type 2: オーラ / 闇の霧】
        // 速度はそのままに、サイン波でブルブル震えながら漂う
            p.position += p.velocity * deltaTime;
            p.position.x += sin(totalTime * 10.0f + p.position.y) * 0.05f;
            p.position.z += cos(totalTime * 8.0f + p.position.x) * 0.05f;
        }

		// フェードアウト処理などは共通で行う
        float normalizedLife = saturate(p.lifeTime / p.maxLifeTime);
        p.color.a = normalizedLife;
    }
    else
    {
        if (isRespawn == 1)
        {
            float rx = rand(float2(index, totalTime)) * 100.0f - 50.0f;
            float ry = rand(float2(index + 1.0f, totalTime)) * 50.0f;
            float rz = rand(float2(index + 2.0f, totalTime)) * 100.0f - 50.0f;

            p.position = float3(rx, ry, rz);
            p.velocity = float3(0, 0, 0);
            p.maxLifeTime = 3.0f + rand(float2(index, totalTime + 1.0f)) * 2.0f;
            p.lifeTime = p.maxLifeTime;
            p.color = float4(1.0f, 1.0f, 0.0f, 0.0f);
            p.size = 0.5f;
        }
        else
        {
            p.position = float3(0, -9999.0f, 0);
        }
    }

    ParticleBuffer[index] = p;
}