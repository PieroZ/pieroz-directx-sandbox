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

	// All tiles with the same size share one VB/IB (geometry is identical, position comes from transform)
	const std::string tag = "$tile_quad_" + std::to_string(size);

	pVertices = VertexBuffer::Resolve(gfx, tag, vbuf);
	pIndices = IndexBuffer::Resolve(gfx, tag, indices);
	pTopology = Topology::Resolve(gfx, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Unlit textured technique - render flat texture with no lighting
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

dx::XMMATRIX Tile::GetTransformXM() const noexcept
{
	return dx::XMMatrixTranslation(pos.x, pos.y, pos.z);
}