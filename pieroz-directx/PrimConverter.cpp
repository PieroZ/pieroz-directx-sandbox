#include "PrimConverter.h"
#include <stdexcept>
#include <algorithm>

namespace dx = DirectX;

IndexedTriangleList ConvertPrimToTriangleList(
	const PrimLoadResult& prim,
	float scale)
{
	using Dvtx::VertexLayout;

	auto layout = VertexLayout{}
		.Append(VertexLayout::Position3D)
		.Append(VertexLayout::Normal)
		.Append(VertexLayout::Texture2D);

	Dvtx::VertexBuffer vbuf(std::move(layout));
	std::vector<unsigned short> indices;

	const dx::XMFLOAT3 defaultNormal{ 0.0f, 1.0f, 0.0f };

	auto getPoint = [&](std::uint16_t idx) -> dx::XMFLOAT3
		{
			if (idx >= prim.points.size())
			{
				throw std::runtime_error("Prim face references invalid point index: " + std::to_string(idx));
			}
			const auto& p = prim.points[idx];
			return
			{
				static_cast<float>(p.X) * scale,
				static_cast<float>(p.Y) * scale,
				static_cast<float>(p.Z) * scale,

			};
		};

	auto uvToFloat2 = [](std::uint8_t u, std::uint8_t v) -> dx::XMFLOAT2
		{
			return
			{
				static_cast<float>(u) / 255.0f,
				static_cast<float>(v) / 255.0f
			};
		};

	// Proces PrimFace3 (triangles)
	for (const auto& f : prim.faces3)
	{
		const auto baseIdx = static_cast<unsigned short>(vbuf.Size());

		for (int v = 0; v < 3; v++)
		{
			vbuf.EmplaceBack(
				getPoint(f.Points[v]),
				defaultNormal,
				uvToFloat2(f.UV[v][0], f.UV[v][1])
			);
		}

		indices.push_back(baseIdx + 0);
		indices.push_back(baseIdx + 1);
		indices.push_back(baseIdx + 2);
	}

	// Process PrimFace4 (quads -> 2 triangles each)
	for (const auto& f : prim.faces4)
	{
		const auto baseIdx = static_cast<unsigned short>(vbuf.Size());
		for (int v = 0; v < 4; v++)
		{
			vbuf.EmplaceBack(
				getPoint(f.Points[v]),
				defaultNormal,
				uvToFloat2(f.UV[v][0], f.UV[v][1])
			);
		}

		// Triangle 1: (0,1,2)
		indices.push_back(baseIdx + 0);
		indices.push_back(baseIdx + 3);
		indices.push_back(baseIdx + 1);
		// Triangle 2: (0,2,3)
		indices.push_back(baseIdx + 0);
		indices.push_back(baseIdx + 2);
		indices.push_back(baseIdx + 3);
	}

	if (vbuf.Size() < 3)
	{
		throw std::runtime_error("Prim must have at least 3 vertices to form a triangle.");
	}

	return IndexedTriangleList(std::move(vbuf), std::move(indices));
}