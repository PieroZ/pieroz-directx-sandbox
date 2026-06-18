// Emissive prim lights (e.g. lamps) - an array of points lights provided by light
// emitting prims placed in the scene. All positions are in VIEW space.
#define MAX_EMISSIVE_LIGHTS 16

struct EmissiveLight
{
    float3 viewPos;
    float pad0;
    float3 color;
    float intensity;
    float attConst;
    float attLin;
    float attQuad;
    float pad1;
};

cbuffer EmissiveLightsCBuf : register(b3)
{
    EmissiveLight emissiveLights[MAX_EMISSIVE_LIGHTS];
    int emissiveLightCount;
    float3 emissivePad;
}

//Accumuilate diffuse + specular contribution from all active emissive emissiveLights.
float3 ComputeEmissiveLights(float3 viewFragPos, float3 viewNormal, float3 viewDir)
{
    float3 total = float3(0.0f, 0.0f, 0.0f);
    [loop]

    for( int i = 0; i < emissiveLightCount; ++i)
    {
        const float3 toL = emissiveLights[i].viewPos - viewFragPos;
        const float dist = length(toL);
        const float3 dirToL = toL / max(dist, 1e-5f);
        const float att = 1.0f / (emissiveLights[i].attConst +
        emissiveLights[i].attLin * dist +
        emissiveLights[i].attQuad * dist * dist);
        const float ndl = max(0.0f, dot(dirToL, viewNormal));
        const float3 halfDir = normalize(dirToL + viewDir);
        const float spec = pow(max(0.0f, dot(viewNormal, halfDir)), 32.0f);
        total += emissiveLights[i].color * emissiveLights[i].intensity * att *
        (ndl + spec * 0.4f);

    }
    
    return total;
}