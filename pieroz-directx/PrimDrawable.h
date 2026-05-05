#pragma once
#include "Drawable.h"
#include "IndexedTriangleList.h"
#include <DirectXMath.h>
#include <string>

namespace Rgph { class RenderGraph; }

// A renderable object created from an IndexedTriangleList (e.g. from PrimConverter).
// Position ca be updated each from to follow mouse cursor or be placed anywhere.
class PrimDrawable : public Drawable
{
public:
	PrimDrawable(Graphics& gfx, IndexedTriangleList triList, const std::string& texturePath = "Images\\white.png");
	DirectX::XMMATRIX GetTransformXM() const noexcept override;
	void SetPosition(const DirectX::XMFLOAT3& pos) noexcept;
	void SetPosition(float x, float y, float z) noexcept;
	DirectX::XMFLOAT3 GetPosition() const noexcept { return position; }
private:
	DirectX::XMFLOAT3 position = { 0.0f, 0.0f, 0.0f };
};
