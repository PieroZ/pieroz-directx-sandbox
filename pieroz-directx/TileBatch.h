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
	};

	// Build a single mesh from all tile instances (same texture, same tile size)
	TileBatch(Graphics& gfx, float tileSize, const std::string& texturePath,
		const std::vector<TileInstance>& instances);

	DirectX::XMMATRIX GetTransformXM() const noexcept override;

	UINT GetTileCount() const noexcept { return tileCount; }

private:
	UINT tileCount;
};