#include "TileMapScene.h"
#include "Channels.h"
#include <stdexcept>
#include <cmath>

namespace dx = DirectX;

TileMapScene::TileMapScene(Graphics& gfx, const TileMapDef& def)
	: currentDef(def)
{
	BuildTiles(gfx);
}

void TileMapScene::LoadMap(Graphics& gfx, const TileMapDef& def)
{
	currentDef = def;
	tiles.clear();
	tilePositions.clear();
	BuildTiles(gfx);
}

void TileMapScene::LoadDynamicModel(Graphics& gfx, const std::string& modelPath, float scale)
{
	// Load with unlit flag = true so materials use Unlit_VS/PS
	dynamicModel = std::make_unique<Model>(gfx, modelPath, scale, true);
}

void TileMapScene::SetDynamicModelTransform(DirectX::XMMATRIX transform)
{
	if (dynamicModel)
	{
		dynamicModel->SetRootTransform(transform);
	}
}

bool TileMapScene::HasDynamicModel() const noexcept
{
	return dynamicModel != nullptr;
}

void TileMapScene::LinkTechniques(Rgph::RenderGraph& rg)
{
	for (const auto& tile : tiles)
	{
		tile->LinkTechniques(rg);
	}
	if (dynamicModel)
	{
		dynamicModel->LinkTechniques(rg);
	}
}

size_t TileMapScene::Submit(size_t channels, Graphics& gfx) const
{
	// Extract frustum planes from current view-projection matrix
	const auto viewProj = gfx.GetCamera() * gfx.GetProjection();
	dx::XMFLOAT4 planes[6];
	ExtractFrustumPlanes(planes, viewProj);

	// Get camera position for distance culling
	const auto invView = dx::XMMatrixInverse(nullptr, gfx.GetCamera());
	dx::XMFLOAT4X4 invViewF;
	dx::XMStoreFloat4x4(&invViewF, invView);
	const float camX = invViewF._41;
	const float camY = invViewF._42;
	const float camZ = invViewF._43;
	const float distSq = drawDistance * drawDistance;

	size_t submitted = 0;

	// Submit only tiles within draw distance and inside frustum
	for (size_t i = 0; i < tiles.size(); i++)
	{
		const auto& p = tilePositions[i];

		// Distance culling (skip if DrawDistance > 0 and tile is too far)
		if (drawDistance > 0.0f)
		{
			const float dx2 = p.x - camX;
			const float dy2 = p.y - camY;
			const float dz2 = p.z - camZ;
			if ((dx2 * dx2 + dy2 * dy2 + dz2 * dz2) > distSq)
			{
				continue; // Tile is beyond draw distance
			}
		}

		if (IsSphereInFrustum(planes, tilePositions[i], cullingRadius))
		{
			tiles[i]->Submit(channels);
			submitted++;
		}
	}

	if (dynamicModel)
	{
		dynamicModel->Submit(channels);
	}

	return submitted;
}

void TileMapScene::BuildTiles(Graphics& gfx)
{
	tiles.reserve(currentDef.tiles.size());
	tilePositions.reserve(currentDef.tiles.size());

	// Bounding sphere radius: half-diagonal of the tile quad
	cullingRadius = currentDef.tileSize * 0.7071f; // sqrt(2)/2

	for (const auto& tileDef : currentDef.tiles)
	{
		const float worldX = currentDef.originX + tileDef.col * currentDef.tileSize;
		const float worldZ = currentDef.originZ + tileDef.row * currentDef.tileSize;
		const float worldY = tileDef.height;

		tilePositions.push_back({ worldX, worldY, worldZ });

		tiles.push_back(std::make_unique<Tile>(
			gfx, currentDef.tileSize, worldX, worldY, worldZ, tileDef.texturePath
		));
	}
}

void TileMapScene::ExtractFrustumPlanes(DirectX::XMFLOAT4 planes[6], DirectX::FXMMATRIX viewProj) noexcept
{
	// Store as row-major for column extraction
	dx::XMFLOAT4X4 m;
	dx::XMStoreFloat4x4(&m, viewProj);

	// Left: col0 + col3
	planes[0] = {
		m._14 + m._11,
		m._24 + m._21,
		m._34 + m._31,
		m._44 + m._41
	};
	// Right: col3 - col0
	planes[1] = {
		m._14 - m._11,
		m._24 - m._21,
		m._34 - m._31,
		m._44 - m._41
	};
	// Bottom: col1 + col3
	planes[2] = {
		m._14 + m._12,
		m._24 + m._22,
		m._34 + m._32,
		m._44 + m._42
	};
	// Top: col3 - col1
	planes[3] = {
		m._14 - m._12,
		m._24 - m._22,
		m._34 - m._32,
		m._44 - m._42
	};
	// Near: col2
	planes[4] = {
		m._13,
		m._23,
		m._33,
		m._43
	};
	// Far: col3 - col2
	planes[5] = {
		m._14 - m._13,
		m._24 - m._23,
		m._34 - m._33,
		m._44 - m._43
	};

	// Normalize each plane
	for (int i = 0; i < 6; i++)
	{
		const float length = std::sqrt(
			planes[i].x * planes[i].x +
			planes[i].y * planes[i].y +
			planes[i].z * planes[i].z
		);
		if (length > 0.0f)
		{
			planes[i].x /= length;
			planes[i].y /= length;
			planes[i].z /= length;
			planes[i].w /= length;
		}
	}
}

bool TileMapScene::IsSphereInFrustum(const DirectX::XMFLOAT4 planes[6], const dx::XMFLOAT3& center, float radius) noexcept
{
	for (int i = 0; i < 6; i++)
	{
		const float distance =
			planes[i].x * center.x +
			planes[i].y * center.y +
			planes[i].z * center.z +
			planes[i].w;
		if (distance < -radius)
		{
			return false; // Sphere is completely outside this plane
		}
	}
	return true; // Sphere intersects or is inside all planes
}