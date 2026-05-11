#include "primLoader.h"
#include <fstream>
#include <stdexcept>
#include <filesystem>

PrimLoadResult LoadPrimObject(const std::string& nprimFilePath, const std::string& primFilePath)
{
    std::string filePath = nprimFilePath;

	std::ifstream file(nprimFilePath, std::ios::binary);

    if (!file)
    {
        filePath = primFilePath;

        file.open(primFilePath, std::ios::binary);

        if (!file)
        {
            throw std::runtime_error("Unable to open file: " + filePath);
        }
    }


    PrimLoadResult result;


    const std::filesystem::path path = filePath;
    const auto filename = path.stem().string();
    if (filename[0] != 'p')
    {
        file.read(reinterpret_cast<char*>(&result.save_type), sizeof(result.save_type));
    }

    
    constexpr int PRIM_NAME_SIZE = 32;
    char buffer[PRIM_NAME_SIZE] = {};
    file.read(buffer, PRIM_NAME_SIZE);
	result.name.assign(buffer, strnlen(buffer, PRIM_NAME_SIZE));

    file.read(reinterpret_cast<char*>(&result.object), sizeof(PrimObject));

    if (filename[0] == 'p')
    {
        std::uint16_t	Dummy[4];
        file.read(reinterpret_cast<char*>(Dummy), sizeof(Dummy));
        result.save_type = Dummy[3];
    }

    if(!file)
		throw std::runtime_error("Unexpected end of file reading header: " + filePath);

	const int numPoints = result.object.EndPoint - result.object.StartPoint;
    if(numPoints < 0)
		throw std::runtime_error("Invalid point range in prim object: " + filePath);
    if (numPoints > 0)
    {

        result.points.resize(static_cast<size_t>(numPoints));
        constexpr int PRIM_START_SAVE_TYPE = 5793;
        if (result.save_type- PRIM_START_SAVE_TYPE == 1)
        {
            file.read(reinterpret_cast<char*>(result.points.data()),
                numPoints * sizeof(PrimPoint));
        }
        else
        {
            for (int i = 0; i < numPoints; i++)
            {
                OldPrimPoint oldPoint;
                file.read(reinterpret_cast<char*>(&oldPoint), sizeof(OldPrimPoint));

                result.points[i].X = static_cast<std::int16_t>(oldPoint.X);
                result.points[i].Y = static_cast<std::int16_t>(oldPoint.Y);
                result.points[i].Z = static_cast<std::int16_t>(oldPoint.Z);
            }
        }
    }

    // Negate X axis
    for (auto& point : result.points)
    {
        point.X = -point.X;
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

    for(auto& f : result.faces3)
    {
		for (int i = 0; i < 3; i++)
            f.Points[i] -= result.object.StartPoint;
    }

	for (auto& f : result.faces4)
    {
        for (int i = 0; i < 4; i++)
            f.Points[i] -= result.object.StartPoint;
    }

    return result;
}