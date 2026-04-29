#pragma once
#include "TileMapDef.h"
#include "TileBatch.h"
#include "Model.h"
#include "Graphics.h"
#include <vector>
#include <memory>
#include <string>
#include <DirectXMath.h>

namespace Rgph { class RenderGraph; }

// Manages a tile-based scene: a grid of textured tiles plus optional dynamic 3D objects.
// Tiles sharing the same texture are merged into batched draw calls for performance.
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

	// Submit batches for rendering. Returns total tile count submitted.
	size_t Submit(size_t channels) const;

	// Draw distance control (0 = unlimited) - requires rebuild
	void SetDrawDistance(float dist) noexcept { drawDistance = dist; }
	float GetDrawDistance() const noexcept { return drawDistance; }

	// Access for ImGui inspection
	const TileMapDef& GetMapDef() const noexcept { return currentDef; }
	Model* GetDynamicModel() const noexcept { return dynamicModel.get(); }
	size_t GetBatchCount() const noexcept { return batches.size(); }

private: 
	void BuildBatches(Graphics& gfx);

	TileMapDef currentDef;
	float drawDistance = 0.0f; // max render distance from camera (0 = unlimited)
	std::vector<std::unique_ptr<TileBatch>> batches;
	size_t totalTileCount = 0;
	std::unique_ptr<Model> dynamicModel;
};