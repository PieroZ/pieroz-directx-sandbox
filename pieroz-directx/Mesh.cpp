#include "Mesh.h"
#include "imgui/imgui.h"
#include "Surface.h"
#include <unordered_map>
#include <sstream>
#include <filesystem>
#include "ChiliXM.h"
#include "DynamicConstant.h"
#include "ConstantBuffersEx.h"
#include "LayoutCodex.h"
#include "Stencil.h"
#include "Picking.h"
#include <assimp/scene.h>

namespace dx = DirectX;


// Mesh
Mesh::Mesh( Graphics& gfx,const Material& mat,const aiMesh& mesh,float scale ) noxnd
	:
	Drawable( gfx,mat,mesh,scale )
{
	// Store CPU-side positions for ray-casting/picking
	cpuPositions.reserve(mesh.mNumVertices);
	for (unsigned int i = 0; i < mesh.mNumVertices; i++)
	{
		const auto& v = mesh.mVertices[i];
		cpuPositions.emplace_back(v.x * scale, v.y * scale, v.z * scale);
	}
	// Store CPU-side indices for ray-casting/picking
	cpuIndices.reserve(mesh.mNumFaces * 3); // Assuming triangulated mesh
	for (unsigned int i = 0; i < mesh.mNumFaces; i++)
	{
		const auto& face = mesh.mFaces[i];
		assert(face.mNumIndices == 3); // Should be triangulated
		cpuIndices.push_back(static_cast<unsigned short>(face.mIndices[0]));
		cpuIndices.push_back(static_cast<unsigned short>(face.mIndices[1]));
		cpuIndices.push_back(static_cast<unsigned short>(face.mIndices[2]));
	}
}

void Mesh::Submit( size_t channels,dx::FXMMATRIX accumulatedTranform ) const noxnd
{
	dx::XMStoreFloat4x4( &transform,accumulatedTranform );
	Drawable::Submit( channels );
}

DirectX::XMMATRIX Mesh::GetTransformXM() const noexcept
{
	return DirectX::XMLoadFloat4x4( &transform );
}

std::optional<std::pair<size_t, float>> Mesh::Intersect(
	dx::FXMVECTOR rayOrigin,
	dx::FXMVECTOR rayDir,
	dx::FXMMATRIX worldTransform) const noexcept
{
	// Transform ray to local space by inverting the world transform
	const dx::XMMATRIX invWorld = dx::XMMatrixInverse(nullptr, worldTransform);
	const dx::XMVECTOR localOrigin = dx::XMVector3TransformCoord(rayOrigin, invWorld);
	const dx::XMVECTOR localDir = dx::XMVector3Normalize(dx::XMVector3TransformNormal(rayDir, invWorld));

	float closestDir = FLT_MAX;
	size_t closestFace = 0;
	bool hit = false;

	const size_t numTriangles = cpuIndices.size() / 3;
	for (size_t i = 0; i < numTriangles; i++)
	{
		const size_t idx0 = cpuIndices[i * 3 + 0];
		const size_t idx1 = cpuIndices[i * 3 + 1];
		const size_t idx2 = cpuIndices[i * 3 + 2];
		const dx::XMVECTOR v0 = dx::XMLoadFloat3(&cpuPositions[idx0]);
		const dx::XMVECTOR v1 = dx::XMLoadFloat3(&cpuPositions[idx1]);
		const dx::XMVECTOR v2 = dx::XMLoadFloat3(&cpuPositions[idx2]);
		if (const auto t = Picking::RayTriangleIntersect(localOrigin, localDir, v0, v1, v2))
		{
			if (*t < closestDir)
			{
				closestDir = *t;
				closestFace = i;
				hit = true;
			}
		}
	}

	if (hit)
	{
		return std::make_pair(closestFace, closestDir);
	}
	else
	{
		return std::nullopt;
	}
}