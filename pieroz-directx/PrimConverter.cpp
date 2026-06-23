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

std::vector<PrimMeshPart> ConvertPrimToTexturedTriangleList(
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
            : vbuf(std::move(layout))
        {}
    };

    auto makeLayout = [] {
        return VertexLayout{}
            .Append(VertexLayout::Position3D)
            .Append(VertexLayout::Normal)
            .Append(VertexLayout::Texture2D);
        };

    // Key: {textureImgNo, doubleSided}. Double-sided faces are kept in separate builders so they can be renderedw ithout back-face culling
    std::map<std::pair<int, bool>, std::unique_ptr<TriListBuilder>> builders;

    auto getPoint = [&](std::uint16_t idx) -> dx::XMFLOAT3
        {
            if (idx >= prim.points.size())
            {
                throw std::runtime_error(
                    "Prim face reference invalid point index: " +
                    std::to_string(idx));
            }

            const auto& p = prim.points[idx];

            return {
                static_cast<float>(p.Z) * scale,
                static_cast<float>(p.Y) * scale,
                static_cast<float>(p.X) * scale,
            };
        };

    auto calcFaceNormal = [](const dx::XMFLOAT3& p0,
        const dx::XMFLOAT3& p1,
        const dx::XMFLOAT3& p2)
        {
            using namespace DirectX;

            const auto v0 = XMLoadFloat3(&p0);
            const auto v1 = XMLoadFloat3(&p1);
            const auto v2 = XMLoadFloat3(&p2);

            const auto e1 = XMVectorSubtract(v1, v0);
            const auto e2 = XMVectorSubtract(v2, v0);

            const auto n = XMVector3Normalize(
                XMVector3Cross(e1, e2));

            dx::XMFLOAT3 result;
            XMStoreFloat3(&result, n);

            return result;
        };

    auto getOrCreateBuilder = [&](int texImgNo, bool doubleSided) -> TriListBuilder&
        {
            const auto key = std::make_pair(texImgNo, doubleSided);
            auto it = builders.find(key);

            if (it == builders.end())
            {
                auto [inserted, _] =
                    builders.emplace(
                        key,
                        std::make_unique<TriListBuilder>(makeLayout()));

                return *inserted->second;
            }

            return *it->second;
        };

    //
    // PrimFace3 (triangles)
    //
    for (const auto& f : prim.faces3)
    {
        int av_u = calcAvUvTriangle(
            f.UV[0][0], f.UV[1][0], f.UV[2][0]);

        int av_v = calcAvUvTriangle(
            f.UV[0][1], f.UV[1][1], f.UV[2][1]);

        int av_u_tile = av_u / TEXTURE_NORM_SIZE;
        int av_v_tile = av_v / TEXTURE_NORM_SIZE;

        int base_u = av_u_tile * TEXTURE_NORM_SIZE;
        int base_v = av_v_tile * TEXTURE_NORM_SIZE;

        int texImgNo =
            calcTextureImgNo(
                av_u_tile,
                av_v_tile,
                f.TexturePage);

        const bool doubleSided =
            (f.DrawFlags & POLY_FLAG_DOUBLESIDED) != 0;
        auto& builder = getOrCreateBuilder(texImgNo, doubleSided);

        const auto baseIdx =
            static_cast<unsigned short>(builder.vbuf.Size());

        const auto p0 = getPoint(f.Points[0]);
        const auto p1 = getPoint(f.Points[1]);
        const auto p2 = getPoint(f.Points[2]);

        const auto normal =
            calcFaceNormal(p0, p1, p2);

        for (int v = 0; v < 3; v++)
        {
            float final_u =
                calcFinalUV(f.UV[v][0], base_u);

            float final_v =
                calcFinalUV(f.UV[v][1], base_v);

            const auto pos =
                (v == 0) ? p0 :
                (v == 1) ? p1 :
                p2;

            builder.vbuf.EmplaceBack(
                pos,
                normal,
                dx::XMFLOAT2{ final_u, final_v });
        }

        builder.indices.push_back(baseIdx + 0);
        builder.indices.push_back(baseIdx + 1);
        builder.indices.push_back(baseIdx + 2);
    }

    //
    // PrimFace4 (quads)
    //
    for (const auto& f : prim.faces4)
    {
        int av_u = calcAvUvQuad(
            f.UV[0][0],
            f.UV[1][0],
            f.UV[2][0],
            f.UV[3][0]);

        int av_v = calcAvUvQuad(
            f.UV[0][1],
            f.UV[1][1],
            f.UV[2][1],
            f.UV[3][1]);

        int av_u_tile = av_u / TEXTURE_NORM_SIZE;
        int av_v_tile = av_v / TEXTURE_NORM_SIZE;

        int base_u = av_u_tile * TEXTURE_NORM_SIZE;
        int base_v = av_v_tile * TEXTURE_NORM_SIZE;

        int texImgNo =
            calcTextureImgNo(
                av_u_tile,
                av_v_tile,
                f.TexturePage);


        const bool doubleSided =
            (f.DrawFlags & POLY_FLAG_DOUBLESIDED) != 0;

        auto& builder = getOrCreateBuilder(texImgNo, doubleSided);

        const auto baseIdx =
            static_cast<unsigned short>(builder.vbuf.Size());

        const auto p0 = getPoint(f.Points[0]);
        const auto p1 = getPoint(f.Points[1]);
        const auto p2 = getPoint(f.Points[2]);
        const auto p3 = getPoint(f.Points[3]);

        const auto normal =
            calcFaceNormal(p0, p1, p2);

        for (int v = 0; v < 4; v++)
        {
            float final_u =
                calcFinalUV(f.UV[v][0], base_u);

            float final_v =
                calcFinalUV(f.UV[v][1], base_v);

            const auto pos =
                (v == 0) ? p0 :
                (v == 1) ? p1 :
                (v == 2) ? p2 :
                p3;

            builder.vbuf.EmplaceBack(
                pos,
                normal,
                dx::XMFLOAT2{ final_u, final_v });
        }

        // Triangle 1: (0,1,3)
        builder.indices.push_back(baseIdx + 0);
        builder.indices.push_back(baseIdx + 1);
        builder.indices.push_back(baseIdx + 3);

        // Triangle 2: (0,3,2)
        builder.indices.push_back(baseIdx + 0);
        builder.indices.push_back(baseIdx + 3);
        builder.indices.push_back(baseIdx + 2);
    }

    //
    // Build final result
    //
    std::vector<PrimMeshPart> result;

    for (auto& [key, builder] : builders)
    {
        if (builder->vbuf.Size() >= 3)
        {
            result.push_back(PrimMeshPart{
                key.first,
                key.second,
                IndexedTriangleList(
                    std::move(builder->vbuf),
                    std::move(builder->indices)) });
        }
    }

    return result;
}

std::string GetPrimTexturePath(int textureImgNo)
{
	std::ostringstream oss;
	oss << "C:/dev/workspaces/repo clones/hw3d/pieroz-directx/pieroz-directx/UC-data/textures/prims/tex"
		<< std::setw(3) << std::setfill('0') << textureImgNo
		<< "hi.tga";
	return oss.str();
}


std::pair<std::string, std::string> GetPrimFilePaths(int primIndex)
{
	std::ostringstream nprim;
	nprim << "C:/Games/Urban Chaos/server/prims/nprim"
		<< std::setw(3) << std::setfill('0') << primIndex
		<< ".prm";

	std::ostringstream prim;
	prim << "C:/Games/Urban Chaos/server/prims/prim"
		<< std::setw(3) << std::setfill('0') << primIndex
		<< ".prm";

	return { nprim.str(), prim.str() };
}