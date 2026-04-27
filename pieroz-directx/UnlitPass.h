#pragma once
#include "RenderQueuePass.h"
#include "Job.h"
#include "Sink.h"
#include "Source.h"
#include "Stencil.h"
#include "Camera.h"
#include "Sampler.h"
#include <vector>

class Graphics;

namespace Rgph
{
	// A render queue pass that renders with no shadow mapping or lighting steup.
	// Suitable for unlit / flat-textured scenes.
	class UnlitPass : public RenderQueuePass
	{
	public:
		UnlitPass(Graphics& gfx, std::string name)
			:
			RenderQueuePass(std::move(name))
		{
			using namespace Bind;
			RegisterSink(DirectBufferSink<RenderTarget>::Make("renderTarget", renderTarget));
			RegisterSink(DirectBufferSink<DepthStencil>::Make("depthStencil", depthStencil));
			AddBind(std::make_shared<Bind::Sampler>(gfx, Bind::Sampler::Type::Anisotropic, false, 2));
			RegisterSource(DirectBufferSource<RenderTarget>::Make("renderTarget", renderTarget));
			RegisterSource(DirectBufferSource<DepthStencil>::Make("depthStencil", depthStencil));
			AddBind(Stencil::Resolve(gfx, Stencil::Mode::Off));
		}
		void BindMainCamera(Camera& cam)
		{
			pMainCamera = &cam;
		}
		void Execute(Graphics& gfx) const noxnd override
		{
			assert(pMainCamera);
			pMainCamera->BindToGraphics(gfx);
			RenderQueuePass::Execute(gfx);
		};
	private:
		const Camera* pMainCamera = nullptr;
	};
}