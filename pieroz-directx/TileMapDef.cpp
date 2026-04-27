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
		tile.texturePath = tileJson.at("texture").get<std::string>();
		def.tiles.push_back(std::move(tile));
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