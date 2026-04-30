#pragma once
#include "primLoader.h"
#include "IndexedTriangleList.h"
#include "Vertex.h"
#include <DirectXMath.h>
#include <vector>

// Covert a loaded PrimLoadResult into IndexedTriangleList suitable for rendering.
// Uses PrimPoint data from the loaded result (int16 X,Y,Z converted to float).
// Quads (PrimFace4) are split into two triangles: (0,1,2) and (0,2,3).
// UV coordinates are converted from uint8 [0...255] to float [0..1].
// Optional scale factor converts int16 coordinates to world units.
IndexedTriangleList ConvertPrimToTriangleList(
	const PrimLoadResult& prim,
	float scale = 1.0f / 256.0f);