#pragma once
#include "TileMapDef.h"
#include "Tile.h"
#include "Model.h"
#include "Graphics.h"
#include <vector>
#include <memory>
#include <string>
#include <DirectXMath.h>

namespace Rgph { class RenderGraph; }

// Manages a tile-based scene: a grid of textured tiles plus optional dynamic 3D objects.
// All rendering uses unlit (flat) shading - no shadows, no directional lighting.
class TileMapScene
{
public:
	TileMapScene(Graphics& gfx, const TileMapDef& def);

	// Load/replace the tile map
	void LoadMap(Graphics& gfx, const TileMapDef& def);

	// Load a 3D model into the scene (unlit)
	void LoadDynamicModel(Graphics& gfx, const std::string& modelPath, float scale = 1.0f);
	void SetDynamicModelTransform(DirectX::XMMATRIX transform);
	bool HasDynamicModel() const noexcept;

	// Link all drawables to the render graph
	void LinkTechniques(Rgph::RenderGraph& rg);

	// Submit only visible drawables for rendering (frustrum culling)
	void Submit(size_t channels, Graphics& gfx) const;

	// Access for ImGui inspection
	const TileMapDef& GetMapDef() const noexcept { return currentDef; }
	Model* GetDynamicModel() const noexcept { return dynamicModel.get(); }

private: 
	void BuildTiles(Graphics& gfx);
	static void ExtractFrustumPlanes(DirectX::XMFLOAT4 planes[6], DirectX::FXMMATRIX viewProj) noexcept;
	static bool IsSphereInFrustum(const DirectX::XMFLOAT4 planes[6], const DirectX::XMFLOAT3& center, float radius) noexcept;

	TileMapDef currentDef;
	float cullingRadius = 0.0f; // bounding sphere radius for each tile
	std::vector<std::unique_ptr<Tile>> tiles;
	std::vector<DirectX::XMFLOAT3> tilePositions;
	std::unique_ptr<Model> dynamicModel;
};