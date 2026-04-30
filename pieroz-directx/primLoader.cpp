#include "primLoader.h"
#include <fstream>
#include <stdexcept>

PrimLoadResult LoadPrimObject(const std::string& filePath)
{
	std::ifstream file(filePath, std::ios::binary);
    if (!file)
        throw std::runtime_error("Unable to open file" + filePath);

    PrimLoadResult result;
    file.read(reinterpret_cast<char*>(&result.save_type), sizeof(result.save_type));

    constexpr int PRIM_NAME_SIZE = 32;
    char buffer[PRIM_NAME_SIZE] = {};
    file.read(buffer, PRIM_NAME_SIZE);
	result.name.assign(buffer, strnlen(buffer, PRIM_NAME_SIZE));

    file.read(reinterpret_cast<char*>(&result.object), sizeof(PrimObject));

    if(!file)
		throw std::runtime_error("Unexpected end of file reading header: " + filePath);

	const int numPoints = result.object.EndPoint - result.object.StartPoint;
    if(numPoints < 0)
		throw std::runtime_error("Invalid point range in prim object: " + filePath);
    if (numPoints > 0)
    {
        result.points.resize(static_cast<size_t>(numPoints));
		file.read(reinterpret_cast<char*>(result.points.data()),
            numPoints * sizeof(PrimPoint));
    }

	const int numFaces3 = result.object.EndFace3 - result.object.StartFace3;
    if (numFaces3 > 0)
    {
		result.faces3.resize(static_cast<size_t>(numFaces3));
        file.read(reinterpret_cast<char*>(result.faces3.data()),
			numFaces3 * sizeof(PrimFace3));
    }

	const int numFaces4 = result.object.EndFace4 - result.object.StartFace4;
    if (numFaces4 > 0)
    {
        result.faces4.resize(static_cast<size_t>(numFaces4));
		file.read(reinterpret_cast<char*>(result.faces4.data()),
			numFaces4 * sizeof(PrimFace4));
    }

    return result;
}