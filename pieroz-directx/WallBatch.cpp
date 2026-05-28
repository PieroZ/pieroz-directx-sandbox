#include "WallBatch.h"
#include "BindableCommon.h"
#include "Vertex.h"
#include "Channels.h"
#include "TextureIdToFilenameHelper.h"
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
		vbuf.EmplaceBack(
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
			dx::XMFLOAT2{ 0.0f, 0.0f });

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
}

dx::XMMATRIX WallBatch::GetTransformXM() const noexcept
{
	// Vertices are already in the world space
	return dx::XMMatrixIdentity();
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

		if (f.FacetType != 1)
			continue;

		const int tileCount =
			std::max(1, static_cast<int>(f.Height) / 4);

		for (int z = 0; z < tileCount; z++)
		{
			int texture_piece = 0;

			std::string texturePath = "Images\\brickwall.jpg";

			const size_t styleIndex = f.StyleIndex + z;

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