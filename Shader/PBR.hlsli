struct VS_OUT
{
    float4 vertex : SV_POSITION;
    float2 texcoord : TEXCOORD;
    float3 normal : NORMAL;
    float3 position : POSITION;
    float3 tangent : TANGENT;
    float3 shadow : SHADOW;
};

#define POINT_MAX (64)
cbuffer CbScene : register(b0)
{
    row_major float4x4 viewProjection;
    float4 lightDirection;
    float4 lightColor;
    float4 cameraPosition;
    row_major float4x4 lightViewProjection;
    float4 pointLight[POINT_MAX];
    float4 pointColor[POINT_MAX];
};

cbuffer CbMesh : register(b1)
{
    float4 materialColor;
    // ボーン行列にワールド行列の計算が加味されているので必要なくなった
    //row_major float4x4 worldTransform;
};
// register b2 ワールド変換用にskeltonで使用

cbuffer CbMaterial : register(b3)
{
    float adjustMetalness; //  金属質調整
    float adjustRoughness; //  粗さ調整
    //float normalScale;     //法線マップのスケール
    float metalicFactor; //金属度
    float roughnessFactor; //粗さ
    float occlusionStrength; //強度
    float metalicindex;
    float2 material_dummy; //16bite区切り用ダミー
    float4 emissiveColor; //エミッシブ色
};

cbuffer CbColor : register(b4)
{
    float isEmissive; //強調発光するかどうか
    float emissiveFactor; //発光度
    float2 color_dummy;
    float4 adjustColor; //色の調整
}

cbuffer CbConstants : register(b5)
{
    float emissivedissolve;
    float dissolve;
    float alphaFactor;
    float dummyconstants;
    float4 OverwriteColor;
}

cbuffer CbSetUp : register(b6)
{
    float IBLDiffuseScale; //ディフューズ調整用
    float IBLSpecularScale; //スペキュラー調整用
    
    float2 SetUpDummy;
    float4 setupdummy;
}

//シャドウマップ用の定数
//	カスケードシャドウマップ
static const int ShadowBufferSize = 4;
cbuffer CbShadow : register(b7)
{
    row_major float4x4 CascadeLightViewProjection[ShadowBufferSize];
    float4 CascadeShadowBias;
    
    float4 cascadeFlags; // DisplayCascadeArea, IsCascade を float にし、余りを使う
    
    float4 shadowColor;
    // UV空間での1ピクセルのサイズ
    float shadowTexelSize;

    float shadowAttenuation; //影の強さ
    
    float shadowBias;
    
    float dummy; //パディング
};