#include "PrimExporter.h"
#include "Model.h"
#include "Node.h"
#include "Mesh.h"
#include "prim.h"
#include <fstream>
#include <filesystem>
#include <vector>
#include <cmath>
#include <cctype>
#include <cstring>
#include <cstdint>
#include <algorithm>
#include <DirectXMath.h>

namespace dx = DirectX;

namespace
{
	constexpr int TEXTURE_NORM_SIZE = 32;
	constexpr int TEXTURE_NORM_SQUARES = 8;
	constexpr int PAGE_MULTIPLIER = 11;

	struct NodeMeshes
	{
		std::vector<const Mesh*> meshes;
		dx::XMFLOAT4X4 worldTransform;
	};

	void CollectNodes(const Node& node, dx::XMMATRIX accumulated, std::vector<NodeMeshes>& out)
	{
		const auto nodeTransform =
			dx::XMLoadFloat4x4(&node.GetAppliedTransform()) *
			dx::XMLoadFloat4x4(&node.GetBaseTransform()) *
			accumulated;

		const auto& meshPtrs = node.GetMeshPtrs();
		if (!meshPtrs.empty())
		{
			NodeMeshes nm;
			for (const auto* pMesh : meshPtrs)
			{
				nm.meshes.push_back(pMesh);
			}
			dx::XMStoreFloat4x4(&nm.worldTransform, nodeTransform);
			out.push_back(std::move(nm));
		}

		for (const auto& child : node.GetChildren())
		{
			CollectNodes(*child, nodeTransform, out);
		}
	}

	int ExtractTextureImgNo(const std::string& texPath)
	{
		if (texPath.empty())
		{
			return 0;
		}

		std::string stem = std::filesystem::path(texPath).stem().string();

		// Strip the "hi" suffix if present (tex138hi -> tex138).
		if(stem.size() >= 2 &&
			(stem[stem.size() - 2] == 'h' || stem[stem.size() - 2] == 'H') &&
			(stem[stem.size() - 1] == 'i' || stem[stem.size() - 1] == 'I'))
		{
			stem = stem.substr(0, stem.size() - 2);
		}

		size_t end = stem.size();
		size_t start = end;
		while (start > 0 && std::isdigit(static_cast<unsigned char>(stem[start - 1])))
		{
			--start;
		}
		if (start == end)
		{
			return 0;
		}
		try
		{
			return std::stoi(stem.substr(start, end - start));
		}
		catch (...)
		{
			return 0;
		}
	}

	void EncodeTexturePage(int texImgNo, int& texturePage, int& uTile, int& vTile)
	{
		if (texImgNo < 0)
		{
			texImgNo = 0;
		}

		const int page = texImgNo + 64 * PAGE_MULTIPLIER;
		texturePage = page / 64;
		const int basePage = (texturePage - PAGE_MULTIPLIER) * 64;
		const int remainder = texImgNo - basePage;
		uTile = remainder % TEXTURE_NORM_SQUARES;
		vTile = remainder / TEXTURE_NORM_SQUARES;
	}

	// Encode a normalized [0..1] texture coordinate into the prim's per tile byte.
	std::uint8_t EncodeUV(float coord, int tile)
	{
		long v = std::lround(coord * TEXTURE_NORM_SIZE) +
			static_cast<long>(tile) * TEXTURE_NORM_SIZE;
		if (v < 0) v = 0;
		if (v > 255)v = 255;
		return static_cast<std::uint8_t>(v);
	}

	template<typename T>
	void Write(std::ofstream& f, const T& v)
	{
		f.write(reinterpret_cast<const char*>(&v), sizeof(T));
	}

	std::int16_t ToInt16Clamped(float value, bool& overflow)
	{
		long r = std::lround(value);
		if (r > 32767){ overflow = true; }
		if (r < -32768) { overflow = true; }
		return static_cast<std::int16_t>(r);
	}

	struct OutTriangle
	{
		int texturePage = 0;
		std::uint16_t points[3] = {};
		std::uint8_t uv[3][2] = {};
	};
}

bool PrimExporter::Export(const Model& model, const std::string& nprimPath, float scale, std::string& errorMsg)
{
	try
	{
		std::vector<NodeMeshes> nodes;
		CollectNodes(model.GetRootNode(), dx::XMMatrixIdentity(), nodes);

		std::vector<PrimPoint> points;
		std::vector<OutTriangle> triangles;
		bool posOverflow = false;

		for (const auto& nm : nodes)
		{
			const auto worldMat = dx::XMLoadFloat4x4(&nm.worldTransform);

			for (const auto* pMesh : nm.meshes)
			{
				const auto& positions = pMesh->GetCpuPositions();
				const auto& uvs = pMesh->GetCpuUVs();
				const auto& indices = pMesh->GetCpuIndices();
				const bool hasUVs = pMesh->HasUVs();

				const std::uint16_t baseIdx = static_cast<std::uint16_t>(points.size());

				for (const auto& p : positions)
				{
					dx::XMFLOAT3 wp;
					dx::XMStoreFloat3(&wp,
						dx::XMVector3TransformCoord(dx::XMLoadFloat3(&p), worldMat));

					PrimPoint pt;
					pt.X = ToInt16Clamped(wp.z * scale, posOverflow);
					pt.Y = ToInt16Clamped(wp.y * scale, posOverflow);
					pt.Z = ToInt16Clamped(wp.x * scale, posOverflow);
					points.push_back(pt);
				}

				const std::string defaultTex = pMesh->GetDefaultDiffuseTexturePath();
				const auto& overrides = pMesh->GetFaceTextureOverrides();
				const size_t numFaces = indices.size() / 3;

				for (size_t fi = 0; fi < numFaces; fi++)
				{
					const auto oit = overrides.find(fi);
					const std::string& tex =
						(oit != overrides.end()) ? oit->second : defaultTex;

					int texturePage = 0;
					int uTile = 0;
					int vTile = 0;
					EncodeTexturePage(ExtractTextureImgNo(tex), texturePage, uTile, vTile);

					const unsigned short i0 = indices[fi * 3 + 0];
					const unsigned short i1 = indices[fi * 3 + 1];
					const unsigned short i2 = indices[fi * 3 + 2];

					// Revers winding to match the reference tool.
					const unsigned short order[3] = { i2,i1,i0 };

					OutTriangle t;
					t.texturePage = texturePage;
					for (int k = 0; k < 3; k++)
					{
						t.points[k] = static_cast<std::uint16_t>(baseIdx + order[k]);

						if (hasUVs && order[k] < uvs.size())
						{
							t.uv[k][0] = EncodeUV(uvs[order[k]].x, uTile);
							t.uv[k][1] = EncodeUV(uvs[order[k]].y, vTile);
						}
						else
						{
							t.uv[k][0] = static_cast<std::uint8_t>(uTile * TEXTURE_NORM_SIZE);
							t.uv[k][1] = static_cast<std::uint8_t>(vTile * TEXTURE_NORM_SIZE);
						}
					}
					triangles.push_back(t);
				}
			}
		}

		if (points.empty() || triangles.empty())
		{
			errorMsg = "Model has no geometry to export.";
			return false;
		}

		if (points.size() > 0xFFFF || triangles.size() > 0x7FFF)
		{
			errorMsg = "Model too large for nprim format (point/face count exceeds 16-bit range)";
			return false;
		}

		std::ofstream file(nprimPath, std::ios::binary);
		if (!file)
		{
			errorMsg = "Failed to open output file: " + nprimPath;
			return false;
		}

		const std::uint16_t saveType = 5794;
		Write(file, saveType);

		// 32- byte null-padded object name dervived from the output file stem.
		char name[32] = {};
		const std::string stem = std::filesystem::path(nprimPath).stem().string();
		const size_t nameLen = std::min<size_t>(stem.size(), 31);
		std::memcpy(name, stem.data(), nameLen);
		file.write(name, sizeof(name));


		// PromObject header
		PrimObject obj{};
		obj.StartPoint = 0;
		obj.EndPoint = static_cast<std::uint16_t>(points.size());
		obj.StartFace4 = 0;
		obj.EndFace4 = 0;
		obj.StartFace3 = 0;
		obj.EndFace3 = static_cast<std::uint16_t>(triangles.size());
		obj.coltype = 1;// none
		obj.damage = 0;
		obj.shadowtype = 0;
		obj.flag = 0;
		Write(file, obj);

		// points
		for (const auto& p : points)
		{
			Write(file, p);
		}

		// triangle faces (28-byte PrimFace3 binary layout

		for (const auto& t : triangles)
		{
			Write(file, static_cast<std::uint8_t>(t.texturePage));
			Write(file, static_cast<std::uint8_t>(POLY_FLAG_GOURAD | POLY_FLAG_TEXTURED));
			Write(file, static_cast<std::uint16_t>(t.points[0]));
			Write(file, static_cast<std::uint16_t>(t.points[1]));
			Write(file, static_cast<std::uint16_t>(t.points[2]));
			Write(file, t.uv[0][0]); Write(file, t.uv[0][1]);
			Write(file, t.uv[1][0]); Write(file, t.uv[1][1]);
			Write(file, t.uv[2][0]); Write(file, t.uv[2][1]);
			Write(file, static_cast<std::uint8_t>(64));
			Write(file, static_cast<std::uint8_t>(64));
			Write(file, static_cast<std::uint8_t>(64));
			Write(file, static_cast<std::uint16_t>(0));
			Write(file, static_cast<std::uint16_t>(143));
			Write(file, static_cast<std::uint16_t>(0));
			Write(file, static_cast<std::uint8_t>(0));
			Write(file, static_cast<std::uint8_t>(0));
			const char pad[3] = {};
			file.write(pad, sizeof(pad));
		}

		if (!file)
		{
			errorMsg = "Write error while saving: " + nprimPath;
			return false;
		}

		errorMsg.clear();
		if (posOverflow)
		{
			errorMsg = "some vertices exceeded the int16 range and were clamped - lower the scale";
		}
		return true;
	}
	catch (const std::exception& e)
	{
		errorMsg = std::string("exception: ") + e.what();
		return false;
	}
}