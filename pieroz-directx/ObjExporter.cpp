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
	struct MeshExportData
	{
		const Mesh* pMesh;
		dx::XMFLOAT4X4 worldTransform;
	};

	// Recursively collect all meshes in the model with their world transforms
	void CollectMeshes(
		const Node& node,
		dx::FXMMATRIX accumulatedTransform,
		std::vector<MeshExportData>& out)
	{
		const auto nodeTransform =
			dx::XMLoadFloat4x4(&node.GetAppliedTransform()) *
			dx::XMLoadFloat4x4(&node.GetBaseTransform()) *
			accumulatedTransform;

		for (const auto* pMesh : node.GetMeshPtrs())
		{
			MeshExportData d;
			d.pMesh = pMesh;
			dx::XMStoreFloat4x4(&d.worldTransform, nodeTransform);
			out.push_back(d);
		}

		for (const auto& child : node.GetChildren())
		{
			CollectMeshes(*child, nodeTransform, out);
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
		objFile << "mtlib " << mtlName << "\n\n";

		// Collect all meshes with transforms
		std::vector<MeshExportData> meshes;
		CollectMeshes(model.GetRootNode(), dx::XMMatrixIdentity(), meshes);

		// Maps: texture path -> material name
		std::unordered_map<std::string, std::string> texToMaterial;
		int matCounter = 0;
		auto getOrCreateMaterial = [&](const std::string& texPath) -> std::string
			{
				if (texToMaterial.empty())
					return "default";
				auto it = texToMaterial.find(texPath);
				if (it != texToMaterial.end())
					return it->second;
				std::string name = "mat_" + SanitizeMaterialName(texPath) + "_" + std::to_string(matCounter++);
				texToMaterial[texPath] = name;
				return name;
			};
		
		// Global vertex offset (OBJ indices are 1-based and global)
		size_t globalVertexOffset = 1;

		for (size_t mi = 0; mi < meshes.size(); mi++)
		{
			const auto& meshData = meshes[mi];
			const auto& pMesh = meshData.pMesh;
			const auto worldMat = dx::XMLoadFloat4x4(&meshData.worldTransform);
			// Inverse-transpose for normals
			const auto worldMatInvT = dx::XMMatrixTranspose(dx::XMMatrixInverse(nullptr, worldMat));

			const auto& positions = pMesh->GetCpuPositions();
			const auto& indices = pMesh->GetCpuIndices();
			const auto& uvs = pMesh->GetCpuUVs();
			const auto& normals = pMesh->GetCpuNormals();
			const bool hasUVs = pMesh->HasUVs();
			const bool hasNormals = !normals.empty();

			objFile << "# Mesh " << mi << " (" << positions.size() << " vertices, " << indices.size() / 3 << " faces)\n";
			objFile << "o Mesh_" << mi << "\n";

			// Write vertices (transformed to world space)
			for (size_t vi = 0; vi < positions.size(); vi++)
			{
				dx::XMFLOAT3 wp;
				dx::XMStoreFloat3(&wp, dx::XMVector3TransformCoord(dx::XMLoadFloat3(&positions[vi]), worldMat));
				objFile << "v " << wp.x << " " << wp.y << " " << wp.z << "\n";
			}

			// Write texture coords
			if(hasUVs)
			{
				for (size_t vi = 0; vi < uvs.size(); vi++)
				{
					// OBJ has Y up (1-v), but since we export as-is, keep original
					objFile << "vt " << uvs[vi].x << " " << uvs[vi].y << "\n";
				}
			}

			// Write normals (transformed)
			if (hasNormals)
			{
				for (size_t vi = 0; vi < normals.size(); vi++)
				{
					dx::XMFLOAT3 wn;
					dx::XMVECTOR n = dx::XMLoadFloat3(&normals[vi]);
					n = dx::XMVector3Normalize(dx::XMVector3TransformNormal(n, worldMat));
					dx::XMStoreFloat3(&wn, n);
					objFile << "vn " << wn.x << " " << wn.y << " " << wn.z << "\n";
				}
			}

			// Group faces by material
			const std::string defaultTex = pMesh->GetDefaultDiffuseTexturePath();
			const auto& overrides = pMesh->GetFaceTextureOverrides();
			const size_t numFaces = indices.size() / 3;

			// Collect faces per material
			std::unordered_map<std::string, std::vector<size_t>> materialFaces;
			for (size_t fi = 0; fi < numFaces; fi++)
			{
				auto oit = overrides.find(fi);
				const std::string& tex = (oit != overrides.end()) ? oit->second : defaultTex;
				materialFaces[tex].push_back(fi);
			}

			// Write face groups
			for (const auto& [texPath, faces] : materialFaces)
			{
				const std::string matName = getOrCreateMaterial(texPath);
				objFile << "usemtl " << matName << "\n";
				for (size_t fi : faces)
				{
					const size_t i0 = indices[fi * 3 + 0];
					const size_t i1 = indices[fi * 3 + 1];
					const size_t i2 = indices[fi * 3 + 2];

					if (hasUVs && hasNormals)
					{
						objFile << "f "
							<< (globalVertexOffset + i0) << "/" << (globalVertexOffset + i0) << "/" << (globalVertexOffset + i0) << " "
							<< (globalVertexOffset + i1) << "/" << (globalVertexOffset + i1) << "/" << (globalVertexOffset + i1) << " "
							<< (globalVertexOffset + i2) << "/" << (globalVertexOffset + i2) << "/" << (globalVertexOffset + i2) << "\n";
					}
					else if (hasUVs)
					{
						objFile << "f "
							<< (globalVertexOffset + i0) << "/" << (globalVertexOffset + i0) << " "
							<< (globalVertexOffset + i1) << "/" << (globalVertexOffset + i1) << " "
							<< (globalVertexOffset + i2) << "/" << (globalVertexOffset + i2) << "\n";
					}
					else
					{
						objFile << "f "
							<< (globalVertexOffset + i0) << " "
							<< (globalVertexOffset + i1) << " "
							<< (globalVertexOffset + i2) << "\n";
					}
				}
			}

			globalVertexOffset += positions.size();
			objFile << "\n";;
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