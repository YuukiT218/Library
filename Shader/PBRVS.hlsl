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
	// テレポートエフェクト: Distortion (空間歪み)
    if (enableDistortion > 0.5 && teleportProgress > 0.0)
    {
        float3 teleportCenter = float3(position.x, teleportCenterY, position.z);
        float3 toCenter = position.xyz - teleportCenter;
        float distance = length(toCenter);
        
        float waveFreq = 3.0;
        float waveSpeed = 5.0;
        float wave = sin(distance * waveFreq - teleportTime * waveSpeed) * teleportProgress;
        
        float3 distortionDir = normalize(toCenter);
        float3 distortion = distortionDir * wave * distortionIntensity;
        
        float angle = atan2(toCenter.x, toCenter.z);
        float spiralWave = sin(angle * 3.0 + distance * 2.0 - teleportTime * 3.0);
        float3 tangentDir = float3(-toCenter.z, 0, toCenter.x);
        float tangentLength = length(tangentDir);
        if (tangentLength > 0.001)
        {
            tangentDir = normalize(tangentDir);
            distortion += tangentDir * spiralWave * teleportProgress * 0.3 * distortionIntensity;
        }
        
        distortion.y += teleportProgress * teleportProgress * 2.0 * distortionIntensity;
        
        position.xyz += distortion;
    }
    // 指定の位置に配置できるようにワールド行列を計算に含める
    vout.vertex = mul(position, viewProjection);
    vout.texcoord = texcoord;
    
    vout.normal = SkinningVector(normal, boneWeight, boneIndices);
    
    // 視線ベクトルを求めるためにスキニング後のワールド座標をピクセルシェーダーに渡す
    vout.position = position;
    
    vout.tangent = SkinningVector(tangent, boneWeight, boneIndices);

    float4 shadow = mul(position, lightViewProjection);
    shadow.xyz /= shadow.w;
    shadow.y = -shadow.y;
    shadow.xy = shadow.xy * 0.5f + 0.5f;
    vout.shadow = shadow.xyz;

    return vout;
}