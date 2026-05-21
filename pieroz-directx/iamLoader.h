#pragma once

#include <fstream>
#include <vector>
#include <stdexcept>
#include "iam.h"
#include "Pap.h"

std::vector<PAP_Hi> LoadPAPFromIamMap(const std::string& filename)
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



template<typename T>
static void ReadValue(std::ifstream& file, T& value)
{
    file.read(reinterpret_cast<char*>(&value), sizeof(T));

    if (!file)
        throw std::runtime_error("File read error");
}

template<typename T>
static void ReadVector(std::ifstream& file, std::vector<T>& vec, size_t count)
{
    vec.resize(count);

    if (count == 0)
        return;

    file.read(reinterpret_cast<char*>(vec.data()), count * sizeof(T));

    if (!file)
        throw std::runtime_error("File read error");
}

iam LoadIamMap(const std::string& filename)
{
    iam iam_result;

    std::ifstream file(filename, std::ios::binary);

    if (!file)
        throw std::runtime_error("Unable to open file: " + filename);

    // -------------------------------------------------
    // Header
    // -------------------------------------------------

    ReadValue(file, iam_result.save_type);
    ReadValue(file, iam_result.ob_size);

    // -------------------------------------------------
    // PAP_HI
    // -------------------------------------------------

    file.read(
        reinterpret_cast<char*>(iam_result.pap_hi.data()),
        iam_result.MAP_SIZE * sizeof(PAP_Hi)
    );

    if (!file)
        throw std::runtime_error("Failed reading pap_hi");

    //// -------------------------------------------------
    //// WARE_roof_tex
    //// -------------------------------------------------

    //file.read(
    //    reinterpret_cast<char*>(iam_result.WARE_roof_tex.data()),
    //    iam_result.MAP_SIZE * sizeof(unsigned short)
    //);

    //if (!file)
    //    throw std::runtime_error("Failed reading WARE_roof_tex");

    // -------------------------------------------------
    // map_thing
    // -------------------------------------------------

    ReadValue(file, iam_result.temp);

    ReadVector(
        file,
        iam_result.map_thing,
        iam_result.temp
    );

    // -------------------------------------------------
    // next values
    // -------------------------------------------------

    ReadValue(file, iam_result.next_dbuilding);
    ReadValue(file, iam_result.next_dfacet);
    ReadValue(file, iam_result.next_dstyle);

    if (iam_result.save_type >= 17)
    {
        ReadValue(file, iam_result.next_paint_mem);
        ReadValue(file, iam_result.next_dstorey);
    }

    // -------------------------------------------------
    // dbuildings / dfacets / dstyles / paint_mem / dstoreys
    // -------------------------------------------------

    ReadVector(
        file,
        iam_result.dbuildings,
        iam_result.next_dbuilding
    );

    ReadVector(
        file,
        iam_result.dfacets,
        iam_result.next_dfacet
    );

    ReadVector(
        file,
        iam_result.dstyles,
        iam_result.next_dstyle
    );

    if (iam_result.save_type >= 17)
    {
        ReadVector(
            file,
            iam_result.paint_mem,
            iam_result.next_paint_mem
        );

        ReadVector(
            file,
            iam_result.dstoreys,
            iam_result.next_dstorey
        );
    }

    // -------------------------------------------------
    // save_type >= 21
    // -------------------------------------------------

    if (iam_result.save_type >= 21)
    {
        ReadValue(file, iam_result.next_inside_storey);
        ReadValue(file, iam_result.next_inside_stair);
        ReadValue(file, iam_result.next_inside_block);

        ReadVector(
            file,
            iam_result.inside_storeys,
            iam_result.next_inside_storey
        );

        ReadVector(
            file,
            iam_result.inside_stairs,
            iam_result.next_inside_stair
        );

        ReadVector(
            file,
            iam_result.inside_block,
            iam_result.next_inside_block
        );
    }

    // -------------------------------------------------
    // dwalkables / roof_faces4
    // -------------------------------------------------

    ReadValue(file, iam_result.next_dwalkable);
    ReadValue(file, iam_result.next_roof_face4);

    ReadVector(
        file,
        iam_result.dwalkables,
        iam_result.next_dwalkable
    );

    ReadVector(
        file,
        iam_result.roof_faces4,
        iam_result.next_roof_face4
    );

    // -------------------------------------------------
    // save_type >= 23
    // -------------------------------------------------

    if (iam_result.save_type >= 23)
    {
        ReadValue(file, iam_result.OB_ob_upto);

        ReadVector(
            file,
            iam_result.OB_ob,
            iam_result.OB_ob_upto
        );
    }

    // -------------------------------------------------
    // OB_mapwho
    // -------------------------------------------------

    ReadVector(
        file,
        iam_result.OB_mapwho,
        iam_result.OB_MAPWHO_ELEMENTS_COUNT
    );

    // -------------------------------------------------
    // texture_set
    // -------------------------------------------------

    ReadValue(file, iam_result.texture_set);

    // -------------------------------------------------
    // data
    // -------------------------------------------------

    file.read(
        reinterpret_cast<char*>(iam_result.data.data()),
        iam_result.data.size() * sizeof(std::array<unsigned short, 5>)
    );

    //if (!file)
    //    throw std::runtime_error("Failed reading data");


    return iam_result;
}