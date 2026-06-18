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
#include "PrimLightRegistry.h"
#include "EmissiveLightsCBuf.h"
#include "ConstantBuffers.h"
#include <memory>
#include <string>
#include <vector>
#include <optional>

class Mesh;
class TriangleIndicator;
class NormalsIndicator;

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
	void RebuildNormalsIndicator();
	void ApplyGlobalRenderMode();
	//Rebuild the world-space list of emissive prim lights from placed prims.
	void RebuildEmissiveLights();
	// Upload the active emissive lights (transformed rto view space) to the GPU.
	void BindEmissiveLights(DirectX::FXMMATRIX view);
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


	// Face-normal arrows overlay for the selected object
	std::unique_ptr<NormalsIndicator> pNormalsIndicator;
	bool showFaceNormals = false;
	float faceNormalLength = 0.3f;

	// === Tile map scene objects (SceneType::TileMap) ===
	std::unique_ptr<TileMapScene> pTileScene;
	std::string tileModelLoadError;
	float tileModelScale = 1.0f;


	// Animated scene light (oscillates like a swinging lamp over the tile grid)
	bool animateLight = true;
	float lightAnimTime =0.0f;	
	int lightAnimAxis = 0;
	float lightAnimSpeed = 0.6f;
	float lightAnimAmplitude = 9.0f;
	DirectX::XMFLOAT3 lightAnimCenter = { 7.0f, 5.0f, 7.0f };

	bool sceneLitMode = true;

	bool flashlightEnabled = false;
	float flashlightColor[3] = { 1.0f, 0.95f, 0.8f };
	float flashlightIntensity = 4.0f;
	float flashlightInnerDeg = 12.0f;
	float flashlightOuterDeg = 22.0f;
	float flashlightRange = 40.0f;
	float flashlightYawOffset = 0.0f;
	float flashlightPitchOffset = 0.0f;
	DirectX::XMFLOAT3 flashlightPosOffset = { 0.0f, 0.0f, 0.0f };
	bool flashlightFollowMouse = true; // aim the cone at the mouse cursor
	// Tile/wall quad measurement (from picking)
	std::optional<QuadMeasurement> pickedQuadMeasurement;
	
	
	// Wall geometry built from DFacets ( one batch per texture ) 
	std::vector<std::unique_ptr<WallBatch>> wallBatches;

	//Prim objects placed in scene
	std::vector<std::vector<std::unique_ptr<PrimDrawable>>> primPlaced;

	// Prim index ( e.g. 1== nprim001.prim) for each placed group parallel to primPlaced.
	std::vector<int> primPlacedIndices;
	// Preview prim following cursor (not yet placed)
	std::vector<std::unique_ptr<PrimDrawable>> primPreview;

	//Prim index of the current preview (-1 0f unknown) carried to primPlacedIndices on place.
	int primPreviewIndex = -1;

	//Emissive prim lights(lamps): per-prim-type defitions + GPU upload b uffer.
	PrimLightRegistry primLightRegistry;
	std::string primLightRegistryPath = "prim_lights.json";
	EmissiveLightsCBuf emissiveLightsData;
	std::unique_ptr<Bind::PixelConstantBuffer<EmissiveLightsCBuf>> pEmissiveLightsCbuf;
	//World-space staging lists viewPos fiel hold WORLD position here transformed to view space each frame in BindEmissiveLights.
	std::vector<EmissiveLightGPU> emissiveWorldLights;

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