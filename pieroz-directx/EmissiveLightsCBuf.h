#pragma once
#include <DirectXMath.h>


// Maximum number of emissive prim lights uploaded to the GPU in a single frame.
// Must match MAX_EMISSIVE_LIGHTS in EmissiveLights.hlsli.
static constexpr int MAX_EMISSIVE_LIGHTS = 16;

// One emissive point light, stored in VIEW space when uploaded to the GPU
// Layout must match the Emissive Light struct in EmissiveLights.hlsli (64 bytes).
struct EmissiveLightGPU
{
	DirectX::XMFLOAT3 viewPos = { 0.0f, 0.0f, 0.0f };
	float pad0 = 0.0f;
	DirectX::XMFLOAT3 color = { 1.0f, 1.0f, 1.0f };
	float intensity = 0.0f;
	DirectX::XMFLOAT3 viewDir = { 0.0f, -1.0f, 0.0f }; // beam direction (View space)
	float spotCosInner = 1.0f;
	float attConst = 1.0f;
	float attLin = 0.045f;
	float attQuad = 0.0075f;
	float spotCosOuter = -2.0f;
};

// Constant buffer mirroring EmissiveLighsCBuf (register b3) in the HLSL.
struct EmissiveLightsCBuf
{
	EmissiveLightGPU lights[MAX_EMISSIVE_LIGHTS];
	int count = 0;
	float pad2[3] = { 0.0f, 0.0f, 0.0f };
};