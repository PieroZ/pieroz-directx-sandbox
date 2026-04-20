#include "TriangleIndicator.h"
#include "BindableCommon.h"
#include "GraphicsThrowMacros.h"
#include "Vertex.h"
#include "Channels.h"
#include "DynamicConstant.h"
#include "ConstantBuffersEx.h"

namespace dx = DirectX;

TriangleIndicator::TriangleIndicator(Graphics& gfx, const dx::XMFLOAT3& v0, const dx::XMFLOAT3& v1, const dx::XMFLOAT3& v2)
{
	using namespace Bind;

	Dvtx::VertexLayout layout;
	layout.Append(Dvtx::VertexLayout::Position3D);
	Dvtx::VertexBuffer vertices(std::move(layout));

	vertices.EmplaceBack(v0);
	vertices.EmplaceBack(v1);
	vertices.EmplaceBack(v2);

	std::vector<unsigned short> indices = { 0, 1, 2 };

	// Use unique tags so these are never shared via codex
	static int uid = 0;
	const std::string tag = "$tri_indicator_" + std::to_string(uid++);

	pVertices = std::make_shared<VertexBuffer>(gfx, tag, vertices);
	pIndices = std::make_shared<IndexBuffer>(gfx, tag, indices);
	pTopology = std::make_shared<Topology>(gfx, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Solid filled triangle rendered on the lambertian pass
	{
		Technique fill{ "TriHighlight", Chan::main, true };
		Step step("lambertian");

		auto pvs = VertexShader::Resolve(gfx, "Solid_VS.cso");
		step.AddBindable(InputLayout::Resolve(gfx, vertices.GetLayout(), *pvs));
		step.AddBindable(std::move(pvs));
		step.AddBindable(PixelShader::Resolve(gfx, "Solid_PS.cso"));

		// Semi-bright highlight color
		{
			Dcb::RawLayout lay;
			lay.Add<Dcb::Float3>("materialColor");
			Dcb::Buffer buf{ std::move(lay) };
			buf["materialColor"] = DirectX::XMFLOAT3{ 0.8f, 0.8f, 0.2f };
			step.AddBindable(std::make_shared<Bind::CachingPixelConstantBufferEx>(gfx, std::move(buf), 1u));
		}

		step.AddBindable(std::make_shared<TransformCbuf>(gfx));

		// Wireframe rasterizer with depth bias so it sits on top of original surface
		step.AddBindable(Rasterizer::Resolve(gfx, true, false)); // two-sided, solid fill
		// Use DepthFirst (LESS_EQUAL, no depth write) so it overlays the existing surface
		step.AddBindable(Stencil::Resolve(gfx, Stencil::Mode::DepthFirst));
		// Enable alpha blending for subtle overylay
		step.AddBindable(Blender::Resolve(gfx, true));

		fill.AddStep(std::move(step));
		AddTechnique(std::move(fill));
	}

	// Wireframe edge outline of the triangle (bright yellow lines)
	{
		Technique wire{ "TriWireframe", Chan::main, true };
		Step step("wireframe");

		auto pvs = VertexShader::Resolve(gfx, "Solid_VS.cso");
		step.AddBindable(InputLayout::Resolve(gfx, vertices.GetLayout(), *pvs));
		step.AddBindable(std::move(pvs));
		step.AddBindable(PixelShader::Resolve(gfx, "Solid_PS.cso"));

		{
			Dcb::RawLayout lay;
			lay.Add<Dcb::Float3>("materialColor");
			Dcb::Buffer buf{ std::move(lay) };
			buf["materialColor"] = DirectX::XMFLOAT3{ 1.0f, 1.0f, 0.0f };
			step.AddBindable(std::make_shared<Bind::CachingPixelConstantBufferEx>(gfx, std::move(buf), 1u));
		}

		step.AddBindable(std::make_shared<TransformCbuf>(gfx));
		step.AddBindable(Rasterizer::Resolve(gfx, true, true)); // two-sided, wireframe
		step.AddBindable(Stencil::Resolve(gfx, Stencil::Mode::DepthFirst));

		wire.AddStep(std::move(step));
		AddTechnique(std::move(wire));
	}
}

dx::XMMATRIX TriangleIndicator::GetTransformXM() const noexcept
{
	return dx::XMMatrixIdentity();
}