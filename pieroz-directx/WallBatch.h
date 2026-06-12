#pragma once

#include "Drawable.h"
#include "TileBatch.h"
#include "iam.h"
#include "tma.h"
#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <DirectXMath.h>

namespace Rgph { class RenderGraph;  }

struct WallPiece
{
	size_t facetIndex;
	int tileIndex;
};


// Batched drawable that renders wall geometry generated from DFacet data.
// Each DFacet produces a vertical quad(2 triangles) between its two base
// points, extending upward by Height.
class WallBatch : public Drawable
{
public:
	// Build wall geometry from range of DFacets.
	// gridScale - multiplier for grid-based x/z byte coordinates
	// yScale   - multiplier for Y (signed short) and Height values
	WallBatch(
		Graphics& gfx,
		const std::vector<DFacet>& facets,
		const std::vector<WallPiece>& wallPieces,
		const std::string& texturePath,
		float gridScale,
		float yScale
	);

	// Factory: groups all facets by texture and returns one WallBatch per texture.
	static std::vector<std::unique_ptr<WallBatch>> CreateBatches(
		Graphics& gfx,
		const std::vector<DFacet>& facets,
		const std::vector<signed short>& styles,
		const std::vector<DStorey>& storeys,
		const tma& tmaData,
		int worldNo,
		float gridScale = 1.0f,
		float yScale = 1.0f / 256.0f);


	DirectX::XMMATRIX GetTransformXM() const noexcept override;

	UINT GetWallCount() const noexcept { return wallCount; }

	// Ray-quad picking: returns measurement of the hit quad, if any.
	std::optional<std::pair<QuadMeasurement, float>> PickQuad(
		DirectX::FXMVECTOR rayOrigin, DirectX::FXMVECTOR rayDir) const;

private:
	UINT wallCount = 0;
	// CPU-side quad vertices for picking(4 per wall: bottom-left, bottom-right, top-left, top-right)
	std::vector<DirectX::XMFLOAT3> cpuQuadVertices;
};