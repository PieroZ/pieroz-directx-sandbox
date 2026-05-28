#pragma once
#include "Pap.h"
#include <vector>
#include <array>

struct	LoadGameThing
{
	unsigned short	Type;
	unsigned short	SubStype;

	signed long	X;
	signed long	Y;
	signed long	Z;
	unsigned long	Flags;

	unsigned short	IndexOther;
	unsigned short	AngleX;

	unsigned short	AngleY;
	unsigned short	AngleZ;

	unsigned long	Dummy[4];
};

struct DBuilding
{
	signed long	X, Y, Z;
	unsigned short	StartFacet;
	unsigned short	EndFacet;
	unsigned short	Walkable;
	unsigned char	Counter[2];
	unsigned short	Padding;
	unsigned char	Ware;		// If this building is a warehouse, this is an index into the WARE_ware[] array
	unsigned char	Type;
};

struct DFacet
{
	unsigned char	FacetType;
	unsigned char	Height;
	unsigned char	x[2];		// these are bytes because they are grid based 
	signed short	Y[2];
	unsigned char	z[2];		// these are bytes because they are grid based 
	unsigned short	FacetFlags;
	unsigned short	StyleIndex;
	unsigned short	Building;
	unsigned short	DStorey;
	unsigned char	FHeight;
	unsigned char	BlockHeight;
	unsigned char	Open;				// How open or closed a STOREY_TYPE_OUTSIDE_DOOR is.
	unsigned char   Dfcache;			// Index into NIGHT_dfcache[] or NULL...
	unsigned char	Shake;				// When a fence has been hit hard by something.
	unsigned char	CutHole;
	unsigned char	Counter[2];
};

struct	DStorey
{
	unsigned short	Style; //replacement style           // maybe this could be a byte
	unsigned short	Index; //Index to painted info
	signed char	Count; //+ve is a style  //-ve is a  //get rid of this
	unsigned char	BloodyPadding;
};

struct	InsideStorey
{
	unsigned char	MinX;           // bounding rectangle of floor
	unsigned char	MinZ;
	unsigned char	MaxX;
	unsigned char	MaxZ;
	unsigned short	InsideBlock;    // index into inside_block (block of data of size bounding rect) data is room numbers 1..15 top 4 bits reserved
	unsigned short	StairCaseHead;  // link list of stair structures for this floor
	unsigned short	TexType;		// Inside style to use for floor
	unsigned short	FacetStart;     // index into facets that make up this building
	unsigned short	FacetEnd;		// Facet After last used facet for inside the floor
	signed short	StoreyY;	    // Y co-ord could come in handy
	unsigned short	Building;
	unsigned short	Dummy[2];
};

struct	Staircase
{
	unsigned char	X, Z;         // pos of staircase
	unsigned char	Flags;       // flags for direction + up or down or both
	unsigned char	ID;          // padding
	signed short	NextStairs;  // link to next stair structure
	signed short	DownInside;  // link to next insidestorey for going downstairs
	signed short	UpInside;  	 // link to next InsideStorey for going upstairs
};

struct	DWalkable
{
	unsigned short	StartPoint;	// Unused nowadays
	unsigned short	EndPoint;  	// Unused nowadays
	unsigned short	StartFace3;	// Unused nowadays
	unsigned short	EndFace3;  	// Unused nowadays

	unsigned short	StartFace4;	// These are indices into the roof faces
	unsigned short	EndFace4;

	unsigned char	X1;
	unsigned char	Z1;
	unsigned char	X2;
	unsigned char	Z2;
	unsigned char	Y;
	unsigned char	StoreyY;
	unsigned short	Next;
	unsigned short	Building;
};

struct	RoofFace4
{
	//	unsigned short	TexturePage; //could use the texture on the floor
	signed short	Y;
	signed char	DY[3];
	unsigned char	DrawFlags;
	unsigned char	RX;
	unsigned char	RZ;
	signed short	Next; //link list of walkables off floor 

};


typedef struct
{
	signed short y;
	unsigned char x;
	unsigned char z;
	unsigned char prim;
	unsigned char yaw;
	unsigned char flags;
	unsigned char InsideIndex;

} OB_Ob;

typedef struct
{
	unsigned short index : 11;
	unsigned short num : 5;

} OB_Mapwho;

struct iam
{
	static constexpr int PAP_SIZE_HI = 128;
	static constexpr int MAP_SIZE = PAP_SIZE_HI * PAP_SIZE_HI;
	unsigned int save_type;
	unsigned int ob_size;
	std::vector<PAP_Hi> pap_hi = std::vector<PAP_Hi>(MAP_SIZE);
	unsigned short temp;
	std::vector<LoadGameThing> map_thing;
	signed short next_dbuilding = 1;
	signed short next_dfacet = 1;
	signed short next_dstyle = 1;
	// save_type >= 17
	signed short next_paint_mem = 1;
	signed short next_dstorey = 1;
	//
	std::vector<DBuilding> dbuildings;
	std::vector<DFacet> dfacets;
	std::vector<signed short> dstyles;
	std::vector<unsigned char> paint_mem;
	std::vector<DStorey> dstoreys;
	// save_type >=21
	unsigned short next_inside_storey = 1;
	unsigned short next_inside_stair = 1;
	signed long next_inside_block = 1;
	std::vector< InsideStorey>inside_storeys;
	std::vector< Staircase>inside_stairs;
	std::vector< unsigned char>inside_block;
	//
	signed short next_dwalkable = 1;
	signed short next_roof_face4 = 1;
	std::vector< DWalkable>dwalkables;
	std::vector< RoofFace4>roof_faces4;

	// save_type >= 23
	signed long OB_ob_upto = 1;
	std::vector<OB_Ob> OB_ob;
	//
	static constexpr int OB_SIZE = 32;
	static constexpr int OB_MAPWHO_ELEMENTS_COUNT = OB_SIZE * OB_SIZE;
	std::vector< OB_Mapwho> OB_mapwho;

	signed long texture_set;
	std::vector<std::array<unsigned short, 5>> data = std::vector<std::array<unsigned short, 5>>(2 * 200);
};