#include "Rasterizer.h"
#include "GraphicsThrowMacros.h"
#include "BindableCodex.h"

namespace Bind
{
	Rasterizer::Rasterizer( Graphics& gfx,bool twoSided, bool wireframe, int depthBias, float slopeScaledDepthBias)
		:
		twoSided( twoSided ),
		wireframe( wireframe ),
		depthBias( depthBias ),
		slopeScaledDepthBias( slopeScaledDepthBias )
	{
		INFOMAN( gfx );

		D3D11_RASTERIZER_DESC rasterDesc = CD3D11_RASTERIZER_DESC( CD3D11_DEFAULT{} );
		rasterDesc.CullMode = twoSided ? D3D11_CULL_NONE : D3D11_CULL_BACK;
		if (wireframe)
		{
			rasterDesc.FillMode = D3D11_FILL_WIREFRAME;
			rasterDesc.AntialiasedLineEnable = TRUE;
			rasterDesc.DepthBias = -100;
			rasterDesc.SlopeScaledDepthBias = -1.0f;
			rasterDesc.DepthBiasClamp = -0.001f;
		}
		if (depthBias != 0 || slopeScaledDepthBias != 0.0f)
		{
			rasterDesc.DepthBias = depthBias;
			rasterDesc.SlopeScaledDepthBias = slopeScaledDepthBias;
		}

		GFX_THROW_INFO( GetDevice( gfx )->CreateRasterizerState( &rasterDesc,&pRasterizer ) );
	}

	void Rasterizer::Bind( Graphics& gfx ) noxnd
	{
		INFOMAN_NOHR( gfx );
		GFX_THROW_INFO_ONLY( GetContext( gfx )->RSSetState( pRasterizer.Get() ) );
	}
	
	std::shared_ptr<Rasterizer> Rasterizer::Resolve( Graphics& gfx,bool twoSided, bool wireframe, int depthBias, float slopeScaledDepthBias )
	{
		return Codex::Resolve<Rasterizer>( gfx,twoSided, wireframe, depthBias, slopeScaledDepthBias );
	}
	std::string Rasterizer::GenerateUID( bool twoSided, bool wireframe, int depthBias, float slopeScaledDepthBias)
	{
		using namespace std::string_literals;

		auto uid = typeid(Rasterizer).name() + "#"s + (twoSided ? "2s" : "1s") + (wireframe ? "wf" : "");
		if (depthBias != 0 || slopeScaledDepthBias != 0.0f)
		{
			uid += "#db"s + std::to_string(depthBias) + "#sdb"s + std::to_string(slopeScaledDepthBias);
		}

		return uid;
	}
	std::string Rasterizer::GetUID() const noexcept
	{
		return GenerateUID( twoSided, wireframe, depthBias, slopeScaledDepthBias );
	}
}