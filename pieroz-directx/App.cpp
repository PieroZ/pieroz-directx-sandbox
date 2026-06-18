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
#include "NormalsIndicator.h"
#include "Node.h"
#include "TileMapDef.h"
#include "iamLoader.h"
#include "iamToJson.h"
#include "primLoader.h"
#include "PrimConverter.h"
#include "WallBatch.h"
#include "tmaLoader.h"
#include "SkyboxPass.h"
#include "ConstantBuffersEx.h"
#include "DynamicConstant.h"
#include "PrimLightDef.h"

#include <commdlg.h> // GetOpenFileName
#include <array>
#include <cmath>
#include <cctype>
#include <fstream>
#include "json.hpp"

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
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
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


		// Point light so lit rendeer modes ColorLit have valid lightdata
		pLight = std::make_unique<PointLight>(wnd.Gfx(), dx::XMFLOAT3{ 5.0f, 15.0f, -5.0f });

		cameras.AddCamera(std::make_unique<Camera>(wnd.Gfx(), "TileCamera", 
			dx::XMFLOAT3{ 5.0f, 10.0f, -5.0f }, PI / 4.0f, PI / 4.0f ));

		// Create a default 8x8 tile grid with a placerholder texture
		auto def = TileMapDef::MakeGrid(8, 8, 2.0f, "Images\\brickwall.jpg");
		pTileScene = std::make_unique<TileMapScene>(wnd.Gfx(), def);
		pTileScene->LinkTechniques(*pUnlitRg);
		cameras.LinkTechniques(*pUnlitRg);

		// Light is a visible, movable scene objects that lights the tiles
		pLight->LinkTechniques(*pUnlitRg);
		pLight->SetPos(lightAnimCenter);

		// Emissive prim lights: GPU buffer (PS register b3) + persisted defitnions
		pEmissiveLightsCbuf = std::make_unique<Bind::PixelConstantBuffer<EmissiveLightsCBuf>>(wnd.Gfx(), 3u);
		try { primLightRegistry.Load(primLightRegistryPath); }
		catch (const std::exception&) { /*start with a nempty registry*/ }
		RebuildEmissiveLights();

		// Apply the initial global render mode to all tile drawables
		ApplyGlobalRenderMode();
	}

	LoadWindowSettings();
	strncpy_s(tileMapPath, tileMapPathString.c_str(), MAX_PATH);
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
		if (wnd.kbd.KeyIsPressed(VK_SHIFT))
		{
			dt *= 5.0f;
		}

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
				if (pickingEmitPoint)
				{
					PlaceEmitPointAtCursor();
				}
				else
				{
					PerformPicking();
				}
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
	
	if (pNormalsIndicator)
	{
		pNormalsIndicator->Submit(Chan::main);
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

	if (showModelLoaderWindow)
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


	ShowWindowControlPanel();
	if(showPickingWindow)
		ShowPickingWindow();

	if (showUvEditorWindow)
	{
		uvEditor.Show(wnd.Gfx(), pPickedMesh, pickedFaceIndex, [this](Mesh* pMesh, size_t faceIdx, const std::string& texPath)
			{
				RebuildTexturedOverlays();
			}
		);
	}
	if (showExportWindow)
	{
		ShowExportWindow();
	}
}

void App::DoFrameTileMap(float dt)
{
	
	// Bind the point light cbuffer(register b0) so lit techniques like ColorLit receive valid light color/intensity data
	if (pLight)
	{
		if (animateLight)
		{
			lightAnimTime += dt;
		}
		const float offset = lightAnimAmplitude * std::sin(lightAnimTime * lightAnimSpeed);
		dx::XMFLOAT3 lampPos = lightAnimCenter;
		if (lightAnimAxis == 0)
		{
			lampPos.x += offset;
		}
		else
		{
			lampPos.z += offset;
		}
		pLight->SetPos(lampPos);

		//const float fyaw = flashlightYawOffset * (PI / 180.0f);
		//const float fpitch = flashlightPitchOffset * (PI / 180.0f);
		//const dx::XMFLOAT3 spotDir = {
		//	std::sin(fyaw) * std::cos(fpitch),
		//	std::sin(fpitch),
		//	std::cos(fyaw) * std::cos(fpitch)
		//};

		dx::XMFLOAT3 spotDir;
		if (flashlightFollowMouse)
		{
			const auto [mouseX, mouseY] = wnd.mouse.GetPos();
			const float vpWidth = (float)wnd.Gfx().GetWidth();
			const float vpHeight = (float)wnd.Gfx().GetHeight();
			const float ndcX = 2.0f * (float)mouseX / vpWidth - 1.0f;
			const float ndcY = 1.0f - 2.0f * (float)mouseY / vpHeight;
			dx::XMFLOAT4X4  proj;
			dx::XMStoreFloat4x4(&proj, cameras->GetProjection());
			const float vx = (proj._11 != 0.0f) ? ndcX / proj._11 : 0.0f;
			const float vy = (proj._22 != 0.0f) ? ndcY / proj._22 : 0.0f;
			dx::XMStoreFloat3(&spotDir,
				dx::XMVector3Normalize(dx::XMVectorSet(vx, vy, 1.0f, 0.0f)));
		
		}
		else
		{
			const float fyaw = flashlightYawOffset * (PI / 180.0f);
			const float fpitch = flashlightPitchOffset * (PI / 180.0f);
			const dx::XMFLOAT3 spotDir = {
				std::sin(fyaw) * std::cos(fpitch),
				std::sin(fpitch),
				std::cos(fyaw) * std::cos(fpitch)
			};
		}
		pLight->SetFlashLight(
			flashlightEnabled,
			flashlightPosOffset,
			spotDir,
			dx::XMFLOAT3{ flashlightColor[0],flashlightColor[1] ,flashlightColor[2] },
			flashlightInnerDeg, flashlightOuterDeg,
			flashlightRange, flashlightIntensity);

		pLight->Bind(wnd.Gfx(), cameras->GetMatrix());
	}

	// Upload emissive prim lights9lamps ) in the view space for the lit techniques
	BindEmissiveLights(cameras->GetMatrix());

	pUnlitRg->BindMainCamera(cameras.GetActiveCamera());

	const size_t submittedTiles = pTileScene->Submit(Chan::main);
	cameras.Submit(Chan::main);

	if (pLight)
	{
		pLight->Submit(Chan::main);
	}

	// Submit permanently placed prims
	for (auto& group : primPlaced)
	{
		for (auto& pd : group)
		{
			pd->Submit(Chan::main);
		}
	}

	// Submit wall geometry
	for (auto& wb : wallBatches)
	{
		if (wb->GetWallCount() > 0)
		{
			wb->Submit(Chan::main);
		}
	}


	// Update and submit prim preview (follows cursor)
	if (!primPreview.empty())
	{
		// Project mouse cursor onto Y=0 ground plane
		const auto [mouseX, mouseY] = wnd.mouse.GetPos();
		const int vpWidth = (int)wnd.Gfx().GetWidth();
		const int vpHeight = (int)wnd.Gfx().GetHeight();
		const auto viewMatrix = cameras->GetMatrix();
		const auto projMatrix = cameras->GetProjection();
		auto [rayOrigin, rayDir] = Picking::ScreenToRay(mouseX, mouseY, vpWidth, vpHeight, projMatrix, viewMatrix);

		// Intersect ray with Y=0 plane
		const float originY = dx::XMVectorGetY(rayOrigin);
		const float dirY = dx::XMVectorGetY(rayDir);
		if (std::abs(dirY) > 1e-6f) // Avoid division by zero
		{
			const float t = -originY / dirY;
			if (t > 0.0f) // Only consider intersections in front of the camera
			{
				const auto hitPoint = dx::XMVectorAdd(rayOrigin, dx::XMVectorScale(rayDir, t));
				for (auto& pd : primPreview)
				{
					pd->SetPosition(
						dx::XMVectorGetX(hitPoint),
						0.0f,
						dx::XMVectorGetZ(hitPoint));
				}
			}
		}

		// On left click, place the preview permanently
		if (wnd.mouse.LeftIsPressed())
		{
			primPlaced.push_back(std::move(primPreview));
			primPlacedIndices.push_back(primPreviewIndex);
			primPreview.clear();
			ApplyGlobalRenderMode();
			RebuildEmissiveLights();
		}
		else
		{
			for (auto& pd : primPreview)
			{
				pd->Submit(Chan::main);
			}
		}
	}



	// Submit triangle indicator if picking is active
	if (pTriIndicator)
	{
		pTriIndicator->Submit(Chan::main);
	}


	// Submit face-normals arrows if enabled
	if (pNormalsIndicator)
	{
		pNormalsIndicator->Submit(Chan::main);
	}

	// Submit textured overlays for picked face
	for (const auto& overlay : texturedOverlays)
	{
		overlay->Submit(Chan::main);
	}

	pUnlitRg->Execute(wnd.Gfx());

	//Skybox controls
	pUnlitRg->RenderSkyboxWindow();

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
		ImGui::Checkbox("Debug Overlay", &showDebugOverlay);
		ImGui::End();
	}


	// Scene light (animated lamp) controls
	{
		ImGui::Begin("Scene Light");

		int litMode = sceneLitMode ? 0 : 1;
		if (ImGui::Combo("Render Mode", &litMode, "Lit (lighting)\0Unlit (flat)\0"))
		{
			sceneLitMode = (litMode == 0);
			ApplyGlobalRenderMode();
		}
		ImGui::Separator();

		ImGui::Checkbox("Animate (oscillate)", &animateLight);
		ImGui::Combo("Sweep Axis", &lightAnimAxis, "X\0Z\0");
		ImGui::SliderFloat("Speed", &lightAnimSpeed, 0.0f, 3.0f, "%.2f");
		ImGui::SliderFloat("Amplitude", &lightAnimAmplitude, 0.0f, 16.0f, "%.1f");
		ImGui::DragFloat3("Center", &lightAnimCenter.x, 0.1f);
		if (pLight)
		{
			const auto p = pLight->GetPos();
			ImGui::Text("Light pos: %.1f, %.1f, %.1f", p.x, p.y, p.z);
		}

		ImGui::Separator();
		ImGui::TextColored({ 1.0f,0.9f,0.4f,1.0f }, "Flashlight (observer torch)");
		ImGui::Checkbox("Enabled##flash", &flashlightEnabled);
		ImGui::ColorEdit3("Color##flash", flashlightColor);
		ImGui::SliderFloat("Intensity##flash", &flashlightIntensity, 0.0f, 10.0f, "%.1f");
		ImGui::SliderFloat("Inner Angle##flash", &flashlightInnerDeg, 1.0f, 60.f, "%.0f deg");
		ImGui::SliderFloat("Outer Angle##flash", &flashlightOuterDeg, 1.0f, 80.f, "%.0f deg");
		ImGui::SliderFloat("Range##flash", &flashlightRange, 1.0f, 100.0f, "%.0f");
		ImGui::SliderFloat("Aim Yaw##flash", &flashlightYawOffset, -60.f, 60.f, "%.0f deg");
		ImGui::SliderFloat("Aim Pitch##flash", &flashlightPitchOffset, -60.f, 60.f, "%.0f deg");
		ImGui::DragFloat3("Origin Offset##flash", &flashlightPosOffset.x, 0.1f);
		ImGui::End();
	}

	ShowWindowControlPanel();

	if (showDebugOverlay)
	{
		DrawDebugOverlay();
	}

	// Always show ehere lamps emit light ( and the live empit-point while editing).
	DrawEmissiveLightOverlay();

	if (showNprimImportWindow)
	{
		ShowNprimImportWindow();
	}
	if (showTileMapWindow)
	{
		ShowTileMapWindow();
	}
	if (showPickingWindow)
	{
		ShowPickingWindow();
	}


	if (showUvEditorWindow)
	{
		uvEditor.Show(wnd.Gfx(), pPickedMesh, pickedFaceIndex, [this](Mesh* pMesh, size_t faceIdx, const std::string& texPath)
			{
				RebuildTexturedOverlays();
			}
		);
	}

	if (showExportWindow)
	{
		ShowExportWindow();
	}
}

void App::ShowTileMapWindow()
{
	ImGui::Begin("Tile Map Scene");

	ImGui::TextColored({ 0.4f, 1.0f, 0.6f, 1.0f }, "Tile Grid: %zu tiles",
		pTileScene->GetMapDef().tiles.size());

	ImGui::Separator();
	ImGui::TextColored({ 0.4f, 1.0f, 0.6f, 1.0f }, "Load Tile Map from .iam");

	ImGui::InputText("Map File", tileMapPath, MAX_PATH);
	if (ImGui::Button("Browse Map..."))
	{
		std::array<char, MAX_PATH> buf{};
		OPENFILENAMEA ofn{};
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = nullptr;
		ofn.lpstrFile = buf.data();
		ofn.nMaxFile = (DWORD)buf.size();
		ofn.lpstrFilter = "iam Files\0*.iam\0All Files\0*.*\0";
		ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
		if (GetOpenFileNameA(&ofn))
		{
			strncpy_s(tileMapPath, buf.data(), MAX_PATH);
		}
	}
	ImGui::SameLine();
	//if(ImGui::Button("Load Json"))

	if(ImGui::Button("Load Map"))
	{
		try
		{
			auto iamResult = LoadIamMap(tileMapPath);

			tileMapPathString = tileMapPath;
			auto mapjson = BuildMapJson(iamResult);


			// TODO: Investigate SLONG texture_quad(POLY_Point *quad[4],SLONG texture_style,SLONG pos,SLONG count,SLONG flipx=0)
			tma tma_result = load_texture_styles(0, iamResult.texture_set);

			std::filesystem::path path(tileMapPath);

			std::string jsonMapFilename = path.stem().string() + ".json";

			SaveIamToJson(mapjson, jsonMapFilename);
			auto def = TileMapDef::LoadFromJSON(jsonMapFilename);
			pTileScene = std::make_unique<TileMapScene>(wnd.Gfx(), def);
			pTileScene->LinkTechniques(*pUnlitRg);

			// Build wall geometry from DFacets
			if (iamResult.next_dfacet > 1)
			{
				wallBatches = WallBatch::CreateBatches(
					wnd.Gfx(),
					iamResult.dfacets,
					iamResult.dstyles,
					iamResult.dstoreys,
					tma_result,
					iamResult.texture_set,
					1.0f, // gridScale: 1 world unit per grid cell
					1.0f / 8.0f // yScale: convert Y/Height to world units
				);
				for(auto& wb: wallBatches)
					wb->LinkTechniques(*pUnlitRg);
			}

			// Load and place prim objects from map
			primPlaced.clear();
			primPlacedIndices.clear();
			for (const auto& primDef : def.prims)
			{
				try
				{
					auto [nprimPath, primPath] = GetPrimFilePaths(primDef.primIndex);
					auto primResult = LoadPrimObject(nprimPath, primPath);
					auto texturedLists = ConvertPrimToTexturedTriangleList(primResult);

					std::vector<std::unique_ptr<PrimDrawable>> group;
					for (auto& [texImgNo, triList] : texturedLists)
					{
						std::string texPath = GetPrimTexturePath(texImgNo);
						auto pd = std::make_unique<PrimDrawable>(wnd.Gfx(), std::move(triList), texPath);
						pd->LinkTechniques(*pUnlitRg);

						// Position: x and z are grid coords, y is height
						float worldX = static_cast<float>(primDef.x) + primDef.xOffset;// -primDef.zOffset;
						float worldY = primDef.y;
						//float worldY = 0;
						float worldZ = static_cast<float>(primDef.z) + primDef.zOffset;// -primDef.xOffset;
						//float radians = to_rad<float>((float)primDef.yaw);
						//float radians = static_cast<float>(primDef.yaw) * (2.0 * PI / 2048.0F);
						float degrees = static_cast<float>(primDef.yaw) * 360.0f / 256.0f;
						float radians = -static_cast<float>(primDef.yaw) * 2.0f * PI / 256.0f;

						pd->SetPosition(worldZ, worldY, worldX);
						pd->SetYaw(radians);

						group.push_back(std::move(pd));
					}
					if (!group.empty())
					{
						primPlaced.push_back(std::move(group));
						primPlacedIndices.push_back(primDef.primIndex);
					}
				}
				catch (const std::exception&)
				{
					// Skipp prims that fail to load
				}
			}

			ApplyGlobalRenderMode();
			RebuildEmissiveLights();
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


	// Reset render mode on previously selected mesh
	if (pPrevRenderModeMesh)
	{
		for (auto& tech : pPrevRenderModeMesh->GetTechniques())
		{
			if (tech.GetName() == "Phong") tech.SetActiveState(false);
			else if (tech.GetName() == "Unlit") tech.SetActiveState(false);
			else if (tech.GetName() == "ColorLit") tech.SetActiveState(false);
			else if (tech.GetName() == "Wireframe") tech.SetActiveState(false);
		}
		pPrevRenderModeMesh = nullptr;
	}
	selectedRenderMode = 0;

	// Deselect previously selected prim
	if (pPrevSelectedPrimGroupIdx >= 0 && pPrevSelectedPrimGroupIdx< (int)primPlaced.size())
	{
		for (auto& pd : primPlaced[pPrevSelectedPrimGroupIdx])
		{
			for (auto& tech : pd->GetTechniques())
			{
				if (tech.GetName() == "Selection")
					tech.SetActiveState(false);
				// Restore the global render mode
				if (tech.GetName() == "PrimUnlit") tech.SetActiveState(!sceneLitMode);
				else if (tech.GetName() == "ColorLit") tech.SetActiveState(sceneLitMode);
				else if (tech.GetName() == "Wireframe") tech.SetActiveState(false);
			}
		}
	}
	pPrevSelectedPrim = nullptr;
	pPrevSelectedPrimGroupIdx = -1;
	pPickedPrim = nullptr;
	pickedPrimGroupIdx = -1;
	pickedPrimIdx = -1;
	selectedPrimRenderMode = 0;

	// clear old triangle indicator
	pTriIndicator.reset();
	// clear old face-normal arrows (rebuilt below if the option is enabled)
	pNormalsIndicator.reset();

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

	// Test prim objects
	for (int gi = 0; gi < (int)primPlaced.size(); gi++)
	{
		for (int pi = 0; pi < (int)primPlaced[gi].size(); pi++)
		{
			auto& pd = primPlaced[gi][pi];
			if (auto hit = pd->Intersect(rayOrigin, rayDir))
			{
				if (hit->second < bestDist)
				{
					bestDist = hit->second;
					pPickedPrim = pd.get();
					pickedPrimGroupIdx = gi;
					pickedPrimIdx = pi;
					pPickedMesh = nullptr; // prim pick takes priority
				}
			}
		}
	}

		
	// Test tile and wall batches for quad measurement
	if (!pPickedPrim)
	{
		pickedQuadMeasurement.reset();
		if (pTileScene)
		{
			// Test tile batches
			for (const auto& batch : pTileScene->GetBatches())
			{
				if (auto hit = batch->PickQuad(rayOrigin, rayDir))
				{
					if (hit->second < bestDist)
					{
						bestDist = hit->second;
						pickedQuadMeasurement = hit->first;
						pPickedMesh = nullptr;
					}
				}
			}
		}
	}

	// Test wall batches
	for (const auto& wb : wallBatches)
	{
		if (wb->GetWallCount() > 0)
		{
			if (auto hit = wb->PickQuad(rayOrigin, rayDir))
			{
				if (hit->second < bestDist)
				{
					bestDist = hit->second;
					pickedQuadMeasurement = hit->first;
					pPickedMesh = nullptr; // quad pick takes priority
				}
			}
		}
	}

	//// Enable selection wireframe on picked prim
	//if (pPickedPrim)
	//{
	//	for (auto& tech : pPickedPrim->GetTechniques())
	//	{
	//		if (tech.GetName() == "Selection")
	//		{
	//			tech.SetActiveState(true);
	//		}
	//	}
	//	pPrevSelectedPrim = pPickedPrim;
	//}

	if (pickedPrimGroupIdx >= 0)
	{
		for (auto& pd : primPlaced[pickedPrimGroupIdx])
		{
			for (auto& tech : pd->GetTechniques())
			{
				if (tech.GetName() == "Wireframe")
					tech.SetActiveState(true);
			}
		}
		pPrevSelectedPrim = pPickedPrim;
		pPrevSelectedPrimGroupIdx = pickedPrimGroupIdx;
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

	// Rebuild face-normal arrows for the newly selected object (if enabled)
	if (showFaceNormals)
	{
		RebuildNormalsIndicator();
	}
}

void App::ShowPickingWindow()
{
	ImGui::Begin("Mesh Picker");

	if (pPickedMesh == nullptr && pPickedPrim == nullptr && !pickedQuadMeasurement.has_value())
	{
		ImGui::TextColored( {0.7f,0.7f, 0.7f, 1.0f}, "Left-click on a mesh to select it");
		ImGui::Text("(cursor must be enabled)");
	}
	else if (pPickedPrim!=nullptr)
	{
		ImGui::TextColored({ 1.0f, 1.0f, 0.3, 1.0f }, "Selected Prim Object");
		const auto pos = pPickedPrim->GetPosition();
		ImGui::Text("Position: (%.3f, %.3f, %.3f)", pos.x, pos.y, pos.z);
		ImGui::Text("Yaw: %.2f degrees", pPickedPrim->GetYaw() * 180.0f / 3.14159f);
		ImGui::Text("Group %d, Index: %d", pickedPrimGroupIdx, pickedPrimIdx);

		ImGui::Separator();
		ImGui::TextColored({ 0.6f, 0.8f, 1.0f, 1.0f }, "Render Mode");
		{
			//const char* renderModes[] = { "PrimUnlit", "ColorLit", "Wireframe" };
			//if (ImGui::Combo("Mode", &selectedPrimRenderMode, renderModes, IM_ARRAYSIZE(renderModes)))
			//{

			//	if (pickedPrimGroupIdx >= 0 && pickedPrimGroupIdx < (int)primPlaced.size())
			//	{
			//		for (auto& pd : primPlaced[pickedPrimGroupIdx])
			//		{
			//			for (auto& tech : pd->GetTechniques())
			//			{
			//				if (tech.GetName() == "PrimUnlit") tech.SetActiveState(selectedPrimRenderMode == 0);
			//				else if (tech.GetName() == "ColorLit") tech.SetActiveState(selectedPrimRenderMode == 1);
			//				else if (tech.GetName() == "Wireframe") tech.SetActiveState(selectedPrimRenderMode == 2);
			//			}
			//		}
			//	}
			//}

			// Lit/Unlit is driven globally
			ImGui::TextDisabled("Lit/Unlit controlled globally (Scene Light: %s)", sceneLitMode ? "Lit" : "Unlit");

			// Color Lit settings for prim
			if (sceneLitMode)
			{
				ImGui::TextColored({ 0.6f, 0.8f, 1.0f, 1.0f }, "ColorLit Settings");
				static float primLightTint[3] = { 0.3f, 0.8f, 1.0f };
				static float primLightIntensity = 1.0f;
				static float primMatColor[3] = { 1.0f, 1.0f, 1.0f }; 
				bool changed = false;
				changed |= ImGui::ColorEdit3("Light Tint", primLightTint);
				changed |= ImGui::SliderFloat("Intensity", &primLightIntensity, 0.0f, 10.0f);
				changed |= ImGui::ColorEdit3("Material Color", primMatColor);

				if (changed && pickedPrimGroupIdx >= 0 && pickedPrimGroupIdx < (int)primPlaced.size())
				{
					// Update the ColorLit technique's constant buffer with new values
					for (auto& pd : primPlaced[pickedPrimGroupIdx])
					{
						for (auto& tech : pd->GetTechniques())
						{
							if (tech.GetName() != "ColorLit") continue;
							for (auto& step : tech.GetSteps())
							{
								for (auto& bindable : step.GetBindables())
								{
									if (auto* pCbuf = dynamic_cast<Bind::CachingPixelConstantBufferEx*>(bindable.get()))
									{
										auto buf = pCbuf->GetBuffer();
										buf["materialColor"] = DirectX::XMFLOAT3(primMatColor[0], primMatColor[1], primMatColor[2]);
										buf["lightTint"] = DirectX::XMFLOAT3(primLightTint[0], primLightTint[1], primLightTint[2]);
										buf["lightIntensity"] = primLightIntensity;
										pCbuf->SetBuffer(buf);
										break;
									}
								}
							}
						}
					}
				}
			}
		}

		// Emissive light editor: makes this prim TYPE (e.g. a lamp) emit light from a local point
		ImGui::Separator();
		ImGui::TextColored({ 1.0f,0.9f,0.4f,1.0f }, "Emissive Light(lamp)");
		{
			const int primIndex = (pickedPrimGroupIdx >= 0 && pickedPrimGroupIdx < (int)primPlacedIndices.size())
				? primPlacedIndices[pickedPrimGroupIdx] : -1;
			if (primIndex < 0)
			{
				ImGui::TextDisabled("Unknown prim index - cannot define emissive light.");
			}
			else
			{
				ImGui::Text("Prim index: %d (applies to all instances)", primIndex);
				PrimLightDef def = primLightRegistry.Get(primIndex);
				bool changed = false;
				changed |= ImGui::Checkbox("Emits Light##emis", &def.enabled);
				if (ImGui::Button(pickingEmitPoint
					? "Picking ... click the lamp in the scene (or click to cancel)##emis"
					: "Pick Emit Point(clock on the lamp)##emis"))
				{
					pickingEmitPoint = !pickingEmitPoint;
				}
				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip("Clikc this, then click the glowing/glass part of the lamp\n in the 3D view to place the light exactly there.");
				}
				if (pickingEmitPoint)
				{
					ImGui::TextColored({ 1.0f,0.5,0.3f,1.0f }, "->Click the lamp model now.");
				}
				changed |= ImGui::DragFloat3("Emit Offset (local)##emis", &def.offset.x, 0.05f);
				changed |= ImGui::ColorEdit3("Light Color##emis", &def.color.x);
				changed |= ImGui::SliderFloat("Light Intensity##emis", &def.intensity, 0.0f, 10.0f, "%.2f");
				changed |= ImGui::SliderFloat("Atten Const##emis", &def.attConst, 0.0f, 5.0f, "%.2f");
				changed |= ImGui::SliderFloat("Atten Linear##emis", &def.attLin, 0.0f, 1.0f, "%.4f");
				changed |= ImGui::SliderFloat("Atten Quad##emis", &def.attQuad, 0.0f, 0.5f, "%.5f");

				if (changed)
				{
					primLightRegistry.Set(primIndex, def);
					RebuildEmissiveLights();
				}

				if (ImGui::Button("Save Light Defs##emis"))
				{
					try
					{
						primLightRegistry.Save(primLightRegistryPath);
						exportError = "OK: SSaved prim lights to " + primLightRegistryPath;
					}
					catch (const std::exception& e)
					{
						exportError = std::string("Save error: ") + e.what();
					}
				}
				ImGui::SameLine();
				if (ImGui::Button("Remove##emis"))
				{
					primLightRegistry.Remove(primIndex);
					RebuildEmissiveLights();
				}
				ImGui::TextDisabled("Active scene lights: %d/ %d",
					emissiveLightsData.count, MAX_EMISSIVE_LIGHTS);
			}
		}

		// Pulsating selection color
		ImGui::Separator();
		{
			static float pulseTime = 0.0f;
			pulseTime += ImGui::GetIO().DeltaTime * 3.0f;
			float pulse = 0.5f + 0.5f * std::sinf(pulseTime);
			dx::XMFLOAT3 selColor = { pulse * 0.8f + 0.2f, pulse * 0.8f + 0.2f, 1.0f };

			for (auto& tech : pPickedPrim->GetTechniques())
			{
				if (tech.GetName() != "Selection") continue;
				for (auto& step : tech.GetSteps())
				{
					for (auto& bindable : step.GetBindables())
					{
						if (auto* pCbuf = dynamic_cast<Bind::CachingPixelConstantBufferEx*>(bindable.get()))
						{
							auto buf = pCbuf->GetBuffer();
							buf["materialColor"] = selColor;
							pCbuf->SetBuffer(buf);
							break;
						}
					}
				}
			}
		}

		ImGui::Separator();
		ImGui::TextColored({ 0.4f, 1.0f, 0.6f, 1.0f }, "Debug");
		{
			bool changed = ImGui::Checkbox("Show Face Normals##prim", &showFaceNormals);
			if (showFaceNormals)
			{
				changed |= ImGui::SliderFloat("Normal Length##prim", &faceNormalLength, 0.05f, 5.0f);
			}
			if (changed)
			{
				RebuildNormalsIndicator();
			}
		}


		ImGui::Separator();
		if (ImGui::Button("Deselect##prim"))
		{
			if (pPrevSelectedPrimGroupIdx >= 0 && pPrevSelectedPrimGroupIdx < (int)primPlaced.size())
			{
				for (auto& pd : primPlaced[pPrevSelectedPrimGroupIdx])
				{
					for (auto& tech : pd->GetTechniques())
					{
						if (tech.GetName() == "Selection")
							tech.SetActiveState(false);
						//Reset render mode to default
						if (tech.GetName() == "PrimUnlit") tech.SetActiveState(!sceneLitMode);
						else if (tech.GetName() == "ColorLit") tech.SetActiveState(sceneLitMode);
						else if (tech.GetName() == "Wireframe") tech.SetActiveState(false);
					}
				}
			}

			pPrevSelectedPrim = nullptr;
			pPrevSelectedPrimGroupIdx = -1;
			pPickedPrim = nullptr;
			pickedPrimGroupIdx = -1;
			pickedPrimIdx = -1;
			selectedPrimRenderMode = 0;
			pNormalsIndicator.reset();
		}
	}
	//else if (pickedQuadMeasurement.has_value())
	//{
	//	const auto& m = pickedQuadMeasurement.value();
	//	ImGui::TextColored({ 1.0f, 1.0f, 0.3, 1.0f }, "Tile/Wall Measurement");
	//	ImGui::Separator();
	//	ImGui::Text("Width (v0-v1): %.4f", m.width);
	//	ImGui::Text("Height (v1-v2): %.4f", m.height);
	//	ImGui::Text("Diagonal (v0-v2): %.4f", m.diagonal0);
	//	ImGui::Text("Diagonal (v0-v1): %.4f", m.diagonal1);
	//	ImGui::Separator();
	//	ImGui::TextColored({ 0.6f, 0.8f, 1.0f, 1.0f }, "Corners (world space):");
	//	ImGui::Text("v0: (%.3f, %.3f, %.3f)", m.v0.x, m.v0.y, m.v0.z);
	//	ImGui::Text("v1: (%.3f, %.3f, %.3f)", m.v1.x, m.v1.y, m.v1.z);
	//	ImGui::Text("v2: (%.3f, %.3f, %.3f)", m.v2.x, m.v2.y, m.v2.z);	
	//	ImGui::Text("v3: (%.3f, %.3f, %.3f)", m.v3.x, m.v3.y, m.v3.z);
	//	ImGui::Separator();
	//	float edgeV1V2 = m.height; // already computed as v1->v2 in wall, v0->v3 in tile
	//	float edgeV2V3 = 0.0f;
	//	{
	//		float dx = m.v2.x - m.v3.x, dy = m.v2.y - m.v3.y, dz = m.v2.z - m.v3.z;
	//		edgeV2V3 = std::sqrtf(dx * dx + dy * dy + dz * dz);
	//	}
	//	float edgeV3V0 = 0.0f;
	//	{
	//		float dx = m.v3.x - m.v0.x, dy = m.v3.y - m.v0.y, dz = m.v3.z - m.v0.z;
	//		edgeV3V0 = std::sqrtf(dx * dx + dy * dy + dz * dz);
	//	}

	//	ImGui::TextColored({ 0.6f, 0.8f, 1.0f, 1.0f }, "All edges:");
	//	ImGui::Text("v0-v1: %.4f", m.width);
	//	ImGui::Text("v1-v2: %.4f", m.height);
	//	ImGui::Text("v2-v3: %.4f", edgeV2V3);
	//	ImGui::Text("v3-v0: %.4f", edgeV3V0);

	//	if (std::abs(m.width - m.height) > 0.001f ||
	//		std::abs(m.width - edgeV2V3) > 0.001f ||
	//		std::abs(m.width - edgeV3V0) > 0.001f)
	//	{
	//		ImGui::TextColored({ 1.0f, 0.3f, 0.3f, 1.0f }, "! Edges are NOT equal !");
	//	}
	//	else
	//	{
	//		ImGui::TextColored({ 0.3f, 1.0f, 0.3f, 1.0f }, "Edges are equal");
	//	}
	//}
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
		
		// == Render Mode selector ===
		ImGui::TextColored({ 0.6f,1.0f, 0.6f, 1.0f }, "Render Mode");
		{
			const char* renderModes[] = { "Default (Phong)", "Unlit", "ColorLit", "Wireframe" };
			if (ImGui::Combo("Mode", &selectedRenderMode, renderModes, IM_ARRAYSIZE(renderModes)))
			{

				// Deactive previous mode on previous mesh if different
				if (pPrevRenderModeMesh && pPrevRenderModeMesh != pPickedMesh)
				{
					// Reset previous mesh to default
					for (auto& tech : pPrevRenderModeMesh->GetTechniques())
					{
						if (tech.GetName() == "Phong") tech.SetActiveState(false);
						else if (tech.GetName() == "Unlit") tech.SetActiveState(false);
						else if (tech.GetName() == "ColorLit") tech.SetActiveState(false);
						else if (tech.GetName() == "Wireframe") tech.SetActiveState(false);
					}
				}

				// Apply selected mode to current mesh
				if (pPickedMesh)
				{
					for (auto& tech : pPickedMesh->GetTechniques())
					{
						if (tech.GetName() == "Phong") tech.SetActiveState(selectedRenderMode == 0);
						else if (tech.GetName() == "Unlit") tech.SetActiveState(selectedRenderMode == 1);
						else if (tech.GetName() == "ColorLit") tech.SetActiveState(selectedRenderMode == 2);
						else if (tech.GetName() == "Wireframe") tech.SetActiveState(selectedRenderMode == 3);
					}
					pPrevRenderModeMesh = pPickedMesh;
					showWireframe = (selectedRenderMode == 3);
					pPrevWireframeMesh = showWireframe ? pPickedMesh : nullptr;
				}
			}
			// ColorLit light settings (editable when ColorLit mode is active)
			if (selectedRenderMode == 2)
			{
				ImGui::TextColored({ 0.8f,0.8f, 1.0f, 1.0f }, "ColorLit Settings");
				static float lightTint[3] = { 0.3f, 0.8f, 1.0f };
				static float lightIntensity = 2.0f;
				static float matColor[3] = { 0.8f, 0.8f, 0.8f };
				bool changed = false;
				changed |= ImGui::ColorEdit3("Light Tint", lightTint);
				changed |= ImGui::SliderFloat("Intensity", &lightIntensity, 0.0f, 10.0f);
				changed |= ImGui::ColorEdit3("Material Color", matColor);

				if (changed)
				{
					// Update the ColorLit technique's constant buffer with new values
					for (auto& tech : pPickedMesh->GetTechniques())
					{
						if (tech.GetName() != "ColorLit") continue;
						for (auto& step : tech.GetSteps())
						{
							for (auto& bindable : step.GetBindables())
							{
								if (auto* pCbuf = dynamic_cast<Bind::CachingPixelConstantBufferEx*>(bindable.get()))
								{
									auto buf = pCbuf->GetBuffer();
									buf["materialColor"] = DirectX::XMFLOAT3(matColor[0], matColor[1], matColor[2]);
									buf["lightTint"] = DirectX::XMFLOAT3(lightTint[0], lightTint[1], lightTint[2]);
									buf["lightIntensity"] = lightIntensity;
									pCbuf->SetBuffer(buf);
									break;
								}
							}
						}
					}
				}
			}
		}

		ImGui::Separator();
		ImGui::TextColored({ 0.4f,1.0f, 0.6f, 1.0f }, "Textures");

		// Find and display current textures for the Phon technique
		if (pPickedMesh)
		{
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
									step.GetBindables()[i] = std::move(nextTex);
								}
							}
							ImGui::PopID();
						}
					}
				}
			}
		}

		ImGui::Separator();
		ImGui::TextColored({ 0.4f,1.0f,0.6f,1.0f }, "Debug");
		{
			bool changed = ImGui::Checkbox("Show Face Normals", &showFaceNormals);
			if (showFaceNormals)
			{
				changed |= ImGui::SliderFloat("Normal Length", &faceNormalLength, 0.05f, 5.0f);
			}
			if (changed)
			{
				RebuildNormalsIndicator();
			}
		}
	}

	ImGui::Separator();
	if (ImGui::Button("Deselect"))
	{
		// Rest render mode on current mesh
		if (pPrevRenderModeMesh)
		{
			for (auto& tech : pPrevRenderModeMesh->GetTechniques())
			{
				if (tech.GetName() == "Phong") tech.SetActiveState(false);
				else if (tech.GetName() == "Unlit") tech.SetActiveState(false);
				else if (tech.GetName() == "ColorLit") tech.SetActiveState(false);
				else if (tech.GetName() == "Wireframe") tech.SetActiveState(false);
			}
			pPrevRenderModeMesh = nullptr;
			selectedRenderMode = 0;
		}

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
		selectedRenderMode = 0;
		pPickedMesh = nullptr;
		pTriIndicator.reset();
		pNormalsIndicator.reset();
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

void App::ApplyGlobalRenderMode()
{
	if (pTileScene)
	{
		for (const auto& batch : pTileScene->GetBatches())
		{
			for (auto& tech : batch->GetTechniques())
			{
				if (tech.GetName() == "Lit") tech.SetActiveState(sceneLitMode);
				else if (tech.GetName() == "Unlit") tech.SetActiveState(!sceneLitMode);
			}
		}
	}

	for (auto& wb : wallBatches)
	{
		for (auto& tech : wb->GetTechniques())
		{
			if (tech.GetName() == "Lit") tech.SetActiveState(sceneLitMode);
			else if (tech.GetName() == "Unlit") tech.SetActiveState(!sceneLitMode);
		}
	}

	for (auto& group : primPlaced)
	{
		for (auto& pd : group)
		{
			for (auto& tech : pd->GetTechniques())
			{
				if (tech.GetName() == "ColorLit") tech.SetActiveState(sceneLitMode);
				else if (tech.GetName() == "PrimUnlit") tech.SetActiveState(!sceneLitMode);
			}
		}
	}
}

void App::RebuildEmissiveLights()
{
	emissiveWorldLights.clear();
	for (size_t g = 0; g < primPlaced.size(); ++g)

	{
		const int primIndex = (g < primPlacedIndices.size()) ? primPlacedIndices[g] : -1;
		if (primIndex < 0 || !primLightRegistry.Has(primIndex))
		{
			continue;
		}
		const PrimLightDef def = primLightRegistry.Get(primIndex);
		if (!def.enabled || primPlaced[g].empty())
		{
			continue;
		}

		// All drawables in a group share the same transform; use the first
		const PrimDrawable* pd = primPlaced[g].front().get();
		const dx::XMFLOAT3 pos = pd->GetPosition();
		const float yaw = pd->GetYaw();

		const dx::XMMATRIX world =
			dx::XMMatrixRotationY(yaw) *
			dx::XMMatrixTranslation(pos.x, pos.y, pos.z);
		const dx::XMVECTOR localOffset = dx::XMVectorSet(def.offset.x, def.offset.y, def.offset.z, 1.0f);
		dx::XMFLOAT3 worldPos;
		dx::XMStoreFloat3(&worldPos, dx::XMVector3Transform(localOffset, world));

		if (emissiveWorldLights.size() >= (size_t)MAX_EMISSIVE_LIGHTS)
		{
			break; // GPU buffer is full
		}

		EmissiveLightGPU light;
		light.viewPos = worldPos; // world position for now; transformed each frame
		light.color = def.color;
		light.intensity = def.intensity;
		light.attConst = def.attConst;
		light.attLin = def.attLin;
		light.attQuad = def.attQuad;
		emissiveWorldLights.push_back(light);
	}
}


void App::BindEmissiveLights(DirectX::FXMMATRIX view)
{
	if (!pEmissiveLightsCbuf)
	{
		return;
	}

	const int count = std::min((int)emissiveWorldLights.size(), MAX_EMISSIVE_LIGHTS);
	emissiveLightsData.count = count;
	for (int i = 0; i < count; ++i)
	{
		EmissiveLightGPU light = emissiveWorldLights[i];
		const dx::XMVECTOR worldPos = dx::XMVectorSet(light.viewPos.x, light.viewPos.y, light.viewPos.z, 1.0f);
		dx::XMStoreFloat3(&light.viewPos, dx::XMVector3Transform(worldPos, view));
		emissiveLightsData.lights[i] = light;
	}
	pEmissiveLightsCbuf->Update(wnd.Gfx(), emissiveLightsData);
	pEmissiveLightsCbuf->Bind(wnd.Gfx());
}

void App::PlaceEmitPointAtCursor()
{
	if (pickedPrimGroupIdx < 0 || pickedPrimGroupIdx >= (int)primPlaced.size()
		|| primPlaced[pickedPrimGroupIdx].empty())
	{
		pickingEmitPoint = false;
		return;
	}
	const int primIndex = (pickedPrimGroupIdx < (int)primPlacedIndices.size())
		? primPlacedIndices[pickedPrimGroupIdx] : -1;
	if (primIndex < 0)
	{
		pickingEmitPoint = false;
		return;
	}

	const auto [mouseX, mouseY] = wnd.mouse.GetPos();
	const int vpWidth = (int)wnd.Gfx().GetWidth();
	const int vpHeight = (int)wnd.Gfx().GetHeight();
	auto [rayOrigin, rayDir] = Picking::ScreenToRay(
		mouseX, mouseY, vpWidth, vpHeight,
		wnd.Gfx().GetProjection(), cameras->GetMatrix());

	//Ray-cast against the selected lamp's own geometry; take the closest hit.
	float bestT = FLT_MAX;
	bool hitAny = false;
	for (auto& pd : primPlaced[pickedPrimGroupIdx])
	{
		if (auto hit = pd->Intersect(rayOrigin, rayDir))
		{
			if (hit->second < bestT)
			{
				bestT = hit->second;
				hitAny = true;
			}
		}
	}
	if (!hitAny)
	{
		// Missed the model - keep the mode active so the user can try again;
		return;
	}

	const dx::XMVECTOR worldHit = dx::XMVectorAdd(rayOrigin, dx::XMVectorScale(rayDir, bestT));

	// Conver the world-space hti into the prim's local space so it follows the prim wherever it is placed/rotated
	const PrimDrawable* pd0 = primPlaced[pickedPrimGroupIdx].front().get();
	const dx::XMFLOAT3 pos = pd0->GetPosition();
	const float yaw = pd0->GetYaw();
	const dx::XMMATRIX world =
		dx::XMMatrixRotationY(yaw) *
		dx::XMMatrixTranslation(pos.x, pos.y, pos.z);
	const dx::XMMATRIX invWorld = dx::XMMatrixInverse(nullptr, world);
	dx::XMFLOAT3 local;
	dx::XMStoreFloat3(&local, dx::XMVector3Transform(worldHit, invWorld));

	PrimLightDef def = primLightRegistry.Get(primIndex);
	def.offset = local;
	def.enabled = true;
	primLightRegistry.Set(primIndex, def);
	RebuildEmissiveLights();

	pickingEmitPoint = false;
}


void App::DrawEmissiveLightOverlay()
{
	const bool haveSelected = (pickedPrimGroupIdx >= 0
		&& pickedPrimGroupIdx < (int)primPlaced.size()
		&& !primPlaced[pickedPrimGroupIdx].empty());
	if (emissiveWorldLights.empty() && !haveSelected)
	{
		return;
	}

	const int vpWidth = (int)wnd.Gfx().GetWidth();
	const int vpHeight = (int)wnd.Gfx().GetHeight();
	const auto viewMatrix = cameras->GetMatrix();
	const auto projMatrix = cameras->GetProjection();
	const auto viewProj = dx::XMMatrixMultiply(viewMatrix, projMatrix);
	auto* drawList = ImGui::GetOverlayDrawList();

	auto WorldToScreen = [&](const dx::XMFLOAT3& worldPos, ImVec2& screenOut) -> bool
		{
			const auto posView = dx::XMVector3TransformCoord(dx::XMLoadFloat3(&worldPos), viewMatrix);
			dx::XMFLOAT3 viewCoord;
			dx::XMStoreFloat3(&viewCoord, posView);
			if (viewCoord.z < 0.0f)
			{
				return false; // behind camera
			}
			const auto pos = dx::XMVector3TransformCoord(dx::XMLoadFloat3(&worldPos), viewProj);
			dx::XMFLOAT4 clip;
			dx::XMStoreFloat4(&clip, pos);
			screenOut.x = (clip.x * 0.5f + 0.5f) * vpWidth;
			screenOut.y = (-clip.y * 0.5f + 0.5f) * vpHeight;
			return true;
		};

	// Marker for every active emissive light (emissiveWorldLights stores WORLD pos).
	for (const auto& l : emissiveWorldLights)
	{
		ImVec2 s;
		if (!WorldToScreen(l.viewPos, s))
		{
			continue;
		}
		const ImU32 col = IM_COL32(
			(int)(std::min(1.0f, l.color.x) * 255.0f),
			(int)(std::min(1.0f, l.color.y) * 255.0f),
			(int)(std::min(1.0f, l.color.z) * 255.0f), 255);
		drawList->AddCircleFilled(s, 5.0f, col);
		drawList->AddCircle(s, 8.0f, IM_COL32(0, 0, 0, 200), 0, 2.0f);
		for (int a = 0; a < 8; ++a)
		{
			const float ang = a * 0.7853981f; // 45 deg
			const ImVec2 p0(s.x + std::cos(ang) * 10.f, s.y + std::sin(ang) * 10.0f);
			const ImVec2 p1(s.x + std::cos(ang) * 15.f, s.y + std::sin(ang) * 15.0f);
			drawList->AddLine(p0, p1, col, 2.0f);
		}
	}

	if (haveSelected)
	{
		const int primIndex = (pickedPrimGroupIdx < (int)primPlacedIndices.size())
			? primPlacedIndices[pickedPrimGroupIdx] : -1;
		if (primIndex < 0)
		{
			const PrimLightDef def = primLightRegistry.Get(primIndex);
			const PrimDrawable* pd = primPlaced[pickedPrimGroupIdx].front().get();
			const dx::XMFLOAT3 pos = pd->GetPosition();
			const float yaw = pd->GetYaw();
			const dx::XMMATRIX world =
				dx::XMMatrixRotationY(yaw) *
				dx::XMMatrixTranslation(pos.x, pos.y, pos.z);
			dx::XMFLOAT3 wp;
			dx::XMStoreFloat3(&wp, dx::XMVector3Transform(
				dx::XMVectorSet(def.offset.x, def.offset.y, def.offset.z, 1.0f), world));

			ImVec2 s;
			if (WorldToScreen(wp, s))
			{
				const ImU32 hl = pickingEmitPoint
					? IM_COL32(255, 90, 60, 255)
					: IM_COL32(255, 230, 80, 255);
				drawList->AddCircle(s, 11.0f, hl, 0, 2.5f);
				drawList->AddLine({ s.x - 15.0f, s.y }, { s.x + 15.0f, s.y }, hl, 1.5f);
				drawList->AddLine({ s.x , s.y - 15.0f }, { s.x , s.y + 15.0f }, hl, 1.5f);
				drawList->AddText({ s.x + 13.0f, s.y + 6.0f }, hl,
					pickingEmitPoint ? "click lamp to set emit point" : "emit point");
			}
		}
	}

}


void App::RebuildNormalsIndicator()
{
	pNormalsIndicator.reset();
	if (!showFaceNormals)
	{
		return;
	}

	// Collect world-space arrow segments {faceCenter, faceCenter + normal*length}
	std::vector<std::pair<dx::XMFLOAT3, dx::XMFLOAT3>> segments;

	const auto appendFaces = [&](const std::vector<dx::XMFLOAT3>& positions,
		const std::vector<unsigned short>& indices,
		dx::FXMMATRIX worldMat)
		{
			for (size_t i = 0; i + 2 < indices.size(); i += 3)
			{
				const auto p0 = dx::XMVector3TransformCoord(dx::XMLoadFloat3(&positions[indices[i + 0]]), worldMat);
				const auto p1 = dx::XMVector3TransformCoord(dx::XMLoadFloat3(&positions[indices[i + 1]]), worldMat);
				const auto p2 = dx::XMVector3TransformCoord(dx::XMLoadFloat3(&positions[indices[i + 2]]), worldMat);

				const auto center = dx::XMVectorScale(dx::XMVectorAdd(dx::XMVectorAdd(p0, p1), p2), 1.0f / 3.0f);

				const auto edge1 = dx::XMVectorSubtract(p1, p0);
				const auto edge2 = dx::XMVectorSubtract(p2, p0);
				auto normal = dx::XMVector3Cross(edge1, edge2);
				if (dx::XMVectorGetX(dx::XMVector3LengthSq(normal)) < 1e-12f)
				{
					continue; // skip degenerate triangle
				}
				normal = dx::XMVector3Normalize(normal);

				const auto tip = dx::XMVectorAdd(center, dx::XMVectorScale(normal, faceNormalLength));

				dx::XMFLOAT3 s, t;
				dx::XMStoreFloat3(&s, center);
				dx::XMStoreFloat3(&t, tip);
				segments.emplace_back(s, t);

			}
		};
	if (pPickedMesh)
	{
		appendFaces(pPickedMesh->GetCpuPositions(), pPickedMesh->GetCpuIndices(),
			dx::XMLoadFloat4x4(&pickedWorldTransform));
	}
	else if (pickedPrimGroupIdx >= 0 && pickedPrimGroupIdx < (int)primPlaced.size())
	{
		// A prim is split into one PrimDrawable per texture; include the whole group
		for (auto& pd : primPlaced[pickedPrimGroupIdx])
		{
			appendFaces(pd->GetCpuPositions(), pd->GetCpuIndices(), pd->GetTransformXM());
		}
	}

	if (segments.empty())
	{
		return;
	}

	pNormalsIndicator = std::make_unique<NormalsIndicator>(wnd.Gfx(), segments);
	pNormalsIndicator->LinkTechniques(GetRenderGraph());
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

void App::ShowNprimImportWindow()
{
	ImGui::Begin("Import Prim");

	static char nprimFilePath[MAX_PATH] = "";
	ImGui::InputText("Prim File", nprimFilePath, MAX_PATH);

	if (ImGui::Button("Browse Prim ..."))
	{
		std::array<char, MAX_PATH> buf{};
		OPENFILENAMEA ofn{};
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = nullptr;
		ofn.lpstrFile = buf.data();
		ofn.nMaxFile = (DWORD)buf.size();
		ofn.lpstrFilter = "Prim Files\0*.prm\0All Files\0*.*\0";
		ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
		if (GetOpenFileNameA(&ofn))
		{
			strncpy_s(nprimFilePath, buf.data(), MAX_PATH);
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("Load Prim"))
	{
		try
		{
			auto def = LoadPrimObject(nprimFilePath);
			auto texturedLists = ConvertPrimToTexturedTriangleList(def);
			primPreview.clear();
			for (auto& [texImgNo, triList] : texturedLists)
			{
				std::string texPath = GetPrimTexturePath(texImgNo);
				auto pd = std::make_unique<PrimDrawable>(wnd.Gfx(), std::move(triList), texPath);
				pd->LinkTechniques(*pUnlitRg);
				primPreview.push_back(std::move(pd));
			}
			// Dertive the prim index from the file name
			primPreviewIndex = -1;
			{
				const std::string fname = nprimFilePath;
				size_t end = fname.find_last_of('.');
				if (end == std::string::npos) end = fname.size();
				size_t digitsEnd = end;
				size_t digitsStart = end;
				while (digitsStart > 0 && std::isdigit((unsigned char)fname[digitsStart - 1]))
				{
					--digitsStart;
				}
				if (digitsStart < digitsEnd)
				{
					try { primPreviewIndex = std::stoi(fname.substr(digitsStart, digitsEnd - digitsStart)); }
					catch (...) { primPreviewIndex = -1; }
				}
			}
		}
		catch (const std::exception& e)
		{
			tileModelLoadError = std::string("Prim load error: ") + e.what();
		}
	}
		
	ImGui::Text("Placed prims: %zu", primPlaced.size());
	if (primPreviewIndex >= 0)
	{
		ImGui::SameLine();
		ImGui::TextDisabled("(prim index%d)", primPreviewIndex);
	}

	if(!primPlaced.empty() && ImGui::Button("Clear All Placed"))
	{
		primPlaced.clear();
		primPlacedIndices.clear();
		RebuildEmissiveLights();
	}
	if (!primPreview.empty())
	{
		ImGui::Text("Preview active - click to place");
	}
	if (!tileModelLoadError.empty())
	{
		ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%s", tileModelLoadError.c_str());
	}

	ImGui::End();
}


void App::DrawDebugOverlay()
{
	const int vpWidth = (int)wnd.Gfx().GetWidth();
	const int vpHeight = (int)wnd.Gfx().GetHeight();
	const auto viewMatrix = cameras->GetMatrix();
	const auto projMatrix = cameras->GetProjection();
	const auto viewProj = dx::XMMatrixMultiply(viewMatrix, projMatrix);

	auto* drawList = ImGui::GetOverlayDrawList();

	// Project a 3D world position to screen coordinates.
	// Return false if the point is behind the camera.
	auto WorldToScreen = [&](const dx::XMFLOAT3& worldPos, ImVec2& screenOut) -> bool
		{
			auto pos = dx::XMVector3TransformCoord(dx::XMLoadFloat3(&worldPos), viewProj);
			dx::XMFLOAT4 clip;
			dx::XMStoreFloat4(&clip, pos);
			// Behind camera check
			auto posView = dx::XMVector3TransformCoord(dx::XMLoadFloat3(&worldPos), viewMatrix);
			dx::XMFLOAT3 viewCoord;
			dx::XMStoreFloat3(&viewCoord, posView);
			if (viewCoord.z < 0.0f)
			{
				return false;
			}

			screenOut.x = (clip.x * 0.5f + 0.5f) * vpWidth;
			screenOut.y = (-clip.y * 0.5f + 0.5f) * vpHeight;
			return true;
		};

	// -- Draw world origin axes ---
	constexpr float axisLen = 5.0f;
	const dx::XMFLOAT3 origin = { 0.0f, 0.0f, 0.0f };
	const dx::XMFLOAT3 axisEndX = { axisLen, 0.0f, 0.0f };
	const dx::XMFLOAT3 axisEndY = { 0.0f, axisLen, 0.0f };
	const dx::XMFLOAT3 axisEndZ = { 0.0f, 0.0f, axisLen };

	ImVec2 screenOrigin, screenX, screenY, screenZ;
	if (WorldToScreen(origin, screenOrigin))
	{
		if (WorldToScreen(axisEndX, screenX))
		{
			drawList->AddLine(screenOrigin, screenX, IM_COL32(255, 50, 50, 255), 3.0f);
			drawList->AddText(screenX, IM_COL32(255, 50, 50, 255), "X");
		}
		if (WorldToScreen(axisEndY, screenY))
		{
			drawList->AddLine(screenOrigin, screenY, IM_COL32(20, 255, 50, 255), 3.0f);
			drawList->AddText(screenY, IM_COL32(50, 255, 50, 255), "Y");
		}
		if (WorldToScreen(axisEndZ, screenZ))
		{
			drawList->AddLine(screenOrigin, screenZ, IM_COL32(50, 50, 255, 255), 3.0f);
			drawList->AddText(screenZ, IM_COL32(50, 50, 255, 255), "Z");
		}
	}

	// -- Draw coordinate labels for placeed prims ---
	for(size_t gi =0; gi< primPlaced.size(); ++gi)
	{
		if (primPlaced[gi].empty())
			continue;

		// Use position of first drawable in the group as representative
		const auto pos = primPlaced[gi][0]->GetPosition();
		ImVec2 screenPos;
		if (WorldToScreen(pos, screenPos))
		{
			char buf[64];
			snprintf(buf, sizeof(buf), "[%zu] (%.1f, %.1f, %.1f)", gi, pos.x, pos.y, pos.z);
			drawList->AddText({ screenPos.x + 5.0f, screenPos.y - 10.f },
				IM_COL32(255, 255, 100, 220), buf);
			drawList->AddCircleFilled(screenPos, 4.0f, IM_COL32(255, 255, 0, 200));
		}
	}
}


void App::ShowWindowControlPanel()
{
	ImGui::Begin("Windows");

	ImGui::TextColored({ 0.4f, 1.0f, 0.6f, 1.0f }, "Toggle window  visibility:");
	ImGui::Separator();

	ImGui::Checkbox("Mesh Picker", &showPickingWindow);
	ImGui::Checkbox("Export", &showExportWindow);
	ImGui::Checkbox("UV Editor", &showUvEditorWindow);
	ImGui::Checkbox("Model Loader", &showModelLoaderWindow);

	if (sceneType == SceneType::TileMap)
	{
		ImGui::Checkbox("Tile Map", &showTileMapWindow);
		ImGui::Checkbox("Import Prim", &showNprimImportWindow);
	}

	ImGui::Separator();
	ImGui::Checkbox("Debug Overlay", &showDebugOverlay);
	ImGui::Checkbox("ImGui Demo", &showDemoWindow);

	ImGui::End();
}

static const char* WINDOW_SETTINGS_FILE = "window_settings.json";

void App::SaveWindowSettings() const
{
	nlohmann::json j;
	j["showPickingWindow"] = showPickingWindow;
	j["showTileMapWindow"] = showTileMapWindow;
	j["showExportWindow"] = showExportWindow;
	j["showNprimImportWindow"] = showNprimImportWindow;
	j["showModelLoaderWindow"] = showModelLoaderWindow;
	j["showUvEditorWindow"] = showUvEditorWindow;
	j["showDebugOverlay"] = showDebugOverlay;
	j["showDemoWindow"] = showDemoWindow;
	j["tileMapPathString"] = tileMapPathString;

	std::ofstream file(WINDOW_SETTINGS_FILE);
	if (file.is_open())
	{
		file << j.dump(2);
	}
}

template<typename T>
static void LoadIfExists(const nlohmann::json& j, const char* key, T& value)
{
	if (j.contains(key))
	{
		value = j[key].get<T>();
	}
}

void App::LoadWindowSettings()
{
	try
	{
		std::ifstream file(WINDOW_SETTINGS_FILE);
		if (!file.is_open())
			return;

		nlohmann::json j;
		file >> j;

		LoadIfExists(j, "showPickingWindow", showPickingWindow);
		LoadIfExists(j, "showTileMapWindow", showTileMapWindow);
		LoadIfExists(j, "showExportWindow", showExportWindow);
		LoadIfExists(j, "showNprimImportWindow", showNprimImportWindow);
		LoadIfExists(j, "showModelLoaderWindow", showModelLoaderWindow);
		LoadIfExists(j, "showUvEditorWindow", showUvEditorWindow);
		LoadIfExists(j, "showDebugOverlay", showDebugOverlay);
		LoadIfExists(j, "showDemoWindow", showDemoWindow);
		LoadIfExists(j, "tileMapPathString", tileMapPathString);
	}
	catch (const std::exception&)
	{
		// If parsing fails, keep defaults
	}
}

App::~App()
{
	SaveWindowSettings();
}

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