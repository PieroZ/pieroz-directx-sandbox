#pragma once

#include <fstream>
#include <vector>
#include <stdexcept>
#include "Pap.h"


std::vector<PAP_Hi> LoadIamMap(const std::string& filename)
{
    constexpr size_t MAP_SIZE = 128 * 128;
    constexpr size_t HEADER_OFFSET = 8;

    std::ifstream file(filename, std::ios::binary);
    if (!file)
        throw std::runtime_error("Unable to open file" + filename);

    file.seekg(HEADER_OFFSET, std::ios::beg);

    std::vector<PAP_Hi> tiles(MAP_SIZE);

    file.read(reinterpret_cast<char*>(tiles.data()),
        MAP_SIZE * sizeof(PAP_Hi));

    if (!file)
        throw std::runtime_error("Error reading file" + filename);

    return tiles;
}