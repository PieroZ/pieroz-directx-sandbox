cbuffer PointLightCBuf : register(b0)
{
    float3 viewLightPos;
    float3 ambient;
    float3 diffuseColor;
    float diffuseIntensity;
    float attConst;
    float attLin;
    float attQuad;
    // Flashlight / spotlight (observer's torch) - all in VIEW space
    float3 spotPos;
    float3 spotDir;
    float3 spotColor;
    float spotInnerCos;
    float spotOuterCos;
    float spotRange;
    float spotIntensity;
    float spotEnabled;
};