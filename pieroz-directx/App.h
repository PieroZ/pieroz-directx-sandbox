#pragma once
#include "Window.h"
#include "ChiliTimer.h"
#include "ImguiManager.h"
#include "CameraContainer.h"
#include "PointLight.h"
#include "TestCube.h"
#include "Model.h"
#include "ScriptCommander.h"
#include "BlurOutlineRenderGraph.h"
#include "ChiliMath.h"
#include "Picking.h"
#include "UVEditorWindow.h"
#include "TexturedTriangleOverlay.h"
#include "ObjExporter.h"
#include <memory>
#include <string>
#include <vector>

class Mesh;
class TriangleIndicator;

class App
{
public:
	App( const std::string& commandLine = "" );
	// master frame / message loop
	int Go();
	~App();
private:
	void DoFrame( float dt );
	void HandleInput( float dt );
	void ShowImguiDemoWindow();
	void PerformPicking();
	void ShowPickingWindow();
	void RebuildTexturedOverlays();
	void ShowExportWindow();

private:
	std::string commandLine;
	bool showDemoWindow = false;
	ImguiManager imgui;
	Window wnd;
	ScriptCommander scriptCommander;
	Rgph::BlurOutlineRenderGraph rg{ wnd.Gfx() };
	ChiliTimer timer;
	float speed_factor = 1.0f;
	CameraContainer cameras;
	PointLight light;
	TestCube cube{ wnd.Gfx(),4.0f };
	TestCube cube2{ wnd.Gfx(),4.0f };
	Model sponza{ wnd.Gfx(),"Models\\sponza\\sponza.obj",1.0f / 20.0f };
	Model gobber{ wnd.Gfx(),"Models\\gobber\\GoblinX.obj",4.0f };
	//Model nano{ wnd.Gfx(),"Models\\nano_textured\\nanosuit.obj",2.0f };

	std::unique_ptr<Model> dynamicModel;
	float dynamicModelScale = 1.0f;
	std::string dynamicModelLoadError;

	// Picking state
	Mesh* pPickedMesh = nullptr;
	size_t pickedFaceIndex = 0;
	float pickedDistance = 0.0f;
	//Mesh* pPrevOutlinedMesh = nullptr;
	Mesh* pPrevWireframeMesh = nullptr;
	bool showWireframe = false;
	std::unique_ptr<TriangleIndicator> pTriIndicator;
	UVEditorWindow uvEditor;
	std::vector<std::unique_ptr<TexturedTriangleOverlay>> texturedOverlays;
	DirectX::XMFLOAT4X4 pickedWorldTransform;
	std::string exportError;


	bool savingDepth = false;
	bool showImguiDebugWindows = false;
};