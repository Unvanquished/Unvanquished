/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2006-2011 Robert Beckebans <trebor_7@users.sourceforge.net>

This file is part of Daemon source code.

Daemon source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Daemon source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// tr_init.c -- functions that are not called every frame
#include "tr_local.h"

#if defined(__cplusplus)
extern "C" {
#endif

glconfig_t      glConfig;
glconfig2_t     glConfig2;

#if defined(USE_D3D10)
dxGlobals_t     dx;
#else
glstate_t       glState;
#endif

float           displayAspect = 0.0f;

static void     GfxInfo_f(void);

cvar_t         *r_glCoreProfile;
cvar_t         *r_glMinMajorVersion;
cvar_t         *r_glMinMinorVersion;

cvar_t         *r_flares;
cvar_t         *r_flareSize;
cvar_t         *r_flareFade;

cvar_t         *r_railWidth;
cvar_t         *r_railCoreWidth;
cvar_t         *r_railSegmentLength;

cvar_t         *r_verbose;
cvar_t         *r_ignore;

cvar_t         *r_displayRefresh;

cvar_t         *r_znear;
cvar_t         *r_zfar;

cvar_t         *r_smp;
cvar_t         *r_showSmp;
cvar_t         *r_skipBackEnd;
cvar_t         *r_skipLightBuffer;

cvar_t         *r_ignorehwgamma;
cvar_t         *r_measureOverdraw;

cvar_t         *r_inGameVideo;
cvar_t         *r_fastsky;
cvar_t         *r_drawSun;

cvar_t         *r_lodBias;
cvar_t         *r_lodScale;
cvar_t         *r_lodTest;

cvar_t         *r_norefresh;
cvar_t         *r_drawentities;
cvar_t         *r_drawworld;
cvar_t         *r_drawpolies;
cvar_t         *r_speeds;
cvar_t         *r_novis;
cvar_t         *r_nocull;
cvar_t         *r_facePlaneCull;
cvar_t         *r_showcluster;
cvar_t         *r_nocurves;
cvar_t         *r_nobatching;
cvar_t         *r_noLightScissors;
cvar_t         *r_noLightVisCull;
cvar_t         *r_noInteractionSort;
cvar_t         *r_dynamicLight;
cvar_t         *r_staticLight;
cvar_t         *r_dynamicLightCastShadows;
cvar_t         *r_precomputedLighting;
cvar_t         *r_vertexLighting;
cvar_t         *r_compressDiffuseMaps;
cvar_t         *r_compressSpecularMaps;
cvar_t         *r_compressNormalMaps;
cvar_t         *r_heatHazeFix;
cvar_t         *r_noMarksOnTrisurfs;

cvar_t         *r_ext_compressed_textures;
cvar_t         *r_ext_occlusion_query;
cvar_t         *r_ext_texture_non_power_of_two;
cvar_t         *r_ext_draw_buffers;
cvar_t         *r_ext_vertex_array_object;
cvar_t         *r_ext_half_float_pixel;
cvar_t         *r_ext_texture_float;
cvar_t         *r_ext_stencil_wrap;
cvar_t         *r_ext_texture_filter_anisotropic;
cvar_t         *r_ext_stencil_two_side;
cvar_t         *r_ext_separate_stencil;
cvar_t         *r_ext_depth_bounds_test;
cvar_t         *r_ext_framebuffer_object;
cvar_t         *r_ext_packed_depth_stencil;
cvar_t         *r_ext_framebuffer_blit;
cvar_t         *r_extx_framebuffer_mixed_formats;
cvar_t         *r_ext_generate_mipmap;

cvar_t         *r_ignoreGLErrors;
cvar_t         *r_logFile;

cvar_t         *r_stencilbits;
cvar_t         *r_depthbits;
cvar_t         *r_colorbits;
cvar_t         *r_stereo;

cvar_t         *r_drawBuffer;
cvar_t         *r_uiFullScreen;
cvar_t         *r_shadows;
cvar_t         *r_softShadows;
cvar_t         *r_shadowBlur;

cvar_t         *r_shadowMapQuality;
cvar_t         *r_shadowMapSizeUltra;
cvar_t         *r_shadowMapSizeVeryHigh;
cvar_t         *r_shadowMapSizeHigh;
cvar_t         *r_shadowMapSizeMedium;
cvar_t         *r_shadowMapSizeLow;

cvar_t         *r_shadowMapSizeSunUltra;
cvar_t         *r_shadowMapSizeSunVeryHigh;
cvar_t         *r_shadowMapSizeSunHigh;
cvar_t         *r_shadowMapSizeSunMedium;
cvar_t         *r_shadowMapSizeSunLow;

cvar_t         *r_shadowOffsetFactor;
cvar_t         *r_shadowOffsetUnits;
cvar_t         *r_shadowLodBias;
cvar_t         *r_shadowLodScale;
cvar_t         *r_noShadowPyramids;
cvar_t         *r_cullShadowPyramidFaces;
cvar_t         *r_cullShadowPyramidCurves;
cvar_t         *r_cullShadowPyramidTriangles;
cvar_t         *r_debugShadowMaps;
cvar_t         *r_noShadowFrustums;
cvar_t         *r_noLightFrustums;
cvar_t         *r_shadowMapLuminanceAlpha;
cvar_t         *r_shadowMapLinearFilter;
cvar_t         *r_lightBleedReduction;
cvar_t         *r_overDarkeningFactor;
cvar_t         *r_shadowMapDepthScale;
cvar_t         *r_parallelShadowSplits;
cvar_t         *r_parallelShadowSplitWeight;
cvar_t         *r_lightSpacePerspectiveWarping;

cvar_t         *r_mode;
cvar_t         *r_collapseStages;
cvar_t         *r_nobind;
cvar_t         *r_singleShader;
cvar_t         *r_roundImagesDown;
cvar_t         *r_colorMipLevels;
cvar_t         *r_picmip;
cvar_t         *r_finish;
cvar_t         *r_clear;
cvar_t         *r_swapInterval;
cvar_t         *r_textureMode;
cvar_t         *r_offsetFactor;
cvar_t         *r_offsetUnits;
cvar_t         *r_forceSpecular;
cvar_t         *r_specularExponent;
cvar_t         *r_specularExponent2;
cvar_t         *r_specularScale;
cvar_t         *r_normalScale;
cvar_t         *r_normalMapping;
cvar_t         *r_wrapAroundLighting;
cvar_t         *r_halfLambertLighting;
cvar_t         *r_rimLighting;
cvar_t         *r_rimExponent;
cvar_t         *r_gamma;
cvar_t         *r_intensity;
cvar_t         *r_lockpvs;
cvar_t         *r_noportals;
cvar_t         *r_portalOnly;
cvar_t         *r_portalSky;

cvar_t         *r_subdivisions;
cvar_t         *r_stitchCurves;

cvar_t         *r_fullscreen;

cvar_t         *r_customwidth;
cvar_t         *r_customheight;
cvar_t         *r_customaspect;

cvar_t         *r_overBrightBits;
cvar_t         *r_mapOverBrightBits;

cvar_t         *r_debugSurface;
cvar_t         *r_simpleMipMaps;

cvar_t         *r_showImages;

cvar_t         *r_forceFog;
cvar_t         *r_wolfFog;
cvar_t         *r_noFog;

cvar_t         *r_forceAmbient;
cvar_t         *r_ambientScale;
cvar_t         *r_lightScale;
cvar_t         *r_debugLight;
cvar_t         *r_debugSort;
cvar_t         *r_printShaders;

cvar_t         *r_maxPolys;
cvar_t         *r_maxPolyVerts;

cvar_t         *r_showTris;
cvar_t         *r_showSky;
cvar_t         *r_showShadowVolumes;
cvar_t         *r_showShadowLod;
cvar_t         *r_showShadowMaps;
cvar_t         *r_showSkeleton;
cvar_t         *r_showEntityTransforms;
cvar_t         *r_showLightTransforms;
cvar_t         *r_showLightInteractions;
cvar_t         *r_showLightScissors;
cvar_t         *r_showLightBatches;
cvar_t         *r_showLightGrid;
cvar_t         *r_showOcclusionQueries;
cvar_t         *r_showBatches;
cvar_t         *r_showLightMaps;
cvar_t         *r_showDeluxeMaps;
cvar_t         *r_showAreaPortals;
cvar_t         *r_showCubeProbes;
cvar_t         *r_showBspNodes;
cvar_t         *r_showParallelShadowSplits;
cvar_t         *r_showDecalProjectors;

cvar_t         *r_showDeferredDiffuse;
cvar_t         *r_showDeferredNormal;
cvar_t         *r_showDeferredSpecular;
cvar_t         *r_showDeferredPosition;
cvar_t         *r_showDeferredRender;
cvar_t         *r_showDeferredLight;

cvar_t         *r_vboFaces;
cvar_t         *r_vboCurves;
cvar_t         *r_vboTriangles;
cvar_t         *r_vboShadows;
cvar_t         *r_vboLighting;
cvar_t         *r_vboModels;
cvar_t         *r_vboOptimizeVertices;
cvar_t         *r_vboVertexSkinning;
cvar_t         *r_vboDeformVertexes;
cvar_t         *r_vboSmoothNormals;

#if defined(USE_BSP_CLUSTERSURFACE_MERGING)
cvar_t         *r_mergeClusterSurfaces;
cvar_t         *r_mergeClusterFaces;
cvar_t         *r_mergeClusterCurves;
cvar_t         *r_mergeClusterTriangles;
#endif

cvar_t         *r_deferredShading;
cvar_t         *r_parallaxMapping;
cvar_t         *r_parallaxDepthScale;

cvar_t         *r_dynamicBspOcclusionCulling;
cvar_t         *r_dynamicEntityOcclusionCulling;
cvar_t         *r_dynamicLightOcclusionCulling;
cvar_t         *r_chcMaxPrevInvisNodesBatchSize;
cvar_t         *r_chcMaxVisibleFrames;
cvar_t         *r_chcVisibilityThreshold;
cvar_t         *r_chcIgnoreLeaves;

cvar_t         *r_hdrRendering;
cvar_t         *r_hdrMinLuminance;
cvar_t         *r_hdrMaxLuminance;
cvar_t         *r_hdrKey;
cvar_t         *r_hdrContrastThreshold;
cvar_t         *r_hdrContrastOffset;
cvar_t         *r_hdrLightmap;
cvar_t         *r_hdrLightmapExposure;
cvar_t         *r_hdrLightmapGamma;
cvar_t         *r_hdrLightmapCompensate;
cvar_t         *r_hdrToneMappingOperator;
cvar_t         *r_hdrGamma;
cvar_t         *r_hdrDebug;

#ifdef EXPERIMENTAL
cvar_t         *r_screenSpaceAmbientOcclusion;
#endif
#ifdef EXPERIMENTAL
cvar_t         *r_depthOfField;
#endif
cvar_t         *r_reflectionMapping;
cvar_t         *r_highQualityNormalMapping;
cvar_t         *r_bloom;
cvar_t         *r_bloomBlur;
cvar_t         *r_bloomPasses;
cvar_t         *r_rotoscope;
cvar_t         *r_cameraPostFX;
cvar_t         *r_cameraVignette;
cvar_t         *r_cameraFilmGrain;
cvar_t         *r_cameraFilmGrainScale;

cvar_t         *r_evsmPostProcess;


static void AssertCvarRange(cvar_t * cv, float minVal, float maxVal, qboolean shouldBeIntegral)
{
	if(shouldBeIntegral)
	{
		if((int)cv->value != cv->integer)
		{
			ri.Printf(PRINT_WARNING, "WARNING: cvar '%s' must be integral (%f)\n", cv->name, cv->value);
			ri.Cvar_Set(cv->name, va("%d", cv->integer));
		}
	}

	if(cv->value < minVal)
	{
		ri.Printf(PRINT_WARNING, "WARNING: cvar '%s' out of range (%f < %f)\n", cv->name, cv->value, minVal);
		ri.Cvar_Set(cv->name, va("%f", minVal));
	}
	else if(cv->value > maxVal)
	{
		ri.Printf(PRINT_WARNING, "WARNING: cvar '%s' out of range (%f > %f)\n", cv->name, cv->value, maxVal);
		ri.Cvar_Set(cv->name, va("%f", maxVal));
	}
}


/*
** InitOpenGL
**
** This function is responsible for initializing a valid OpenGL subsystem.  This
** is done by calling GLimp_Init (which gives us a working OGL subsystem) then
** setting variables, checking GL constants, and reporting the gfx system config
** to the user.
*/
#if !defined(USE_D3D10)
static void InitOpenGL(void)
{
	char            renderer_buffer[1024];

	//
	// initialize OS specific portions of the renderer
	//
	// GLimp_Init directly or indirectly references the following cvars:
	//      - r_fullscreen
	//      - r_glDriver
	//      - r_mode
	//      - r_(color|depth|stencil)bits
	//      - r_ignorehwgamma
	//      - r_gamma
	//

	if(glConfig.vidWidth == 0)
	{
		GLint           temp;

		GLimp_Init();

		GL_CheckErrors();

		strcpy(renderer_buffer, glConfig.renderer_string);
		Q_strlwr(renderer_buffer);

		// OpenGL driver constants
		glGetIntegerv(GL_MAX_TEXTURE_SIZE, &temp);
		glConfig.maxTextureSize = temp;

		// stubbed or broken drivers may have reported 0...
		if(glConfig.maxTextureSize <= 0)
		{
			glConfig.maxTextureSize = 0;
		}

#if defined(GLSL_COMPILE_STARTUP_ONLY)
		GLSL_InitGPUShaders();
#endif
	}

	GL_CheckErrors();

	// init command buffers and SMP
	R_InitCommandBuffers();
	GL_CheckErrors();

	// print info
	GfxInfo_f();
	GL_CheckErrors();

	// set default state
	GL_SetDefaultState();
	GL_CheckErrors();
}
#endif

/*
==================
GL_CheckErrors
==================
*/
void GL_CheckErrors_(const char *fileName, int line)
{
	int             err;
	char            s[128];

	if(glConfig.smpActive)
	{
		// we can't print onto the console while rendering in another thread
		return;
	}

	if(r_ignoreGLErrors->integer)
	{
		return;
	}

	err = glGetError();
	if(err == GL_NO_ERROR)
	{
		return;
	}

	switch (err)
	{
		case GL_INVALID_ENUM:
			strcpy(s, "GL_INVALID_ENUM");
			break;
		case GL_INVALID_VALUE:
			strcpy(s, "GL_INVALID_VALUE");
			break;
		case GL_INVALID_OPERATION:
			strcpy(s, "GL_INVALID_OPERATION");
			break;
		case GL_STACK_OVERFLOW:
			strcpy(s, "GL_STACK_OVERFLOW");
			break;
		case GL_STACK_UNDERFLOW:
			strcpy(s, "GL_STACK_UNDERFLOW");
			break;
		case GL_OUT_OF_MEMORY:
			strcpy(s, "GL_OUT_OF_MEMORY");
			break;
		case GL_TABLE_TOO_LARGE:
			strcpy(s, "GL_TABLE_TOO_LARGE");
			break;
		case GL_INVALID_FRAMEBUFFER_OPERATION_EXT:
			strcpy(s, "GL_INVALID_FRAMEBUFFER_OPERATION_EXT");
			break;
		default:
			Com_sprintf(s, sizeof(s), "0x%X", err);
			break;
	}

	ri.Error(ERR_FATAL, "caught OpenGL error: %s in file %s line %i", s, fileName, line);
}


/*
** R_GetModeInfo
*/
typedef struct vidmode_s
{
	const char     *description;
	int             width, height;
	float           pixelAspect;	// pixel width / height
} vidmode_t;

vidmode_t       r_vidModes[] = {
	{"Mode  0: 320x240", 320, 240, 1},
	{"Mode  1: 400x300", 400, 300, 1},
	{"Mode  2: 512x384", 512, 384, 1},
	{"Mode  3: 640x480", 640, 480, 1},
	{"Mode  4: 800x600", 800, 600, 1},
	{"Mode  5: 960x720", 960, 720, 1},
	{"Mode  6: 1024x768", 1024, 768, 1},
	{"Mode  7: 1152x864", 1152, 864, 1},
	{"Mode  8: 1280x720 (16:9)", 1280, 720, 1},
	{"Mode  9: 1280x768 (16:10)", 1280, 768, 1},
	{"Mode 10: 1280x800 (16:10)", 1280, 800, 1},
	{"Mode 11: 1280x1024", 1280, 1024, 1},
	{"Mode 12: 1360x768 (16:9)", 1360, 768, 1},
	{"Mode 13: 1440x900 (16:10)", 1440, 900, 1},
	{"Mode 14: 1680x1050 (16:10)", 1680, 1050, 1},
	{"Mode 15: 1600x1200", 1600, 1200, 1},
	{"Mode 16: 1920x1080 (16:9)", 1920, 1080, 1},
	{"Mode 17: 1920x1200 (16:10)", 1920, 1200, 1},
	{"Mode 18: 2048x1536", 2048, 1536, 1},
	{"Mode 19: 2560x1600 (16:10)", 2560, 1600, 1},
};
static int      s_numVidModes = (sizeof(r_vidModes) / sizeof(r_vidModes[0]));

qboolean R_GetModeInfo(int *width, int *height, float *windowAspect, int mode)
{
	vidmode_t      *vm;

	if(mode < -1)
	{
		return qfalse;
	}
	if(mode >= s_numVidModes)
	{
		return qfalse;
	}

	if(mode == -1)
	{
		*width = r_customwidth->integer;
		*height = r_customheight->integer;
		*windowAspect = r_customaspect->value;
		return qtrue;
	}

	vm = &r_vidModes[mode];

	*width = vm->width;
	*height = vm->height;
	*windowAspect = (float)vm->width / (vm->height * vm->pixelAspect);

	return qtrue;
}

/*
** R_ModeList_f
*/
static void R_ModeList_f(void)
{
	int             i;

	ri.Printf(PRINT_ALL, "\n");
	for(i = 0; i < s_numVidModes; i++)
	{
		ri.Printf(PRINT_ALL, "%s\n", r_vidModes[i].description);
	}
	ri.Printf(PRINT_ALL, "\n");
}



/*
==============================================================================

						SCREEN SHOTS

screenshots get written in fs_homepath + fs_gamedir
.. base/screenshots\*.*

three commands: "screenshot", "screenshotJPEG" and "screenshotPNG"

the format is etxreal-YYYY_MM_DD-HH_MM_SS-MS.tga/jpeg/png

==============================================================================
*/

/*
==================
RB_ReadPixels

Reads an image but takes care of alignment issues for reading RGB images.
Prepends the specified number of (uninitialized) bytes to the buffer.

The returned buffer must be freed with ri.Hunk_FreeTempMemory().
==================
*/
static byte *RB_ReadPixels(int x, int y, int width, int height, size_t offset)
{
	GLint packAlign;
	int lineLen, paddedLineLen;
	byte *buffer, *pixels;
	int i;

	glGetIntegerv(GL_PACK_ALIGNMENT, &packAlign);

	lineLen = width * 3;
	paddedLineLen = PAD(lineLen, packAlign);

	// Allocate a few more bytes so that we can choose an alignment we like
	buffer = (byte*)ri.Hunk_AllocateTempMemory(offset + paddedLineLen * height + packAlign - 1);

	pixels = (byte*)PADP(buffer + offset, packAlign);
	glReadPixels(x, y, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels);

	// Drop alignment and line padding bytes
	for(i = 0; i < height; ++i)
	{
		memmove(buffer + offset + i * lineLen, pixels + i * paddedLineLen, lineLen);
	}

	return buffer;
}

/*
==================
R_TakeScreenshot
==================
*/
static void RB_TakeScreenshot(int x, int y, int width, int height, char *fileName)
{
	byte *buffer;
	int dataSize;
	byte *end, *p;

	// with 18 bytes for the TGA file header
	buffer = RB_ReadPixels(x, y, width, height, 18);
	Com_Memset(buffer, 0, 18);

	buffer[2] = 2; // uncompressed type
	buffer[12] = width & 255;
	buffer[13] = width >> 8;
	buffer[14] = height & 255;
	buffer[15] = height >> 8;
	buffer[16] = 24; // pixel size

	dataSize = 3 * width * height;

	// swap RGB to BGR
	end = buffer + 18 + dataSize;
	for(p = buffer + 18; p < end; p += 3)
	{
		byte temp = p[0];
		p[0] = p[2];
		p[2] = temp;
	}

	if(tr.overbrightBits > 0 && glConfig.deviceSupportsGamma)
	{
		R_GammaCorrect(buffer + 18, dataSize);
	}

	ri.FS_WriteFile(fileName, buffer, 18 + dataSize);

	ri.Hunk_FreeTempMemory(buffer);
}

/*
==================
R_TakeScreenshotJPEG
==================
*/
static void RB_TakeScreenshotJPEG(int x, int y, int width, int height, char *fileName)
{
	byte *buffer = RB_ReadPixels(x, y, width, height, 0);

	if(tr.overbrightBits > 0 && glConfig.deviceSupportsGamma)
	{
		R_GammaCorrect(buffer, 3 * width * height);
	}

	SaveJPG(fileName, 90, width, height, buffer);
	ri.Hunk_FreeTempMemory(buffer);
}

/*
==================
R_TakeScreenshotPNG
==================
*/
static void RB_TakeScreenshotPNG(int x, int y, int width, int height, char *fileName)
{
	byte *buffer = RB_ReadPixels(x, y, width, height, 0);

	if(tr.overbrightBits > 0 && glConfig.deviceSupportsGamma)
	{
		R_GammaCorrect(buffer, 3 * width * height);
	}

	SavePNG(fileName, buffer, width, height, 3, qfalse);
	ri.Hunk_FreeTempMemory(buffer);
}

/*
==================
RB_TakeScreenshotCmd
==================
*/
const void     *RB_TakeScreenshotCmd(const void *data)
{
	const screenshotCommand_t *cmd;

	cmd = (const screenshotCommand_t *)data;

	switch (cmd->format)
	{
		case SSF_TGA:
			RB_TakeScreenshot(cmd->x, cmd->y, cmd->width, cmd->height, cmd->fileName);
			break;

		case SSF_JPEG:
			RB_TakeScreenshotJPEG(cmd->x, cmd->y, cmd->width, cmd->height, cmd->fileName);
			break;

		case SSF_PNG:
			RB_TakeScreenshotPNG(cmd->x, cmd->y, cmd->width, cmd->height, cmd->fileName);
			break;
	}

	return (const void *)(cmd + 1);
}

/*
==================
R_TakeScreenshot
==================
*/
void R_TakeScreenshot(const char *name, ssFormat_t format)
{
	static char     fileName[MAX_OSPATH];	// bad things may happen if two screenshots per frame are taken.
	screenshotCommand_t *cmd;
	int             lastNumber;

	cmd = (screenshotCommand_t *) R_GetCommandBuffer(sizeof(*cmd));
	if(!cmd)
	{
		return;
	}

	if(ri.Cmd_Argc() == 2)
	{
		Com_sprintf(fileName, sizeof(fileName), "screenshots/" PRODUCT_NAME_LOWER "-%s.%s", ri.Cmd_Argv(1), name);
	}
	else
	{
		qtime_t         t;

		ri.RealTime(&t);

		// scan for a free filename
		for(lastNumber = 0; lastNumber <= 999; lastNumber++)
		{
			Com_sprintf(fileName, sizeof(fileName), "screenshots/" PRODUCT_NAME_LOWER "_%04d-%02d-%02d_%02d%02d%02d_%03d.%s",
						1900 + t.tm_year, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec, lastNumber, name);

			if(!ri.FS_FileExists(fileName))
			{
				break;			// file doesn't exist
			}
		}

		if(lastNumber == 1000)
		{
			ri.Printf(PRINT_ALL, "ScreenShot: Couldn't create a file\n");
			return;
		}

		lastNumber++;
	}
	ri.Printf(PRINT_ALL, "Wrote %s\n", fileName);

	cmd->commandId = RC_SCREENSHOT;
	cmd->x = 0;
	cmd->y = 0;
	cmd->width = glConfig.vidWidth;
	cmd->height = glConfig.vidHeight;
	cmd->fileName = fileName;
	cmd->format = format;
}

/*
==================
R_ScreenShot_f

screenshot
screenshot [filename]
==================
*/
static void R_ScreenShot_f(void)
{
	R_TakeScreenshot("tga", SSF_TGA);
}

static void R_ScreenShotJPEG_f(void)
{
	R_TakeScreenshot("jpg", SSF_JPEG);
}

static void R_ScreenShotPNG_f(void)
{
	R_TakeScreenshot("png", SSF_PNG);
}

/*
static int QDECL MaterialNameCompare(const void *a, const void *b)
{
	char           *s1, *s2;
	int             c1, c2;

	s1 = *(char **)a;
	s2 = *(char **)b;

	do
	{
		c1 = *s1++;
		c2 = *s2++;

		if(c1 >= 'a' && c1 <= 'z')
		{
			c1 -= ('a' - 'A');
		}
		if(c2 >= 'a' && c2 <= 'z')
		{
			c2 -= ('a' - 'A');
		}

		if(c1 == '\\' || c1 == ':')
		{
			c1 = '/';
		}
		if(c2 == '\\' || c2 == ':')
		{
			c2 = '/';
		}

		if(c1 < c2)
		{
			// strings not equal
			return -1;
		}
		if(c1 > c2)
		{
			return 1;
		}
	} while(c1);

	// strings are equal
	return 0;
}
*/

/*
static void R_GenerateMaterialFile_f(void)
{
	char          **dirnames;
	int             ndirs;
	int             i;
	fileHandle_t    f;
	char            fileName[MAX_QPATH];
	char            fileName2[MAX_QPATH];
	char            cleanName[MAX_QPATH];
	char            cleanName2[MAX_QPATH];
	char            baseName[MAX_QPATH];
	char            baseName2[MAX_QPATH];
	char            path[MAX_QPATH];
	char            extension[MAX_QPATH];
	int				len;

	if(Cmd_Argc() < 3)
	{
		Com_Printf("usage: generatemtr <directory> [image extension]\n");
		return;
	}

	Q_strncpyz(path, ri.Cmd_Argv(1), sizeof(path));

	Q_strncpyz(extension, ri.Cmd_Argv(2), sizeof(extension));
	Q_strreplace(extension, sizeof(extension), ".", "");

	Q_strncpyz(fileName, ri.Cmd_Argv(1), sizeof(fileName));
	Com_DefaultExtension(fileName, sizeof(fileName), ".mtr");
	Com_Printf("Writing %s.\n", fileName);
	f = FS_FOpenFileWrite(fileName);
	if(!f)
	{
		Com_Printf("Couldn't write %s.\n", fileName);
		return;
	}

	FS_Printf(f, "// generated by XreaL\n\n");

	dirnames = FS_ListFiles(path, extension, &ndirs);

	qsort(dirnames, ndirs, sizeof(char *), MaterialNameCompare);

	for(i = 0; i < ndirs; i++)
	{
		// clean name
		Q_strncpyz(fileName, dirnames[i], sizeof(fileName));
		Q_strncpyz(cleanName, dirnames[i], sizeof(cleanName));

		Q_strreplace(cleanName, sizeof(cleanName), "MaPZone[", "");
		Q_strreplace(cleanName, sizeof(cleanName), "]", "");
		Q_strreplace(cleanName, sizeof(cleanName), "&", "_");

		if(strcmp(fileName, cleanName))
		{
			Com_sprintf(fileName2, sizeof(fileName2), "%s/%s", path, fileName);
			Com_sprintf(cleanName2, sizeof(cleanName2), "%s/%s", path, cleanName);

			Com_Printf("renaming '%s' into '%s'\n", fileName2, cleanName2);
			FS_Rename(fileName2, cleanName2);
		}

		COM_StripExtension3(cleanName, cleanName, sizeof(cleanName));
		if(!Q_stristr(cleanName, "_nm") && !Q_stristr(cleanName, "blend"))
		{
			Q_strncpyz(baseName, cleanName, sizeof(baseName));
			Q_strreplace(baseName, sizeof(baseName), "_diffuse", "");

			FS_Printf(f, "%s/%s\n", path, baseName);
			FS_Printf(f, "{\n");
			FS_Printf(f, "\t qer_editorImage\t %s/%s.%s\n", path, cleanName, extension);
			FS_Printf(f, "\n");
			FS_Printf(f, "\t parallax\n");
			FS_Printf(f, "\n");
			FS_Printf(f, "\t diffuseMap\t\t %s/%s.%s\n", path, baseName, extension);

			Com_sprintf(fileName, sizeof(fileName), "%s/%s_nm.%s", path, baseName, extension);
			if(ri.FS_ReadFile(fileName, NULL) > 0)
			{
				FS_Printf(f, "\t normalMap\t\t %s/%s_nm.%s\n", path, baseName, extension);
			}

			Q_strncpyz(baseName2, baseName, sizeof(baseName2));
			len = strlen(baseName);
			baseName2[len -1] = '\0';

			Com_sprintf(fileName, sizeof(fileName), "%s/speculars/%s_s.%s", path, baseName2, extension);
			if(ri.FS_ReadFile(fileName, NULL) > 0)
			{
				FS_Printf(f, "\t specularMap\t %s/speculars/%s_s.%s\n", path, baseName2, extension);
			}

			Com_sprintf(fileName, sizeof(fileName), "%s/%s.blend.%s", path, baseName, extension);
			if(ri.FS_ReadFile(fileName, NULL) > 0)
			{
				FS_Printf(f, "\t glowMap\t\t %s/%s.blend.%s\n", path, baseName, extension);
			}

			FS_Printf(f, "}\n\n");
		}
	}
	ri.FS_FreeFileList(dirnames);

	ri.FS_FCloseFile(f);
}
*/

//============================================================================

/*
==================
RB_TakeVideoFrameCmd
==================
*/
const void     *RB_TakeVideoFrameCmd(const void *data)
{
	const videoFrameCommand_t *cmd;
	GLint packAlign;
	int lineLen, captureLineLen;
	byte *pixels;
	int i;
	int outputSize;
	int j;
	int aviLineLen;

	cmd = (const videoFrameCommand_t *)data;

	// RB: it is possible to we still have a videoFrameCommand_t but we already stopped
	// video recording
	if(ri.CL_VideoRecording())
	{
		// take care of alignment issues for reading RGB images..

		glGetIntegerv(GL_PACK_ALIGNMENT, &packAlign);

		lineLen = cmd->width * 3;
		captureLineLen = PAD(lineLen, packAlign);

		pixels = (byte*)PADP(cmd->captureBuffer, packAlign);
		glReadPixels(0, 0, cmd->width, cmd->height, GL_RGB, GL_UNSIGNED_BYTE, pixels);

		if(tr.overbrightBits > 0 && glConfig.deviceSupportsGamma)
		{
			// this also runs over the padding...
			R_GammaCorrect(pixels, captureLineLen * cmd->height);
		}

		if(cmd->motionJpeg)
		{
			// Drop alignment and line padding bytes
			for(i = 0; i < cmd->height; ++i)
			{
				memmove(cmd->captureBuffer + i * lineLen, pixels + i * captureLineLen, lineLen);
			}

			outputSize = SaveJPGToBuffer(cmd->encodeBuffer, 3 * cmd->width * cmd->height, 90, cmd->width, cmd->height, cmd->captureBuffer);
			ri.CL_WriteAVIVideoFrame(cmd->encodeBuffer, outputSize);
		}
		else
		{
			aviLineLen = PAD(lineLen, AVI_LINE_PADDING);

			for(i = 0; i < cmd->height; ++i)
			{
				for(j = 0; j < lineLen; j += 3)
				{
					cmd->encodeBuffer[i * aviLineLen + j + 0] = pixels[i * captureLineLen + j + 2];
					cmd->encodeBuffer[i * aviLineLen + j + 1] = pixels[i * captureLineLen + j + 1];
					cmd->encodeBuffer[i * aviLineLen + j + 2] = pixels[i * captureLineLen + j + 0];
				}
				while(j < aviLineLen)
				{
					cmd->encodeBuffer[i * aviLineLen + j++] = 0;
				}
			}

			ri.CL_WriteAVIVideoFrame(cmd->encodeBuffer, aviLineLen * cmd->height);
		}
	}

	return (const void *)(cmd + 1);
}

//============================================================================

/*
** GL_SetDefaultState
*/
void GL_SetDefaultState(void)
{
	int             i;

	GLimp_LogComment("--- GL_SetDefaultState ---\n");

	GL_ClearDepth(1.0f);

	if(glConfig.stencilBits >= 4)
	{
		GL_ClearStencil(128);
	}

	GL_FrontFace(GL_CCW);
	GL_CullFace(GL_FRONT);

	glState.faceCulling = CT_TWO_SIDED;
	glDisable(GL_CULL_FACE);

	GL_CheckErrors();

	glVertexAttrib4fARB(ATTR_INDEX_COLOR, 1, 1, 1, 1);

	GL_CheckErrors();

	// initialize downstream texture units if we're running
	// in a multitexture environment
	if(glConfig.driverType == GLDRV_OPENGL3)
	{
		for(i = 31; i >= 0; i--)
		{
			GL_SelectTexture(i);
			GL_TextureMode(r_textureMode->string);
		}
	}
	else
	{
		if(GLEW_ARB_multitexture)
		{
			for(i = glConfig.maxActiveTextures - 1; i >= 0; i--)
			{
				GL_SelectTexture(i);
				GL_TextureMode(r_textureMode->string);

				/*
				if(i != 0)
					glDisable(GL_TEXTURE_2D);
				else
					glEnable(GL_TEXTURE_2D);
				*/
			}
		}
	}

	GL_CheckErrors();

	GL_DepthFunc(GL_LEQUAL);

	// make sure our GL state vector is set correctly
	glState.glStateBits = GLS_DEPTHTEST_DISABLE | GLS_DEPTHMASK_TRUE;
	glState.vertexAttribsState = 0;
	glState.vertexAttribPointersSet = 0;

	glState.currentProgram = 0;
	glUseProgramObjectARB(0);

	glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
	glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
	glState.currentVBO = NULL;
	glState.currentIBO = NULL;

	GL_CheckErrors();

	// the vertex array is always enabled, but the color and texture
	// arrays are enabled and disabled around the compiled vertex array call
	glEnableVertexAttribArrayARB(ATTR_INDEX_POSITION);

	/*
	   OpenGL 3.0 spec: E.1. PROFILES AND DEPRECATED FEATURES OF OPENGL 3.0 405
	   Calling VertexAttribPointer when no buffer object or no
	   vertex array object is bound will generate an INVALID OPERATION error,
	   as will calling any array drawing command when no vertex array object is
	   bound.
	 */

	if(glConfig2.framebufferObjectAvailable)
	{
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
		glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);
		glState.currentFBO = NULL;
	}


	/*
	   if(glConfig2.drawBuffersAvailable && glConfig2.maxDrawBuffers >= 4)
	   {
	   // enable all attachments as draw buffers
	   GLenum drawbuffers[] = {GL_DRAW_BUFFER0_ARB,
	   GL_DRAW_BUFFER1_ARB,
	   GL_DRAW_BUFFER2_ARB,
	   GL_DRAW_BUFFER3_ARB};

	   glDrawBuffersARB(4, drawbuffers);
	   }
	 */

	GL_PolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	GL_DepthMask(GL_TRUE);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_SCISSOR_TEST);
	glDisable(GL_BLEND);

	GL_CheckErrors();

	glState.stackIndex = 0;
	for(i = 0; i < MAX_GLSTACK; i++)
	{
		MatrixIdentity(glState.modelViewMatrix[i]);
		MatrixIdentity(glState.projectionMatrix[i]);
		MatrixIdentity(glState.modelViewProjectionMatrix[i]);
	}
}



/*
================
GfxInfo_f
================
*/
void GfxInfo_f(void)
{
	/*const char     *enablestrings[] = {
		"disabled",
		"enabled"
	};*/
	const char     *fsstrings[] = {
		"windowed",
		"fullscreen"
	};

	ri.Printf(PRINT_ALL, "\nGL_VENDOR: %s\n", glConfig.vendor_string);
	ri.Printf(PRINT_ALL, "GL_RENDERER: %s\n", glConfig.renderer_string);
	ri.Printf(PRINT_ALL, "GL_VERSION: %s\n", glConfig.version_string);
	ri.Printf(PRINT_ALL, "GL_EXTENSIONS: %s\n", glConfig.extensions_string);
	ri.Printf(PRINT_ALL, "GL_MAX_TEXTURE_SIZE: %d\n", glConfig.maxTextureSize);

	if(glConfig.driverType != GLDRV_OPENGL3)
	{
		ri.Printf(PRINT_ALL, "GL_MAX_TEXTURE_UNITS_ARB: %d\n", glConfig.maxActiveTextures);
	}

	/*
	   if(glConfig.fragmentProgramAvailable)
	   {
	   ri.Printf(PRINT_ALL, "GL_MAX_TEXTURE_IMAGE_UNITS_ARB: %d\n", glConfig.maxTextureImageUnits);
	   }
	 */

	ri.Printf(PRINT_ALL, "GL_SHADING_LANGUAGE_VERSION_ARB: %s\n", glConfig2.shadingLanguageVersion);

	ri.Printf(PRINT_ALL, "GL_MAX_VERTEX_UNIFORM_COMPONENTS_ARB %d\n", glConfig2.maxVertexUniforms);
//	ri.Printf(PRINT_ALL, "GL_MAX_VARYING_FLOATS_ARB %d\n", glConfig2.maxVaryingFloats);
	ri.Printf(PRINT_ALL, "GL_MAX_VERTEX_ATTRIBS_ARB %d\n", glConfig2.maxVertexAttribs);

	if(glConfig2.occlusionQueryAvailable)
	{
		ri.Printf(PRINT_ALL, "%d occlusion query bits\n", glConfig2.occlusionQueryBits);
	}

	if(glConfig2.drawBuffersAvailable)
	{
		ri.Printf(PRINT_ALL, "GL_MAX_DRAW_BUFFERS_ARB: %d\n", glConfig2.maxDrawBuffers);
	}

	if(glConfig2.textureAnisotropyAvailable)
	{
		ri.Printf(PRINT_ALL, "GL_TEXTURE_MAX_ANISOTROPY_EXT: %f\n", glConfig2.maxTextureAnisotropy);
	}

	if(glConfig2.framebufferObjectAvailable)
	{
		ri.Printf(PRINT_ALL, "GL_MAX_RENDERBUFFER_SIZE_EXT: %d\n", glConfig2.maxRenderbufferSize);
		ri.Printf(PRINT_ALL, "GL_MAX_COLOR_ATTACHMENTS_EXT: %d\n", glConfig2.maxColorAttachments);
	}

	ri.Printf(PRINT_ALL, "\nPIXELFORMAT: color(%d-bits) Z(%d-bit) stencil(%d-bits)\n", glConfig.colorBits,
			  glConfig.depthBits, glConfig.stencilBits);
	ri.Printf(PRINT_ALL, "MODE: %d, %d x %d %s hz:", r_mode->integer, glConfig.vidWidth, glConfig.vidHeight,
			  fsstrings[r_fullscreen->integer == 1]);

	if(glConfig.displayFrequency)
	{
		ri.Printf(PRINT_ALL, "%d\n", glConfig.displayFrequency);
	}
	else
	{
		ri.Printf(PRINT_ALL, "N/A\n");
	}

	if(glConfig.deviceSupportsGamma)
	{
		ri.Printf(PRINT_ALL, "GAMMA: hardware w/ %d overbright bits\n", tr.overbrightBits);
	}
	else
	{
		ri.Printf(PRINT_ALL, "GAMMA: software w/ %d overbright bits\n", tr.overbrightBits);
	}

	ri.Printf(PRINT_ALL, "texturemode: %s\n", r_textureMode->string);
	ri.Printf(PRINT_ALL, "picmip: %d\n", r_picmip->integer);

	if(glConfig.driverType == GLDRV_OPENGL3)
	{
		int				contextFlags, profile;

		ri.Printf(PRINT_ALL, S_COLOR_GREEN "Using OpenGL 3.x context\n");

		// check if we have a core-profile
		glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &profile);
		if(profile == GL_CONTEXT_CORE_PROFILE_BIT)
		{
			ri.Printf(PRINT_ALL, S_COLOR_GREEN "Having a core profile\n");
		}
		else
		{
			ri.Printf(PRINT_ALL, S_COLOR_RED "Having a compatibility profile\n");
		}
		
		// check if context is forward compatible
		glGetIntegerv(GL_CONTEXT_FLAGS, &contextFlags);
		if(contextFlags & GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT)
		{
			ri.Printf(PRINT_ALL, S_COLOR_GREEN "Context is forward compatible\n");
		}
		else
		{
			ri.Printf(PRINT_ALL, S_COLOR_RED "Context is NOT forward compatible\n");
		}
	}

	if(glConfig.hardwareType == GLHW_ATI)
	{
		ri.Printf(PRINT_ALL, "HACK: ATI approximations\n");
	}

	if(glConfig.textureCompression != TC_NONE)
	{
		ri.Printf(PRINT_ALL, "Using S3TC (DXTC) texture compression\n");
	}

	if(glConfig.hardwareType == GLHW_ATI_DX10)
	{
		ri.Printf(PRINT_ALL, "Using ATI DirectX 10 hardware features\n");
	}

	if(glConfig.hardwareType == GLHW_NV_DX10)
	{
		ri.Printf(PRINT_ALL, "Using NVIDIA DirectX 10 hardware features\n");
	}

	if(glConfig2.vboVertexSkinningAvailable)
	{
		ri.Printf(PRINT_ALL, "Using GPU vertex skinning with max %i bones in a single pass\n", glConfig2.maxVertexSkinningBones);
	}

	if(glConfig.smpActive)
	{
		ri.Printf(PRINT_ALL, "Using dual processor acceleration\n");
	}

	if(r_finish->integer)
	{
		ri.Printf(PRINT_ALL, "Forcing glFinish\n");
	}
}

#if !defined(USE_D3D10)
static void GLSL_restart_f(void)
{
	// make sure the render thread is stopped
	R_SyncRenderThread();

	GLSL_ShutdownGPUShaders();
	GLSL_InitGPUShaders();
}
#endif

/*
===============
R_Register
===============
*/
void R_Register(void)
{
	r_glCoreProfile = ri.Cvar_Get("r_glCoreProfile", "1", CVAR_INIT);

	r_glMinMajorVersion = ri.Cvar_Get("r_glMinMajorVersion", "3", CVAR_LATCH);
	r_glMinMinorVersion = ri.Cvar_Get("r_glMinMinorVersion", "2", CVAR_LATCH);

	// latched and archived variables
	r_ext_compressed_textures = ri.Cvar_Get("r_ext_compressed_textures", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_ext_occlusion_query = ri.Cvar_Get("r_ext_occlusion_query", "1", CVAR_CHEAT | CVAR_LATCH);
	r_ext_texture_non_power_of_two = ri.Cvar_Get("r_ext_texture_non_power_of_two", "1", CVAR_CHEAT | CVAR_LATCH);
	r_ext_draw_buffers = ri.Cvar_Get("r_ext_draw_buffers", "1", CVAR_CHEAT | CVAR_LATCH);
	r_ext_vertex_array_object = ri.Cvar_Get("r_ext_vertex_array_object", "1", CVAR_CHEAT | CVAR_LATCH);
	r_ext_half_float_pixel = ri.Cvar_Get("r_ext_half_float_pixel", "1", CVAR_CHEAT | CVAR_LATCH);
	r_ext_texture_float = ri.Cvar_Get("r_ext_texture_float", "1", CVAR_CHEAT | CVAR_LATCH);
	r_ext_stencil_wrap = ri.Cvar_Get("r_ext_stencil_wrap", "1", CVAR_CHEAT | CVAR_LATCH);
	r_ext_texture_filter_anisotropic = ri.Cvar_Get("r_ext_texture_filter_anisotropic", "4", CVAR_ARCHIVE | CVAR_LATCH);
	r_ext_stencil_two_side = ri.Cvar_Get("r_ext_stencil_two_side", "1", CVAR_CHEAT | CVAR_LATCH);
	r_ext_separate_stencil = ri.Cvar_Get("r_ext_separate_stencil", "1", CVAR_CHEAT | CVAR_LATCH);
	r_ext_depth_bounds_test = ri.Cvar_Get("r_ext_depth_bounds_test", "1", CVAR_CHEAT | CVAR_LATCH);
	r_ext_framebuffer_object = ri.Cvar_Get("r_ext_framebuffer_object", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_ext_packed_depth_stencil = ri.Cvar_Get("r_ext_packed_depth_stencil", "1", CVAR_CHEAT | CVAR_LATCH);
	r_ext_framebuffer_blit = ri.Cvar_Get("r_ext_framebuffer_blit", "1", CVAR_CHEAT | CVAR_LATCH);
	r_extx_framebuffer_mixed_formats = ri.Cvar_Get("r_extx_framebuffer_mixed_formats", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_ext_generate_mipmap = ri.Cvar_Get("r_ext_generate_mipmap", "1", CVAR_CHEAT | CVAR_LATCH);

	r_collapseStages = ri.Cvar_Get("r_collapseStages", "1", CVAR_LATCH | CVAR_CHEAT);
	r_picmip = ri.Cvar_Get("r_picmip", "0", CVAR_ARCHIVE | CVAR_LATCH);
	AssertCvarRange(r_picmip, 0, 3, qtrue);
	r_roundImagesDown = ri.Cvar_Get("r_roundImagesDown", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_colorMipLevels = ri.Cvar_Get("r_colorMipLevels", "0", CVAR_LATCH);
	r_colorbits = ri.Cvar_Get("r_colorbits", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_stereo = ri.Cvar_Get("r_stereo", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_stencilbits = ri.Cvar_Get("r_stencilbits", "8", CVAR_ARCHIVE | CVAR_LATCH);
	r_depthbits = ri.Cvar_Get("r_depthbits", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_ignorehwgamma = ri.Cvar_Get("r_ignorehwgamma", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_mode = ri.Cvar_Get("r_mode", "3", CVAR_ARCHIVE | CVAR_LATCH);
	r_fullscreen = ri.Cvar_Get("r_fullscreen", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_customwidth = ri.Cvar_Get("r_customwidth", "1600", CVAR_ARCHIVE | CVAR_LATCH);
	r_customheight = ri.Cvar_Get("r_customheight", "1024", CVAR_ARCHIVE | CVAR_LATCH);
	r_customaspect = ri.Cvar_Get("r_customaspect", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_simpleMipMaps = ri.Cvar_Get("r_simpleMipMaps", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_uiFullScreen = ri.Cvar_Get("r_uifullscreen", "0", 0);
	r_subdivisions = ri.Cvar_Get("r_subdivisions", "4", CVAR_ARCHIVE | CVAR_LATCH);
	r_deferredShading = ri.Cvar_Get("r_deferredShading", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_parallaxMapping = ri.Cvar_Get("r_parallaxMapping", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_dynamicLightCastShadows = ri.Cvar_Get("r_dynamicLightCastShadows", "1", CVAR_ARCHIVE);
	r_precomputedLighting = ri.Cvar_Get("r_precomputedLighting", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_vertexLighting = ri.Cvar_Get("r_vertexLighting", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_compressDiffuseMaps = ri.Cvar_Get("r_compressDiffuseMaps", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_compressSpecularMaps = ri.Cvar_Get("r_compressSpecularMaps", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_compressNormalMaps = ri.Cvar_Get("r_compressNormalMaps", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_heatHazeFix = ri.Cvar_Get("r_heatHazeFix", "0", CVAR_CHEAT);
	r_noMarksOnTrisurfs = ri.Cvar_Get("r_noMarksOnTrisurfs", "1", CVAR_CHEAT);

	r_forceFog = ri.Cvar_Get("r_forceFog", "0", CVAR_CHEAT /* | CVAR_LATCH */ );
	AssertCvarRange(r_forceFog, 0.0f, 1.0f, qfalse);
	r_wolfFog = ri.Cvar_Get("r_wolfFog", "1", CVAR_CHEAT);
	r_noFog = ri.Cvar_Get("r_noFog", "0", CVAR_CHEAT);
#ifdef EXPERIMENTAL
	r_screenSpaceAmbientOcclusion = ri.Cvar_Get("r_screenSpaceAmbientOcclusion", "0", CVAR_ARCHIVE);
	//AssertCvarRange(r_screenSpaceAmbientOcclusion, 0, 2, qtrue);
#endif
#ifdef EXPERIMENTAL
	r_depthOfField = ri.Cvar_Get("r_depthOfField", "0", CVAR_ARCHIVE);
#endif

	r_reflectionMapping = ri.Cvar_Get("r_reflectionMapping", "0", CVAR_CHEAT);
	r_highQualityNormalMapping = ri.Cvar_Get("r_highQualityNormalMapping", "0", CVAR_ARCHIVE | CVAR_LATCH);

	r_forceAmbient = ri.Cvar_Get("r_forceAmbient", "0.125", CVAR_ARCHIVE | CVAR_LATCH);
	AssertCvarRange(r_forceAmbient, 0.0f, 0.3f, qfalse);

	r_smp = ri.Cvar_Get("r_smp", "0", CVAR_ARCHIVE | CVAR_LATCH);

	// temporary latched variables that can only change over a restart
	r_displayRefresh = ri.Cvar_Get("r_displayRefresh", "0", CVAR_LATCH);
	AssertCvarRange(r_displayRefresh, 0, 200, qtrue);
#if defined(COMPAT_Q3A) || defined(COMPAT_ET)
	r_overBrightBits = ri.Cvar_Get("r_overBrightBits", "1", CVAR_LATCH);
	r_mapOverBrightBits = ri.Cvar_Get("r_mapOverBrightBits", "2", CVAR_LATCH);
#else
	r_overBrightBits = ri.Cvar_Get("r_overBrightBits", "0", CVAR_LATCH);
	r_mapOverBrightBits = ri.Cvar_Get("r_mapOverBrightBits", "0", CVAR_LATCH);
#endif

	AssertCvarRange(r_overBrightBits, 0, 1, qtrue);	// ydnar: limit to overbrightbits 1 (sorry 1337 players)
	AssertCvarRange(r_mapOverBrightBits, 0, 3, qtrue);

	r_intensity = ri.Cvar_Get("r_intensity", "1", CVAR_LATCH);
	AssertCvarRange(r_intensity, 0, 5, qfalse);

	r_singleShader = ri.Cvar_Get("r_singleShader", "0", CVAR_CHEAT | CVAR_LATCH);
	r_stitchCurves = ri.Cvar_Get("r_stitchCurves", "1", CVAR_CHEAT | CVAR_LATCH);
	r_debugShadowMaps = ri.Cvar_Get("r_debugShadowMaps", "0", CVAR_CHEAT);
	r_shadowMapLuminanceAlpha = ri.Cvar_Get("r_shadowMapLuminanceAlpha", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_shadowMapLinearFilter = ri.Cvar_Get("r_shadowMapLinearFilter", "1", CVAR_CHEAT | CVAR_LATCH);
	r_lightBleedReduction = ri.Cvar_Get("r_lightBleedReduction", "0", CVAR_CHEAT);
	r_overDarkeningFactor = ri.Cvar_Get("r_overDarkeningFactor", "30.0", CVAR_CHEAT);
	r_shadowMapDepthScale = ri.Cvar_Get("r_shadowMapDepthScale", "1.41", CVAR_CHEAT);

	r_parallelShadowSplitWeight = ri.Cvar_Get("r_parallelShadowSplitWeight", "0.9", CVAR_CHEAT);
	r_parallelShadowSplits = ri.Cvar_Get("r_parallelShadowSplits", "2", CVAR_CHEAT);
	AssertCvarRange(r_parallelShadowSplits, 0, MAX_SHADOWMAPS -1, qtrue);

	r_lightSpacePerspectiveWarping = ri.Cvar_Get("r_lightSpacePerspectiveWarping", "1", CVAR_CHEAT);

	// archived variables that can change at any time
	r_lodBias = ri.Cvar_Get("r_lodBias", "0", CVAR_ARCHIVE);
	r_flares = ri.Cvar_Get("r_flares", "0", CVAR_ARCHIVE);
	r_znear = ri.Cvar_Get("r_znear", "4", CVAR_CHEAT);
	r_zfar = ri.Cvar_Get("r_zfar", "1024", CVAR_CHEAT);
	r_ignoreGLErrors = ri.Cvar_Get("r_ignoreGLErrors", "1", CVAR_ARCHIVE);
	r_fastsky = ri.Cvar_Get("r_fastsky", "0", CVAR_ARCHIVE);
	r_inGameVideo = ri.Cvar_Get("r_inGameVideo", "1", CVAR_ARCHIVE);
	r_drawSun = ri.Cvar_Get("r_drawSun", "0", CVAR_ARCHIVE);
	r_finish = ri.Cvar_Get("r_finish", "0", CVAR_CHEAT);
	r_textureMode = ri.Cvar_Get("r_textureMode", "GL_LINEAR_MIPMAP_NEAREST", CVAR_ARCHIVE);
	r_swapInterval = ri.Cvar_Get("r_swapInterval", "0", CVAR_ARCHIVE);
	r_gamma = ri.Cvar_Get("r_gamma", "1", CVAR_ARCHIVE);
	r_facePlaneCull = ri.Cvar_Get("r_facePlaneCull", "1", CVAR_ARCHIVE);

	r_railWidth = ri.Cvar_Get("r_railWidth", "96", CVAR_ARCHIVE);
	r_railCoreWidth = ri.Cvar_Get("r_railCoreWidth", "16", CVAR_ARCHIVE);
	r_railSegmentLength = ri.Cvar_Get("r_railSegmentLength", "32", CVAR_ARCHIVE);

	r_ambientScale = ri.Cvar_Get("r_ambientScale", "0.6", CVAR_CHEAT);
	r_lightScale = ri.Cvar_Get("r_lightScale", "2", CVAR_CHEAT);

	r_vboFaces = ri.Cvar_Get("r_vboFaces", "1", CVAR_CHEAT);
	r_vboCurves = ri.Cvar_Get("r_vboCurves", "1", CVAR_CHEAT);
	r_vboTriangles = ri.Cvar_Get("r_vboTriangles", "1", CVAR_CHEAT);
	r_vboShadows = ri.Cvar_Get("r_vboShadows", "1", CVAR_CHEAT);
	r_vboLighting = ri.Cvar_Get("r_vboLighting", "1", CVAR_CHEAT);
	r_vboModels = ri.Cvar_Get("r_vboModels", "1", CVAR_ARCHIVE);
	r_vboOptimizeVertices = ri.Cvar_Get("r_vboOptimizeVertices", "1", CVAR_CHEAT | CVAR_LATCH);
	r_vboVertexSkinning = ri.Cvar_Get("r_vboVertexSkinning", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_vboDeformVertexes = ri.Cvar_Get("r_vboDeformVertexes", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_vboSmoothNormals = ri.Cvar_Get("r_vboSmoothNormals", "1", CVAR_ARCHIVE | CVAR_LATCH);

#if defined(USE_BSP_CLUSTERSURFACE_MERGING)
	r_mergeClusterSurfaces = ri.Cvar_Get("r_mergeClusterSurfaces", "0", CVAR_CHEAT);
	r_mergeClusterFaces = ri.Cvar_Get("r_mergeClusterFaces", "1", CVAR_CHEAT);
	r_mergeClusterCurves = ri.Cvar_Get("r_mergeClusterCurves", "1", CVAR_CHEAT);
	r_mergeClusterTriangles = ri.Cvar_Get("r_mergeClusterTriangles", "1", CVAR_CHEAT);
#endif

	r_dynamicBspOcclusionCulling = ri.Cvar_Get("r_dynamicBspOcclusionCulling", "0", CVAR_ARCHIVE);
	r_dynamicEntityOcclusionCulling = ri.Cvar_Get("r_dynamicEntityOcclusionCulling", "0", CVAR_CHEAT);
	r_dynamicLightOcclusionCulling = ri.Cvar_Get("r_dynamicLightOcclusionCulling", "0", CVAR_CHEAT);
	r_chcMaxPrevInvisNodesBatchSize = ri.Cvar_Get("r_chcMaxPrevInvisNodesBatchSize", "50", CVAR_CHEAT);
	r_chcMaxVisibleFrames = ri.Cvar_Get("r_chcMaxVisibleFrames", "10", CVAR_CHEAT);
	r_chcVisibilityThreshold = ri.Cvar_Get("r_chcVisibilityThreshold", "20", CVAR_CHEAT);
	r_chcIgnoreLeaves = ri.Cvar_Get("r_chcIgnoreLeaves", "0", CVAR_CHEAT);

	r_hdrRendering = ri.Cvar_Get("r_hdrRendering", "0", CVAR_ARCHIVE | CVAR_LATCH);

	// HACK turn off HDR for development
	if(r_deferredShading->integer)
	{
		AssertCvarRange(r_hdrRendering, 0, 0, qtrue);
	}

	r_hdrMinLuminance = ri.Cvar_Get("r_hdrMinLuminance", "0.18", CVAR_CHEAT);
	r_hdrMaxLuminance = ri.Cvar_Get("r_hdrMaxLuminance", "3000", CVAR_CHEAT);
	r_hdrKey = ri.Cvar_Get("r_hdrKey", "0.28", CVAR_CHEAT);
	r_hdrContrastThreshold = ri.Cvar_Get("r_hdrContrastThreshold", "1.3", CVAR_CHEAT);
	r_hdrContrastOffset = ri.Cvar_Get("r_hdrContrastOffset", "3.0", CVAR_CHEAT);
	r_hdrLightmap = ri.Cvar_Get("r_hdrLightmap", "1", CVAR_CHEAT | CVAR_LATCH);
	r_hdrLightmapExposure = ri.Cvar_Get("r_hdrLightmapExposure", "1.0", CVAR_CHEAT | CVAR_LATCH);
	r_hdrLightmapGamma = ri.Cvar_Get("r_hdrLightmapGamma", "1.7", CVAR_CHEAT | CVAR_LATCH);
	r_hdrLightmapCompensate = ri.Cvar_Get("r_hdrLightmapCompensate", "1.0", CVAR_CHEAT | CVAR_LATCH);
	r_hdrToneMappingOperator = ri.Cvar_Get("r_hdrToneMappingOperator", "1", CVAR_CHEAT);
	r_hdrGamma = ri.Cvar_Get("r_hdrGamma", "1.1", CVAR_CHEAT);
	r_hdrDebug = ri.Cvar_Get("r_hdrDebug", "0", CVAR_CHEAT);

	r_evsmPostProcess = ri.Cvar_Get("r_evsmPostProcess", "0", CVAR_ARCHIVE | CVAR_LATCH);

	r_printShaders = ri.Cvar_Get("r_printShaders", "0", CVAR_ARCHIVE);

	r_bloom = ri.Cvar_Get("r_bloom", "0", CVAR_ARCHIVE);
	r_bloomBlur = ri.Cvar_Get("r_bloomBlur", "5.0", CVAR_CHEAT);
	r_bloomPasses = ri.Cvar_Get("r_bloomPasses", "2", CVAR_CHEAT);
	r_rotoscope = ri.Cvar_Get("r_rotoscope", "0", CVAR_ARCHIVE);
	r_cameraPostFX = ri.Cvar_Get("r_cameraPostFX", "0", CVAR_ARCHIVE);
	r_cameraVignette = ri.Cvar_Get("r_cameraVignette", "1", CVAR_ARCHIVE);
	r_cameraFilmGrain = ri.Cvar_Get("r_cameraFilmGrain", "1", CVAR_ARCHIVE);
	r_cameraFilmGrainScale = ri.Cvar_Get("r_cameraFilmGrainScale", "3", CVAR_ARCHIVE);

	// temporary variables that can change at any time
	r_showImages = ri.Cvar_Get("r_showImages", "0", CVAR_TEMP);

	r_debugLight = ri.Cvar_Get("r_debuglight", "0", CVAR_TEMP);
	r_debugSort = ri.Cvar_Get("r_debugSort", "0", CVAR_CHEAT);

	r_nocurves = ri.Cvar_Get("r_nocurves", "0", CVAR_CHEAT);
	r_nobatching = ri.Cvar_Get("r_nobatching", "0", CVAR_CHEAT);
	r_noLightScissors = ri.Cvar_Get("r_noLightScissors", "0", CVAR_CHEAT);
	r_noLightVisCull = ri.Cvar_Get("r_noLightVisCull", "0", CVAR_CHEAT);
	r_noInteractionSort = ri.Cvar_Get("r_noInteractionSort", "0", CVAR_CHEAT);
	r_dynamicLight = ri.Cvar_Get("r_dynamicLight", "1", CVAR_ARCHIVE);
	r_staticLight = ri.Cvar_Get("r_staticLight", "1", CVAR_CHEAT);
	r_drawworld = ri.Cvar_Get("r_drawworld", "1", CVAR_CHEAT);
	r_portalOnly = ri.Cvar_Get("r_portalOnly", "0", CVAR_CHEAT);
	r_portalSky = ri.Cvar_Get("cg_skybox", "1", 0);

	r_flareSize = ri.Cvar_Get("r_flareSize", "40", CVAR_CHEAT);
	r_flareFade = ri.Cvar_Get("r_flareFade", "7", CVAR_CHEAT);

	r_showSmp = ri.Cvar_Get("r_showSmp", "0", CVAR_CHEAT);
	r_skipBackEnd = ri.Cvar_Get("r_skipBackEnd", "0", CVAR_CHEAT);
	r_skipLightBuffer = ri.Cvar_Get("r_skipLightBuffer", "0", CVAR_CHEAT);

	r_measureOverdraw = ri.Cvar_Get("r_measureOverdraw", "0", CVAR_CHEAT);
	r_lodScale = ri.Cvar_Get("r_lodScale", "5", CVAR_CHEAT);
	r_lodTest = ri.Cvar_Get("r_lodTest", "0.5", CVAR_CHEAT);
	r_norefresh = ri.Cvar_Get("r_norefresh", "0", CVAR_CHEAT);
	r_drawentities = ri.Cvar_Get("r_drawentities", "1", CVAR_CHEAT);
	r_drawpolies = ri.Cvar_Get("r_drawpolies", "1", CVAR_CHEAT);
	r_ignore = ri.Cvar_Get("r_ignore", "1", CVAR_CHEAT);
	r_nocull = ri.Cvar_Get("r_nocull", "0", CVAR_CHEAT);
	r_novis = ri.Cvar_Get("r_novis", "0", CVAR_CHEAT);
	r_showcluster = ri.Cvar_Get("r_showcluster", "0", CVAR_CHEAT);
	r_speeds = ri.Cvar_Get("r_speeds", "0", 0);
	r_verbose = ri.Cvar_Get("r_verbose", "0", CVAR_CHEAT);
	r_logFile = ri.Cvar_Get("r_logFile", "0", CVAR_CHEAT);
	r_debugSurface = ri.Cvar_Get("r_debugSurface", "0", CVAR_CHEAT);
	r_nobind = ri.Cvar_Get("r_nobind", "0", CVAR_CHEAT);
	r_clear = ri.Cvar_Get("r_clear", "1", CVAR_CHEAT);
	r_offsetFactor = ri.Cvar_Get("r_offsetFactor", "-1", CVAR_CHEAT);
	r_offsetUnits = ri.Cvar_Get("r_offsetUnits", "-2", CVAR_CHEAT);
	r_forceSpecular = ri.Cvar_Get("r_forceSpecular", "0", CVAR_CHEAT);
	r_specularExponent = ri.Cvar_Get("r_specularExponent", "16", CVAR_CHEAT);
	r_specularExponent2 = ri.Cvar_Get("r_specularExponent2", "3", CVAR_CHEAT);
	r_specularScale = ri.Cvar_Get("r_specularScale", "1.4", CVAR_CHEAT);
	r_normalScale = ri.Cvar_Get("r_normalScale", "1.1", CVAR_CHEAT);
	r_normalMapping = ri.Cvar_Get("r_normalMapping", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_parallaxDepthScale = ri.Cvar_Get("r_parallaxDepthScale", "0.03", CVAR_CHEAT);

	r_wrapAroundLighting = ri.Cvar_Get("r_wrapAroundLighting", "0.7", CVAR_CHEAT);
	r_halfLambertLighting = ri.Cvar_Get("r_halfLambertLighting", "1", CVAR_CHEAT);
	r_rimLighting = ri.Cvar_Get("r_rimLighting", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_rimExponent = ri.Cvar_Get("r_rimExponent", "3", CVAR_CHEAT);
	AssertCvarRange(r_rimExponent, 0.5, 8.0, qfalse);

	r_drawBuffer = ri.Cvar_Get("r_drawBuffer", "GL_BACK", CVAR_CHEAT);
	r_lockpvs = ri.Cvar_Get("r_lockpvs", "0", CVAR_CHEAT);
	r_noportals = ri.Cvar_Get("r_noportals", "0", CVAR_CHEAT);

	r_shadows = ri.Cvar_Get("cg_shadows", "1", CVAR_ARCHIVE | CVAR_LATCH);
	AssertCvarRange(r_shadows, 0, SHADOWING_EVSM32, qtrue);

	r_softShadows = ri.Cvar_Get("r_softShadows", "0", CVAR_ARCHIVE);
	AssertCvarRange(r_softShadows, 0, 6, qtrue);

	r_shadowBlur = ri.Cvar_Get("r_shadowBlur", "2", CVAR_ARCHIVE);

	r_shadowMapQuality = ri.Cvar_Get("r_shadowMapQuality", "3", CVAR_ARCHIVE | CVAR_LATCH);
	AssertCvarRange(r_shadowMapQuality, 0, 4, qtrue);

	r_shadowMapSizeUltra = ri.Cvar_Get("r_shadowMapSizeUltra", "1024", CVAR_ARCHIVE | CVAR_LATCH);
	AssertCvarRange(r_shadowMapSizeUltra, 32, 2048, qtrue);

	r_shadowMapSizeVeryHigh = ri.Cvar_Get("r_shadowMapSizeVeryHigh", "512", CVAR_ARCHIVE | CVAR_LATCH);
	AssertCvarRange(r_shadowMapSizeVeryHigh, 32, 2048, qtrue);

	r_shadowMapSizeHigh = ri.Cvar_Get("r_shadowMapSizeHigh", "256", CVAR_ARCHIVE | CVAR_LATCH);
	AssertCvarRange(r_shadowMapSizeHigh, 32, 2048, qtrue);

	r_shadowMapSizeMedium = ri.Cvar_Get("r_shadowMapSizeMedium", "128", CVAR_ARCHIVE | CVAR_LATCH);
	AssertCvarRange(r_shadowMapSizeMedium, 32, 2048, qtrue);

	r_shadowMapSizeLow = ri.Cvar_Get("r_shadowMapSizeLow", "64", CVAR_ARCHIVE | CVAR_LATCH);
	AssertCvarRange(r_shadowMapSizeLow, 32, 2048, qtrue);


	shadowMapResolutions[0] = r_shadowMapSizeUltra->integer;
	shadowMapResolutions[1] = r_shadowMapSizeVeryHigh->integer;
	shadowMapResolutions[2] = r_shadowMapSizeHigh->integer;
	shadowMapResolutions[3] = r_shadowMapSizeMedium->integer;
	shadowMapResolutions[4] = r_shadowMapSizeLow->integer;



	r_shadowMapSizeSunUltra = ri.Cvar_Get("r_shadowMapSizeSunUltra", "1024", CVAR_ARCHIVE | CVAR_LATCH);
	AssertCvarRange(r_shadowMapSizeSunUltra, 32, 2048, qtrue);

	r_shadowMapSizeSunVeryHigh = ri.Cvar_Get("r_shadowMapSizeSunVeryHigh", "1024", CVAR_ARCHIVE | CVAR_LATCH);
	AssertCvarRange(r_shadowMapSizeSunVeryHigh, 512, 2048, qtrue);

	r_shadowMapSizeSunHigh = ri.Cvar_Get("r_shadowMapSizeSunHigh", "1024", CVAR_ARCHIVE | CVAR_LATCH);
	AssertCvarRange(r_shadowMapSizeSunHigh, 512, 2048, qtrue);

	r_shadowMapSizeSunMedium = ri.Cvar_Get("r_shadowMapSizeSunMedium", "1024", CVAR_ARCHIVE | CVAR_LATCH);
	AssertCvarRange(r_shadowMapSizeSunMedium, 512, 2048, qtrue);

	r_shadowMapSizeSunLow = ri.Cvar_Get("r_shadowMapSizeSunLow", "1024", CVAR_ARCHIVE | CVAR_LATCH);
	AssertCvarRange(r_shadowMapSizeSunLow, 512, 2048, qtrue);


	sunShadowMapResolutions[0] = r_shadowMapSizeSunUltra->integer;
	sunShadowMapResolutions[1] = r_shadowMapSizeSunVeryHigh->integer;
	sunShadowMapResolutions[2] = r_shadowMapSizeSunHigh->integer;
	sunShadowMapResolutions[3] = r_shadowMapSizeSunMedium->integer;
	sunShadowMapResolutions[4] = r_shadowMapSizeSunLow->integer;


	r_shadowOffsetFactor = ri.Cvar_Get("r_shadowOffsetFactor", "0", CVAR_CHEAT);
	r_shadowOffsetUnits = ri.Cvar_Get("r_shadowOffsetUnits", "0", CVAR_CHEAT);
	r_shadowLodBias = ri.Cvar_Get("r_shadowLodBias", "0", CVAR_CHEAT);
	r_shadowLodScale = ri.Cvar_Get("r_shadowLodScale", "0.8", CVAR_CHEAT);
	r_noShadowPyramids = ri.Cvar_Get("r_noShadowPyramids", "0", CVAR_CHEAT);
	r_cullShadowPyramidFaces = ri.Cvar_Get("r_cullShadowPyramidFaces", "0", CVAR_CHEAT);
	r_cullShadowPyramidCurves = ri.Cvar_Get("r_cullShadowPyramidCurves", "1", CVAR_CHEAT);
	r_cullShadowPyramidTriangles = ri.Cvar_Get("r_cullShadowPyramidTriangles", "1", CVAR_CHEAT);
	r_noShadowFrustums = ri.Cvar_Get("r_noShadowFrustums", "0", CVAR_CHEAT);
	r_noLightFrustums = ri.Cvar_Get("r_noLightFrustums", "0", CVAR_CHEAT);

	r_maxPolys = ri.Cvar_Get("r_maxpolys", "10000", 0);	// 600 in vanilla Q3A
	AssertCvarRange(r_maxPolys, 600, 30000, qtrue);

	r_maxPolyVerts = ri.Cvar_Get("r_maxpolyverts", "100000", 0);	// 3000 in vanilla Q3A
	AssertCvarRange(r_maxPolyVerts, 3000, 200000, qtrue);

	r_showTris = ri.Cvar_Get("r_showTris", "0", CVAR_CHEAT);
	r_showSky = ri.Cvar_Get("r_showSky", "0", CVAR_CHEAT);
	r_showShadowVolumes = ri.Cvar_Get("r_showShadowVolumes", "0", CVAR_CHEAT);
	r_showShadowLod = ri.Cvar_Get("r_showShadowLod", "0", CVAR_CHEAT);
	r_showShadowMaps = ri.Cvar_Get("r_showShadowMaps", "0", CVAR_CHEAT);
	r_showSkeleton = ri.Cvar_Get("r_showSkeleton", "0", CVAR_CHEAT);
	r_showEntityTransforms = ri.Cvar_Get("r_showEntityTransforms", "0", CVAR_CHEAT);
	r_showLightTransforms = ri.Cvar_Get("r_showLightTransforms", "0", CVAR_CHEAT);
	r_showLightInteractions = ri.Cvar_Get("r_showLightInteractions", "0", CVAR_CHEAT);
	r_showLightScissors = ri.Cvar_Get("r_showLightScissors", "0", CVAR_CHEAT);
	r_showLightBatches = ri.Cvar_Get("r_showLightBatches", "0", CVAR_CHEAT);
	r_showLightGrid = ri.Cvar_Get("r_showLightGrid", "0", CVAR_CHEAT);
	r_showOcclusionQueries = ri.Cvar_Get("r_showOcclusionQueries", "0", CVAR_CHEAT);
	r_showBatches = ri.Cvar_Get("r_showBatches", "0", CVAR_CHEAT);
	r_showLightMaps = ri.Cvar_Get("r_showLightMaps", "0", CVAR_CHEAT);
	r_showDeluxeMaps = ri.Cvar_Get("r_showDeluxeMaps", "0", CVAR_CHEAT);
	r_showAreaPortals = ri.Cvar_Get("r_showAreaPortals", "0", CVAR_CHEAT);
	r_showCubeProbes = ri.Cvar_Get("r_showCubeProbes", "0", CVAR_CHEAT);
	r_showBspNodes = ri.Cvar_Get("r_showBspNodes", "0", CVAR_CHEAT);
	r_showParallelShadowSplits = ri.Cvar_Get("r_showParallelShadowSplits", "0", CVAR_CHEAT);
	r_showDecalProjectors = ri.Cvar_Get("r_showDecalProjectors", "0", CVAR_CHEAT);

	r_showDeferredDiffuse = ri.Cvar_Get("r_showDeferredDiffuse", "0", CVAR_CHEAT);
	r_showDeferredNormal = ri.Cvar_Get("r_showDeferredNormal", "0", CVAR_CHEAT);
	r_showDeferredSpecular = ri.Cvar_Get("r_showDeferredSpecular", "0", CVAR_CHEAT);
	r_showDeferredPosition = ri.Cvar_Get("r_showDeferredPosition", "0", CVAR_CHEAT);
	r_showDeferredRender = ri.Cvar_Get("r_showDeferredRender", "0", CVAR_CHEAT);
	r_showDeferredLight = ri.Cvar_Get("r_showDeferredLight", "0", CVAR_CHEAT);

	// make sure all the commands added here are also removed in R_Shutdown
	ri.Cmd_AddCommand("imagelist", R_ImageList_f);
	ri.Cmd_AddCommand("shaderlist", R_ShaderList_f);
	ri.Cmd_AddCommand("shaderexp", R_ShaderExp_f);
	ri.Cmd_AddCommand("skinlist", R_SkinList_f);
	ri.Cmd_AddCommand("modellist", R_Modellist_f);
	ri.Cmd_AddCommand("modelist", R_ModeList_f);

#if defined(USE_REFENTITY_ANIMATIONSYSTEM)
	ri.Cmd_AddCommand("animationlist", R_AnimationList_f);
#endif

	ri.Cmd_AddCommand("fbolist", R_FBOList_f);
	ri.Cmd_AddCommand("vbolist", R_VBOList_f);
	ri.Cmd_AddCommand("screenshot", R_ScreenShot_f);
	ri.Cmd_AddCommand("screenshotJPEG", R_ScreenShotJPEG_f);
	ri.Cmd_AddCommand("screenshotPNG", R_ScreenShotPNG_f);
	ri.Cmd_AddCommand("gfxinfo", GfxInfo_f);
//	ri.Cmd_AddCommand("generatemtr", R_GenerateMaterialFile_f);
	ri.Cmd_AddCommand("buildcubemaps", R_BuildCubeMaps);

#if !defined(USE_D3D10)
	ri.Cmd_AddCommand("glsl_restart", GLSL_restart_f);
#endif
}

/*
===============
R_Init
===============
*/
void R_Init(void)
{
	int             err;
	int             i;

	ri.Printf(PRINT_ALL, "----- R_Init -----\n");
	
	// clear all our internal state
	Com_Memset(&tr, 0, sizeof(tr));
	Com_Memset(&backEnd, 0, sizeof(backEnd));
	Com_Memset(&tess, 0, sizeof(tess));

	if((intptr_t) tess.xyz & 15)
		Com_Printf("WARNING: tess.xyz not 16 byte aligned\n");

	// init function tables
	for(i = 0; i < FUNCTABLE_SIZE; i++)
	{
		tr.sinTable[i] = sin(DEG2RAD(i * 360.0f / ((float)(FUNCTABLE_SIZE - 1))));
		tr.squareTable[i] = (i < FUNCTABLE_SIZE / 2) ? 1.0f : -1.0f;
		tr.sawToothTable[i] = (float)i / FUNCTABLE_SIZE;
		tr.inverseSawToothTable[i] = 1.0f - tr.sawToothTable[i];

		if(i < FUNCTABLE_SIZE / 2)
		{
			if(i < FUNCTABLE_SIZE / 4)
			{
				tr.triangleTable[i] = (float)i / (FUNCTABLE_SIZE / 4);
			}
			else
			{
				tr.triangleTable[i] = 1.0f - tr.triangleTable[i - FUNCTABLE_SIZE / 4];
			}
		}
		else
		{
			tr.triangleTable[i] = -tr.triangleTable[i - FUNCTABLE_SIZE / 2];
		}
	}

	R_InitFogTable();

	R_NoiseInit();

	R_Register();

	backEndData[0] = (backEndData_t *) ri.Hunk_Alloc(sizeof(*backEndData[0]), h_low);
	backEndData[0]->polys = (srfPoly_t *) ri.Hunk_Alloc(r_maxPolys->integer * sizeof(srfPoly_t), h_low);
	backEndData[0]->polyVerts = (polyVert_t *) ri.Hunk_Alloc(r_maxPolyVerts->integer * sizeof(polyVert_t), h_low);
	backEndData[0]->polybuffers = (srfPolyBuffer_t *) ri.Hunk_Alloc(r_maxPolys->integer * sizeof(srfPolyBuffer_t), h_low);
	
	if(r_smp->integer)
	{
		backEndData[1] = (backEndData_t *) ri.Hunk_Alloc(sizeof(*backEndData[1]), h_low);
		backEndData[1]->polys = (srfPoly_t *) ri.Hunk_Alloc(r_maxPolys->integer * sizeof(srfPoly_t), h_low);
		backEndData[1]->polyVerts = (polyVert_t *) ri.Hunk_Alloc(r_maxPolyVerts->integer * sizeof(polyVert_t), h_low);
		backEndData[1]->polybuffers = (srfPolyBuffer_t *) ri.Hunk_Alloc(r_maxPolys->integer * sizeof(srfPolyBuffer_t), h_low);
	}
	else
	{
		backEndData[1] = NULL;
	}

	R_ToggleSmpFrame();

#if defined(USE_D3D10)
	if(glConfig.vidWidth == 0)
	{
		DXGI_SWAP_CHAIN_DESC sd;
		SDL_SysWMinfo   info;
		HRESULT hr = S_OK;
		RECT rc;
		ID3D10Texture2D* backBuffer;
		UINT createDeviceFlags = 0;
		int i;

		D3D10_DRIVER_TYPE driverTypes[] =
		{
			D3D10_DRIVER_TYPE_HARDWARE,
			D3D10_DRIVER_TYPE_REFERENCE,
		};
		UINT numDriverTypes = sizeof(driverTypes) / sizeof(driverTypes[0]);

		GLimp_Init();

		ri.Printf(PRINT_ALL, "------- D3D10 Initialization -------\n");

		SDL_VERSION(&info.version);
		if(!SDL_GetWMInfo(&info))
		{
			ri.Error(ERR_FATAL, "R_Init: Failed to obtain HWND from SDL (InputRegistry)");
		}

		//GetClientRect(info.window, &rc);
		//UINT width = rc.right - rc.left;
		//UINT height = rc.bottom - rc.top;


#ifdef _DEBUG
		createDeviceFlags |= D3D10_CREATE_DEVICE_DEBUG;
#endif

		ZeroMemory(&sd, sizeof(sd));
		sd.BufferCount = 1;
		sd.BufferDesc.Width = glConfig.vidWidth;
		sd.BufferDesc.Height = glConfig.vidHeight;
		sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		sd.BufferDesc.RefreshRate.Numerator = 60;
		sd.BufferDesc.RefreshRate.Denominator = 1;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.OutputWindow = info.window;
		sd.SampleDesc.Count = 1;
		sd.SampleDesc.Quality = 0;
		sd.Windowed = TRUE;

#if 1
		// Look for 'NVIDIA PerfHUD' adapter
		// If it is present, override default settings
		IDXGIFactory *pDXGIFactory;
		hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&pDXGIFactory);

		// Search for a PerfHUD adapter.
		UINT nAdapter = 0;
		IDXGIAdapter* adapter = NULL;
		IDXGIAdapter* selectedAdapter = NULL;
		dx.driverType = D3D10_DRIVER_TYPE_HARDWARE;

		ri.Printf(PRINT_ALL, "Looking for PerfHUD...");
		bool gotPerfHUD = false;
		while (pDXGIFactory->EnumAdapters(nAdapter, &adapter) != DXGI_ERROR_NOT_FOUND)
		{
			if(adapter)
			{
				DXGI_ADAPTER_DESC adaptDesc;
				if(SUCCEEDED(adapter->GetDesc(&adaptDesc)))
				{
					const bool isPerfHUD = wcscmp(adaptDesc.Description, L"NVIDIA PerfHUD") == 0;

					// Select the first adapter in normal circumstances or the PerfHUD one if it exists.
					if(nAdapter == 0 || isPerfHUD)
						selectedAdapter = adapter;

					if(isPerfHUD)
					{
						gotPerfHUD = true;
						ri.Printf(PRINT_ALL, "found\n");
						dx.driverType = D3D10_DRIVER_TYPE_REFERENCE;
						break;
					}
				}
			}
			++nAdapter;
		}
		if(!gotPerfHUD)
			ri.Printf(PRINT_ALL, "failed\n");

		hr = D3D10CreateDeviceAndSwapChain( selectedAdapter, dx.driverType, NULL, createDeviceFlags,
											D3D10_SDK_VERSION, &sd, &dx.swapChain, &dx.d3dDevice);

		if(FAILED(hr))
#endif
		{
			ri.Printf(PRINT_ALL, "R_Init: Failed to find PerfHUD");

			for(i = 0; i < numDriverTypes; i++ )
			{
				dx.driverType = driverTypes[i];
				hr = D3D10CreateDeviceAndSwapChain( NULL, dx.driverType, NULL, createDeviceFlags,
												D3D10_SDK_VERSION, &sd, &dx.swapChain, &dx.d3dDevice);
				if(SUCCEEDED(hr))
					break;
			}

			if(FAILED(hr))
			{
				ri.Error(ERR_FATAL, "R_Init: Failed to create a D3D10 device and swap chain");
			}
		}

		// create a render target view
		hr = dx.swapChain->GetBuffer(0, __uuidof(ID3D10Texture2D), (LPVOID*) &backBuffer);
		if(FAILED(hr))
			ri.Error(ERR_FATAL, "R_Init: Failed to get a D3D10 back buffer");

		hr = dx.d3dDevice->CreateRenderTargetView(backBuffer, NULL, &dx.renderTargetView);
		backBuffer->Release();
		if(FAILED(hr))
			ri.Error(ERR_FATAL, "R_Init: Failed to create a D3D10 render target view");

		dx.d3dDevice->OMSetRenderTargets(1, &dx.renderTargetView, NULL);

		// TODO move this to renderer backend

		// setup the viewport
		D3D10_VIEWPORT vp;
		vp.Width = glConfig.vidWidth;
		vp.Height = glConfig.vidHeight;
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
		vp.TopLeftX = 0;
		vp.TopLeftY = 0;
		dx.d3dDevice->RSSetViewports(1, &vp);


#if 0
		 // create the effect
		DWORD dwShaderFlags = D3D10_SHADER_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
		// Set the D3D10_SHADER_DEBUG flag to embed debug information in the shaders.
		// Setting this flag improves the shader debugging experience, but still allows
		// the shaders to be optimized and to run exactly the way they will run in
		// the release configuration of this program.
		dwShaderFlags |= D3D10_SHADER_DEBUG;
#endif

		byte *effectBuffer;
		int effectBufferLen;

		effectBufferLen = ri.FS_ReadFile("shaders/Generic.fx", (void**) &effectBuffer);
		if(effectBufferLen == 0)
		{
			ri.Error(ERR_FATAL, "The FX file cannot be located.  Please run this executable from the directory that contains the FX file.");
		}

		hr = D3DX10CreateEffectFromMemory(effectBuffer, effectBufferLen, "shaders/Generic.fx", NULL, NULL, "fx_4_0", dwShaderFlags, 0,
											 dx.d3dDevice, NULL, NULL, &dx.genericEffect, NULL, NULL);
		if(FAILED(hr))
		{
			ri.Error(ERR_FATAL, "D3DX10CreateEffect failed %i", hr);
		}

		// obtain the technique
		dx.genericTechnique = dx.genericEffect->GetTechniqueByName("Render");

		// define the input layout
		D3D10_INPUT_ELEMENT_DESC layout[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D10_INPUT_PER_VERTEX_DATA, 0 },
		};
		UINT numElements = sizeof(layout) / sizeof(layout[0]);

		// create the input layout
		D3D10_PASS_DESC PassDesc;
		dx.genericTechnique->GetPassByIndex(0)->GetDesc(&PassDesc);

		hr = dx.d3dDevice->CreateInputLayout( layout, numElements, PassDesc.pIAInputSignature,
											  PassDesc.IAInputSignatureSize, &dx.vertexLayout );
		if(FAILED(hr))
			ri.Error(ERR_FATAL, "R_Init: Failed to create a D3D10 input layout");

		// set the input layout
		dx.d3dDevice->IASetInputLayout(dx.vertexLayout);

		// create vertex buffer
		D3DXVECTOR3 vertices[] =
		{
			D3DXVECTOR3( 0.0f, 0.5f, 0.5f ),
			D3DXVECTOR3( 0.5f, -0.5f, 0.5f ),
			D3DXVECTOR3( -0.5f, -0.5f, 0.5f ),
		};
		D3D10_BUFFER_DESC bd;
		bd.Usage = D3D10_USAGE_DEFAULT;
		bd.ByteWidth = sizeof(D3DXVECTOR3) * 3;
		bd.BindFlags = D3D10_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = 0;
		bd.MiscFlags = 0;
		D3D10_SUBRESOURCE_DATA InitData;
		InitData.pSysMem = vertices;
		hr = dx.d3dDevice->CreateBuffer(&bd, &InitData, &dx.vertexBuffer);
		if(FAILED(hr))
			ri.Error(ERR_FATAL, "R_Init: Failed to create a D3D10 input layout");

		// set vertex buffer
		UINT stride = sizeof(D3DXVECTOR3);
		UINT offset = 0;
		dx.d3dDevice->IASetVertexBuffers(0, 1, &dx.vertexBuffer, &stride, &offset);

		// set primitive topology
		dx.d3dDevice->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
#endif

		ri.Printf(PRINT_ALL, "------------------------------------\n");
	}

	// init command buffers and SMP
	R_InitCommandBuffers();

	// print info
	GfxInfo_f();

	// set default state
	//D3D10_SetDefaultState();
#else
	InitOpenGL();

#if !defined(GLSL_COMPILE_STARTUP_ONLY)
	GLSL_InitGPUShaders();
#endif

#endif

	R_InitImages();

	R_InitFBOs();

	if(glConfig.driverType == GLDRV_OPENGL3)
	{
		tr.vao = 0;
		glGenVertexArrays(1, &tr.vao);
		glBindVertexArray(tr.vao);
	}

	R_InitVBOs();

	R_InitShaders();

	R_InitSkins();

	R_ModelInit();

#if defined(USE_REFENTITY_ANIMATIONSYSTEM)
	R_InitAnimations();
#endif

	R_InitFreeType();

	if(glConfig2.textureAnisotropyAvailable)
	{
		AssertCvarRange(r_ext_texture_filter_anisotropic, 0, glConfig2.maxTextureAnisotropy, qfalse);
	}

	if(glConfig2.occlusionQueryBits && glConfig.driverType != GLDRV_MESA)
	{
		glGenQueriesARB(MAX_OCCLUSION_QUERIES, tr.occlusionQueryObjects);
	}

	GL_CheckErrors();

	ri.Printf(PRINT_ALL, "----- finished R_Init -----\n");
}

/*
===============
RE_Shutdown
===============
*/
void RE_Shutdown(qboolean destroyWindow)
{
	ri.Printf(PRINT_ALL, "RE_Shutdown( destroyWindow = %i )\n", destroyWindow);

	ri.Cmd_RemoveCommand("modellist");
	ri.Cmd_RemoveCommand("screenshotPNG");
	ri.Cmd_RemoveCommand("screenshotJPEG");
	ri.Cmd_RemoveCommand("screenshot");
	ri.Cmd_RemoveCommand("imagelist");
	ri.Cmd_RemoveCommand("shaderlist");
	ri.Cmd_RemoveCommand("shaderexp");
	ri.Cmd_RemoveCommand("skinlist");
	ri.Cmd_RemoveCommand("gfxinfo");
	ri.Cmd_RemoveCommand("modelist");
	ri.Cmd_RemoveCommand("shaderstate");
	ri.Cmd_RemoveCommand("animationlist");
	ri.Cmd_RemoveCommand("fbolist");
	ri.Cmd_RemoveCommand("vbolist");
	ri.Cmd_RemoveCommand("generatemtr");
	ri.Cmd_RemoveCommand("buildcubemaps");

	ri.Cmd_RemoveCommand("glsl_restart");

	if(tr.registered)
	{
		R_SyncRenderThread();

		R_ShutdownCommandBuffers();
		R_ShutdownImages();
		R_ShutdownVBOs();
		R_ShutdownFBOs();

		if(glConfig.driverType == GLDRV_OPENGL3)
		{
			glDeleteVertexArrays(1, &tr.vao);
			tr.vao = 0;
		}

		if(glConfig2.occlusionQueryBits && glConfig.driverType != GLDRV_MESA)
		{
			glDeleteQueriesARB(MAX_OCCLUSION_QUERIES, tr.occlusionQueryObjects);

			if(tr.world)
			{
				int				j;
				bspNode_t      *node;
				//trRefLight_t   *light;

				for(j = 0; j < tr.world->numnodes; j++)
				{
					node = &tr.world->nodes[j];

					glDeleteQueriesARB(MAX_VIEWS, node->occlusionQueryObjects);
				}

				/*
				for(j = 0; j < tr.world->numLights; j++)
				{
					light = &tr.world->lights[j];

					glDeleteQueriesARB(MAX_VIEWS, light->occlusionQueryObjects);
				}
				*/
			}
		}

#if !defined(GLSL_COMPILE_STARTUP_ONLY)
		GLSL_ShutdownGPUShaders();
#endif

		//GLimp_ShutdownRenderThread();
	}

	R_DoneFreeType();

	// shut down platform specific OpenGL stuff

	// Tr3B: this should be always executed if we want to avoid some GLSL problems with SMP
	// Update: Having the JVM running with all its threads can cause problems with an old OpenGL context.
	// Maybe an OpenGL driver problem. It is safer to destroy the context in that case or you will get really weird crashes when rendering stuff.
	//

#if !defined(SMP)// && !defined(USE_JAVA)
	if(destroyWindow)
#endif
	{

#if defined(GLSL_COMPILE_STARTUP_ONLY)
		GLSL_ShutdownGPUShaders();
#endif

		GLimp_Shutdown();

#if defined(USE_D3D10)
		if(dx.d3dDevice)
			dx.d3dDevice->ClearState();

		if(dx.renderTargetView)
			dx.renderTargetView->Release();

		if(dx.swapChain)
			dx.swapChain->Release();

		if(dx.d3dDevice)
			dx.d3dDevice->Release();
#endif
	}

	tr.registered = qfalse;
}


/*
=============
RE_EndRegistration

Touch all images to make sure they are resident
=============
*/
void RE_EndRegistration(void)
{
	R_SyncRenderThread();

	/*
	   if(!Sys_LowPhysicalMemory())
	   {
	   RB_ShowImages();
	   }
	 */
}

static void RE_PurgeCache(void)
{
	ri.Printf(PRINT_ALL, S_COLOR_RED "TODO RE_PurgeCache\n");

	/*
	R_PurgeShaders(9999999);
	R_PurgeBackupImages(9999999);
	R_PurgeModels(9999999);
	*/
}


/*
=====================
GetRefAPI
=====================
*/
//#if defined(__cplusplus)
//extern "C" {
//#endif
refexport_t* GetRefAPI(int apiVersion, refimport_t * rimp)
{
	static refexport_t re;

	ri = *rimp;

	ri.Printf(PRINT_ALL, "GetRefAPI()\n");

	Com_Memset(&re, 0, sizeof(re));

	if(apiVersion != REF_API_VERSION)
	{
		ri.Printf(PRINT_ALL, "Mismatched REF_API_VERSION: expected %i, got %i\n", REF_API_VERSION, apiVersion);
		return NULL;
	}

	// the RE_ functions are Renderer Entry points

	// Q3A BEGIN
	re.Shutdown = RE_Shutdown;

	re.BeginRegistration = RE_BeginRegistration;
	re.RegisterModel = RE_RegisterModel;
	
	re.RegisterSkin = RE_RegisterSkin;
	re.RegisterShader = RE_RegisterShader;
	re.RegisterShaderNoMip = RE_RegisterShaderNoMip;
#if !defined(COMPAT_ET)
	re.RegisterShaderLightAttenuation = RE_RegisterShaderLightAttenuation;
#endif

	re.LoadWorld = RE_LoadWorldMap;
	re.SetWorldVisData = RE_SetWorldVisData;
	re.EndRegistration = RE_EndRegistration;

	re.BeginFrame = RE_BeginFrame;
	re.EndFrame = RE_EndFrame;

	re.MarkFragments = R_MarkFragments;

	re.LerpTag = RE_LerpTagET;

	re.ModelBounds = R_ModelBounds;

	re.ClearScene = RE_ClearScene;
	re.AddRefEntityToScene = RE_AddRefEntityToScene;

#if defined(COMPAT_ET)
	re.AddPolyToScene = RE_AddPolyToSceneET;
	re.AddPolysToScene = RE_AddPolysToScene;
#else
	re.AddPolyToScene = RE_AddPolyToSceneQ3A;
#endif
#if !defined(COMPAT_ET)
	re.LightForPoint = R_LightForPoint;
#endif

#if defined(COMPAT_ET)
	re.AddLightToScene = RE_AddDynamicLightToSceneET;
	re.AddAdditiveLightToScene = RE_AddDynamicLightToSceneQ3A;
#else

#endif

	re.RenderScene = RE_RenderScene;

	re.SetColor = RE_SetColor;
	re.SetClipRegion = RE_SetClipRegion;
	re.DrawStretchPic = RE_StretchPic;
	re.DrawStretchRaw = RE_StretchRaw;
	re.UploadCinematic = RE_UploadCinematic;

	re.DrawRotatedPic = RE_RotatedPic;
	re.Add2dPolys = RE_2DPolyies;
	re.DrawStretchPicGradient = RE_StretchPicGradient;

	re.RegisterFont = RE_RegisterFont;
	re.RemapShader = R_RemapShader;
	re.GetEntityToken = R_GetEntityToken;
	re.inPVS = R_inPVS;
	// Q3A END

	// ET BEGIN
#if defined(COMPAT_ET)
	re.GetSkinModel = RE_GetSkinModel;
	re.GetShaderFromModel = RE_GetShaderFromModel;

	re.ProjectDecal = RE_ProjectDecal;
	re.ClearDecals = RE_ClearDecals;

	re.DrawDebugPolygon = R_DebugPolygon;
	re.DrawDebugText = R_DebugText;

	re.SaveViewParms = RE_SaveViewParms;
	re.RestoreViewParms = RE_RestoreViewParms;

	re.AddCoronaToScene = RE_AddCoronaToScene;
	re.AddPolyBufferToScene = RE_AddPolyBufferToScene;
	
	re.SetFog = RE_SetFog;
	re.SetGlobalFog = RE_SetGlobalFog;

	re.purgeCache = RE_PurgeCache;

	re.LoadDynamicShader = RE_LoadDynamicShader;
	re.GetTextureId = RE_GetTextureId;
	re.RenderToTexture = RE_RenderToTexture;
	re.Finish = RE_Finish;
#endif
	// ET END

	// XreaL BEGIN
	re.TakeVideoFrame = RE_TakeVideoFrame;

#if !defined(COMPAT_ET)
	re.TakeScreenshotPNG = RB_TakeScreenshotPNG;
#endif

#if defined(USE_REFLIGHT)
	re.RegisterShaderLightAttenuation = RE_RegisterShaderLightAttenuation;
	re.AddRefLightToScene = RE_AddRefLightToScene;
#endif

#if defined(USE_REFENTITY_ANIMATIONSYSTEM)
	re.RegisterAnimation = RE_RegisterAnimation;
	re.CheckSkeleton = RE_CheckSkeleton;
	re.BuildSkeleton = RE_BuildSkeleton;
	re.BlendSkeleton = RE_BlendSkeleton;
	re.BoneIndex = RE_BoneIndex;
	re.AnimNumFrames = RE_AnimNumFrames;
	re.AnimFrameRate = RE_AnimFrameRate;
#endif

	// XreaL END

	return &re;
}


#ifndef REF_HARD_LINKED

// this is only here so the functions in q_shared.c and q_math.c can link

#if defined(__cplusplus)
extern "C" {
#endif
void QDECL Com_Printf(const char *msg, ...)
{
	va_list         argptr;
	char            text[1024];

	va_start(argptr, msg);
	Q_vsnprintf(text, sizeof(text), msg, argptr);
	va_end(argptr);

	ri.Printf(PRINT_ALL, "%s", text);
}

void QDECL Com_DPrintf(const char *msg, ...)
{
	va_list         argptr;
	char            text[1024];

	va_start(argptr, msg);
	Q_vsnprintf(text, sizeof(text), msg, argptr);
	va_end(argptr);

	ri.Printf(PRINT_DEVELOPER, "%s", text);
}

void QDECL Com_Error(int level, const char *error, ...)
{
	va_list         argptr;
	char            text[1024];

	va_start(argptr, error);
	Q_vsnprintf(text, sizeof(text), error, argptr);
	va_end(argptr);

	ri.Error(level, "%s", text);
}
#if defined(__cplusplus)
}
#endif

#endif

#if defined(__cplusplus)
}
#endif

