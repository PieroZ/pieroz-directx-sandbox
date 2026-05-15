#include "WallBatch.h"
#include "BindableCommon.h"
#include "Vertex.h"
#include "Channels.h"
#include "TextureIdToFilenameHelper.h"
#include <cmath>

namespace dx = DirectX;

WallBatch::WallBatch(Graphics& gfx,
	const std::vector<DFacet>& facets,
	const std::vector<unsigned short>& styles,
	const tma& tmaData,
	int worldNo,
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

	// Skip index 0 ( null entry) - valid facets start at 1
	for (size_t fi = 1; fi < facets.size(); fi++)
	{
		std::string texturePath = "Images\\brickwall.jpg";

		const auto& f = facets[fi];

		// Skip facets with zero height (no visible wall)
		if (f.BlockHeight == 0)
			continue;

		// Skipp facets type different than STOREY_TYPE_NORMAL
		if (f.FacetType != 1)
			continue;

		if (styles[f.StyleIndex])
		{
			//auto texRes = get_texture_paths(styles[f.StyleIndex]);
			int page = tmaData.dx_textures_xy[0][styles[f.StyleIndex]].Page;

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

		// Convert grid-based byte coorindates to world space
		const float z0 = static_cast<float>(f.x[0] - 0.5f) * gridScale;
		const float z1 = static_cast<float>(f.x[1] - 0.5f) * gridScale;
		const float x0 = static_cast<float>(f.z[0] - 0.5f) * gridScale;
		const float x1 = static_cast<float>(f.z[1] - 0.5f) * gridScale;

		const float heightScale = f.Height / 4;

		// Y coordinates (signed short) scaled to world space
		const float y0_bottom = static_cast<float>(f.Y[0]) * yScale* 1/64.0f;// * heightScale;
		const float y1_bottom = static_cast<float>(f.Y[1]) * yScale* 1/64.0f;// * heightScale;

		// Top of the wall: base Y minus Height (Y decreases going up in this format)
		const float height = static_cast<float>(f.BlockHeight) * yScale * 1 / 2.0f * 0.5 * heightScale;// *heightScale;
		const float y0_top = y0_bottom + height;
		const float y1_top = y1_bottom + height;


		// Compute wall normal (outward-facing)
		// Wall direction vector along the base
		const float dx_ = x1 - x0;
		const float dz_ = z1 - z0;
		// Normal is perpendicular to wall direction in the XZ plane
		const float len = std::sqrt(dx_ * dx_ + dz_ * dz_);
		dx::XMFLOAT3 normal{ 0.0f, 0.0f, 1.0f };
		if (len > 1e-6f)
		{
			normal = { -dz_ / len, 0.0f, dx_ / len };
		}

		// UV mapping: U along the wall length, V along the height
		const float uLen = len / gridScale; // tile the texture every gridScale units

		// 4 vertices per wall quad
		// v0 = bottom-left, v1 =bottom-right, v2 = top-right, v3 = top-left
		vbuf.EmplaceBack(dx::XMFLOAT3{ x0, y0_bottom, z0 }, normal, dx::XMFLOAT2{ 0.0f, 1.0f });
		vbuf.EmplaceBack(dx::XMFLOAT3{ x1, y1_bottom, z1 }, normal, dx::XMFLOAT2{ uLen, 1.0f });
		vbuf.EmplaceBack(dx::XMFLOAT3{ x1, y1_top, z1 }, normal, dx::XMFLOAT2{ uLen, 0.0f });
		vbuf.EmplaceBack(dx::XMFLOAT3{ x0, y0_top, z0 }, normal, dx::XMFLOAT2{ 0.0f, 0.0f });

		const auto base = static_cast<unsigned short>(wallCount * 4);
		// Two triangles: ( 1,2,3) and (0,2,3)
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

	// Unlit textured technique (same as TileBatch)
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
		step.AddBindable(Rasterizer::Resolve(gfx, true, false, 1000, 1.0f)); // no backface culling - walls visible from both sides

		unlit.AddStep(std::move(step));
		AddTechnique(std::move(unlit));
	}
}

dx::XMMATRIX WallBatch::GetTransformXM() const noexcept
{
	// Vertices are already in the world space
	return dx::XMMatrixIdentity();
}