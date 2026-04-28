#pragma once
#include <cstdint>

#pragma pack(push, 1)
struct PAP_Hi
{
    uint16_t Texture;
    uint16_t Flags;
    int8_t   Alt;
    int8_t   Height;
};
#pragma pack(pop)