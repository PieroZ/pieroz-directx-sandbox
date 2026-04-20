#pragma once
#include "Graphics.h"
#include "Drawable.h"
#include "ConditionalNoexcept.h"
#include <optional>

class Material;
class FrameCommander;
struct aiMesh;


class Mesh : public Drawable
{
public:
	Mesh( Graphics& gfx,const Material& mat,const aiMesh& mesh,float scale = 1.0f ) noxnd;
	DirectX::XMMATRIX GetTransformXM() const noexcept override;
	void Submit( size_t channels,DirectX::FXMMATRIX accumulatedTranform ) const noxnd;
	// Picking: test ray (in world space) against mesh triangles transformed by worldTransform
	std::optional<std::pair<size_t, float>> Intersect(
		DirectX::FXMVECTOR rayOrigin,
		DirectX::FXMVECTOR rayDir,
		DirectX::FXMMATRIX worldTransform) const noexcept;
private:
	mutable DirectX::XMFLOAT4X4 transform;
	std::vector<DirectX::XMFLOAT3> cpuPositions;
	std::vector<unsigned short> cpuIndices;
};