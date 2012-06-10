/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

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

// tr_init.c -- functions that are not called every frame

#include "tr_local.h"

glconfig_t  glConfig;
glconfig2_t glConfig2;
glstate_t   glState;

int         maxAnisotropy = 0;
float       displayAspect = 0.0f;

static void GfxInfo_f( void );

cvar_t      *r_flareSize;
cvar_t      *r_flareFade;

cvar_t      *r_railWidth;
cvar_t      *r_railCoreWidth;
cvar_t      *r_railSegmentLength;

cvar_t      *r_ignoreFastPath;

cvar_t      *r_verbose;
cvar_t      *r_ignore;

cvar_t      *r_displayRefresh;

cvar_t      *r_detailTextures;

cvar_t      *r_znear;
cvar_t      *r_zfar;

cvar_t      *r_smp;
cvar_t      *r_showSmp;
cvar_t      *r_skipBackEnd;

cvar_t      *r_ignorehwgamma;
cvar_t      *r_measureOverdraw;

cvar_t      *r_inGameVideo;
cvar_t      *r_fastsky;
cvar_t      *r_drawSun;
cvar_t      *r_dynamiclight;
cvar_t      *r_dlightBacks;

cvar_t      *r_lodbias;
cvar_t      *r_lodscale;

cvar_t      *r_norefresh;
cvar_t      *r_drawentities;
cvar_t      *r_drawworld;
cvar_t      *r_drawfoliage; // ydnar
cvar_t      *r_speeds;

//cvar_t    *r_fullbright; // JPW NERVE removed per atvi request
cvar_t      *r_novis;
cvar_t      *r_nocull;
cvar_t      *r_facePlaneCull;
cvar_t      *r_showcluster;
cvar_t      *r_nocurves;

cvar_t      *r_allowExtensions;

cvar_t      *r_ext_compressed_textures;
cvar_t      *r_ext_gamma_control;
cvar_t      *r_ext_multitexture;
cvar_t      *r_ext_compiled_vertex_array;
cvar_t      *r_ext_texture_env_add;

cvar_t      *r_clampToEdge; // ydnar: opengl 1.2 GL_CLAMP_TO_EDGE SUPPORT

//----(SA)  added
cvar_t      *r_ext_texture_filter_anisotropic;

cvar_t      *r_ext_NV_fog_dist;
cvar_t      *r_nv_fogdist_mode;

cvar_t      *r_ext_ATI_pntriangles;
cvar_t      *r_ati_truform_tess; //
cvar_t      *r_ati_truform_normalmode; // linear/quadratic
cvar_t      *r_ati_truform_pointmode; // linear/cubic

//----(SA)  end

cvar_t *r_ati_fsaa_samples; //DAJ valids are 1, 2, 4

cvar_t *r_ignoreGLErrors;
cvar_t *r_logFile;

cvar_t *r_stencilbits;
cvar_t *r_depthbits;
cvar_t *r_colorbits;
cvar_t *r_stereo;
cvar_t *r_primitives;
cvar_t *r_texturebits;
cvar_t *r_ext_multisample;

cvar_t *r_drawBuffer;
cvar_t *r_glDriver;
cvar_t *r_glIgnoreWicked3D;
cvar_t *r_lightmap;
cvar_t *r_uiFullScreen;
cvar_t *r_shadows;
cvar_t *r_portalsky; //----(SA)  added
cvar_t *r_flares;
cvar_t *r_mode;
cvar_t *r_oldMode; // ydnar
cvar_t *r_nobind;
cvar_t *r_singleShader;
cvar_t *r_roundImagesDown;
cvar_t *r_colorMipLevels;
cvar_t *r_picmip;
cvar_t *r_showtris;
cvar_t *r_trisColor;
cvar_t *r_showsky;
cvar_t *r_shownormals;
cvar_t *r_normallength;
cvar_t *r_showmodelbounds;
cvar_t *r_finish;
cvar_t *r_clear;
cvar_t *r_swapInterval;
cvar_t *r_textureMode;
cvar_t *r_textureAnisotropy;
cvar_t *r_offsetFactor;
cvar_t *r_offsetUnits;
cvar_t *r_gamma;
cvar_t *r_intensity;
cvar_t *r_lockpvs;
cvar_t *r_noportals;
cvar_t *r_portalOnly;

cvar_t *r_subdivisions;
cvar_t *r_lodCurveError;

cvar_t *r_fullscreen;

cvar_t *r_customwidth;
cvar_t *r_customheight;
cvar_t *r_customaspect;

cvar_t *r_overBrightBits;
cvar_t *r_mapOverBrightBits;

cvar_t *r_debugSurface;
cvar_t *r_simpleMipMaps;

cvar_t *r_showImages;

cvar_t *r_ambientScale;
cvar_t *r_directedScale;
cvar_t *r_debugLight;
cvar_t *r_debugSort;
cvar_t *r_printShaders;
cvar_t *r_saveFontData;

// Ridah
cvar_t *r_cache;
cvar_t *r_cacheShaders;
cvar_t *r_cacheModels;

cvar_t *r_cacheGathering;

cvar_t *r_buildScript;

cvar_t *r_bonesDebug;

// done.

// Rafael - wolf fog
cvar_t *r_wolffog;

// done

cvar_t *r_fontScale;

cvar_t         *r_highQualityVideo;
cvar_t         *r_rmse;

cvar_t         *r_maxpolys;
int            max_polys;
cvar_t         *r_maxpolyverts;
int            max_polyverts;

vec4hack_t     tess_xyz[ SHADER_MAX_VERTEXES ];
vec4hack_t     tess_normal[ SHADER_MAX_VERTEXES ];
vec2hack_t     tess_texCoords0[ SHADER_MAX_VERTEXES ];
vec2hack_t     tess_texCoords1[ SHADER_MAX_VERTEXES ];
glIndex_t      tess_indexes[ SHADER_MAX_INDEXES ];
color4ubhack_t tess_vertexColors[ SHADER_MAX_VERTEXES ];

/*
The tessellation level and normal generation mode are specified with:

        void glPNTriangles{if}ATI(enum pname, T param)

        If <pname> is:
                GL_PN_TRIANGLES_NORMAL_MODE_ATI -
                        <param> must be one of the symbolic constants:
                                - GL_PN_TRIANGLES_NORMAL_MODE_LINEAR_ATI or
                                - GL_PN_TRIANGLES_NORMAL_MODE_QUADRATIC_ATI
                        which will select linear or quadratic normal interpolation respectively.
                GL_PN_TRIANGLES_POINT_MODE_ATI -
                        <param> must be one of the symbolic  constants:
                                - GL_PN_TRIANGLES_POINT_MODE_LINEAR_ATI or
                                - GL_PN_TRIANGLES_POINT_MODE_CUBIC_ATI
                        which will select linear or cubic interpolation respectively.
                GL_PN_TRIANGLES_TESSELATION_LEVEL_ATI -
                        <param> should be a value specifying the number of evaluation points on each edge.  This value must be
                        greater than 0 and less than or equal to the value given by GL_MAX_PN_TRIANGLES_TESSELATION_LEVEL_ATI.

        An INVALID_VALUE error will be generated if the value for <param> is less than zero or greater than the max value.

Associated 'gets':
Get Value                               Get Command Type     Minimum Value                Attribute
---------                               ----------- ----     ------------               ---------
PN_TRIANGLES_ATI            IsEnabled   B   False                                       PN Triangles/enable
PN_TRIANGLES_NORMAL_MODE_ATI      GetIntegerv Z2    PN_TRIANGLES_NORMAL_MODE_QUADRATIC_ATI    PN Triangles
PN_TRIANGLES_POINT_MODE_ATI       GetIntegerv Z2    PN_TRIANGLES_POINT_MODE_CUBIC_ATI     PN Triangles
PN_TRIANGLES_TESSELATION_LEVEL_ATI    GetIntegerv Z+    1                     PN Triangles
MAX_PN_TRIANGLES_TESSELATION_LEVEL_ATI  GetIntegerv Z+    1                     -




*/
//----(SA)  end

static void AssertCvarRange( cvar_t *cv, float minVal, float maxVal, qboolean shouldBeIntegral )
{
	if ( shouldBeIntegral )
	{
		if ( ( int ) cv->value != cv->integer )
		{
			ri.Printf( PRINT_WARNING, "WARNING: cvar '%s' must be integral (%f)\n", cv->name, cv->value );
			ri.Cvar_Set( cv->name, va( "%d", cv->integer ) );
		}
	}

	if ( cv->value < minVal )
	{
		ri.Printf( PRINT_WARNING, "WARNING: cvar '%s' out of range (%f < %f)\n", cv->name, cv->value, minVal );
		ri.Cvar_Set( cv->name, va( "%f", minVal ) );
	}
	else if ( cv->value > maxVal )
	{
		ri.Printf( PRINT_WARNING, "WARNING: cvar '%s' out of range (%f > %f)\n", cv->name, cv->value, maxVal );
		ri.Cvar_Set( cv->name, va( "%f", maxVal ) );
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
static void InitOpenGL( void )
{
	char renderer_buffer[ 1024 ];

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

	if ( glConfig.vidWidth == 0 )
	{
		GLint temp;

		GLimp_Init();

		strcpy( renderer_buffer, glConfig.renderer_string );
		Q_strlwr( renderer_buffer );

		// OpenGL driver constants
		glGetIntegerv( GL_MAX_TEXTURE_SIZE, &temp );
		glConfig.maxTextureSize = temp;

		// stubbed or broken drivers may have reported 0...
		if ( glConfig.maxTextureSize <= 0 )
		{
			glConfig.maxTextureSize = 0;
		}
	}

	// init command buffers and SMP
	R_InitCommandBuffers();

	// print info
	GfxInfo_f();

	// set default state
	GL_SetDefaultState();
}

/*
==================
GL_CheckErrors
==================
*/
void GL_CheckErrors_( const char *fileName, int line )
{
	int  err;
	char s[ 128 ];

	if ( glConfig.smpActive )
	{
		// we can't print onto the console while rendering in another thread
		return;
	}

	if ( r_ignoreGLErrors->integer )
	{
		return;
	}

	err = glGetError();

	if ( err == GL_NO_ERROR )
	{
		return;
	}

	switch ( err )
	{
		case GL_INVALID_ENUM:
			strcpy( s, "GL_INVALID_ENUM" );
			break;

		case GL_INVALID_VALUE:
			strcpy( s, "GL_INVALID_VALUE" );
			break;

		case GL_INVALID_OPERATION:
			strcpy( s, "GL_INVALID_OPERATION" );
			break;

		case GL_STACK_OVERFLOW:
			strcpy( s, "GL_STACK_OVERFLOW" );
			break;

		case GL_STACK_UNDERFLOW:
			strcpy( s, "GL_STACK_UNDERFLOW" );
			break;

		case GL_OUT_OF_MEMORY:
			strcpy( s, "GL_OUT_OF_MEMORY" );
			break;

		case GL_TABLE_TOO_LARGE:
			strcpy( s, "GL_TABLE_TOO_LARGE" );
			break;

		case GL_INVALID_FRAMEBUFFER_OPERATION_EXT:
			strcpy( s, "GL_INVALID_FRAMEBUFFER_OPERATION_EXT" );
			break;

		default:
			Com_sprintf( s, sizeof( s ), "0x%X", err );
			break;
	}

	ri.Error( ERR_FATAL, "caught OpenGL error: %s in file %s line %i", s, fileName, line );
}

/*
** R_GetModeInfo
*/
typedef struct vidmode_s
{
	const char *description;
	int        width, height;
	float      pixelAspect; // pixel width / height
} vidmode_t;

static const vidmode_t r_vidModes[] =
{
	{ " 320x240",           320,  240, 1 },
	{ " 400x300",           400,  300, 1 },
	{ " 512x384",           512,  384, 1 },
	{ " 640x480",           640,  480, 1 },
	{ " 800x600",           800,  600, 1 },
	{ " 960x720",           960,  720, 1 },
	{ "1024x768",          1024,  768, 1 },
	{ "1152x864",          1152,  864, 1 },
	{ "1280x720  (16:9)",  1280,  720, 1 },
	{ "1280x768  (16:10)", 1280,  768, 1 },
	{ "1280x800  (16:10)", 1280,  800, 1 },
	{ "1280x1024",         1280, 1024, 1 },
	{ "1360x768  (16:9)",  1360,  768,  1 },
	{ "1440x900  (16:10)", 1440,  900, 1 },
	{ "1680x1050 (16:10)", 1680, 1050, 1 },
	{ "1600x1200",         1600, 1200, 1 },
	{ "1920x1080 (16:9)",  1920, 1080, 1 },
	{ "1920x1200 (16:10)", 1920, 1200, 1 },
	{ "2048x1536",         2048, 1536, 1 },
	{ "2560x1600 (16:10)", 2560, 1600, 1 },
};
static const int s_numVidModes = ( sizeof( r_vidModes ) / sizeof( r_vidModes[ 0 ] ) );

qboolean R_GetModeInfo( int *width, int *height, float *windowAspect, int mode )
{
	const vidmode_t *vm;

	if ( mode < -2 )
	{
		return qfalse;
	}

	if ( mode >= s_numVidModes )
	{
		return qfalse;
	}

	if ( mode == -2 )
	{
		// Must set width and height to display size before calling this function!
		*windowAspect = ( float ) *width / *height;
	}
	else if ( mode == -1 )
	{
		*width = r_customwidth->integer;
		*height = r_customheight->integer;
		*windowAspect = r_customaspect->value;
	}
	else
	{
		vm = &r_vidModes[ mode ];

		*width = vm->width;
		*height = vm->height;
		*windowAspect = ( float ) vm->width / ( vm->height * vm->pixelAspect );
	}

	return qtrue;
}

/*
** R_ModeList_f
*/
static void R_ModeList_f( void )
{
	int i;

	ri.Printf( PRINT_ALL, "\n" );

	for ( i = 0; i < s_numVidModes; i++ )
	{
		ri.Printf( PRINT_ALL, "Mode %2d: %s\n", i, r_vidModes[ i ].description );
	}

	ri.Printf( PRINT_ALL, "\n" );
}

/*
==============================================================================

                                                SCREEN SHOTS

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
static byte *RB_ReadPixels( int x, int y, int width, int height, size_t offset )
{
	GLint packAlign;
	int   lineLen, paddedLineLen;
	byte  *buffer, *pixels;
	int   i;

	glGetIntegerv( GL_PACK_ALIGNMENT, &packAlign );

	lineLen = width * 3;
	paddedLineLen = PAD( lineLen, packAlign );

	// Allocate a few more bytes so that we can choose an alignment we like
	buffer = ( byte * ) ri.Hunk_AllocateTempMemory( offset + paddedLineLen * height + packAlign - 1 );

	pixels = ( byte * ) PADP( buffer + offset, packAlign );
	glReadPixels( x, y, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels );

	// Drop alignment and line padding bytes
	for ( i = 0; i < height; ++i )
	{
		memmove( buffer + offset + i * lineLen, pixels + i * paddedLineLen, lineLen );
	}

	return buffer;
}

/*
==================
R_TakeScreenshot
==================
*/
static void R_TakeScreenshot( int x, int y, int width, int height, char *fileName )
{
	byte *buffer;
	int  dataSize;
	byte *end, *p;

	// with 18 bytes for the TGA file header
	buffer = RB_ReadPixels( x, y, width, height, 18 );
	Com_Memset( buffer, 0, 18 );

	buffer[ 2 ] = 2; // uncompressed type
	buffer[ 12 ] = width & 255;
	buffer[ 13 ] = width >> 8;
	buffer[ 14 ] = height & 255;
	buffer[ 15 ] = height >> 8;
	buffer[ 16 ] = 24; // pixel size

	dataSize = 3 * width * height;

	// RGB to BGR
	end = buffer + 18 + dataSize;

	for ( p = buffer + 18; p < end; p += 3 )
	{
		byte temp = p[ 0 ];
		p[ 0 ] = p[ 2 ];
		p[ 2 ] = temp;
	}

	if ( tr.overbrightBits > 0 && glConfig.deviceSupportsGamma )
	{
		R_GammaCorrect( buffer + 18, dataSize );
	}

	ri.FS_WriteFile( fileName, buffer, 18 + dataSize );

	ri.Hunk_FreeTempMemory( buffer );
}

/*
==================
R_TakeScreenshotJPEG
==================
*/
static void R_TakeScreenshotJPEG( int x, int y, int width, int height, char *fileName )
{
	byte *buffer = RB_ReadPixels( x, y, width, height, 0 );

	if ( tr.overbrightBits > 0 && glConfig.deviceSupportsGamma )
	{
		R_GammaCorrect( buffer, 3 * width * height );
	}

	SaveJPG( fileName, 90, width, height, buffer );
	ri.Hunk_FreeTempMemory( buffer );
}

/*
==================
R_TakeScreenshotPNG
==================
*/
static void R_TakeScreenshotPNG( int x, int y, int width, int height, char *fileName )
{
	byte *buffer = RB_ReadPixels( x, y, width, height, 0 );

	if ( tr.overbrightBits > 0 && glConfig.deviceSupportsGamma )
	{
		R_GammaCorrect( buffer, 3 * width * height );
	}

	SavePNG( fileName, buffer, width, height, 3, qfalse );
	ri.Hunk_FreeTempMemory( buffer );
}

/*
==================
R_ScreenshotFilename
==================
*/
void R_ScreenshotFilename( int lastNumber, char *fileName )
{
	if ( lastNumber < 0 || lastNumber > 9999 )
	{
		Com_sprintf( fileName, MAX_OSPATH, "screenshots/shot9999.tga" );
		return;
	}

	Com_sprintf( fileName, MAX_OSPATH, "screenshots/shot%04i.tga", lastNumber );
}

/*
==============
R_ScreenshotFilenameJPEG
==============
*/
void R_ScreenshotFilenameJPEG( int lastNumber, char *fileName )
{
	if ( lastNumber < 0 || lastNumber > 9999 )
	{
		Com_sprintf( fileName, MAX_OSPATH, "screenshots/shot9999.jpg" );
		return;
	}

	Com_sprintf( fileName, MAX_OSPATH, "screenshots/shot%04i.jpg", lastNumber );
}

/*
==============
R_ScreenshotFilenamePNG
==============
*/
void R_ScreenshotFilenamePNG( int lastNumber, char *fileName )
{
	if ( lastNumber < 0 || lastNumber > 9999 )
	{
		Com_sprintf( fileName, MAX_OSPATH, "screenshots/shot9999.png" );
		return;
	}

	Com_sprintf( fileName, MAX_OSPATH, "screenshots/shot%04i.png", lastNumber );
}

/*
==================
R_ScreenShot_f

screenshot[JPEG|PNG] [silent | <filename>]
==================
*/
void R_ScreenShot_f( void )
{
	char       checkname[ MAX_OSPATH ];
	int        len;
	static int lastNumber = -1;
	ssFormat_t format;
	qboolean   silent;

	if ( !Q_stricmp( ri.Cmd_Argv( 0 ), "screenshotJPEG" ) )
	{
		format = SSF_JPEG;
	}
	else if ( !Q_stricmp( ri.Cmd_Argv( 0 ), "screenshotPNG" ) )
	{
		format = SSF_PNG;
	}
	else
	{
		format = SSF_TGA;
	}

	if ( !strcmp( ri.Cmd_Argv( 1 ), "silent" ) )
	{
		silent = qtrue;
	}
	else
	{
		silent = qfalse;
	}

	if ( ri.Cmd_Argc() == 2 && !silent )
	{
		// explicit filename
		switch ( format )
		{
			case SSF_TGA:
				Com_sprintf( checkname, MAX_OSPATH, "screenshots/%s.tga", ri.Cmd_Argv( 1 ) );
				break;

			case SSF_JPEG:
				Com_sprintf( checkname, MAX_OSPATH, "screenshots/%s.jpeg", ri.Cmd_Argv( 1 ) );
				break;

			case SSF_PNG:
				Com_sprintf( checkname, MAX_OSPATH, "screenshots/%s.png", ri.Cmd_Argv( 1 ) );
				break;
		}
	}
	else
	{
		// scan for a free filename

		// if we have saved a previous screenshot, don't scan
		// again, because recording demo avis can involve
		// thousands of shots
		if ( lastNumber == -1 )
		{
			lastNumber = 0;
		}

		// scan for a free number
		for ( ; lastNumber <= 9999; lastNumber++ )
		{
			switch ( format )
			{
				case SSF_TGA:
					R_ScreenshotFilename( lastNumber, checkname );
					break;

				case SSF_JPEG:
					R_ScreenshotFilenameJPEG( lastNumber, checkname );
					break;

				case SSF_PNG:
					R_ScreenshotFilenamePNG( lastNumber, checkname );
					break;
			}

			len = ri.FS_ReadFile( checkname, NULL );

			if ( len <= 0 )
			{
				break; // file doesn't exist
			}
		}

		if ( lastNumber >= 10000 )
		{
			ri.Printf( PRINT_ALL, "ScreenShot: Couldn't create a file\n" );
			return;
		}

		lastNumber++;
	}

	switch ( format )
	{
		case SSF_TGA:
			R_TakeScreenshot( 0, 0, glConfig.vidWidth, glConfig.vidHeight, checkname );
			break;

		case SSF_JPEG:
			R_TakeScreenshotJPEG( 0, 0, glConfig.vidWidth, glConfig.vidHeight, checkname );
			break;

		case SSF_PNG:
			R_TakeScreenshotPNG( 0, 0, glConfig.vidWidth, glConfig.vidHeight, checkname );
			break;
	}

	if ( !silent )
	{
		ri.Printf( PRINT_ALL, "Wrote %s\n", checkname );
	}
}

//============================================================================

/*
==================
RB_TakeVideoFrameCmd
==================
*/
const void     *RB_TakeVideoFrameCmd( const void *data )
{
	const videoFrameCommand_t *cmd;
	GLint                     packAlign;
	int                       lineLen, captureLineLen;
	byte                      *pixels;
	int                       i;
	int                       outputSize;
	int                       j;
	int                       aviLineLen;

	cmd = ( const videoFrameCommand_t * ) data;

	// RB: it is possible to we still have a videoFrameCommand_t but we already stopped
	// video recording
	if ( ri.CL_VideoRecording() )
	{
		// take care of alignment issues for reading RGB images..

		glGetIntegerv( GL_PACK_ALIGNMENT, &packAlign );

		lineLen = cmd->width * 3;
		captureLineLen = PAD( lineLen, packAlign );

		pixels = ( byte * ) PADP( cmd->captureBuffer, packAlign );
		glReadPixels( 0, 0, cmd->width, cmd->height, GL_RGB, GL_UNSIGNED_BYTE, pixels );

		if ( tr.overbrightBits > 0 && glConfig.deviceSupportsGamma )
		{
			// this also runs over the padding...
			R_GammaCorrect( pixels, captureLineLen * cmd->height );
		}

		if ( cmd->motionJpeg )
		{
			// Drop alignment and line padding bytes
			for ( i = 0; i < cmd->height; ++i )
			{
				memmove( cmd->captureBuffer + i * lineLen, pixels + i * captureLineLen, lineLen );
			}

			outputSize = SaveJPGToBuffer( cmd->encodeBuffer, 3 * cmd->width * cmd->height, 90, cmd->width, cmd->height, cmd->captureBuffer );
			ri.CL_WriteAVIVideoFrame( cmd->encodeBuffer, outputSize );
		}
		else
		{
			aviLineLen = PAD( lineLen, AVI_LINE_PADDING );

			for ( i = 0; i < cmd->height; ++i )
			{
				for ( j = 0; j < lineLen; j += 3 )
				{
					cmd->encodeBuffer[ i * aviLineLen + j + 0 ] = pixels[ i * captureLineLen + j + 2 ];
					cmd->encodeBuffer[ i * aviLineLen + j + 1 ] = pixels[ i * captureLineLen + j + 1 ];
					cmd->encodeBuffer[ i * aviLineLen + j + 2 ] = pixels[ i * captureLineLen + j + 0 ];
				}

				while ( j < aviLineLen )
				{
					cmd->encodeBuffer[ i * aviLineLen + j++ ] = 0;
				}
			}

			ri.CL_WriteAVIVideoFrame( cmd->encodeBuffer, aviLineLen * cmd->height );
		}
	}

	return ( const void * )( cmd + 1 );
}

//============================================================================

/*
** GL_SetDefaultState
*/
void GL_SetDefaultState( void )
{
	glClearDepth( 1.0f );

	glCullFace( GL_FRONT );

	glColor4f( 1, 1, 1, 1 );

	// initialize downstream texture unit if we're running
	// in a multitexture environment
	if ( GLEW_ARB_multitexture )
	{
		GL_SelectTexture( 1 );
		GL_TextureMode( r_textureMode->string );
		GL_TextureAnisotropy( r_textureAnisotropy->value );
		GL_TexEnv( GL_MODULATE );
		glDisable( GL_TEXTURE_2D );
		GL_SelectTexture( 0 );
	}

	glEnable( GL_TEXTURE_2D );
	GL_TextureMode( r_textureMode->string );
	GL_TextureAnisotropy( r_textureAnisotropy->value );
	GL_TexEnv( GL_MODULATE );

	glShadeModel( GL_SMOOTH );
	glDepthFunc( GL_LEQUAL );

	// the vertex array is always enabled, but the color and texture
	// arrays are enabled and disabled around the compiled vertex array call
	glEnableClientState( GL_VERTEX_ARRAY );

	//
	// make sure our GL state vector is set correctly
	//
	glState.glStateBits = GLS_DEPTHTEST_DISABLE | GLS_DEPTHMASK_TRUE;

	glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	glDepthMask( GL_TRUE );
	glDisable( GL_DEPTH_TEST );
	glEnable( GL_SCISSOR_TEST );
	glDisable( GL_CULL_FACE );
	glDisable( GL_BLEND );

//----(SA)  added.
#if 0

	// ATI pn_triangles
	if ( GLEW_ATI_pn_triangles )
	{
		int maxtess;

		// get max supported tesselation
		glGetIntegerv( GL_MAX_PN_TRIANGLES_TESSELATION_LEVEL_ATI, ( GLint * ) &maxtess );
#ifdef __MACOS__
		glConfig.ATIMaxTruformTess = 7;
#else
		glConfig.ATIMaxTruformTess = maxtess;
#endif

		// cap if necessary
		if ( r_ati_truform_tess->value > maxtess )
		{
			ri.Cvar_Set( "r_ati_truform_tess", va( "%d", maxtess ) );
		}

		// set Wolf defaults
		glPNTrianglesfATI( GL_PN_TRIANGLES_TESSELATION_LEVEL_ATI, r_ati_truform_tess->value );
	}

#endif

	if ( glConfig.anisotropicAvailable )
	{
		float maxAnisotropy;

		glGetFloatv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy );
		glConfig.maxAnisotropy = maxAnisotropy;

		// set when rendering
//     glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, glConfig.maxAnisotropy);
	}

//----(SA)  end
}

/*
================
GfxInfo_f
================
*/
void GfxInfo_f( void )
{
	cvar_t     *sys_cpustring = ri.Cvar_Get( "sys_cpustring", "", 0 );
	static const char enablestrings[][16] =
	{
		"disabled",
		"enabled"
	};
	static const char fsstrings[][16] =
	{
		"windowed",
		"fullscreen"
	};

	ri.Printf( PRINT_ALL, "\nGL_VENDOR: %s\n", glConfig.vendor_string );
	ri.Printf( PRINT_ALL, "GL_RENDERER: %s\n", glConfig.renderer_string );
	ri.Printf( PRINT_ALL, "GL_VERSION: %s\n", glConfig.version_string );
	ri.Printf( PRINT_ALL, "GL_EXTENSIONS: %s\n", glConfig.extensions_string );
	ri.Printf( PRINT_ALL, "GL_MAX_TEXTURE_SIZE: %d\n", glConfig.maxTextureSize );
	ri.Printf( PRINT_ALL, "GL_MAX_ACTIVE_TEXTURES_ARB: %d\n", glConfig.maxActiveTextures );
	ri.Printf( PRINT_ALL, "\nPIXELFORMAT: color(%d-bits) Z(%d-bit) stencil(%d-bits)\n", glConfig.colorBits, glConfig.depthBits,
	           glConfig.stencilBits );
	ri.Printf( PRINT_ALL, "MODE: %d, %d x %d %s hz:", r_mode->integer, glConfig.vidWidth, glConfig.vidHeight,
	           fsstrings[ r_fullscreen->integer == 1 ] );

	if ( glConfig.displayFrequency )
	{
		ri.Printf( PRINT_ALL, "%d\n", glConfig.displayFrequency );
	}
	else
	{
		ri.Printf( PRINT_ALL, "N/A\n" );
	}

	if ( glConfig.deviceSupportsGamma )
	{
		ri.Printf( PRINT_ALL, "GAMMA: hardware w/ %d overbright bits\n", tr.overbrightBits );
	}
	else
	{
		ri.Printf( PRINT_ALL, "GAMMA: software w/ %d overbright bits\n", tr.overbrightBits );
	}

	ri.Printf( PRINT_ALL, "CPU: %s\n", sys_cpustring->string );

	// rendering primitives
	{
		int primitives;

		// default is to use triangles if compiled vertex arrays are present
		ri.Printf( PRINT_ALL, "rendering primitives: " );
		primitives = r_primitives->integer;

		if ( primitives == 0 )
		{
			if ( GL_EXT_compiled_vertex_array )
			{
				primitives = 2;
			}
			else
			{
				primitives = 1;
			}
		}

		if ( primitives == -1 )
		{
			ri.Printf( PRINT_ALL, "none\n" );
		}
		else if ( primitives == 2 )
		{
			ri.Printf( PRINT_ALL, "single glDrawElements\n" );
		}
		else if ( primitives == 1 )
		{
			ri.Printf( PRINT_ALL, "multiple glArrayElement\n" );
		}
		else if ( primitives == 3 )
		{
			ri.Printf( PRINT_ALL, "multiple glColor4ubv + glTexCoord2fv + glVertex3fv\n" );
		}
	}

	ri.Printf( PRINT_ALL, "texturemode: %s\n", r_textureMode->string );
	ri.Printf( PRINT_ALL, "picmip: %d\n", r_picmip->integer );
	ri.Printf( PRINT_ALL, "texture bits: %d\n", r_texturebits->integer );
	ri.Printf( PRINT_ALL, "multitexture: %s\n", enablestrings[ GLEW_ARB_multitexture ] );
	ri.Printf( PRINT_ALL, "compiled vertex arrays: %s\n", enablestrings[ GL_EXT_compiled_vertex_array ] );
	ri.Printf( PRINT_ALL, "texenv add: %s\n", enablestrings[ glConfig.textureEnvAddAvailable != 0 ] );
	ri.Printf( PRINT_ALL, "compressed textures: %s\n", enablestrings[ glConfig.textureCompression != TC_NONE ] );
	ri.Printf( PRINT_ALL, "anisotropy: %s\n", r_textureAnisotropy->string );

	ri.Printf( PRINT_ALL, "NV distance fog: %s\n", enablestrings[ glConfig.NVFogAvailable != 0 ] );

	if ( glConfig.NVFogAvailable )
	{
		ri.Printf( PRINT_ALL, "Fog Mode: %s\n", r_nv_fogdist_mode->string );
	}

	if ( glConfig.hardwareType == GLHW_PERMEDIA2 )
	{
		ri.Printf( PRINT_ALL, "HACK: using vertex lightmap approximation\n" );
	}

	if ( glConfig.hardwareType == GLHW_RAGEPRO )
	{
		ri.Printf( PRINT_ALL, "HACK: ragePro approximations\n" );
	}

	if ( glConfig.hardwareType == GLHW_RIVA128 )
	{
		ri.Printf( PRINT_ALL, "HACK: riva128 approximations\n" );
	}

	if ( glConfig.smpActive )
	{
		ri.Printf( PRINT_ALL, "Using dual processor acceleration\n" );
	}

	if ( r_finish->integer )
	{
		ri.Printf( PRINT_ALL, "Forcing glFinish\n" );
	}
}

/*
===============
R_Register
===============
*/
void R_Register( void )
{
	//
	// latched and archived variables
	//
	r_glDriver = ri.Cvar_Get( "r_glDriver", OPENGL_DRIVER_NAME, CVAR_ARCHIVE | CVAR_LATCH | CVAR_UNSAFE );
	r_allowExtensions = ri.Cvar_Get( "r_allowExtensions", "1", CVAR_ARCHIVE | CVAR_LATCH | CVAR_UNSAFE );
	r_ext_compressed_textures = ri.Cvar_Get( "r_ext_compressed_textures", "1", CVAR_ARCHIVE | CVAR_LATCH | CVAR_UNSAFE );  // (SA) ew, a spelling change I missed from the missionpack
	r_ext_gamma_control = ri.Cvar_Get( "r_ext_gamma_control", "1", CVAR_ARCHIVE | CVAR_LATCH | CVAR_UNSAFE );
	r_ext_multitexture = ri.Cvar_Get( "r_ext_multitexture", "1", CVAR_ARCHIVE | CVAR_LATCH | CVAR_UNSAFE );
	r_ext_compiled_vertex_array = ri.Cvar_Get( "r_ext_compiled_vertex_array", "1", CVAR_ARCHIVE | CVAR_LATCH | CVAR_UNSAFE );
	r_glIgnoreWicked3D = ri.Cvar_Get( "r_glIgnoreWicked3D", "0", CVAR_ARCHIVE | CVAR_LATCH | CVAR_UNSAFE );

//----(SA)  added
	r_ext_ATI_pntriangles = ri.Cvar_Get( "r_ext_ATI_pntriangles", "0", CVAR_ARCHIVE | CVAR_LATCH | CVAR_UNSAFE );  //----(SA)  default to '0'
	r_ati_truform_tess = ri.Cvar_Get( "r_ati_truform_tess", "1", CVAR_ARCHIVE | CVAR_UNSAFE );
	r_ati_truform_normalmode =
	  ri.Cvar_Get( "r_ati_truform_normalmode", "GL_PN_TRIANGLES_NORMAL_MODE_LINEAR", CVAR_ARCHIVE | CVAR_UNSAFE );
	r_ati_truform_pointmode =
	  ri.Cvar_Get( "r_ati_truform_pointmode", "GL_PN_TRIANGLES_POINT_MODE_LINEAR", CVAR_ARCHIVE | CVAR_UNSAFE );

	r_ati_fsaa_samples = ri.Cvar_Get( "r_ati_fsaa_samples", "1", CVAR_ARCHIVE | CVAR_UNSAFE );  //DAJ valids are 1, 2, 4

	r_ext_texture_filter_anisotropic =
	  ri.Cvar_Get( "r_ext_texture_filter_anisotropic", "0", CVAR_ARCHIVE | CVAR_LATCH | CVAR_UNSAFE );

	r_ext_NV_fog_dist = ri.Cvar_Get( "r_ext_NV_fog_dist", "0", CVAR_ARCHIVE | CVAR_LATCH | CVAR_UNSAFE );
	r_nv_fogdist_mode = ri.Cvar_Get( "r_nv_fogdist_mode", "GL_EYE_RADIAL_NV", CVAR_ARCHIVE | CVAR_UNSAFE );  // default to 'looking good'
//----(SA)  end

#ifdef __linux__ // broken on linux
	r_ext_texture_env_add = ri.Cvar_Get( "r_ext_texture_env_add", "0", CVAR_ARCHIVE | CVAR_LATCH | CVAR_UNSAFE );
#else
	r_ext_texture_env_add = ri.Cvar_Get( "r_ext_texture_env_add", "1", CVAR_ARCHIVE | CVAR_LATCH | CVAR_UNSAFE );
#endif

	r_clampToEdge = ri.Cvar_Get( "r_clampToEdge", "1", CVAR_ARCHIVE | CVAR_LATCH | CVAR_UNSAFE );  // ydnar: opengl 1.2 GL_CLAMP_TO_EDGE support

	r_picmip = ri.Cvar_Get( "r_picmip", "1", CVAR_ARCHIVE | CVAR_LATCH );  //----(SA)    mod for DM and DK for id build.  was "1" // JPW NERVE pushed back to 1
	r_roundImagesDown = ri.Cvar_Get( "r_roundImagesDown", "1", CVAR_ARCHIVE | CVAR_LATCH );
	r_rmse = ri.Cvar_Get( "r_rmse", "0.0", CVAR_ARCHIVE | CVAR_LATCH );
	r_colorMipLevels = ri.Cvar_Get( "r_colorMipLevels", "0", CVAR_LATCH );
	AssertCvarRange( r_picmip, 0, 3, qtrue );
	r_detailTextures = ri.Cvar_Get( "r_detailtextures", "1", CVAR_ARCHIVE | CVAR_LATCH );
	r_texturebits = ri.Cvar_Get( "r_texturebits", "0", CVAR_ARCHIVE | CVAR_LATCH | CVAR_UNSAFE );
	r_colorbits = ri.Cvar_Get( "r_colorbits", "0", CVAR_ARCHIVE | CVAR_LATCH | CVAR_UNSAFE );
	r_stereo = ri.Cvar_Get( "r_stereo", "0", CVAR_ARCHIVE | CVAR_LATCH | CVAR_UNSAFE );
#ifdef __linux__
	r_stencilbits = ri.Cvar_Get( "r_stencilbits", "0", CVAR_ARCHIVE | CVAR_LATCH | CVAR_UNSAFE );
#else
	r_stencilbits = ri.Cvar_Get( "r_stencilbits", "0", CVAR_ARCHIVE | CVAR_LATCH | CVAR_UNSAFE );
#endif
	r_depthbits = ri.Cvar_Get( "r_depthbits", "0", CVAR_ARCHIVE | CVAR_LATCH | CVAR_UNSAFE );
	r_ext_multisample = ri.Cvar_Get( "r_ext_multisample", "0", CVAR_ARCHIVE | CVAR_LATCH | CVAR_UNSAFE );
	r_overBrightBits = ri.Cvar_Get( "r_overBrightBits", "0", CVAR_ARCHIVE | CVAR_LATCH );  // Arnout: disable overbrightbits by default
	AssertCvarRange( r_overBrightBits, 0, 1, qtrue );  // ydnar: limit to overbrightbits 1 (sorry 1337 players)
	r_ignorehwgamma = ri.Cvar_Get( "r_ignorehwgamma", "0", CVAR_ARCHIVE | CVAR_LATCH );  // ydnar: use hw gamma by default
	r_mode = ri.Cvar_Get( "r_mode", "6", CVAR_ARCHIVE | CVAR_LATCH | CVAR_UNSAFE );
	r_oldMode = ri.Cvar_Get( "r_oldMode", "", CVAR_ARCHIVE );  // ydnar: previous "good" video mode
	r_fullscreen = ri.Cvar_Get( "r_fullscreen", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_customwidth = ri.Cvar_Get( "r_customwidth", "1600", CVAR_ARCHIVE | CVAR_LATCH );
	r_customheight = ri.Cvar_Get( "r_customheight", "1024", CVAR_ARCHIVE | CVAR_LATCH );
	r_customaspect = ri.Cvar_Get( "r_customaspect", "1", CVAR_ARCHIVE | CVAR_LATCH );
	r_simpleMipMaps = ri.Cvar_Get( "r_simpleMipMaps", "1", CVAR_ARCHIVE | CVAR_LATCH );
	r_uiFullScreen = ri.Cvar_Get( "r_uifullscreen", "0", 0 );
	r_subdivisions = ri.Cvar_Get( "r_subdivisions", "4", CVAR_ARCHIVE | CVAR_LATCH );
#ifdef MACOS_X
	// debe: r_smp doesn't work on MACOS_X yet...
	r_smp = ri.Cvar_Get( "r_smp", "0", CVAR_ARCHIVE | CVAR_LATCH | CVAR_UNSAFE | CVAR_ROM );
	ri.Cvar_Set( "r_smp", "0" );
#elif defined WIN32
	// ydnar: r_smp is nonfunctional on windows
	r_smp = ri.Cvar_Get( "r_smp", "0", CVAR_ARCHIVE | CVAR_LATCH | CVAR_UNSAFE | CVAR_ROM );
	ri.Cvar_Set( "r_smp", "0" );
#else
	r_smp = ri.Cvar_Get( "r_smp", "0", CVAR_ARCHIVE | CVAR_LATCH | CVAR_UNSAFE );
#endif
	r_ignoreFastPath = ri.Cvar_Get( "r_ignoreFastPath", "0", CVAR_ARCHIVE | CVAR_LATCH );  // ydnar: use fast path by default
#if MAC_STVEF_HM || MAC_WOLF2_MP
	r_ati_fsaa_samples = ri.Cvar_Get( "r_ati_fsaa_samples", "1", CVAR_ARCHIVE );  //DAJ valids are 1, 2, 4
#endif
	//
	// temporary latched variables that can only change over a restart
	//
	r_displayRefresh = ri.Cvar_Get( "r_displayRefresh", "0", CVAR_LATCH | CVAR_UNSAFE );
	AssertCvarRange( r_displayRefresh, 0, 200, qtrue );
	r_mapOverBrightBits = ri.Cvar_Get( "r_mapOverBrightBits", "2", CVAR_LATCH );
	AssertCvarRange( r_mapOverBrightBits, 0, 3, qtrue );
	r_intensity = ri.Cvar_Get( "r_intensity", "1", CVAR_LATCH );
	AssertCvarRange( r_intensity, 0, 1.5, qfalse );
	r_singleShader = ri.Cvar_Get( "r_singleShader", "0", CVAR_CHEAT | CVAR_LATCH );

	//
	// archived variables that can change at any time
	//
	r_lodCurveError = ri.Cvar_Get( "r_lodCurveError", "250", CVAR_ARCHIVE );
	r_lodbias = ri.Cvar_Get( "r_lodbias", "0", CVAR_ARCHIVE );
	r_flares = ri.Cvar_Get( "r_flares", "1", CVAR_ARCHIVE );
	r_znear = ri.Cvar_Get( "r_znear", "3", CVAR_CHEAT );  // ydnar: changed it to 3 (from 4) because of lean/fov cheats
	AssertCvarRange( r_znear, 0.001f, 200, qtrue );
//----(SA)  added
	r_zfar = ri.Cvar_Get( "r_zfar", "0", CVAR_CHEAT );
//----(SA)  end
	r_ignoreGLErrors = ri.Cvar_Get( "r_ignoreGLErrors", "1", CVAR_ARCHIVE );
	r_fastsky = ri.Cvar_Get( "r_fastsky", "0", CVAR_ARCHIVE );
	r_inGameVideo = ri.Cvar_Get( "r_inGameVideo", "1", CVAR_ARCHIVE );
	r_drawSun = ri.Cvar_Get( "r_drawSun", "1", CVAR_ARCHIVE );
	r_dynamiclight = ri.Cvar_Get( "r_dynamiclight", "1", CVAR_ARCHIVE );
	r_dlightBacks = ri.Cvar_Get( "r_dlightBacks", "1", CVAR_ARCHIVE );
	r_finish = ri.Cvar_Get( "r_finish", "0", CVAR_ARCHIVE );
	r_textureMode = ri.Cvar_Get( "r_textureMode", "GL_LINEAR_MIPMAP_NEAREST", CVAR_ARCHIVE );
	r_textureAnisotropy = ri.Cvar_Get( "r_textureAnisotropy", "1.0", CVAR_ARCHIVE );
	r_swapInterval = ri.Cvar_Get( "r_swapInterval", "0", CVAR_ARCHIVE );
#ifdef __MACOS__
	r_gamma = ri.Cvar_Get( "r_gamma", "1.2", CVAR_ARCHIVE );
#else
	r_gamma = ri.Cvar_Get( "r_gamma", "1.3", CVAR_ARCHIVE );
#endif
	r_facePlaneCull = ri.Cvar_Get( "r_facePlaneCull", "1", CVAR_ARCHIVE );

	r_railWidth = ri.Cvar_Get( "r_railWidth", "16", CVAR_ARCHIVE );
	r_railCoreWidth = ri.Cvar_Get( "r_railCoreWidth", "1", CVAR_ARCHIVE );
	r_railSegmentLength = ri.Cvar_Get( "r_railSegmentLength", "32", CVAR_ARCHIVE );

	r_primitives = ri.Cvar_Get( "r_primitives", "0", CVAR_ARCHIVE );

	r_ambientScale = ri.Cvar_Get( "r_ambientScale", "0.5", CVAR_CHEAT );
	r_directedScale = ri.Cvar_Get( "r_directedScale", "1", CVAR_CHEAT );

	//
	// temporary variables that can change at any time
	//
	r_showImages = ri.Cvar_Get( "r_showImages", "0", CVAR_TEMP );

	r_debugLight = ri.Cvar_Get( "r_debuglight", "0", CVAR_TEMP );
	r_debugSort = ri.Cvar_Get( "r_debugSort", "0", CVAR_CHEAT );
	r_printShaders = ri.Cvar_Get( "r_printShaders", "0", 0 );
	r_saveFontData = ri.Cvar_Get( "r_saveFontData", "0", 0 );

	r_fontScale = ri.Cvar_Get( "r_fontScale", "36", CVAR_ARCHIVE | CVAR_LATCH);

	// Ridah
	// TTimo show_bug.cgi?id=440
	//   with r_cache enabled, non-win32 OSes were leaking 24Mb per R_Init..
	r_cache = ri.Cvar_Get( "r_cache", "1", CVAR_LATCH );  // leaving it as this for backwards compability. but it caches models and shaders also
// (SA) disabling cacheshaders
//  ri.Cvar_Set( "r_cacheShaders", "0");
	// Gordon: enabling again..
	r_cacheShaders = ri.Cvar_Get( "r_cacheShaders", "1", CVAR_LATCH );
//----(SA)  end

	r_cacheModels = ri.Cvar_Get( "r_cacheModels", "1", CVAR_LATCH );
	r_cacheGathering = ri.Cvar_Get( "cl_cacheGathering", "0", 0 );
	r_buildScript = ri.Cvar_Get( "com_buildscript", "0", 0 );
	r_bonesDebug = ri.Cvar_Get( "r_bonesDebug", "0", CVAR_CHEAT );
	// done.

	// Rafael - wolf fog
	r_wolffog = ri.Cvar_Get( "r_wolffog", "1", CVAR_CHEAT );  // JPW NERVE cheat protected per id request
	// done

	r_nocurves = ri.Cvar_Get( "r_nocurves", "0", CVAR_CHEAT );
	r_drawworld = ri.Cvar_Get( "r_drawworld", "1", CVAR_CHEAT );
	r_drawfoliage = ri.Cvar_Get( "r_drawfoliage", "1", CVAR_CHEAT );  // ydnar
	r_lightmap = ri.Cvar_Get( "r_lightmap", "0", CVAR_CHEAT );  // DHM - NERVE :: cheat protect
	r_portalOnly = ri.Cvar_Get( "r_portalOnly", "0", CVAR_CHEAT );

	r_flareSize = ri.Cvar_Get( "r_flareSize", "40", CVAR_CHEAT );
	ri.Cvar_Set( "r_flareFade", "5" );  // to force this when people already have "7" in their config
	r_flareFade = ri.Cvar_Get( "r_flareFade", "5", CVAR_CHEAT );

	r_showSmp = ri.Cvar_Get( "r_showSmp", "0", CVAR_CHEAT );
	r_skipBackEnd = ri.Cvar_Get( "r_skipBackEnd", "0", CVAR_CHEAT );

	r_measureOverdraw = ri.Cvar_Get( "r_measureOverdraw", "0", CVAR_CHEAT );
	r_lodscale = ri.Cvar_Get( "r_lodscale", "5", CVAR_CHEAT );
	r_norefresh = ri.Cvar_Get( "r_norefresh", "0", CVAR_CHEAT );
	r_drawentities = ri.Cvar_Get( "r_drawentities", "1", CVAR_CHEAT );
	r_ignore = ri.Cvar_Get( "r_ignore", "1", CVAR_CHEAT );
	r_nocull = ri.Cvar_Get( "r_nocull", "0", CVAR_CHEAT );
	r_novis = ri.Cvar_Get( "r_novis", "0", CVAR_CHEAT );
	r_showcluster = ri.Cvar_Get( "r_showcluster", "0", CVAR_CHEAT );
	r_speeds = ri.Cvar_Get( "r_speeds", "0", CVAR_CHEAT );
	r_verbose = ri.Cvar_Get( "r_verbose", "0", CVAR_CHEAT );
	r_logFile = ri.Cvar_Get( "r_logFile", "0", CVAR_CHEAT );
	r_debugSurface = ri.Cvar_Get( "r_debugSurface", "0", CVAR_CHEAT );
	r_nobind = ri.Cvar_Get( "r_nobind", "0", CVAR_CHEAT );
	r_showtris = ri.Cvar_Get( "r_showtris", "0", CVAR_CHEAT );
	r_trisColor = ri.Cvar_Get( "r_trisColor", "1.0 1.0 1.0 1.0", CVAR_ARCHIVE );
	r_showsky = ri.Cvar_Get( "r_showsky", "0", CVAR_CHEAT );
	r_shownormals = ri.Cvar_Get( "r_shownormals", "0", CVAR_CHEAT );
	r_normallength = ri.Cvar_Get( "r_normallength", "0.5", CVAR_ARCHIVE );
	r_showmodelbounds = ri.Cvar_Get( "r_showmodelbounds", "0", CVAR_CHEAT );
	r_clear = ri.Cvar_Get( "r_clear", "0", CVAR_CHEAT );
	r_offsetFactor = ri.Cvar_Get( "r_offsetfactor", "-1", CVAR_CHEAT );
	r_offsetUnits = ri.Cvar_Get( "r_offsetunits", "-2", CVAR_CHEAT );
	r_drawBuffer = ri.Cvar_Get( "r_drawBuffer", "GL_BACK", CVAR_CHEAT );
	r_lockpvs = ri.Cvar_Get( "r_lockpvs", "0", CVAR_CHEAT );
	r_noportals = ri.Cvar_Get( "r_noportals", "0", CVAR_CHEAT );
	r_shadows = ri.Cvar_Get( "cg_shadows", "1", 0 );

	r_portalsky = ri.Cvar_Get( "cg_skybox", "1", 0 );
	r_maxpolys = ri.Cvar_Get( "r_maxpolys", va( "%d", MAX_POLYS ), 0 );
	r_maxpolyverts = ri.Cvar_Get( "r_maxpolyverts", va( "%d", MAX_POLYVERTS ), 0 );

	r_highQualityVideo = ri.Cvar_Get( "r_highQualityVideo", "1", CVAR_ARCHIVE );
	// make sure all the commands added here are also
	// removed in R_Shutdown
	ri.Cmd_AddCommand( "imagelist", R_ImageList_f );
	ri.Cmd_AddCommand( "shaderlist", R_ShaderList_f );
	ri.Cmd_AddCommand( "skinlist", R_SkinList_f );
	ri.Cmd_AddCommand( "modellist", R_Modellist_f );
	ri.Cmd_AddCommand( "modelist", R_ModeList_f );
	ri.Cmd_AddCommand( "screenshot", R_ScreenShot_f );
	ri.Cmd_AddCommand( "screenshotJPEG", R_ScreenShot_f );
	ri.Cmd_AddCommand( "screenshotPNG", R_ScreenShot_f );
	ri.Cmd_AddCommand( "gfxinfo", GfxInfo_f );
	ri.Cmd_AddCommand( "taginfo", R_TagInfo_f );
}

/*
===============
R_Init
===============
*/
void R_Init( void )
{
	int i;

	ri.Printf( PRINT_ALL, "----- R_Init -----\n" );

	// clear all our internal state
	memset( &tr, 0, sizeof( tr ) );
	memset( &backEnd, 0, sizeof( backEnd ) );
	memset( &tess, 0, sizeof( tess ) );

	tess.xyz = tess_xyz;
	tess.texCoords0 = tess_texCoords0;
	tess.texCoords1 = tess_texCoords1;
	tess.indexes = tess_indexes;
	tess.normal = tess_normal;
	tess.vertexColors = tess_vertexColors;

	tess.maxShaderVerts = SHADER_MAX_VERTEXES;
	tess.maxShaderIndicies = SHADER_MAX_INDEXES;

	if ( ( intptr_t ) tess.xyz & 15 )
	{
		Com_Printf( "WARNING: tess.xyz not 16 byte aligned\n" );
	}

	memset( tess.constantColor255, 255, sizeof( tess.constantColor255 ) );

	//
	// init function tables
	//
	for ( i = 0; i < FUNCTABLE_SIZE; i++ )
	{
		tr.sinTable[ i ] = sin( DEG2RAD( i * 360.0f / ( ( float )( FUNCTABLE_SIZE - 1 ) ) ) );
		tr.squareTable[ i ] = ( i < FUNCTABLE_SIZE / 2 ) ? 1.0f : -1.0f;
		tr.sawToothTable[ i ] = ( float ) i / FUNCTABLE_SIZE;
		tr.inverseSawToothTable[ i ] = 1.0f - tr.sawToothTable[ i ];

		if ( i < FUNCTABLE_SIZE / 2 )
		{
			if ( i < FUNCTABLE_SIZE / 4 )
			{
				tr.triangleTable[ i ] = ( float ) i / ( FUNCTABLE_SIZE / 4 );
			}
			else
			{
				tr.triangleTable[ i ] = 1.0f - tr.triangleTable[ i - FUNCTABLE_SIZE / 4 ];
			}
		}
		else
		{
			tr.triangleTable[ i ] = -tr.triangleTable[ i - FUNCTABLE_SIZE / 2 ];
		}
	}

	// Ridah, init the virtual memory
	R_Hunk_Begin();

	R_InitFogTable();

	R_NoiseInit();

	R_Register();

	max_polys = r_maxpolys->integer;

	if ( max_polys < MAX_POLYS )
	{
		max_polys = MAX_POLYS;
	}

	max_polyverts = r_maxpolyverts->integer;

	if ( max_polyverts < MAX_POLYVERTS )
	{
		max_polyverts = MAX_POLYVERTS;
	}

//  backEndData[0] = ri.Hunk_Alloc( sizeof( *backEndData[0] ), h_low );
	backEndData[ 0 ] =
	  ri.Hunk_Alloc( sizeof( *backEndData[ 0 ] ) + sizeof( srfPoly_t ) * max_polys + sizeof( polyVert_t ) * max_polyverts, h_low );

	if ( r_smp->integer )
	{
//      backEndData[1] = ri.Hunk_Alloc( sizeof( *backEndData[1] ), h_low );
		backEndData[ 1 ] =
		  ri.Hunk_Alloc( sizeof( *backEndData[ 1 ] ) + sizeof( srfPoly_t ) * max_polys + sizeof( polyVert_t ) * max_polyverts, h_low );
	}
	else
	{
		backEndData[ 1 ] = NULL;
	}

	R_ToggleSmpFrame();

	InitOpenGL();

	R_InitImages();

	R_InitShaders();

	R_InitSkins();

	R_ModelInit();

#if defined( USE_REFENTITY_ANIMATIONSYSTEM )
	R_InitAnimations();
#endif

	R_InitFreeType();

	GL_CheckErrors();

	ri.Printf( PRINT_ALL, "----- finished R_Init -----\n" );
}

void R_PurgeCache( void )
{
	R_PurgeShaders( 9999999 );
	R_PurgeBackupImages( 9999999 );
	R_PurgeModels( 9999999 );
}

/*
===============
RE_Shutdown
===============
*/
void RE_Shutdown( qboolean destroyWindow )
{
	ri.Printf( PRINT_ALL, "RE_Shutdown( %i )\n", destroyWindow );

	ri.Cmd_RemoveCommand( "modellist" );
	ri.Cmd_RemoveCommand( "screenshotPNG" );
	ri.Cmd_RemoveCommand( "screenshotJPEG" );
	ri.Cmd_RemoveCommand( "screenshot" );
	ri.Cmd_RemoveCommand( "imagelist" );
	ri.Cmd_RemoveCommand( "shaderlist" );
	ri.Cmd_RemoveCommand( "skinlist" );
	ri.Cmd_RemoveCommand( "gfxinfo" );
	ri.Cmd_RemoveCommand( "modelist" );
	ri.Cmd_RemoveCommand( "shaderstate" );
	ri.Cmd_RemoveCommand( "taginfo" );

	// Ridah
	ri.Cmd_RemoveCommand( "cropimages" );
	// done.

	R_ShutdownCommandBuffers();

	// Ridah, keep a backup of the current images if possible
	// clean out any remaining unused media from the last backup
	R_PurgeCache();

	if ( r_cache->integer )
	{
		if ( tr.registered )
		{
			if ( destroyWindow )
			{
				R_SyncRenderThread();
				R_ShutdownCommandBuffers();
				R_DeleteTextures();
			}
			else
			{
				// backup the current media
				R_ShutdownCommandBuffers();

				R_BackupModels();
				R_BackupShaders();
				R_BackupImages();
			}
		}
	}
	else if ( tr.registered )
	{
		R_SyncRenderThread();
		R_ShutdownCommandBuffers();
		R_DeleteTextures();
	}

	R_DoneFreeType();

	// shut down platform specific OpenGL stuff
	if ( destroyWindow )
	{
		GLimp_Shutdown();

		// Ridah, release the virtual memory
		R_Hunk_End();
		R_FreeImageBuffer();
		ri.Tag_Free(); // wipe all render alloc'd zone memory
	}

	tr.registered = qfalse;
}

/*
=============
RE_EndRegistration

Touch all images to make sure they are resident
=============
*/
void RE_EndRegistration( void )
{
	R_SyncRenderThread();

	/*
	RB: disabled unneeded reference to Sys_LowPhysicalMemory
	if(!Sys_LowPhysicalMemory())
	{
	//      RB_ShowImages();
	}
	*/
}

void R_DebugPolygon( int color, int numPoints, float *points );

/*
=====================
GetRefAPI
=====================
*/
#if defined( __cplusplus )
extern "C" {
#endif
	refexport_t    *GetRefAPI( int apiVersion, refimport_t *rimp )
	{
		static refexport_t re;

		ri = *rimp;

		memset( &re, 0, sizeof( re ) );

		if ( apiVersion != REF_API_VERSION )
		{
			ri.Printf( PRINT_ALL, "Mismatched REF_API_VERSION: expected %i, got %i\n", REF_API_VERSION, apiVersion );
			return NULL;
		}

		// the RE_ functions are Renderer Entry points

		re.Shutdown = RE_Shutdown;

		re.BeginRegistration = RE_BeginRegistration;
		re.RegisterModel = RE_RegisterModel;
		re.RegisterSkin = RE_RegisterSkin;
		re.RegisterShader = RE_RegisterShader;
		re.RegisterShaderNoMip = RE_RegisterShaderNoMip;
#if defined( USE_REFLIGHT )
		re.RegisterShaderLightAttenuation = NULL;
#endif
		re.RegisterFont = RE_RegisterFont;
		re.Glyph = RE_Glyph;
		re.GlyphChar = RE_GlyphChar;
		re.UnregisterFont = RE_UnregisterFont;
		re.RegisterFontVM = RE_RegisterFontVM;
		re.GlyphVM = RE_GlyphVM;
		re.GlyphCharVM = RE_GlyphCharVM;
		re.UnregisterFontVM = RE_UnregisterFontVM;
		re.LoadWorld = RE_LoadWorldMap;
//----(SA) added
		re.GetSkinModel = RE_GetSkinModel;
		re.GetShaderFromModel = RE_GetShaderFromModel;
//----(SA) end
		re.SetWorldVisData = RE_SetWorldVisData;
		re.EndRegistration = RE_EndRegistration;

		re.ClearScene = RE_ClearScene;
		re.AddRefEntityToScene = RE_AddRefEntityToScene;
		re.LightForPoint = R_LightForPoint;

		re.AddPolyToScene = RE_AddPolyToScene;
		// Ridah
		re.AddPolysToScene = RE_AddPolysToScene;
		// done.
		re.AddLightToScene = RE_AddLightToScene;
		re.AddAdditiveLightToScene = NULL;
//----(SA)
		re.AddCoronaToScene = RE_AddCoronaToScene;
		re.SetFog = R_SetFog;
//----(SA)

		re.RenderScene = RE_RenderScene;
		re.SaveViewParms = RE_SaveViewParms;
		re.RestoreViewParms = RE_RestoreViewParms;

		re.SetColor = RE_SetColor;
		re.SetClipRegion = RE_SetClipRegion;
		re.DrawStretchPic = RE_StretchPic;
		re.DrawRotatedPic = RE_RotatedPic; // NERVE - SMF
		re.DrawStretchPicGradient = RE_StretchPicGradient;
		re.Add2dPolys = RE_2DPolyies;
		re.DrawStretchRaw = RE_StretchRaw;
		re.UploadCinematic = RE_UploadCinematic;
		re.BeginFrame = RE_BeginFrame;
		re.EndFrame = RE_EndFrame;

		re.MarkFragments = R_MarkFragments;
		re.ProjectDecal = RE_ProjectDecal;
		re.ClearDecals = RE_ClearDecals;

		re.LerpTag = R_LerpTag;
		re.ModelBounds = R_ModelBounds;

		re.RemapShader = R_RemapShader;
		re.DrawDebugPolygon = R_DebugPolygon;
		re.DrawDebugText = R_DebugText;
		re.GetEntityToken = R_GetEntityToken;

		re.AddPolyBufferToScene = RE_AddPolyBufferToScene;

		re.SetGlobalFog = RE_SetGlobalFog;

		re.inPVS = R_inPVS;

		re.purgeCache = R_PurgeCache;

		//bani
		re.LoadDynamicShader = RE_LoadDynamicShader;
		// fretn
		re.RenderToTexture = RE_RenderToTexture;
		re.GetTextureId = R_GetTextureId;
		//bani
		re.Finish = RE_Finish;

		re.TakeVideoFrame = RE_TakeVideoFrame;
#if defined( USE_REFLIGHT )
		re.AddRefLightToScene = NULL;
#endif

		// RB: alternative skeletal animation system
#if defined( USE_REFENTITY_ANIMATIONSYSTEM )
		re.RegisterAnimation = RE_RegisterAnimation;
		re.CheckSkeleton = RE_CheckSkeleton;
		re.BuildSkeleton = RE_BuildSkeleton;
		re.BlendSkeleton = RE_BlendSkeleton;
		re.BoneIndex = RE_BoneIndex;
		re.AnimNumFrames = RE_AnimNumFrames;
		re.AnimFrameRate = RE_AnimFrameRate;
#endif

		return &re;
	}

#if defined( __cplusplus )
} // extern "C"
#endif

#ifndef REF_HARD_LINKED
// this is only here so the functions in q_shared.c and bg_*.c can link

void QDECL Com_Printf( const char *msg, ... )
{
	va_list argptr;
	char    text[ 1024 ];

	va_start( argptr, msg );
	Q_vsnprintf( text, sizeof( text ), msg, argptr );
	va_end( argptr );

	ri.Printf( PRINT_ALL, "%s", text );
}

void QDECL Com_DPrintf( const char *msg, ... )
{
	va_list argptr;
	char    text[ 1024 ];

	va_start( argptr, msg );
	Q_vsnprintf( text, sizeof( text ), msg, argptr );
	va_end( argptr );

	ri.Printf( PRINT_DEVELOPER, "%s", text );
}

void QDECL Com_Error( int level, const char *error, ... )
{
	va_list argptr;
	char    text[ 1024 ];

	va_start( argptr, error );
	Q_vsnprintf( text, sizeof( text ), error, argptr );
	va_end( argptr );

	ri.Error( level, "%s", text );
}

#endif
