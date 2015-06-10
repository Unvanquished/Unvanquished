/*
===========================================================================
Copyright (C) 2006 Kirk Barnes
Copyright (C) 2006-2008 Robert Beckebans <trebor_7@users.sourceforge.net>

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
// tr_fbo.c
#include "tr_local.h"

/*
=============
R_CheckFBO
=============
*/
bool R_CheckFBO( const FBO_t *fbo )
{
	int code;
	int id;

	glGetIntegerv( GL_FRAMEBUFFER_BINDING, &id );
	glBindFramebuffer( GL_FRAMEBUFFER, fbo->frameBuffer );

	code = glCheckFramebufferStatus( GL_FRAMEBUFFER );

	if ( code == GL_FRAMEBUFFER_COMPLETE )
	{
		glBindFramebuffer( GL_FRAMEBUFFER, id );
		return true;
	}

	// an error occurred
	switch ( code )
	{
		case GL_FRAMEBUFFER_COMPLETE:
			break;

		case GL_FRAMEBUFFER_UNSUPPORTED:
			ri.Printf( PRINT_WARNING, "R_CheckFBO: (%s) Unsupported framebuffer format\n", fbo->name );
			break;

		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
			ri.Printf( PRINT_WARNING, "R_CheckFBO: (%s) Framebuffer incomplete attachment\n", fbo->name );
			break;

		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
			ri.Printf( PRINT_WARNING, "R_CheckFBO: (%s) Framebuffer incomplete, missing attachment\n", fbo->name );
			break;

		case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
			ri.Printf( PRINT_WARNING, "R_CheckFBO: (%s) Framebuffer incomplete, missing draw buffer\n", fbo->name );
			break;

		case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
			ri.Printf( PRINT_WARNING, "R_CheckFBO: (%s) Framebuffer incomplete, missing read buffer\n", fbo->name );
			break;

		default:
			ri.Printf( PRINT_WARNING, "R_CheckFBO: (%s) unknown error 0x%X\n", fbo->name, code );
			break;
	}

	glBindFramebuffer( GL_FRAMEBUFFER, id );

	return false;
}

/*
============
R_CreateFBO
============
*/
FBO_t          *R_CreateFBO( const char *name, int width, int height )
{
	FBO_t *fbo;

	if ( strlen( name ) >= MAX_QPATH )
	{
		ri.Error( ERR_DROP, "R_CreateFBO: \"%s\" is too long", name );
	}

	if ( width <= 0 || width > glConfig2.maxRenderbufferSize )
	{
		ri.Error( ERR_DROP, "R_CreateFBO: bad width %i", width );
	}

	if ( height <= 0 || height > glConfig2.maxRenderbufferSize )
	{
		ri.Error( ERR_DROP, "R_CreateFBO: bad height %i", height );
	}

	if ( tr.numFBOs == MAX_FBOS )
	{
		ri.Error( ERR_DROP, "R_CreateFBO: MAX_FBOS hit" );
	}

	fbo = tr.fbos[ tr.numFBOs ] = (FBO_t*) ri.Hunk_Alloc( sizeof( *fbo ), h_low );
	Q_strncpyz( fbo->name, name, sizeof( fbo->name ) );
	fbo->index = tr.numFBOs++;
	fbo->width = width;
	fbo->height = height;

	glGenFramebuffers( 1, &fbo->frameBuffer );

	return fbo;
}

/*
================
R_CreateFBOColorBuffer

Framebuffer must be bound
================
*/
void R_CreateFBOColorBuffer( FBO_t *fbo, int format, int index )
{
	bool absent;

	if ( index < 0 || index >= glConfig2.maxColorAttachments )
	{
		ri.Printf( PRINT_WARNING, "R_CreateFBOColorBuffer: invalid attachment index %i\n", index );
		return;
	}

	fbo->colorFormat = format;

	absent = fbo->colorBuffers[ index ] == 0;

	if ( absent )
	{
		glGenRenderbuffers( 1, &fbo->colorBuffers[ index ] );
	}

	glBindRenderbuffer( GL_RENDERBUFFER, fbo->colorBuffers[ index ] );
	glRenderbufferStorage( GL_RENDERBUFFER, format, fbo->width, fbo->height );

	if ( absent )
	{
		glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, GL_RENDERBUFFER,
					   fbo->colorBuffers[ index ] );
	}

	GL_CheckErrors();
}

/*
================
R_CreateFBODepthBuffer
================
*/
void R_CreateFBODepthBuffer( FBO_t *fbo, int format )
{
	bool absent;

	if ( format != GL_DEPTH_COMPONENT &&
	     format != GL_DEPTH_COMPONENT16 && format != GL_DEPTH_COMPONENT24 && format != GL_DEPTH_COMPONENT32_ARB )
	{
		ri.Printf( PRINT_WARNING, "R_CreateFBODepthBuffer: format %i is not depth-renderable\n", format );
		return;
	}

	fbo->depthFormat = format;

	absent = fbo->depthBuffer == 0;

	if ( absent )
	{
		glGenRenderbuffers( 1, &fbo->depthBuffer );
	}

	glBindRenderbuffer( GL_RENDERBUFFER, fbo->depthBuffer );
	glRenderbufferStorage( GL_RENDERBUFFER, fbo->depthFormat, fbo->width, fbo->height );

	if ( absent )
	{
		glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, fbo->depthBuffer );
	}

	GL_CheckErrors();
}

/*
================
R_CreateFBOStencilBuffer
================
*/
void R_CreateFBOStencilBuffer( FBO_t *fbo, int format )
{
	bool absent;

	if ( format != GL_STENCIL_INDEX &&
	     format != GL_STENCIL_INDEX1_EXT &&
	     format != GL_STENCIL_INDEX4_EXT && format != GL_STENCIL_INDEX8_EXT && format != GL_STENCIL_INDEX16_EXT )
	{
		ri.Printf( PRINT_WARNING, "R_CreateFBOStencilBuffer: format %i is not stencil-renderable\n", format );
		return;
	}

	fbo->stencilFormat = format;

	absent = fbo->stencilBuffer == 0;

	if ( absent )
	{
		glGenRenderbuffers( 1, &fbo->stencilBuffer );
	}

	glBindRenderbuffer( GL_RENDERBUFFER, fbo->stencilBuffer );
	glRenderbufferStorage( GL_RENDERBUFFER, fbo->stencilFormat, fbo->width, fbo->height );
	GL_CheckErrors();

	if ( absent )
	{
		glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, fbo->stencilBuffer );
	}

	GL_CheckErrors();
}

/*
================
R_CreateFBOPackedDepthStencilBuffer
================
*/
void R_CreateFBOPackedDepthStencilBuffer( FBO_t *fbo, int format )
{
	bool absent;

	if ( format != GL_DEPTH_STENCIL_EXT && format != GL_DEPTH24_STENCIL8_EXT )
	{
		ri.Printf( PRINT_WARNING, "R_CreateFBOPackedDepthStencilBuffer: format %i is not depth-stencil-renderable\n", format );
		return;
	}

	fbo->packedDepthStencilFormat = format;

	absent = fbo->packedDepthStencilBuffer == 0;

	if ( absent )
	{
		glGenRenderbuffers( 1, &fbo->packedDepthStencilBuffer );
	}

	glBindRenderbuffer( GL_RENDERBUFFER, fbo->packedDepthStencilBuffer );
	glRenderbufferStorage( GL_RENDERBUFFER, fbo->packedDepthStencilFormat, fbo->width, fbo->height );
	GL_CheckErrors();

	if ( absent )
	{
		glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER,
					   fbo->packedDepthStencilBuffer );
		glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
					   fbo->packedDepthStencilBuffer );
	}

	GL_CheckErrors();
}

/*
=================
R_AttachFBOTexture1D
=================
*/
void R_AttachFBOTexture1D( int texId, int index )
{
	if ( index < 0 || index >= glConfig2.maxColorAttachments )
	{
		ri.Printf( PRINT_WARNING, "R_AttachFBOTexture1D: invalid attachment index %i\n", index );
		return;
	}

	glFramebufferTexture1D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, GL_TEXTURE_1D, texId, 0 );
}

/*
=================
R_AttachFBOTexture2D
=================
*/
void R_AttachFBOTexture2D( int target, int texId, int index )
{
	if ( target != GL_TEXTURE_2D && ( target < GL_TEXTURE_CUBE_MAP_POSITIVE_X || target > GL_TEXTURE_CUBE_MAP_NEGATIVE_Z ) )
	{
		ri.Printf( PRINT_WARNING, "R_AttachFBOTexture2D: invalid target %i\n", target );
		return;
	}

	if ( index < 0 || index >= glConfig2.maxColorAttachments )
	{
		ri.Printf( PRINT_WARNING, "R_AttachFBOTexture2D: invalid attachment index %i\n", index );
		return;
	}

	glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, target, texId, 0 );
}

/*
=================
R_AttachFBOTexture3D
=================
*/
void R_AttachFBOTexture3D( int texId, int index, int zOffset )
{
	if ( index < 0 || index >= glConfig2.maxColorAttachments )
	{
		ri.Printf( PRINT_WARNING, "R_AttachFBOTexture3D: invalid attachment index %i\n", index );
		return;
	}

	glFramebufferTexture3D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, GL_TEXTURE_3D, texId, 0, zOffset );
}

/*
=================
R_AttachFBOTextureDepth
=================
*/
void R_AttachFBOTextureDepth( int texId )
{
	glFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texId, 0 );
}

/*
=================
R_AttachFBOTexturePackedDepthStencil
=================
*/
void R_AttachFBOTexturePackedDepthStencil( int texId )
{
	glFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texId, 0 );
	glFramebufferTexture2D( GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, texId, 0 );
}

/*
============
R_BindFBO
============
*/
void R_BindFBO( FBO_t *fbo )
{
	if ( !fbo )
	{
		R_BindNullFBO();
		return;
	}

	if ( r_logFile->integer )
	{
		// don't just call LogComment, or we will get a call to va() every frame!
		GLimp_LogComment( va( "--- R_BindFBO( %s ) ---\n", fbo->name ) );
	}

	if ( glState.currentFBO != fbo )
	{
		glBindFramebuffer( GL_FRAMEBUFFER, fbo->frameBuffer );

		glState.currentFBO = fbo;
	}
}

/*
============
R_BindNullFBO
============
*/
void R_BindNullFBO()
{
	if ( r_logFile->integer )
	{
		GLimp_LogComment( "--- R_BindNullFBO ---\n" );
	}

	if ( glState.currentFBO )
	{
		glBindFramebuffer( GL_FRAMEBUFFER, 0 );
		glBindRenderbuffer( GL_RENDERBUFFER, 0 );
		glState.currentFBO = nullptr;
	}
}

/*
============
R_InitFBOs
============
*/
void R_InitFBOs()
{
	int i;
	int width, height;

	ri.Printf( PRINT_DEVELOPER, "------- R_InitFBOs -------\n" );

	if ( !glConfig2.framebufferObjectAvailable )
	{
		return;
	}

	tr.numFBOs = 0;

	GL_CheckErrors();

	// make sure the render thread is stopped
	R_SyncRenderThread();

	if ( glConfig2.textureNPOTAvailable )
	{
		width = glConfig.vidWidth;
		height = glConfig.vidHeight;
	}
	else
	{
		width = NearestPowerOfTwo( glConfig.vidWidth );
		height = NearestPowerOfTwo( glConfig.vidHeight );
	}

	tr.occlusionRenderFBO = R_CreateFBO( "_occlusionRender", width, height );
	R_BindFBO( tr.occlusionRenderFBO );

	if ( glConfig.hardwareType == GLHW_ATI_DX10 )
	{
		R_CreateFBODepthBuffer( tr.occlusionRenderFBO, GL_DEPTH_COMPONENT16 );
	}
	else if ( glConfig.hardwareType == GLHW_NV_DX10 )
	{
		R_CreateFBODepthBuffer( tr.occlusionRenderFBO, GL_DEPTH_COMPONENT24 );
	}
	else if ( glConfig2.framebufferPackedDepthStencilAvailable )
	{
		R_CreateFBOPackedDepthStencilBuffer( tr.occlusionRenderFBO, GL_DEPTH24_STENCIL8_EXT );
	}
	else
	{
		R_CreateFBODepthBuffer( tr.occlusionRenderFBO, GL_DEPTH_COMPONENT24 );
	}

	R_AttachFBOTexture2D( GL_TEXTURE_2D, tr.occlusionRenderFBOImage->texnum, 0 );

	R_CheckFBO( tr.occlusionRenderFBO );

	if ( r_shadows->integer >= SHADOWING_ESM16 && glConfig2.textureFloatAvailable )
	{
		// shadowMap FBOs for shadow mapping offscreen rendering
		for ( i = 0; i < MAX_SHADOWMAPS; i++ )
		{
			width = height = shadowMapResolutions[ i ];

			tr.shadowMapFBO[ i ] = R_CreateFBO( va( "_shadowMap%d", i ), width, height );
			R_BindFBO( tr.shadowMapFBO[ i ] );
			R_AttachFBOTexture2D( GL_TEXTURE_2D,
					      tr.shadowMapFBOImage[ i ]->texnum,
					      0 );

			R_CreateFBODepthBuffer( tr.shadowMapFBO[ i ], GL_DEPTH_COMPONENT24 );

			R_CheckFBO( tr.shadowMapFBO[ i ] );
		}

		// sun requires different resolutions
		for ( i = 0; i < MAX_SHADOWMAPS; i++ )
		{
			width = height = sunShadowMapResolutions[ i ];

			tr.sunShadowMapFBO[ i ] = R_CreateFBO( va( "_sunShadowMap%d", i ), width, height );
			R_BindFBO( tr.sunShadowMapFBO[ i ] );
			R_AttachFBOTexture2D( GL_TEXTURE_2D,
					      tr.sunShadowMapFBOImage[ i ]->texnum,
					      0 );

			R_CreateFBODepthBuffer( tr.sunShadowMapFBO[ i ], GL_DEPTH_COMPONENT24 );

			if ( r_shadows->integer == SHADOWING_EVSM32 && r_evsmPostProcess->integer )
			{
				R_AttachFBOTextureDepth( tr.sunShadowMapFBOImage[ i ]->texnum );

				/*
				Since we don't have a color attachment, the framebuffer will be considered incomplete.
				Consequently, we must inform the driver that we do not wish to render to the color buffer.
				We do this with a call to set the draw-buffer and read-buffer to GL_NONE:
				*/
				glDrawBuffer( GL_NONE );
				glReadBuffer( GL_NONE );
			}

			R_CheckFBO( tr.sunShadowMapFBO[ i ] );
		}
	}

	{
		if ( glConfig2.textureNPOTAvailable )
		{
			width = glConfig.vidWidth;
			height = glConfig.vidHeight;
		}
		else
		{
			width = NearestPowerOfTwo( glConfig.vidWidth );
			height = NearestPowerOfTwo( glConfig.vidHeight );
		}

		// portalRender FBO for portals, mirrors, water, cameras et cetera
		tr.portalRenderFBO = R_CreateFBO( "_portalRender", width, height );
		R_BindFBO( tr.portalRenderFBO );

		R_AttachFBOTexture2D( GL_TEXTURE_2D, tr.portalRenderImage->texnum, 0 );

		R_CheckFBO( tr.portalRenderFBO );
	}

	{
		if ( glConfig2.textureNPOTAvailable )
		{
			width = glConfig.vidWidth * 0.25f;
			height = glConfig.vidHeight * 0.25f;
		}
		else
		{
			width = NearestPowerOfTwo( glConfig.vidWidth * 0.25f );
			height = NearestPowerOfTwo( glConfig.vidHeight * 0.25f );
		}

		tr.downScaleFBO_quarter = R_CreateFBO( "_downScale_quarter", width, height );
		R_BindFBO( tr.downScaleFBO_quarter );

		R_AttachFBOTexture2D( GL_TEXTURE_2D, tr.downScaleFBOImage_quarter->texnum, 0 );
		R_CheckFBO( tr.downScaleFBO_quarter );

		tr.downScaleFBO_64x64 = R_CreateFBO( "_downScale_64x64", 64, 64 );
		R_BindFBO( tr.downScaleFBO_64x64 );

		R_AttachFBOTexture2D( GL_TEXTURE_2D, tr.downScaleFBOImage_64x64->texnum, 0 );
		R_CheckFBO( tr.downScaleFBO_64x64 );

		if ( glConfig2.textureNPOTAvailable )
		{
			width = glConfig.vidWidth * 0.25f;
			height = glConfig.vidHeight * 0.25f;
		}
		else
		{
			width = NearestPowerOfTwo( glConfig.vidWidth * 0.25f );
			height = NearestPowerOfTwo( glConfig.vidHeight * 0.25f );
		}

		tr.contrastRenderFBO = R_CreateFBO( "_contrastRender", width, height );
		R_BindFBO( tr.contrastRenderFBO );

		R_AttachFBOTexture2D( GL_TEXTURE_2D, tr.contrastRenderFBOImage->texnum, 0 );

		R_CheckFBO( tr.contrastRenderFBO );

		for ( i = 0; i < 2; i++ )
		{
			tr.bloomRenderFBO[ i ] = R_CreateFBO( va( "_bloomRender%d", i ), width, height );
			R_BindFBO( tr.bloomRenderFBO[ i ] );

			R_AttachFBOTexture2D( GL_TEXTURE_2D, tr.bloomRenderFBOImage[ i ]->texnum, 0 );

			R_CheckFBO( tr.bloomRenderFBO[ i ] );
		}
	}

	GL_CheckErrors();

	R_BindNullFBO();
}

/*
============
R_ShutdownFBOs
============
*/
void R_ShutdownFBOs()
{
	int   i, j;
	FBO_t *fbo;

	ri.Printf( PRINT_DEVELOPER, "------- R_ShutdownFBOs -------\n" );

	if ( !glConfig2.framebufferObjectAvailable )
	{
		return;
	}

	R_BindNullFBO();

	for ( i = 0; i < tr.numFBOs; i++ )
	{
		fbo = tr.fbos[ i ];

		for ( j = 0; j < glConfig2.maxColorAttachments; j++ )
		{
			if ( fbo->colorBuffers[ j ] )
			{
				glDeleteRenderbuffers( 1, &fbo->colorBuffers[ j ] );
			}
		}

		if ( fbo->depthBuffer )
		{
			glDeleteRenderbuffers( 1, &fbo->depthBuffer );
		}

		if ( fbo->stencilBuffer )
		{
			glDeleteRenderbuffers( 1, &fbo->stencilBuffer );
		}

		if ( fbo->frameBuffer )
		{
			glDeleteFramebuffers( 1, &fbo->frameBuffer );
		}
	}
}

/*
============
R_FBOList_f
============
*/
void R_FBOList_f()
{
	int   i;
	FBO_t *fbo;

	if ( !glConfig2.framebufferObjectAvailable )
	{
		ri.Printf( PRINT_ALL, "GL_ARB_framebuffer_object is not available.\n" );
		return;
	}

	ri.Printf( PRINT_ALL, "             size       name\n" );
	ri.Printf( PRINT_ALL, "----------------------------------------------------------\n" );

	for ( i = 0; i < tr.numFBOs; i++ )
	{
		fbo = tr.fbos[ i ];

		ri.Printf( PRINT_ALL, "  %4i: %4i %4i %s\n", i, fbo->width, fbo->height, fbo->name );
	}

	ri.Printf( PRINT_ALL, " %i FBOs\n", tr.numFBOs );
}
