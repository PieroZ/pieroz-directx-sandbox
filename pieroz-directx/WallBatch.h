#pragma once

#include "Drawable.h"
#include "iam.h"
#include "tma.h"
#include <string>
#include <vector>
#include <DirectXMath.h>

namespace Rgph { class RenderGraph;  }

// Batched drawable that renders wall geometry generated from DFacet data.
// Each DFacet produces a vertical quad(2 triangles) between its two base
// points, extending upward by Height.
class WallBatch : public Drawable
{
public:
	// Build wall geometry from range of DFacets.
	// gridScale - multiplier for grid-based x/z byte coordinates
	// yScale   - multiplier for Y (signed short) and Height values
	WallBatch(Graphics& gfx,
		const std::vector<DFacet>& facets,
		const std::vector<unsigned short>& styles,
		const tma& tmaData,
		int worldNo,
		float gridScale = 1.0f,
		float yScale = 1.0f / 256.0f);

	DirectX::XMMATRIX GetTransformXM() const noexcept override;

	UINT GetWallCount() const noexcept { return wallCount; }

private:
	UINT wallCount = 0;
};