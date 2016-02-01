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

// This file is part of the VM ABI. Changes here may cause incompatibilities.

#ifndef __TR_TYPES_H
#define __TR_TYPES_H

// XreaL BEGIN
#define MAX_REF_LIGHTS     1024
#define MAX_REF_ENTITIES   1023 // can't be increased without changing drawsurf bit packing
#define MAX_BONES          256
#define MAX_WEIGHTS        4 // GPU vertex skinning limit, never change this without rewriting many GLSL shaders
// XreaL END

#define MAX_CORONAS        32 //----(SA)  not really a reason to limit this other than trying to keep a reasonable count
#define MAX_DLIGHTS        32 // can't be increased, because bit flags are used on surfaces
#define MAX_ENTITIES       MAX_REF_ENTITIES // RB: for compatibility

// renderfx flags
#define RF_MINLIGHT        0x000001 // always have some light (viewmodel, some items)
#define RF_THIRD_PERSON    0x000002 // don't draw through eyes, only mirrors (player bodies, chat sprites)
#define RF_FIRST_PERSON    0x000004 // only draw through eyes (view weapon, damage blood blob)
#define RF_DEPTHHACK       0x000008 // for view weapon Z crunching
#define RF_NOSHADOW        0x000010 // don't add stencil shadows

#define RF_LIGHTING_ORIGIN 0x000020 // use refEntity->lightingOrigin instead of refEntity->origin
// for lighting.  This allows entities to sink into the floor
// with their origin going solid, and allows all parts of a
// player to get the same lighting
#define RF_WRAP_FRAMES   0x000080 // mod the model frames by the maxframes to allow continuous
// animation without needing to know the frame count
#define RF_FORCENOLOD    0x000400
#define RF_SWAPCULL      0x000800 // swap CT_FRONT_SIDED and CT_BACK_SIDED

// refdef flags
#define RDF_NOWORLDMODEL ( 1 << 0 ) // used for player configuration screen
#define RDF_NOSHADOWS    ( 1 << 1 ) // force renderer to use faster lighting only path
#define RDF_HYPERSPACE   ( 1 << 2 ) // teleportation effect

// Rafael
#define RDF_SKYBOXPORTAL ( 1 << 3 )

//----(SA)
#define RDF_UNDERWATER   ( 1 << 4 ) // so the renderer knows to use underwater fog when the player is underwater
#define RDF_DRAWINGSKY   ( 1 << 5 )

// XreaL BEGIN
#define RDF_NOCUBEMAP    ( 1 << 7 ) // RB: don't use cubemaps
#define RDF_NOBLOOM      ( 1 << 8 ) // RB: disable bloom. useful for HUD models
// XreaL END

#define MAX_ALTSHADERS   64 // alternative shaders ('when <condition> <shader>') – selection controlled from cgame

#define GL_INDEX_TYPE GL_UNSIGNED_INT
using glIndex_t = unsigned int;

enum RegisterShaderFlags_t {
	RSF_DEFAULT           = 0x00,
	RSF_NOMIP             = 0x01,
	RSF_LIGHT_ATTENUATION = 0x02,
	RSF_NOLIGHTSCALE      = 0x04,
	RSF_SPRITE            = 0x08
};

struct polyVert_t
{
	vec3_t xyz;
	float  st[ 2 ];
	byte   modulate[ 4 ];
};

struct poly_t
{
	qhandle_t  hShader;
	int        numVerts;
	polyVert_t *verts;
};

enum class refEntityType_t
{
  RT_MODEL,
  RT_SPRITE,
  RT_PORTALSURFACE, // doesn't draw anything, just info for portals

  RT_MAX_REF_ENTITY_TYPE
};

// XreaL BEGIN

// RB: defining any of the following macros would break the compatibility to old ET mods
//#define USE_REFENTITY_NOSHADOWID 1

// RB: having bone names for each refEntity_t takes several MiBs
// in backEndData_t so only use it for debugging and development
// enabling this will show the bone names with r_showSkeleton 1

struct refBone_t
{
#if defined( REFBONE_NAMES )
	char   name[ 64 ];
#endif
	short  parentIndex; // parent index (-1 if root)
	transform_t t;
};

enum class refSkeletonType_t
{
  SK_INVALID,
  SK_RELATIVE,
  SK_ABSOLUTE
};

ALIGNED(16, struct refSkeleton_t
{
	refSkeletonType_t type; // skeleton has been reset

	unsigned short numBones;

	vec3_t            bounds[ 2 ]; // bounds of all applied animations
	vec_t             scale;

	refBone_t         bones[ MAX_BONES ];
});

// XreaL END

struct refEntity_t
{
	refEntityType_t reType;
	int             renderfx;

	qhandle_t       hModel; // opaque type outside refresh

	// most recent data
	vec3_t    lightingOrigin; // so multi-part models can be lit identically (RF_LIGHTING_ORIGIN)

	vec3_t    axis[ 3 ]; // rotation vectors
	bool  nonNormalizedAxes; // axis are not normalized, i.e. they have scale
	vec3_t    origin;
	int       frame;

	// previous data for frame interpolation
	vec3_t    oldorigin; // also used as MODEL_BEAM's "to"
	int       oldframe;
	float     backlerp; // 0.0 = current, 1.0 = old

	// texturing
	int       skinNum; // inline skin index
	qhandle_t customSkin; // nullptr for default skin
	qhandle_t customShader; // use one image for the entire thing

	// misc
	Color::Color32Bit shaderRGBA; // colors used by rgbgen entity shaders
	float shaderTexCoord[ 2 ]; // texture coordinates used by tcMod entity modifiers
	float shaderTime; // subtracted from refdef time to control effect start times

	// extra sprite information
	float radius;
	float rotation;

	// Ridah, entity fading (gibs, debris, etc)
	int   fadeStartTime, fadeEndTime;

	int   entityNum; // currentState.number, so we can attach rendering effects to specific entities (Zombie)

#if defined( USE_REFENTITY_NOSHADOWID )
	// extra light interaction information
	short noShadowID;
#endif

	int altShaderIndex;

	// KEEP SKELETON AT THE END OF THE STRUCTURE
	// it is to make a serialization hack for refEntity_t easier
	// by memcpying up to skeleton and then serializing skeleton
	refSkeleton_t skeleton;

};

// ================================================================================================

// XreaL BEGIN

enum class refLightType_t
{
  RL_OMNI, // point light
  RL_PROJ, // spot light
  RL_DIRECTIONAL, // sun light

  RL_MAX_REF_LIGHT_TYPE
};

struct refLight_t
{
	refLightType_t rlType;
//  int             lightfx;

	qhandle_t attenuationShader;

	vec3_t    origin;
	quat_t    rotation;
	vec3_t    center;
	vec3_t    color; // range from 0.0 to 1.0, should be color normalized

	float     scale; // r_lightScale if not set

	// omni-directional light specific
	float     radius;

	// projective light specific
	vec3_t   projTarget;
	vec3_t   projRight;
	vec3_t   projUp;
	vec3_t   projStart;
	vec3_t   projEnd;

	bool noShadows;
	short    noShadowID; // don't cast shadows of all entities with this id

	bool inverseShadows; // don't cast light and draw shadows by darken the scene
	// this is useful for drawing player shadows with shadow mapping
};

// XreaL END

// ================================================================================================

#define MAX_RENDER_STRINGS       8
#define MAX_RENDER_STRING_LENGTH 32

struct refdef_t
{
	int    x, y, width, height;
	float  fov_x, fov_y;
	vec3_t vieworg;
	vec3_t viewaxis[ 3 ]; // transformation matrix
	vec3_t blurVec;       // motion blur direction

	int    time; // time in milliseconds for shader effects and other time dependent rendering issues
	int    rdflags; // RDF_NOWORLDMODEL, etc

	// 1 bits will prevent the associated area from rendering at all
	byte areamask[ MAX_MAP_AREA_BYTES ];

	vec4_t  gradingWeights;
};

// XreaL BEGIN

// cg_shadows modes
enum class shadowingMode_t
{
  SHADOWING_NONE,
  SHADOWING_BLOB,
  SHADOWING_ESM16,
  SHADOWING_ESM32,
  SHADOWING_VSM16,
  SHADOWING_VSM32,
  SHADOWING_EVSM32,
};
// XreaL END

enum class textureCompression_t
{
  TC_NONE,
  TC_S3TC,
  TC_EXT_COMP_S3TC
};

// Keep the list in sdl_glimp.c:reportDriverType in sync with this
enum class glDriverType_t
{
  GLDRV_UNKNOWN = -1,
  GLDRV_ICD, // driver is integrated with window system
  // WARNING: there are tests that check for
  // > GLDRV_ICD for minidriverness, so this
  // should always be the lowest value in this
  // enum set
  GLDRV_STANDALONE, // driver is a non-3Dfx standalone driver

// XreaL BEGIN
  GLDRV_OPENGL3, // new driver system
  GLDRV_MESA, // crap
// XreaL END
};

// Keep the list in sdl_glimp.c:reportHardwareType in sync with this
enum class glHardwareType_t
{
  GLHW_UNKNOWN = -1,
  GLHW_GENERIC, // where everthing works the way it should

// XreaL BEGIN
  GLHW_ATI, // where you don't have proper GLSL support
  GLHW_ATI_DX10, // ATI Radeon HD series DX10 hardware
  GLHW_NV_DX10 // Geforce 8/9 class DX10 hardware
// XreaL END
};

/**
 * Contains variables specific to the OpenGL configuration
 * being run right now.  These are constant once the OpenGL
 * subsystem is initialized.
 */
struct glconfig_t
{
	char                 renderer_string[ MAX_STRING_CHARS ];
	char                 vendor_string[ MAX_STRING_CHARS ];
	char                 version_string[ MAX_STRING_CHARS ];
	char                 extensions_string[ MAX_STRING_CHARS * 4 ]; // TTimo - bumping, some cards have a big extension string

	int                  maxTextureSize; // queried from GL
	int                  unused;

	int                  colorBits, depthBits, stencilBits;

	glDriverType_t       driverType;
	glHardwareType_t     hardwareType;

	textureCompression_t textureCompression;
	bool             textureEnvAddAvailable;
	bool             anisotropicAvailable; //----(SA)  added
	float                maxAnisotropy; //----(SA)  added

	int      vidWidth, vidHeight;
	// aspect is the screen's physical width / height, which may be different
	// than scrWidth / scrHeight if the pixels are non-square
	float windowAspect;

	int   displayFrequency;

	// synonymous with "does rendering consume the entire screen?", therefore
	// a Win32 ICD that used CDS will have this set to TRUE
	bool isFullscreen;
	bool smpActive; // dual processor
};

// XreaL BEGIN
struct glconfig2_t
{
	bool ARBTextureCompressionAvailable;

	int      maxCubeMapTextureSize;

	bool occlusionQueryAvailable;
	int      occlusionQueryBits;

	char     shadingLanguageVersionString[ MAX_STRING_CHARS ];
	int      shadingLanguageVersion;

	int      maxVertexUniforms;
//	int             maxVaryingFloats;
	int      maxVertexAttribs;
	bool vboVertexSkinningAvailable;
	int      maxVertexSkinningBones;

	bool textureNPOTAvailable;

	bool drawBuffersAvailable;
	bool textureHalfFloatAvailable;
	bool textureFloatAvailable;
	bool textureRGAvailable;
	int      maxDrawBuffers;

	float    maxTextureAnisotropy;
	bool textureAnisotropyAvailable;

	int      maxRenderbufferSize;
	int      maxColorAttachments;
	bool framebufferPackedDepthStencilAvailable;

	bool generateMipmapAvailable;
	bool getProgramBinaryAvailable;
	bool bufferStorageAvailable;
	bool mapBufferRangeAvailable;
	bool syncAvailable;
};
// XreaL END

// =========================================
// Gordon, these MUST NOT exceed the values for SHADER_MAX_VERTEXES/SHADER_MAX_INDEXES
#define MAX_PB_VERTS    1025
#define MAX_PB_INDICIES ( MAX_PB_VERTS * 6 )

struct polyBuffer_t
{
	vec4_t    xyz[ MAX_PB_VERTS ];
	vec2_t    st[ MAX_PB_VERTS ];
	byte      color[ MAX_PB_VERTS ][ 4 ];
	int       numVerts;

	glIndex_t indicies[ MAX_PB_INDICIES ];
	int       numIndicies;

	qhandle_t shader;
};

// =========================================

#endif // __TR_TYPES_H
