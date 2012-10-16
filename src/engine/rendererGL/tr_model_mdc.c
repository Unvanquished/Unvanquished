/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
Copyright (C) 2010 Robert Beckebans <trebor_7@users.sourceforge.net>

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
// tr_model_mdc.c -- Enemy Territory .mdc model loading and caching

#include "tr_local.h"

#define LL(x) x = LittleLong(x)
#define LF(x) x = LittleFloat(x)

#define NUMMDCVERTEXNORMALS 256

// *INDENT-OFF*
#if 0
static const float r_anormals[ NUMMDCVERTEXNORMALS ][ 3 ] =
{
	{ 1.000000,  0.000000,  0.000000  },
	{ 0.980785,  0.195090,  0.000000  },
	{ 0.923880,  0.382683,  0.000000  },
	{ 0.831470,  0.555570,  0.000000  },
	{ 0.707107,  0.707107,  0.000000  },
	{ 0.555570,  0.831470,  0.000000  },
	{ 0.382683,  0.923880,  0.000000  },
	{ 0.195090,  0.980785,  0.000000  },
	{ -0.000000, 1.000000,  0.000000  },
	{ -0.195090, 0.980785,  0.000000  },
	{ -0.382683, 0.923880,  0.000000  },
	{ -0.555570, 0.831470,  0.000000  },
	{ -0.707107, 0.707107,  0.000000  },
	{ -0.831470, 0.555570,  0.000000  },
	{ -0.923880, 0.382683,  0.000000  },
	{ -0.980785, 0.195090,  0.000000  },
	{ -1.000000, -0.000000, 0.000000  },
	{ -0.980785, -0.195090, 0.000000  },
	{ -0.923880, -0.382683, 0.000000  },
	{ -0.831470, -0.555570, 0.000000  },
	{ -0.707107, -0.707107, 0.000000  },
	{ -0.555570, -0.831469, 0.000000  },
	{ -0.382684, -0.923880, 0.000000  },
	{ -0.195090, -0.980785, 0.000000  },
	{ 0.000000,  -1.000000, 0.000000  },
	{ 0.195090,  -0.980785, 0.000000  },
	{ 0.382684,  -0.923879, 0.000000  },
	{ 0.555570,  -0.831470, 0.000000  },
	{ 0.707107,  -0.707107, 0.000000  },
	{ 0.831470,  -0.555570, 0.000000  },
	{ 0.923880,  -0.382683, 0.000000  },
	{ 0.980785,  -0.195090, 0.000000  },
	{ 0.980785,  0.000000,  -0.195090 },
	{ 0.956195,  0.218245,  -0.195090 },
	{ 0.883657,  0.425547,  -0.195090 },
	{ 0.766809,  0.611510,  -0.195090 },
	{ 0.611510,  0.766809,  -0.195090 },
	{ 0.425547,  0.883657,  -0.195090 },
	{ 0.218245,  0.956195,  -0.195090 },
	{ -0.000000, 0.980785,  -0.195090 },
	{ -0.218245, 0.956195,  -0.195090 },
	{ -0.425547, 0.883657,  -0.195090 },
	{ -0.611510, 0.766809,  -0.195090 },
	{ -0.766809, 0.611510,  -0.195090 },
	{ -0.883657, 0.425547,  -0.195090 },
	{ -0.956195, 0.218245,  -0.195090 },
	{ -0.980785, -0.000000, -0.195090 },
	{ -0.956195, -0.218245, -0.195090 },
	{ -0.883657, -0.425547, -0.195090 },
	{ -0.766809, -0.611510, -0.195090 },
	{ -0.611510, -0.766809, -0.195090 },
	{ -0.425547, -0.883657, -0.195090 },
	{ -0.218245, -0.956195, -0.195090 },
	{ 0.000000,  -0.980785, -0.195090 },
	{ 0.218245,  -0.956195, -0.195090 },
	{ 0.425547,  -0.883657, -0.195090 },
	{ 0.611510,  -0.766809, -0.195090 },
	{ 0.766809,  -0.611510, -0.195090 },
	{ 0.883657,  -0.425547, -0.195090 },
	{ 0.956195,  -0.218245, -0.195090 },
	{ 0.923880,  0.000000,  -0.382683 },
	{ 0.892399,  0.239118,  -0.382683 },
	{ 0.800103,  0.461940,  -0.382683 },
	{ 0.653281,  0.653281,  -0.382683 },
	{ 0.461940,  0.800103,  -0.382683 },
	{ 0.239118,  0.892399,  -0.382683 },
	{ -0.000000, 0.923880,  -0.382683 },
	{ -0.239118, 0.892399,  -0.382683 },
	{ -0.461940, 0.800103,  -0.382683 },
	{ -0.653281, 0.653281,  -0.382683 },
	{ -0.800103, 0.461940,  -0.382683 },
	{ -0.892399, 0.239118,  -0.382683 },
	{ -0.923880, -0.000000, -0.382683 },
	{ -0.892399, -0.239118, -0.382683 },
	{ -0.800103, -0.461940, -0.382683 },
	{ -0.653282, -0.653281, -0.382683 },
	{ -0.461940, -0.800103, -0.382683 },
	{ -0.239118, -0.892399, -0.382683 },
	{ 0.000000,  -0.923880, -0.382683 },
	{ 0.239118,  -0.892399, -0.382683 },
	{ 0.461940,  -0.800103, -0.382683 },
	{ 0.653281,  -0.653282, -0.382683 },
	{ 0.800103,  -0.461940, -0.382683 },
	{ 0.892399,  -0.239117, -0.382683 },
	{ 0.831470,  0.000000,  -0.555570 },
	{ 0.790775,  0.256938,  -0.555570 },
	{ 0.672673,  0.488726,  -0.555570 },
	{ 0.488726,  0.672673,  -0.555570 },
	{ 0.256938,  0.790775,  -0.555570 },
	{ -0.000000, 0.831470,  -0.555570 },
	{ -0.256938, 0.790775,  -0.555570 },
	{ -0.488726, 0.672673,  -0.555570 },
	{ -0.672673, 0.488726,  -0.555570 },
	{ -0.790775, 0.256938,  -0.555570 },
	{ -0.831470, -0.000000, -0.555570 },
	{ -0.790775, -0.256938, -0.555570 },
	{ -0.672673, -0.488726, -0.555570 },
	{ -0.488725, -0.672673, -0.555570 },
	{ -0.256938, -0.790775, -0.555570 },
	{ 0.000000,  -0.831470, -0.555570 },
	{ 0.256938,  -0.790775, -0.555570 },
	{ 0.488725,  -0.672673, -0.555570 },
	{ 0.672673,  -0.488726, -0.555570 },
	{ 0.790775,  -0.256938, -0.555570 },
	{ 0.707107,  0.000000,  -0.707107 },
	{ 0.653281,  0.270598,  -0.707107 },
	{ 0.500000,  0.500000,  -0.707107 },
	{ 0.270598,  0.653281,  -0.707107 },
	{ -0.000000, 0.707107,  -0.707107 },
	{ -0.270598, 0.653282,  -0.707107 },
	{ -0.500000, 0.500000,  -0.707107 },
	{ -0.653281, 0.270598,  -0.707107 },
	{ -0.707107, -0.000000, -0.707107 },
	{ -0.653281, -0.270598, -0.707107 },
	{ -0.500000, -0.500000, -0.707107 },
	{ -0.270598, -0.653281, -0.707107 },
	{ 0.000000,  -0.707107, -0.707107 },
	{ 0.270598,  -0.653281, -0.707107 },
	{ 0.500000,  -0.500000, -0.707107 },
	{ 0.653282,  -0.270598, -0.707107 },
	{ 0.555570,  0.000000,  -0.831470 },
	{ 0.481138,  0.277785,  -0.831470 },
	{ 0.277785,  0.481138,  -0.831470 },
	{ -0.000000, 0.555570,  -0.831470 },
	{ -0.277785, 0.481138,  -0.831470 },
	{ -0.481138, 0.277785,  -0.831470 },
	{ -0.555570, -0.000000, -0.831470 },
	{ -0.481138, -0.277785, -0.831470 },
	{ -0.277785, -0.481138, -0.831470 },
	{ 0.000000,  -0.555570, -0.831470 },
	{ 0.277785,  -0.481138, -0.831470 },
	{ 0.481138,  -0.277785, -0.831470 },
	{ 0.382683,  0.000000,  -0.923880 },
	{ 0.270598,  0.270598,  -0.923880 },
	{ -0.000000, 0.382683,  -0.923880 },
	{ -0.270598, 0.270598,  -0.923880 },
	{ -0.382683, -0.000000, -0.923880 },
	{ -0.270598, -0.270598, -0.923880 },
	{ 0.000000,  -0.382683, -0.923880 },
	{ 0.270598,  -0.270598, -0.923880 },
	{ 0.195090,  0.000000,  -0.980785 },
	{ -0.000000, 0.195090,  -0.980785 },
	{ -0.195090, -0.000000, -0.980785 },
	{ 0.000000,  -0.195090, -0.980785 },
	{ 0.980785,  0.000000,  0.195090  },
	{ 0.956195,  0.218245,  0.195090  },
	{ 0.883657,  0.425547,  0.195090  },
	{ 0.766809,  0.611510,  0.195090  },
	{ 0.611510,  0.766809,  0.195090  },
	{ 0.425547,  0.883657,  0.195090  },
	{ 0.218245,  0.956195,  0.195090  },
	{ -0.000000, 0.980785,  0.195090  },
	{ -0.218245, 0.956195,  0.195090  },
	{ -0.425547, 0.883657,  0.195090  },
	{ -0.611510, 0.766809,  0.195090  },
	{ -0.766809, 0.611510,  0.195090  },
	{ -0.883657, 0.425547,  0.195090  },
	{ -0.956195, 0.218245,  0.195090  },
	{ -0.980785, -0.000000, 0.195090  },
	{ -0.956195, -0.218245, 0.195090  },
	{ -0.883657, -0.425547, 0.195090  },
	{ -0.766809, -0.611510, 0.195090  },
	{ -0.611510, -0.766809, 0.195090  },
	{ -0.425547, -0.883657, 0.195090  },
	{ -0.218245, -0.956195, 0.195090  },
	{ 0.000000,  -0.980785, 0.195090  },
	{ 0.218245,  -0.956195, 0.195090  },
	{ 0.425547,  -0.883657, 0.195090  },
	{ 0.611510,  -0.766809, 0.195090  },
	{ 0.766809,  -0.611510, 0.195090  },
	{ 0.883657,  -0.425547, 0.195090  },
	{ 0.956195,  -0.218245, 0.195090  },
	{ 0.923880,  0.000000,  0.382683  },
	{ 0.892399,  0.239118,  0.382683  },
	{ 0.800103,  0.461940,  0.382683  },
	{ 0.653281,  0.653281,  0.382683  },
	{ 0.461940,  0.800103,  0.382683  },
	{ 0.239118,  0.892399,  0.382683  },
	{ -0.000000, 0.923880,  0.382683  },
	{ -0.239118, 0.892399,  0.382683  },
	{ -0.461940, 0.800103,  0.382683  },
	{ -0.653281, 0.653281,  0.382683  },
	{ -0.800103, 0.461940,  0.382683  },
	{ -0.892399, 0.239118,  0.382683  },
	{ -0.923880, -0.000000, 0.382683  },
	{ -0.892399, -0.239118, 0.382683  },
	{ -0.800103, -0.461940, 0.382683  },
	{ -0.653282, -0.653281, 0.382683  },
	{ -0.461940, -0.800103, 0.382683  },
	{ -0.239118, -0.892399, 0.382683  },
	{ 0.000000,  -0.923880, 0.382683  },
	{ 0.239118,  -0.892399, 0.382683  },
	{ 0.461940,  -0.800103, 0.382683  },
	{ 0.653281,  -0.653282, 0.382683  },
	{ 0.800103,  -0.461940, 0.382683  },
	{ 0.892399,  -0.239117, 0.382683  },
	{ 0.831470,  0.000000,  0.555570  },
	{ 0.790775,  0.256938,  0.555570  },
	{ 0.672673,  0.488726,  0.555570  },
	{ 0.488726,  0.672673,  0.555570  },
	{ 0.256938,  0.790775,  0.555570  },
	{ -0.000000, 0.831470,  0.555570  },
	{ -0.256938, 0.790775,  0.555570  },
	{ -0.488726, 0.672673,  0.555570  },
	{ -0.672673, 0.488726,  0.555570  },
	{ -0.790775, 0.256938,  0.555570  },
	{ -0.831470, -0.000000, 0.555570  },
	{ -0.790775, -0.256938, 0.555570  },
	{ -0.672673, -0.488726, 0.555570  },
	{ -0.488725, -0.672673, 0.555570  },
	{ -0.256938, -0.790775, 0.555570  },
	{ 0.000000,  -0.831470, 0.555570  },
	{ 0.256938,  -0.790775, 0.555570  },
	{ 0.488725,  -0.672673, 0.555570  },
	{ 0.672673,  -0.488726, 0.555570  },
	{ 0.790775,  -0.256938, 0.555570  },
	{ 0.707107,  0.000000,  0.707107  },
	{ 0.653281,  0.270598,  0.707107  },
	{ 0.500000,  0.500000,  0.707107  },
	{ 0.270598,  0.653281,  0.707107  },
	{ -0.000000, 0.707107,  0.707107  },
	{ -0.270598, 0.653282,  0.707107  },
	{ -0.500000, 0.500000,  0.707107  },
	{ -0.653281, 0.270598,  0.707107  },
	{ -0.707107, -0.000000, 0.707107  },
	{ -0.653281, -0.270598, 0.707107  },
	{ -0.500000, -0.500000, 0.707107  },
	{ -0.270598, -0.653281, 0.707107  },
	{ 0.000000,  -0.707107, 0.707107  },
	{ 0.270598,  -0.653281, 0.707107  },
	{ 0.500000,  -0.500000, 0.707107  },
	{ 0.653282,  -0.270598, 0.707107  },
	{ 0.555570,  0.000000,  0.831470  },
	{ 0.481138,  0.277785,  0.831470  },
	{ 0.277785,  0.481138,  0.831470  },
	{ -0.000000, 0.555570,  0.831470  },
	{ -0.277785, 0.481138,  0.831470  },
	{ -0.481138, 0.277785,  0.831470  },
	{ -0.555570, -0.000000, 0.831470  },
	{ -0.481138, -0.277785, 0.831470  },
	{ -0.277785, -0.481138, 0.831470  },
	{ 0.000000,  -0.555570, 0.831470  },
	{ 0.277785,  -0.481138, 0.831470  },
	{ 0.481138,  -0.277785, 0.831470  },
	{ 0.382683,  0.000000,  0.923880  },
	{ 0.270598,  0.270598,  0.923880  },
	{ -0.000000, 0.382683,  0.923880  },
	{ -0.270598, 0.270598,  0.923880  },
	{ -0.382683, -0.000000, 0.923880  },
	{ -0.270598, -0.270598, 0.923880  },
	{ 0.000000,  -0.382683, 0.923880  },
	{ 0.270598,  -0.270598, 0.923880  },
	{ 0.195090,  0.000000,  0.980785  },
	{ -0.000000, 0.195090,  0.980785  },
	{ -0.195090, -0.000000, 0.980785  },
	{ 0.000000,  -0.195090, 0.980785  },
};
#endif
// *INDENT-ON*

// NOTE: MDC_MAX_ERROR is effectively the compression level. the lower this value, the higher
// the accuracy, but with lower compression ratios.
#define MDC_MAX_ERROR     0.1 // if any compressed vert is off by more than this from the
// actual vert, make this a baseframe

#define MDC_DIST_SCALE    0.05 // lower for more accuracy, but less range

// note: we are locked in at 8 or less bits since changing to byte-encoded normals
#define MDC_BITS_PER_AXIS 8
#define MDC_MAX_OFS       127.0 // to be safe

#define MDC_MAX_DIST      ( MDC_MAX_OFS * MDC_DIST_SCALE )

#if 0
void R_MDC_DecodeXyzCompressed( mdcXyzCompressed_t *xyzComp, vec3_t out, vec3_t normal );

#else // optimized version
#define R_MDC_DecodeXyzCompressed( ofsVec, out, normal ) \
        ( out )[ 0 ] = ( (float)( ( ofsVec ) & 255 ) - MDC_MAX_OFS ) * MDC_DIST_SCALE; \
        ( out )[ 1 ] = ( (float)( ( ofsVec >> 8 ) & 255 ) - MDC_MAX_OFS ) * MDC_DIST_SCALE; \
        ( out )[ 2 ] = ( (float)( ( ofsVec >> 16 ) & 255 ) - MDC_MAX_OFS ) * MDC_DIST_SCALE; \
        VectorCopy( ( r_anormals )[ ( ofsVec >> 24 ) ], normal ); //This doesn't do anything...

#define R_MDC_DecodeXyzCompressed2( ofsVec, out ) \
        ( out )[ 0 ] = ( (float)( ( ofsVec ) & 255 ) - MDC_MAX_OFS ) * MDC_DIST_SCALE; \
        ( out )[ 1 ] = ( (float)( ( ofsVec >> 8 ) & 255 ) - MDC_MAX_OFS ) * MDC_DIST_SCALE; \
        ( out )[ 2 ] = ( (float)( ( ofsVec >> 16 ) & 255 ) - MDC_MAX_OFS ) * MDC_DIST_SCALE; \
        //VectorCopy( ( r_anormals )[( ofsVec >> 24 )], normal ); //This doesn't do anything...
#endif

/*
=================
R_LoadMDC
=================
*/
qboolean R_LoadMDC( model_t *mod, int lod, void *buffer, int bufferSize, const char *modName )
{
	int                i, j, k;

	mdcHeader_t        *mdcModel;
	md3Frame_t         *mdcFrame;
	mdcSurface_t       *mdcSurf;
	md3Shader_t        *mdcShader;
	md3Triangle_t      *mdcTri;
	md3St_t            *mdcst;
	md3XyzNormal_t     *mdcxyz;
	mdcXyzCompressed_t *mdcxyzComp;
	mdcTag_t           *mdcTag;
	mdcTagName_t       *mdcTagName;

	mdvModel_t         *mdvModel;
	mdvFrame_t         *frame;
	mdvSurface_t       *surf; //, *surface; //unused
	srfTriangle_t      *tri;
	mdvXyz_t           *v;
	mdvSt_t            *st;
	mdvTag_t           *tag;
	mdvTagName_t       *tagName;
	short              *ps;

	int                version;
	int                size;

	mdcModel = ( mdcHeader_t * ) buffer;

	version = LittleLong( mdcModel->version );

	if ( version != MDC_VERSION )
	{
		ri.Printf( PRINT_WARNING, "R_LoadMD3: %s has wrong version (%i should be %i)\n", modName, version, MDC_VERSION );
		return qfalse;
	}

	mod->type = MOD_MESH;
	size = LittleLong( mdcModel->ofsEnd );
	mod->dataSize += size;
	mdvModel = mod->mdv[ lod ] = ri.Hunk_Alloc( sizeof( mdvModel_t ), h_low );

	LL( mdcModel->ident );
	LL( mdcModel->version );
	LL( mdcModel->numFrames );
	LL( mdcModel->numTags );
	LL( mdcModel->numSurfaces );
	LL( mdcModel->ofsFrames );
	LL( mdcModel->ofsTags );
	LL( mdcModel->ofsSurfaces );
	LL( mdcModel->ofsEnd );
	LL( mdcModel->ofsEnd );
	LL( mdcModel->flags );
	LL( mdcModel->numSkins );

	if ( mdcModel->numFrames < 1 )
	{
		ri.Printf( PRINT_WARNING, "R_LoadMDC: '%s' has no frames\n", modName );
		return qfalse;
	}

	// swap all the frames
	mdvModel->numFrames = mdcModel->numFrames;
	mdvModel->frames = frame = ri.Hunk_Alloc( sizeof( *frame ) * mdcModel->numFrames, h_low );

	mdcFrame = ( md3Frame_t * )( ( byte * ) mdcModel + mdcModel->ofsFrames );

	for ( i = 0; i < mdcModel->numFrames; i++, frame++, mdcFrame++ )
	{
#if 1

		// RB: ET HACK
		if ( strstr( mod->name, "sherman" ) || strstr( mod->name, "mg42" ) )
		{
			frame->radius = 256;

			for ( j = 0; j < 3; j++ )
			{
				frame->bounds[ 0 ][ j ] = 128;
				frame->bounds[ 1 ][ j ] = -128;
				frame->localOrigin[ j ] = LittleFloat( mdcFrame->localOrigin[ j ] );
			}
		}
		else
#endif
		{
			frame->radius = LittleFloat( mdcFrame->radius );

			for ( j = 0; j < 3; j++ )
			{
				frame->bounds[ 0 ][ j ] = LittleFloat( mdcFrame->bounds[ 0 ][ j ] );
				frame->bounds[ 1 ][ j ] = LittleFloat( mdcFrame->bounds[ 1 ][ j ] );
				frame->localOrigin[ j ] = LittleFloat( mdcFrame->localOrigin[ j ] );
			}
		}
	}

	// swap all the tags
	mdvModel->numTags = mdcModel->numTags;
	mdvModel->tags = tag = ri.Hunk_Alloc( sizeof( *tag ) * ( mdcModel->numTags * mdcModel->numFrames ), h_low );

	mdcTag = ( mdcTag_t * )( ( byte * ) mdcModel + mdcModel->ofsTags );

	for ( i = 0; i < mdcModel->numTags * mdcModel->numFrames; i++, tag++, mdcTag++ )
	{
		vec3_t angles;

		for ( j = 0; j < 3; j++ )
		{
			tag->origin[ j ] = ( float ) LittleShort( mdcTag->xyz[ j ] ) * MD3_XYZ_SCALE;
			angles[ j ] = ( float ) LittleShort( mdcTag->angles[ j ] ) * MDC_TAG_ANGLE_SCALE;
		}

		AnglesToAxis( angles, tag->axis );
	}

	mdvModel->tagNames = tagName = ri.Hunk_Alloc( sizeof( *tagName ) * ( mdcModel->numTags ), h_low );

	mdcTagName = ( mdcTagName_t * )( ( byte * ) mdcModel + mdcModel->ofsTagNames );

	for ( i = 0; i < mdcModel->numTags; i++, tagName++, mdcTagName++ )
	{
		Q_strncpyz( tagName->name, mdcTagName->name, sizeof( tagName->name ) );
	}

	// swap all the surfaces
	mdvModel->numSurfaces = mdcModel->numSurfaces;
	mdvModel->surfaces = surf = ri.Hunk_Alloc( sizeof( *surf ) * mdcModel->numSurfaces, h_low );

	mdcSurf = ( mdcSurface_t * )( ( byte * ) mdcModel + mdcModel->ofsSurfaces );

	for ( i = 0; i < mdcModel->numSurfaces; i++ )
	{
		LL( mdcSurf->ident );
		LL( mdcSurf->flags );
		LL( mdcSurf->numBaseFrames );
		LL( mdcSurf->numCompFrames );
		LL( mdcSurf->numShaders );
		LL( mdcSurf->numTriangles );
		LL( mdcSurf->ofsTriangles );
		LL( mdcSurf->numVerts );
		LL( mdcSurf->ofsShaders );
		LL( mdcSurf->ofsSt );
		LL( mdcSurf->ofsXyzNormals );
		LL( mdcSurf->ofsXyzNormals );
		LL( mdcSurf->ofsXyzCompressed );
		LL( mdcSurf->ofsFrameBaseFrames );
		LL( mdcSurf->ofsFrameCompFrames );
		LL( mdcSurf->ofsEnd );

		if ( mdcSurf->numVerts > SHADER_MAX_VERTEXES )
		{
			ri.Error( ERR_DROP, "R_LoadMDC: %s has more than %i verts on a surface (%i)",
			          modName, SHADER_MAX_VERTEXES, mdcSurf->numVerts );
		}

		if ( mdcSurf->numTriangles > SHADER_MAX_TRIANGLES )
		{
			ri.Error( ERR_DROP, "R_LoadMDC: %s has more than %i triangles on a surface (%i)",
			          modName, SHADER_MAX_TRIANGLES, mdcSurf->numTriangles );
		}

		// change to surface identifier
		surf->surfaceType = SF_MDV;

		// give pointer to model for Tess_SurfaceMDX
		surf->model = mdvModel;

		// copy surface name
		Q_strncpyz( surf->name, mdcSurf->name, sizeof( surf->name ) );

		// lowercase the surface name so skin compares are faster
		Q_strlwr( surf->name );

		// strip off a trailing _1 or _2
		// this is a crutch for q3data being a mess
		j = strlen( surf->name );

		if ( j > 2 && surf->name[ j - 2 ] == '_' )
		{
			surf->name[ j - 2 ] = 0;
		}

		// register the shaders

		/*
		   surf->numShaders = md3Surf->numShaders;
		   surf->shaders = shader = ri.Hunk_Alloc(sizeof(*shader) * md3Surf->numShaders, h_low);

		   md3Shader = (md3Shader_t *) ((byte *) md3Surf + md3Surf->ofsShaders);
		   for(j = 0; j < md3Surf->numShaders; j++, shader++, md3Shader++)
		   {
		   shader_t       *sh;

		   sh = R_FindShader(md3Shader->name, SHADER_3D_DYNAMIC, qtrue);
		   if(sh->defaultShader)
		   {
		   shader->shaderIndex = 0;
		   }
		   else
		   {
		   shader->shaderIndex = sh->index;
		   }
		   }
		 */

		// only consider the first shader
		mdcShader = ( md3Shader_t * )( ( byte * ) mdcSurf + mdcSurf->ofsShaders );
		surf->shader = R_FindShader( mdcShader->name, SHADER_3D_DYNAMIC, qtrue );

		// swap all the triangles
		surf->numTriangles = mdcSurf->numTriangles;
		surf->triangles = tri = ri.Hunk_Alloc( sizeof( *tri ) * mdcSurf->numTriangles, h_low );

		mdcTri = ( md3Triangle_t * )( ( byte * ) mdcSurf + mdcSurf->ofsTriangles );

		for ( j = 0; j < mdcSurf->numTriangles; j++, tri++, mdcTri++ )
		{
			tri->indexes[ 0 ] = LittleLong( mdcTri->indexes[ 0 ] );
			tri->indexes[ 1 ] = LittleLong( mdcTri->indexes[ 1 ] );
			tri->indexes[ 2 ] = LittleLong( mdcTri->indexes[ 2 ] );
		}

		// swap all the XyzNormals
		mdcxyz = ( md3XyzNormal_t * )( ( byte * ) mdcSurf + mdcSurf->ofsXyzNormals );

		for ( j = 0; j < mdcSurf->numVerts * mdcSurf->numBaseFrames; j++, mdcxyz++ )
		{
			mdcxyz->xyz[ 0 ] = LittleShort( mdcxyz->xyz[ 0 ] );
			mdcxyz->xyz[ 1 ] = LittleShort( mdcxyz->xyz[ 1 ] );
			mdcxyz->xyz[ 2 ] = LittleShort( mdcxyz->xyz[ 2 ] );

			mdcxyz->normal = LittleShort( mdcxyz->normal );
		}

		// swap all the XyzCompressed
		mdcxyzComp = ( mdcXyzCompressed_t * )( ( byte * ) mdcSurf + mdcSurf->ofsXyzCompressed );

		for ( j = 0; j < mdcSurf->numVerts * mdcSurf->numCompFrames; j++, mdcxyzComp++ )
		{
			LL( mdcxyzComp->ofsVec );
		}

		// swap the frameBaseFrames
		ps = ( short * )( ( byte * ) mdcSurf + mdcSurf->ofsFrameBaseFrames );

		for ( j = 0; j < mdcModel->numFrames; j++, ps++ )
		{
			*ps = LittleShort( *ps );
		}

		// swap the frameCompFrames
		ps = ( short * )( ( byte * ) mdcSurf + mdcSurf->ofsFrameCompFrames );

		for ( j = 0; j < mdcModel->numFrames; j++, ps++ )
		{
			*ps = LittleShort( *ps );
		}

		surf->numVerts = mdcSurf->numVerts;
		surf->verts = v = ri.Hunk_Alloc( sizeof( *v ) * ( mdcSurf->numVerts * mdcModel->numFrames ), h_low );

		for ( j = 0; j < mdcModel->numFrames; j++ )
		{
			int baseFrame;
			int compFrame = 0;

			baseFrame = ( int ) * ( ( short * )( ( byte * ) mdcSurf + mdcSurf->ofsFrameBaseFrames ) + j );

			mdcxyz = ( md3XyzNormal_t * )( ( byte * ) mdcSurf + mdcSurf->ofsXyzNormals + baseFrame * mdcSurf->numVerts * sizeof( md3XyzNormal_t ) );

			if ( mdcSurf->numCompFrames > 0 )
			{
				compFrame = ( int ) * ( ( short * )( ( byte * ) mdcSurf + mdcSurf->ofsFrameCompFrames ) + j );

				if ( compFrame >= 0 )
				{
					mdcxyzComp = ( mdcXyzCompressed_t * )( ( byte * ) mdcSurf + mdcSurf->ofsXyzCompressed + compFrame * mdcSurf->numVerts * sizeof( mdcXyzCompressed_t ) );
				}
			}

			for ( k = 0; k < mdcSurf->numVerts; k++, v++, mdcxyz++ )
			{
				v->xyz[ 0 ] = LittleShort( mdcxyz->xyz[ 0 ] ) * MD3_XYZ_SCALE;
				v->xyz[ 1 ] = LittleShort( mdcxyz->xyz[ 1 ] ) * MD3_XYZ_SCALE;
				v->xyz[ 2 ] = LittleShort( mdcxyz->xyz[ 2 ] ) * MD3_XYZ_SCALE;

				if ( mdcSurf->numCompFrames > 0 && compFrame >= 0 )
				{
					vec3_t ofsVec;
					//vec3_t    normal;

					R_MDC_DecodeXyzCompressed2( LittleShort( mdcxyzComp->ofsVec ), ofsVec /*, normal*/ );
					VectorAdd( v->xyz, ofsVec, v->xyz );

					mdcxyzComp++;
				}
			}
		}

		// swap all the ST
		surf->st = st = ri.Hunk_Alloc( sizeof( *st ) * mdcSurf->numVerts, h_low );

		mdcst = ( md3St_t * )( ( byte * ) mdcSurf + mdcSurf->ofsSt );

		for ( j = 0; j < mdcSurf->numVerts; j++, mdcst++, st++ )
		{
			st->st[ 0 ] = LittleFloat( mdcst->st[ 0 ] );
			st->st[ 1 ] = LittleFloat( mdcst->st[ 1 ] );
		}

		// find the next surface
		mdcSurf = ( mdcSurface_t * )( ( byte * ) mdcSurf + mdcSurf->ofsEnd );
		surf++;
	}

#if 1
	// create VBO surfaces from md3 surfaces
	{
		mdvNormTanBi_t  *vertexes;
		mdvNormTanBi_t  *vert;

		growList_t      vboSurfaces;
		srfVBOMDVMesh_t *vboSurf;

		byte            *data;
		int             dataSize;
		int             dataOfs;

		vec4_t          tmp;

		GLuint          ofsTexCoords;
		GLuint          ofsTangents;
		GLuint          ofsBinormals;
		GLuint          ofsNormals;

		GLuint          sizeXYZ = 0;
		GLuint          sizeTangents = 0;
		GLuint          sizeBinormals = 0;
		GLuint          sizeNormals = 0;

		int             vertexesNum;
		int             f;

		Com_InitGrowList( &vboSurfaces, 10 );

		for ( i = 0, surf = mdvModel->surfaces; i < mdvModel->numSurfaces; i++, surf++ )
		{
			//allocate temp memory for vertex data
			vertexes = (mdvNormTanBi_t*)ri.Hunk_AllocateTempMemory( sizeof( *vertexes ) * surf->numVerts * mdvModel->numFrames );

			// calc tangent spaces
			{
				const float *v0, *v1, *v2;
				const float *t0, *t1, *t2;
				vec3_t      tangent;
				vec3_t      binormal;
				vec3_t      normal;

				for ( j = 0, vert = vertexes; j < ( surf->numVerts * mdvModel->numFrames ); j++, vert++ )
				{
					VectorClear( vert->tangent );
					VectorClear( vert->binormal );
					VectorClear( vert->normal );
				}

				for ( f = 0; f < mdvModel->numFrames; f++ )
				{
					for ( j = 0, tri = surf->triangles; j < surf->numTriangles; j++, tri++ )
					{
						v0 = surf->verts[ surf->numVerts * f + tri->indexes[ 0 ] ].xyz;
						v1 = surf->verts[ surf->numVerts * f + tri->indexes[ 1 ] ].xyz;
						v2 = surf->verts[ surf->numVerts * f + tri->indexes[ 2 ] ].xyz;

						t0 = surf->st[ tri->indexes[ 0 ] ].st;
						t1 = surf->st[ tri->indexes[ 1 ] ].st;
						t2 = surf->st[ tri->indexes[ 2 ] ].st;

#if 1
						R_CalcTangentSpace( tangent, binormal, normal, v0, v1, v2, t0, t1, t2 );
#else
						R_CalcNormalForTriangle( normal, v0, v1, v2 );
						R_CalcTangentsForTriangle( tangent, binormal, v0, v1, v2, t0, t1, t2 );
#endif

						for ( k = 0; k < 3; k++ )
						{
							float *v;

							v = vertexes[ surf->numVerts * f + tri->indexes[ k ] ].tangent;
							VectorAdd( v, tangent, v );

							v = vertexes[ surf->numVerts * f + tri->indexes[ k ] ].binormal;
							VectorAdd( v, binormal, v );

							v = vertexes[ surf->numVerts * f + tri->indexes[ k ] ].normal;
							VectorAdd( v, normal, v );
						}
					}
				}

				for ( j = 0, vert = vertexes; j < ( surf->numVerts * mdvModel->numFrames ); j++, vert++ )
				{
					VectorNormalize( vert->tangent );
					VectorNormalize( vert->binormal );
					VectorNormalize( vert->normal );
				}
			}

			//ri.Printf(PRINT_ALL, "...calculating MDC mesh VBOs ( '%s', %i verts %i tris )\n", surf->name, surf->numVerts, surf->numTriangles);

			// create surface
			vboSurf = ri.Hunk_Alloc( sizeof( *vboSurf ), h_low );
			Com_AddToGrowList( &vboSurfaces, vboSurf );

			vboSurf->surfaceType = SF_VBO_MDVMESH;
			vboSurf->mdvModel = mdvModel;
			vboSurf->mdvSurface = surf;
			vboSurf->numIndexes = surf->numTriangles * 3;
			vboSurf->numVerts = surf->numVerts;

			/*
			vboSurf->vbo = R_CreateVBO2(va("staticWorldMesh_vertices %i", vboSurfaces.currentElements), numVerts, optimizedVerts,
			                                                   ATTR_POSITION | ATTR_TEXCOORD | ATTR_LIGHTCOORD | ATTR_TANGENT | ATTR_BINORMAL | ATTR_NORMAL
			                                                   | ATTR_COLOR);
			                                                   */

			vboSurf->ibo = R_CreateIBO2( va( "staticMDCMesh_IBO %s", surf->name ), surf->numTriangles, surf->triangles, VBO_USAGE_STATIC );

			// create VBO
			vertexesNum = surf->numVerts;

			dataSize = ( surf->numVerts * mdvModel->numFrames * sizeof( vec4_t ) * 4 ) +  // xyz, tangent, binormal, normal
			           ( surf->numVerts * sizeof( vec4_t ) );  // texcoords
			data = ri.Hunk_AllocateTempMemory( dataSize );
			dataOfs = 0;

			// feed vertex XYZ
			for ( f = 0; f < mdvModel->numFrames; f++ )
			{
				for ( j = 0; j < vertexesNum; j++ )
				{
					for ( k = 0; k < 3; k++ )
					{
						tmp[ k ] = surf->verts[ f * vertexesNum + j ].xyz[ k ];
					}

					tmp[ 3 ] = 1;
					Com_Memcpy( data + dataOfs, ( float * ) tmp, sizeof( vec4_t ) );
					dataOfs += sizeof( vec4_t );
				}

				if ( f == 0 )
				{
					sizeXYZ = dataOfs;
				}
			}

			// feed vertex texcoords
			ofsTexCoords = dataOfs;

			for ( j = 0; j < vertexesNum; j++ )
			{
				for ( k = 0; k < 2; k++ )
				{
					tmp[ k ] = surf->st[ j ].st[ k ];
				}

				tmp[ 2 ] = 0;
				tmp[ 3 ] = 1;
				Com_Memcpy( data + dataOfs, ( float * ) tmp, sizeof( vec4_t ) );
				dataOfs += sizeof( vec4_t );
			}

			// feed vertex tangents
			ofsTangents = dataOfs;

			for ( f = 0; f < mdvModel->numFrames; f++ )
			{
				for ( j = 0; j < vertexesNum; j++ )
				{
					for ( k = 0; k < 3; k++ )
					{
						tmp[ k ] = vertexes[ f * vertexesNum + j ].tangent[ k ];
					}

					tmp[ 3 ] = 1;
					Com_Memcpy( data + dataOfs, ( float * ) tmp, sizeof( vec4_t ) );
					dataOfs += sizeof( vec4_t );
				}

				if ( f == 0 )
				{
					sizeTangents = dataOfs - ofsTangents;
				}
			}

			// feed vertex binormals
			ofsBinormals = dataOfs;

			for ( f = 0; f < mdvModel->numFrames; f++ )
			{
				for ( j = 0; j < vertexesNum; j++ )
				{
					for ( k = 0; k < 3; k++ )
					{
						tmp[ k ] = vertexes[ f * vertexesNum + j ].binormal[ k ];
					}

					tmp[ 3 ] = 1;
					Com_Memcpy( data + dataOfs, ( float * ) tmp, sizeof( vec4_t ) );
					dataOfs += sizeof( vec4_t );
				}

				if ( f == 0 )
				{
					sizeBinormals = dataOfs - ofsBinormals;
				}
			}

			// feed vertex normals
			ofsNormals = dataOfs;

			for ( f = 0; f < mdvModel->numFrames; f++ )
			{
				for ( j = 0; j < vertexesNum; j++ )
				{
					for ( k = 0; k < 3; k++ )
					{
						tmp[ k ] = vertexes[ f * vertexesNum + j ].normal[ k ];
					}

					tmp[ 3 ] = 1;
					Com_Memcpy( data + dataOfs, ( float * ) tmp, sizeof( vec4_t ) );
					dataOfs += sizeof( vec4_t );
				}

				if ( f == 0 )
				{
					sizeNormals = dataOfs - ofsNormals;
				}
			}

			vboSurf->vbo = R_CreateVBO( va( "staticMDCMesh_VBO '%s'", surf->name ), data, dataSize, VBO_USAGE_STATIC );
			vboSurf->vbo->ofsXYZ = 0;
			vboSurf->vbo->ofsTexCoords = ofsTexCoords;
			vboSurf->vbo->ofsLightCoords = ofsTexCoords;
			vboSurf->vbo->ofsTangents = ofsTangents;
			vboSurf->vbo->ofsBinormals = ofsBinormals;
			vboSurf->vbo->ofsNormals = ofsNormals;

			vboSurf->vbo->sizeXYZ = sizeXYZ;
			vboSurf->vbo->sizeTangents = sizeTangents;
			vboSurf->vbo->sizeBinormals = sizeBinormals;
			vboSurf->vbo->sizeNormals = sizeNormals;

			ri.Hunk_FreeTempMemory( data );
			ri.Hunk_FreeTempMemory( vertexes );
		}

		// move VBO surfaces list to hunk
		mdvModel->numVBOSurfaces = vboSurfaces.currentElements;
		mdvModel->vboSurfaces = ri.Hunk_Alloc( mdvModel->numVBOSurfaces * sizeof( *mdvModel->vboSurfaces ), h_low );

		for ( i = 0; i < mdvModel->numVBOSurfaces; i++ )
		{
			mdvModel->vboSurfaces[ i ] = ( srfVBOMDVMesh_t * ) Com_GrowListElement( &vboSurfaces, i );
		}

		Com_DestroyGrowList( &vboSurfaces );
	}
#endif

	return qtrue;
}
