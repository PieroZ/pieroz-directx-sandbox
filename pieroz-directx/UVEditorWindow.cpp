#include "UVEditorWindow.h"
#include "Mesh.h"
#include "Texture.h"
#include "Technique.h"
#include "Step.h"
#include "Vertex.h"
#include <algorithm>
#include <commdlg.h>
#include <array>

namespace dx = DirectX;

static std::string OpenTextureFileDialogUV()
{
	std::array<char, MAX_PATH> buf{};
	OPENFILENAMEA ofn{};
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = nullptr;
	ofn.lpstrFile = buf.data();
	ofn.nMaxFile = (DWORD)buf.size();
	ofn.lpstrFilter = "Image Files\0*.png;*.jpg;*.jpeg;*.bmp;*.tga\0All Files\0*.*\0";
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
	if (GetOpenFileNameA(&ofn))
	{
		return std::string( buf.data() );
	}
	return {};
}

Bind::Texture* UVEditorWindow::FindDiffuseTexture(Mesh* pMesh) const
{
	for (auto& tech : pMesh->GetTechniques())
	{
		if (tech.GetName() != "Phong")
			continue;
		for (auto& step : tech.GetSteps())
		{
			for (auto& bindable : step.GetBindables())
			{
				if( auto* pTex = dynamic_cast<Bind::Texture*> (bindable.get()) )
				{
					if (pTex->GetSlot() == 0) // diffuse
						return pTex;
				}
			}
		}
	}
	return nullptr;
}

void UVEditorWindow::Show(Graphics& gfx, Mesh* pMesh, size_t faceIndex,
	std::function<void(Mesh*, size_t, const std::string&)> onFaceTextureChanged)
{
	ImGui::Begin("UV Editor");

	if (!pMesh || !pMesh->HasUVs())
	{
		ImGui::TextColored({ 0.7f,0.7f,0.7f,1.0f }, "Select a textured triangle to edit UVs");
		ImGui::End();
		return;
	}

	const auto& indices = pMesh->GetCpuIndices();
	const auto& uvs = pMesh->GetCpuUVs();

	if (faceIndex * 3 + 2 >= indices.size())
	{
		ImGui::TextColored({ 1.0f,0.7f,0.7f,1.0f }, "Invalid face index");
		ImGui::End();
		return;
	}

	const size_t i0 = indices[faceIndex * 3 + 0];
	const size_t i1 = indices[faceIndex * 3 + 1];
	const size_t i2 = indices[faceIndex * 3 + 2];

	// Read current UVs (mutable copy for editing)
	dx::XMFLOAT2 uv0 = uvs[i0];
	dx::XMFLOAT2 uv1 = uvs[i1];
	dx::XMFLOAT2 uv2 = uvs[i2];

	// UV input fields
	bool uvChanged = false;
	ImGui::TextColored({ 0.4f,1.0f,0.6f,1.0f }, "UV Coordinates (vertex index: u, v)");
	ImGui::PushItemWidth(200.0f);

	char label0[64], label1[64], label2[64];
	snprintf(label0, sizeof(label0), "uv0 [vtx %zu]", (size_t)i0);
	snprintf(label1, sizeof(label1), "uv1 [vtx %zu]", (size_t)i1);
	snprintf(label2, sizeof(label2), "uv2 [vtx %zu]", (size_t)i2);

	if (ImGui::DragFloat2(label0, &uv0.x, 0.001f, 0.0f, 1.0f, "%.4f")) uvChanged = true;
	if (ImGui::DragFloat2(label1, &uv1.x, 0.001f, 0.0f, 1.0f, "%.4f")) uvChanged = true;
	if (ImGui::DragFloat2(label2, &uv2.x, 0.001f, 0.0f, 1.0f, "%.4f")) uvChanged = true;

	ImGui::PopItemWidth();

	// Find diffuse texture (default from Phong technique)
	auto* pDiffuseTex = FindDiffuseTexture(pMesh);

	// If the current face has a texture override, show that instead
	std::shared_ptr<Bind::Texture> pOverrideTex;
	ID3D11ShaderResourceView* pPreviewSRV = nullptr;
	std::string previewTexPath;

	if(pMesh->HasFaceTextureOverride(faceIndex))
	{
		const auto& overridePath = pMesh->GetFaceTextureOverrides().at(faceIndex);
		pOverrideTex = Bind::Texture::Resolve(gfx, overridePath, 0u);
		pPreviewSRV = pOverrideTex->GetShaderResourceView();
		previewTexPath = overridePath;
	}
	else if (pDiffuseTex)
	{
		pPreviewSRV = pDiffuseTex->GetShaderResourceView();
		previewTexPath = pDiffuseTex->GetPath();
	}


	// Texture image + UV triangle overlay
	const float canvasSize = 400.0f;
	ImGui::Separator();
	ImGui::TextColored({ 0.4f,1.0f,0.6f,1.0f }, "Texture Preview");

	if (pPreviewSRV)
	{
		ImGui::Text(" Texture %s)", previewTexPath.c_str());
	}
	else
	{
		ImGui::Text(" No diffuse texture found");
	}

	// Canvas area for texture + UV triangle
	const ImVec2 canvasPos = ImGui::GetCursorScreenPos();
	const ImVec2 canvasEnd = ImVec2(canvasPos.x + canvasSize, canvasPos.y + canvasSize);

	// Draw texture image as background
	if (pPreviewSRV)
	{
		ImGui::Image(
			(ImTextureID)pPreviewSRV,
			ImVec2(canvasSize, canvasSize),
			ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f)
		);
		// Make the image area interactive - place invisible button on top
		ImGui::SetCursorScreenPos(canvasPos);
		ImGui::InvisibleButton("uv_canvas_interact", ImVec2(canvasSize, canvasSize));
	}
	else
	{
		// Draw empty canvas
		ImGui::InvisibleButton("uv_canvas_interact", ImVec2(canvasSize, canvasSize));
	}
	const bool canvasHovered = ImGui::IsItemHovered();
	const bool canvasActive = ImGui::IsItemActive();

	// Draw UV triangle overlay
	ImDrawList* drawList = ImGui::GetWindowDrawList();

	// Uv to screen position helper(UV 0,0 = top-left of canvas; UV 1,1 = bottom-right of canvas)
	auto uvToScreen = [&](const dx::XMFLOAT2& uv) -> ImVec2
		{
			return ImVec2(
				canvasPos.x + uv.x * canvasSize,
				canvasPos.y + uv.y * canvasSize
			);
		};

	const ImVec2 p0 = uvToScreen(uv0);
	const ImVec2 p1 = uvToScreen(uv1);
	const ImVec2 p2 = uvToScreen(uv2);

	// Filled triangle (semi-transparent)
	drawList->AddTriangleFilled(p0, p1, p2, IM_COL32(255, 255, 0, 100));
	// Wireframe outline
	drawList->AddTriangle(p0, p1, p2, IM_COL32(255, 255, 0, 255), 2.0f);

	// Vertex handles (draggable circles)
	const float handleRadius = 6.0f;
	const ImU32 handleColors[3] = { IM_COL32(255, 80, 80, 255), IM_COL32(80, 255, 80, 255), IM_COL32(80, 80, 255, 255) };
	const ImVec2 handles[3] = { p0, p1, p2 };
	dx::XMFLOAT2* uvPtrs[3] = { &uv0, &uv1, &uv2 };

	// Mouse interaction for dragging UV vertices on the canvas
	const ImVec2 mousePos = ImGui::GetIO().MousePos;
	const bool mouseDown = ImGui::GetIO().MouseDown[0];
	const bool mouseClicked = ImGui::GetIO().MouseClicked[0];

	// Start drag on click near a handle
	if (mouseClicked && canvasHovered)
	{
		for (int i = 0; i < 3; i++)
		{
			const float ddx = mousePos.x - handles[i].x;
			const float ddy = mousePos.y - handles[i].y;
			if (ddx * ddx + ddy * ddy <= handleRadius * handleRadius * 4.0f)
			{
				dragVertex = i;
				break;
			}
		}
	}
	// Stop drag on release
	if(!mouseDown)
	{
		dragVertex = -1;
	}

	// Update dragged vertex UV
	if (dragVertex >= 0 && mouseDown)
	{
		float newU = (mousePos.x - canvasPos.x) / canvasSize;
		float newV = (mousePos.y - canvasPos.y) / canvasSize;
		newU = std::max(0.0f, std::min(1.0f, newU));
		newV = std::max(0.0f, std::min(1.0f, newV));
		uvPtrs[dragVertex]->x = newU;
		uvPtrs[dragVertex]->y = newV;
		uvChanged = true;
	}

	// Draw handles
	for (int i = 0; i < 3; i++)
	{
		ImVec2 hPos = uvToScreen(*uvPtrs[i]);
		drawList->AddCircleFilled(hPos, handleRadius, handleColors[i]);
		drawList->AddCircle(hPos, handleRadius, IM_COL32(255, 255, 255, 200), 0, 2.0f);

		// Label
		char vtxLabel[8];
		snprintf(vtxLabel, sizeof(vtxLabel), "v%d", i);
		drawList->AddText(ImVec2(hPos.x + handleRadius + 2, hPos.y - 6), handleColors[i], vtxLabel);
	}

	// Apply UV changes back to CPU data and update GPU vertex buffer

	if (uvChanged)
	{
		pMesh->SetCpuUV(i0, uv0);
		pMesh->SetCpuUV(i1, uv1);
		pMesh->SetCpuUV(i2, uv2);
		pMesh->UpdateGpuVertexBuffer(gfx);
	}

	// Per-face texture override section
	ImGui::Separator();
	ImGui::TextColored({ 0.4f,1.0f,0.6f,1.0f }, "Face Texture Override");

	if (pMesh->HasFaceTextureOverride(faceIndex))
	{
		const auto& overrideTex = pMesh->GetFaceTextureOverrides().at(faceIndex);
		ImGui::Text("Override %s", overrideTex.c_str());
		if (ImGui::Button("Remove Override"))
		{
			pMesh->ClearFaceTextureOverride(faceIndex);
			if(onFaceTextureChanged)
				onFaceTextureChanged(pMesh, faceIndex, "");
		}
	}
	else
	{
		ImGui::Text("Using mesh default texture");
	}

	if (ImGui::Button("Change Face Texture..."))
	{
		const auto newPath = OpenTextureFileDialogUV();
		if (!newPath.empty())
		{
			pMesh->SetFaceTextureOverride(faceIndex, newPath);
			if(onFaceTextureChanged)
				onFaceTextureChanged(pMesh, faceIndex, newPath);
		}
	}

	ImGui::End();
}