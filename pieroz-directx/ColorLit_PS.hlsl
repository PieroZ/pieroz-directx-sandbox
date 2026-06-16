#include "ShaderOps.hlsli"
#include "LightVectorData.hlsli"
#include "PointLight.hlsli"

Texture2D tex : register(t0);
SamplerState splr : register(s0);

cbuffer ObjectCBuf : register(b1)
{
    float3 materialColor;
    float padding0;
    float3 lightTint;
    float lightTintIntensity;
};

float4 main(
    float3 viewFragPos : Position,
    float3 viewNormal : Normal,
    float2 tc : Texcoord
) : SV_Target
{
    // Sample texture
    float4 texColor = tex.Sample(splr, tc);

    // Respect alpha cutout like Unlit shader
    clip(texColor.a - 0.1f);

    // Normalize normal
    viewNormal = normalize(viewNormal);

    // Light vector data
    const LightVectorData lv =
        CalculateLightVectorData(viewLightPos, viewFragPos);

    // Attenuation
    const float att =
        Attenuate(attConst, attLin, attQuad, lv.distToL);

    // Colored light
    const float3 lightColor =
        lightTint * lightTintIntensity;

    // Diffuse
    const float3 diffuse =
        lightColor * att *
        max(0.0f, dot(lv.dirToL, viewNormal));

    // Specular
    const float3 viewDir =
        normalize(-viewFragPos);

    const float3 halfDir =
        normalize(lv.dirToL + viewDir);

    const float specFactor =
        pow(max(0.0f, dot(viewNormal, halfDir)), 32.0f);

    const float3 specular =
        lightColor * att * specFactor * 0.4f;

    // Final lighting
    const float3 lighting =
        (diffuse + ambient) * materialColor +
        specular;

    // Texture modulated by lighting
    return float4(ambient.rgb, 1.0f);
}