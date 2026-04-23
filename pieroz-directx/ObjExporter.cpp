#include "ObjExporter.h"
#include "Model.h"
#include "Node.h"
#include "Mesh.h"
#include "Texture.h"
#include <fstream>
#include <filesystem>
#include <unordered_map>
#include <unordered_set>
#include <DirectXMath.h>

namespace dx = DirectX;

namespace
{
	struct NodeExportData
	{
		std::string name;
		std::vector<const Mesh*> meshes;
		dx::XMFLOAT4X4 worldTransform;
	};

	// Recursively collect nodes that contain meshes, grouping all meshes under same node
	void CollectNodes(
		const Node& node,
		dx::FXMMATRIX accumulatedTransform,
		std::vector<NodeExportData>& out,
		int& nameSuffix)
	{
		const auto nodeTransform =
			dx::XMLoadFloat4x4(&node.GetAppliedTransform()) *
			dx::XMLoadFloat4x4(&node.GetBaseTransform()) *
			accumulatedTransform;

		const auto& meshPtrs = node.GetMeshPtrs();
		if (!meshPtrs.empty())
		{
			NodeExportData d;
			d.name = node.GetName();
			if (d.name.empty())
			{
				d.name = "Part_" + std::to_string(nameSuffix++);
			}
			for (const auto& pMesh : meshPtrs)
			{
				d.meshes.push_back(pMesh);
			}
			dx::XMStoreFloat4x4(&d.worldTransform, nodeTransform);
			out.push_back(std::move(d));
		}

		for (const auto& child : node.GetChildren())
		{
			CollectNodes(*child, nodeTransform, out, nameSuffix);
		}
	}

	std::string SanitizeMaterialName(const std::string& name)
	{
		std::string sanitized = name;
		for (char& c : sanitized)
		{
			if (!std::isalnum(c))
				c = '_';
		}
		return sanitized;
	}
}

bool ObjExporter::Export(const Model& model, const std::string& objPath, std::string& errorMsg)
{
	try
	{
		std::ofstream objFile(objPath);
		if (!objFile.is_open())
		{
			errorMsg = "Failed to open file: " + objPath;
			return false;
		}

		const auto objDir = std::filesystem::path(objPath).parent_path();
		const auto mtlName = std::filesystem::path(objPath).stem().string() + ".mtl";
		const auto mtlPath = (objDir / mtlName).string();

		objFile << "# Exported by pieroz-directx UV Editor\n";
		objFile << "mtllib " << mtlName << "\n\n";

		// Collect all meshes with transforms
		std::vector<NodeExportData> nodes;
		int nameSuffix = 0;
		CollectNodes(model.GetRootNode(), dx::XMMatrixIdentity(), nodes, nameSuffix);

		// Maps: texture path -> material name
		std::unordered_map<std::string, std::string> texToMaterial;
		int matCounter = 0;
		auto getOrCreateMaterial = [&](const std::string& texPath) -> std::string
			{
				if (texPath.empty())
					return "default";
				auto it = texToMaterial.find(texPath);
				if (it != texToMaterial.end())
					return it->second;
				std::string name = "mat_" + SanitizeMaterialName(
					std::filesystem::path(texPath).stem().string()
				) + "_" + std::to_string(matCounter++);
				texToMaterial[texPath] = name;
				return name;
			};
		
		// Global vertex offset (OBJ indices are 1-based and global)
		size_t globalVertexOffset = 1;
		size_t globalUVOffset = 1;
		size_t globalNormalOffset = 1;

		// Track used node names to avoid duplicates
		std::unordered_map<std::string, int> nameCount;

		for (size_t mi = 0; mi < nodes.size(); mi++)
		{
			const auto& nodeData = nodes[mi];
			const auto worldMat = dx::XMLoadFloat4x4(&nodeData.worldTransform);
			// Inverse-transpose for normals
			const auto worldMatInvT = dx::XMMatrixTranspose(dx::XMMatrixInverse(nullptr, worldMat));

			// Generate unique object name
			std::string objName = nodeData.name;
			auto& count = nameCount[objName];
			if (count > 0)
			{
				objName += "_" + std::to_string(count);
			}
			count++;

			objFile << "# " << objName << " (" << nodeData.meshes.size() << " sub-meshes)\n";
			objFile << "o " << objName << "\n";

			// Track per-node vertex ranges for each sub-mesh
			struct SubMeshRange
			{
				size_t vertexStart;
				size_t uvStart;
				size_t normalStart;
				size_t vertexCount;
				size_t uvCount;
				size_t normalCount;
			};
			std::vector<SubMeshRange> subRanges;

			//First pass: write all vertex data for all sub-meshes in this node
			for (const auto* pMesh : nodeData.meshes)
			{
				SubMeshRange range;
				range.vertexStart = globalVertexOffset;
				range.uvStart = globalUVOffset;
				range.normalStart = globalNormalOffset;

				const auto& positions = pMesh->GetCpuPositions();
				const auto& uvs = pMesh->GetCpuUVs();
				const auto& normals = pMesh->GetCpuNormals();

				// Write vertices (transformed to world space)
				for (const auto& pos : positions)
				{
					dx::XMFLOAT3 wp;
					dx::XMStoreFloat3(&wp, dx::XMVector3TransformCoord(dx::XMLoadFloat3(&pos), worldMat));
					objFile << "v " << wp.x << " " << wp.y << " " << wp.z << "\n";
				}

				// Write texture coords (flip V: DirectX V=0 top -> OBJ V=0 bottom)
				for (const auto& uv : uvs)
				{
					objFile << "vt " << uv.x << " " << (1.0f - uv.y) << "\n";
				}

				// Write normals (transformed)
				for (const auto& nrm : normals)
				{
					dx::XMFLOAT3 wn;
					dx::XMVECTOR n = dx::XMVector3Normalize(
						dx::XMVector3TransformNormal(dx::XMLoadFloat3(&nrm), worldMat)
					);
					dx::XMStoreFloat3(&wn, n);
					objFile << "vn " << wn.x << " " << wn.y << " " << wn.z << "\n";
				}

				range.vertexCount = positions.size();
				range.uvCount = uvs.size();
				range.normalCount = normals.size();
				globalVertexOffset += positions.size();
				globalUVOffset += uvs.size();
				globalNormalOffset += normals.size();
				subRanges.push_back(range);
			}

			// Second pass: write faces grouped by material across all sub-meshes
			std::unordered_map<std::string, std::vector<std::string>> materialFaceLines;

			for (size_t si = 0; si < nodeData.meshes.size(); si++)
			{
				const auto* pMesh = nodeData.meshes[si];
				const auto& range = subRanges[si];
				const auto& indices = pMesh->GetCpuIndices();
				const bool hasUVs = pMesh->HasUVs();
				const bool hasNormals = !pMesh->GetCpuNormals().empty();
				const size_t numFaces = indices.size() / 3;

				const std::string defaultTex = pMesh->GetDefaultDiffuseTexturePath();
				const auto& overrides = pMesh->GetFaceTextureOverrides();

				for (size_t fi = 0; fi < numFaces; fi++)
				{
					auto oit = overrides.find(fi);
					const std::string& tex = (oit != overrides.end()) ? oit->second : defaultTex;
					const std::string matName = getOrCreateMaterial(tex);

					const size_t i0 = indices[fi * 3 + 0];
					const size_t i1 = indices[fi * 3 + 1];
					const size_t i2 = indices[fi * 3 + 2];

					std::string faceLine = "f";
					auto appendIdx = [&](size_t idx)
						{
							faceLine += " ";
							faceLine += std::to_string(range.vertexStart + idx);
							if (hasUVs || hasNormals)
							{
								faceLine += "/";
								if (hasUVs)
									faceLine += std::to_string(range.uvStart + idx);
								if (hasNormals)
									faceLine += "/" + std::to_string(range.normalStart + idx);
							}
						};
					appendIdx(i0);
					appendIdx(i1);
					appendIdx(i2);

					materialFaceLines[matName].push_back(std::move(faceLine));
				}
			}

			// Write face groups sorted by material
			for (const auto& [matName, faceLines] : materialFaceLines)
			{
				objFile << "usemtl " << matName << "\n";
				for (const auto& line : faceLines)
				{
					objFile << line << "\n";
				}
			}

			objFile << "\n";
		}

		objFile.close();

		// Write MTL file
		std::ofstream mtlFile(mtlPath);
		if (!mtlFile.is_open())
		{
			errorMsg = "Failed to write MTL file: " + mtlPath;
			return false;
		}

		mtlFile << "# Material file exported by pieroz-directx UV Editor\n\n";

		// Default material
		mtlFile << "newmtl default\n";
		mtlFile << "Ka 0.2 0.2 0.2\n";
		mtlFile << "Kd 0.8 0.8 0.8\n";
		mtlFile << "Ks 0.0 0.0 0.0\n\n";

		for (const auto& [texPath, matName] : texToMaterial)
		{
			mtlFile << "newmtl " << matName << "\n";
			mtlFile << "Ka 0.2 0.2 0.2\n";
			mtlFile << "Kd 0.8 0.8 0.8\n";
			mtlFile << "Ks 0.0 0.0 0.0\n";

			// Write texture path relative to the OBJ directory if possible
			auto texFsPath = std::filesystem::path(texPath);
			auto relPath = std::filesystem::proximate(texFsPath, objDir);
			mtlFile << "map_Kd " << relPath.string() << "\n\n";
		}
		mtlFile.close();
		return true;
	}
	catch (const std::exception& ex)
	{
		errorMsg = "Exception during export: ";
		errorMsg += ex.what();
		return false;
	}
}