/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
Copyright (C) 2006-2010 Robert Beckebans

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

#ifndef __TR_PUBLIC_H
#define __TR_PUBLIC_H

#include "tr_types.h"
#include "engine/botlib/bot_debug.h"

#define REF_API_VERSION 10

// *INDENT-OFF*

//
// these are the functions exported by the refresh module
//
typedef struct
{
	// called before the library is unloaded
	// if the system is just reconfiguring, pass destroyWindow = false,
	// which will keep the screen from flashing to the desktop.
	void ( *Shutdown )( bool destroyWindow );

	// All data that will be used in a level should be
	// registered before rendering any frames to prevent disk hits,
	// but they can still be registered at a later time
	// if necessary.
	//
	// BeginRegistration makes any existing media pointers invalid
	// and returns the current gl configuration, including screen width
	// and height, which can be used by the client to intelligently
	// size display elements. Returns false if the renderer couldn't
	// be initialized.
	bool( *BeginRegistration )( glconfig_t *config, glconfig2_t *glconfig2 );
	qhandle_t ( *RegisterModel )( const char *name );
	//qhandle_t   (*RegisterModelAllLODs) (const char *name);
	qhandle_t ( *RegisterSkin )( const char *name );
	qhandle_t ( *RegisterShader )( const char *name,
				       RegisterShaderFlags_t flags );
	void   ( *RegisterFont )( const char *fontName, const char *fallbackName, int pointSize, fontInfo_t **font );
	void   ( *UnregisterFont )( fontInfo_t *font );
	void   ( *RegisterFontVM )( const char *fontName, const char *fallbackName, int pointSize, fontMetrics_t * );
	void   ( *UnregisterFontVM )( fontHandle_t font );
	void   ( *Glyph )( fontInfo_t *font, const char *str, glyphInfo_t *glyph );
	void   ( *GlyphChar )( fontInfo_t *font, int ch, glyphInfo_t *glyph );
	void   ( *GlyphVM )( fontHandle_t font, const char *ch, glyphInfo_t *glyph );
	void   ( *GlyphCharVM )( fontHandle_t font, int ch, glyphInfo_t *glyph );

	void ( *LoadWorld )( const char *name );

	// the vis data is a large enough block of data that we go to the trouble
	// of sharing it with the clipmodel subsystem
	void ( *SetWorldVisData )( const byte *vis );

	// EndRegistration will draw a tiny polygon with each texture, forcing
	// them to be loaded into card memory
	void ( *EndRegistration )();

	// a scene is built up by calls to R_ClearScene and the various R_Add functions.
	// Nothing is drawn until R_RenderScene is called.
	void ( *ClearScene )();
	void ( *AddRefEntityToScene )( const refEntity_t *re );

	int ( *LightForPoint )( vec3_t point, vec3_t ambientLight, vec3_t directedLight, vec3_t lightDir );

	void ( *AddPolyToScene )( qhandle_t hShader, int numVerts, const polyVert_t *verts );
	void ( *AddPolysToScene )( qhandle_t hShader, int numVerts, const polyVert_t *verts, int numPolys );

	void ( *AddLightToScene )( const vec3_t org, float radius, float intensity, float r, float g, float b,
	                           qhandle_t hShader, int flags );

	void ( *AddAdditiveLightToScene )( const vec3_t org, float intensity, float r, float g, float b );

	void ( *RenderScene )( const refdef_t *fd );

	void ( *SetColor )( const Color::Color& rgba );             // nullptr = 1,1,1,1
	void ( *SetClipRegion )( const float *region );
	void ( *DrawStretchPic )( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader );             // 0 = white
	void ( *DrawRotatedPic )( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader, float angle );             // NERVE - SMF
	void ( *DrawStretchPicGradient )( float x, float y, float w, float h, float s1, float t1, float s2, float t2,
	                                  qhandle_t hShader, const Color::Color& gradientColor, int gradientType );
	void ( *Add2dPolys )( polyVert_t *polys, int numverts, qhandle_t hShader );

	// Draw images for cinematic rendering, pass as 32 bit rgba
	void ( *DrawStretchRaw )( int x, int y, int w, int h, int cols, int rows, const byte *data, int client,
	                          bool dirty );
	void ( *UploadCinematic )( int cols, int rows, const byte *data, int client, bool dirty );

	void ( *BeginFrame )();

	// if the pointers are not nullptr, timing info will be returned
	void ( *EndFrame )( int *frontEndMsec, int *backEndMsec );

	int ( *MarkFragments )( int numPoints, const vec3_t *points, const vec3_t projection,
	                        int maxPoints, vec3_t pointBuffer, int maxFragments, markFragment_t *fragmentBuffer );

	void ( *ProjectDecal )( qhandle_t hShader, int numPoints, vec3_t *points, vec4_t projection, const Color::Color& color,
	                        int lifeTime, int fadeTime );
	void ( *ClearDecals )();

	int ( *LerpTag )( orientation_t *tag, const refEntity_t *refent, const char *tagName, int startIndex );
	void ( *ModelBounds )( qhandle_t model, vec3_t mins, vec3_t maxs );

	void ( *RemapShader )( const char *oldShader, const char *newShader, const char *offsetTime );

	void ( *DrawDebugPolygon )( int color, int numpoints, float *points );
	void ( *DrawDebugText )( const vec3_t org, float r, float g, float b, const char *text, bool neverOcclude );

	bool( *GetEntityToken )( char *buffer, int size );

	void ( *AddPolyBufferToScene )( polyBuffer_t *pPolyBuffer );

	bool( *inPVS )( const vec3_t p1, const vec3_t p2 );
	bool( *inPVVS )( const vec3_t p1, const vec3_t p2 );

	bool( *LoadDynamicShader )( const char *shadername, const char *shadertext );

	void ( *Finish )();

	// XreaL BEGIN
	void ( *TakeVideoFrame )( int h, int w, byte *captureBuffer, byte *encodeBuffer, bool motionJpeg );

	void ( *AddRefLightToScene )( const refLight_t *light );

	// RB: alternative skeletal animation system
	qhandle_t ( *RegisterAnimation )( const char *name );
	int ( *CheckSkeleton )( refSkeleton_t *skel, qhandle_t model, qhandle_t anim );
	int ( *BuildSkeleton )( refSkeleton_t *skel, qhandle_t anim, int startFrame, int endFrame, float frac,
	                        bool clearOrigin );
	int ( *BlendSkeleton )( refSkeleton_t *skel, const refSkeleton_t *blend, float frac );
	int ( *BoneIndex )( qhandle_t hModel, const char *boneName );
	int ( *AnimNumFrames )( qhandle_t hAnim );
	int ( *AnimFrameRate )( qhandle_t hAnim );

	// XreaL END

	// VisTest API
	qhandle_t ( *RegisterVisTest ) ();
	void      ( *AddVisTestToScene ) ( qhandle_t hTest, const vec3_t pos,
					   float depthAdjust, float area );
	float     ( *CheckVisibility ) ( qhandle_t hTest );
	void      ( *UnregisterVisTest ) ( qhandle_t hTest );

	// color grading
	void      ( *SetColorGrading ) ( int slot, qhandle_t hShader );

	void ( *ScissorEnable ) ( bool enable );
	void ( *ScissorSet ) ( int x, int y, int w, int h );

	void ( *SetAltShaderTokens ) ( const char * );

	void ( *GetTextureSize )( int textureID, int *width, int *height );
	void ( *Add2dPolysIndexed )( polyVert_t *polys, int numverts, int *indexes, int numindexes, int trans_x, int trans_y, qhandle_t shader );
	qhandle_t ( *GenerateTexture )( const byte *pic, int width, int height );
	const char *( *ShaderNameFromHandle )( qhandle_t shader );
} refexport_t;

//
// these are the functions imported by the refresh module
//
typedef struct
{
	// print message on the local console
	void ( QDECL *Printf )( int printLevel, const char *fmt, ... ) PRINTF_LIKE(2);

	// abort the game
	void ( QDECL *Error )( int errorLevel, const char *fmt, ... ) PRINTF_LIKE(2) NORETURN_PTR;

	// milliseconds should only be used for profiling, never
	// for anything game related.  Get time from the refdef
	int ( *Milliseconds )();

	int ( *RealTime )( qtime_t *qtime );

	// stack based memory allocation for per-level things that
	// won't be freed
	void ( *Hunk_Clear )();
#ifdef HUNK_DEBUG
	void            *( *Hunk_AllocDebug )( int size, ha_pref pref, const char *label, const char *file, int line );
#else
	void            *( *Hunk_Alloc )( int size, ha_pref pref );
#endif
	void            *( *Hunk_AllocateTempMemory )( int size );
	void ( *Hunk_FreeTempMemory )( void *block );

	// dynamic memory allocator for things that need to be freed
	void            *( *Z_Malloc )( int bytes );
	void ( *Free )( void *buf );
	void ( *Tag_Free )();

	cvar_t          *( *Cvar_Get )( const char *name, const char *value, int flags );
	void ( *Cvar_Set )( const char *name, const char *value );

	void ( *Cmd_AddCommand )( const char *name, void ( *cmd )() );
	void ( *Cmd_RemoveCommand )( const char *name );

	int ( *Cmd_Argc )();
	const char *( *Cmd_Argv )( int i );

	const char *( *Cmd_QuoteString )( const char *text );

	int ( *Cvar_VariableIntegerValue )( const char *var_name );

	// visualization for debugging collision detection
	int ( *CM_PointContents )( const vec3_t p, clipHandle_t model );
	void ( *CM_DrawDebugSurface )( void ( *drawPoly )( int color, int numPoints, float *points ) );

	// a -1 return means the file does not exist
	// nullptr can be passed for buf to just determine existence
	int ( *FS_FileIsInPAK )( const char *name, int *pChecksum );
	int ( *FS_ReadFile )( const char *name, void **buf );
	void ( *FS_FreeFile )( void *buf );
	char           **( *FS_ListFiles )( const char *name, const char *extension, int *numfilesfound );
	void ( *FS_FreeFileList )( char **filelist );
	void ( *FS_WriteFile )( const char *qpath, const void *buffer, int size );
	bool( *FS_FileExists )( const char *file );
	int ( *FS_Seek )( fileHandle_t f, long offset, int origin );
	int ( *FS_FTell )( fileHandle_t f );
	int ( *FS_Read )( void *buffer, int len, fileHandle_t f );
	int ( *FS_FCloseFile )( fileHandle_t f );
	int ( *FS_FOpenFileRead )( const char *qpath, fileHandle_t *file, bool uniqueFILE );

	// cinematic stuff
	void ( *CIN_UploadCinematic )( int handle );
	int ( *CIN_PlayCinematic )( const char *arg0, int xpos, int ypos, int width, int height, int bits );
	e_status( *CIN_RunCinematic )( int handle );

	// XreaL BEGIN
	bool( *CL_VideoRecording )();
	void ( *CL_WriteAVIVideoFrame )( const byte *buffer, int size );
	// XreaL END

	// input event handling
	void ( *IN_Init )( void *windowData );
	void ( *IN_Shutdown )();
	void ( *IN_Restart )();
	void ( *Bot_DrawDebugMesh )( BotDebugInterface_t *in );
} refimport_t;

// this is the only function actually exported at the linker level
// If the module can't init to a valid rendering state, nullptr will be
// returned.

// RB: changed to GetRefAPI_t
typedef refexport_t *( *GetRefAPI_t )( int apiVersion, refimport_t *rimp );

#endif // __TR_PUBLIC_H
