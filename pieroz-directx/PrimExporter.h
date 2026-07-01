#pragma once
#include <string>

class Model;

namespace PrimExporter
{
	bool Export(const Model& model, const std::string& nrpimPath, float scale, std::string& errorMsg);
}