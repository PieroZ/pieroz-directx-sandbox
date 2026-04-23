#include "Transform.hlsli"

struct VSOut
{
    float2 tc : TEXCOORD;
    float4 pos : SV_Position;
};

// Accepts same layout as PhongDif (GetRenderTargetSamplePosition+normalize+Texcoord) but ignores normalize
VSOut main(float3 pos: Position, float3 n : Normal, float2 tc : Texcoord)
{
    VSOut vso;
    vso.pos = mul(float4(pos, 1.0f), modelViewProj);
    vso.tc = tc;
    return vso;
}