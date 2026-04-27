#include "TileMapScene.h"
#include "Channels.h"
#include <stdexcept>

namespace dx = DirectX;

TileMapScene::TileMapScene(Graphics& gfx, const TileMapDef& def)
	: currentDef(def)
{
	BuildTiles(gfx);
}

void TileMapScene::LoadMap(Graphics& gfx, const TileMapDef& def)
{
	currentDef = def;
	tiles.clear();
	BuildTiles(gfx);
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
	for (const auto& tile : tiles)
	{
		tile->LinkTechniques(rg);
	}
	if (dynamicModel)
	{
		dynamicModel->LinkTechniques(rg);
	}
}

void TileMapScene::Submit(size_t channels) const
{
	for (const auto& tile : tiles)
	{
		tile->Submit(channels);
	}
	if (dynamicModel)
	{
		dynamicModel->Submit(channels);
	}
}

void TileMapScene::BuildTiles(Graphics& gfx)
{
	tiles.reserve(currentDef.tiles.size());

	for (const auto& tileDef : currentDef.tiles)
	{
		const float worldX = currentDef.originX + tileDef.col * currentDef.tileSize;
		const float worldZ = currentDef.originZ + tileDef.row * currentDef.tileSize;
		const float worldY = tileDef.height;
		tiles.push_back(std::make_unique<Tile>(
			gfx, currentDef.tileSize, worldX, worldY, worldZ, tileDef.texturePath
		));
	}
}