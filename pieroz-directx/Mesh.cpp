#include "Mesh.h"
#include "Material.h"
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
#include "VertexBuffer.h"
#include "Texture.h"
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
	// Store CPU-side UVs for UV editor
	if (mesh.mTextureCoords[0]) // Check if UVs are present
	{
		cpuUVs.reserve(mesh.mNumVertices);
		for (unsigned int i = 0; i < mesh.mNumVertices; i++)
		{
			const auto& uv = mesh.mTextureCoords[0][i];
			cpuUVs.emplace_back(uv.x, uv.y);
		}
	}
	// Store CPU-side normals for export
	if(mesh.mNormals)
	{
		cpuNormals.reserve(mesh.mNumVertices);
		for (unsigned int i = 0; i < mesh.mNumVertices; i++)
		{
			const auto& n = mesh.mNormals[i];
			cpuNormals.emplace_back(n.x, n.y, n.z);
		}
	}

	// Store the full CPU-side vertex data for GPU updates
	{
		auto vtc = mat.ExtractVertices(mesh);
		if (scale != 1.0f)
		{
			for (auto i = 0u; i < vtc.Size(); i++)
			{
				dx::XMFLOAT3& pos = vtc[i].Attr<Dvtx::VertexLayout::ElementType::Position3D>();
				pos.x *= scale;
				pos.y *= scale;
				pos.z *= scale;
			}
		}
		cpuVertexData.emplace(std::move(vtc));
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

void Mesh::SetCpuUV(size_t vertexIndex, const DirectX::XMFLOAT2& uv) noexcept
{
	if (vertexIndex < cpuUVs.size())
	{
		cpuUVs[vertexIndex] = uv;
		// Also update tehe CPU vertex data copy
		if (cpuVertexData && cpuVertexData->GetLayout().Has(Dvtx::VertexLayout::ElementType::Texture2D))
		{
			(*cpuVertexData)[(int)vertexIndex].Attr<Dvtx::VertexLayout::Texture2D>() = uv;
		}
		gpuDirty = true;
	}
}
void Mesh::UpdateGpuVertexBuffer(Graphics& gfx)
{
	if (gpuDirty && cpuVertexData && pVertices)
	{
		pVertices->Update(gfx, *cpuVertexData);
		gpuDirty = false;
	}
}

void Mesh::SetFaceTextureOverride(size_t faceIndex, const std::string& texturePath)
{
	faceTextureOverrides[faceIndex] = texturePath;
}

void Mesh::ClearFaceTextureOverride(size_t faceIndex)
{
	faceTextureOverrides.erase(faceIndex);
}

bool Mesh::HasFaceTextureOverride(size_t faceIndex) const noexcept
{
	return faceTextureOverrides.count(faceIndex) > 0;
}

std::string Mesh::GetDefaultDiffuseTexturePath() const
{
	for (const auto& tech : techniques)
	{
		if(tech.GetName() != "Phong")
			continue;
		for (const auto& step : tech.GetSteps())
		{
			for (const auto& bindable : step.GetBindables())
			{
				if (auto* pTex = dynamic_cast<Bind::Texture*>(bindable.get()))
				{
					if (pTex->GetSlot() == 0) // diffuse
						return pTex->GetPath();
				}
			}
		}
	}
}