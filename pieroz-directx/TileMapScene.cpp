#include "TileMapScene.h"
#include "Channels.h"
#include <stdexcept>
#include <unordered_map>

namespace dx = DirectX;


// Max tiles per batch: 16-bit indices limit = 65535, 4 verts per tile -> 16383 tiles max
static constexpr size_t MAX_TILES_PER_BATCH = 16383;

TileMapScene::TileMapScene(Graphics& gfx, const TileMapDef& def)
	: currentDef(def)
{
	BuildBatches(gfx);
}

void TileMapScene::LoadMap(Graphics& gfx, const TileMapDef& def)
{
	currentDef = def;
	batches.clear();
	totalTileCount = 0;
	BuildBatches(gfx);
}

void TileMapScene::LoadDynamicModel(Graphics& gfx, const std::string& modelPath, float scale)
{
	// Load with unlit flag = true so materials use Unlit_VS/PS
	dynamicModel = std::make_unique<Model>(gfx, modelPath, scale, true);
}

void TileMapScene::SetDynamicModelTransform(DirectX::XMMATRIX transform)
{
	if (dynamicModel)
	{
		dynamicModel->SetRootTransform(transform);
	}
}

bool TileMapScene::HasDynamicModel() const noexcept
{
	return dynamicModel != nullptr;
}

void TileMapScene::LinkTechniques(Rgph::RenderGraph& rg)
{
	for (const auto& batch : batches)
	{
		batch->LinkTechniques(rg);
	}
	if (dynamicModel)
	{
		dynamicModel->LinkTechniques(rg);
	}
}

size_t TileMapScene::Submit(size_t channels) const
{
	for (const auto& batch : batches)
	{
		batch->Submit(channels);
	}

	if (dynamicModel)
	{
		dynamicModel->Submit(channels);
	}

	return totalTileCount;
}

void TileMapScene::BuildBatches(Graphics& gfx)
{
	// Group tiles by texture path
	std::unordered_map<std::string, std::vector<TileBatch::TileInstance>> groups;

	for (const auto& tileDef : currentDef.tiles)
	{
		const float worldX = currentDef.originX + tileDef.col * currentDef.tileSize;
		const float worldZ = currentDef.originZ + tileDef.row * currentDef.tileSize;
		const float worldY = tileDef.height;

		groups[tileDef.texturePath].push_back({ worldX, worldY, worldZ });
	}

	totalTileCount = currentDef.tiles.size();

	// Create batches (split if a group exceeds 16-bit index limit)
	for (const auto& [texturePath, instances] : groups)
	{
		for (size_t offset = 0; offset < instances.size(); offset += MAX_TILES_PER_BATCH)
		{
			const size_t batchSize = std::min(MAX_TILES_PER_BATCH, instances.size() - offset);
			std::vector<TileBatch::TileInstance> batchInstances(
				instances.begin() + offset,
				instances.begin() + offset + batchSize
			);
			batches.push_back(std::make_unique<TileBatch>(
				gfx, currentDef.tileSize, texturePath, batchInstances
			));
		}
	}
}