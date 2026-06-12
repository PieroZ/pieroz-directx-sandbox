#include "PrimDrawable.h"
#include "BindableCommon.h"
#include "Channels.h"
#include "Picking.h"
#include "DynamicConstant.h"
#include "ConstantBuffersEx.h"
#include "Stencil.h"

namespace dx = DirectX;

PrimDrawable::PrimDrawable(Graphics& gfx, IndexedTriangleList triList, const std::string& texturePath)
{
	using namespace Bind;
	using Elements = Dvtx::VertexLayout::ElementType;

	const auto& vbuf = triList.vertices;

	// Store CPU data for picking
	cpuIndices = triList.indices;
	cpuPositions.reserve(vbuf.Size());
	for (size_t i = 0; i < vbuf.Size(); i++)	
	{
		cpuPositions.push_back(vbuf[i].Attr<Elements::Position3D>());
	}

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
		step.AddBindable(Bind::Texture::Resolve(gfx, texturePath, 0u));
		step.AddBindable(Sampler::Resolve(gfx));
		step.AddBindable(std::make_shared<TransformCbuf>(gfx));
		step.AddBindable(Rasterizer::Resolve(gfx, false));
		step.AddBindable(Blender::Resolve(gfx, true));

		unlit.AddStep(std::move(step));
		AddTechnique(std::move(unlit));
	}

	// ColorLit technique (inactive by default)
	{
		Technique colorLit{ "ColorLit", Chan::main,false };
		Step step("lambertian");
	
		step.AddBindable(std::make_shared<TransformCbuf>(gfx, 0u));
		auto pvs = VertexShader::Resolve(gfx, "ColorLit_VS.cso");
		step.AddBindable(InputLayout::Resolve(gfx, vbuf.GetLayout(), *pvs));
		step.AddBindable(std::move(pvs));
		step.AddBindable(PixelShader::Resolve(gfx, "ColorLit_PS.cso"));

		// Material + light tint parameters
		{
			Dcb::RawLayout lay;
			lay.Add<Dcb::Float3>("materialColor");
			lay.Add<Dcb::Float>("padding0");
			lay.Add<Dcb::Float3>("lightTint");
			lay.Add<Dcb::Float>("lightIntensity");
			auto buf = Dcb::Buffer(std::move(lay));
			buf["materialColor"] = DirectX::XMFLOAT3{ 0.8f,0.8f,0.8f };
			buf["padding0"] = 0.0f;
			buf["lightTint"] = DirectX::XMFLOAT3{ 0.3f,0.8f,0.8f }; // default: cyan tint
			buf["lightIntensity"] = 2.0f;
			step.AddBindable(std::make_shared<Bind::CachingPixelConstantBufferEx>(gfx, buf, 1u));
		}

		colorLit.AddStep(std::move(step));
		AddTechnique(std::move(colorLit));
	}
	// Wireframe technique (inactive by default)
	{
		Technique wire{ "Wireframe", Chan::main, false };
		Step step("wireframe");
		{
			Dcb::RawLayout lay;
			lay.Add<Dcb::Float3>("materialColor");
			auto buf = Dcb::Buffer(std::move(lay));
			buf["materialColor"] = DirectX::XMFLOAT3(1.0f, 1.0f, 0.0f);
			step.AddBindable(std::make_shared<Bind::CachingPixelConstantBufferEx>(gfx, buf, 1u));
		}

		auto pvs = VertexShader::Resolve(gfx, "Solid_VS.cso");
		step.AddBindable(InputLayout::Resolve(gfx, vbuf.GetLayout(), *pvs));
		step.AddBindable(std::move(pvs));
		step.AddBindable(PixelShader::Resolve(gfx, "Solid_PS.cso"));
		step.AddBindable(Rasterizer::Resolve(gfx, true, true));
		step.AddBindable(Stencil::Resolve(gfx, Stencil::Mode::DepthFirst));
		step.AddBindable(std::make_shared<TransformCbuf>(gfx));


		wire.AddStep(std::move(step));
		AddTechnique(std::move(wire));
	}

	// Selection wireframe overlay (pulsating outline for selected prims)
	{
		Technique sel{ "Selection", Chan::main, false };
		Step draw("wireframe");
		{
			Dcb::RawLayout lay;
			lay.Add<Dcb::Float3>("materialColor");
			auto buf = Dcb::Buffer(std::move(lay));
			buf["materialColor"] = DirectX::XMFLOAT3(0.0f, 1.0f, 5.0f); 
			draw.AddBindable(std::make_shared<Bind::CachingPixelConstantBufferEx>(gfx, buf, 1u));
		}

		auto pvs = VertexShader::Resolve(gfx, "Solid_VS.cso");
		draw.AddBindable(InputLayout::Resolve(gfx, vbuf.GetLayout(), *pvs));
		draw.AddBindable(std::move(pvs));
		draw.AddBindable(PixelShader::Resolve(gfx, "Solid_PS.cso"));
		draw.AddBindable(Rasterizer::Resolve(gfx, true, true));
		draw.AddBindable(Stencil::Resolve(gfx, Stencil::Mode::DepthFirst));
		draw.AddBindable(std::make_shared<TransformCbuf>(gfx));


		sel.AddStep(std::move(draw));
		AddTechnique(std::move(sel));
	}
}

DirectX::XMMATRIX PrimDrawable::GetTransformXM() const noexcept
{
	return         
		DirectX::XMMatrixRotationY(yaw) *
		DirectX::XMMatrixTranslation(position.x, position.y, position.z);
}

void PrimDrawable::SetPosition(const DirectX::XMFLOAT3& pos) noexcept
{
	this->position = pos;
}

void PrimDrawable::SetPosition(float x, float y, float z) noexcept
{
	this->position = { x, y, z };
}

void PrimDrawable::SetYaw(float y) noexcept
{
	yaw = y;
}

std::optional<std::pair<size_t, float>> PrimDrawable::Intersect(
	DirectX::FXMVECTOR rayOrigin, 
	DirectX::FXMVECTOR rayDir) const noexcept
{
	// Transform ray to local space
	const dx::XMMATRIX world = GetTransformXM();
	const dx::XMMATRIX invWorld = dx::XMMatrixInverse(nullptr, world);
	const dx::XMVECTOR localOrigin = dx::XMVector3TransformCoord(rayOrigin, invWorld);
	const dx::XMVECTOR localDir = dx::XMVector3TransformNormal(rayDir, invWorld);

	float closestDist = FLT_MAX;
	size_t closestFace = 0;
	bool hit = false;

	const size_t numTriangles = cpuIndices.size() / 3;
	for (size_t i = 0; i < numTriangles; i++)
	{
		const dx::XMVECTOR  v0 = dx::XMLoadFloat3(&cpuPositions[cpuIndices[i * 3 + 0]]);
		const dx::XMVECTOR  v1 = dx::XMLoadFloat3(&cpuPositions[cpuIndices[i * 3 + 1]]);
		const dx::XMVECTOR  v2 = dx::XMLoadFloat3(&cpuPositions[cpuIndices[i * 3 + 2]]);

		if (const  auto t = Picking::RayTriangleIntersect(localOrigin, localDir, v0, v1, v2))
		{
			if (*t < closestDist)
			{
				closestDist = *t;
				closestFace = i;
				hit = true;
			}
		}
	}
	if (hit)
		return std::make_pair(closestFace, closestDist);
	return std::nullopt;
}