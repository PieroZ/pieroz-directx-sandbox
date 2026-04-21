#pragma once
#include "Graphics.h"
#include "imgui/imgui.h"

class Mesh;

namespace Bind { class Texture; }

class UVEditorWindow
{
public:
	// Call each frame. Pass the currently picked mesh adn face index(or nullptr if nothing is picked)
	void Show(Graphics& gfx, Mesh* pMesh, size_t faceIndex);
private:
	// Find the diffuse texture (slot 0) from the Phong technique of the mesh
	Bind::Texture* FindDiffuseTexture(Mesh* pMesh) const;
	// Dragging state
	int dragVertex = -1; // 0,1,2 or -1
};