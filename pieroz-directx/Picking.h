#pragma once
#include <DirectXMath.h>
#include <optional>

class Mesh;

struct PickResult
{
	Mesh* pMesh = nullptr;
	size_t faceIndex = 0;
	float distance = 0.0f;
};

namespace Picking
{
	inline std::pair<DirectX::XMVECTOR, DirectX::XMVECTOR> ScreenToRay(
		int mouseX, int mouseY,
		int viewportWidth, int viewportHeight,
		DirectX::FXMMATRIX projection,
		DirectX::FXMMATRIX view
	)
	{
		namespace dx = DirectX;

		const dx::XMVECTOR nearPoint = dx::XMVector3Unproject(
			dx::XMVectorSet((float)mouseX, (float)mouseY, 0.0f, 0.0f),
			0.0f, 0.0f,
			(float)viewportWidth, (float)viewportHeight,
			0.0f, 1.0f,
			projection,
			view,
			dx::XMMatrixIdentity()
		);

		const dx::XMVECTOR farPoint = dx::XMVector3Unproject(
			dx::XMVectorSet((float)mouseX, (float)mouseY, 1.0f, 0.0f),
			0.0f, 0.0f,
			(float)viewportWidth, (float)viewportHeight,
			0.0f, 1.0f,
			projection,
			view,
			dx::XMMatrixIdentity()
		);

		const dx::XMVECTOR direction = dx::XMVector3Normalize(dx::XMVectorSubtract(farPoint, nearPoint));

		return { nearPoint, direction };
	}

	// Moller-Trumbore ray-triangle intersection algorithm
	inline std::optional<float> RayTriangleIntersect(
		const DirectX::XMVECTOR& rayOrigin,
		const DirectX::XMVECTOR& rayDir,
		const DirectX::XMVECTOR& v0,
		const DirectX::XMVECTOR& v1,
		const DirectX::XMVECTOR& v2
	)
	{
		namespace dx = DirectX;

		constexpr float EPSILON = 1e-6f;

		const dx::XMVECTOR edge1 = dx::XMVectorSubtract(v1, v0);
		const dx::XMVECTOR edge2 = dx::XMVectorSubtract(v2, v0);
		const dx::XMVECTOR h = dx::XMVector3Cross(rayDir, edge2);
		const float a = dx::XMVectorGetX(dx::XMVector3Dot(edge1, h));

		//if (fabs(a) < EPSILON)
		if (a > -EPSILON && a < EPSILON)
			return std::nullopt; // Ray is parallel to triangle

		const float f = 1.0f / a;
		const dx::XMVECTOR s = dx::XMVectorSubtract(rayOrigin, v0);
		const float u = f * dx::XMVectorGetX(dx::XMVector3Dot(s, h));

		if (u < 0.0f || u > 1.0f)
			return std::nullopt;

		const dx::XMVECTOR q = dx::XMVector3Cross(s, edge1);
		const float v = f * dx::XMVectorGetX(dx::XMVector3Dot(rayDir, q));

		if (v < 0.0f || u + v > 1.0f)
			return std::nullopt;

		const float t = f * dx::XMVectorGetX(dx::XMVector3Dot(edge2, q));
		if (t > EPSILON) // Intersection
			return t;

		return std::nullopt; // No intersection
	}
}