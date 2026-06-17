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
#include "UnlitRenderGraph.h"
#include "RenderGraph.h"
#include "ChiliMath.h"
#include "Picking.h"
#include "UVEditorWindow.h"
#include "TexturedTriangleOverlay.h"
#include "ObjExporter.h"
#include "TileMapScene.h"
#include "TileBatch.h"
#include "PrimDrawable.h"
#include "WallBatch.h"
#include <memory>
#include <string>
#include <vector>
#include <optional>

class Mesh;
class TriangleIndicator;

enum class SceneType
{
	Default,	// Sponza + gobber + lighting + shadows
	TileMap,	// Flat textured tile grid, unlit, dynamic model loading
};

class App
{
public:
	App( const std::string& commandLine = "", SceneType scene = SceneType::Default );
	// master frame / message loop
	int Go();
	~App();
private:
	void DoFrame( float dt );
	void DoFrameDefault(float dt);
	void DoFrameTileMap(float dt);
	void HandleInput( float dt );
	void ShowImguiDemoWindow();
	void PerformPicking();
	void ShowPickingWindow();
	void RebuildTexturedOverlays();
	void ShowExportWindow();
	void ShowNprimImportWindow();
	void ShowTileMapWindow();
	void DrawDebugOverlay();
	void ShowWindowControlPanel();
	void SaveWindowSettings() const;
	void LoadWindowSettings();

	// Helper to get active render graph
	Rgph::RenderGraph& GetRenderGraph() noexcept;

private:
	std::string commandLine;
	SceneType sceneType;
	bool showDemoWindow = false;
	ImguiManager imgui;
	Window wnd;
	ScriptCommander scriptCommander;
	ChiliTimer timer;
	float speed_factor = 1.0f;
	CameraContainer cameras;
	
	// Render graphs (only one active based on sceneType)
	std::unique_ptr<Rgph::BlurOutlineRenderGraph> pBlurRg;
	std::unique_ptr<Rgph::UnlitRenderGraph> pUnlitRg;

	// === Default scene objects (SceneType::Default) ===
	std::unique_ptr<PointLight> pLight;
	std::unique_ptr<TestCube> pCube;
	std::unique_ptr<TestCube> pCube2;
	std::unique_ptr<Model> pSponza;
	std::unique_ptr<Model> pGobber;

	std::unique_ptr<Model> dynamicModel;
	float dynamicModelScale = 1.0f;
	std::string dynamicModelLoadError;

	// Picking state
	Mesh* pPickedMesh = nullptr;
	size_t pickedFaceIndex = 0;
	float pickedDistance = 0.0f;
	Mesh* pPrevWireframeMesh = nullptr;
	Mesh* pPrevRenderModeMesh = nullptr;
	bool showWireframe = false;
	int selectedRenderMode = 0; // 0=Phong, 1=Unlit, 2=ColorLit, 3=Wireframe

	// Prim picking state
	PrimDrawable* pPickedPrim = nullptr;
	int pickedPrimGroupIdx = -1;
	int pickedPrimIdx = -1;
	PrimDrawable* pPrevSelectedPrim = nullptr;
	int pPrevSelectedPrimGroupIdx = -1;
	int selectedPrimRenderMode = 0;
	std::unique_ptr<TriangleIndicator> pTriIndicator;
	UVEditorWindow uvEditor;
	std::vector<std::unique_ptr<TexturedTriangleOverlay>> texturedOverlays;
	DirectX::XMFLOAT4X4 pickedWorldTransform;
	std::string exportError;


	// === Tile map scene objects (SceneType::TileMap) ===
	std::unique_ptr<TileMapScene> pTileScene;
	std::string tileModelLoadError;
	float tileModelScale = 1.0f;


	
	// Tile/wall quad measurement (from picking)
	std::optional<QuadMeasurement> pickedQuadMeasurement;
	
	
	// Wall geometry built from DFacets ( one batch per texture ) 
	std::vector<std::unique_ptr<WallBatch>> wallBatches;

	//Prim objects placed in scene
	std::vector<std::vector<std::unique_ptr<PrimDrawable>>> primPlaced;
	// Preview prim following cursor (not yet placed)
	std::vector<std::unique_ptr<PrimDrawable>> primPreview;

	bool savingDepth = false;
	bool showImguiDebugWindows = false;
	bool showDebugOverlay = false;

	// Window visibility toggles 
	bool showPickingWindow = true;
	bool showTileMapWindow = true;
	bool showExportWindow = true;
	bool showNprimImportWindow = true;
	bool showModelLoaderWindow = true;
	bool showUvEditorWindow = true;

	std::string tileMapPathString;

	char tileMapPath[MAX_PATH];
};