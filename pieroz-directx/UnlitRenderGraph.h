#pragma once
#include "RenderGraph.h"

class Graphics;
class Camera;

namespace Rgph
{
	// Simplified render graph for unlit tile-based scenes.
	// Pipeline: clearRT -> clearDS -> lambertian -> wireframe -> backbuffer
	// No shadows, no outline blur, no skybox.
	class UnlitRenderGraph : public Rgph::RenderGraph
	{
	public:
		UnlitRenderGraph( Graphics& gfx );
		void BindMainCamera(Camera& cam);
	};
}
