#pragma once
#include <vector>


struct	DXTXTY
{
	unsigned short	Page;
	unsigned short	Flip;

};
struct	TXTY
{
	unsigned char	Page, Tx, Ty, Flip;

};

struct tma
{
    signed long save_type = 1;

    std::vector<std::vector<TXTY>> textures_xy;
    std::vector<std::vector<DXTXTY>> dx_textures_xy;
    std::vector<std::string> texture_style_names;
    std::vector<std::vector<unsigned char>> textures_flags;
};

struct	TextureInfo
{
	unsigned char	Type;
	unsigned char	SubType;
};
