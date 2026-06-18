#include "WallBatch.h"
#include "BindableCommon.h"
#include "Vertex.h"
#include "Channels.h"
#include "TextureIdToFilenameHelper.h"
#include "DynamicConstant.h"
#include "ConstantBuffersEx.h"
#include <DirectXCollision.h>
#include <cmath>
#include <unordered_map>


namespace dx = DirectX;

WallBatch::WallBatch(Graphics& gfx,
	const std::vector<DFacet>& facets,
	const std::vector<WallPiece>& wallPieces,
	const std::string& texturePath,
	float gridScale,
	float yScale)
{
	using namespace Bind;

	Dvtx::VertexLayout layout;
	layout.Append(Dvtx::VertexLayout::Position3D);
	layout.Append(Dvtx::VertexLayout::Normal);
	layout.Append(Dvtx::VertexLayout::Texture2D);

	Dvtx::VertexBuffer vbuf(std::move(layout));
	std::vector<unsigned short> indices;

	for (const auto& piece : wallPieces)
	{
		const auto& f = facets[piece.facetIndex];
		const int tile = piece.tileIndex;

		// Convert grid-based byte coordinates to world space
		const float z0 = static_cast<float>(f.x[0] - 0.5f) * gridScale;
		const float z1 = static_cast<float>(f.x[1] - 0.5f) * gridScale;
		const float x0 = static_cast<float>(f.z[0] - 0.5f) * gridScale;
		const float x1 = static_cast<float>(f.z[1] - 0.5f) * gridScale;

		const float heightScale = f.BlockHeight / 4;

		// Bottom Y
		const float y0_bottom = static_cast<float>(f.Y[0]) * yScale * 1 / 32.0f;
		const float y1_bottom = static_cast<float>(f.Y[1]) * yScale * 1 / 32.0f;

		// Single tile height
		const float tileHeight = 4.0f * yScale * 1 / 2.0f * heightScale;

		// Current tile offsets
		const float tileBottomOffset = static_cast<float>(tile) * tileHeight;
		const float tileTopOffset = static_cast<float>(tile + 1) * tileHeight;

		const float ty0_bottom = y0_bottom + tileBottomOffset;
		const float ty1_bottom = y1_bottom + tileBottomOffset;
		const float ty0_top = y0_bottom + tileTopOffset;
		const float ty1_top = y1_bottom + tileTopOffset;

		// Compute wall normal
		const float dx_ = x1 - x0;
		const float dz_ = z1 - z0;

		const float len = std::sqrt(dx_ * dx_ + dz_ * dz_);

		dx::XMFLOAT3 normal{ 0.0f, 0.0f, 1.0f };

		if (len > 1e-6f)
		{
			normal = { -dz_ / len, 0.0f, dx_ / len };
		}

		// UV mapping
		const float uLen = len / gridScale;

		// Vertices
		dx::XMFLOAT3 wv0{ x0, ty0_bottom, z0 };
		dx::XMFLOAT3 wv1{ x1, ty1_bottom, z1 };
		dx::XMFLOAT3 wv2{ x1, ty1_top, z1 };
		dx::XMFLOAT3 wv3{ x0, ty0_top, z0 };



		/*vbuf.EmplaceBack(
			dx::XMFLOAT3{ x0, ty0_bottom, z0 },
			normal,
			dx::XMFLOAT2{ 0.0f, 1.0f });

		vbuf.EmplaceBack(
			dx::XMFLOAT3{ x1, ty1_bottom, z1 },
			normal,
			dx::XMFLOAT2{ uLen, 1.0f });

		vbuf.EmplaceBack(
			dx::XMFLOAT3{ x1, ty1_top, z1 },
			normal,
			dx::XMFLOAT2{ uLen, 0.0f });

		vbuf.EmplaceBack(
			dx::XMFLOAT3{ x0, ty0_top, z0 },
			normal,
			dx::XMFLOAT2{ 0.0f, 0.0f });*/

		vbuf.EmplaceBack(wv0, normal, dx::XMFLOAT2{ 0.0f, 1.0f });
		vbuf.EmplaceBack(wv1, normal, dx::XMFLOAT2{ uLen, 1.0f });
		vbuf.EmplaceBack(wv2, normal, dx::XMFLOAT2{ uLen, 0.0f });
		vbuf.EmplaceBack(wv3, normal, dx::XMFLOAT2{ 0.0f, 0.0f });

		// Store CPU-side quad vertices for picking
		cpuQuadVertices.push_back(wv0);
		cpuQuadVertices.push_back(wv1);
		cpuQuadVertices.push_back(wv2);
		cpuQuadVertices.push_back(wv3);
		

		const auto base = static_cast<unsigned short>(wallCount * 4);

		indices.push_back(base + 0);
		indices.push_back(base + 1);
		indices.push_back(base + 2);

		indices.push_back(base + 0);
		indices.push_back(base + 2);
		indices.push_back(base + 3);

		wallCount++;
	}

	if (wallCount == 0)
		return;

	static int uid = 0;
	const std::string tag = "$wallbatch_" + std::to_string(uid++);

	pVertices = std::make_shared<VertexBuffer>(gfx, tag, vbuf);
	pIndices = std::make_shared<IndexBuffer>(gfx, tag, indices);
	pTopology = Topology::Resolve(gfx, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	{
		Technique unlit{ "Unlit", Chan::main, true };
		Step step("lambertian");

		auto pvs = VertexShader::Resolve(gfx, "Unlit_VS.cso");

		step.AddBindable(InputLayout::Resolve(gfx, vbuf.GetLayout(), *pvs));
		step.AddBindable(std::move(pvs));

		step.AddBindable(PixelShader::Resolve(gfx, "Unlit_PS.cso"));

		step.AddBindable(Bind::Texture::Resolve(gfx, texturePath, 0u));
		step.AddBindable(Sampler::Resolve(gfx));

		step.AddBindable(std::make_shared<TransformCbuf>(gfx));

		step.AddBindable(
			Rasterizer::Resolve(gfx, true, false, 1000, 1.0f));

		unlit.AddStep(std::move(step));
		AddTechnique(std::move(unlit));
	}

	// Lit technique - reacts to the scene point light (register b0 in PS).
	// Inactive by default; toggled by the global render-mode switch.
	{
		Technique lit{"Lit", Chan::main,false };
		Step step("lambertian");

		auto pvs = VertexShader::Resolve(gfx, "ColorLit_VS.cso");
		step.AddBindable(InputLayout::Resolve(gfx, vbuf.GetLayout(), *pvs));
		step.AddBindable(std::move(pvs));
		step.AddBindable(PixelShader::Resolve(gfx, "ColorLit_PS.cso"));


		step.AddBindable(Bind::Texture::Resolve(gfx, texturePath, 0u));
		step.AddBindable(Sampler::Resolve(gfx));

		// Neutral material + white light tint so walls keep their texture and are lit by the actual scene light color/intensity
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

		step.AddBindable(std::make_shared<TransformCbuf>(gfx));
		step.AddBindable(Rasterizer::Resolve(gfx, false, false, 1000, 1.0f)); // no backface culling

		lit.AddStep(std::move(step));
		AddTechnique(std::move(lit));
	}
}

dx::XMMATRIX WallBatch::GetTransformXM() const noexcept
{
	// Vertices are already in the world space
	return dx::XMMatrixIdentity();
}

std::optional<std::pair<QuadMeasurement, float>> WallBatch::PickQuad(
	DirectX::FXMVECTOR rayOrigin, DirectX::FXMVECTOR rayDir) const
{
	float bestDist = FLT_MAX;
	int bestQuad = -1;

	for (UINT q = 0; q < wallCount; q++)
	{
		const auto& qv0 = cpuQuadVertices[q * 4 + 0];
		const auto& qv1 = cpuQuadVertices[q * 4 + 1];
		const auto& qv2 = cpuQuadVertices[q * 4 + 2];
		const auto& qv3 = cpuQuadVertices[q * 4 + 3];

		// Triangle 1: v0, v 1, v2
		float dist = 0.0f;
		if (dx::TriangleTests::Intersects(rayOrigin, rayDir, 
			dx::XMLoadFloat3(&qv0), dx::XMLoadFloat3(&qv1), dx::XMLoadFloat3(&qv2), dist))
		{
			if (dist < bestDist)
			{
				bestDist = dist;
				bestQuad = (int)q;
			}
		}
		// Triangle 2: v0, v2, v3
		if (dx::TriangleTests::Intersects(rayOrigin, rayDir,
			dx::XMLoadFloat3(&qv0), dx::XMLoadFloat3(&qv2), dx::XMLoadFloat3(&qv3), dist))
		{
			if (dist < bestDist)
			{
				bestDist = dist;
				bestQuad = (int)q;
			}
		}
	}

	if (bestQuad < 0)
		return std::nullopt;

	const auto& v0 = cpuQuadVertices[bestQuad * 4 + 0];
	const auto& v1 = cpuQuadVertices[bestQuad * 4 + 1];
	const auto& v2 = cpuQuadVertices[bestQuad * 4 + 2];
	const auto& v3 = cpuQuadVertices[bestQuad * 4 + 3];

	auto vecLen = [](const dx::XMFLOAT3& a, const dx::XMFLOAT3& b) -> float
		{
			const float dx = b.x - a.x;
			const float dy = b.y - a.y;
			const float dz = b.z - a.z;
			return std::sqrt(dx * dx + dy * dy + dz * dz);
		};

	QuadMeasurement m;
	m.v0 = v0; m.v1 = v1; m.v2 = v2; m.v3 = v3;
	m.width = vecLen(v0, v1);
	m.height = vecLen(v0, v3);
	m.diagonal0 = vecLen(v0, v2);
	m.diagonal1 = vecLen(v1, v3);

	return std::make_pair(m, bestDist);
}

ULONG facet_rand(void)
{
	static	ULONG	facet_seed = 0x12345678;
	facet_seed = (facet_seed * 69069) + 1;

	return(facet_seed >> 7);
}


std::vector<std::unique_ptr<WallBatch>> WallBatch::CreateBatches(
	Graphics& gfx,
	const std::vector<DFacet>& facets,
	const std::vector<signed short>& styles,
	const std::vector<DStorey>& storeys,
	const tma& tmaData,
	int worldNo,
	float gridScale,
	float yScale)
{
	std::unordered_map<std::string, std::vector<WallPiece>> textureGroups;

	for (size_t fi = 1; fi < facets.size(); fi++)
	{
		const auto& f = facets[fi];

		if (f.BlockHeight == 0)
			continue;

		/*if (f.FacetType != 1)
			continue;*/

		const int tileCount =
			std::max(1, static_cast<int>(f.Height) / 4);

		for (int z = 0; z < tileCount; z++)
		{
			int texture_piece = 0;

			std::string texturePath = "Images\\brickwall.jpg";

			size_t styleIndex = f.StyleIndex + z;

			if (styleIndex < styles.size() && styles[styleIndex])
			{
				int page = 0;
				if (styles[styleIndex] < 200 && styles[styleIndex] >= 0)

				{
					page =
						tmaData.dx_textures_xy[styles[styleIndex]]
						[texture_piece]
						.Page;
				}
				else if (styles[styleIndex] < 0)
				{
					try
					{
						DStorey storey = storeys.at(-styles[styleIndex]);

						signed long index = storey.Index;
						int pos = 0;
						if (pos == 0)
						{
							styleIndex = storey.Style;

							page =
								tmaData.dx_textures_xy.at(styles[styleIndex])
								[texture_piece]
								.Page;

						}
					}
					catch (...)
					{
						page = 0;
					}
				}

				auto paths = get_texture_paths(
					page,
					"UC-data/textures/world" + std::to_string(worldNo) + "/",
					"UC-data/textures/shared/",
					"UC-data/textures/inside/",
					"UC-data/textures/people/",
					"UC-data/textures/prims/",
					"UC-data/textures/people2/"
				);

				texturePath = paths.res64;
			}

			textureGroups[texturePath].push_back({
				fi,
				z
				});
		}
	}

	std::vector<std::unique_ptr<WallBatch>> batches;

	batches.reserve(textureGroups.size());

	for (auto& [texPath, pieces] : textureGroups)
	{
		auto batch = std::make_unique<WallBatch>(
			gfx,
			facets,
			pieces,
			texPath,
			gridScale,
			yScale
		);

		if (batch->GetWallCount() > 0)
		{
			batches.push_back(std::move(batch));
		}
	}

	return batches;
}