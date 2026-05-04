#include "PrimDrawable.h"
#include "BindableCommon.h"
#include "Channels.h"

namespace dx = DirectX;

PrimDrawable::PrimDrawable(Graphics& gfx, IndexedTriangleList triList)
{
	using namespace Bind;

	const auto& vbuf = triList.vertices;

	static int uid = 0;
	const std::string tag = "$prim_drawable_" + std::to_string(uid++);

	pVertices = std::make_shared<VertexBuffer>(gfx, tag, vbuf);
	pIndices = std::make_shared<IndexBuffer>(gfx, tag, triList.indices);
	pTopology = Topology::Resolve(gfx, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Unlit solid-color technique (no texture needed for raw prim geometry)
	{
		Technique unlit{ "PrimUnlit", Chan::main, true };
		Step step("lambertian");

		auto pvs = VertexShader::Resolve(gfx, "Unlit_VS.cso");
		step.AddBindable(InputLayout::Resolve(gfx, vbuf.GetLayout(), *pvs));
		step.AddBindable(std::move(pvs));
		step.AddBindable(PixelShader::Resolve(gfx, "Unlit_PS.cso"));

		// Use a white 1x1 fallback texture so Unlit_PS sampler doesn't fail
		step.AddBindable(Bind::Texture::Resolve(gfx, "Images\\white.png", 0u));
		step.AddBindable(Sampler::Resolve(gfx));
		step.AddBindable(std::make_shared<TransformCbuf>(gfx));
		step.AddBindable(Rasterizer::Resolve(gfx, false));

		unlit.AddStep(std::move(step));
		AddTechnique(std::move(unlit));
	}
}

DirectX::XMMATRIX PrimDrawable::GetTransformXM() const noexcept
{
	return DirectX::XMMatrixTranslation(position.x, position.y, position.z);
}

void PrimDrawable::SetPosition(const DirectX::XMFLOAT3& pos) noexcept
{
	this->position = pos;
}

void PrimDrawable::SetPosition(float x, float y, float z) noexcept
{
	this->position = { x, y, z };
}