#pragma once
#include "Graphics.h"
#include "Drawable.h"
#include "ConditionalNoexcept.h"
#include "Vertex.h"
#include <optional>
#include <unordered_map>

class Material;
class FrameCommander;
struct aiMesh;


class Mesh : public Drawable
{
public:
	Mesh( Graphics& gfx,const Material& mat,const aiMesh& mesh,float scale = 1.0f ) noxnd;
	DirectX::XMMATRIX GetTransformXM() const noexcept override;
	void Submit( size_t channels,DirectX::FXMMATRIX accumulatedTranform ) const noxnd;
	// Picking: test ray (in world space) against mesh triangles transformed by worldTransform
	std::optional<std::pair<size_t, float>> Intersect(
		DirectX::FXMVECTOR rayOrigin,
		DirectX::FXMVECTOR rayDir,
		DirectX::FXMMATRIX worldTransform) const noexcept;
	const std::vector<DirectX::XMFLOAT3>& GetCpuPositions() const noexcept { return cpuPositions; }
	const std::vector<unsigned short>& GetCpuIndices() const noexcept { return cpuIndices; }
	const std::vector<DirectX::XMFLOAT2>& GetCpuUVs() const noexcept { return cpuUVs; }
	void SetCpuUV(size_t vertexIndex, const DirectX::XMFLOAT2 & uv) noexcept;
	const std::vector<DirectX::XMFLOAT3>& GetCpuNormals() const noexcept { return cpuNormals; }
	bool HasUVs() const noexcept { return !cpuUVs.empty(); }
	void UpdateGpuVertexBuffer(Graphics& gfx);
	// Per-face texture override
	void SetFaceTextureOverride(size_t faceIndex, const std::string& texturePath);
	void ClearFaceTextureOverride(size_t faceIndex);
	const std::unordered_map<size_t, std::string>& GetFaceTextureOverrides() const noexcept { return faceTextureOverrides; }
	bool HasFaceTextureOverride(size_t faceIndex) const noexcept;
	std::string GetDefaultDiffuseTexturePath() const;

private:
	mutable DirectX::XMFLOAT4X4 transform;
	std::vector<DirectX::XMFLOAT3> cpuPositions;
	std::vector<unsigned short> cpuIndices;
	std::vector<DirectX::XMFLOAT2> cpuUVs;
	std::vector<DirectX::XMFLOAT3> cpuNormals;
	std::optional<Dvtx::VertexBuffer> cpuVertexData;
	bool gpuDirty = false;
	std::unordered_map<size_t, std::string> faceTextureOverrides;
};