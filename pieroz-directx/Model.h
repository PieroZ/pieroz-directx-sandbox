#pragma once
#include "Graphics.h"
#include "Picking.h"
#include <string>
#include <memory>
#include <filesystem>
#include <optional>

class Node;
class Mesh;
class ModelWindow;
struct aiMesh;
struct aiMaterial;
struct aiNode;

namespace Rgph
{
	class RenderGraph;
}

class Model
{
public:
	Model( Graphics& gfx,const std::string& pathString,float scale = 1.0f, bool unlit = false );
	void Submit( size_t channels ) const noxnd;
	void SetRootTransform( DirectX::FXMMATRIX tf ) noexcept;
	void Accept( class ModelProbe& probe );
	void LinkTechniques( Rgph::RenderGraph& );
	std::optional<PickResult> Pick(DirectX::XMVECTOR& rayOrigin, DirectX::XMVECTOR& rayDir) const noexcept;
	~Model() noexcept;
	const std::vector<std::unique_ptr<Mesh>>& GetMeshes() const noexcept { return meshPtrs; }
	const Node& GetRootNode() const noexcept { return *pRoot; }
private:
	static std::unique_ptr<Mesh> ParseMesh( Graphics& gfx,const aiMesh& mesh,const aiMaterial* const* pMaterials,const std::filesystem::path& path,float scale, bool unlit = false );
	std::unique_ptr<Node> ParseNode( int& nextId,const aiNode& node,float scale ) noexcept;
private:
	std::unique_ptr<Node> pRoot;
	// sharing meshes here perhaps dangerous?
	std::vector<std::unique_ptr<Mesh>> meshPtrs;
};