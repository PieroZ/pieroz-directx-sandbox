#pragma once
#include "Drawable.h"
#include <string>
#include <vector>
#include <DirectXMath.h>

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

private:
	UINT tileCount;
};