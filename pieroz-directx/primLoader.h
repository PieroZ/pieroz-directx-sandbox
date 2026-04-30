#pragma once
#include "prim.h"
#include <string>
#include <vector>


struct PrimLoadResult
{
	std::uint16_t save_type;
	std::string name;
	PrimObject object;
	std::vector<PrimPoint> points;
	std::vector<PrimFace3> faces3;
	std::vector<PrimFace4> faces4;
};


PrimLoadResult LoadPrimObject(const std::string& filePath);
