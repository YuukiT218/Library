#include "GpuParticle.hlsli"

struct GS_IN
{
    float3 pos : POSITION;
    float4 color : COLOR;
    float size : SIZE;
    float lifeTime : LIFETIME;
};

struct PS_IN
{
    float4 pos : SV_POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
};

// 点(point)を入力として受け取り、最大4つの頂点(四角形)を出力します
[maxvertexcount(4)]
void main(point GS_IN gin[1], inout TriangleStream<PS_IN> stream)
{
    // 死んでいるパーティクルはここで処理を打ち切り、描画しません！
    if (gin[0].lifeTime <= 0.0f)
        return;

    float3 pos = gin[0].pos;
    float halfSize = gin[0].size * 0.5f;

    // ビュー行列からカメラの「右方向」と「上方向」を抜き出して、常にカメラを向くようにします
    float3 right = float3(view._11, view._21, view._31);
    float3 up = float3(view._12, view._22, view._32);

    // 四角形の4つの角のローカル座標を計算
    float3 corners[4];
    corners[0] = pos + (right - up) * halfSize; // 右下
    corners[1] = pos + (right + up) * halfSize; // 右上
    corners[2] = pos + (-right - up) * halfSize; // 左下
    corners[3] = pos + (-right + up) * halfSize; // 左上

    float2 uvs[4] =
    {
        float2(1.0f, 1.0f), float2(1.0f, 0.0f),
        float2(0.0f, 1.0f), float2(0.0f, 0.0f)
    };

    float4x4 viewProj = mul(view, projection);

    // 頂点を生成してストリームに流し込む
    [unroll]
    for (int i = 0; i < 4; ++i)
    {
        PS_IN pout;
        pout.pos = mul(float4(corners[i], 1.0f), viewProj);
        pout.color = gin[0].color;
        pout.uv = uvs[i];
        stream.Append(pout);
    }
}