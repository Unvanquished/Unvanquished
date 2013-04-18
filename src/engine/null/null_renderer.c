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

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "../renderer/tr_public.h"


void RE_Shutdown( qboolean destroyWindow ) { }
qhandle_t RE_RegisterModel( const char *name ) { return 1; }
qhandle_t RE_RegisterSkin( const char *name ) { return 1; }
qhandle_t RE_RegisterShader( const char *name, RegisterShaderFlags_t flags ) { return 1; }
qhandle_t RE_RegisterShaderNoMip( const char *name ) { return 1; }
qhandle_t RE_RegisterLightAttenuation( const char *name ) { return  1; }
void RE_RegisterFont( const char *fontName, const char *fallbackName, int pointSize, fontInfo_t *font ) { }
void RE_Glyph( fontInfo_t *font, const char *str, glyphInfo_t *glyphh ) { }
void RE_GlyphChar( fontInfo_t *font, int ch, glyphInfo_t *glyph ) { }
void RE_UnregisterFont( fontInfo_t *font ) { }
void RE_RegisterFontVM( const char *fontName, const char *fallbackName, int pointSize, fontMetrics_t *font ) { }
void RE_GlyphVM( fontHandle_t font, const char *ch, glyphInfo_t *glyph ) { }
void RE_GlyphCharVM( fontHandle_t font, int ch, glyphInfo_t *glyph ) { }
void RE_UnregisterFontVM( fontHandle_t font ) { }
void RE_LoadWorldMap( const char *name ) { }
qboolean RE_GetSkinModel( qhandle_t skinid, const char *type, char *name ) { return qtrue; }
qhandle_t RE_GetShaderFromModel( qhandle_t modelid, int surfnum, int withlightmap ) { return qtrue; }
void RE_SetWorldVisData( const byte *vis ) { }
void RE_EndRegistration( void ) { }
void RE_ClearScene( void ) { }
void RE_AddRefEntityToScene( const refEntity_t *ref ) { }
int R_LightForPoint( vec3_t point, vec3_t ambientLight, vec3_t directedLight, vec3_t lightDir ) { return 0; }
void RE_AddPolyToScene( qhandle_t hShader, int numVerts, const polyVert_t *verts ) { }
void RE_AddPolysToScene( qhandle_t hShader, int numVerts, const polyVert_t *verts, int numPolys ) { }
void RE_AddLightToScene( const vec3_t org, float radius, float intensity, float r, float g, float b, qhandle_t hShader, int flags ) { }
void RE_AddLightToSceneQ3A( const vec3_t org, float intensity, float r, float g, float b ) { }
void RE_AddCoronaToScene( const vec3_t org, float r, float g, float b, float scale, int id, qboolean visible ) { }
void R_SetFog( int fogvar, int var1, int var2, float r, float g, float b, float density ) { }
void RE_RenderScene( const refdef_t *fd ) { }
void RE_SaveViewParms( void ) { }
void RE_RestoreViewParms( void ) { }
void RE_SetColor( const float *rgba ) { }
void RE_SetClipRegion( const float *region ) { }
void RE_StretchPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader ) { }
void RE_RotatedPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader, float angle ) { }
void RE_StretchPicGradient( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader, const float *gradientColor, int gradientType ) { }
void RE_2DPolyies( polyVert_t *polys, int numverts, qhandle_t hShader ) { }
void RE_StretchRaw( int x, int y, int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty ) { }
void RE_UploadCinematic( int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty ) { }
void RE_BeginFrame( stereoFrame_t stereoFrame ) { }
void RE_EndFrame( int *frontEndMsec, int *backEndMsec ) { }
int R_MarkFragments( int numPoints, const vec3_t *points, const vec3_t projection, int maxPoints, vec3_t pointBuffer, int maxFragments, markFragment_t *fragmentBuffer ) { return 0; }
void RE_ProjectDecal( qhandle_t hShader, int numPoints, vec3_t *points, vec4_t projection, vec4_t color, int lifeTime, int fadeTime ) { }
void RE_ClearDecals( void ) { }
int R_LerpTag( orientation_t *tag, const refEntity_t *refent, const char *tagName, int startIndex ) { return 0; }
void R_ModelBounds( qhandle_t model, vec3_t mins, vec3_t maxs ) { }
void R_RemapShader( const char *oldShader, const char *newShader, const char *offsetTime ) { }
void R_DebugPolygon( int color, int numpoints, float *points ) { }
void R_DebugText( const vec3_t org, float r, float g, float b, const char *text, qboolean neverOcclude ) { }
qboolean R_GetEntityToken( char *buffer, int size ) { return qtrue; }
void RE_AddPolyBufferToScene( polyBuffer_t *pPolyBuffer ) { }
void RE_SetGlobalFog( qboolean restore, int duration, float r, float g, float b, float depthForOpaque ) { }
qboolean R_inPVS( const vec3_t p1, const vec3_t p2 ) { return qfalse; }
qboolean R_inPVVS( const vec3_t p1, const vec3_t p2 ) { return qfalse; }
void R_PurgeCache( void ) { }
qboolean RE_LoadDynamicShader( const char *shadername, const char *shadertext ) { return qtrue; }
void RE_RenderToTexture( int textureid, int x, int y, int w, int h ) { }
int R_GetTextureId( const char *imagename ) { return 0; }
void RE_Finish( void ) { }
void RE_TakeVideoFrame( int h, int w, byte *captureBuffer, byte *encodeBuffer, qboolean motionJpeg ) { }
void RE_AddRefLightToScene( const refLight_t *light ) { }
int RE_RegisterAnimation( const char *name ) { return 1; }
int RE_CheckSkeleton( refSkeleton_t *skel, qhandle_t model, qhandle_t anim ) { return 1; }
int RE_BuildSkeleton( refSkeleton_t *skel, qhandle_t anim, int startFrame, int endFrame, float frac, qboolean clearOrigin ) { return 0; }
int RE_BlendSkeleton( refSkeleton_t *skel, const refSkeleton_t *blend, float frac ) { return 0; }
int RE_BoneIndex( qhandle_t hModel, const char *boneName ) { return 0; }
int RE_AnimNumFrames( qhandle_t hAnim ) { return 1; }
int RE_AnimFrameRate( qhandle_t hAnim ) { return 1; }
qhandle_t RE_RegisterVisTest( void ) { return 1; }
void RE_AddVisTestToScene( qhandle_t hTest, vec3_t pos, float depthAdjust, float area ) { }
float RE_CheckVisibility( qhandle_t hTest ) { return 0.0f; }
void RE_UnregisterVisTest( qhandle_t hTest ) { }

qboolean RE_BeginRegistration( glconfig_t *config, glconfig2_t *glconfig2 )
{
	Com_Memset( config, 0, sizeof( glconfig_t ) );
	Com_Memset( glconfig2, 0, sizeof( glconfig2_t ) );

	return qtrue;
}

#if defined( __cplusplus )
extern "C" {
	#endif
	refexport_t    *GetRefAPI( int apiVersion, refimport_t *rimp )
	{
		static refexport_t re;

		memset( &re, 0, sizeof( re ) );

		re.Shutdown = RE_Shutdown;

		re.BeginRegistration = RE_BeginRegistration;
		re.RegisterModel = RE_RegisterModel;
		re.RegisterSkin = RE_RegisterSkin;
		re.RegisterShader = RE_RegisterShader;
		#if defined( USE_REFLIGHT )
		re.RegisterShaderLightAttenuation = RE_RegisterLightAttenuation;
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
		re.AddAdditiveLightToScene = RE_AddLightToSceneQ3A;
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
		re.inPVVS = R_inPVVS;

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
		re.AddRefLightToScene = RE_AddRefLightToScene;
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

		re.RegisterVisTest = RE_RegisterVisTest;
		re.AddVisTestToScene = RE_AddVisTestToScene;
		re.CheckVisibility = RE_CheckVisibility;
		re.UnregisterVisTest = RE_UnregisterVisTest;

		return &re;
	}

	#if defined( __cplusplus )
} // extern "C"
#endif

