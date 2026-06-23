#pragma once
#include "Drawable.h"
#include "IndexedTriangleList.h"
#include <DirectXMath.h>
#include <string>
#include <vector>
#include <optional>

namespace Rgph { class RenderGraph; }

// A renderable object created from an IndexedTriangleList (e.g. from PrimConverter).
// Position ca be updated each from to follow mouse cursor or be placed anywhere.
class PrimDrawable : public Drawable
{
public:
	PrimDrawable(Graphics& gfx, IndexedTriangleList triList, const std::string& texturePath = "Images\\white.png", bool doubleSided = false);
	DirectX::XMMATRIX GetTransformXM() const noexcept override;
	void SetPosition(const DirectX::XMFLOAT3& pos) noexcept;
	void SetPosition(float x, float y, float z) noexcept;
	void SetYaw(float y) noexcept;
	DirectX::XMFLOAT3 GetPosition() const noexcept { return position; }
	float GetYaw() const noexcept { return yaw; }

	// Picking: ray-triangle intersection in world space
	std::optional<std::pair<size_t, float>> Intersect(
		DirectX::FXMVECTOR rayOrigin, DirectX::FXMVECTOR rayDir) const noexcept;

	const std::vector<DirectX::XMFLOAT3>& GetCpuPositions() const noexcept { return cpuPositions; };
	const std::vector< unsigned short>& GetCpuIndices() const noexcept { return cpuIndices; };

private:
	DirectX::XMFLOAT3 position = { 0.0f, 0.0f, 0.0f };
	float yaw = 0.0f;
	std::vector<DirectX::XMFLOAT3> cpuPositions; // for picking
	std::vector< unsigned short> cpuIndices; // for picking
};
