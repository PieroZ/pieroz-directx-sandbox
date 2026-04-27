#pragma once
#include "TileMapDef.h"
#include "Tile.h"
#include "Model.h"
#include <vector>
#include <memory>
#include <string>

class Graphics;
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

	// Submit all drawables for rendering
	void Submit(size_t channels) const;

	// Access for ImGui inspection
	const TileMapDef& GetMapDef() const noexcept { return currentDef; }
	Model* GetDynamicModel() const noexcept { return dynamicModel.get(); }

private: 
	void BuildTiles(Graphics& gfx);

	TileMapDef currentDef;
	std::vector<std::unique_ptr<Tile>> tiles;
	std::unique_ptr<Model> dynamicModel;
};