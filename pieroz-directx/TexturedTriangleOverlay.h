#pragma once
#include "Drawable.h"
#include <string>

namespace Rgph
{
	class RenderGraph;
}

// Renders a single textured triangle at given world-space positions + UVs.
// Uses a depth-biased rasterizer so it overlays original geometry.
class TexturedTriangleOverlay : public Drawable
{
public:
	TexturedTriangleOverlay(
		Graphics& gfx,
		const DirectX::XMFLOAT3& v0, const DirectX::XMFLOAT3& v1, const DirectX::XMFLOAT3& v2,
		const DirectX::XMFLOAT2& uv0, const DirectX::XMFLOAT2& uv1, const DirectX::XMFLOAT2& uv2,
		const std::string& texturePath
	);
	DirectX::XMMATRIX GetTransformXM() const noexcept override;
};