#include "TileMapDef.h"
#include "json.hpp"
#include <fstream>
#include <stdexcept>

using json = nlohmann::json;

TileMapDef TileMapDef::LoadFromJSON(const std::string& path)
{
	std::ifstream file(path);
	if (!file.is_open())
	{
		throw std::runtime_error("TileMapDef: cannot open file: " + path);
	}

	json j;
	file >> j;

	TileMapDef def;
	def.tileSize = j.value("tileSize", 1.0f);
	def.originX = j.value("originX", 0.0f);
	def.originZ = j.value("originZ", 0.0f);

	for (const auto& tileJson : j.at("tiles"))
	{
		TileDef tile;
		tile.col = tileJson.at("col").get<int>();
		tile.row = tileJson.at("row").get<int>();
		tile.height = tileJson.value("height", 0.0f);
		tile.alt = tileJson.value("alt", 0);
		tile.texturePath = tileJson.at("texture").get<std::string>();
		tile.rotation = tileJson.value("rotation", 0);
		tile.flip = tileJson.value("flip", 0);
		def.tiles.push_back(std::move(tile));
	}

	// Parse prim placements if present
	if (j.contains("prims"))
	{
		for (const auto& primJson : j.at("prims"))
		{
			PrimPlacementDef prim;
			prim.primIndex = primJson.at("prim").get<int>();
			prim.x = primJson.at("x").get<int>();
			prim.y = primJson.value("y", 0.0f);
			prim.z = primJson.at("z").get<int>();
			prim.xOffset = primJson.value("xOffset", 0.0f);
			prim.zOffset = primJson.value("zOffset", 0.0f);
			prim.yaw = primJson.value("yaw", 0);
			prim.flags = primJson.value("flags", 0);
			prim.insideIndex = primJson.value("InsideIndex", 0);
			def.prims.push_back(std::move(prim));
		}
	}

	return def;
}

TileMapDef TileMapDef::MakeGrid(int cols, int rows, float tileSize, const std::string& texture)
{
	TileMapDef def;
	def.tileSize = tileSize;
	def.originX = 0.0f;
	def.originZ = 0.0f;
	for (int r = 0; r < rows; r++)
	{
		for (int c = 0; c < cols; c++)
		{
			TileDef tile;
			tile.col = c;
			tile.row = r;
			tile.height = 0.0f;
			tile.texturePath = texture;
			def.tiles.push_back(std::move(tile));
		}
	}
	return def;
}