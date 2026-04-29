#include "App.h"
#include <algorithm>

#include "ChiliMath.h"
#include "imgui/imgui.h"
#include "ChiliUtil.h"
#include "Testing.h"
#include "PerfLog.h"
#include "TestModelProbe.h"
#include "Testing.h"
#include "Camera.h"
#include "Channels.h"
#include "Picking.h"
#include "Mesh.h"
#include "Texture.h"
#include "TriangleIndicator.h"
#include "Node.h"
#include "TileMapDef.h"
#include "iamLoader.h"
#include "iamToJson.h"

#include <commdlg.h> // GetOpenFileName
#include <array>

namespace dx = DirectX;

static std::string OpenModelFileDialog()
{
	std::array<char, MAX_PATH> buf{};
	OPENFILENAMEA ofn{};
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = nullptr; 
	ofn.lpstrFile = buf.data();
	ofn.nMaxFile = (DWORD)buf.size();
	ofn.lpstrFilter = "Model Files\0*.obj;*.fbx;*.gltf;*.dae;*.3ds\0All Files\0*.*\0";
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
	if (GetOpenFileNameA(&ofn))
	{
		return std::string(buf.data());
	}
	return {};
}

static std::string OpenTextureFileDialog()
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
		return std::string(buf.data());
	}
	return {};
}

App::App( const std::string& commandLine, SceneType scene )
	:
	commandLine( commandLine ),
	sceneType (scene),
	wnd( 1920,1080,"UC NPRIM Editor" ),
	scriptCommander( TokenizeQuoted( commandLine ) )
{
	if (sceneType == SceneType::Default)
	{
		// -- Default scene: Sponza + lighting + shadows ---
		pBlurRg = std::make_unique<Rgph::BlurOutlineRenderGraph>(wnd.Gfx());
		pLight = std::make_unique<PointLight>(wnd.Gfx(), dx::XMFLOAT3{ 10.0f, 5.0f, 0.0f });

		cameras.AddCamera(std::make_unique<Camera>(wnd.Gfx(), "A", dx::XMFLOAT3{ -13.5f, 6.0f, 3.5f }, 0.0f, PI / 2.0f));
		cameras.AddCamera(std::make_unique<Camera>(wnd.Gfx(), "B", dx::XMFLOAT3{ -13.5f, 28.0f, -6.4f }, PI / 180.0f, PI / 180.0f * 61.0f));
		cameras.AddCamera(pLight->ShareCamera());

		pCube = std::make_unique<TestCube>(wnd.Gfx(), 4.0f);
		pCube2 = std::make_unique<TestCube>(wnd.Gfx(), 4.0f);
		pSponza = std::make_unique<Model>(wnd.Gfx(), "Models\\sponza\\sponza.obj",  1.0f/ 20.f);
		pGobber = std::make_unique<Model>(wnd.Gfx(), "Models\\gobber\\GoblinX.obj",  4.0f);

		pCube->SetPos({ 10.0f, 5.0f, 6.0f });
		pCube2->SetPos({ 10.0f, 5.0f, 14.0f });
		pGobber->SetRootTransform(
			dx::XMMatrixRotationY(-PI / 2.f) *
			dx::XMMatrixTranslation(-8.f, 10.f, 0.f)
		);

		pCube->LinkTechniques(*pBlurRg);
		pCube2->LinkTechniques(*pBlurRg);
		pLight->LinkTechniques(*pBlurRg);
		pSponza->LinkTechniques(*pBlurRg);
		pGobber->LinkTechniques(*pBlurRg);
		cameras.LinkTechniques(*pBlurRg);

		pBlurRg->BindMainCamera(*pLight->ShareCamera());
	}
	else if (sceneType == SceneType::TileMap)
	{
		// -- Tile map scene: flat grid , unlit, no shadows ---
		pUnlitRg = std::make_unique<Rgph::UnlitRenderGraph>(wnd.Gfx());

		cameras.AddCamera(std::make_unique<Camera>(wnd.Gfx(), "TileCamera", 
			dx::XMFLOAT3{ 5.0f, 10.0f, -5.0f }, PI / 4.0f, PI / 4.0f ));

		// Create a default 8x8 tile grid with a placerholder texture
		auto def = TileMapDef::MakeGrid(8, 8, 2.0f, "Images\\brickwall.jpg");
		pTileScene = std::make_unique<TileMapScene>(wnd.Gfx(), def);
		pTileScene->LinkTechniques(*pUnlitRg);
		cameras.LinkTechniques(*pUnlitRg);
	}

	std::string testMapPath = "UC-data\\maps\\bball2.iam";
	std::vector<PAP_Hi> tiles = LoadIamMap(testMapPath);


	auto mapjson = BuildMapJson(tiles);

	SaveIamToJson(mapjson, "bball2_map.json");
}

void App::HandleInput( float dt )
{
	while( const auto e = wnd.kbd.ReadKey() )
	{
		if( !e->IsPress() )
		{
			continue;
		}

		switch( e->GetCode() )
		{
		case VK_ESCAPE:
			if( wnd.CursorEnabled() )
			{
				wnd.DisableCursor();
				wnd.mouse.EnableRaw();
			}
			else
			{
				wnd.EnableCursor();
				wnd.mouse.DisableRaw();
			}
			break;
		case VK_F1:
			showDemoWindow = true;
			break;
		case VK_F2:
			showImguiDebugWindows = !showImguiDebugWindows;
			break;
		case VK_F4:
			wnd.ToggleFullscreen();
			break;
		case VK_RETURN:
			savingDepth = true;
			break;
		}
	}

	if( !wnd.CursorEnabled() )
	{
		if( wnd.kbd.KeyIsPressed( 'W' ) )
		{
			cameras->Translate( { 0.0f,0.0f,dt } );
		}
		if( wnd.kbd.KeyIsPressed( 'A' ) )
		{
			cameras->Translate( { -dt,0.0f,0.0f } );
		}
		if( wnd.kbd.KeyIsPressed( 'S' ) )
		{
			cameras->Translate( { 0.0f,0.0f,-dt } );
		}
		if( wnd.kbd.KeyIsPressed( 'D' ) )
		{
			cameras->Translate( { dt,0.0f,0.0f } );
		}
		if( wnd.kbd.KeyIsPressed( 'R' ) )
		{
			cameras->Translate( { 0.0f,dt,0.0f } );
		}
		if( wnd.kbd.KeyIsPressed( 'F' ) )
		{
			cameras->Translate( { 0.0f,-dt,0.0f } );
		}
	}

	while( const auto delta = wnd.mouse.ReadRawDelta() )
	{
		if( !wnd.CursorEnabled() )
		{
			cameras->Rotate( (float)delta->x,(float)delta->y );
		}
	}

	// Left-click picking when cursor is enabled
	while(const auto e = wnd.mouse.Read() )
	{
		if( e->GetType() == Mouse::Event::Type::LPress && wnd.CursorEnabled() )
		{
			// Don't pick if ImGui captured the mouse
			if (!ImGui::GetIO().WantCaptureMouse)
			{
				PerformPicking();
				//break; // Only handle one click per frame
			}
		}
	}
}

void App::DoFrame(float dt)
{
	wnd.Gfx().BeginFrame(0.07f, 0.0f, 0.12f);

	if (sceneType == SceneType::Default)
	{
		DoFrameDefault(dt);
	}
	else if (sceneType == SceneType::TileMap)
	{
		DoFrameTileMap(dt);
	}

	// present
	wnd.Gfx().EndFrame();
	GetRenderGraph().Reset();
}

Rgph::RenderGraph& App::GetRenderGraph() noexcept
{
	if (pBlurRg) return *pBlurRg;
	return *pUnlitRg;
}

void App::DoFrameDefault(float dt)
{
	pLight->Bind(wnd.Gfx(), cameras->GetMatrix());
	pBlurRg->BindMainCamera(cameras.GetActiveCamera());
		
	pLight->Submit( Chan::main );
	pCube->Submit( Chan::main );
	pSponza->Submit( Chan::main );
	pCube2->Submit( Chan::main );
	pGobber->Submit( Chan::main );
	cameras.Submit( Chan::main );

	if (pTriIndicator)
	{
		pTriIndicator->Submit(Chan::main);
	}

	if (dynamicModel)
	{
		dynamicModel->Submit(Chan::main);
	}

	for (const auto& overlay : texturedOverlays)
	{
		overlay->Submit(Chan::main);
	}


	pSponza->Submit( Chan::shadow );
	pCube->Submit( Chan::shadow );
	pSponza->Submit( Chan::shadow );
	pCube2->Submit( Chan::shadow );
	pGobber->Submit( Chan::shadow );
	//pNano->Submit( Chan::shadow );

	pBlurRg->Execute( wnd.Gfx() );

	if( savingDepth )
	{
		pBlurRg->DumpShadowMap( wnd.Gfx(),"shadow.png" );
		savingDepth = false;
	}



	if (showImguiDebugWindows)
	{
		static MP sponzeProbe{ "Sponza" };
		static MP gobberProbe{ "Gobber" };
		static MP userMeshProbe{ "UserMesh" };
		sponzeProbe.SpawnWindow(*pSponza);
		gobberProbe.SpawnWindow(*pGobber);
		//nanoProbe.SpawnWindow(nano);

		if (dynamicModel)
		{
			userMeshProbe.SpawnWindow(*dynamicModel);
		}
		pLight->SpawnControlWindow();
	}

	{
		ImGui::Begin("Model Loader");
		static char pathBuf[MAX_PATH] = "";
		ImGui::InputText("Path", &pathBuf[0], MAX_PATH);
		ImGui::InputFloat("Scale", &dynamicModelScale, 0.1f, 1.0f, "%.3f");

		if (ImGui::Button("Browse..."))
		{
			const auto sel = OpenModelFileDialog();
			if (!sel.empty())
			{
				strncpy_s(pathBuf, sel.c_str(), MAX_PATH);
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Load Model"))
		{
			try
			{
				dynamicModel = std::make_unique<Model>(wnd.Gfx(), std::string(pathBuf), dynamicModelScale);
				dynamicModelLoadError.clear();

				dynamicModel->SetRootTransform(
					dx::XMMatrixRotationY(PI / 2.f) *
					dx::XMMatrixTranslation(27.f, -0.56f, 1.7f)
				);

				dynamicModel->LinkTechniques(*pBlurRg);
			}
			catch (const std::exception& e)
			{
				dynamicModelLoadError = e.what();
			}
		}

		if (!dynamicModelLoadError.empty())
		{
			ImGui::TextColored(ImVec4(1, 0, 0, 1), "Load error: %s", dynamicModelLoadError.c_str());
		}
		ImGui::End();
	}

	ShowPickingWindow();

	uvEditor.Show(wnd.Gfx(), pPickedMesh, pickedFaceIndex, [this](Mesh* pMesh, size_t faceIdx, const std::string& texPath)
		{
			RebuildTexturedOverlays();
		}
	);
	ShowExportWindow();
}

void App::DoFrameTileMap(float dt)
{
	pUnlitRg->BindMainCamera(cameras.GetActiveCamera());

	const size_t submittedTiles = pTileScene->Submit(Chan::main);
	cameras.Submit(Chan::main);


	// Submit triangle indicator if picking is active
	if (pTriIndicator)
	{
		pTriIndicator->Submit(Chan::main);
	}

	// Submit textured overlays for picked face
	for (const auto& overlay : texturedOverlays)
	{
		overlay->Submit(Chan::main);
	}

	pUnlitRg->Execute(wnd.Gfx());

	// Debug stats overlay (top-left corner, semi-transparent)
	{
		ImGui::SetNextWindowPos({ 10.0f, 10.0f }, ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowBgAlpha(0.6f);
		ImGui::Begin("##DebugStats", nullptr,
			ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize |
			ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);

		ImGui::TextColored({ 0.0f, 1.0f, 0.0f, 1.0f }, "FPS: %.1f (%.2f ms)",
			1.0f / dt, dt * 1000.0f);
		/*ImGui::TextColored({ 1.0f, 1.0f, 0.0f, 1.0f }, "Rendered tiles: %zu / %zu",
			submittedTiles, pTileScene->GetMapDef().tiles.size());

		float drawDistance = pTileScene->GetDrawDistance();
		if (ImGui::SliderFloat("Draw Distance", &drawDistance, 0.0f, 500.0f, "%.0f"))
		{
			pTileScene->SetDrawDistance(drawDistance);
		}
		if (drawDistance == 0.0f)
		{
			ImGui::SameLine();
			ImGui::TextDisabled({ " (unlimited)" });
		}*/

		ImGui::TextColored({ 1.0f, 1.0f, 0.0f, 1.0f }, "Tiles %zu | Draw calls: %zu",
			submittedTiles, pTileScene->GetBatchCount());
		ImGui::End();
	}


	ShowTileMapWindow();
	ShowPickingWindow();

	uvEditor.Show(wnd.Gfx(), pPickedMesh, pickedFaceIndex, [this](Mesh* pMesh, size_t faceIdx, const std::string& texPath)
		{
			RebuildTexturedOverlays();
		}
	);

	ShowExportWindow();
}

void App::ShowTileMapWindow()
{
	ImGui::Begin("Tile Map Scene");

	ImGui::TextColored({ 0.4f, 1.0f, 0.6f, 1.0f }, "Tile Grid: %zu tiles",
		pTileScene->GetMapDef().tiles.size());

	ImGui::Separator();
	ImGui::TextColored({ 0.4f, 1.0f, 0.6f, 1.0f }, "Load Tile Map from JSON");

	static char tileMapPath[MAX_PATH] = "";
	ImGui::InputText("Map File", tileMapPath, MAX_PATH);
	if (ImGui::Button("Browse Map..."))
	{
		std::array<char, MAX_PATH> buf{};
		OPENFILENAMEA ofn{};
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = nullptr;
		ofn.lpstrFile = buf.data();
		ofn.nMaxFile = (DWORD)buf.size();
		ofn.lpstrFilter = "JSON Files\0*.json\0All Files\0*.*\0";
		ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
		if (GetOpenFileNameA(&ofn))
		{
			strncpy_s(tileMapPath, buf.data(), MAX_PATH);
		}
	}
	ImGui::SameLine();
	if(ImGui::Button("Load Map"))
	{
		try
		{
			auto def = TileMapDef::LoadFromJSON(tileMapPath);
			pTileScene = std::make_unique<TileMapScene>(wnd.Gfx(), def);
			pTileScene->LinkTechniques(*pUnlitRg);
		}
		catch (const std::exception& e)
		{
			tileModelLoadError = std::string("Map load error: ") + e.what();
		}
	}

	ImGui::Separator();
	ImGui::TextColored({ 0.4f, 1.0f, 0.6f, 1.0f }, "Load 3D Object (Unlit)");

	static char modelPath[MAX_PATH] = ""; 
	ImGui::InputText("Model Path", modelPath, MAX_PATH);
	ImGui::InputFloat("Scale", &tileModelScale, 0.1f, 1.0f, "%.3f");

	if (ImGui::Button("Browse Model..."))
	{
		const auto sel = OpenModelFileDialog();
		if(!sel.empty())
		{
			strncpy_s(modelPath, sel.c_str(), MAX_PATH);
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("Load 3D Object"))
	{
		try
		{
			pTileScene->LoadDynamicModel(wnd.Gfx(), std::string(modelPath), tileModelScale);
			pTileScene->SetDynamicModelTransform(
				dx::XMMatrixTranslation(8.f, 0.f, 8.f)
			);
			//pTileScene->LinkTechniques(*pUnlitRg);
			//Link only the newly loaded model, not the entire scene
			pTileScene->GetDynamicModel()->LinkTechniques(*pUnlitRg);
			tileModelLoadError.clear();
		}
		catch (const std::exception& e)
		{
			tileModelLoadError = e.what();
		}
	}

	if (!tileModelLoadError.empty())
	{
		ImGui::TextColored(ImVec4(1, 0, 0, 1), "%s", tileModelLoadError.c_str());
	}

	ImGui::End();
}

void App::ShowImguiDemoWindow()
{
	if( showDemoWindow )
	{
		ImGui::ShowDemoWindow( &showDemoWindow );
	}
}

void App::PerformPicking()
{
	const auto [mouseX, mouseY] = wnd.mouse.GetPos();
	const int vpWidth = (int)wnd.Gfx().GetWidth();
	const int vpHeight = (int)wnd.Gfx().GetHeight();

	const auto viewMatrix = cameras->GetMatrix();
	const auto projMatrix = wnd.Gfx().GetProjection();

	auto [rayOrigin, rayDir] = Picking::ScreenToRay(mouseX, mouseY, vpWidth, vpHeight, projMatrix, viewMatrix);

	// Disable outline on previously selected mesh
	//if( pPrevOutlinedMesh)
	//{
	//	for (auto& tech : pPrevOutlinedMesh->GetTechniques())
	//	{
	//		if( tech.GetName() == "Outline")
	//		{
	//			tech.SetActiveState(false);
	//		}
	//	}
	//	pPrevOutlinedMesh = nullptr;
	//}


	// Disable wireframe on previously selected mesh
	if (pPrevWireframeMesh)
	{
		for (auto& tech : pPrevWireframeMesh->GetTechniques())
		{
			if (tech.GetName() == "Wireframe")
			{
				tech.SetActiveState(false);
			}
		}
		pPrevWireframeMesh = nullptr;
	}
	showWireframe = false;

	// clear old triangle indicator
	pTriIndicator.reset();

	// Test all models
	pPickedMesh = nullptr;
	float bestDist = FLT_MAX;
	DirectX::XMFLOAT4X4 bestWorldTransform;

	auto testModel = [&](Model& model)
	{
		if (auto hit = model.Pick(rayOrigin, rayDir))
		{
			if (hit->distance < bestDist)
			{
				bestDist = hit->distance;
				pPickedMesh = hit->pMesh;
				pickedFaceIndex = hit->faceIndex;
				pickedDistance = hit->distance;
				bestWorldTransform = hit->worldTransform;
			}
		}
	};

	if(pSponza) testModel(*pSponza);
	if(pGobber) testModel(*pGobber);
	if(dynamicModel)
	{
		testModel(*dynamicModel);
	}
	if (pTileScene && pTileScene->GetDynamicModel())
	{
		testModel(*pTileScene->GetDynamicModel());
	}

	// Build single-triangle indicator for the picked face
	if (pPickedMesh)
	{
		const auto& indices = pPickedMesh->GetCpuIndices();
		const auto& positions = pPickedMesh->GetCpuPositions();
		const auto worldMat = DirectX::XMLoadFloat4x4(&bestWorldTransform);

		// Transform local-space triangle vertices to world space
		DirectX::XMFLOAT3 wv0, wv1, wv2;
		DirectX::XMStoreFloat3(&wv0, DirectX::XMVector3TransformCoord(DirectX::XMLoadFloat3(&positions[indices[pickedFaceIndex * 3 + 0]]), worldMat));
		DirectX::XMStoreFloat3(&wv1, DirectX::XMVector3TransformCoord(DirectX::XMLoadFloat3(&positions[indices[pickedFaceIndex * 3 + 1]]), worldMat));
		DirectX::XMStoreFloat3(&wv2, DirectX::XMVector3TransformCoord(DirectX::XMLoadFloat3(&positions[indices[pickedFaceIndex * 3 + 2]]), worldMat));


		pTriIndicator = std::make_unique<TriangleIndicator>(wnd.Gfx(), wv0, wv1, wv2);
		pTriIndicator->LinkTechniques(GetRenderGraph());
		pickedWorldTransform = bestWorldTransform;
	}
}

void App::ShowPickingWindow()
{
	ImGui::Begin("Mesh Picker");

	if (pPickedMesh == nullptr)
	{
		ImGui::TextColored( {0.7f,0.7f, 0.7f, 1.0f}, "Left-click on a mesh to select it");
		ImGui::Text("(cursor must be enabled)");
	}
	else
	{
		//const size_t totalFaces = pPickedMesh->GetCpuIndices().size() / 3;
		//const size_t totalVertices = pPickedMesh->GetCpuPositions().size();

		ImGui::TextColored({ 0.4f,1.0f, 0.6f, 1.0f }, "Selected Mesh");
		ImGui::Text("Face index %zu", pickedFaceIndex);
		ImGui::Text("Distance %.2f", pickedDistance);
		//ImGui::Text("Total faces", pPickedMesh->GetCpuIndices().size);
		//ImGui::Text("Total vertices %zu", pPickedMesh->getCpu);


		ImGui::Separator();
		//Wireframe toggle
		{
			if (ImGui::Checkbox("Show Wireframe", &showWireframe))
			{
				//Disable old wireframe mesh if different
				if (pPrevWireframeMesh && pPrevWireframeMesh != pPickedMesh)
				{
					for (auto& tech : pPrevWireframeMesh->GetTechniques())
					{
						if (tech.GetName() == "Wireframe")
						{
							tech.SetActiveState(false);
						}
					}
				}

				for (auto& tech : pPickedMesh->GetTechniques())
				{
					if (tech.GetName() == "Wireframe")
					{
						tech.SetActiveState(showWireframe);
					}
				}
				pPrevWireframeMesh = showWireframe ? pPickedMesh : nullptr;
			}
		}

		ImGui::Separator();
		ImGui::TextColored({ 0.4f,1.0f, 0.6f, 1.0f }, "Textures");

		// Find and display current textures for the Phon technique
		for (auto& tech : pPickedMesh->GetTechniques())
		{

			if (tech.GetName() != "Phong")
			{
				continue;
			}

			for (auto& step : tech.GetSteps())
			{
				for (size_t i = 0; i < step.GetBindables().size(); i++)
				{
					auto& bindable = step.GetBindables()[i];
					if (auto* pTex = dynamic_cast<Bind::Texture*>(bindable.get()))
					{
						const char* slotNames[] = { "Diffuse", "Specular", "Normal" };
						UINT slot = pTex->GetSlot();
						const char* slotName = (slot < 3) ? slotNames[slot] : "Unknown";

						ImGui::PushID((int)i);
						ImGui::Text("%s: %s", slotName, pTex->GetPath().c_str());

						std::string btnLabel = std::string("Change") + slotName + "...";
						if (ImGui::Button(btnLabel.c_str()))
						{
							const auto newPath = OpenTextureFileDialog();
							if (!newPath.empty())
							{
								// Replace the texture bindable with a new one
								auto nextTex = std::make_shared<Bind::Texture>(wnd.Gfx(), newPath, slot);
								step.GetBindables()[i] = std::move(nextTex)	;
							}
						}
						ImGui::PopID();
					}
				}
			}
		}
	}

	ImGui::Separator();
	if (ImGui::Button("Deselect"))
	{
		if (pPrevWireframeMesh)
		{
			for (auto& tech : pPrevWireframeMesh->GetTechniques())
			{
				if (tech.GetName() == "Wireframe")
				{
					tech.SetActiveState(false);
				}
			}
			pPrevWireframeMesh = nullptr;
		}
		showWireframe = false;
		pPickedMesh = nullptr;
		pTriIndicator.reset();
	}
	ImGui::End();

	//if (pPickedMesh)
	//{
	//	ImGui::Text("Picked Mesh: %p", pPickedMesh);
	//	ImGui::Text("Face Index: %zu", pickedFaceIndex);
	//	ImGui::Text("Distance: %.3f", pickedDistance);
	//}
	//else
	//{
	//	ImGui::Text("No mesh picked");
	//}
	//ImGui::End();
}


void App::RebuildTexturedOverlays()
{
	texturedOverlays.clear();

	// Helper to process one model's meshes
	auto processModel = [this](Model& model)
		{
			// We need node traversal to get world transforms. Use a recursive lambda.
			struct NodeTraverser
			{
				App* app;
				void Traverse(const Node& node, DirectX::FXMMATRIX accum) const
				{
					const auto built =
						DirectX::XMLoadFloat4x4(&node.GetAppliedTransform()) *
						DirectX::XMLoadFloat4x4(&node.GetBaseTransform()) *
						accum;
					for (const auto* pMesh : node.GetMeshPtrs())
					{
						const auto& overrides = pMesh->GetFaceTextureOverrides();
						if(overrides.empty() )
							continue;

						const auto& indices = pMesh->GetCpuIndices();
						const auto& positions = pMesh->GetCpuPositions();
						const auto& uvs = pMesh->GetCpuUVs();

						for (const auto& [faceIdx, texPath] : overrides)
						{
							if (faceIdx * 3 + 2 >= indices.size() || uvs.empty())
								continue;
							
							const size_t i0 = indices[faceIdx * 3 + 0];
							const size_t i1 = indices[faceIdx * 3 + 1];
							const size_t i2 = indices[faceIdx * 3 + 2];

							DirectX::XMFLOAT3 wv0, wv1, wv2;
							DirectX::XMStoreFloat3(&wv0, DirectX::XMVector3TransformCoord(DirectX::XMLoadFloat3(&positions[i0]), built));
							DirectX::XMStoreFloat3(&wv1, DirectX::XMVector3TransformCoord(DirectX::XMLoadFloat3(&positions[i1]), built));
							DirectX::XMStoreFloat3(&wv2, DirectX::XMVector3TransformCoord(DirectX::XMLoadFloat3(&positions[i2]), built));

							DirectX::XMFLOAT2 uv0 = uvs[i0];
							DirectX::XMFLOAT2 uv1 = uvs[i1];
							DirectX::XMFLOAT2 uv2 = uvs[i2];

							auto overlay = std::make_unique<TexturedTriangleOverlay>(
								app->wnd.Gfx(), wv0, wv1, wv2, uv0, uv1, uv2, texPath
							);
							overlay->LinkTechniques(app->GetRenderGraph());
							app->texturedOverlays.push_back(std::move(overlay));
						}
					}
					for (const auto& child : node.GetChildren())
					{
						Traverse(*child, built);
					}
				}
			};

			NodeTraverser traverser{ this };
			traverser.Traverse(model.GetRootNode(), DirectX::XMMatrixIdentity());
		};

	if(pSponza) processModel(*pSponza);
	if(pGobber) processModel(*pGobber);
	if(dynamicModel) processModel(*dynamicModel);

	if (pTileScene && pTileScene->HasDynamicModel())
	{
		processModel(*pTileScene->GetDynamicModel());
	}
}

void App::ShowExportWindow()
{
	ImGui::Begin("Export");

	static char exportPath[MAX_PATH] = "exported_model.obj";
	ImGui::InputText("Output Path", exportPath, MAX_PATH);

	if (ImGui::Button("Browse..."))
	{
		std::array<char, MAX_PATH> buf{};
		strncpy_s(buf.data(), buf.size(), exportPath, _TRUNCATE);
		OPENFILENAMEA ofn{};
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = nullptr;
		ofn.lpstrFile = buf.data();
		ofn.nMaxFile = (DWORD)buf.size();
		ofn.lpstrFilter = "OBJ Files\0*.obj\0All Files\0*.*\0";
		ofn.Flags = OFN_OVERWRITEPROMPT;
		ofn.lpstrDefExt = "obj";
		if(GetSaveFileNameA(&ofn))
		{
			strncpy_s(exportPath, buf.data(), MAX_PATH);
		}
	}


	Model* pExportModel = nullptr;
	if (dynamicModel)
		pExportModel = dynamicModel.get();
	else if (pTileScene && pTileScene->HasDynamicModel())
		pExportModel = pTileScene->GetDynamicModel();

	if (pExportModel)
	{

		ImGui::SameLine();
		if (ImGui::Button("Export Dynamic"))
		{
			exportError.clear();
			if (!ObjExporter::Export(*pExportModel, exportPath, exportError))
			{
				// error stored
			}
			else
			{
				exportError = "OK: Exported to " + std::string("exportPath");
			}
		}
	}
	if (!exportError.empty())
	{
		if (exportError.substr(0, 3) == "OK:")
		{
			ImGui::TextColored({ 0.4f, 1.0f, 0.6f, 1.0f }, "%s", exportError.c_str());
		}
		else
		{
			ImGui::TextColored({ 1.0f, 0.4f, 0.4f, 1.0f }, "Error %s", exportError.c_str());
		}

	}
	ImGui::End();
}


App::~App()
{}

int App::Go()
{
	while( true )
	{
		// process all messages pending, but to not block for new messages
		if( const auto ecode = Window::ProcessMessages() )
		{
			// if return optional has value, means we're quitting so return exit code
			return *ecode;
		}
		// execute the game logic
		const auto dt = timer.Mark() * speed_factor;
		HandleInput( dt );
		DoFrame( dt );
	}
}