// 定数バッファ（ModelRenderer側から色を送る）
cbuffer CbSilhouette : register(b0)
{
    float4 Color;
};

float4 main() : SV_TARGET
{
    return Color; // 単色を返すだけ
}