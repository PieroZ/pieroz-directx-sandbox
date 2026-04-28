#pragma once

#include <string>
#include <cstdio>

struct TexturePaths
{
    std::string res32;
    std::string res64;
    std::string res128;
};

inline int get_page(uint16_t texture)
{
    return texture & 0x3ff;
}

TexturePaths get_texture_paths(
    int page,
    const std::string& world_dir,
    const std::string& shared_dir,
    const std::string& inside_dir,
    const std::string& people_dir,
    const std::string& prims_dir,
    const std::string& people_dir2)
{
    auto make = [](const std::string& dir, int p)
        {
            char buf32[64], buf64[64], buf128[64];

            sprintf_s(buf32, sizeof(buf32), "%stex%03d.png", dir.c_str(), p);
            sprintf_s(buf64, sizeof(buf64), "%stex%03dhi.png", dir.c_str(), p);
            sprintf_s(buf128, sizeof(buf128), "%stex%03dto.png", dir.c_str(), p);

            return TexturePaths{ buf32, buf64, buf128 };
        };

    if (page < 64 * 4)
        return make(world_dir, page);

    else if (page < 64 * 8)
        return make(shared_dir, page);

    else if (page < 64 * 9)
        return make(inside_dir, page - 64 * 8);

    else if (page < 64 * 11)
        return make(people_dir, page - 64 * 9);

    else if (page < 64 * 18)
        return make(prims_dir, page - 64 * 11);

    else if (page < 64 * 21)
        return make(people_dir2, page - 64 * 18);

    return {};
}