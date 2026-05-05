#pragma once
#include "primLoader.h"
#include "IndexedTriangleList.h"
#include "Vertex.h"
#include <DirectXMath.h>
#include <vector>
#include <map>
#include <string>

// Covert a loaded PrimLoadResult into IndexedTriangleList grouped by texture.
// Each face's texture is determined from its TexturePage and UV coordinates.
// UVs are remapped to tile-local coordinates [0..1] within each 32x32 texture tile.
// Returns a map of textureImgNo -> IndexedTriangeList
std::map<int, IndexedTriangleList> ConvertPrimToTexturedTriangleList(
	const PrimLoadResult& prim,
	float scale = 1.0f / 256.0f);

// Build the texture file path for a given texture image number.
// Currently hardcoded to Urban Chaos prim textures location.
std::string GetPrimTexturePath(int textureImgNo);