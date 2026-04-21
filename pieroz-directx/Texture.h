#pragma once
#include "Bindable.h"

class Surface;

namespace Bind
{
	class Texture : public Bindable
	{
	public:
		Texture( Graphics& gfx,const std::string& path,UINT slot = 0 );
		void Bind( Graphics& gfx ) noxnd override;
		static std::shared_ptr<Texture> Resolve( Graphics& gfx,const std::string& path,UINT slot = 0 );
		static std::string GenerateUID( const std::string& path,UINT slot = 0 );
		std::string GetUID() const noexcept override;
		bool HasAlpha() const noexcept;
		const std::string& GetPath() const noexcept { return path; }
		UINT GetSlot() const noexcept { return slot; }
		ID3D11ShaderResourceView* GetShaderResourceView() const noexcept { return pTextureView.Get(); }
	private:
		static UINT CalculateNumberOfMipLevels( UINT width,UINT height ) noexcept;
	private:
		unsigned int slot;
	protected:
		bool hasAlpha = false;
		std::string path;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> pTextureView;
	};
}
