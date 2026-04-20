#include "Rasterizer.h"
#include "GraphicsThrowMacros.h"
#include "BindableCodex.h"

namespace Bind
{
	Rasterizer::Rasterizer( Graphics& gfx,bool twoSided, bool wireframe )
		:
		twoSided( twoSided ),
		wireframe( wireframe )
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

		GFX_THROW_INFO( GetDevice( gfx )->CreateRasterizerState( &rasterDesc,&pRasterizer ) );
	}

	void Rasterizer::Bind( Graphics& gfx ) noxnd
	{
		INFOMAN_NOHR( gfx );
		GFX_THROW_INFO_ONLY( GetContext( gfx )->RSSetState( pRasterizer.Get() ) );
	}
	
	std::shared_ptr<Rasterizer> Rasterizer::Resolve( Graphics& gfx,bool twoSided, bool wireframe )
	{
		return Codex::Resolve<Rasterizer>( gfx,twoSided, wireframe );
	}
	std::string Rasterizer::GenerateUID( bool twoSided, bool wireframe )
	{
		using namespace std::string_literals;
		return typeid(Rasterizer).name() + "#"s + (twoSided ? "2s" : "1s") + (wireframe ? "wf" : "");
	}
	std::string Rasterizer::GetUID() const noexcept
	{
		return GenerateUID( twoSided, wireframe );
	}
}