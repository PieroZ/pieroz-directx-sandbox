#pragma once
#include <DirectXMath.h>

// Per-prim-type emissive light definition.
// Keyed by prim index (e.g. prim index 1 == nprim001.prm). All placed instances
// of that prim type emite a point light using these properties. The emission point
// is expressed in the prim's local space (relative to its origin) so it follows
// the prim wherever it is placed and however it is rotated.
struct PrimLightDef
{
	bool enabled = false;
	DirectX::XMFLOAT3 offset = { 0.0f, 1.0f, 0.0f }; // local emission point
	DirectX::XMFLOAT3 color = { 1.0f, 0.85f, 0.6f }; // warm lamp glow by default
	float intensity = 2.0f;
	float attConst = 1.0f;
	float attLin = 0.045f;
	float attQuad = 0.0075f;

	// Cone/ spolight shaping. When 'spotlight' is true the light only illuminates inside a cone aimed along direction
	bool spotlight = true;
	DirectX::XMFLOAT3 direction = { 0.0f, -1.0f, 0.0f }; // local beam direction (downward)
	float innerConeDeg = 25.0f;
	float outerConeDeg = 40.0f;
};