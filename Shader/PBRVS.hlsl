#include "Skinning.hlsli"
#include "PBR.hlsli"

VS_OUT main(
    float4 position : POSITION,
    float4 boneWeight : BONE_WEIGHTS,
    uint4 boneIndices : BONE_INDICES,
    float2 texcoord : TEXCOORD,
    float3 normal : NORMAL,
    float3 tangent : TANGENT)
{
    VS_OUT vout = (VS_OUT) 0;
    
    position = SkinningPosition(position, boneWeight, boneIndices);
    //vout.vertex = mul(position, viewProjection);
    // 指定の位置に配置できるようにワールド行列を計算に含める
    //vout.vertex = mul(position, mul(worldTransform, viewProjection));
    vout.vertex = mul(position, viewProjection);
    vout.texcoord = texcoord;
    
    vout.normal = SkinningVector(normal, boneWeight, boneIndices);
    
    // 視線ベクトルを求めるためにスキニング後のワールド座標をピクセルシェーダーに渡す
    vout.position = position.xyz;
    
    vout.tangent = SkinningVector(tangent, boneWeight, boneIndices);
    
    // ライトから見たNDC座標を算出
    float4 shadow = mul(position, lightViewProjection);
    shadow.xyz /= shadow.w;
    shadow.y = -shadow.y;
    // NDC座標をUV座標に変換
    shadow.xy = shadow.xy * 0.5f + 0.5f;
    vout.shadow = shadow.xyz;
    
    return vout;
}