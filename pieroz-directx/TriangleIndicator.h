#pragma once
#include "Drawable.h"

namespace Rgph
{
	class RenderGraph;
}

class TriangleIndicator : public Drawable
{
public:
	// Builds a single-triangle overlay at given world-space positions
	TriangleIndicator(Graphics& gfx, const DirectX::XMFLOAT3& v0, const DirectX::XMFLOAT3& v1, const DirectX::XMFLOAT3& v2);
	DirectX::XMMATRIX GetTransformXM() const noexcept;
};