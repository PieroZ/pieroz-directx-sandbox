#pragma once
#include "RenderQueuePass.h"
#include "Job.h"
#include <vector>
#include "Sink.h"
#include "Source.h"
#include "Stencil.h"
#include "Camera.h"
#include "Sampler.h"
#include "Rasterizer.h"
#include "DepthStencil.h"
#include "Cube.h"
#include "CubeTexture.h"
#include "SkyboxTransformCbuf.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "Topology.h"
#include "InputLayout.h"
#include "PixelShader.h"
#include "VertexShader.h"
#include "Sphere.h"
#include "imgui/imgui.h"

class Graphics;

namespace Rgph
{
	class SkyboxPass : public BindingPass
	{
	public:
		SkyboxPass( Graphics& gfx,std::string name )
			:
			BindingPass( std::move( name ) )
		{
			using namespace Bind;
			RegisterSink( DirectBufferSink<RenderTarget>::Make( "renderTarget",renderTarget ) );
			RegisterSink( DirectBufferSink<DepthStencil>::Make( "depthStencil",depthStencil ) );
			AddBind( std::make_shared<CubeTexture>( gfx,"Images\\SpaceBox" ) );
			AddBind( Stencil::Resolve( gfx,Stencil::Mode::DepthFirst ) );
			AddBind( Sampler::Resolve( gfx,Sampler::Type::Bilinear ) );
			AddBind( Rasterizer::Resolve( gfx,true ) );
			AddBind( std::make_shared<SkyboxTransformCbuf>( gfx ) );
			AddBind( PixelShader::Resolve( gfx,"Skybox_PS.cso" ) );
			AddBind( Topology::Resolve( gfx,D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST ) );
			{ // geometry-related
				auto pvs = Bind::VertexShader::Resolve( gfx,"Skybox_VS.cso" );
				{ // cube
					auto model = Cube::Make();
					const auto geometryTag = "$cube_map";
					pCubeVertices = VertexBuffer::Resolve( gfx,geometryTag,std::move( model.vertices ) );
					pCubeIndices = IndexBuffer::Resolve( gfx,geometryTag,std::move( model.indices ) );
					cubeCount = (UINT)model.indices.size();
					// layout is shared between cube and sphere; use cube data to generate
					AddBind( InputLayout::Resolve( gfx,model.vertices.GetLayout(),*pvs ) );
				}
				{ // sphere
					auto model = Sphere::Make();
					const auto geometryTag = "$sphere_map";
					pSphereVertices = VertexBuffer::Resolve( gfx,geometryTag,std::move( model.vertices ) );
					pSphereIndices = IndexBuffer::Resolve( gfx,geometryTag,std::move( model.indices ) );
					sphereCount = (UINT)model.indices.size();
				}
				AddBind( std::move( pvs ) );
			}
			RegisterSource( DirectBufferSource<RenderTarget>::Make( "renderTarget",renderTarget ) );
			RegisterSource( DirectBufferSource<DepthStencil>::Make( "depthStencil",depthStencil ) );
		}
		void BindMainCamera( const Camera& cam ) noexcept
		{
			pMainCamera = &cam;
		}
		void Execute( Graphics& gfx ) const noxnd override
		{
			if (!enabled)
				return;
			assert( pMainCamera );
			UINT indexCount;
			pMainCamera->BindToGraphics( gfx );
			if( useSphere )
			{
				pSphereVertices->Bind( gfx );
				pSphereIndices->Bind( gfx );
				indexCount = sphereCount;
			}
			else
			{
				pCubeVertices->Bind( gfx );
				pCubeIndices->Bind( gfx );
				indexCount = cubeCount;
			}
			BindAll( gfx ); 
			gfx.DrawIndexed( indexCount );
		}
		void RenderWindow()
		{
			if( ImGui::Begin( "Skybox" ) )
			{
				ImGui::Checkbox("Enabled", &enabled);
				ImGui::Checkbox( "Use sphere",&useSphere );
				ImGui::Separator();
				ImGui::Text("Current: %s", currenPath.c_str());
				static char pathBuf[256] = "Images\\SpaceBox";
				ImGui::InputText("Path", pathBuf, sizeof(pathBuf));
				if (ImGui::Button("Load Skybox"))
				{
					SetCubeMap(std::string(pathBuf));
				}
			}
			ImGui::End();
		}
		void SetEnabled(bool e) noexcept { enabled = e; }
		bool IsEnabled() const noexcept { return enabled; }
		void SetCubeMap(const std::string& path)
		{
			currenPath = path;
			pCubeTexture = std::make_shared<Bind::CubeTexture>(*pGfx, path);
		}
	private:
		bool enabled = true;
		bool useSphere = true;
		std::string currenPath = "Images\\SpaceBox";
		Graphics* pGfx = nullptr;
		const Camera* pMainCamera = nullptr;
		std::shared_ptr<Bind::CubeTexture> pCubeTexture;
		std::shared_ptr<Bind::VertexBuffer> pCubeVertices;
		std::shared_ptr<Bind::IndexBuffer> pCubeIndices;
		UINT cubeCount;
		std::shared_ptr<Bind::VertexBuffer> pSphereVertices;
		std::shared_ptr<Bind::IndexBuffer> pSphereIndices;
		UINT sphereCount;
	};
}