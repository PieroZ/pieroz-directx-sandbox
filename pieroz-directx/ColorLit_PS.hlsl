#include "ShaderOps.hlsli"
#include "LightVectorData.hlsli"
#include "PointLight.hlsli"
#include "EmissiveLights.hlsli"

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
    
    const float ndl = max(0.0f, dot(lv.dirToL, viewNormal));
    
    const float3 coloredLight =
    lightTint * lightTintIntensity *
    (ambient + diffuseColor * diffuseIntensity * att * ndl);

    // Specular highlight using the same colored light
    const float3 viewDir =
        normalize(-viewFragPos);

    const float3 halfDir =
        normalize(lv.dirToL + viewDir);

    const float specFactor =
        pow(max(0.0f, dot(viewNormal, halfDir)), 32.0f);

    const float3 specular =
    diffuseColor * diffuseIntensity *
    lightTint * lightTintIntensity *
    att * specFactor * 0.4f;

    // Albedo comes from the texture, optionally tinted by the material color
    const float3 albedo = texColor.rgb * materialColor;
    
    // Emissive prim lights(lamps placed in the scene)
    const float3 emissive = ComputeEmissiveLights(viewFragPos, viewNormal, viewDir);
    
    // Flashlight (spotlight) contribution - defined entirely in view space so it shines from the observer's perspective along a steerable cone.
    float3 spotContribution = float3(0.0f, 0.0f, 0.0f);
    if(spotEnabled > 0.5f)
    {
        const float3 fragToSpot = spotPos - viewFragPos;
        const float spotDist = length(fragToSpot);
        const float3 dirToSpot = fragToSpot / max(spotDist, 1e-5f);
        // cosine of the angle between the cone axis and the direction to this fragment
        const float theta = dot(-dirToSpot, spotDir);
        const float coneFalloff = smoothstep(spotOuterCos, spotInnerCos, theta);
        if (coneFalloff > 0.0f)
        {
            const float spotAtt = saturate(1.0f - spotDist / spotRange);
            const float spotNdl = max(0.0f, dot(dirToSpot, viewNormal));
            const float3 spotHalf = normalize(dirToSpot + viewDir);
            const float spotSpec = pow(max(0.0f, dot(viewNormal, spotHalf)), 32.0f);
            spotContribution =
                spotColor * spotIntensity * coneFalloff * spotAtt *
            (spotNdl + spotSpec * 0.4f);
        }

    }

    //const float3 finalColor = albedo * (1.0f + coloredLight) + specular;
    const float3 finalColor = albedo * (coloredLight + spotContribution + emissive) + specular;
    
    // Texture modulated by lighting
    return float4(finalColor, texColor.a);
}