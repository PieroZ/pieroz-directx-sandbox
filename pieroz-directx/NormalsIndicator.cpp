#include "NormalsIndicator.h"
#include "BindableCommon.h"
#include "Vertex.h"
#include "Channels.h"
#include "DynamicConstant.h"
#include "ConstantBuffersEx.h"
#include "Stencil.h"
#include <cmath>
#include <string>

namespace dx = DirectX;

NormalsIndicator::NormalsIndicator(Graphics& gfx,
	const std::vector<std::pair<DirectX::XMFLOAT3, DirectX::XMFLOAT3>>& segments,
	const DirectX::XMFLOAT3& color)
{
	using namespace Bind;

	Dvtx::VertexLayout layout;
	layout.Append(Dvtx::VertexLayout::Position3D);
	Dvtx::VertexBuffer vertices(std::move(layout));
	std::vector<unsigned short> indices;

	const auto addLine = [&](const dx::XMFLOAT3& a, const dx::XMFLOAT3& b)
	{
		const auto base = static_cast<unsigned short>(vertices.Size());
		vertices.EmplaceBack(a);
		vertices.EmplaceBack(b);
		indices.push_back(base);
		indices.push_back(static_cast<unsigned short>(base + 1));
	};

	for (const auto& seg : segments)
	{
		const auto start = dx::XMLoadFloat3(&seg.first);
		const auto tip = dx::XMLoadFloat3(&seg.second);

		// Main shaft
		addLine(seg.first, seg.second);

		// Build a simple arrowheard from two backward-angled barbs
		const auto delta = dx::XMVectorSubtract(tip, start);
		const float len = dx::XMVectorGetX(dx::XMVector3Length(delta));
		if (len > 1e-6f)
		{
			const auto dir = dx::XMVector3Normalize(delta);

			// Pick and arbitrary vector not parallel to dir to derive a perpendicular
			dx::XMVECTOR up = dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
			if (std::abs(dx::XMVectorGetX(dx::XMVector3Dot(dir, up))) > 0.99f)
			{
				up = dx::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
			}
			const auto perp = dx::XMVector3Normalize(dx::XMVector3Cross(dir, up));
			
			const float barb = len * 0.25f;
			const auto backTip = dx::XMVectorSubtract(tip, dx::XMVectorScale(dir, barb));

			dx::XMFLOAT3 barb1, barb2;
			dx::XMStoreFloat3(&barb1, dx::XMVectorAdd(backTip, dx::XMVectorScale(perp, barb * 0.5f)));
			dx::XMStoreFloat3(&barb2, dx::XMVectorSubtract(backTip, dx::XMVectorScale(perp, barb * 0.5f)));

			addLine(seg.second, barb1);
			addLine(seg.second, barb2);
		}
	}

	// Guard against an emptry geometry set (degenerate buffers are invalid)
	if (indices.empty())
	{
		vertices.EmplaceBack(dx::XMFLOAT3{ 0.0f,0.0f,0.0f });
		vertices.EmplaceBack(dx::XMFLOAT3{ 0.0f,0.0f,0.0f });
		indices.push_back(0);
		indices.push_back(1);
	}

	static int uid = 0;
	const std::string tag = "$normals_indicator_" + std::to_string(uid++);

	pVertices = std::make_shared<VertexBuffer>(gfx, tag, vertices);
	pIndices = std::make_shared<IndexBuffer>(gfx, indices);
	pTopology = std::make_shared<Topology>(gfx, D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

	Technique line{ "NormalsLines", Chan::main, true };
	Step step("lambertian");

	auto pvs = VertexShader::Resolve(gfx, "Solid_VS.cso");
	step.AddBindable(InputLayout::Resolve(gfx, vertices.GetLayout(), *pvs));
	step.AddBindable(std::move(pvs));
	step.AddBindable(PixelShader::Resolve(gfx, "Solid_PS.cso"));

	{
		Dcb::RawLayout lay;
		lay.Add<Dcb::Float3 > ("materialColor");
		Dcb::Buffer buf{ std::move(lay) };
		buf["materialColor"] = color;
		step.AddBindable(std::make_shared<Bind::CachingPixelConstantBufferEx>(gfx, std::move(buf), 1u));
	}

	step.AddBindable(std::make_shared<TransformCbuf>(gfx));
	step.AddBindable(Rasterizer::Resolve(gfx, true, false));
	step.AddBindable(Stencil::Resolve(gfx, Stencil::Mode::DepthFirst));

	line.AddStep(std::move(step));
	AddTechnique(std::move(line));
}

dx::XMMATRIX NormalsIndicator::GetTransformXM() const noexcept
{
	return dx::XMMatrixIdentity();
}