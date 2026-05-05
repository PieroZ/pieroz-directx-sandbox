#include "PrimConverter.h"
#include <stdexcept>
#include <algorithm>
#include <cmath>
#include <sstream>
#include <iomanip>

namespace dx = DirectX;

namespace
{
	constexpr int TEXTURE_NORM_SIZE = 32;
	constexpr int TEXTURE_NORM_SQUARES = 8;

	int calcAvUvTriangle(int a, int b, int c)
	{
		return (a + b + c) / 3;
	}

	int calcAvUvQuad(int a, int b, int c, int d)
	{
		return (a + b + c + d) >> 2;
	}

	float calcFinalUV(int a, int base_a)
	{
		return static_cast<float>(a - base_a) / static_cast<float>(TEXTURE_NORM_SIZE);
	}
	
	int calcTextureImgNo(int av_u_tile, int av_v_tile, int texturePage)
	{
		int page = av_u_tile + av_v_tile * TEXTURE_NORM_SQUARES
			+ texturePage * TEXTURE_NORM_SQUARES * TEXTURE_NORM_SQUARES;

		return page - 64 * 11;
	}
}

std::map<int, IndexedTriangleList> ConvertPrimToTexturedTriangleList(
	const PrimLoadResult& prim,
	float scale)
{
	using Dvtx::VertexLayout;

	// Per-texture group: accumulate vertices and indices separately
	struct TriListBuilder
	{
		Dvtx::VertexBuffer vbuf;
		std::vector<unsigned short> indices;
		TriListBuilder(Dvtx::VertexLayout layout)
			: vbuf(std::move(layout)) {}
	};

	auto makeLayout = [] {
		return VertexLayout{}
			.Append(VertexLayout::Position3D)
			.Append(VertexLayout::Normal)
			.Append(VertexLayout::Texture2D);
		};

	std::map<int, std::unique_ptr<TriListBuilder>> builders;

	const dx::XMFLOAT3 defaultNormal{ 0.0f, 1.0f, 0.0f };

	auto getPoint = [&](std::uint16_t idx) -> dx::XMFLOAT3
		{
			if (idx >= prim.points.size())
			{
				throw std::runtime_error("Prim face reference invalid point index: " + std::to_string(idx));
			}
			const auto& p = prim.points[idx];

			return {
				static_cast<float>(p.X) * scale,
				static_cast<float>(p.Y) * scale,
				static_cast<float>(p.Z) * scale,
			};
		};

	auto getOrCreateBuilder = [&](int texImgNo) -> TriListBuilder&
		{
			auto it = builders.find(texImgNo);
			if (it == builders.end())
			{
				auto [inserted, _] = builders.emplace(texImgNo, std::make_unique<TriListBuilder>(makeLayout()));
				return *inserted->second;
			}
			return *it->second;
		};


	// Process PrimFace3 ( triangles )
	for (const auto& f : prim.faces3)
	{
		int av_u = calcAvUvTriangle(f.UV[0][0], f.UV[1][0], f.UV[2][0]);
		int av_v = calcAvUvTriangle(f.UV[0][1], f.UV[1][1], f.UV[2][1]);
		int av_u_tile = av_u / TEXTURE_NORM_SIZE;
		int av_v_tile = av_v / TEXTURE_NORM_SIZE;
		int base_u = av_u_tile * TEXTURE_NORM_SIZE;
		int base_v = av_v_tile * TEXTURE_NORM_SIZE;

		int texImgNo = calcTextureImgNo(av_u_tile, av_v_tile, f.TexturePage);

		auto& builder = getOrCreateBuilder(texImgNo);
		const auto baseIdx = static_cast<unsigned short>(builder.vbuf.Size());

		for (int v = 0; v < 3; v++)
		{
			float final_u = calcFinalUV(f.UV[v][0], base_u);
			float final_v = calcFinalUV(f.UV[v][1], base_v);

			builder.vbuf.EmplaceBack(
				getPoint(f.Points[v]),
				defaultNormal,
				dx::XMFLOAT2{ final_u, final_v }
			);
		}

		builder.indices.push_back(baseIdx + 0);
		builder.indices.push_back(baseIdx + 2);
		builder.indices.push_back(baseIdx + 1);
	}

	// Process PrimFace4 ( quads -> 2 triangles each)
	for (const auto& f : prim.faces4)
	{
		int av_u = calcAvUvQuad(f.UV[0][0], f.UV[1][0], f.UV[2][0], f.UV[3][0]);
		int av_v = calcAvUvQuad(f.UV[0][1], f.UV[1][1], f.UV[2][1], f.UV[3][1]);
		int av_u_tile = av_u / TEXTURE_NORM_SIZE;
		int av_v_tile = av_v / TEXTURE_NORM_SIZE;
		int base_u = av_u_tile * TEXTURE_NORM_SIZE;
		int base_v = av_v_tile * TEXTURE_NORM_SIZE;

		int texImgNo = calcTextureImgNo(av_u_tile, av_v_tile, f.TexturePage);

		auto& builder = getOrCreateBuilder(texImgNo);
		const auto baseIdx = static_cast<unsigned short>(builder.vbuf.Size());

		for (int v = 0; v < 4; v++)
		{
			float final_u = calcFinalUV(f.UV[v][0], base_u);
			float final_v = calcFinalUV(f.UV[v][1], base_v);

			builder.vbuf.EmplaceBack(
				getPoint(f.Points[v]),
				defaultNormal,
				dx::XMFLOAT2{ final_u, final_v }
			);
		}
		// Triangle 1: (0,3,1)
		builder.indices.push_back(baseIdx + 0);
		builder.indices.push_back(baseIdx + 3);
		builder.indices.push_back(baseIdx + 1);
		// Triangle 2: (0,2,3)
		builder.indices.push_back(baseIdx + 0);
		builder.indices.push_back(baseIdx + 2);
		builder.indices.push_back(baseIdx + 3);
	}


	// Convert builder to result map
	std::map<int, IndexedTriangleList> result;
	for (auto& [texImgNo, builder] : builders)
	{
		if (builder->vbuf.Size() >= 3)
		{
			result.emplace(texImgNo, IndexedTriangleList(std::move(builder->vbuf), std::move(builder->indices)));
		}
	}

	return result;
}

std::string GetPrimTexturePath(int textureImgNo)
{
	std::ostringstream oss;
	oss << "C:/dev/workspaces/repo clones/hw3d/pieroz-directx/pieroz-directx/UC-data/textures/prims/tex"
		<< std::setw(3) << std::setfill('0') << textureImgNo
		<< "hi.png";
	return oss.str();
}