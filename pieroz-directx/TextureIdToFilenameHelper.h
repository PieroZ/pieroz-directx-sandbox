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
    const std::string& people_dir2);