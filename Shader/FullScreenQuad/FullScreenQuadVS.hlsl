#include "FullScreenQuad.hlsli"

// SV_VERTEXIDには頂点の番号が送られる。
// 4頂点で描画した場合は(0～3)の番号が渡される
VS_OUT main(uint vertexId : SV_VERTEXID)
{
    const float2 position[4] =
    {
        { -1, +1 }, // 左上
        { +1, +1 }, // 右上
        { -1, -1 }, // 左下
        { +1, -1 } // 右下
    };
    const float2 texcoords[4] =
    {
        { 0, 0 }, // 左上
        { 1, 0 }, // 右上
        { 0, 1 }, // 左下
        { 1, 1 } // 右下
    };
    VS_OUT vout;
    vout.position = float4(position[vertexId], 0, 1);
    vout.texcoord = texcoords[vertexId];
    return vout;
}