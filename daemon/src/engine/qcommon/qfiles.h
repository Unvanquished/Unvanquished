/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
Copyright (C) 2010 Robert Beckebans

This file is part of the Daemon GPL Source Code (Daemon Source Code).

Daemon Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Daemon Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Daemon Source Code is also subject to certain additional terms.
You should have received a copy of these additional terms immediately following the
terms and conditions of the GNU General Public License which accompanied the Daemon
Source Code.  If not, please request a copy in writing from id Software at the address
below.

If you have questions concerning this license or the applicable additional terms, you
may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville,
Maryland 20850 USA.

===========================================================================
*/

#ifndef __QFILES_H__
#define __QFILES_H__

//
// qfiles.h: quake file formats
// This file must be identical in the quake and utils directories
//

// surface geometry should not exceed these limits
#define SHADER_MAX_VERTEXES  10000 // Arnout: 1024+1 (1 buffer for RB_EndSurface overflow check) // JPW NERVE was 4000, 1000 in q3ta
#define SHADER_MAX_INDEXES   ( 6 * SHADER_MAX_VERTEXES )
#define SHADER_MAX_TRIANGLES ( SHADER_MAX_INDEXES / 3 )

//
// RB: DON'T USE MAX_QPATH HERE SO WE CAN INCREASE IT !!
//

/*
========================================================================

QVM files

========================================================================
*/

#define VM_MAGIC      0x12721444
#define VM_MAGIC_VER2 0x12721445
struct vmHeader_t
{
	int vmMagic;

	int instructionCount;

	int codeOffset;
	int codeLength;

	int dataOffset;
	int dataLength;
	int litLength; // ( dataLength - litLength ) should be byteswapped on load
	int bssLength; // zero filled memory appended to datalength

	//!!! below here is VM_MAGIC_VER2 !!!
	int jtrgLength; // number of jump table targets
};

/*
========================================================================

TGA files are used for 24/32 bit images

========================================================================
*/

struct TargaHeader
{
	unsigned char  id_length, colormap_type, image_type;
	unsigned short colormap_index, colormap_length;
	unsigned char  colormap_size;
	unsigned short x_origin, y_origin, width, height;
	unsigned char  pixel_size, attributes;
};

/*
========================================================================

.MD3 triangle model file format

========================================================================
*/

#define MD3_IDENT         ( ( '3' << 24 ) + ( 'P' << 16 ) + ( 'D' << 8 ) + 'I' )
#define MD3_VERSION       15

// limits
#define MD3_MAX_LODS      4
#define MD3_MAX_SURFACES  32 // per model

// vertex scales
#define MD3_XYZ_SCALE     ( 1.0 / 64 )

struct md3Frame_t
{
	vec3_t bounds[ 2 ];
	vec3_t localOrigin;
	float  radius;
	char   name[ 16 ];
};

struct md3Tag_t
{
	char   name[ 64 ]; // tag name
	vec3_t origin;
	vec3_t axis[ 3 ];
};

/*
** md3Surface_t
**
** CHUNK      SIZE
** header     sizeof( md3Surface_t )
** shaders      sizeof( md3Shader_t ) * numShaders
** triangles[0]   sizeof( md3Triangle_t ) * numTriangles
** st       sizeof( md3St_t ) * numVerts
** XyzNormals   sizeof( md3XyzNormal_t ) * numVerts * numFrames
*/
struct md3Surface_t
{
	int  ident; //

	char name[ 64 ]; // polyset name

	int  flags;
	int  numFrames; // all surfaces in a model should have the same

	int  numShaders; // all surfaces in a model should have the same
	int  numVerts;

	int  numTriangles;
	int  ofsTriangles;

	int  ofsShaders; // offset from start of md3Surface_t
	int  ofsSt; // texture coords are common for all frames
	int  ofsXyzNormals; // numVerts * numFrames

	int  ofsEnd; // next surface follows
};

struct md3Shader_t
{
	char name[ 64 ];
	int  shaderIndex; // for in-game use
};

struct md3Triangle_t
{
	int indexes[ 3 ];
};

struct md3St_t
{
	float st[ 2 ];
};

struct md3XyzNormal_t
{
	short xyz[ 3 ];
	unsigned short normal;
};

struct md3Header_t
{
	int  ident;
	int  version;

	char name[ 64 ]; // model name

	int  flags;

	int  numFrames;
	int  numTags;
	int  numSurfaces;

	int  numSkins;

	int  ofsFrames; // offset for first frame
	int  ofsTags; // numFrames * numTags
	int  ofsSurfaces; // first surface, others follow

	int  ofsEnd; // end of file
};


/*
==============================================================================

  .BSP file format

==============================================================================
*/

#define BSP_VERSION_Q3 46
#define BSP_VERSION    47

#define MAX_MAP_FOGS         0x100

// the editor uses these predefined yaw angles to orient entities up or down
#define ANGLE_UP             -1
#define ANGLE_DOWN           -2

#define MAX_WORLD_COORD      ( 128 * 1024 )
#define MIN_WORLD_COORD      ( -128 * 1024 )

//=============================================================================

struct lump_t
{
	int fileofs, filelen;
};

#define LUMP_ENTITIES     0
#define LUMP_SHADERS      1
#define LUMP_PLANES       2
#define LUMP_NODES        3
#define LUMP_LEAFS        4
#define LUMP_LEAFSURFACES 5
#define LUMP_LEAFBRUSHES  6
#define LUMP_MODELS       7
#define LUMP_BRUSHES      8
#define LUMP_BRUSHSIDES   9
#define LUMP_DRAWVERTS    10
#define LUMP_DRAWINDEXES  11
#define LUMP_FOGS         12
#define LUMP_SURFACES     13
#define LUMP_LIGHTMAPS    14
#define LUMP_LIGHTGRID    15
#define LUMP_VISIBILITY   16
#define HEADER_LUMPS      17

struct dheader_t
{
	int    ident;
	int    version;

	lump_t lumps[ HEADER_LUMPS ];
};

struct dmodel_t
{
	float mins[ 3 ], maxs[ 3 ];
	int   firstSurface, numSurfaces;
	int   firstBrush, numBrushes;
};

struct dshader_t
{
	char shader[ 64 ];
	int  surfaceFlags;
	int  contentFlags;
};

// planes x^1 is always the opposite of plane x

struct dplane_t
{
	float normal[ 3 ];
	float dist;
};

struct dnode_t
{
	int planeNum;
	int children[ 2 ]; // negative numbers are -(leafs+1), not nodes
	int mins[ 3 ]; // for frustom culling
	int maxs[ 3 ];
};

struct dleaf_t
{
	int cluster; // -1 = opaque cluster (do I still store these?)
	int area;

	int mins[ 3 ]; // for frustum culling
	int maxs[ 3 ];

	int firstLeafSurface;
	int numLeafSurfaces;

	int firstLeafBrush;
	int numLeafBrushes;
};

struct dbrushside_t
{
	int planeNum; // positive plane side faces out of the leaf
	int shaderNum;
};

struct dbrush_t
{
	int firstSide;
	int numSides;
	int shaderNum; // the shader that determines the contents flags
};

struct dfog_t
{
	char shader[ 64 ];
	int  brushNum;
	int  visibleSide; // the brush side that ray tests need to clip against (-1 == none)
};

// light grid
struct dgridPoint_t
{
	byte ambient[ 3 ];
	byte directed[ 3 ];
	byte latLong[ 2 ];
};

struct drawVert_t
{
	vec3_t xyz;
	float  st[ 2 ];
	float  lightmap[ 2 ];
	vec3_t normal;
	byte   color[ 4 ];
};

enum class mapSurfaceType_t : int
{
  MST_PLANAR = 1,
  MST_PATCH,
  MST_TRIANGLE_SOUP,
  MST_FLARE,
  MST_FOLIAGE
};

struct dsurface_t
{
	int    shaderNum;
	int    fogNum;
	mapSurfaceType_t    surfaceType;

	int    firstVert;
	int    numVerts; // ydnar: num verts + foliage origins (for cleaner lighting code in q3map)

	int    firstIndex;
	int    numIndexes;

	int    lightmapNum;
	int    lightmapX, lightmapY;
	int    lightmapWidth, lightmapHeight;

	vec3_t lightmapOrigin;
	vec3_t lightmapVecs[ 3 ]; // for patches, [0] and [1] are lodbounds

	int    patchWidth; // ydnar: num foliage instances
	int    patchHeight; // ydnar: num foliage mesh verts
};

#endif
