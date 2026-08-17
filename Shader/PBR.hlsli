struct VS_OUT
{
    float4 vertex : SV_POSITION;
    float2 texcoord : TEXCOORD;
    float3 normal : NORMAL;
    float4 position : POSITION;
    float3 tangent : TANGENT;
    float3 shadow : SHADOW;
};

#define POINT_MAX (64)

cbuffer CbMesh : register(b0)
{
    float4 materialColor;
};
// register b2 ワールド変換用にskeltonで使用

cbuffer CbMaterial : register(b1)
{
    float adjustMetalness; //  金属質調整
    float adjustRoughness; //  粗さ調整
    float normalScale;     //法線マップのスケール
    float metalicFactor; //金属度
    float roughnessFactor; //粗さ
    float occlusionStrength; //強度
    float metalicindex;
    float material_Dummy;
    //float2 material_dummy; //16bite区切り用ダミー
    float4 emissiveColor; //エミッシブ色
};

cbuffer CbColor : register(b2)
{
    float isEmissive; //強調発光するかどうか
    float emissiveFactor; //発光度
    float hasEmissiveTexture; //エミッシブテクスチャを持っているか
    float color_dummy;
    float4 adjustColor; //色の調整
}


cbuffer CbSetUp : register(b3)
{
    float IBLDiffuseScale; //ディフューズ調整用
    float IBLSpecularScale; //スペキュラー調整用
    
    float2 SetUpDummy;
    //float4 setupdummy;
}

//シャドウマップ用の定数
//	カスケードシャドウマップ
static const int ShadowBufferSize = 4;
cbuffer CbShadow : register(b4)
{
    row_major float4x4 CascadeLightViewProjection[ShadowBufferSize];

    //  各カスケードの終端距離
    float4 cascadeSplits;

    //  影の色
    float4 shadowColor;

    //  x: カスケードを色分け表示するか
    float4 cascadeFlags;

    //  UV空間での1ピクセルのサイズ
    float shadowTexelSize;

    //  影の濃さ
    float shadowAttenuation;

    //  深度値補正
    float shadowBias;

    //  法線方向へずらす量(法線マップの起伏で影が途切れるのを防ぐ)
    float shadowNormalOffset;

    //  間接光にも影をどれだけ効かせるか
    float indirectShadowStrength;

    float3 shadowPadding;

    //  カスケードごとのシャドウマップ1テクセルのワールドサイズ
    float4 cascadeTexelWorldSize;
};

cbuffer CbConstants : register(b5)
{
    float emissivedissolve;
    float dissolve;
    float alphaFactor;
    float dummyconstants;
    float4 OverwriteColor;
}

cbuffer CbScene : register(b7)
{
    row_major float4x4 viewProjection;
    float4 lightDirection;
    float4 lightColor;
    float4 cameraPosition;
    row_major float4x4 lightViewProjection;
    float4 pointLight[POINT_MAX];
    float4 pointColor[POINT_MAX];
    float4 targetPosition;
};

cbuffer CbRimLight : register(b8)
{
    float rimPower;
    float rimIntensity;
    float4 rimColor;
    float2 rimDummy;
}

cbuffer CbTeleport : register(b9)
{
    float teleportProgress; // 0.0?1.0 (0=通常, 1=完全テレポート)
    float teleportTime; // アニメーション用経過時間
    float dissolveEdgeWidth; // ディゾルブエッジの幅
    float distortionIntensity; // 歪み強度
    
    float3 dissolveEdgeColor; // エッジの発光色
    float teleportCenterY; // テレポート中心のY座標
    
    float afterimageAlpha;
    float afterimageDarkness;
    float enableDissolve;
    float enableDistortion;
    float enableDither;
    float3 teleportDummy;

    //  残像の色味(rgb)と明るさ(a)
    float4 afterimageColor;
}