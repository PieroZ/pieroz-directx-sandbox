#pragma once
#include "Pap.h"
#include "iam.h"
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
            tile["height"] = static_cast<float>(t.Height / 4.0f);
            tile["alt"] = static_cast<int>(t.Alt);

            tile["texture"] = paths.res64;
            tile["rotation"] = trot;
            tile["flip"] = rtflip;
            tile["tsize"] = tsize;

            j["tiles"].push_back(tile);
        }
    }

    return j;
}

json BuildMapJson(const iam& iamData)
{
    json j = BuildMapJson(iamData.pap_hi, iamData.texture_set);

    // Add OB_Ob prim objects
    int id = 0;
    j["prims"] = json::array();
    for (size_t i = 0; i < iamData.OB_mapwho.size(); ++i)
    {
        if (iamData.OB_mapwho[i].index != 0)
        {
            for (int kk = 0; kk < iamData.OB_mapwho[i].num; kk++)
            {
                int index = iamData.OB_mapwho[i].index + kk;
                const auto& ob = iamData.OB_ob[index];
                if (ob.prim == 0 && ob.x == 0 && ob.z == 0)
                    continue; // skip empty entries

                json primJson;
                primJson["id"] = id++;
                primJson["prim"] = ob.prim;
                primJson["x"] = (i / 32) * 4;
                primJson["y"] = ob.y / 512.f + (ob.y % 256) / 512.f ;
                primJson["z"] = (i % 32) * 4;
                primJson["xOffset"] = (ob.x / 64.0f) - 0.5f;
                primJson["zOffset"] = (ob.z / 64.0f) - 0.5f;
                primJson["yaw"] = ob.yaw;
                primJson["flags"] = ob.flags;
                primJson["InsideIndex"] = ob.InsideIndex;
                j["prims"].push_back(primJson);
            }
        }
    }
    //for (size_t i = 0; i < iamData.OB_ob.size(); ++i)
    //{
    //    const auto& ob = iamData.OB_ob[i];
    //    if (ob.prim == 0 && ob.x == 0 && ob.z == 0)
    //        continue; // skip empty entries

    //    json primJson;
    //    primJson["prim"] = ob.prim;
    //    primJson["x"] = ob.x;
    //    primJson["y"] = ob.y;
    //    primJson["z"] = ob.z;
    //    primJson["yaw"] = ob.yaw;
    //    primJson["flags"] = ob.flags;
    //    primJson["InsideIndex"] = ob.InsideIndex;
    //    j["prims"].push_back(primJson);
    //}

    return j;
}

void SaveIamToJson(const json& j, const std::string& filename)
{
    std::ofstream out(filename);
    out << j.dump(2); // pretty print
}