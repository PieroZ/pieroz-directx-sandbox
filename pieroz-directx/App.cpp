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

App::App( const std::string& commandLine )
	:
	commandLine( commandLine ),
	wnd( 1920,1080,"The Donkey Fart Box" ),
	scriptCommander( TokenizeQuoted( commandLine ) ),
	light( wnd.Gfx(),{ 10.0f,5.0f,0.0f } )
{
	cameras.AddCamera( std::make_unique<Camera>( wnd.Gfx(),"A",dx::XMFLOAT3{ -13.5f,6.0f,3.5f },0.0f,PI / 2.0f ) );
	cameras.AddCamera( std::make_unique<Camera>( wnd.Gfx(),"B",dx::XMFLOAT3{ -13.5f,28.8f,-6.4f },PI / 180.0f * 13.0f,PI / 180.0f * 61.0f ) );
	cameras.AddCamera( light.ShareCamera() );

	cube.SetPos( { 10.0f,5.0f,6.0f } );
	cube2.SetPos( { 10.0f,5.0f,14.0f } );
	//nano.SetRootTransform(
	//	dx::XMMatrixRotationY( PI / 2.f ) *
	//	dx::XMMatrixTranslation( 27.f,-0.56f,1.7f )
	//);
	gobber.SetRootTransform(
		dx::XMMatrixRotationY( -PI / 2.f ) *
		dx::XMMatrixTranslation( -8.f,10.f,0.f )
	);
	
	cube.LinkTechniques( rg );
	cube2.LinkTechniques( rg );
	light.LinkTechniques( rg );
	sponza.LinkTechniques( rg );
	gobber.LinkTechniques( rg );
	//nano.LinkTechniques( rg );
	cameras.LinkTechniques( rg );

	rg.BindShadowCamera( *light.ShareCamera() );
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

void App::DoFrame( float dt )
{
	wnd.Gfx().BeginFrame( 0.07f,0.0f,0.12f );
	light.Bind( wnd.Gfx(),cameras->GetMatrix() );
	rg.BindMainCamera( cameras.GetActiveCamera() );
		
	light.Submit( Chan::main );
	cube.Submit( Chan::main );
	sponza.Submit( Chan::main );
	cube2.Submit( Chan::main );
	gobber.Submit( Chan::main );
	//nano.Submit( Chan::main );
	cameras.Submit( Chan::main );


	if (dynamicModel)
	{
		dynamicModel->Submit(Chan::main);
		dynamicModel->Submit(Chan::shadow);
	}


	sponza.Submit( Chan::shadow );
	cube.Submit( Chan::shadow );
	sponza.Submit( Chan::shadow );
	cube2.Submit( Chan::shadow );
	gobber.Submit( Chan::shadow );
	//nano.Submit( Chan::shadow );

	rg.Execute( wnd.Gfx() );

	if( savingDepth )
	{
		rg.DumpShadowMap( wnd.Gfx(),"shadow.png" );
		savingDepth = false;
	}



	if (showImguiDebugWindows)
	{

		// imgui windows
		static MP sponzeProbe{ "Sponza" };
		static MP gobberProbe{ "Gobber" };
		static MP userMeshProbe{ "UserMesh" };
		sponzeProbe.SpawnWindow(sponza);
		gobberProbe.SpawnWindow(gobber);
		//nanoProbe.SpawnWindow(nano);

		if (dynamicModel)
		{
			userMeshProbe.SpawnWindow(*dynamicModel);
		}




		//cameras.SpawnWindow(wnd.Gfx());
		light.SpawnControlWindow();
		//ShowImguiDemoWindow();
		//cube.SpawnControlWindow(wnd.Gfx(), "Cube 1");
		//cube2.SpawnControlWindow(wnd.Gfx(), "Cube 2");

		//rg.RenderWindows(wnd.Gfx());
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
				// skopiuj ścieżkę do inputa
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

				dynamicModel->LinkTechniques(rg);
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

	// Picking/selection window
	ShowPickingWindow();

	// present
	wnd.Gfx().EndFrame();
	rg.Reset();
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
	if( pPrevOutlinedMesh)
	{
		for (auto& tech : pPrevOutlinedMesh->GetTechniques())
		{
			if( tech.GetName() == "Outline")
			{
				tech.SetActiveState(false);
			}
		}
		pPrevOutlinedMesh = nullptr;
	}

	// Test all models
	pPickedMesh = nullptr;
	float bestDist = FLT_MAX;

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
			}
		}
	};

	testModel(sponza);
	testModel(gobber);
	if(dynamicModel)
	{
		testModel(*dynamicModel);
	}

	// Enable outline on newly picked mesh
	if (pPickedMesh)
	{
		for (auto& tech : pPickedMesh->GetTechniques())
		{
			if (tech.GetName() == "Outline")
			{
				tech.SetActiveState(true);
			}
		}
		pPrevOutlinedMesh = pPickedMesh;
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
		ImGui::TextColored({ 0.4f,1.0f, 0.6f, 1.0f }, "Selected Mesh");
		ImGui::Text("Face index %zu", pickedFaceIndex);
		ImGui::Text("Distance %.2f", pickedDistance);
		//ImGui::Text("Total faces", pPickedMesh->GetCpuIndices().size);
		//ImGui::Text("Total vertices %zu", pPickedMesh->getCpu);

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
		if (pPrevOutlinedMesh)
		{
			for (auto& tech : pPrevOutlinedMesh->GetTechniques())
			{
				if (tech.GetName() == "Outline")
				{
					tech.SetActiveState(false);
				}
			}
			pPrevOutlinedMesh = nullptr;
			pPickedMesh = nullptr;
		}
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