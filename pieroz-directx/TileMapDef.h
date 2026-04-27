#pragma once
#include <string>
#include <vector>

// Definition of a single tile in the tile map.
// Extensible: height can be varied per-tile in the future
struct TileDef
{
	int col = 0;				// grid column (X)
	int row = 0;				// grid row (Z)
	float height = 0.0f;		// Y offset (all same for now, extensible)
	std::string texturePath;	// path to tile texture
};

// Definition of the entire tile map.
struct TileMapDef
{
	float tileSize = 1.0f; 			// world-space size of each tile (square)
	float originX = 0.0f;			// world-space X of tile (0,0) center
	float originZ = 0.0f;			// world-space Z of tile (0,0) center
	std::vector<TileDef> tiles;

	// Load from JSON file. Format:
	/*{
		"tileSize": 2.0,
			"originX" : 0.0,
			"originZ" : 0.0,
			"tiles" : [
		{ "col":  0, "row" : 0, "height" : 0.0, "texture" : "Images\\brickwall.jpg" },
		{ "col":  1, "row" : 0, "height" : 0.0, "texture" : "Images\\brickwall.jpg" },
			]
	}*/
	static TileMapDef LoadFromJSON(const std::string& path);

	// Build a simple rectangular grid programmatically (all same texture)
	static TileMapDef MakeGrid(int cols, int rows, float tileSize, const std::string& texture);
};