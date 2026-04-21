#pragma once
#include <string>

class Model;

// Export a Model to Wavefront OBJ + MTL format.
// Takes into account per-face texture overtrides stored on Mesh objects.
namespace ObjExporter
{
	// Exports the model to the given .obj path (the .mtl file is written next to it).
	// Return true on success, false on failure (with errorMsg set to a descriptive message).
	bool Export(const Model& model, const std::string& objPath, std::string& errorMsg);
}