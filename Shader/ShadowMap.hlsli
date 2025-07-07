
float3 ShadowMapFetchPCF(in Texture2DArray shadowmap, SamplerState shadowsampler, int index, float3 texcoord,
    float4 ShadowColor, float shadowattenuation, float shadowbias, float radius = 3.0f)
{
    float3 tex_size;
    shadowmap.GetDimensions(tex_size.x, tex_size.y, tex_size.z);

    //  PCFÉmÉCÉY
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
        float depth = shadowmap.Sample(shadowsampler, texcoordUVW);
        float value = step(texcoord.z, depth + shadowbias);
        attenuation += value.xxx;
    }
    return lerp(ShadowColor.rgb * shadowattenuation, float3(1.0f, 1.0f, 1.0f), attenuation / noise_size);
}