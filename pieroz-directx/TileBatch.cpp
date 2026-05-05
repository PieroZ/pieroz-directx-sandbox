#include "TileBatch.h"
#include "BindableCommon.h"
#include "Vertex.h"
#include "Channels.h"

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

		// Base UVs: top-left, top-right, bottom-right, bottom-left
		dx::XMFLOAT2 uvs[4] = {
			{ 0.0f, 0.0f},
			{ 1.0f, 0.0f},
			{ 1.0f, 1.0f},
			{ 0.0f, 1.0f}
		};

		// Apply flip first ( bit 0 = horizontal flip, bit 1 = vertical flip)
		if (flip & 0x1)
		{
			for (auto& uv : uvs) uv.x = 1.0f - uv.x;
		}
		if (flip & 0x2)
		{
			for (auto& uv : uvs) uv.y = 1.0f - uv.y;
		}

		// Apply rotation (rotate UV indices clock2wise by rot * 90)
		// Rotating UVs means shifting which UV goes to which vertex
		dx::XMFLOAT2 rotatedUvs[4];
		for (int v = 0; v < 4; v++)
		{
			rotatedUvs[v] = uvs[(v + rot + 1) % 4];
		}


		// 4 vertices per tile, pre-transformed to world space
		vbuf.EmplaceBack(dx::XMFLOAT3{ cx - half, cy, cz + half }, normal, rotatedUvs[0]);
		vbuf.EmplaceBack(dx::XMFLOAT3{ cx + half, cy, cz + half }, normal, rotatedUvs[1]);
		vbuf.EmplaceBack(dx::XMFLOAT3{ cx + half, cy, cz - half }, normal, rotatedUvs[2]);
		vbuf.EmplaceBack(dx::XMFLOAT3{ cx - half, cy, cz - half }, normal, rotatedUvs[3]);

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
		Technique unlit{ "Unlit", Chan::main, true };
		Step step("lambertian");

		auto pvs = VertexShader::Resolve(gfx, "Unlit_VS.cso");
		step.AddBindable(InputLayout::Resolve(gfx, vbuf.GetLayout(), *pvs));
		step.AddBindable(std::move(pvs));
		step.AddBindable(PixelShader::Resolve(gfx, "Unlit_PS.cso"));

		step.AddBindable(Bind::Texture::Resolve(gfx, texturePath, 0u));
		step.AddBindable(Sampler::Resolve(gfx));
		step.AddBindable(std::make_shared<TransformCbuf>(gfx));
		step.AddBindable(Rasterizer::Resolve(gfx, false)); // no backface culling

		unlit.AddStep(std::move(step));
		AddTechnique(std::move(unlit));
	}
}

dx::XMMATRIX TileBatch::GetTransformXM() const noexcept
{
	// Vertices are already in world space
	return dx::XMMatrixIdentity();
}