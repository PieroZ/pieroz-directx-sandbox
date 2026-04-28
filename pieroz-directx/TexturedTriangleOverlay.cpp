#include "TexturedTriangleOverlay.h"
#include "BindableCommon.h"
#include "GraphicsThrowMacros.h"
#include "Vertex.h"
#include "Channels.h"

namespace dx = DirectX;

TexturedTriangleOverlay::TexturedTriangleOverlay(
	Graphics& gfx,
	const DirectX::XMFLOAT3& v0, const DirectX::XMFLOAT3& v1, const DirectX::XMFLOAT3& v2,
	const DirectX::XMFLOAT2& uv0, const DirectX::XMFLOAT2& uv1, const DirectX::XMFLOAT2& uv2,
	const std::string& texturePath
)
{
	using namespace Bind;

	Dvtx::VertexLayout layout;
	layout.Append(Dvtx::VertexLayout::Position3D);
	layout.Append(Dvtx::VertexLayout::Texture2D);
	Dvtx::VertexBuffer vertices(std::move(layout));

	vertices.EmplaceBack(v0, uv0);
	vertices.EmplaceBack(v1, uv1);
	vertices.EmplaceBack(v2, uv2);

	std::vector<unsigned short> indices = { 0, 1, 2 };

	static int uid = 0;
	const std::string tag = "$tex_tri_overlay_" + std::to_string(uid++);

	pVertices = std::make_shared<VertexBuffer>(gfx, tag, vertices);
	pIndices = std::make_shared<IndexBuffer>(gfx, indices);
	pTopology = std::make_shared<Topology>(gfx, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	{
		Technique fill{ "TexturedOverlay", Chan::main, true };
		Step step("lambertian");
		
		auto pvs = VertexShader::Resolve(gfx, "TexturedOverlay_VS.cso");
		step.AddBindable(InputLayout::Resolve(gfx, vertices.GetLayout(), *pvs));
		step.AddBindable(std::move(pvs));
		step.AddBindable(PixelShader::Resolve(gfx, "TexturedOverlay_PS.cso"));

		// Bind the diffuse texture
		step.AddBindable(std::make_shared<Bind::Texture>(gfx, texturePath, 0u));
		step.AddBindable(Sampler::Resolve(gfx));

		step.AddBindable(std::make_shared<TransformCbuf>(gfx));

		// Two-side solid fill with depth bias to prevent z-fighting with original geometry
		step.AddBindable(Rasterizer::Resolve(gfx, true, false, -100, -1.0f));
		step.AddBindable(Stencil::Resolve(gfx, Stencil::Mode::DepthFirst));
		// No blending - fully opaque textured overlay
		step.AddBindable(Blender::Resolve(gfx, false));

		fill.AddStep(std::move(step));
		AddTechnique(std::move(fill));
	}
}

dx::XMMATRIX TexturedTriangleOverlay::GetTransformXM() const noexcept
{
	return dx::XMMatrixIdentity();
}