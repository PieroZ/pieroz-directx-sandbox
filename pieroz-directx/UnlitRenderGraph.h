#pragma once
#include "RenderGraph.h"

class Graphics;
class Camera;

namespace Rgph
{
	// Simplified render graph for unlit tile-based scenes.
	// Pipeline: clearRT -> clearDS -> lambertian -> skybox -> wireframe -> backbuffer
	// No shadows, no outline blur.
	class UnlitRenderGraph : public Rgph::RenderGraph
	{
	public:
		UnlitRenderGraph( Graphics& gfx );
		void BindMainCamera(Camera& cam);
		void RenderSkyboxWindow();
	};
}
