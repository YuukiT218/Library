//TrailRenderer
struct VS_IN
{
    float4 position : POSITION;
    float4 color : COLOR;
    float2 texcoord : TEXCOORD;
    float dissolve : DISSOLVE;
};

struct VS_OUT
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 texcoord : TEXCOORD;
    float dissolve : DISSOLVE;
};

cbuffer CbScene : register(b0)
{
    row_major float4x4 view_projection;
    float2 Direction; //ディゾルブのUVスクロール
    float Timer; //UVスクロール用タイマー
    float dummy;
    
    float4 lightDirection;
    float4 lightColor;
};
