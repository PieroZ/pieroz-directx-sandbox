#include "TileBatch.h"
#include "BindableCommon.h"
#include "Vertex.h"
#include "Channels.h"
#include "DynamicConstant.h"
#include "ConstantBuffersEx.h"
#include <DirectXCollision.h>
#include <cmath>

namespace dx = DirectX;

TileBatch::TileBatch(Graphics& gfx, float tileSize, const std::string& texturePath,
	const std::vector<TileInstance>& instances)
	:
	tileCount(static_cast<UINT>(instances.size()))
{
	using namespace Bind;

	const float half = tileSize / 2.0f;
	dx::XMFLOAT3 normal{ 0.0f, 1.0f, 0.0f };

	Dvtx::VertexLayout layout;
	layout.Append(Dvtx::VertexLayout::Position3D);
	layout.Append(Dvtx::VertexLayout::Normal);
	layout.Append(Dvtx::VertexLayout::Texture2D);

	Dvtx::VertexBuffer vbuf(std::move(layout));

	// Use 32-bit indices sinces tile count * 4 can exceed 65535
	std::vector<unsigned short> indices;
	indices.reserve(instances.size() * 6);

	for (size_t i = 0; i < instances.size(); i++)
	{
		const float cx = instances[i].worldX;
		const float cy = instances[i].worldY;
		const float cz = instances[i].worldZ;
		const int rot = instances[i].rotation;
		const int flip = instances[i].flip;
		//const float altOffset = instances[i].alt / 32.0f;

		// Base UVs: top-left, top-right, bottom-right, bottom-left
		dx::XMFLOAT2 uvs[4] = {
			{ 0.0f, 0.0f},
			{ 1.0f, 0.0f},
			{ 1.0f, 1.0f},
			{ 0.0f, 1.0f}
		};

		// Apply flip first ( bit 0 = horizontal flip, bit 1 = vertical flip)
		//if (flip & 0x1)
		//{
		//	for (auto& uv : uvs) uv.x = 1.0f - uv.x;
		//}
		//if (flip & 0x2)
		//{
		//	for (auto& uv : uvs) uv.y = 1.0f - uv.y;
		//}

		// Apply rotation (rotate UV indices clockwise by rot * 90)
		// Rotating UVs means shifting which UV goes to which vertex
		dx::XMFLOAT2 rotatedUvs[4];
		for (int v = 0; v < 4; v++)
		{
			rotatedUvs[v] = uvs[(v + rot + 1) % 4];
		}

		//// Vertex positions with altitude slope
		//// Vertices: 0=(-half, +half) 1 = +half,+half)...
		//// Alt creates a slope: +Z side is raised by altOffset (ramp going from-Z to +Z)
		//// The slope direction rotates wit hthe tile rotation
		//float heights[4]; // per-verrtex Y offsets from alt
		//if (altOffset != 0.0f)
		//{
		//	// Base slope goes from low (-Z) to high (+Z)
		//	switch ((rot+1)%4)
		//	{
		//	case 0: // no rotation, slope along Z
		//		heights[0] = altOffset; // top-left (-Z)
		//		heights[1] = altOffset; // top-right (-Z)
		//		heights[2] = 0.0f; // bottom-right (+Z)
		//		heights[3] = 0.0f; // bottom-left (+Z)
		//		break;
		//	case 1:
		//		heights[0] = 0.0f; // top-left (-X)
		//		heights[1] = altOffset; // top-right (+X)
		//		heights[2] = altOffset; // bottom-right (+X)
		//		heights[3] = 0.0f; // bottom-left (-X)
		//		break;
		//	case 2:
		//		heights[0] = 0.0f; // top-left (+Z)
		//		heights[1] = 0.0f; // top-right (+Z)
		//		heights[2] = altOffset; // bottom-right (-Z)
		//		heights[3] = altOffset; // bottom-left (-Z)
		//		break;
		//	case 3:
		//		heights[0] = altOffset; // top-left (+X)
		//		heights[1] = 0.0f; // top-right (-X)
		//		heights[2] = 0.0f; // bottom-right (-X)
		//		heights[3] = altOffset; // bottom-left (+X)
		//		break;
		//	default:
		//		heights[0] = 0.0f;
		//		heights[1] = 0.0f;
		//		heights[2] = 0.0f;
		//		heights[3] = 0.0f;
		//	}
		//}
		//else
		//{
		//	heights[0] = 0.0f;
		//	heights[1] = 0.0f;
		//	heights[2] = 0.0f;
		//	heights[3] = 0.0f;
		//}

		//// Compute face normal from the (potentially sloped) quad
		//dx::XMFLOAT3 v0{ cx - half, cy + heights[0], cz + half };
		//dx::XMFLOAT3 v1{ cx + half, cy + heights[1], cz + half };
		//dx::XMFLOAT3 v2{ cx + half, cy + heights[2], cz - half };
		//dx::XMFLOAT3 v3{ cx - half, cy + heights[3], cz - half };

		dx::XMFLOAT3 v0{ cx - half, cy + instances[i].altCorners[0], cz + half };
		dx::XMFLOAT3 v1{ cx + half, cy + instances[i].altCorners[1], cz + half };
		dx::XMFLOAT3 v2{ cx + half, cy + instances[i].altCorners[2], cz - half };
		dx::XMFLOAT3 v3{ cx - half, cy + instances[i].altCorners[3], cz - half };

		// Store CPU quad vertices for picking
		cpuQuadVertices.push_back(v0);
		cpuQuadVertices.push_back(v1);
		cpuQuadVertices.push_back(v2);
		cpuQuadVertices.push_back(v3);


		// normalize((v1-v0) x (v3-v0))
		dx::XMVECTOR edge1 = dx::XMVectorSubtract(dx::XMLoadFloat3(&v1), dx::XMLoadFloat3(&v0));
		dx::XMVECTOR edge2 = dx::XMVectorSubtract(dx::XMLoadFloat3(&v3), dx::XMLoadFloat3(&v0));
		dx::XMVECTOR faceNormal = dx::XMVector3Normalize(dx::XMVector3Cross(edge1, edge2));
		dx::XMFLOAT3 n;
		dx::XMStoreFloat3(&n, faceNormal);

		// 4 vertices per tile, pre-transformed to world space
		vbuf.EmplaceBack(v0, n, rotatedUvs[0]);
		vbuf.EmplaceBack(v1, n, rotatedUvs[1]);
		vbuf.EmplaceBack(v2, n, rotatedUvs[2]);
		vbuf.EmplaceBack(v3, n, rotatedUvs[3]);

		const auto base = static_cast<unsigned short>(i * 4);
		indices.push_back(base + 0);
		indices.push_back(base + 1);
		indices.push_back(base + 2);
		indices.push_back(base + 0);
		indices.push_back(base + 2);
		indices.push_back(base + 3);
	}

	// Unique tag per texture batch
	const std::string tag = "$tilebatch_" + texturePath;

	pVertices = std::make_shared<VertexBuffer>(gfx, tag, vbuf);
	pIndices = std::make_shared<IndexBuffer>(gfx, tag, indices);
	pTopology = Topology::Resolve(gfx, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Unlit textured technique 
	{
		Technique unlit{ "Unlit", Chan::main, false };
		Step step("lambertian");

		auto pvs = VertexShader::Resolve(gfx, "Unlit_VS.cso");
		step.AddBindable(InputLayout::Resolve(gfx, vbuf.GetLayout(), *pvs));
		step.AddBindable(std::move(pvs));
		step.AddBindable(PixelShader::Resolve(gfx, "Unlit_PS.cso"));

		step.AddBindable(Bind::Texture::Resolve(gfx, texturePath, 0u));
		step.AddBindable(Sampler::Resolve(gfx));
		step.AddBindable(std::make_shared<TransformCbuf>(gfx));
		step.AddBindable(Rasterizer::Resolve(gfx, false, false, 1000, 1.0f));

		unlit.AddStep(std::move(step));
		AddTechnique(std::move(unlit));

	}

	// Lit textured technique
	{
		//Technique unlit{ "Unlit", Chan::main, true };
		Technique lit{ "Lit", Chan::main, true };
		Step step("lambertian");

		//auto pvs = VertexShader::Resolve(gfx, "Unlit_VS.cso");
		auto pvs = VertexShader::Resolve(gfx, "ColorLit_VS.cso");
		step.AddBindable(InputLayout::Resolve(gfx, vbuf.GetLayout(), *pvs));
		step.AddBindable(std::move(pvs));
		//step.AddBindable(PixelShader::Resolve(gfx, "Unlit_PS.cso"));
		step.AddBindable(PixelShader::Resolve(gfx, "ColorLit_PS.cso"));

		// Neutral material + white light tint so the tile show its texture and is lit by the actual scene light color/intensity.
		{
			Dcb::RawLayout lay;
			lay.Add<Dcb::Float3>("materialColor");
			lay.Add<Dcb::Float>("padding0");
			lay.Add<Dcb::Float3>("lightTint");
			lay.Add<Dcb::Float>("lightIntensity");
			Dcb::Buffer buf{ std::move(lay) };
			buf["materialColor"] = dx::XMFLOAT3{ 1.0f,1.0f,1.0f };
			buf["padding0"] = 0.0f;
			buf["lightTint"] = dx::XMFLOAT3{ 1.0f,1.0f,1.0f };
			buf["lightIntensity"] = 1.0f;
			step.AddBindable(std::make_shared<Bind::CachingPixelConstantBufferEx>(gfx, std::move(buf), 1u));

		}

		step.AddBindable(Bind::Texture::Resolve(gfx, texturePath, 0u));
		step.AddBindable(Sampler::Resolve(gfx));
		step.AddBindable(std::make_shared<TransformCbuf>(gfx));
		step.AddBindable(Rasterizer::Resolve(gfx, false, false, 1000, 1.0f)); // no backface culling

		/*unlit.AddStep(std::move(step));
		AddTechnique(std::move(unlit));*/

		lit.AddStep(std::move(step));
		AddTechnique(std::move(lit));
	}
}

dx::XMMATRIX TileBatch::GetTransformXM() const noexcept
{
	// Vertices are already in world space
	return dx::XMMatrixIdentity();
}

std::optional<std::pair<QuadMeasurement, float>> TileBatch::PickQuad(
	DirectX::FXMVECTOR rayOrigin, DirectX::FXMVECTOR rayDir) const
{
	float bestDist = FLT_MAX;
	int bestQuad = -1;

	for (UINT q = 0; q < tileCount; q++)
	{
		const auto& qv0 = cpuQuadVertices[q * 4 + 0];
		const auto& qv1 = cpuQuadVertices[q * 4 + 1];
		const auto& qv2 = cpuQuadVertices[q * 4 + 2];
		const auto& qv3 = cpuQuadVertices[q * 4 + 3];

		// Triangle 1: v0, v1, v2
		float dist = 0.0f;
		if(dx::TriangleTests::Intersects(rayOrigin, rayDir,
			dx::XMLoadFloat3(&qv0), dx::XMLoadFloat3(&qv1), dx::XMLoadFloat3(&qv2), dist))
		{
			if (dist < bestDist)
			{
				bestDist = dist;
				bestQuad = static_cast<int>(q);
			}
		}

		// Triangle 2: v0, v2, v3
		if (dx::TriangleTests::Intersects(rayOrigin, rayDir,
			dx::XMLoadFloat3(&qv0), dx::XMLoadFloat3(&qv2), dx::XMLoadFloat3(&qv3), dist))
		{
			if (dist < bestDist)
			{
				bestDist = dist;
				bestQuad = static_cast<int>(q);
			}
		}
	}
	if (bestQuad < 0)
		return std::nullopt;

	const  auto& v0 = cpuQuadVertices[bestQuad * 4 + 0];
	const  auto& v1 = cpuQuadVertices[bestQuad * 4 + 1];
	const  auto& v2 = cpuQuadVertices[bestQuad * 4 + 2];
	const  auto& v3 = cpuQuadVertices[bestQuad * 4 + 3];

	auto vecLen = [](const dx::XMFLOAT3& a, const dx::XMFLOAT3& b) -> float
		{
			float dx = b.x - a.x, dy = b.y - a.y, dz = b.z - a.z;
			return sqrtf(dx * dx + dy * dy + dz * dz);
		};

	QuadMeasurement m;
	m.v0 = v0; m.v1 = v1; m.v2 = v2; m.v3 = v3;
	m.width = vecLen(v0, v1);
	m.height = vecLen(v0, v3);
	m.diagonal0 = vecLen(v0, v2);
	m.diagonal1 = vecLen(v1, v3);

	return std::make_pair(m, bestDist);
}