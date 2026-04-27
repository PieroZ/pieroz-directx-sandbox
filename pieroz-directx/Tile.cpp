#include "Tile.h"
#include "BindableCommon.h"
#include "Vertex.h"
#include "Channels.h"

namespace dx = DirectX;

Tile::Tile( Graphics& gfx, float size, float worldX, float worldY, float worldZ,
	const std::string& texturePath)
	:
	pos( worldX,worldY,worldZ )
{
	using namespace Bind;

	const float half = size / 2.0f;

	// Vertex layout: Position3D + Normal + Texture2D (matches Unlit_VS input)
	Dvtx::VertexLayout layout;
	layout.Append(Dvtx::VertexLayout::Position3D);
	layout.Append(Dvtx::VertexLayout::Normal);
	layout.Append(Dvtx::VertexLayout::Texture2D);

	Dvtx::VertexBuffer vbuf(std::move(layout));

	// Flat quad on XZ plane (Y=0), normal pointing up
	dx::XMFLOAT3 normal{ 0.0f,1.0f,0.0f };

	vbuf.EmplaceBack(dx::XMFLOAT3{ -half,0.0f, half }, normal, dx::XMFLOAT2{ 0.0f,0.0f });
	vbuf.EmplaceBack(dx::XMFLOAT3{  half,0.0f, half }, normal, dx::XMFLOAT2{ 1.0f,0.0f });
	vbuf.EmplaceBack(dx::XMFLOAT3{  half,0.0f,-half }, normal, dx::XMFLOAT2{ 1.0f,1.0f });
	vbuf.EmplaceBack(dx::XMFLOAT3{ -half,0.0f,-half }, normal, dx::XMFLOAT2{ 0.0f,1.0f });

	std::vector<unsigned short> indices = { 0,1,2, 0,2,3 };

	// Unique tag per tile positino so buffers aren't shared incorrectly
	static int uid = 0;
	const std::string tag = "$tile_" + std::to_string(uid++);

	pVertices = std::make_shared<VertexBuffer>(gfx, tag, vbuf);
	pIndices = std::make_shared<IndexBuffer>(gfx, tag, indices);
	pTopology = std::make_shared<Topology>(gfx, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Unlit textured technique - render flat texture with no lighting
	{
		Technique unlit{ "Unlit", Chan::main, true };
		Step step("lambertian");

		auto pvs = VertexShader::Resolve(gfx, "Unlit_VS.cso");
		step.AddBindable(InputLayout::Resolve(gfx, vbuf.GetLayout(), *pvs));
		step.AddBindable(std::move(pvs));
		step.AddBindable(PixelShader::Resolve(gfx, "Unlit_PS.cso"));

		step.AddBindable(std::make_shared<Bind::Texture>(gfx, texturePath, 0u));
		step.AddBindable(Sampler::Resolve(gfx));
		step.AddBindable(std::make_shared<TransformCbuf>(gfx));
		step.AddBindable(Rasterizer::Resolve(gfx, false)); // no backface culling

		unlit.AddStep(std::move(step));
		AddTechnique(std::move(unlit));

	}
}

dx::XMMATRIX Tile::GetTransformXM() const noexcept
{
	return dx::XMMatrixTranslation(pos.x, pos.y, pos.z);
}