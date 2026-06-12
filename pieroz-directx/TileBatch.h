#pragma once
#include "Drawable.h"
#include <string>
#include <vector>
#include <optional>
#include <DirectXMath.h>

struct QuadMeasurement
{
	DirectX::XMFLOAT3 v0, v1, v2, v3; // quad corners
	float width;
	float height; // length of left edge (v0->v3)
	float diagonal0; // v0->v2
	float diagonal1; // v1->v3
};

// A batched drawable that merges many tiles sharing the same texture into one draw call.
// Vertices are pre-transformed to world space. Transform return identity.
class TileBatch : public Drawable
{
public:
	struct TileInstance
	{
		float worldX, worldY, worldZ;
		int rotation = 0; // 0-3 0, 90, 180, 270
		int flip = 0;
		// Per-corner altitude offsets (Y): [0](-X,+Z), [1](+X,+Z)
		float altCorners[4] = { 0.0f,0.0f,0.0f,0.0f }; 
	};

	// Build a single mesh from all tile instances (same texture, same tile size)
	TileBatch(Graphics& gfx, float tileSize, const std::string& texturePath,
		const std::vector<TileInstance>& instances);

	DirectX::XMMATRIX GetTransformXM() const noexcept override;

	UINT GetTileCount() const noexcept { return tileCount; }

	// Ray-quad picking: returns measurement of the hig quad, if any.
	std::optional<std::pair<QuadMeasurement, float>> PickQuad(
		DirectX::FXMVECTOR rayOrigin, DirectX::FXMVECTOR rayDir) const;

private:
	UINT tileCount;
	// CPU-side quad vertices for picking(4  per tile: v0,v1,v2,v3)
	std::vector<DirectX::XMFLOAT3> cpuQuadVertices;
};