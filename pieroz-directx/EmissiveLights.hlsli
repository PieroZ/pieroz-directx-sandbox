// Emissive prim lights (e.g. lamps) - an array of points lights provided by light
// emitting prims placed in the scene. All positions are in VIEW space.
#define MAX_EMISSIVE_LIGHTS 16

struct EmissiveLight
{
    float3 viewPos;
    float pad0;
    float3 color;
    float intensity;
    float3 viewDir; // beam direction in view space
    float spotCosInner; // cos(inner half-angle)
    float attConst;
    float attLin;
    float attQuad;
    float spotCosOuter; // cos(outer half-angl); <= -1.5 means no cone (omni)
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
        
        // Spolight cone shaping: only light fragments inside the beam cone.
        // dir ToL points fragment-> light so -dirToL is light->fragment, which
        // we compare against the beam direction. Soft edge between inner/outer angle.
        float spotFactor = 1.0f;
        if(emissiveLights[i].spotCosOuter>-1.5f)
        {
            const float cosAngle = dot(-dirToL, emissiveLights[i].viewDir);
            spotFactor = smoothstep(
            emissiveLights[i].spotCosOuter,
            emissiveLights[i].spotCosInner,
            cosAngle);

        }
        const float ndl = max(0.0f, dot(dirToL, viewNormal));
        const float3 halfDir = normalize(dirToL + viewDir);
        const float spec = pow(max(0.0f, dot(viewNormal, halfDir)), 32.0f);
        total += emissiveLights[i].color * emissiveLights[i].intensity * att * spotFactor *
        (ndl + spec * 0.4f);

    }
    
    return total;
}