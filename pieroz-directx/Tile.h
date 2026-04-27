#pragma once
#include "Drawable.h"
#include <string>

// A single textured quad tile rendered with unlit (flat) shading.
// Position is set at construction and stored as a world-space translation.

class Tile : public Drawable
{
public:
	// Creates a tile of given size centered at (worldX, wordlY, worldZ)
	Tile(Graphics& gfx, float size, float worldX, float worldY, float worldZ,
		const std::string& texturePath);
	DirectX::XMMATRIX GetTransformXM() const noexcept override;
private:
	DirectX::XMFLOAT3 pos;
};
