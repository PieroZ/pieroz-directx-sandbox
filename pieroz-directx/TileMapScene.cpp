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

	// Build a 2D lookup of alt values for corner computation
	// Find grid dimensions
	int maxCol = 0, maxRow = 0;
	for (const auto& t : currentDef.tiles)
	{
		if (t.col > maxCol) maxCol = t.col;;
		if (t.row > maxRow) maxRow = t.row;;
	}
	const int gridW = maxCol + 1;
	const int gridH = maxRow + 1;

	//Create alt grid (default 0)
	std::vector<int> altGrid(gridW * gridH, 0);
	for (const auto& t : currentDef.tiles)
	{
		altGrid[t.row * gridW + t.col] = t.alt;
	}

	// Helper to safely get alt at grid position
	// FIXME: Adjust values - rounding error
	auto getAlt = [&](int col, int row) -> int
		{
			if (col < 0 || col >= gridW || row < 0 || row >= gridH)
				return 0.0f; // default alt for out-of-bounds
			return altGrid[row * gridW + col] / 16.0f;
		};

	for (const auto& tileDef : currentDef.tiles)
	{
		const float worldX = currentDef.originX + tileDef.col * currentDef.tileSize;
		const float worldZ = currentDef.originZ + tileDef.row * currentDef.tileSize;
		const float worldY = tileDef.height;

		//groups[tileDef.texturePath].push_back({ worldX, worldY, worldZ, tileDef.rotation, tileDef.flip, tileDef.alt });
		// Tile corner use Alt values from the 4 grid vertices surrounding the tile.
		// Tile at (col, row) has corner at grid points:
		// (col, row), (col+1, row)...
		// Vertex layout: [0]= (-X+Z), [1]= (+X,+Z), [2] = (+X,-Z), [3] = (-X,-Z)
		// Mapping: vertex0 = grid(col, row+1), vertex1 = grid(col+1, row+1), vertex2 = grid(col+1, row), vertex3 = grid(col, row)
		TileBatch::TileInstance instance;
		instance.worldX = worldX;
		instance.worldY = worldY;
		instance.worldZ = worldZ;
		instance.rotation = tileDef.rotation;
		instance.flip = tileDef.flip;
		instance.altCorners[0] = getAlt(tileDef.col, tileDef.row + 1); // vertex 0 = grid(col, row+1)
		instance.altCorners[1] = getAlt(tileDef.col + 1, tileDef.row + 1); // vertex 1 = grid(col+1, row+1)
		instance.altCorners[2] = getAlt(tileDef.col + 1, tileDef.row); // vertex 2 = grid(col+1, row)
		instance.altCorners[3] = getAlt(tileDef.col, tileDef.row); // vertex 3 = grid(col, row)

		groups[tileDef.texturePath].push_back(instance);
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