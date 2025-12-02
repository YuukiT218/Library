//TrailRendererVS
#include "TrailRenderer.hlsli"

VS_OUT main(VS_IN vin)
{
    VS_OUT vout;
    vout.position = mul(vin.position, view_projection);
    vout.color = vin.color;
    vout.texcoord = vin.texcoord;
    vout.dissolve = vin.dissolve;
    
    return vout;
}
