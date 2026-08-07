Texture2D tex : register(t0);
SamplerState sam : register(s0);

struct PS_IN
{
    float4 pos : SV_POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
};

float4 main(PS_IN pin) : SV_TARGET
{
    // テクスチャの色を取得し、パーティクルの色（アルファ値含む）と掛け合わせます
    float4 texColor = tex.Sample(sam, pin.uv);
    return texColor * pin.color;
}