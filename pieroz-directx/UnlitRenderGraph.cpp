#include "UnlitRenderGraph.h"
#include "BufferClearPass.h"
#include "UnlitPass.h"
#include "WireframePass.h"
#include "Source.h"
#include "RenderTarget.h"
#include "DepthStencil.h"
#include "SkyboxPass.h"

namespace Rgph
{
	UnlitRenderGraph::UnlitRenderGraph(Graphics& gfx) : RenderGraph(gfx)
	{
		// Clear render target
		{
			auto pass = std::make_unique<BufferClearPass>("clearRT");
			pass->SetSinkLinkage("buffer", "$.backbuffer");
			AppendPass(std::move(pass));
		}

		// Clear depth stencil
		{
			auto pass = std::make_unique<BufferClearPass>("clearDS");
			pass->SetSinkLinkage("buffer", "$.masterDepth");
			AppendPass(std::move(pass));
		}

		// Main unlit render pass (named "lambertian" so existing Steps target it)
		{
			auto pass = std::make_unique<UnlitPass>(gfx, "lambertian");
			pass->SetSinkLinkage("renderTarget", "clearRT.buffer");
			pass->SetSinkLinkage("depthStencil", "clearDS.buffer");
			AppendPass(std::move(pass));
		}

		// Skybox pass
		{
			auto pass = std::make_unique<SkyboxPass>(gfx, "skybox");
			pass->SetSinkLinkage("renderTarget", "lambertian.renderTarget");
			pass->SetSinkLinkage("depthStencil", "lambertian.depthStencil");
			AppendPass(std::move(pass));
		}

		// Wireframe overlay pass
		{
			auto pass = std::make_unique<WireframePass>(gfx, "wireframe");
			pass->SetSinkLinkage("renderTarget", "skybox.renderTarget");
			pass->SetSinkLinkage("depthStencil", "skybox.depthStencil");
			AppendPass(std::move(pass));
		}

		SetSinkTarget("backbuffer", "wireframe.renderTarget");
		Finalize();
	}

	void UnlitRenderGraph::BindMainCamera(Camera& cam)
	{
		dynamic_cast<UnlitPass&>(FindPassByName("lambertian")).BindMainCamera(cam);
		dynamic_cast<SkyboxPass&>(FindPassByName("skybox")).BindMainCamera(cam);

	}

	void UnlitRenderGraph::RenderSkyboxWindow()
	{
		dynamic_cast<SkyboxPass&>(FindPassByName("skybox")).RenderWindow();
	}

}