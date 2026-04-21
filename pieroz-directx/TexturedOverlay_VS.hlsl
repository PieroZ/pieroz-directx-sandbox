#include "Transform.hlsli"

struct VSOut
{
    float2 tc : TEXCOORD;
    float4 pos : SV_Position;
};

VSOut main(float3 pos : POSITION, float2 tc : Texcoord)
{
    VSOut vso;
    vso.pos = mul(float4(pos, 1.0f), modelViewProj);
    vso.tc = tc;
    return vso;
}