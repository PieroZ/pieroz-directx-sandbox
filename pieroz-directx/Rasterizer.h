#pragma once
#include "Bindable.h"
#include <array>

namespace Bind
{
	class Rasterizer : public Bindable
	{
	public:
		Rasterizer( Graphics& gfx,bool twoSided, bool wireframe = false );
		void Bind( Graphics& gfx ) noxnd override;
		static std::shared_ptr<Rasterizer> Resolve( Graphics& gfx,bool twoSided, bool wireframe = false );
		static std::string GenerateUID( bool twoSided, bool wireframe = false );
		std::string GetUID() const noexcept override;
	protected:
		Microsoft::WRL::ComPtr<ID3D11RasterizerState> pRasterizer;
		bool twoSided;
		bool wireframe;
	};
}
