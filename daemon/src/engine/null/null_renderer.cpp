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

#include "qcommon/q_shared.h"
#include "qcommon/qcommon.h"
#include "renderer/tr_public.h"


void RE_Shutdown( bool ) { }
qhandle_t RE_RegisterModel( const char *name )
{
	return FS_FOpenFileRead( name, nullptr, false );
}
qhandle_t RE_RegisterSkin( const char *name )
{
	return FS_FOpenFileRead( name, nullptr, false );
}
qhandle_t RE_RegisterShader( const char *, RegisterShaderFlags_t )
{
	return 1;
}
void RE_RegisterFont( const char *, const char *, int pointSize, fontInfo_t **font )
{
	if (!*font)
		return;
	(*font)->pointSize = pointSize;
	(*font)->height = 1;
	(*font)->glyphScale = 1.0f;
	(*font)->name[0] = '\0';
}
void RE_Glyph( fontInfo_t *, const char *, glyphInfo_t *glyph )
{
	glyph->height = 1;
	glyph->top = 1;
	glyph->bottom = 0;
	glyph->pitch = 1;
	glyph->xSkip = 1;
	glyph->imageWidth = 1;
	glyph->imageHeight = 1;
	glyph->s = 0.0f;
	glyph->t = 0.0f;
	glyph->s2 = 1.0f;
	glyph->t2 = 1.0f;
	glyph->glyph = 1;
	glyph->shaderName[0] = '\0';
}
void RE_GlyphChar( fontInfo_t *font, int, glyphInfo_t *glyph )
{
	RE_Glyph( font, nullptr, glyph );
}
void RE_UnregisterFont( fontInfo_t* ) { }
void RE_RegisterFontVM( const char *, const char *, int pointSize, fontMetrics_t *font )
{
	font->handle = 1;
	font->isBitmap = true;
	font->pointSize = pointSize;
	font->height = 1;
	font->glyphScale = 1.0f;
}
void RE_GlyphVM( fontHandle_t, const char *, glyphInfo_t * ) { }
void RE_GlyphCharVM( fontHandle_t, int, glyphInfo_t * ) { }
void RE_UnregisterFontVM( fontHandle_t ) { }
void RE_LoadWorldMap( const char * ) { }
void RE_SetWorldVisData( const byte * ) { }
void RE_EndRegistration() { }
void RE_ClearScene() { }
void RE_AddRefEntityToScene( const refEntity_t * ) { }
int R_LightForPoint( vec3_t, vec3_t, vec3_t, vec3_t )
{
	return 0;
}
void RE_AddPolyToScene( qhandle_t, int, const polyVert_t* ) { }
void RE_AddPolysToScene( qhandle_t, int, const polyVert_t*, int ) { }
void RE_AddLightToScene( const vec3_t, float, float, float, float, float, qhandle_t, int ) { }
void RE_AddLightToSceneQ3A( const vec3_t, float, float, float, float ) { }
void RE_RenderScene( const refdef_t* ) { }
void RE_SetColor( const Color::Color& ) { }
void RE_SetClipRegion( const float* ) { }
void RE_StretchPic( float, float, float, float, float, float, float, float, qhandle_t ) { }
void RE_RotatedPic( float, float, float, float, float, float, float, float, qhandle_t, float ) { }
void RE_StretchPicGradient( float, float, float, float, float, float, float, float, qhandle_t, const Color::Color&, int ) { }
void RE_2DPolyies( polyVert_t*, int, qhandle_t ) { }
void RE_StretchRaw( int, int, int, int, int, int, const byte*, int, bool ) { }
void RE_UploadCinematic( int, int, const byte*, int, bool ) { }
void RE_BeginFrame() { }
void RE_EndFrame( int*, int* ) { }
int R_MarkFragments( int, const vec3_t*, const vec3_t, int, vec3_t, int, markFragment_t* )
{
	return 0;
}
void RE_ProjectDecal( qhandle_t, int, vec3_t*, vec4_t, const Color::Color&, int, int ) { }
void RE_ClearDecals() { }
int R_LerpTag( orientation_t*, const refEntity_t*, const char*, int )
{
	return 0;
}
void R_ModelBounds( qhandle_t, vec3_t, vec3_t ) { }
void R_RemapShader( const char*, const char*, const char* ) { }
bool R_GetEntityToken( char*, int )
{
	return true;
}
void RE_AddPolyBufferToScene( polyBuffer_t* ) { }
bool R_inPVS( const vec3_t, const vec3_t )
{
	return false;
}
bool R_inPVVS( const vec3_t, const vec3_t )
{
	return false;
}
bool RE_LoadDynamicShader( const char*, const char* )
{
	return true;
}
void RE_Finish() { }
void RE_TakeVideoFrame( int, int, byte*, byte*, bool ) { }
void RE_AddRefLightToScene( const refLight_t* ) { }
int RE_RegisterAnimation( const char* )
{
	return 1;
}
int RE_CheckSkeleton( refSkeleton_t*, qhandle_t, qhandle_t )
{
	return 1;
}
int RE_BuildSkeleton( refSkeleton_t *skel, qhandle_t, int, int, float, bool )
{
	skel->numBones = 0;
	return 1;
}
int RE_BlendSkeleton( refSkeleton_t*, const refSkeleton_t*, float )
{
	return 1;
}
int RE_BoneIndex( qhandle_t, const char* )
{
	return 0;
}
int RE_AnimNumFrames( qhandle_t )
{
	return 1;
}
int RE_AnimFrameRate( qhandle_t )
{
	return 1;
}
qhandle_t RE_RegisterVisTest()
{
	return 1;
}
void RE_AddVisTestToScene( qhandle_t, const vec3_t, float, float ) { }
float RE_CheckVisibility( qhandle_t )
{
	return 0.0f;
}
void RE_UnregisterVisTest( qhandle_t ) { }
void     RE_SetColorGrading( int, qhandle_t ) { }
void                                RE_ScissorEnable( bool ) { }
void                                RE_ScissorSet( int, int, int, int ) { }
void     R_SetAltShaderTokens( const char* ) { }

void RE_GetTextureSize( int, int *width, int *height )
{
    *width = 1;
    *height = 1;
}
void RE_Add2dPolysIndexed( polyVert_t*, int, int*, int, int, int, qhandle_t ) { }
qhandle_t RE_GenerateTexture( const byte*, int, int )
{
    return 1;
}
const char *RE_ShaderNameFromHandle( qhandle_t )
{
    return "";
}

bool RE_BeginRegistration( glconfig_t *config, glconfig2_t *glconfig2 )
{
	Com_Memset( config, 0, sizeof( glconfig_t ) );
	config->vidWidth = 640;
	config->vidHeight = 480;
	config->windowAspect = 1.0f;
	Com_Memset( glconfig2, 0, sizeof( glconfig2_t ) );

	return true;
}

refexport_t    *GetRefAPI( int, refimport_t* )
{
    static refexport_t re;

    memset( &re, 0, sizeof( re ) );

    re.Shutdown = RE_Shutdown;

    re.BeginRegistration = RE_BeginRegistration;
    re.RegisterModel = RE_RegisterModel;
    re.RegisterSkin = RE_RegisterSkin;
    re.RegisterShader = RE_RegisterShader;
    re.RegisterShader = RE_RegisterShader;
    re.RegisterFont = RE_RegisterFont;
    re.Glyph = RE_Glyph;
    re.GlyphChar = RE_GlyphChar;
    re.UnregisterFont = RE_UnregisterFont;
    re.RegisterFontVM = RE_RegisterFontVM;
    re.GlyphVM = RE_GlyphVM;
    re.GlyphCharVM = RE_GlyphCharVM;
    re.UnregisterFontVM = RE_UnregisterFontVM;
    re.LoadWorld = RE_LoadWorldMap;
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

    re.RenderScene = RE_RenderScene;

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
    re.GetEntityToken = R_GetEntityToken;

    re.AddPolyBufferToScene = RE_AddPolyBufferToScene;

    re.inPVS = R_inPVS;
    re.inPVVS = R_inPVVS;

    //bani
    re.LoadDynamicShader = RE_LoadDynamicShader;
    //bani
    re.Finish = RE_Finish;

    re.TakeVideoFrame = RE_TakeVideoFrame;
    re.AddRefLightToScene = RE_AddRefLightToScene;

    // RB: alternative skeletal animation system
    re.RegisterAnimation = RE_RegisterAnimation;
    re.CheckSkeleton = RE_CheckSkeleton;
    re.BuildSkeleton = RE_BuildSkeleton;
    re.BlendSkeleton = RE_BlendSkeleton;
    re.BoneIndex = RE_BoneIndex;
    re.AnimNumFrames = RE_AnimNumFrames;
    re.AnimFrameRate = RE_AnimFrameRate;

    re.RegisterVisTest = RE_RegisterVisTest;
    re.AddVisTestToScene = RE_AddVisTestToScene;
    re.CheckVisibility = RE_CheckVisibility;
    re.UnregisterVisTest = RE_UnregisterVisTest;

    re.SetColorGrading = RE_SetColorGrading;
    re.SetAltShaderTokens = R_SetAltShaderTokens;

    re.ScissorEnable = RE_ScissorEnable;
    re.ScissorSet = RE_ScissorSet;

    re.GetTextureSize = RE_GetTextureSize;
    re.Add2dPolysIndexed = RE_Add2dPolysIndexed;
    re.GenerateTexture = RE_GenerateTexture;
    re.ShaderNameFromHandle = RE_ShaderNameFromHandle;

    return &re;
}

