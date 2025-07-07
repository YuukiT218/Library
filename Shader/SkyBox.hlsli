struct VS_OUT
{
    float4 position : SV_Position;
    float4 color : COLOR;
    float4 worldPosition : WORLD_POSITION;
};

cbuffer SCENE_CONSTANT_BUFFER : register(b1)
{
    row_major float4x4 viewProjection;
    //float4 options;
    float4 cameraPosition;
    row_major float4x4 inverseViewProjwction;
}