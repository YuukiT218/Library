
float3 ShadowMapFetchPCF(in Texture2DArray shadowmap, SamplerState shadowsampler, int index, float3 texcoord,
    float4 ShadowColor, float shadowattenuation, float shadowbias, float radius = 3.0f)
{
    float3 tex_size;
    shadowmap.GetDimensions(tex_size.x, tex_size.y, tex_size.z);

    //  PCFノイズ
    static const float2 noise[16] =
    {
        { 0.772417068, -0.423055708 },
        { -0.714291692, 0.509235382 },
        { -0.780199528, -0.183640003 },
        { 0.472295880, 0.162610054 },
        { -0.442540765, -0.263137937 },
        { -0.960392714, -0.472289205 },
        { -0.347602427, 0.830676079 },
        { -0.573735476, -0.472531378 },
        { -0.629811406, -0.642059684 },
        { -0.352298439, -0.320050240 },
        { 0.599728346, 0.426071405 },
        { -0.173549354, 0.653944254 },
        { -0.287484705, -0.0727353096 },
        { 0.324079156, 0.534158945 },
        { 0.968484163, -0.229418755 },
        { -0.377048135, 0.338529825 },
    };
    static const uint noise_size = 16;

    float3 attenuation = (float3) 0.0f;
    [unroll]
    for (uint pcf_index = 0; pcf_index < noise_size; ++pcf_index)
    {
        float3 texcoordUVW = float3(texcoord.xy + radius.xx * (noise[pcf_index] / tex_size.xy), index);
        float depth = shadowmap.Sample(shadowsampler, texcoordUVW).r;
        float value = step(texcoord.z, depth + shadowbias);
        attenuation += value.xxx;
    }
    return lerp(ShadowColor.rgb * shadowattenuation, float3(1.0f, 1.0f, 1.0f), attenuation / noise_size);
}

//  ワールド座標をシャドウマップのテクスチャ座標へ変換する
float3 ComputeShadowCoord(float3 worldPosition, row_major float4x4 lightViewProjection)
{
    float4 lightPosition = mul(float4(worldPosition, 1.0f), lightViewProjection);
    lightPosition.xyz /= lightPosition.w;

    float2 uv = lightPosition.xy * float2(0.5f, -0.5f) + 0.5f;
    return float3(uv, lightPosition.z);
}

//  カスケードごとのテクセルのワールドサイズをfloat4から取り出す
float GetCascadeTexelWorldSize(float4 texelWorldSizes, int cascadeIndex)
{
    return cascadeIndex == 0 ? texelWorldSizes.x
         : cascadeIndex == 1 ? texelWorldSizes.y
         : cascadeIndex == 2 ? texelWorldSizes.z
         : texelWorldSizes.w;
}

//  ワールド座標がどのカスケードに含まれるかを調べる
//  手前のカスケードほど解像度が高いので、含まれる最初の段を採用する
//  どの段にも入らない場合は -1 を返す（影を落とさない）
int SelectShadowCascade(float3 worldPosition, int cascadeCount,
    in row_major float4x4 cascadeLightViewProjection[4], out float3 shadowCoord)
{
    shadowCoord = (float3) 0.0f;

    //  PCFで周囲を参照するため、端に余白を持たせて判定する
    const float border = 0.02f;

    int selected = -1;

    [unroll]
    for (int i = 0; i < 4; ++i)
    {
        float3 coord = ComputeShadowCoord(worldPosition, cascadeLightViewProjection[i]);

        bool inside = (i < cascadeCount)
                   && all(coord.xy >= border) && all(coord.xy <= 1.0f - border)
                   && coord.z >= 0.0f && coord.z <= 1.0f;

        //  まだ決まっていなければ、この段を採用する
        if (inside && selected < 0)
        {
            shadowCoord = coord;
            selected = i;
        }
    }

    return selected;
}

//  カスケードごとの確認用の色
float3 GetCascadeDebugColor(int cascadeIndex)
{
    const float3 colors[4] =
    {
        float3(1.0f, 0.4f, 0.4f),
        float3(0.4f, 1.0f, 0.4f),
        float3(0.4f, 0.4f, 1.0f),
        float3(1.0f, 1.0f, 0.4f),
    };

    return colors[clamp(cascadeIndex, 0, 3)];
}
