#pragma once
#include <fstream>
#include <string>
#include "building.h"
#include "tma.h"


//template<typename T>
//static void ReadValue(std::ifstream& file, T& value)
//{
//	file.read(reinterpret_cast<char*>(&value), sizeof(T));
//
//	if (!file)
//		throw std::runtime_error("File read error");
//}


void TEXTURE_fix_texture_styles(tma& tma_result)
{
    constexpr int TEXTURE_NORM_SIZE = 32;
    constexpr int TEXTURE_NORM_SQUARES = 8;
    signed long	style, piece;

    signed long page;

    signed long av_u;
    signed long av_v;

    signed long base_u;
    signed long base_v;

    for (style = 0; style < 200; style++)
    {
        for (piece = 0; piece < 5; piece++)
        {
            base_u = tma_result.textures_xy[style][piece].Tx * 32;
            base_v = tma_result.textures_xy[style][piece].Ty * 32;

            av_u = base_u / TEXTURE_NORM_SIZE;
            av_v = base_v / TEXTURE_NORM_SIZE;

            page = av_u + av_v * TEXTURE_NORM_SQUARES + tma_result.textures_xy[style][piece].Page * TEXTURE_NORM_SQUARES * TEXTURE_NORM_SQUARES;
            tma_result.dx_textures_xy[style][piece].Page = page;
            tma_result.dx_textures_xy[style][piece].Flip = tma_result.textures_xy[style][piece].Flip;
        }
    }
}


tma load_texture_styles(unsigned char editor, unsigned char world)
{
    tma tma_result;

    unsigned short temp;
    unsigned short temp2;

    std::string filename =
        "UC-data/textures/world" +
        std::to_string(world) +
        "/style.tma";

    std::ifstream file(filename, std::ios::binary);

    if (!file)
    {
        throw std::runtime_error("Unable to open file: " + filename);
    }


    file.read(reinterpret_cast<char*>(&tma_result.save_type), sizeof(tma_result.save_type));


    if (tma_result.save_type > 1)
    {
        //
        // Old TextureInfo block
        //
        if (tma_result.save_type < 4)
        {
            file.read(reinterpret_cast<char*>(&temp), 2);

            file.seekg(
                sizeof(TextureInfo) * 8 * 8 * temp,
                std::ios::cur
            );
        }

        //
        // textures_xy
        //
        file.read(reinterpret_cast<char*>(&temp), 2);
        file.read(reinterpret_cast<char*>(&temp2), 2);

        // temp = 200
        // temp2 = usually 5 or 8

        tma_result.textures_xy.resize(temp);
        tma_result.dx_textures_xy.resize(temp);

        for (auto& row : tma_result.textures_xy)
        {
            row.resize(5);
        }


        for (auto& row : tma_result.dx_textures_xy)
        {
            row.resize(5);
        }
        if (tma_result.save_type < 5)
        {
            //
            // Old format:
            // first 3 entries skipped
            // last 5 loaded
            //
            for (int c0 = 0; c0 < temp; c0++)
            {
                file.seekg(sizeof(TXTY) * 3, std::ios::cur);

                file.read(
                    reinterpret_cast<char*>(
                        tma_result.textures_xy[c0].data()
                        ),
                    sizeof(TXTY) * (temp2 - 3)
                );
            }
        }
        else
        {
            //
            // New format
            //
            if (temp2 != 5)
            {
                throw std::runtime_error("Unexpected textures_xy width");
            }

            for (int c0 = 0; c0 < temp; c0++)
            {
                file.read(
                    reinterpret_cast<char*>(
                        tma_result.textures_xy[c0].data()
                        ),
                    sizeof(TXTY) * temp2
                );
            }
        }

        //
        // texture_style_names
        //
        file.read(reinterpret_cast<char*>(&temp), 2);
        file.read(reinterpret_cast<char*>(&temp2), 2);

        // temp = 200
        // temp2 = 21

        if (editor)
        {
            tma_result.texture_style_names.resize(temp);

            for (int i = 0; i < temp; i++)
            {
                std::vector<char> buffer(temp2);

                file.read(buffer.data(), temp2);

                tma_result.texture_style_names[i] =
                    std::string(buffer.data());
            }
        }
        else
        {
            file.seekg(temp * temp2, std::ios::cur);
        }

        //
        // textures_flags
        //
        if (tma_result.save_type > 2)
        {
            file.read(reinterpret_cast<char*>(&temp), 2);
            file.read(reinterpret_cast<char*>(&temp2), 2);

            tma_result.textures_flags.resize(temp);

            for (auto& row : tma_result.textures_flags)
            {
                row.resize(5);
            }

            if (tma_result.save_type < 5)
            {
                for (int c0 = 0; c0 < temp; c0++)
                {
                    file.seekg(3, std::ios::cur);

                    file.read(
                        reinterpret_cast<char*>(
                            tma_result.textures_flags[c0].data()
                            ),
                        sizeof(unsigned char) * (temp2 - 3)
                    );
                }
            }
            else
            {
                if (temp2 != 5)
                {
                    //
                    // fallback
                    //
                    for (int c0 = 0; c0 < temp; c0++)
                    {
                        for (int c1 = 0; c1 < 5; c1++)
                        {
                            //tma_result.textures_flags[c0][c1] = POLY_GT;
                        }
                    }

                    return tma_result;
                }

                for (int c0 = 0; c0 < temp; c0++)
                {
                    file.read(
                        reinterpret_cast<char*>(
                            tma_result.textures_flags[c0].data()
                            ),
                        sizeof(unsigned char) * temp2
                    );
                }
            }
        }
    }

    TEXTURE_fix_texture_styles(tma_result);

    return tma_result;
}