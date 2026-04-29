#include "primLoader.h"
#include <fstream>

bool load_prim_object(const std::string& filePath, PrimObject& primObject, std::vector<PrimFace3>& faces3, std::vector<PrimFace4>& faces4)
{
    std::uint16_t save_type;
	const int PRIM_NAME_SIZE = 32; 
	char buffer[PRIM_NAME_SIZE];

	std::ifstream file(filePath, std::ios::binary);
    if (!file)
        throw std::runtime_error("Unable to open file" + filePath);

	file.read(reinterpret_cast<char*>(&save_type), sizeof(save_type));
	file.read(buffer, PRIM_NAME_SIZE);
	file.read(reinterpret_cast<char*>(&primObject), sizeof(primObject));
}
