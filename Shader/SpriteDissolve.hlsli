struct VS_OUT
{
    float4 position : SV_Position;
    float4 color : COLOR;
    float2 texcoord : TEXCOORD;
};

cbuffer SCENE_CONSTANT_BUFFER : register(b1)
{
    row_major float4x4 view_projection;
    float4 options;
}