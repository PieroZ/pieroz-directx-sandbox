#pragma once
#include "Pap.h"
#include "json.hpp"
#include "TextureIdToFilenameHelper.h"
#include <vector>

using json = nlohmann::json;

json BuildMapJson(const std::vector<PAP_Hi>& tiles)
{
    json j;

    j["tileSize"] = 1.0;
    j["originX"] = 0.0;
    j["originZ"] = 0.0;
    j["tiles"] = json::array();

    const int MAP_SIZE = 128;

    for (int row = 0; row < MAP_SIZE; ++row)
    {
        for (int col = 0; col < MAP_SIZE; ++col)
        {
            const auto& t = tiles[row * MAP_SIZE + col];

            int page = get_page(t.Texture);

            auto paths = get_texture_paths(
                page,
                "UC-data/textures/world3/",
                "UC-data/textures/shared/",
                "UC-data/textures/inside/",
                "UC-data/textures/people/",
                "UC-data/textures/prims/",
                "UC-data/textures/people2/"
            );

            json tile;
            tile["col"] = col;
            tile["row"] = row;
            tile["height"] = static_cast<float>(t.Height);

            tile["texture"] = paths.res64;

            j["tiles"].push_back(tile);
        }
    }

    return j;
}

void SaveIamToJson(const json& j, const std::string& filename)
{
    std::ofstream out(filename);
    out << j.dump(2); // pretty print
}