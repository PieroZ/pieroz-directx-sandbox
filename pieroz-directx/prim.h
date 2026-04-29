#pragma once

#include <cstdint>

struct PrimFace3
{
	std::uint8_t TexturePage;
	std::uint8_t DrawFlags;
	std::uint16_t Points[3];
	std::uint8_t UV[3][2];
	std::int16_t Bright[3]; // make into byte
	std::int16_t ThingIndex;
	std::uint16_t Col2;
	std::uint16_t FaceFlags;
	std::uint8_t Type; // move after bright
	std::int8_t ID;    // delete
};

struct	PrimFace4
{
    std::uint8_t TexturePage;
    std::uint8_t DrawFlags;
    std::uint16_t Points[4];
    std::uint8_t UV[4][2];

    union
    {
        std::int16_t Bright[4]; // Used for people.

        struct // We cant use a LIGHT_Col because of circluar #include problems :-(
        {
            std::uint8_t red;
            std::uint8_t green;
            std::uint8_t blue;

        } col; // Used for building faces...
    };

    std::int16_t ThingIndex;
    std::int16_t Col2;
    std::uint16_t FaceFlags;
    std::uint8_t Type; // move after bright
    std::int8_t ID;
};

struct	PrimObject
{
    std::uint16_t StartPoint;
    std::uint16_t EndPoint;
    std::uint16_t StartFace4;
    std::uint16_t EndFace4;
    std::int16_t StartFace3;
    std::int16_t EndFace3;

    std::uint8_t coltype;
    std::uint8_t damage; // How this prim gets damaged
    std::uint8_t shadowtype;
    std::uint8_t flag;
};