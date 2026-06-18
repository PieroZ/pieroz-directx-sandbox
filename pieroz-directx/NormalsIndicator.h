#pragma once
#include "Drawable.h"
#include <DirectXMath.h>
#include <vector>

namespace Rgph
{
	class RenderGraph;
}

class NormalsIndicator : public Drawable
{
public:
	// Each segment is a world-space pair: {faceCenter, faceCenter + normal*length}.
	NormalsIndicator(
		Graphics& gfx,
		const std::vector<std::pair<DirectX::XMFLOAT3, DirectX::XMFLOAT3>>& segments,
		const DirectX::XMFLOAT3& color = { 0.1f, 1.0f, 0.2f });
	DirectX::XMMATRIX GetTransformXM() const noexcept override;
};