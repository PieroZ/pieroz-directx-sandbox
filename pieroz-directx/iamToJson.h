#pragma once
#include "Pap.h"
#include "json.hpp"
#include "TextureIdToFilenameHelper.h"
#include <vector>

using json = nlohmann::json;

json BuildMapJson(const std::vector<PAP_Hi>& tiles, int worldNo)
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

            //int page = get_page(t.Texture);
            int num = t.Texture & 0x3ff;
            int trot = (t.Texture >> 0xa) & 0x3;
            int rtflip  = (t.Texture >> 0xc) & 0x3;
            int tsize  = (t.Texture >> 0xe) & 0x3;

            auto paths = get_texture_paths(
                num,
                "UC-data/textures/world" + std::to_string(worldNo) + "/",
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
            tile["rotation"] = trot;
            tile["flip"] = rtflip;
            tile["tsize"] = tsize;

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