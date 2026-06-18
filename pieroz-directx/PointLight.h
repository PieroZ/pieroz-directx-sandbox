#pragma once
#include "Graphics.h"
#include "SolidSphere.h"
#include "ConstantBuffers.h"
#include "ConditionalNoexcept.h"

namespace Rgph
{
	class RenderGraph;
}

class Camera;

class PointLight
{
public:
	PointLight( Graphics& gfx,DirectX::XMFLOAT3 pos = { 10.0f,9.0f,2.5f },float radius = 0.5f );
	void SpawnControlWindow() noexcept;
	void Reset() noexcept;
	void Submit( size_t channels ) const noxnd;
	void Bind( Graphics& gfx,DirectX::FXMMATRIX view ) const noexcept;
	void LinkTechniques( Rgph::RenderGraph& );
	std::shared_ptr<Camera> ShareCamera() const noexcept;
	// World-space positions accessors (used to anime the light as a scene object)
	void SetPos(DirectX::XMFLOAT3 pos) noexcept;
	DirectX::XMFLOAT3 GetPos() const noexcept;
	void SetFlashLight(bool enabled, DirectX::XMFLOAT3 viewPos, DirectX::XMFLOAT3 viewDir,
		DirectX::XMFLOAT3 color, float innerDeg, float outerDeg, float range, float intensity) noexcept;
private:
	struct PointLightCBuf
	{
		alignas(16) DirectX::XMFLOAT3 pos;
		alignas(16) DirectX::XMFLOAT3 ambient;
		alignas(16) DirectX::XMFLOAT3 diffuseColor;
		float diffuseIntensity;
		float attConst;
		float attLin;
		float attQuad;

		// Flashlight /spotlight (view space)
		alignas(16) DirectX::XMFLOAT3 spotPos;
		alignas(16) DirectX::XMFLOAT3 spotDir;
		alignas(16) DirectX::XMFLOAT3 spotColor;
		float spotInnerCos;
		float spotOuterCos;
		float spotRange;
		float spotIntensity;
		float spotEnabled;
	};
private:
	PointLightCBuf home;
	PointLightCBuf cbData;
	mutable SolidSphere mesh;
	mutable Bind::PixelConstantBuffer<PointLightCBuf> cbuf;
	std::shared_ptr<Camera> pCamera;
};