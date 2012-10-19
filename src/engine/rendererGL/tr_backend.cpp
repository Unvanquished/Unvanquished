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
// tr_backend.c

#include "tr_local.h"
#include "gl_shader.h"

backEndData_t  *backEndData[ SMP_FRAMES ];
backEndState_t backEnd;

void GL_Bind( image_t *image )
{
	int texnum;

	if ( !image )
	{
		ri.Printf( PRINT_WARNING, "GL_Bind: NULL image\n" );
		image = tr.defaultImage;
	}
	else
	{
		if ( r_logFile->integer )
		{
			// don't just call LogComment, or we will get a call to va() every frame!
			GLimp_LogComment( va( "--- GL_Bind( %s ) ---\n", image->name ) );
		}
	}

	texnum = image->texnum;

	if ( r_nobind->integer && tr.blackImage )
	{
		// performance evaluation option
		texnum = tr.blackImage->texnum;
	}

	if ( glState.currenttextures[ glState.currenttmu ] != texnum )
	{
		image->frameUsed = tr.frameCount;
		glState.currenttextures[ glState.currenttmu ] = texnum;
		glBindTexture( image->type, texnum );
	}
}

void GL_Unbind()
{
	GLimp_LogComment( "--- GL_Unbind() ---\n" );

	glBindTexture( GL_TEXTURE_2D, 0 );
}

void BindAnimatedImage( textureBundle_t *bundle )
{
	int index;

	if ( bundle->isVideoMap )
	{
		ri.CIN_RunCinematic( bundle->videoMapHandle );
		ri.CIN_UploadCinematic( bundle->videoMapHandle );
		return;
	}

	if ( bundle->numImages <= 1 )
	{
		GL_Bind( bundle->image[ 0 ] );
		return;
	}

	// it is necessary to do this messy calc to make sure animations line up
	// exactly with waveforms of the same frequency
	index = XreaL_Q_ftol( backEnd.refdef.floatTime * bundle->imageAnimationSpeed * FUNCTABLE_SIZE );
	index >>= FUNCTABLE_SIZE2;

	if ( index < 0 )
	{
		index = 0; // may happen with shader time offsets
	}

	index %= bundle->numImages;

	GL_Bind( bundle->image[ index ] );
}

void GL_TextureFilter( image_t *image, filterType_t filterType )
{
	if ( !image )
	{
		ri.Printf( PRINT_WARNING, "GL_TextureFilter: NULL image\n" );
	}
	else
	{
		if ( r_logFile->integer )
		{
			// don't just call LogComment, or we will get a call to va() every frame!
			GLimp_LogComment( va( "--- GL_TextureFilter( %s ) ---\n", image->name ) );
		}
	}

	if ( image->filterType == filterType )
	{
		return;
	}

	// set filter type
	switch ( image->filterType )
	{
			/*
			   case FT_DEFAULT:
			   glTexParameterf(image->type, GL_TEXTURE_MIN_FILTER, gl_filter_min);
			   glTexParameterf(image->type, GL_TEXTURE_MAG_FILTER, gl_filter_max);

			   // set texture anisotropy
			   if(glConfig2.textureAnisotropyAvailable)
			   glTexParameterf(image->type, GL_TEXTURE_MAX_ANISOTROPY_EXT, r_ext_texture_filter_anisotropic->value);
			   break;
			 */

		case FT_LINEAR:
			glTexParameterf( image->type, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
			glTexParameterf( image->type, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
			break;

		case FT_NEAREST:
			glTexParameterf( image->type, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
			glTexParameterf( image->type, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
			break;

		default:
			break;
	}
}

void GL_BindProgram( shaderProgram_t *program )
{
	if ( !program )
	{
		GL_BindNullProgram();
		return;
	}

	if ( r_logFile->integer )
	{
		// don't just call LogComment, or we will get a call to va() every frame!
		GLimp_LogComment( va( "--- GL_BindProgram( name = '%s', macros = '%s' ) ---\n", program->name, program->compileMacros ) );
	}

	if ( glState.currentProgram != program )
	{
		glUseProgram( program->program );
		glState.currentProgram = program;
	}
}

void GL_BindNullProgram( void )
{
	if ( r_logFile->integer )
	{
		GLimp_LogComment( "--- GL_BindNullProgram ---\n" );
	}

	if ( glState.currentProgram )
	{
		glUseProgram( 0 );
		glState.currentProgram = NULL;
	}
}

void GL_SelectTexture( int unit )
{
	if ( glState.currenttmu == unit )
	{
		return;
	}

	if ( unit >= 0 && unit <= 31 )
	{
		glActiveTexture( GL_TEXTURE0 + unit );

		if ( r_logFile->integer )
		{
			GLimp_LogComment( va( "glActiveTexture( GL_TEXTURE%i )\n", unit ) );
		}
	}
	else
	{
		ri.Error( ERR_DROP, "GL_SelectTexture: unit = %i", unit );
	}

	glState.currenttmu = unit;
}

void GL_BlendFunc( GLenum sfactor, GLenum dfactor )
{
	if ( glState.blendSrc != ( signed ) sfactor || glState.blendDst != ( signed ) dfactor )
	{
		glState.blendSrc = sfactor;
		glState.blendDst = dfactor;

		glBlendFunc( sfactor, dfactor );
	}
}

void GL_ClearColor( GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha )
{
	if ( glState.clearColorRed != red || glState.clearColorGreen != green || glState.clearColorBlue != blue || glState.clearColorAlpha != alpha )
	{
		glState.clearColorRed = red;
		glState.clearColorGreen = green;
		glState.clearColorBlue = blue;
		glState.clearColorAlpha = alpha;

		glClearColor( red, green, blue, alpha );
	}
}

void GL_ClearDepth( GLclampd depth )
{
	if ( glState.clearDepth != depth )
	{
		glState.clearDepth = depth;

		glClearDepth( depth );
	}
}

void GL_ClearStencil( GLint s )
{
	if ( glState.clearStencil != s )
	{
		glState.clearStencil = s;

		glClearStencil( s );
	}
}

void GL_ColorMask( GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha )
{
	if ( glState.colorMaskRed != red || glState.colorMaskGreen != green || glState.colorMaskBlue != blue || glState.colorMaskAlpha != alpha )
	{
		glState.colorMaskRed = red;
		glState.colorMaskGreen = green;
		glState.colorMaskBlue = blue;
		glState.colorMaskAlpha = alpha;

		glColorMask( red, green, blue, alpha );
	}
}

void GL_CullFace( GLenum mode )
{
	if ( glState.cullFace != ( signed ) mode )
	{
		glState.cullFace = mode;

		glCullFace( mode );
	}
}

void GL_DepthFunc( GLenum func )
{
	if ( glState.depthFunc != ( signed ) func )
	{
		glState.depthFunc = func;

		glDepthFunc( func );
	}
}

void GL_DepthMask( GLboolean flag )
{
	if ( glState.depthMask != flag )
	{
		glState.depthMask = flag;

		glDepthMask( flag );
	}
}

void GL_DrawBuffer( GLenum mode )
{
	if ( glState.drawBuffer != ( signed ) mode )
	{
		glState.drawBuffer = mode;

		glDrawBuffer( mode );
	}
}

void GL_FrontFace( GLenum mode )
{
	if ( glState.frontFace != ( signed ) mode )
	{
		glState.frontFace = mode;

		glFrontFace( mode );
	}
}

void GL_LoadModelViewMatrix( const matrix_t m )
{
#if 1

	if ( MatrixCompare( glState.modelViewMatrix[ glState.stackIndex ], m ) )
	{
		return;
	}

#endif

	MatrixCopy( m, glState.modelViewMatrix[ glState.stackIndex ] );
	MatrixMultiply( glState.projectionMatrix[ glState.stackIndex ], glState.modelViewMatrix[ glState.stackIndex ],
	                glState.modelViewProjectionMatrix[ glState.stackIndex ] );
}

void GL_LoadProjectionMatrix( const matrix_t m )
{
#if 1

	if ( MatrixCompare( glState.projectionMatrix[ glState.stackIndex ], m ) )
	{
		return;
	}

#endif

	MatrixCopy( m, glState.projectionMatrix[ glState.stackIndex ] );
	MatrixMultiply( glState.projectionMatrix[ glState.stackIndex ], glState.modelViewMatrix[ glState.stackIndex ],
	                glState.modelViewProjectionMatrix[ glState.stackIndex ] );
}

void GL_PushMatrix()
{
	glState.stackIndex++;

	if ( glState.stackIndex >= MAX_GLSTACK )
	{
		glState.stackIndex = MAX_GLSTACK - 1;
		ri.Error( ERR_DROP, "GL_PushMatrix: stack overflow = %i", glState.stackIndex );
	}
}

void GL_PopMatrix()
{
	glState.stackIndex--;

	if ( glState.stackIndex < 0 )
	{
		glState.stackIndex = 0;
		ri.Error( ERR_DROP, "GL_PushMatrix: stack underflow" );
	}
}

void GL_PolygonMode( GLenum face, GLenum mode )
{
	if ( glState.polygonFace != ( signed ) face || glState.polygonMode != ( signed ) mode )
	{
		glState.polygonFace = face;
		glState.polygonMode = mode;

		glPolygonMode( face, mode );
	}
}

void GL_Scissor( GLint x, GLint y, GLsizei width, GLsizei height )
{
	if ( glState.scissorX != x || glState.scissorY != y || glState.scissorWidth != width || glState.scissorHeight != height )
	{
		glState.scissorX = x;
		glState.scissorY = y;
		glState.scissorWidth = width;
		glState.scissorHeight = height;

		glScissor( x, y, width, height );
	}
}

void GL_Viewport( GLint x, GLint y, GLsizei width, GLsizei height )
{
	if ( glState.viewportX != x || glState.viewportY != y || glState.viewportWidth != width || glState.viewportHeight != height )
	{
		glState.viewportX = x;
		glState.viewportY = y;
		glState.viewportWidth = width;
		glState.viewportHeight = height;

		glViewport( x, y, width, height );
	}
}

void GL_PolygonOffset( float factor, float units )
{
	if ( glState.polygonOffsetFactor != factor || glState.polygonOffsetUnits != units )
	{
		glState.polygonOffsetFactor = factor;
		glState.polygonOffsetUnits = units;

		glPolygonOffset( factor, units );
	}
}

void GL_Cull( int cullType )
{
	if ( glState.faceCulling == cullType )
	{
		return;
	}

#if 1

	if ( cullType == CT_TWO_SIDED )
	{
		glDisable( GL_CULL_FACE );
	}
	else
	{
		if( glState.faceCulling == CT_TWO_SIDED )
			glEnable( GL_CULL_FACE );

		if ( cullType == CT_BACK_SIDED )
		{
			GL_CullFace( GL_BACK );

			if ( backEnd.viewParms.isMirror )
			{
				GL_FrontFace( GL_CW );
			}
			else
			{
				GL_FrontFace( GL_CCW );
			}
		}
		else
		{
			GL_CullFace( GL_FRONT );

			if ( backEnd.viewParms.isMirror )
			{
				GL_FrontFace( GL_CW );
			}
			else
			{
				GL_FrontFace( GL_CCW );
			}
		}
	}
	glState.faceCulling = cullType;
#else
	glState.faceCulling = CT_TWO_SIDED;
	glDisable( GL_CULL_FACE );
#endif
}

/*
GL_State

This routine is responsible for setting the most commonly changed state
in Q3.
*/
void GL_State( uint32_t stateBits )
{
	uint32_t diff = stateBits ^ glState.glStateBits;

	if ( !diff )
	{
		return;
	}

	// check depthFunc bits
	if ( diff & GLS_DEPTHFUNC_BITS )
	{
		switch ( stateBits & GLS_DEPTHFUNC_BITS )
		{
			default:
				GL_DepthFunc( GL_LEQUAL );
				break;

			case GLS_DEPTHFUNC_LESS:
				GL_DepthFunc( GL_LESS );
				break;

			case GLS_DEPTHFUNC_EQUAL:
				GL_DepthFunc( GL_EQUAL );
				break;
		}
	}

	// check blend bits
	if ( diff & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS ) )
	{
		GLenum srcFactor, dstFactor;

		if ( stateBits & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS ) )
		{
			switch ( stateBits & GLS_SRCBLEND_BITS )
			{
				case GLS_SRCBLEND_ZERO:
					srcFactor = GL_ZERO;
					break;

				case GLS_SRCBLEND_ONE:
					srcFactor = GL_ONE;
					break;

				case GLS_SRCBLEND_DST_COLOR:
					srcFactor = GL_DST_COLOR;
					break;

				case GLS_SRCBLEND_ONE_MINUS_DST_COLOR:
					srcFactor = GL_ONE_MINUS_DST_COLOR;
					break;

				case GLS_SRCBLEND_SRC_ALPHA:
					srcFactor = GL_SRC_ALPHA;
					break;

				case GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA:
					srcFactor = GL_ONE_MINUS_SRC_ALPHA;
					break;

				case GLS_SRCBLEND_DST_ALPHA:
					srcFactor = GL_DST_ALPHA;
					break;

				case GLS_SRCBLEND_ONE_MINUS_DST_ALPHA:
					srcFactor = GL_ONE_MINUS_DST_ALPHA;
					break;

				case GLS_SRCBLEND_ALPHA_SATURATE:
					srcFactor = GL_SRC_ALPHA_SATURATE;
					break;

				default:
					srcFactor = GL_ONE; // to get warning to shut up
					ri.Error( ERR_DROP, "GL_State: invalid src blend state bits" );
			}

			switch ( stateBits & GLS_DSTBLEND_BITS )
			{
				case GLS_DSTBLEND_ZERO:
					dstFactor = GL_ZERO;
					break;

				case GLS_DSTBLEND_ONE:
					dstFactor = GL_ONE;
					break;

				case GLS_DSTBLEND_SRC_COLOR:
					dstFactor = GL_SRC_COLOR;
					break;

				case GLS_DSTBLEND_ONE_MINUS_SRC_COLOR:
					dstFactor = GL_ONE_MINUS_SRC_COLOR;
					break;

				case GLS_DSTBLEND_SRC_ALPHA:
					dstFactor = GL_SRC_ALPHA;
					break;

				case GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA:
					dstFactor = GL_ONE_MINUS_SRC_ALPHA;
					break;

				case GLS_DSTBLEND_DST_ALPHA:
					dstFactor = GL_DST_ALPHA;
					break;

				case GLS_DSTBLEND_ONE_MINUS_DST_ALPHA:
					dstFactor = GL_ONE_MINUS_DST_ALPHA;
					break;

				default:
					dstFactor = GL_ONE; // to get warning to shut up
					ri.Error( ERR_DROP, "GL_State: invalid dst blend state bits" );
			}

			glEnable( GL_BLEND );
			GL_BlendFunc( srcFactor, dstFactor );
		}
		else
		{
			glDisable( GL_BLEND );
		}
	}

	// check colormask
	if ( diff & GLS_COLORMASK_BITS )
	{
		GL_ColorMask( ( stateBits & GLS_REDMASK_FALSE ) ? GL_FALSE : GL_TRUE,
			      ( stateBits & GLS_GREENMASK_FALSE ) ? GL_FALSE : GL_TRUE,
			      ( stateBits & GLS_BLUEMASK_FALSE ) ? GL_FALSE : GL_TRUE,
			      ( stateBits & GLS_ALPHAMASK_FALSE ) ? GL_FALSE : GL_TRUE );
	}

	// check depthmask
	if ( diff & GLS_DEPTHMASK_TRUE )
	{
		if ( stateBits & GLS_DEPTHMASK_TRUE )
		{
			GL_DepthMask( GL_TRUE );
		}
		else
		{
			GL_DepthMask( GL_FALSE );
		}
	}

	// fill/line mode
	if ( diff & GLS_POLYMODE_LINE )
	{
		if ( stateBits & GLS_POLYMODE_LINE )
		{
			GL_PolygonMode( GL_FRONT_AND_BACK, GL_LINE );
		}
		else
		{
			GL_PolygonMode( GL_FRONT_AND_BACK, GL_FILL );
		}
	}

	// depthtest
	if ( diff & GLS_DEPTHTEST_DISABLE )
	{
		if ( stateBits & GLS_DEPTHTEST_DISABLE )
		{
			glDisable( GL_DEPTH_TEST );
		}
		else
		{
			glEnable( GL_DEPTH_TEST );
		}
	}

	// alpha test - deprecated in OpenGL 3.0
#if 0

	if ( diff & GLS_ATEST_BITS )
	{
		switch ( stateBits & GLS_ATEST_BITS )
		{
			case GLS_ATEST_GT_0:
			case GLS_ATEST_LT_128:
			case GLS_ATEST_GE_128:
				//case GLS_ATEST_GT_CUSTOM:
				glEnable( GL_SAMPLE_ALPHA_TO_COVERAGE );
				break;

			default:
			case 0:
				glDisable( GL_SAMPLE_ALPHA_TO_COVERAGE );
				break;
		}
	}

#endif

	/*
	   if(diff & GLS_ATEST_BITS)
	   {
	   switch (stateBits & GLS_ATEST_BITS)
	   {
	   case 0:
	   glDisable(GL_ALPHA_TEST);
	   break;
	   case GLS_ATEST_GT_0:
	   glEnable(GL_ALPHA_TEST);
	   glAlphaFunc(GL_GREATER, 0.0f);
	   break;
	   case GLS_ATEST_LT_80:
	   glEnable(GL_ALPHA_TEST);
	   glAlphaFunc(GL_LESS, 0.5f);
	   break;
	   case GLS_ATEST_GE_80:
	   glEnable(GL_ALPHA_TEST);
	   glAlphaFunc(GL_GEQUAL, 0.5f);
	   break;
	   case GLS_ATEST_GT_CUSTOM:
	   // FIXME
	   glEnable(GL_ALPHA_TEST);
	   glAlphaFunc(GL_GREATER, 0.5f);
	   break;
	   default:
	   assert(0);
	   break;
	   }
	   }
	 */

	// stenciltest
	if ( diff & GLS_STENCILTEST_ENABLE )
	{
		if ( stateBits & GLS_STENCILTEST_ENABLE )
		{
			glEnable( GL_STENCIL_TEST );
		}
		else
		{
			glDisable( GL_STENCIL_TEST );
		}
	}

	glState.glStateBits = stateBits;
}

void GL_VertexAttribsState( uint32_t stateBits )
{
	uint32_t diff;

	if ( glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning )
	{
		stateBits |= ( ATTR_BONE_INDEXES | ATTR_BONE_WEIGHTS );
	}

	GL_VertexAttribPointers( stateBits );

	diff = stateBits ^ glState.vertexAttribsState;

	if ( !diff )
	{
		return;
	}

	if ( diff & ATTR_POSITION )
	{
		if ( stateBits & ATTR_POSITION )
		{
			if ( r_logFile->integer )
			{
				GLimp_LogComment( "glEnableVertexAttribArray( ATTR_INDEX_POSITION )\n" );
			}

			glEnableVertexAttribArray( ATTR_INDEX_POSITION );
		}
		else
		{
			if ( r_logFile->integer )
			{
				GLimp_LogComment( "glDisableVertexAttribArray( ATTR_INDEX_POSITION )\n" );
			}

			glDisableVertexAttribArray( ATTR_INDEX_POSITION );
		}
	}

	if ( diff & ATTR_TEXCOORD )
	{
		if ( stateBits & ATTR_TEXCOORD )
		{
			if ( r_logFile->integer )
			{
				GLimp_LogComment( "glEnableVertexAttribArray( ATTR_INDEX_TEXCOORD )\n" );
			}

			glEnableVertexAttribArray( ATTR_INDEX_TEXCOORD0 );
		}
		else
		{
			if ( r_logFile->integer )
			{
				GLimp_LogComment( "glDisableVertexAttribArray( ATTR_INDEX_TEXCOORD )\n" );
			}

			glDisableVertexAttribArray( ATTR_INDEX_TEXCOORD0 );
		}
	}

	if ( diff & ATTR_LIGHTCOORD )
	{
		if ( stateBits & ATTR_LIGHTCOORD )
		{
			if ( r_logFile->integer )
			{
				GLimp_LogComment( "glEnableVertexAttribArray( ATTR_INDEX_LIGHTCOORD )\n" );
			}

			glEnableVertexAttribArray( ATTR_INDEX_TEXCOORD1 );
		}
		else
		{
			if ( r_logFile->integer )
			{
				GLimp_LogComment( "glDisableVertexAttribArray( ATTR_INDEX_LIGHTCOORD )\n" );
			}

			glDisableVertexAttribArray( ATTR_INDEX_TEXCOORD1 );
		}
	}

	if ( diff & ATTR_TANGENT )
	{
		if ( stateBits & ATTR_TANGENT )
		{
			if ( r_logFile->integer )
			{
				GLimp_LogComment( "glEnableVertexAttribArray( ATTR_INDEX_TANGENT )\n" );
			}

			glEnableVertexAttribArray( ATTR_INDEX_TANGENT );
		}
		else
		{
			if ( r_logFile->integer )
			{
				GLimp_LogComment( "glDisableVertexAttribArray( ATTR_INDEX_TANGENT )\n" );
			}

			glDisableVertexAttribArray( ATTR_INDEX_TANGENT );
		}
	}

	if ( diff & ATTR_BINORMAL )
	{
		if ( stateBits & ATTR_BINORMAL )
		{
			if ( r_logFile->integer )
			{
				GLimp_LogComment( "glEnableVertexAttribArray( ATTR_INDEX_BINORMAL )\n" );
			}

			glEnableVertexAttribArray( ATTR_INDEX_BINORMAL );
		}
		else
		{
			if ( r_logFile->integer )
			{
				GLimp_LogComment( "glDisableVertexAttribArray( ATTR_INDEX_BINORMAL )\n" );
			}

			glDisableVertexAttribArray( ATTR_INDEX_BINORMAL );
		}
	}

	if ( diff & ATTR_NORMAL )
	{
		if ( stateBits & ATTR_NORMAL )
		{
			if ( r_logFile->integer )
			{
				GLimp_LogComment( "glEnableVertexAttribArray( ATTR_INDEX_NORMAL )\n" );
			}

			glEnableVertexAttribArray( ATTR_INDEX_NORMAL );
		}
		else
		{
			if ( r_logFile->integer )
			{
				GLimp_LogComment( "glDisableVertexAttribArray( ATTR_INDEX_NORMAL )\n" );
			}

			glDisableVertexAttribArray( ATTR_INDEX_NORMAL );
		}
	}

	if ( diff & ATTR_COLOR )
	{
		if ( stateBits & ATTR_COLOR )
		{
			if ( r_logFile->integer )
			{
				GLimp_LogComment( "glEnableVertexAttribArray( ATTR_INDEX_COLOR )\n" );
			}

			glEnableVertexAttribArray( ATTR_INDEX_COLOR );
		}
		else
		{
			if ( r_logFile->integer )
			{
				GLimp_LogComment( "glDisableVertexAttribArray( ATTR_INDEX_COLOR )\n" );
			}

			glDisableVertexAttribArray( ATTR_INDEX_COLOR );
		}
	}

#if !defined( COMPAT_Q3A ) && !defined( COMPAT_ET )

	if ( diff & ATTR_PAINTCOLOR )
	{
		if ( stateBits & ATTR_PAINTCOLOR )
		{
			if ( r_logFile->integer )
			{
				GLimp_LogComment( "glEnableVertexAttribArray( ATTR_INDEX_PAINTCOLOR )\n" );
			}

			glEnableVertexAttribArray( ATTR_INDEX_PAINTCOLOR );
		}
		else
		{
			if ( r_logFile->integer )
			{
				GLimp_LogComment( "glDisableVertexAttribArray( ATTR_INDEX_PAINTCOLOR )\n" );
			}

			glDisableVertexAttribArray( ATTR_INDEX_PAINTCOLOR );
		}
	}

	if ( diff & ATTR_LIGHTDIRECTION )
	{
		if ( stateBits & ATTR_LIGHTDIRECTION )
		{
			if ( r_logFile->integer )
			{
				GLimp_LogComment( "glEnableVertexAttribArray( ATTR_INDEX_LIGHTDIRECTION )\n" );
			}

			glEnableVertexAttribArray( ATTR_INDEX_LIGHTDIRECTION );
		}
		else
		{
			if ( r_logFile->integer )
			{
				GLimp_LogComment( "glDisableVertexAttribArray( ATTR_INDEX_LIGHTDIRECTION )\n" );
			}

			glDisableVertexAttribArray( ATTR_INDEX_LIGHTDIRECTION );
		}
	}

#endif

	if ( diff & ATTR_BONE_INDEXES )
	{
		if ( stateBits & ATTR_BONE_INDEXES )
		{
			if ( r_logFile->integer )
			{
				GLimp_LogComment( "glEnableVertexAttribArray( ATTR_INDEX_BONE_INDEXES )\n" );
			}

			glEnableVertexAttribArray( ATTR_INDEX_BONE_INDEXES );
		}
		else
		{
			if ( r_logFile->integer )
			{
				GLimp_LogComment( "glDisableVertexAttribArray( ATTR_INDEX_BONE_INDEXES )\n" );
			}

			glDisableVertexAttribArray( ATTR_INDEX_BONE_INDEXES );
		}
	}

	if ( diff & ATTR_BONE_WEIGHTS )
	{
		if ( stateBits & ATTR_BONE_WEIGHTS )
		{
			if ( r_logFile->integer )
			{
				GLimp_LogComment( "glEnableVertexAttribArray( ATTR_INDEX_BONE_WEIGHTS )\n" );
			}

			glEnableVertexAttribArray( ATTR_INDEX_BONE_WEIGHTS );
		}
		else
		{
			if ( r_logFile->integer )
			{
				GLimp_LogComment( "glDisableVertexAttribArray( ATTR_INDEX_BONE_WEIGHTS )\n" );
			}

			glDisableVertexAttribArray( ATTR_INDEX_BONE_WEIGHTS );
		}
	}

	if ( diff & ATTR_POSITION2 )
	{
		if ( stateBits & ATTR_POSITION2 )
		{
			if ( r_logFile->integer )
			{
				GLimp_LogComment( "glEnableVertexAttribArray( ATTR_INDEX_POSITION2 )\n" );
			}

			glEnableVertexAttribArray( ATTR_INDEX_POSITION2 );
		}
		else
		{
			if ( r_logFile->integer )
			{
				GLimp_LogComment( "glDisableVertexAttribArray( ATTR_INDEX_POSITION2 )\n" );
			}

			glDisableVertexAttribArray( ATTR_INDEX_POSITION2 );
		}
	}

	if ( diff & ATTR_TANGENT2 )
	{
		if ( stateBits & ATTR_TANGENT2 )
		{
			if ( r_logFile->integer )
			{
				GLimp_LogComment( "glEnableVertexAttribArray( ATTR_INDEX_TANGENT2 )\n" );
			}

			glEnableVertexAttribArray( ATTR_INDEX_TANGENT2 );
		}
		else
		{
			if ( r_logFile->integer )
			{
				GLimp_LogComment( "glDisableVertexAttribArray( ATTR_INDEX_TANGENT2 )\n" );
			}

			glDisableVertexAttribArray( ATTR_INDEX_TANGENT2 );
		}
	}

	if ( diff & ATTR_BINORMAL2 )
	{
		if ( stateBits & ATTR_BINORMAL2 )
		{
			if ( r_logFile->integer )
			{
				GLimp_LogComment( "glEnableVertexAttribArray( ATTR_INDEX_BINORMAL2 )\n" );
			}

			glEnableVertexAttribArray( ATTR_INDEX_BINORMAL2 );
		}
		else
		{
			if ( r_logFile->integer )
			{
				GLimp_LogComment( "glDisableVertexAttribArray( ATTR_INDEX_BINORMAL2 )\n" );
			}

			glDisableVertexAttribArray( ATTR_INDEX_BINORMAL2 );
		}
	}

	if ( diff & ATTR_NORMAL2 )
	{
		if ( stateBits & ATTR_NORMAL2 )
		{
			if ( r_logFile->integer )
			{
				GLimp_LogComment( "glEnableVertexAttribArray( ATTR_INDEX_NORMAL2 )\n" );
			}

			glEnableVertexAttribArray( ATTR_INDEX_NORMAL2 );
		}
		else
		{
			if ( r_logFile->integer )
			{
				GLimp_LogComment( "glDisableVertexAttribArray( ATTR_INDEX_NORMAL2 )\n" );
			}

			glDisableVertexAttribArray( ATTR_INDEX_NORMAL2 );
		}
	}

	glState.vertexAttribsState = stateBits;
}

void GL_VertexAttribPointers( uint32_t attribBits )
{
	if ( !glState.currentVBO )
	{
		ri.Error( ERR_FATAL, "GL_VertexAttribPointers: no VBO bound" );
	}

	if ( r_logFile->integer )
	{
		// don't just call LogComment, or we will get a call to va() every frame!
		GLimp_LogComment( va( "--- GL_VertexAttribPointers( %s ) ---\n", glState.currentVBO->name ) );
	}

	if ( glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning )
	{
		attribBits |= ( ATTR_BONE_INDEXES | ATTR_BONE_WEIGHTS );
	}

	if ( ( attribBits & ATTR_POSITION ) )
	{
		if ( r_logFile->integer )
		{
			GLimp_LogComment( "glVertexAttribPointer( ATTR_INDEX_POSITION )\n" );
		}

		glVertexAttribPointer( ATTR_INDEX_POSITION, 4, GL_FLOAT, 0, 0, BUFFER_OFFSET( glState.currentVBO->ofsXYZ + ( glState.vertexAttribsOldFrame * glState.currentVBO->sizeXYZ ) ) );
		glState.vertexAttribPointersSet |= ATTR_POSITION;
	}

	if ( ( attribBits & ATTR_TEXCOORD ) )
	{
		if ( r_logFile->integer )
		{
			GLimp_LogComment( "glVertexAttribPointer( ATTR_INDEX_TEXCOORD )\n" );
		}

		glVertexAttribPointer( ATTR_INDEX_TEXCOORD0, 4, GL_FLOAT, 0, 0, BUFFER_OFFSET( glState.currentVBO->ofsTexCoords ) );
		glState.vertexAttribPointersSet |= ATTR_TEXCOORD;
	}

	if ( ( attribBits & ATTR_LIGHTCOORD ) )
	{
		if ( r_logFile->integer )
		{
			GLimp_LogComment( "glVertexAttribPointer( ATTR_INDEX_LIGHTCOORD )\n" );
		}

		glVertexAttribPointer( ATTR_INDEX_TEXCOORD1, 4, GL_FLOAT, 0, 0, BUFFER_OFFSET( glState.currentVBO->ofsLightCoords ) );
		glState.vertexAttribPointersSet |= ATTR_LIGHTCOORD;
	}

	if ( ( attribBits & ATTR_TANGENT ) )
	{
		if ( r_logFile->integer )
		{
			GLimp_LogComment( "glVertexAttribPointer( ATTR_INDEX_TANGENT )\n" );
		}

		glVertexAttribPointer( ATTR_INDEX_TANGENT, 3, GL_FLOAT, 0, 16, BUFFER_OFFSET( glState.currentVBO->ofsTangents + ( glState.vertexAttribsOldFrame * glState.currentVBO->sizeTangents ) ) );
		glState.vertexAttribPointersSet |= ATTR_TANGENT;
	}

	if ( ( attribBits & ATTR_BINORMAL ) )
	{
		if ( r_logFile->integer )
		{
			GLimp_LogComment( "glVertexAttribPointer( ATTR_INDEX_BINORMAL )\n" );
		}

		glVertexAttribPointer( ATTR_INDEX_BINORMAL, 3, GL_FLOAT, 0, 16, BUFFER_OFFSET( glState.currentVBO->ofsBinormals + ( glState.vertexAttribsOldFrame * glState.currentVBO->sizeBinormals ) ) );
		glState.vertexAttribPointersSet |= ATTR_BINORMAL;
	}

	if ( ( attribBits & ATTR_NORMAL ) )
	{
		if ( r_logFile->integer )
		{
			GLimp_LogComment( "glVertexAttribPointer( ATTR_INDEX_NORMAL )\n" );
		}

		glVertexAttribPointer( ATTR_INDEX_NORMAL, 3, GL_FLOAT, 0, 16, BUFFER_OFFSET( glState.currentVBO->ofsNormals + ( glState.vertexAttribsOldFrame * glState.currentVBO->sizeNormals ) ) );
		glState.vertexAttribPointersSet |= ATTR_NORMAL;
	}

	if ( ( attribBits & ATTR_COLOR ) )
	{
		if ( r_logFile->integer )
		{
			GLimp_LogComment( "glVertexAttribPointer( ATTR_INDEX_COLOR )\n" );
		}

		glVertexAttribPointer( ATTR_INDEX_COLOR, 4, GL_FLOAT, 0, 0, BUFFER_OFFSET( glState.currentVBO->ofsColors ) );
		glState.vertexAttribPointersSet |= ATTR_COLOR;
	}

#if !defined( COMPAT_Q3A ) && !defined( COMPAT_ET )

	if ( ( attribBits & ATTR_PAINTCOLOR ) )
	{
		if ( r_logFile->integer )
		{
			GLimp_LogComment( "glVertexAttribPointer( ATTR_INDEX_PAINTCOLOR )\n" );
		}

		glVertexAttribPointer( ATTR_INDEX_PAINTCOLOR, 4, GL_FLOAT, 0, 0, BUFFER_OFFSET( glState.currentVBO->ofsPaintColors ) );
		glState.vertexAttribPointersSet |= ATTR_PAINTCOLOR;
	}

	if ( ( attribBits & ATTR_LIGHTDIRECTION ) )
	{
		if ( r_logFile->integer )
		{
			GLimp_LogComment( "glVertexAttribPointer( ATTR_INDEX_LIGHTDIRECTION )\n" );
		}

		glVertexAttribPointer( ATTR_INDEX_LIGHTDIRECTION, 3, GL_FLOAT, 0, 16, BUFFER_OFFSET( glState.currentVBO->ofsLightDirections ) );
		glState.vertexAttribPointersSet |= ATTR_LIGHTDIRECTION;
	}

#endif // #if !defined(COMPAT_Q3A) && !defined(COMPAT_ET)

	if ( ( attribBits & ATTR_BONE_INDEXES ) )
	{
		if ( r_logFile->integer )
		{
			GLimp_LogComment( "glVertexAttribPointer( ATTR_INDEX_BONE_INDEXES )\n" );
		}

		glVertexAttribPointer( ATTR_INDEX_BONE_INDEXES, 4, GL_INT, 0, 0, BUFFER_OFFSET( glState.currentVBO->ofsBoneIndexes ) );
		glState.vertexAttribPointersSet |= ATTR_BONE_INDEXES;
	}

	if ( ( attribBits & ATTR_BONE_WEIGHTS ) )
	{
		if ( r_logFile->integer )
		{
			GLimp_LogComment( "glVertexAttribPointer( ATTR_INDEX_BONE_WEIGHTS )\n" );
		}

		glVertexAttribPointer( ATTR_INDEX_BONE_WEIGHTS, 4, GL_FLOAT, 0, 0, BUFFER_OFFSET( glState.currentVBO->ofsBoneWeights ) );
		glState.vertexAttribPointersSet |= ATTR_BONE_WEIGHTS;
	}

	if ( glState.vertexAttribsInterpolation > 0 )
	{
		if ( ( attribBits & ATTR_POSITION2 ) )
		{
			if ( r_logFile->integer )
			{
				GLimp_LogComment( "glVertexAttribPointer( ATTR_INDEX_POSITION2 )\n" );
			}

			glVertexAttribPointer( ATTR_INDEX_POSITION2, 4, GL_FLOAT, 0, 0, BUFFER_OFFSET( glState.currentVBO->ofsXYZ + ( glState.vertexAttribsNewFrame * glState.currentVBO->sizeXYZ ) ) );
			glState.vertexAttribPointersSet |= ATTR_POSITION2;
		}

		if ( ( attribBits & ATTR_TANGENT2 ) )
		{
			if ( r_logFile->integer )
			{
				GLimp_LogComment( "glVertexAttribPointer( ATTR_INDEX_TANGENT2 )\n" );
			}

			glVertexAttribPointer( ATTR_INDEX_TANGENT2, 3, GL_FLOAT, 0, 16, BUFFER_OFFSET( glState.currentVBO->ofsTangents + ( glState.vertexAttribsNewFrame * glState.currentVBO->sizeTangents ) ) );
			glState.vertexAttribPointersSet |= ATTR_TANGENT2;
		}

		if ( ( attribBits & ATTR_BINORMAL2 ) )
		{
			if ( r_logFile->integer )
			{
				GLimp_LogComment( "glVertexAttribPointer( ATTR_INDEX_BINORMAL2 )\n" );
			}

			glVertexAttribPointer( ATTR_INDEX_BINORMAL2, 3, GL_FLOAT, 0, 16, BUFFER_OFFSET( glState.currentVBO->ofsBinormals + ( glState.vertexAttribsNewFrame * glState.currentVBO->sizeBinormals ) ) );
			glState.vertexAttribPointersSet |= ATTR_BINORMAL2;
		}

		if ( ( attribBits & ATTR_NORMAL2 ) )
		{
			if ( r_logFile->integer )
			{
				GLimp_LogComment( "glVertexAttribPointer( ATTR_INDEX_NORMAL2 )\n" );
			}

			glVertexAttribPointer( ATTR_INDEX_NORMAL2, 3, GL_FLOAT, 0, 16, BUFFER_OFFSET( glState.currentVBO->ofsNormals + ( glState.vertexAttribsNewFrame * glState.currentVBO->sizeNormals ) ) );
			glState.vertexAttribPointersSet |= ATTR_NORMAL2;
		}
	}
}

/*
================
RB_Hyperspace

A player has predicted a teleport, but hasn't arrived yet
================
*/
static void RB_Hyperspace( void )
{
	float c;

	if ( !backEnd.isHyperspace )
	{
		// do initialization shit
	}

	c = ( backEnd.refdef.time & 255 ) / 255.0f;
	GL_ClearColor( c, c, c, 1 );
	glClear( GL_COLOR_BUFFER_BIT );

	backEnd.isHyperspace = qtrue;
}

static void SetViewportAndScissor( void )
{
	GL_LoadProjectionMatrix( backEnd.viewParms.projectionMatrix );

	// set the window clipping
	GL_Viewport( backEnd.viewParms.viewportX, backEnd.viewParms.viewportY,
	             backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight );

	GL_Scissor( backEnd.viewParms.viewportX, backEnd.viewParms.viewportY,
	            backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight );
}

/*
================
RB_SetGL2D
================
*/
static void RB_SetGL2D( void )
{
	matrix_t proj;

	GLimp_LogComment( "--- RB_SetGL2D ---\n" );

	// disable offscreen rendering
	if ( glConfig2.framebufferObjectAvailable )
	{
		R_BindNullFBO();
	}

	backEnd.projection2D = qtrue;

	// set 2D virtual screen size
	GL_Viewport( 0, 0, glConfig.vidWidth, glConfig.vidHeight );
	GL_Scissor( 0, 0, glConfig.vidWidth, glConfig.vidHeight );

	MatrixOrthogonalProjection( proj, 0, glConfig.vidWidth, glConfig.vidHeight, 0, 0, 1 );
	GL_LoadProjectionMatrix( proj );
	GL_LoadModelViewMatrix( matrixIdentity );

	GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );

	GL_Cull( CT_TWO_SIDED );

	// set time for 2D shaders
	backEnd.refdef.time = ri.Milliseconds();
	backEnd.refdef.floatTime = backEnd.refdef.time * 0.001f;
}

enum renderDrawSurfaces_e
{
  DRAWSURFACES_WORLD_ONLY,
  DRAWSURFACES_ENTITIES_ONLY,
  DRAWSURFACES_ALL
};

static void RB_RenderDrawSurfaces( bool opaque, bool depthFill, renderDrawSurfaces_e drawSurfFilter )
{
	trRefEntity_t *entity, *oldEntity;
	shader_t      *shader, *oldShader;
	int           lightmapNum, oldLightmapNum;
	int           fogNum, oldFogNum;
	qboolean      depthRange, oldDepthRange;
	int           i;
	drawSurf_t    *drawSurf;

	GLimp_LogComment( "--- RB_RenderDrawSurfaces ---\n" );

	// draw everything
	oldEntity = NULL;
	oldShader = NULL;
	oldLightmapNum = -1;
	oldFogNum = -1;
	oldDepthRange = qfalse;
	depthRange = qfalse;
	backEnd.currentLight = NULL;

	for ( i = 0, drawSurf = backEnd.viewParms.drawSurfs; i < backEnd.viewParms.numDrawSurfs; i++, drawSurf++ )
	{
		// update locals
		entity = drawSurf->entity;
		shader = tr.sortedShaders[ drawSurf->shaderNum ];
		lightmapNum = drawSurf->lightmapNum;
		fogNum = drawSurf->fogNum;

		switch ( drawSurfFilter )
		{
			case DRAWSURFACES_WORLD_ONLY:
				if ( entity != &tr.worldEntity )
				{
					continue;
				}

				break;

			case DRAWSURFACES_ENTITIES_ONLY:
				if ( entity == &tr.worldEntity )
				{
					continue;
				}

				break;

			case DRAWSURFACES_ALL:
				break;
		};

		if ( glConfig2.occlusionQueryBits && glConfig.driverType != GLDRV_MESA && r_dynamicEntityOcclusionCulling->integer && !entity->occlusionQuerySamples )
		{
			continue;
		}

		if ( opaque )
		{
			// skip all translucent surfaces that don't matter for this pass
			if ( shader->sort > SS_OPAQUE )
			{
				break;
			}
		}
		else
		{
			// skip all opaque surfaces that don't matter for this pass
			if ( shader->sort <= SS_OPAQUE )
			{
				continue;
			}
		}

		if ( entity == oldEntity && shader == oldShader && lightmapNum == oldLightmapNum && fogNum == oldFogNum )
		{
			// fast path, same as previous sort
			rb_surfaceTable[ *drawSurf->surface ]( drawSurf->surface );
			continue;
		}

		// change the tess parameters if needed
		// an "entityMergable" shader is a shader that can have surfaces from separate
		// entities merged into a single batch, like smoke and blood puff sprites
		if ( shader != oldShader || lightmapNum != oldLightmapNum || fogNum != oldFogNum || ( entity != oldEntity && !shader->entityMergable ) )
		{
			if ( oldShader != NULL )
			{
				Tess_End();
			}

			if ( depthFill )
			{
				Tess_Begin( Tess_StageIteratorGBuffer, NULL, shader, NULL, qtrue, qfalse, lightmapNum, fogNum );
			}
			else
			{
				Tess_Begin( Tess_StageIteratorGeneric, NULL, shader, NULL, qfalse, qfalse, lightmapNum, fogNum );
			}

			oldShader = shader;
			oldLightmapNum = lightmapNum;
			oldFogNum = fogNum;
		}

		// change the modelview matrix if needed
		if ( entity != oldEntity )
		{
			depthRange = qfalse;

			if ( entity != &tr.worldEntity )
			{
				backEnd.currentEntity = entity;

				// set up the transformation matrix
				R_RotateEntityForViewParms( backEnd.currentEntity, &backEnd.viewParms, &backEnd.orientation );

				if ( backEnd.currentEntity->e.renderfx & RF_DEPTHHACK )
				{
					// hack the depth range to prevent view model from poking into walls
					depthRange = qtrue;
				}
			}
			else
			{
				backEnd.currentEntity = &tr.worldEntity;
				backEnd.orientation = backEnd.viewParms.world;
			}

			GL_LoadModelViewMatrix( backEnd.orientation.modelViewMatrix );

			// change depthrange if needed
			if ( oldDepthRange != depthRange )
			{
				if ( depthRange )
				{
					glDepthRange( 0, 0.3 );
				}
				else
				{
					glDepthRange( 0, 1 );
				}

				oldDepthRange = depthRange;
			}

			oldEntity = entity;
		}

		// add the triangles for this surface
		rb_surfaceTable[ *drawSurf->surface ]( drawSurf->surface );
	}

	// draw the contents of the last shader batch
	if ( oldShader != NULL )
	{
		Tess_End();
	}

	// go back to the world modelview matrix
	GL_LoadModelViewMatrix( backEnd.viewParms.world.modelViewMatrix );

	if ( depthRange )
	{
		glDepthRange( 0, 1 );
	}

	GL_CheckErrors();
}

#if 0
static void RB_RenderOpaqueSurfacesIntoDepth( bool onlyWorld )
{
	trRefEntity_t *entity, *oldEntity;
	shader_t      *shader, *oldShader;
	qboolean      depthRange, oldDepthRange;
	qboolean      alphaTest, oldAlphaTest;
	deformType_t  deformType, oldDeformType;
	int           i;
	drawSurf_t    *drawSurf;

	GLimp_LogComment( "--- RB_RenderOpaqueSurfacesIntoDepth ---\n" );

	// draw everything
	oldEntity = NULL;
	oldShader = NULL;
	oldDepthRange = depthRange = qfalse;
	oldAlphaTest = alphaTest = qfalse;
	oldDeformType = deformType = DEFORM_TYPE_NONE;
	backEnd.currentLight = NULL;

	for ( i = 0, drawSurf = backEnd.viewParms.drawSurfs; i < backEnd.viewParms.numDrawSurfs; i++, drawSurf++ )
	{
		// update locals
		entity = drawSurf->entity;
		shader = tr.sortedShaders[ drawSurf->shaderNum ];
		alphaTest = shader->alphaTest;

#if 0

		if ( onlyWorld && ( entity != &tr.worldEntity ) )
		{
			continue;
		}

#endif

		// skip all translucent surfaces that don't matter for this pass
		if ( shader->sort > SS_OPAQUE )
		{
			break;
		}

		if ( shader->numDeforms )
		{
			deformType = ShaderRequiresCPUDeforms( shader ) ? DEFORM_TYPE_CPU : DEFORM_TYPE_GPU;
		}
		else
		{
			deformType = DEFORM_TYPE_NONE;
		}

		// change the tess parameters if needed
		// an "entityMergable" shader is a shader that can have surfaces from separate
		// entities merged into a single batch, like smoke and blood puff sprites
		//if(shader != oldShader || lightmapNum != oldLightmapNum || (entity != oldEntity && !shader->entityMergable))

		if ( entity == oldEntity && ( alphaTest ? shader == oldShader : alphaTest == oldAlphaTest ) && deformType == oldDeformType )
		{
			// fast path, same as previous sort
			rb_surfaceTable[ *drawSurf->surface ]( drawSurf->surface );
			continue;
		}
		else
		{
			if ( oldShader != NULL )
			{
				Tess_End();
			}

			Tess_Begin( Tess_StageIteratorDepthFill, NULL, shader, NULL, qtrue, qfalse, -1, 0 );

			oldShader = shader;
			oldAlphaTest = alphaTest;
			oldDeformType = deformType;
		}

		// change the modelview matrix if needed
		if ( entity != oldEntity )
		{
			depthRange = qfalse;

			if ( entity != &tr.worldEntity )
			{
				backEnd.currentEntity = entity;

				// set up the transformation matrix
				R_RotateEntityForViewParms( backEnd.currentEntity, &backEnd.viewParms, &backEnd.orientation );

				if ( backEnd.currentEntity->e.renderfx & RF_DEPTHHACK )
				{
					// hack the depth range to prevent view model from poking into walls
					depthRange = qtrue;
				}
			}
			else
			{
				backEnd.currentEntity = &tr.worldEntity;
				backEnd.orientation = backEnd.viewParms.world;
			}

			GL_LoadModelViewMatrix( backEnd.orientation.modelViewMatrix );

			// change depthrange if needed
			if ( oldDepthRange != depthRange )
			{
				if ( depthRange )
				{
					glDepthRange( 0, 0.3 );
				}
				else
				{
					glDepthRange( 0, 1 );
				}

				oldDepthRange = depthRange;
			}

			oldEntity = entity;
		}

		// add the triangles for this surface
		rb_surfaceTable[ *drawSurf->surface ]( drawSurf->surface );
	}

	// draw the contents of the last shader batch
	if ( oldShader != NULL )
	{
		Tess_End();
	}

	// go back to the world modelview matrix
	GL_LoadModelViewMatrix( backEnd.viewParms.world.modelViewMatrix );

	if ( depthRange )
	{
		glDepthRange( 0, 1 );
	}

	GL_CheckErrors();
}

#endif

// *INDENT-OFF*
#ifdef VOLUMETRIC_LIGHTING
static void Render_lightVolume( interaction_t *ia )
{
	int           j;
	trRefLight_t  *light;
	shader_t      *lightShader;
	shaderStage_t *attenuationXYStage;
	shaderStage_t *attenuationZStage;
	matrix_t      ortho;
	vec4_t        quadVerts[ 4 ];

	light = ia->light;

	// set the window clipping
	GL_Viewport( backEnd.viewParms.viewportX, backEnd.viewParms.viewportY,
	             backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight );

	// set light scissor to reduce fillrate
	GL_Scissor( ia->scissorX, ia->scissorY, ia->scissorWidth, ia->scissorHeight );

	// set 2D virtual screen size
	GL_PushMatrix();
	MatrixOrthogonalProjection( ortho, backEnd.viewParms.viewportX,
	                            backEnd.viewParms.viewportX + backEnd.viewParms.viewportWidth,
	                            backEnd.viewParms.viewportY,
	                            backEnd.viewParms.viewportY + backEnd.viewParms.viewportHeight, -99999, 99999 );
	GL_LoadProjectionMatrix( ortho );
	GL_LoadModelViewMatrix( matrixIdentity );

	switch ( light->l.rlType )
	{
		case RL_PROJ:
			{
				MatrixSetupTranslation( light->attenuationMatrix, 0.5, 0.5, 0.0 );  // bias
				MatrixMultiplyScale( light->attenuationMatrix, 0.5, 0.5, 1.0 / Q_min( light->falloffLength, 1.0 ) );   // scale
				break;
			}

		case RL_OMNI:
		default:
			{
				MatrixSetupTranslation( light->attenuationMatrix, 0.5, 0.5, 0.5 );  // bias
				MatrixMultiplyScale( light->attenuationMatrix, 0.5, 0.5, 0.5 );  // scale
				break;
			}
	}

	MatrixMultiply2( light->attenuationMatrix, light->projectionMatrix );  // light projection (frustum)
	MatrixMultiply2( light->attenuationMatrix, light->viewMatrix );

	lightShader = light->shader;
	attenuationZStage = lightShader->stages[ 0 ];

	for ( j = 1; j < MAX_SHADER_STAGES; j++ )
	{
		attenuationXYStage = lightShader->stages[ j ];

		if ( !attenuationXYStage )
		{
			break;
		}

		if ( attenuationXYStage->type != ST_ATTENUATIONMAP_XY )
		{
			continue;
		}

		if ( !RB_EvalExpression( &attenuationXYStage->ifExp, 1.0 ) )
		{
			continue;
		}

		Tess_ComputeColor( attenuationXYStage );
		R_ComputeFinalAttenuation( attenuationXYStage, light );

		if ( light->l.rlType == RL_OMNI )
		{
			vec3_t   viewOrigin;
			vec3_t   lightOrigin;
			vec4_t   lightColor;
			qboolean shadowCompare;

			GLimp_LogComment( "--- Render_lightVolume_omni ---\n" );

			// enable shader, set arrays
			gl_lightVolumeShader_omni->BindProgram();
			//GL_VertexAttribsState(tr.lightVolumeShader_omni.attribs);
			GL_Cull( CT_TWO_SIDED );
			GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE );
			//GL_State(GLS_DEPTHFUNC_LESS | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE);
			//GL_State(GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
			//GL_State(attenuationXYStage->stateBits & ~(GLS_DEPTHMASK_TRUE | GLS_DEPTHTEST_DISABLE));

			// set uniforms
			VectorCopy( backEnd.viewParms.orientation.origin, viewOrigin );  // in world space
			VectorCopy( light->origin, lightOrigin );
			VectorCopy( tess.svars.color, lightColor );

			shadowCompare = (qboolean)((int)r_shadows->integer >= SHADOWING_ESM16 && !light->l.noShadows && light->shadowLOD >= 0);

			gl_lightVolumeShader_omni->SetUniform_ViewOrigin( viewOrigin );
			gl_lightVolumeShader_omni->SetUniform_LightOrigin( lightOrigin );
			gl_lightVolumeShader_omni->SetUniform_LightColor( lightColor );
			gl_lightVolumeShader_omni->SetUniform_LightRadius( light->sphereRadius );
			gl_lightVolumeShader_omni->SetUniform_LightScale( light->l.scale );
			gl_lightVolumeShader_omni->SetUniform_LightAttenuationMatrix( light->attenuationMatrix2 );

			// FIXME gl_lightVolumeShader_omni->SetUniform_ShadowMatrix( light->attenuationMatrix );
			gl_lightVolumeShader_omni->SetShadowing( shadowCompare );
			gl_lightVolumeShader_omni->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );

			gl_lightVolumeShader_omni->SetUniform_UnprojectMatrix( backEnd.viewParms.unprojectionMatrix );

			//gl_lightVolumeShader_omni->SetUniform_PortalClipping( backEnd.viewParms.isPortal );

			// bind u_DepthMap
			GL_SelectTexture( 0 );

			if ( r_deferredShading->integer && glConfig2.framebufferObjectAvailable && glConfig2.textureFloatAvailable &&
			     glConfig2.drawBuffersAvailable && glConfig2.maxDrawBuffers >= 4 )
			{
				GL_Bind( tr.depthRenderImage );
			}
			else if ( r_hdrRendering->integer && glConfig2.framebufferObjectAvailable && glConfig2.textureFloatAvailable )
			{
				GL_Bind( tr.depthRenderImage );
			}
			else
			{
				// depth texture is not bound to a FBO
				GL_Bind( tr.depthRenderImage );
				glCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, 0, tr.depthRenderImage->uploadWidth, tr.depthRenderImage->uploadHeight );
			}

			// bind u_AttenuationMapXY
			GL_SelectTexture( 1 );
			BindAnimatedImage( &attenuationXYStage->bundle[ TB_COLORMAP ] );

			// bind u_AttenuationMapZ
			GL_SelectTexture( 2 );
			BindAnimatedImage( &attenuationZStage->bundle[ TB_COLORMAP ] );

			// bind u_ShadowMap
			if ( shadowCompare )
			{
				GL_SelectTexture( 3 );
				GL_Bind( tr.shadowCubeFBOImage[ light->shadowLOD ] );
			}

			// draw light scissor rectangle
			Vector4Set( quadVerts[ 0 ], ia->scissorX, ia->scissorY, 0, 1 );
			Vector4Set( quadVerts[ 1 ], ia->scissorX + ia->scissorWidth - 1, ia->scissorY, 0, 1 );
			Vector4Set( quadVerts[ 2 ], ia->scissorX + ia->scissorWidth - 1, ia->scissorY + ia->scissorHeight - 1, 0,
			            1 );
			Vector4Set( quadVerts[ 3 ], ia->scissorX, ia->scissorY + ia->scissorHeight - 1, 0, 1 );
			Tess_InstantQuad( quadVerts );

			GL_CheckErrors();
		}
	}

	GL_PopMatrix();
}

#endif
// *INDENT-ON*

/*
 * helper function for parallel split shadow mapping
 */
static int MergeInteractionBounds( const matrix_t lightViewProjectionMatrix, interaction_t *ia, int iaCount, vec3_t bounds[ 2 ], bool shadowCasters )
{
	int           i;
	int           j;
	surfaceType_t *surface;
	vec4_t        point;
	vec4_t        transf;
	vec3_t        worldBounds[ 2 ];
	//vec3_t    viewBounds[2];
	//vec3_t    center;
	//float     radius;
	int       numCasters;

	frustum_t frustum;
	cplane_t  *clipPlane;
	int       r;

	numCasters = 0;
	ClearBounds( bounds[ 0 ], bounds[ 1 ] );

	// calculate frustum planes using the modelview projection matrix
	R_SetupFrustum2( frustum, lightViewProjectionMatrix );

	while ( iaCount < backEnd.viewParms.numInteractions )
	{
		surface = ia->surface;

		if ( shadowCasters )
		{
			if ( ia->type == IA_LIGHTONLY )
			{
				goto skipInteraction;
			}
		}
		else
		{
			// we only merge shadow receivers
			if ( ia->type == IA_SHADOWONLY )
			{
				goto skipInteraction;
			}
		}

		if ( *surface == SF_FACE || *surface == SF_GRID || *surface == SF_TRIANGLES )
		{
			srfGeneric_t *gen = ( srfGeneric_t * ) surface;

			VectorCopy( gen->bounds[ 0 ], worldBounds[ 0 ] );
			VectorCopy( gen->bounds[ 1 ], worldBounds[ 1 ] );
		}
		else if ( *surface == SF_VBO_MESH )
		{
			srfVBOMesh_t *srf = ( srfVBOMesh_t * ) surface;

			//ri.Printf(PRINT_ALL, "merging vbo mesh bounds\n");

			VectorCopy( srf->bounds[ 0 ], worldBounds[ 0 ] );
			VectorCopy( srf->bounds[ 1 ], worldBounds[ 1 ] );
		}
		else if ( *surface == SF_MDV )
		{
			//Tess_AddCube(vec3_origin, entity->localBounds[0], entity->localBounds[1], lightColor);
			goto skipInteraction;
		}
		else
		{
			goto skipInteraction;
		}

#if 1

		// use the frustum planes to cut off shadow casters beyond the split frustum
		for ( i = 0; i < 6; i++ )
		{
			clipPlane = &frustum[ i ];

			r = BoxOnPlaneSide( worldBounds[ 0 ], worldBounds[ 1 ], clipPlane );

			if ( r == 2 )
			{
				goto skipInteraction;
			}
		}

#endif

		if ( shadowCasters && ia->type != IA_LIGHTONLY )
		{
			numCasters++;
		}

#if 1

		for ( j = 0; j < 8; j++ )
		{
			point[ 0 ] = worldBounds[ j & 1 ][ 0 ];
			point[ 1 ] = worldBounds[( j >> 1 ) & 1 ][ 1 ];
			point[ 2 ] = worldBounds[( j >> 2 ) & 1 ][ 2 ];
			point[ 3 ] = 1;

			MatrixTransform4( lightViewProjectionMatrix, point, transf );
			transf[ 0 ] /= transf[ 3 ];
			transf[ 1 ] /= transf[ 3 ];
			transf[ 2 ] /= transf[ 3 ];

			AddPointToBounds( transf, bounds[ 0 ], bounds[ 1 ] );
		}

#elif 0
		ClearBounds( viewBounds[ 0 ], viewBounds[ 1 ] );

		for ( j = 0; j < 8; j++ )
		{
			point[ 0 ] = worldBounds[ j & 1 ][ 0 ];
			point[ 1 ] = worldBounds[( j >> 1 ) & 1 ][ 1 ];
			point[ 2 ] = worldBounds[( j >> 2 ) & 1 ][ 2 ];
			point[ 3 ] = 1;

			MatrixTransform4( lightViewProjectionMatrix, point, transf );
			transf[ 0 ] /= transf[ 3 ];
			transf[ 1 ] /= transf[ 3 ];
			transf[ 2 ] /= transf[ 3 ];

			AddPointToBounds( transf, viewBounds[ 0 ], viewBounds[ 1 ] );
		}

		// get sphere of AABB
		VectorAdd( viewBounds[ 0 ], viewBounds[ 1 ], center );
		VectorScale( center, 0.5, center );

		radius = RadiusFromBounds( viewBounds[ 0 ], viewBounds[ 1 ] );

		for ( j = 0; j < 3; j++ )
		{
			if ( ( transf[ j ] - radius ) < bounds[ 0 ][ j ] )
			{
				bounds[ 0 ][ j ] = transf[ i ] - radius;
			}

			if ( ( transf[ j ] + radius ) > bounds[ 1 ][ j ] )
			{
				bounds[ 1 ][ j ] = transf[ i ] + radius;
			}
		}

#else

		ClearBounds( viewBounds[ 0 ], viewBounds[ 1 ] );

		for ( j = 0; j < 8; j++ )
		{
			point[ 0 ] = worldBounds[ j & 1 ][ 0 ];
			point[ 1 ] = worldBounds[( j >> 1 ) & 1 ][ 1 ];
			point[ 2 ] = worldBounds[( j >> 2 ) & 1 ][ 2 ];
			point[ 3 ] = 1;

			MatrixTransform4( lightViewProjectionMatrix, point, transf );
			//transf[0] /= transf[3];
			//transf[1] /= transf[3];
			//transf[2] /= transf[3];

			AddPointToBounds( transf, viewBounds[ 0 ], viewBounds[ 1 ] );
		}

		// get sphere of AABB
		VectorAdd( viewBounds[ 0 ], viewBounds[ 1 ], center );
		VectorScale( center, 0.5, center );

		//MatrixTransform4(lightViewProjectionMatrix, center, transf);
		//transf[0] /= transf[3];
		//transf[1] /= transf[3];
		//transf[2] /= transf[3];

		radius = RadiusFromBounds( viewBounds[ 0 ], viewBounds[ 1 ] );

		if ( ( transf[ 2 ] + radius ) > bounds[ 1 ][ 2 ] )
		{
			bounds[ 1 ][ 2 ] = transf[ 2 ] + radius;
		}

#endif

skipInteraction:

		if ( !ia->next )
		{
			// this is the last interaction of the current light
			break;
		}
		else
		{
			// just continue
			ia = ia->next;
			iaCount++;
		}
	}

	return numCasters;
}

/*
=================
RB_RenderInteractions
=================
*/
static void RB_RenderInteractions()
{
	shader_t      *shader, *oldShader;
	trRefEntity_t *entity, *oldEntity;
	trRefLight_t  *light, *oldLight;
	interaction_t *ia;
	qboolean      depthRange, oldDepthRange;
	int           iaCount;
	surfaceType_t *surface;
	vec3_t        tmp;
	matrix_t      modelToLight;
	int           startTime = 0, endTime = 0;

	GLimp_LogComment( "--- RB_RenderInteractions ---\n" );

	if ( r_speeds->integer == RSPEEDS_SHADING_TIMES )
	{
		glFinish();
		startTime = ri.Milliseconds();
	}

	// draw everything
	oldLight = NULL;
	oldEntity = NULL;
	oldShader = NULL;
	oldDepthRange = qfalse;
	depthRange = qfalse;

	// render interactions
	for ( iaCount = 0, ia = &backEnd.viewParms.interactions[ 0 ]; iaCount < backEnd.viewParms.numInteractions; )
	{
		backEnd.currentLight = light = ia->light;
		backEnd.currentEntity = entity = ia->entity;
		surface = ia->surface;
		shader = ia->surfaceShader;

		if ( glConfig2.occlusionQueryBits && glConfig.driverType != GLDRV_MESA )
		{
			// skip all interactions of this light because it failed the occlusion query
			if ( r_dynamicLightOcclusionCulling->integer && !ia->occlusionQuerySamples )
			{
				goto skipInteraction;
			}

			if ( r_dynamicEntityOcclusionCulling->integer && !entity->occlusionQuerySamples )
			{
				goto skipInteraction;
			}
		}

		if ( !shader->interactLight )
		{
			// skip this interaction because the surface shader has no ability to interact with light
			// this will save texcoords and matrix calculations
			goto skipInteraction;
		}

		if ( ia->type == IA_SHADOWONLY )
		{
			// skip this interaction because the interaction is meant for shadowing only
			goto skipInteraction;
		}

		if ( light != oldLight )
		{
			GLimp_LogComment( "----- Rendering new light -----\n" );

			// set light scissor to reduce fillrate
			GL_Scissor( ia->scissorX, ia->scissorY, ia->scissorWidth, ia->scissorHeight );
		}

		// Tr3B: this should never happen in the first iteration
		if ( light == oldLight && entity == oldEntity && shader == oldShader )
		{
			// fast path, same as previous
			rb_surfaceTable[ *surface ]( surface );
			goto nextInteraction;
		}

		// draw the contents of the last shader batch
		Tess_End();

		// begin a new batch
		Tess_Begin( Tess_StageIteratorLighting, NULL, shader, light->shader, qfalse, qfalse, -1, 0 );

		// change the modelview matrix if needed
		if ( entity != oldEntity )
		{
			depthRange = qfalse;

			if ( entity != &tr.worldEntity )
			{
				// set up the transformation matrix
				R_RotateEntityForViewParms( backEnd.currentEntity, &backEnd.viewParms, &backEnd.orientation );

				if ( backEnd.currentEntity->e.renderfx & RF_DEPTHHACK )
				{
					// hack the depth range to prevent view model from poking into walls
					depthRange = qtrue;
				}
			}
			else
			{
				backEnd.orientation = backEnd.viewParms.world;
			}

			GL_LoadModelViewMatrix( backEnd.orientation.modelViewMatrix );

			// change depthrange if needed
			if ( oldDepthRange != depthRange )
			{
				if ( depthRange )
				{
					glDepthRange( 0, 0.3 );
				}
				else
				{
					glDepthRange( 0, 1 );
				}

				oldDepthRange = depthRange;
			}
		}

		// change the attenuation matrix if needed
		if ( light != oldLight || entity != oldEntity )
		{
			// transform light origin into model space for u_LightOrigin parameter
			if ( entity != &tr.worldEntity )
			{
				VectorSubtract( light->origin, backEnd.orientation.origin, tmp );
				light->transformed[ 0 ] = DotProduct( tmp, backEnd.orientation.axis[ 0 ] );
				light->transformed[ 1 ] = DotProduct( tmp, backEnd.orientation.axis[ 1 ] );
				light->transformed[ 2 ] = DotProduct( tmp, backEnd.orientation.axis[ 2 ] );
			}
			else
			{
				VectorCopy( light->origin, light->transformed );
			}

			// build the attenuation matrix using the entity transform
			MatrixMultiply( light->viewMatrix, backEnd.orientation.transformMatrix, modelToLight );

			switch ( light->l.rlType )
			{
				case RL_PROJ:
					{
						MatrixSetupTranslation( light->attenuationMatrix, 0.5, 0.5, 0.0 );  // bias
						MatrixMultiplyScale( light->attenuationMatrix, 0.5, 0.5, 1.0 / Q_min( light->falloffLength, 1.0 ) );   // scale
						break;
					}

				case RL_OMNI:
				default:
					{
						MatrixSetupTranslation( light->attenuationMatrix, 0.5, 0.5, 0.5 );  // bias
						MatrixMultiplyScale( light->attenuationMatrix, 0.5, 0.5, 0.5 );  // scale
						break;
					}
			}

			MatrixMultiply2( light->attenuationMatrix, light->projectionMatrix );  // light projection (frustum)
			MatrixMultiply2( light->attenuationMatrix, modelToLight );
		}

		// add the triangles for this surface
		rb_surfaceTable[ *surface ]( surface );

nextInteraction:

		// remember values
		oldLight = light;
		oldEntity = entity;
		oldShader = shader;

skipInteraction:

		if ( !ia->next )
		{
			// draw the contents of the last shader batch
			Tess_End();

#ifdef VOLUMETRIC_LIGHTING

			// draw the light volume if needed
			if ( light->shader->volumetricLight )
			{
				Render_lightVolume( ia );
			}

#endif

			if ( iaCount < ( backEnd.viewParms.numInteractions - 1 ) )
			{
				// jump to next interaction and continue
				ia++;
				iaCount++;
			}
			else
			{
				// increase last time to leave for loop
				iaCount++;
			}

			// force updates
			oldLight = NULL;
			oldEntity = NULL;
			oldShader = NULL;
		}
		else
		{
			// just continue
			ia = ia->next;
			iaCount++;
		}
	}

	// go back to the world modelview matrix
	GL_LoadModelViewMatrix( backEnd.viewParms.world.modelViewMatrix );

	if ( depthRange )
	{
		glDepthRange( 0, 1 );
	}

	// reset scissor
	GL_Scissor( backEnd.viewParms.viewportX, backEnd.viewParms.viewportY,
	            backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight );

	GL_CheckErrors();

	if ( r_speeds->integer == RSPEEDS_SHADING_TIMES )
	{
		glFinish();
		endTime = ri.Milliseconds();
		backEnd.pc.c_forwardLightingTime = endTime - startTime;
	}
}

/*
=================
RB_RenderInteractionsShadowMapped
=================
*/
static void RB_RenderInteractionsShadowMapped()
{
	shader_t       *shader, *oldShader;
	trRefEntity_t  *entity, *oldEntity;
	trRefLight_t   *light, *oldLight;
	interaction_t  *ia;
	int            iaCount;
	int            iaFirst;
	surfaceType_t  *surface;
	qboolean       depthRange, oldDepthRange;
	qboolean       alphaTest, oldAlphaTest;
	deformType_t   deformType, oldDeformType;
	vec3_t         tmp;
	matrix_t       modelToLight;
	qboolean       drawShadows;
	int            cubeSide;
	int            splitFrustumIndex;
	int            startTime = 0, endTime = 0;
	static const matrix_t bias = { 0.5,     0.0, 0.0, 0.0,
	                               0.0,     0.5, 0.0, 0.0,
	                               0.0,     0.0, 0.5, 0.0,
	                               0.5,     0.5, 0.5, 1.0
	                      };

	if ( !glConfig2.framebufferObjectAvailable || !glConfig2.textureFloatAvailable )
	{
		RB_RenderInteractions();
		return;
	}

	GLimp_LogComment( "--- RB_RenderInteractionsShadowMapped ---\n" );

	if ( r_speeds->integer == RSPEEDS_SHADING_TIMES )
	{
		glFinish();
		startTime = ri.Milliseconds();
	}

	// draw everything
	oldLight = NULL;
	oldEntity = NULL;
	oldShader = NULL;
	oldDepthRange = depthRange = qfalse;
	oldAlphaTest = alphaTest = qfalse;
	oldDeformType = deformType = DEFORM_TYPE_NONE;
	drawShadows = qtrue;
	cubeSide = 0;
	splitFrustumIndex = 0;

	// if we need to clear the FBO color buffers then it should be white
	GL_ClearColor( 1.0f, 1.0f, 1.0f, 1.0f );

	// render interactions
	for ( iaCount = 0, iaFirst = 0, ia = &backEnd.viewParms.interactions[ 0 ]; iaCount < backEnd.viewParms.numInteractions; )
	{
		backEnd.currentLight = light = ia->light;
		backEnd.currentEntity = entity = ia->entity;
		surface = ia->surface;
		shader = ia->surfaceShader;
		alphaTest = shader->alphaTest;

		if ( shader->numDeforms )
		{
			deformType = ShaderRequiresCPUDeforms( shader ) ? DEFORM_TYPE_CPU : DEFORM_TYPE_GPU;
		}
		else
		{
			deformType = DEFORM_TYPE_NONE;
		}

		if ( glConfig2.occlusionQueryBits && glConfig.driverType != GLDRV_MESA && r_dynamicLightOcclusionCulling->integer && !ia->occlusionQuerySamples )
		{
			// skip all interactions of this light because it failed the occlusion query
			goto skipInteraction;
		}

		if ( light->l.inverseShadows )
		{
			// handle those lights in RB_RenderInteractionsDeferredInverseShadows
			goto skipInteraction;
		}

		// only iaCount == iaFirst if first iteration or counters were reset
		if ( iaCount == iaFirst )
		{
			if ( drawShadows )
			{
				// HACK: bring OpenGL into a safe state or strange FBO update problems will occur
				GL_BindProgram( NULL );
				GL_State( GLS_DEFAULT );
				//GL_VertexAttribsState(ATTR_POSITION);

				GL_SelectTexture( 0 );
				GL_Bind( tr.whiteImage );

				if ( light->l.noShadows || light->shadowLOD < 0 )
				{
					if ( r_logFile->integer )
					{
						// don't just call LogComment, or we will get
						// a call to va() every frame!
						GLimp_LogComment( va( "----- Skipping shadowCube side: %i -----\n", cubeSide ) );
					}

					goto skipInteraction;
				}
				else
				{
					switch ( light->l.rlType )
					{
						case RL_OMNI:
							{
								//float           xMin, xMax, yMin, yMax;
								//float           width, height, depth;
								float    zNear, zFar;
								float    fovX, fovY;
								qboolean flipX, flipY;
								//float          *proj;
								vec3_t   angles;
								matrix_t rotationMatrix, transformMatrix, viewMatrix;

								if ( r_logFile->integer )
								{
									// don't just call LogComment, or we will get
									// a call to va() every frame!
									GLimp_LogComment( va( "----- Rendering shadowCube side: %i -----\n", cubeSide ) );
								}

								R_BindFBO( tr.shadowMapFBO[ light->shadowLOD ] );
								R_AttachFBOTexture2D( GL_TEXTURE_CUBE_MAP_POSITIVE_X + cubeSide,
								                      tr.shadowCubeFBOImage[ light->shadowLOD ]->texnum, 0 );

								if ( !r_ignoreGLErrors->integer )
								{
									R_CheckFBO( tr.shadowMapFBO[ light->shadowLOD ] );
								}

								// set the window clipping
								GL_Viewport( 0, 0, shadowMapResolutions[ light->shadowLOD ], shadowMapResolutions[ light->shadowLOD ] );
								GL_Scissor( 0, 0, shadowMapResolutions[ light->shadowLOD ], shadowMapResolutions[ light->shadowLOD ] );

								glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

								switch ( cubeSide )
								{
									case 0:
										{
											// view parameters
											VectorSet( angles, 0, 0, 90 );

											// projection parameters
											flipX = qfalse;
											flipY = qfalse;
											break;
										}

									case 1:
										{
											VectorSet( angles, 0, 180, 90 );
											flipX = qtrue;
											flipY = qtrue;
											break;
										}

									case 2:
										{
											VectorSet( angles, 0, 90, 0 );
											flipX = qfalse;
											flipY = qfalse;
											break;
										}

									case 3:
										{
											VectorSet( angles, 0, -90, 0 );
											flipX = qtrue;
											flipY = qtrue;
											break;
										}

									case 4:
										{
											VectorSet( angles, -90, 90, 0 );
											flipX = qfalse;
											flipY = qfalse;
											break;
										}

									case 5:
										{
											VectorSet( angles, 90, 90, 0 );
											flipX = qtrue;
											flipY = qtrue;
											break;
										}

									default:
										{
											// shut up compiler
											VectorSet( angles, 0, 0, 0 );
											flipX = qfalse;
											flipY = qfalse;
											break;
										}
								}

								// Quake -> OpenGL view matrix from light perspective
								MatrixFromAngles( rotationMatrix, angles[ PITCH ], angles[ YAW ], angles[ ROLL ] );
								MatrixSetupTransformFromRotation( transformMatrix, rotationMatrix, light->origin );
								MatrixAffineInverse( transformMatrix, viewMatrix );

								// convert from our coordinate system (looking down X)
								// to OpenGL's coordinate system (looking down -Z)
								MatrixMultiply( quakeToOpenGLMatrix, viewMatrix, light->viewMatrix );

								// OpenGL projection matrix
								fovX = 90;
								fovY = 90;

								zNear = 1.0;
								zFar = light->sphereRadius;

								if ( flipX )
								{
									fovX = -fovX;
								}

								if ( flipY )
								{
									fovY = -fovY;
								}

								MatrixPerspectiveProjectionFovXYRH( light->projectionMatrix, fovX, fovY, zNear, zFar );

								GL_LoadProjectionMatrix( light->projectionMatrix );
								break;
							}

						case RL_PROJ:
							{
								GLimp_LogComment( "--- Rendering projective shadowMap ---\n" );

								R_BindFBO( tr.shadowMapFBO[ light->shadowLOD ] );
								R_AttachFBOTexture2D( GL_TEXTURE_2D, tr.shadowMapFBOImage[ light->shadowLOD ]->texnum, 0 );

								if ( !r_ignoreGLErrors->integer )
								{
									R_CheckFBO( tr.shadowMapFBO[ light->shadowLOD ] );
								}

								// set the window clipping
								GL_Viewport( 0, 0, shadowMapResolutions[ light->shadowLOD ], shadowMapResolutions[ light->shadowLOD ] );
								GL_Scissor( 0, 0, shadowMapResolutions[ light->shadowLOD ], shadowMapResolutions[ light->shadowLOD ] );

								glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

								GL_LoadProjectionMatrix( light->projectionMatrix );
								break;
							}

						case RL_DIRECTIONAL:
							{
								int      j;
								vec3_t   angles;
								vec4_t   forward, side, up;
								vec3_t   lightDirection;
								vec3_t   viewOrigin, viewDirection;
								matrix_t rotationMatrix, transformMatrix, viewMatrix, projectionMatrix, viewProjectionMatrix;
								matrix_t cropMatrix;
								vec4_t   splitFrustum[ 6 ];
								vec3_t   splitFrustumCorners[ 8 ];
								vec3_t   splitFrustumBounds[ 2 ];
//							vec3_t     splitFrustumViewBounds[2];
								vec3_t   splitFrustumClipBounds[ 2 ];
//							float      splitFrustumRadius;
								int      numCasters;
								vec3_t   casterBounds[ 2 ];
								vec3_t   receiverBounds[ 2 ];
								vec3_t   cropBounds[ 2 ];
								vec4_t   point;
								vec4_t   transf;

								GLimp_LogComment( "--- Rendering directional shadowMap ---\n" );

								R_BindFBO( tr.sunShadowMapFBO[ splitFrustumIndex ] );

								if ( !r_evsmPostProcess->integer )
								{
									R_AttachFBOTexture2D( GL_TEXTURE_2D, tr.sunShadowMapFBOImage[ splitFrustumIndex ]->texnum, 0 );
								}
								else
								{
									R_AttachFBOTextureDepth( tr.sunShadowMapFBOImage[ splitFrustumIndex ]->texnum );
								}

								if ( !r_ignoreGLErrors->integer )
								{
									R_CheckFBO( tr.sunShadowMapFBO[ splitFrustumIndex ] );
								}

								// set the window clipping
								GL_Viewport( 0, 0, sunShadowMapResolutions[ splitFrustumIndex ], sunShadowMapResolutions[ splitFrustumIndex ] );
								GL_Scissor( 0, 0, sunShadowMapResolutions[ splitFrustumIndex ], sunShadowMapResolutions[ splitFrustumIndex ] );

								glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

#if 1
								VectorCopy( tr.sunDirection, lightDirection );
#else
								VectorCopy( light->direction, lightDirection );
#endif

								if ( r_parallelShadowSplits->integer )
								{
									// original light direction is from surface to light
									VectorInverse( lightDirection );
									VectorNormalize( lightDirection );

									VectorCopy( backEnd.viewParms.orientation.origin, viewOrigin );
									VectorCopy( backEnd.viewParms.orientation.axis[ 0 ], viewDirection );
									VectorNormalize( viewDirection );

#if 1
									// calculate new up dir
									CrossProduct( lightDirection, viewDirection, side );
									VectorNormalize( side );

									CrossProduct( side, lightDirection, up );
									VectorNormalize( up );

									VectorToAngles( lightDirection, angles );
									MatrixFromAngles( rotationMatrix, angles[ PITCH ], angles[ YAW ], angles[ ROLL ] );
									AngleVectors( angles, forward, side, up );

									MatrixLookAtRH( light->viewMatrix, viewOrigin, lightDirection, up );
#else
									MatrixLookAtRH( light->viewMatrix, viewOrigin, lightDirection, viewDirection );
#endif

									for ( j = 0; j < 6; j++ )
									{
										VectorCopy( backEnd.viewParms.frustums[ 1 + splitFrustumIndex ][ j ].normal, splitFrustum[ j ] );
										splitFrustum[ j ][ 3 ] = backEnd.viewParms.frustums[ 1 + splitFrustumIndex ][ j ].dist;
									}

									// calculate split frustum corner points
									PlanesGetIntersectionPoint( splitFrustum[ FRUSTUM_LEFT ], splitFrustum[ FRUSTUM_TOP ], splitFrustum[ FRUSTUM_NEAR ], splitFrustumCorners[ 0 ] );
									PlanesGetIntersectionPoint( splitFrustum[ FRUSTUM_RIGHT ], splitFrustum[ FRUSTUM_TOP ], splitFrustum[ FRUSTUM_NEAR ], splitFrustumCorners[ 1 ] );
									PlanesGetIntersectionPoint( splitFrustum[ FRUSTUM_RIGHT ], splitFrustum[ FRUSTUM_BOTTOM ], splitFrustum[ FRUSTUM_NEAR ], splitFrustumCorners[ 2 ] );
									PlanesGetIntersectionPoint( splitFrustum[ FRUSTUM_LEFT ], splitFrustum[ FRUSTUM_BOTTOM ], splitFrustum[ FRUSTUM_NEAR ], splitFrustumCorners[ 3 ] );

									PlanesGetIntersectionPoint( splitFrustum[ FRUSTUM_LEFT ], splitFrustum[ FRUSTUM_TOP ], splitFrustum[ FRUSTUM_FAR ], splitFrustumCorners[ 4 ] );
									PlanesGetIntersectionPoint( splitFrustum[ FRUSTUM_RIGHT ], splitFrustum[ FRUSTUM_TOP ], splitFrustum[ FRUSTUM_FAR ], splitFrustumCorners[ 5 ] );
									PlanesGetIntersectionPoint( splitFrustum[ FRUSTUM_RIGHT ], splitFrustum[ FRUSTUM_BOTTOM ], splitFrustum[ FRUSTUM_FAR ], splitFrustumCorners[ 6 ] );
									PlanesGetIntersectionPoint( splitFrustum[ FRUSTUM_LEFT ], splitFrustum[ FRUSTUM_BOTTOM ], splitFrustum[ FRUSTUM_FAR ], splitFrustumCorners[ 7 ] );

									if ( r_logFile->integer )
									{
										vec3_t rayIntersectionNear, rayIntersectionFar;
										float  zNear, zFar;

										// don't just call LogComment, or we will get
										// a call to va() every frame!
										//GLimp_LogComment(va("----- Skipping shadowCube side: %i -----\n", cubeSide));

										PlaneIntersectRay( viewOrigin, viewDirection, splitFrustum[ FRUSTUM_FAR ], rayIntersectionFar );
										zFar = Distance( viewOrigin, rayIntersectionFar );

										VectorInverse( viewDirection );

										PlaneIntersectRay( rayIntersectionFar, viewDirection, splitFrustum[ FRUSTUM_NEAR ], rayIntersectionNear );
										zNear = Distance( viewOrigin, rayIntersectionNear );

										VectorInverse( viewDirection );

										GLimp_LogComment( va( "split frustum %i: near = %5.3f, far = %5.3f\n", splitFrustumIndex, zNear, zFar ) );
										GLimp_LogComment( va( "pyramid nearCorners\n" ) );

										for ( j = 0; j < 4; j++ )
										{
											GLimp_LogComment( va( "(%5.3f, %5.3f, %5.3f)\n", splitFrustumCorners[ j ][ 0 ], splitFrustumCorners[ j ][ 1 ], splitFrustumCorners[ j ][ 2 ] ) );
										}

										GLimp_LogComment( va( "pyramid farCorners\n" ) );

										for ( j = 4; j < 8; j++ )
										{
											GLimp_LogComment( va( "(%5.3f, %5.3f, %5.3f)\n", splitFrustumCorners[ j ][ 0 ], splitFrustumCorners[ j ][ 1 ], splitFrustumCorners[ j ][ 2 ] ) );
										}
									}

									ClearBounds( splitFrustumBounds[ 0 ], splitFrustumBounds[ 1 ] );

									for ( j = 0; j < 8; j++ )
									{
										AddPointToBounds( splitFrustumCorners[ j ], splitFrustumBounds[ 0 ], splitFrustumBounds[ 1 ] );
									}

#if 0
									//
									// Scene-Independent Projection
									//

									// find the bounding box of the current split in the light's view space
									ClearBounds( splitFrustumViewBounds[ 0 ], splitFrustumViewBounds[ 1 ] );

									//numCasters = MergeInteractionBounds(light->viewMatrix, ia, iaCount, splitFrustumViewBounds, qtrue);
									for ( j = 0; j < 8; j++ )
									{
										VectorCopy( splitFrustumCorners[ j ], point );
										point[ 3 ] = 1;

										MatrixTransform4( light->viewMatrix, point, transf );
										transf[ 0 ] /= transf[ 3 ];
										transf[ 1 ] /= transf[ 3 ];
										transf[ 2 ] /= transf[ 3 ];

										AddPointToBounds( transf, splitFrustumViewBounds[ 0 ], splitFrustumViewBounds[ 1 ] );
									}

									//MatrixScaleTranslateToUnitCube(projectionMatrix, splitFrustumViewBounds[0], splitFrustumViewBounds[1]);
									//MatrixOrthogonalProjectionRH(projectionMatrix, -1, 1, -1, 1, -splitFrustumViewBounds[1][2], -splitFrustumViewBounds[0][2]);
#if 1
									MatrixOrthogonalProjectionRH( projectionMatrix,  splitFrustumViewBounds[ 0 ][ 0 ],
									                              splitFrustumViewBounds[ 1 ][ 0 ],
									                              splitFrustumViewBounds[ 0 ][ 1 ],
									                              splitFrustumViewBounds[ 1 ][ 1 ],
									                              -splitFrustumViewBounds[ 1 ][ 2 ],
									                              -splitFrustumViewBounds[ 0 ][ 2 ] );
#endif
									MatrixMultiply( projectionMatrix, light->viewMatrix, viewProjectionMatrix );

									// find the bounding box of the current split in the light's clip space
									ClearBounds( splitFrustumClipBounds[ 0 ], splitFrustumClipBounds[ 1 ] );

									for ( j = 0; j < 8; j++ )
									{
										VectorCopy( splitFrustumCorners[ j ], point );
										point[ 3 ] = 1;

										MatrixTransform4( viewProjectionMatrix, point, transf );
										transf[ 0 ] /= transf[ 3 ];
										transf[ 1 ] /= transf[ 3 ];
										transf[ 2 ] /= transf[ 3 ];

										AddPointToBounds( transf, splitFrustumClipBounds[ 0 ], splitFrustumClipBounds[ 1 ] );
									}

									splitFrustumClipBounds[ 0 ][ 2 ] = 0;
									splitFrustumClipBounds[ 1 ][ 2 ] = 1;

									MatrixCrop( cropMatrix, splitFrustumClipBounds[ 0 ], splitFrustumClipBounds[ 1 ] );
									//MatrixIdentity(cropMatrix);

									if ( r_logFile->integer )
									{
										GLimp_LogComment( va( "split frustum light view space bounds (%5.3f, %5.3f, %5.3f) (%5.3f, %5.3f, %5.3f)\n",
										                      splitFrustumViewBounds[ 0 ][ 0 ], splitFrustumViewBounds[ 0 ][ 1 ], splitFrustumViewBounds[ 0 ][ 2 ],
										                      splitFrustumViewBounds[ 1 ][ 0 ], splitFrustumViewBounds[ 1 ][ 1 ], splitFrustumViewBounds[ 1 ][ 2 ] ) );

										GLimp_LogComment( va( "split frustum light clip space bounds (%5.3f, %5.3f, %5.3f) (%5.3f, %5.3f, %5.3f)\n",
										                      splitFrustumClipBounds[ 0 ][ 0 ], splitFrustumClipBounds[ 0 ][ 1 ], splitFrustumClipBounds[ 0 ][ 2 ],
										                      splitFrustumClipBounds[ 1 ][ 0 ], splitFrustumClipBounds[ 1 ][ 1 ], splitFrustumClipBounds[ 1 ][ 2 ] ) );
									}

#else

									//
									// Scene-Dependent Projection
									//

									// find the bounding box of the current split in the light's view space
									ClearBounds( cropBounds[ 0 ], cropBounds[ 1 ] );

									for ( j = 0; j < 8; j++ )
									{
										VectorCopy( splitFrustumCorners[ j ], point );
										point[ 3 ] = 1;
#if 1
										MatrixTransform4( light->viewMatrix, point, transf );
										transf[ 0 ] /= transf[ 3 ];
										transf[ 1 ] /= transf[ 3 ];
										transf[ 2 ] /= transf[ 3 ];
#else
										MatrixTransformPoint( light->viewMatrix, point, transf );
#endif

										AddPointToBounds( transf, cropBounds[ 0 ], cropBounds[ 1 ] );
									}

									MatrixOrthogonalProjectionRH( projectionMatrix, cropBounds[ 0 ][ 0 ], cropBounds[ 1 ][ 0 ], cropBounds[ 0 ][ 1 ], cropBounds[ 1 ][ 1 ], -cropBounds[ 1 ][ 2 ], -cropBounds[ 0 ][ 2 ] );

									MatrixMultiply( projectionMatrix, light->viewMatrix, viewProjectionMatrix );

									numCasters = MergeInteractionBounds( viewProjectionMatrix, ia, iaCount, casterBounds, qtrue );
									MergeInteractionBounds( viewProjectionMatrix, ia, iaCount, receiverBounds, qfalse );

									// find the bounding box of the current split in the light's clip space
									ClearBounds( splitFrustumClipBounds[ 0 ], splitFrustumClipBounds[ 1 ] );

									for ( j = 0; j < 8; j++ )
									{
										VectorCopy( splitFrustumCorners[ j ], point );
										point[ 3 ] = 1;

										MatrixTransform4( viewProjectionMatrix, point, transf );
										transf[ 0 ] /= transf[ 3 ];
										transf[ 1 ] /= transf[ 3 ];
										transf[ 2 ] /= transf[ 3 ];

										AddPointToBounds( transf, splitFrustumClipBounds[ 0 ], splitFrustumClipBounds[ 1 ] );
									}

									if ( r_logFile->integer )
									{
										GLimp_LogComment( va( "shadow casters = %i\n", numCasters ) );

										GLimp_LogComment( va( "split frustum light space clip bounds (%5.3f, %5.3f, %5.3f) (%5.3f, %5.3f, %5.3f)\n",
										                      splitFrustumClipBounds[ 0 ][ 0 ], splitFrustumClipBounds[ 0 ][ 1 ], splitFrustumClipBounds[ 0 ][ 2 ],
										                      splitFrustumClipBounds[ 1 ][ 0 ], splitFrustumClipBounds[ 1 ][ 1 ], splitFrustumClipBounds[ 1 ][ 2 ] ) );

										GLimp_LogComment( va( "shadow caster light space clip bounds (%5.3f, %5.3f, %5.3f) (%5.3f, %5.3f, %5.3f)\n",
										                      casterBounds[ 0 ][ 0 ], casterBounds[ 0 ][ 1 ], casterBounds[ 0 ][ 2 ],
										                      casterBounds[ 1 ][ 0 ], casterBounds[ 1 ][ 1 ], casterBounds[ 1 ][ 2 ] ) );

										GLimp_LogComment( va( "light receiver light space clip bounds (%5.3f, %5.3f, %5.3f) (%5.3f, %5.3f, %5.3f)\n",
										                      receiverBounds[ 0 ][ 0 ], receiverBounds[ 0 ][ 1 ], receiverBounds[ 0 ][ 2 ],
										                      receiverBounds[ 1 ][ 0 ], receiverBounds[ 1 ][ 1 ], receiverBounds[ 1 ][ 2 ] ) );
									}

									// scene-dependent bounding volume
									cropBounds[ 0 ][ 0 ] = Q_max( Q_max( casterBounds[ 0 ][ 0 ], receiverBounds[ 0 ][ 0 ] ), splitFrustumClipBounds[ 0 ][ 0 ] );
									cropBounds[ 0 ][ 1 ] = Q_max( Q_max( casterBounds[ 0 ][ 1 ], receiverBounds[ 0 ][ 1 ] ), splitFrustumClipBounds[ 0 ][ 1 ] );

									cropBounds[ 1 ][ 0 ] = Q_min( Q_min( casterBounds[ 1 ][ 0 ], receiverBounds[ 1 ][ 0 ] ), splitFrustumClipBounds[ 1 ][ 0 ] );
									cropBounds[ 1 ][ 1 ] = Q_min( Q_min( casterBounds[ 1 ][ 1 ], receiverBounds[ 1 ][ 1 ] ), splitFrustumClipBounds[ 1 ][ 1 ] );

									cropBounds[ 0 ][ 2 ] = Q_min( casterBounds[ 0 ][ 2 ], splitFrustumClipBounds[ 0 ][ 2 ] );
									//cropBounds[0][2] = casterBounds[0][2];
									//cropBounds[0][2] = splitFrustumClipBounds[0][2];
									cropBounds[ 1 ][ 2 ] = Q_min( receiverBounds[ 1 ][ 2 ], splitFrustumClipBounds[ 1 ][ 2 ] );
									//cropBounds[1][2] = splitFrustumClipBounds[1][2];

									if ( numCasters == 0 )
									{
										VectorCopy( splitFrustumClipBounds[ 0 ], cropBounds[ 0 ] );
										VectorCopy( splitFrustumClipBounds[ 1 ], cropBounds[ 1 ] );
									}

									MatrixCrop( cropMatrix, cropBounds[ 0 ], cropBounds[ 1 ] );
#endif

									MatrixMultiply( cropMatrix, projectionMatrix, light->projectionMatrix );

									GL_LoadProjectionMatrix( light->projectionMatrix );
								}
								else
								{
									// original light direction is from surface to light
									VectorInverse( lightDirection );

									// Quake -> OpenGL view matrix from light perspective
#if 1
									VectorToAngles( lightDirection, angles );
									MatrixFromAngles( rotationMatrix, angles[ PITCH ], angles[ YAW ], angles[ ROLL ] );
									MatrixSetupTransformFromRotation( transformMatrix, rotationMatrix, backEnd.viewParms.orientation.origin );
									MatrixAffineInverse( transformMatrix, viewMatrix );
									MatrixMultiply( quakeToOpenGLMatrix, viewMatrix, light->viewMatrix );
#else
									MatrixLookAtRH( light->viewMatrix, backEnd.viewParms.orientation.origin, lightDirection, backEnd.viewParms.orientation.axis[ 0 ] );
#endif

									ClearBounds( splitFrustumBounds[ 0 ], splitFrustumBounds[ 1 ] );
									//BoundsAdd(splitFrustumBounds[0], splitFrustumBounds[1], backEnd.viewParms.visBounds[0], backEnd.viewParms.visBounds[1]);
									BoundsAdd( splitFrustumBounds[ 0 ], splitFrustumBounds[ 1 ], light->worldBounds[ 0 ], light->worldBounds[ 1 ] );

									ClearBounds( cropBounds[ 0 ], cropBounds[ 1 ] );

									for ( j = 0; j < 8; j++ )
									{
										point[ 0 ] = splitFrustumBounds[ j & 1 ][ 0 ];
										point[ 1 ] = splitFrustumBounds[( j >> 1 ) & 1 ][ 1 ];
										point[ 2 ] = splitFrustumBounds[( j >> 2 ) & 1 ][ 2 ];
										point[ 3 ] = 1;

										MatrixTransform4( light->viewMatrix, point, transf );
										transf[ 0 ] /= transf[ 3 ];
										transf[ 1 ] /= transf[ 3 ];
										transf[ 2 ] /= transf[ 3 ];

										AddPointToBounds( transf, cropBounds[ 0 ], cropBounds[ 1 ] );
									}

									// transform from OpenGL's right handed into D3D's left handed coordinate system
#if 0
									MatrixScaleTranslateToUnitCube( projectionMatrix, cropBounds[ 0 ], cropBounds[ 1 ] );
									MatrixMultiply( flipZMatrix, projectionMatrix, light->projectionMatrix );
#else
									MatrixOrthogonalProjectionRH( light->projectionMatrix, cropBounds[ 0 ][ 0 ], cropBounds[ 1 ][ 0 ], cropBounds[ 0 ][ 1 ], cropBounds[ 1 ][ 1 ], -cropBounds[ 1 ][ 2 ], -cropBounds[ 0 ][ 2 ] );
#endif
									GL_LoadProjectionMatrix( light->projectionMatrix );
								}

								break;
							}

						default:
							break;
					}
				}

				if ( r_logFile->integer )
				{
					// don't just call LogComment, or we will get
					// a call to va() every frame!
					GLimp_LogComment( va( "----- First Shadow Interaction: %i -----\n", iaCount ) );
				}
			}
			else
			{
				GLimp_LogComment( "--- Rendering lighting ---\n" );

				if ( r_logFile->integer )
				{
					// don't just call LogComment, or we will get
					// a call to va() every frame!
					GLimp_LogComment( va( "----- First Light Interaction: %i -----\n", iaCount ) );
				}

				if ( r_hdrRendering->integer )
				{
					R_BindFBO( tr.deferredRenderFBO );
				}
				else
				{
					R_BindNullFBO();
				}

				// set the window clipping
				GL_Viewport( backEnd.viewParms.viewportX, backEnd.viewParms.viewportY,
				             backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight );

				GL_Scissor( backEnd.viewParms.viewportX, backEnd.viewParms.viewportY,
				            backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight );

				// restore camera matrices
				GL_LoadProjectionMatrix( backEnd.viewParms.projectionMatrix );
				GL_LoadModelViewMatrix( backEnd.orientation.modelViewMatrix );

				// reset light view and projection matrices
				switch ( light->l.rlType )
				{
					case RL_OMNI:
						{
							MatrixAffineInverse( light->transformMatrix, light->viewMatrix );
							MatrixSetupScale( light->projectionMatrix, 1.0 / light->l.radius[ 0 ], 1.0 / light->l.radius[ 1 ],
							                  1.0 / light->l.radius[ 2 ] );
							break;
						}

					case RL_DIRECTIONAL:
						{
							// draw split frustum shadow maps
							if ( r_showShadowMaps->integer )
							{
								int      frustumIndex;
								float    x, y, w, h;
								matrix_t ortho;
								vec4_t   quadVerts[ 4 ];

								// set 2D virtual screen size
								GL_PushMatrix();
								MatrixOrthogonalProjection( ortho, backEnd.viewParms.viewportX,
								                            backEnd.viewParms.viewportX + backEnd.viewParms.viewportWidth,
								                            backEnd.viewParms.viewportY,
								                            backEnd.viewParms.viewportY + backEnd.viewParms.viewportHeight, -99999, 99999 );
								GL_LoadProjectionMatrix( ortho );
								GL_LoadModelViewMatrix( matrixIdentity );

								for ( frustumIndex = 0; frustumIndex <= r_parallelShadowSplits->integer; frustumIndex++ )
								{
									GL_Cull( CT_TWO_SIDED );
									GL_State( GLS_DEPTHTEST_DISABLE );

									gl_debugShadowMapShader->BindProgram();
									gl_debugShadowMapShader->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );

									GL_SelectTexture( 0 );
									GL_Bind( tr.sunShadowMapFBOImage[ frustumIndex ] );

									w = 200;
									h = 200;

									x = 205 * frustumIndex;
									y = 70;

									Vector4Set( quadVerts[ 0 ], x, y, 0, 1 );
									Vector4Set( quadVerts[ 1 ], x + w, y, 0, 1 );
									Vector4Set( quadVerts[ 2 ], x + w, y + h, 0, 1 );
									Vector4Set( quadVerts[ 3 ], x, y + h, 0, 1 );

									Tess_InstantQuad( quadVerts );

									{
										int    j;
										vec4_t splitFrustum[ 6 ];
										vec3_t farCorners[ 4 ];
										vec3_t nearCorners[ 4 ];

										GL_Viewport( x, y, w, h );
										GL_Scissor( x, y, w, h );

										GL_PushMatrix();

										gl_genericShader->DisableAlphaTesting();
										gl_genericShader->DisablePortalClipping();
										gl_genericShader->DisableVertexSkinning();
										gl_genericShader->DisableVertexAnimation();
										gl_genericShader->DisableDeformVertexes();
										gl_genericShader->DisableTCGenEnvironment();

										gl_genericShader->BindProgram();

										// set uniforms
										gl_genericShader->SetUniform_ColorModulate( CGEN_VERTEX, AGEN_VERTEX );
										gl_genericShader->SetUniform_Color( colorBlack );

										GL_State( GLS_POLYMODE_LINE | GLS_DEPTHTEST_DISABLE );
										GL_Cull( CT_TWO_SIDED );

										// bind u_ColorMap
										GL_SelectTexture( 0 );
										GL_Bind( tr.whiteImage );
										gl_genericShader->SetUniform_ColorTextureMatrix( matrixIdentity );

										gl_genericShader->SetUniform_ModelViewProjectionMatrix( light->shadowMatrices[ frustumIndex ] );

										tess.multiDrawPrimitives = 0;
										tess.numIndexes = 0;
										tess.numVertexes = 0;

										for ( j = 0; j < 6; j++ )
										{
											VectorCopy( backEnd.viewParms.frustums[ 1 + frustumIndex ][ j ].normal, splitFrustum[ j ] );
											splitFrustum[ j ][ 3 ] = backEnd.viewParms.frustums[ 1 + frustumIndex ][ j ].dist;
										}

										// calculate split frustum corner points
										PlanesGetIntersectionPoint( splitFrustum[ FRUSTUM_LEFT ], splitFrustum[ FRUSTUM_TOP ], splitFrustum[ FRUSTUM_NEAR ], nearCorners[ 0 ] );
										PlanesGetIntersectionPoint( splitFrustum[ FRUSTUM_RIGHT ], splitFrustum[ FRUSTUM_TOP ], splitFrustum[ FRUSTUM_NEAR ], nearCorners[ 1 ] );
										PlanesGetIntersectionPoint( splitFrustum[ FRUSTUM_RIGHT ], splitFrustum[ FRUSTUM_BOTTOM ], splitFrustum[ FRUSTUM_NEAR ], nearCorners[ 2 ] );
										PlanesGetIntersectionPoint( splitFrustum[ FRUSTUM_LEFT ], splitFrustum[ FRUSTUM_BOTTOM ], splitFrustum[ FRUSTUM_NEAR ], nearCorners[ 3 ] );

										PlanesGetIntersectionPoint( splitFrustum[ FRUSTUM_LEFT ], splitFrustum[ FRUSTUM_TOP ], splitFrustum[ FRUSTUM_FAR ], farCorners[ 0 ] );
										PlanesGetIntersectionPoint( splitFrustum[ FRUSTUM_RIGHT ], splitFrustum[ FRUSTUM_TOP ], splitFrustum[ FRUSTUM_FAR ], farCorners[ 1 ] );
										PlanesGetIntersectionPoint( splitFrustum[ FRUSTUM_RIGHT ], splitFrustum[ FRUSTUM_BOTTOM ], splitFrustum[ FRUSTUM_FAR ], farCorners[ 2 ] );
										PlanesGetIntersectionPoint( splitFrustum[ FRUSTUM_LEFT ], splitFrustum[ FRUSTUM_BOTTOM ], splitFrustum[ FRUSTUM_FAR ], farCorners[ 3 ] );

										// draw outer surfaces
										for ( j = 0; j < 4; j++ )
										{
											Vector4Set( quadVerts[ 0 ], nearCorners[ j ][ 0 ], nearCorners[ j ][ 1 ], nearCorners[ j ][ 2 ], 1 );
											Vector4Set( quadVerts[ 1 ], farCorners[ j ][ 0 ], farCorners[ j ][ 1 ], farCorners[ j ][ 2 ], 1 );
											Vector4Set( quadVerts[ 2 ], farCorners[( j + 1 ) % 4 ][ 0 ], farCorners[( j + 1 ) % 4 ][ 1 ], farCorners[( j + 1 ) % 4 ][ 2 ], 1 );
											Vector4Set( quadVerts[ 3 ], nearCorners[( j + 1 ) % 4 ][ 0 ], nearCorners[( j + 1 ) % 4 ][ 1 ], nearCorners[( j + 1 ) % 4 ][ 2 ], 1 );
											Tess_AddQuadStamp2( quadVerts, colorCyan );
										}

										// draw far cap
										Vector4Set( quadVerts[ 0 ], farCorners[ 3 ][ 0 ], farCorners[ 3 ][ 1 ], farCorners[ 3 ][ 2 ], 1 );
										Vector4Set( quadVerts[ 1 ], farCorners[ 2 ][ 0 ], farCorners[ 2 ][ 1 ], farCorners[ 2 ][ 2 ], 1 );
										Vector4Set( quadVerts[ 2 ], farCorners[ 1 ][ 0 ], farCorners[ 1 ][ 1 ], farCorners[ 1 ][ 2 ], 1 );
										Vector4Set( quadVerts[ 3 ], farCorners[ 0 ][ 0 ], farCorners[ 0 ][ 1 ], farCorners[ 0 ][ 2 ], 1 );
										Tess_AddQuadStamp2( quadVerts, colorBlue );

										// draw near cap
										Vector4Set( quadVerts[ 0 ], nearCorners[ 0 ][ 0 ], nearCorners[ 0 ][ 1 ], nearCorners[ 0 ][ 2 ], 1 );
										Vector4Set( quadVerts[ 1 ], nearCorners[ 1 ][ 0 ], nearCorners[ 1 ][ 1 ], nearCorners[ 1 ][ 2 ], 1 );
										Vector4Set( quadVerts[ 2 ], nearCorners[ 2 ][ 0 ], nearCorners[ 2 ][ 1 ], nearCorners[ 2 ][ 2 ], 1 );
										Vector4Set( quadVerts[ 3 ], nearCorners[ 3 ][ 0 ], nearCorners[ 3 ][ 1 ], nearCorners[ 3 ][ 2 ], 1 );
										Tess_AddQuadStamp2( quadVerts, colorGreen );

										Tess_UpdateVBOs( ATTR_POSITION | ATTR_COLOR );
										Tess_DrawElements();

										// draw light volume
										if ( light->isStatic && light->frustumVBO && light->frustumIBO )
										{
											gl_genericShader->SetUniform_ColorModulate( CGEN_CUSTOM_RGB, AGEN_CUSTOM );
											gl_genericShader->SetUniform_Color( colorYellow );

											R_BindVBO( light->frustumVBO );
											R_BindIBO( light->frustumIBO );

											GL_VertexAttribsState( ATTR_POSITION );

											tess.numVertexes = light->frustumVerts;
											tess.numIndexes = light->frustumIndexes;

											Tess_DrawElements();
										}

										tess.multiDrawPrimitives = 0;
										tess.numIndexes = 0;
										tess.numVertexes = 0;

										GL_PopMatrix();

										GL_Viewport( backEnd.viewParms.viewportX, backEnd.viewParms.viewportY,
										             backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight );

										GL_Scissor( backEnd.viewParms.viewportX, backEnd.viewParms.viewportY,
										            backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight );
									}
								}

								GL_PopMatrix();
							}
						}

					default:
						break;
				}
			}
		} // end if(iaCount == iaFirst)

		if ( drawShadows )
		{
			if ( entity->e.renderfx & ( RF_NOSHADOW | RF_DEPTHHACK ) )
			{
				goto skipInteraction;
			}

			if ( shader->isSky )
			{
				goto skipInteraction;
			}

			if ( shader->sort > SS_OPAQUE )
			{
				goto skipInteraction;
			}

			if ( shader->noShadows || light->l.noShadows || light->shadowLOD < 0 )
			{
				goto skipInteraction;
			}

			/*
			   if(light->l.inverseShadows && (entity == &tr.worldEntity))
			   {
			   // this light only casts shadows by its player and their items
			   goto skipInteraction;
			   }
			 */

			if ( ia->type == IA_LIGHTONLY )
			{
				goto skipInteraction;
			}

			if ( light->l.rlType == RL_OMNI && !( ia->cubeSideBits & ( 1 << cubeSide ) ) )
			{
				goto skipInteraction;
			}

			switch ( light->l.rlType )
			{
				case RL_OMNI:
				case RL_PROJ:
				case RL_DIRECTIONAL:
					{
						if ( light == oldLight && entity == oldEntity && ( alphaTest ? shader == oldShader : alphaTest == oldAlphaTest ) && deformType == oldDeformType )
						{
							if ( r_logFile->integer )
							{
								// don't just call LogComment, or we will get
								// a call to va() every frame!
								GLimp_LogComment( va( "----- Batching Shadow Interaction: %i -----\n", iaCount ) );
							}

							// fast path, same as previous
							rb_surfaceTable[ *surface ]( surface );
							goto nextInteraction;
						}
						else
						{
							if ( oldLight )
							{
								// draw the contents of the last shader batch
								Tess_End();
							}

							if ( r_logFile->integer )
							{
								// don't just call LogComment, or we will get
								// a call to va() every frame!
								GLimp_LogComment( va( "----- Beginning Shadow Interaction: %i -----\n", iaCount ) );
							}

							// we don't need tangent space calculations here
							Tess_Begin( Tess_StageIteratorShadowFill, NULL, shader, light->shader, qtrue, qfalse, -1, 0 );
						}

						break;
					}

				default:
					break;
			}
		}
		else
		{
			if ( !shader->interactLight )
			{
				goto skipInteraction;
			}

			if ( ia->type == IA_SHADOWONLY )
			{
				goto skipInteraction;
			}

			if ( glConfig2.occlusionQueryBits && glConfig.driverType != GLDRV_MESA && r_dynamicEntityOcclusionCulling->integer && !entity->occlusionQuerySamples )
			{
				goto skipInteraction;
			}

			if ( light == oldLight && entity == oldEntity && shader == oldShader )
			{
				if ( r_logFile->integer )
				{
					// don't just call LogComment, or we will get
					// a call to va() every frame!
					GLimp_LogComment( va( "----- Batching Light Interaction: %i -----\n", iaCount ) );
				}

				// fast path, same as previous
				rb_surfaceTable[ *surface ]( surface );
				goto nextInteraction;
			}
			else
			{
				if ( oldLight )
				{
					// draw the contents of the last shader batch
					Tess_End();
				}

				if ( r_logFile->integer )
				{
					// don't just call LogComment, or we will get
					// a call to va() every frame!
					GLimp_LogComment( va( "----- Beginning Light Interaction: %i -----\n", iaCount ) );
				}

				// begin a new batch
				Tess_Begin( Tess_StageIteratorLighting, NULL, shader, light->shader, light->l.inverseShadows, qfalse, -1, 0 );
			}
		}

		// change the modelview matrix if needed
		if ( entity != oldEntity )
		{
			depthRange = qfalse;

			if ( entity != &tr.worldEntity )
			{
				// set up the transformation matrix
				if ( drawShadows )
				{
					R_RotateEntityForLight( entity, light, &backEnd.orientation );
				}
				else
				{
					R_RotateEntityForViewParms( entity, &backEnd.viewParms, &backEnd.orientation );
				}

				if ( entity->e.renderfx & RF_DEPTHHACK )
				{
					// hack the depth range to prevent view model from poking into walls
					depthRange = qtrue;
				}
			}
			else
			{
				// set up the transformation matrix
				if ( drawShadows )
				{
					Com_Memset( &backEnd.orientation, 0, sizeof( backEnd.orientation ) );

					backEnd.orientation.axis[ 0 ][ 0 ] = 1;
					backEnd.orientation.axis[ 1 ][ 1 ] = 1;
					backEnd.orientation.axis[ 2 ][ 2 ] = 1;
					VectorCopy( light->l.origin, backEnd.orientation.viewOrigin );

					MatrixIdentity( backEnd.orientation.transformMatrix );
					//MatrixAffineInverse(backEnd.orientation.transformMatrix, backEnd.orientation.viewMatrix);
					MatrixMultiply( light->viewMatrix, backEnd.orientation.transformMatrix, backEnd.orientation.viewMatrix );
					MatrixCopy( backEnd.orientation.viewMatrix, backEnd.orientation.modelViewMatrix );
				}
				else
				{
					// transform by the camera placement
					backEnd.orientation = backEnd.viewParms.world;
				}
			}

			GL_LoadModelViewMatrix( backEnd.orientation.modelViewMatrix );

			// change depthrange if needed
			if ( oldDepthRange != depthRange )
			{
				if ( depthRange )
				{
					glDepthRange( 0, 0.3 );
				}
				else
				{
					glDepthRange( 0, 1 );
				}

				oldDepthRange = depthRange;
			}
		}

		// change the attenuation matrix if needed
		if ( light != oldLight || entity != oldEntity )
		{
			// transform light origin into model space for u_LightOrigin parameter
			if ( entity != &tr.worldEntity )
			{
				VectorSubtract( light->origin, backEnd.orientation.origin, tmp );
				light->transformed[ 0 ] = DotProduct( tmp, backEnd.orientation.axis[ 0 ] );
				light->transformed[ 1 ] = DotProduct( tmp, backEnd.orientation.axis[ 1 ] );
				light->transformed[ 2 ] = DotProduct( tmp, backEnd.orientation.axis[ 2 ] );
			}
			else
			{
				VectorCopy( light->origin, light->transformed );
			}

			MatrixMultiply( light->viewMatrix, backEnd.orientation.transformMatrix, modelToLight );

			// build the attenuation matrix using the entity transform
			switch ( light->l.rlType )
			{
				case RL_OMNI:
					{
						MatrixSetupTranslation( light->attenuationMatrix, 0.5, 0.5, 0.5 );  // bias
						MatrixMultiplyScale( light->attenuationMatrix, 0.5, 0.5, 0.5 );  // scale
						MatrixMultiply2( light->attenuationMatrix, light->projectionMatrix );
						MatrixMultiply2( light->attenuationMatrix, modelToLight );

						MatrixCopy( light->attenuationMatrix, light->shadowMatrices[ 0 ] );
						break;
					}

				case RL_PROJ:
					{
						MatrixSetupTranslation( light->attenuationMatrix, 0.5, 0.5, 0.0 );  // bias
						MatrixMultiplyScale( light->attenuationMatrix, 0.5, 0.5, 1.0 / Q_min( light->falloffLength, 1.0 ) );   // scale
						MatrixMultiply2( light->attenuationMatrix, light->projectionMatrix );
						MatrixMultiply2( light->attenuationMatrix, modelToLight );

						MatrixCopy( light->attenuationMatrix, light->shadowMatrices[ 0 ] );
						break;
					}

				case RL_DIRECTIONAL:
					{
						MatrixSetupTranslation( light->attenuationMatrix, 0.5, 0.5, 0.5 );  // bias
						MatrixMultiplyScale( light->attenuationMatrix, 0.5, 0.5, 0.5 );  // scale
						MatrixMultiply2( light->attenuationMatrix, light->projectionMatrix );
						MatrixMultiply2( light->attenuationMatrix, modelToLight );
						break;
					}

				case RL_MAX_REF_LIGHT_TYPE:
					{
						//Nothing for right now...
						break;
					}
			}
		}

		if ( drawShadows )
		{
			switch ( light->l.rlType )
			{
				case RL_OMNI:
				case RL_PROJ:
				case RL_DIRECTIONAL:
					{
						// add the triangles for this surface
						rb_surfaceTable[ *surface ]( surface );
						break;
					}

				default:
					break;
			}
		}
		else
		{
			// add the triangles for this surface
			rb_surfaceTable[ *surface ]( surface );
		}

nextInteraction:

		// remember values
		oldLight = light;
		oldEntity = entity;
		oldShader = shader;
		oldAlphaTest = alphaTest;
		oldDeformType = deformType;

skipInteraction:

		if ( !ia->next )
		{
			// if ia->next does not point to any other interaction then
			// this is the last interaction of the current light

			if ( r_logFile->integer )
			{
				// don't just call LogComment, or we will get
				// a call to va() every frame!
				GLimp_LogComment( va( "----- Last Interaction: %i -----\n", iaCount ) );
			}

			// draw the contents of the last shader batch
			Tess_End();

			if ( drawShadows )
			{
				switch ( light->l.rlType )
				{
					case RL_OMNI:
						{
							if ( cubeSide == 5 )
							{
								cubeSide = 0;
								drawShadows = qfalse;
							}
							else
							{
								cubeSide++;
							}

							// jump back to first interaction of this light
							ia = &backEnd.viewParms.interactions[ iaFirst ];
							iaCount = iaFirst;
							break;
						}

					case RL_PROJ:
						{
							// jump back to first interaction of this light and start lighting
							ia = &backEnd.viewParms.interactions[ iaFirst ];
							iaCount = iaFirst;
							drawShadows = qfalse;
							break;
						}

					case RL_DIRECTIONAL:
						{
							// set shadow matrix including scale + offset
							MatrixCopy( bias, light->shadowMatricesBiased[ splitFrustumIndex ] );
							MatrixMultiply2( light->shadowMatricesBiased[ splitFrustumIndex ], light->projectionMatrix );
							MatrixMultiply2( light->shadowMatricesBiased[ splitFrustumIndex ], light->viewMatrix );

							MatrixMultiply( light->projectionMatrix, light->viewMatrix, light->shadowMatrices[ splitFrustumIndex ] );

							if ( r_parallelShadowSplits->integer )
							{
								if ( splitFrustumIndex == r_parallelShadowSplits->integer )
								{
									splitFrustumIndex = 0;
									drawShadows = qfalse;
								}
								else
								{
									splitFrustumIndex++;
								}

								// jump back to first interaction of this light
								ia = &backEnd.viewParms.interactions[ iaFirst ];
								iaCount = iaFirst;
							}
							else
							{
								// jump back to first interaction of this light and start lighting
								ia = &backEnd.viewParms.interactions[ iaFirst ];
								iaCount = iaFirst;
								drawShadows = qfalse;
							}

							break;
						}

					default:
						break;
				}
			}
			else
			{
#ifdef VOLUMETRIC_LIGHTING

				// draw the light volume if needed
				if ( light->shader->volumetricLight )
				{
					Render_lightVolume( ia );
				}

#endif

				if ( iaCount < ( backEnd.viewParms.numInteractions - 1 ) )
				{
					// jump to next interaction and start shadowing
					ia++;
					iaCount++;
					iaFirst = iaCount;
					drawShadows = qtrue;
				}
				else
				{
					// increase last time to leave for loop
					iaCount++;
				}
			}

			// force updates
			oldLight = NULL;
			oldEntity = NULL;
			oldShader = NULL;
		}
		else
		{
			// just continue
			ia = ia->next;
			iaCount++;
		}
	}

	// go back to the world modelview matrix
	GL_LoadModelViewMatrix( backEnd.viewParms.world.modelViewMatrix );

	if ( depthRange )
	{
		glDepthRange( 0, 1 );
	}

	// reset scissor clamping
	GL_Scissor( backEnd.viewParms.viewportX, backEnd.viewParms.viewportY,
	            backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight );

	// reset clear color
	GL_ClearColor( 0.0f, 0.0f, 0.0f, 1.0f );

	GL_CheckErrors();

	if ( r_speeds->integer == RSPEEDS_SHADING_TIMES )
	{
		glFinish();
		endTime = ri.Milliseconds();
		backEnd.pc.c_forwardLightingTime = endTime - startTime;
	}
}

#if 0
static void RB_RenderDrawSurfacesIntoGeometricBuffer()
{
	trRefEntity_t *entity, *oldEntity;
	shader_t      *shader, *oldShader;
	int           lightmapNum, oldLightmapNum;
	qboolean      depthRange, oldDepthRange;
	int           i;
	drawSurf_t    *drawSurf;
	int           startTime = 0, endTime = 0;

	GLimp_LogComment( "--- RB_RenderDrawSurfacesIntoGeometricBuffer ---\n" );

	if ( r_speeds->integer == RSPEEDS_SHADING_TIMES )
	{
		glFinish();
		startTime = ri.Milliseconds();
	}

	// draw everything
	oldEntity = NULL;
	oldShader = NULL;
	oldLightmapNum = -1;
	oldDepthRange = qfalse;
	depthRange = qfalse;
	backEnd.currentLight = NULL;

	GL_CheckErrors();

	for ( i = 0, drawSurf = backEnd.viewParms.drawSurfs; i < backEnd.viewParms.numDrawSurfs; i++, drawSurf++ )
	{
		// update locals
		entity = drawSurf->entity;
		shader = tr.sortedShaders[ drawSurf->shaderNum ];
		lightmapNum = drawSurf->lightmapNum;

		// skip all translucent surfaces that don't matter for this pass
		if ( shader->sort > SS_OPAQUE )
		{
			break;
		}

		/*
		if(DS_PREPASS_LIGHTING_ENABLED())
		{
		        if(entity == oldEntity && shader == oldShader)
		        {
		                // fast path, same as previous sort
		                rb_surfaceTable[*drawSurf->surface] (drawSurf->surface);
		                continue;
		        }

		        // change the tess parameters if needed
		        // an "entityMergable" shader is a shader that can have surfaces from separate
		        // entities merged into a single batch, like smoke and blood puff sprites
		        if(shader != oldShader || (entity != oldEntity && !shader->entityMergable))
		        {
		                if(oldShader != NULL)
		                {
		                        Tess_End();
		                }

		                Tess_Begin(Tess_StageIteratorGBufferNormalsOnly, NULL, shader, NULL, qfalse, qfalse, -1, 0);
		                oldShader = shader;
		                oldLightmapNum = lightmapNum;
		        }
		}
		else
		*/
		{
			if ( entity == oldEntity && shader == oldShader && lightmapNum == oldLightmapNum )
			{
				// fast path, same as previous sort
				rb_surfaceTable[ *drawSurf->surface ]( drawSurf->surface );
				continue;
			}

			// change the tess parameters if needed
			// an "entityMergable" shader is a shader that can have surfaces from separate
			// entities merged into a single batch, like smoke and blood puff sprites
			if ( shader != oldShader || ( entity != oldEntity && !shader->entityMergable ) )
			{
				if ( oldShader != NULL )
				{
					Tess_End();
				}

				Tess_Begin( Tess_StageIteratorGBuffer, NULL, shader, NULL, qfalse, qfalse, lightmapNum, 0 );
				oldShader = shader;
				oldLightmapNum = lightmapNum;
			}
		}

		// change the modelview matrix if needed
		if ( entity != oldEntity )
		{
			depthRange = qfalse;

			if ( entity != &tr.worldEntity )
			{
				backEnd.currentEntity = entity;

				// set up the transformation matrix
				R_RotateEntityForViewParms( backEnd.currentEntity, &backEnd.viewParms, &backEnd.orientation );

				if ( backEnd.currentEntity->e.renderfx & RF_DEPTHHACK )
				{
					// hack the depth range to prevent view model from poking into walls
					depthRange = qtrue;
				}
			}
			else
			{
				backEnd.currentEntity = &tr.worldEntity;
				backEnd.orientation = backEnd.viewParms.world;
			}

			GL_LoadModelViewMatrix( backEnd.orientation.modelViewMatrix );

			// change depthrange if needed
			if ( oldDepthRange != depthRange )
			{
				if ( depthRange )
				{
					glDepthRange( 0, 0.3 );
				}
				else
				{
					glDepthRange( 0, 1 );
				}

				oldDepthRange = depthRange;
			}

			oldEntity = entity;
		}

		// add the triangles for this surface
		rb_surfaceTable[ *drawSurf->surface ]( drawSurf->surface );
	}

	// draw the contents of the last shader batch
	if ( oldShader != NULL )
	{
		Tess_End();
	}

	// go back to the world modelview matrix
	GL_LoadModelViewMatrix( backEnd.viewParms.world.modelViewMatrix );

	if ( depthRange )
	{
		glDepthRange( 0, 1 );
	}

	// disable offscreen rendering
	R_BindNullFBO();

	GL_CheckErrors();

	if ( r_speeds->integer == RSPEEDS_SHADING_TIMES )
	{
		glFinish();
		endTime = ri.Milliseconds();
		backEnd.pc.c_deferredGBufferTime = endTime - startTime;
	}
}

#endif

void RB_RenderInteractionsDeferred()
{
	interaction_t *ia;
	int           iaCount;
	trRefLight_t  *light; //, *oldLight = NULL;
	shader_t      *lightShader;
	shaderStage_t *attenuationXYStage;
	shaderStage_t *attenuationZStage;
//	int             i;
	int           j;
	//vec3_t          viewOrigin;
	//vec3_t          lightOrigin;
//	vec4_t          lightColor;
	matrix_t ortho;
	vec4_t   quadVerts[ 4 ];
	vec4_t   lightFrustum[ 6 ];
	int      startTime = 0, endTime = 0;

	GLimp_LogComment( "--- RB_RenderInteractionsDeferred ---\n" );

	if ( r_skipLightBuffer->integer )
	{
		return;
	}

	if ( r_speeds->integer == RSPEEDS_SHADING_TIMES )
	{
		glFinish();
		startTime = ri.Milliseconds();
	}

	GL_State( GLS_DEFAULT );

	// set the window clipping
	GL_Viewport( backEnd.viewParms.viewportX, backEnd.viewParms.viewportY,
	             backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight );

	GL_Scissor( backEnd.viewParms.viewportX, backEnd.viewParms.viewportY,
	            backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight );

	tess.multiDrawPrimitives = 0;
	tess.numIndexes = 0;
	tess.numVertexes = 0;

	R_BindNullVBO();
	R_BindNullIBO();

	R_BindFBO( tr.geometricRenderFBO );
	glDrawBuffers( 1, geometricRenderTargets );

	// helper matrix for 2D rendering
	MatrixOrthogonalProjection( ortho, backEnd.viewParms.viewportX,
	                            backEnd.viewParms.viewportX + backEnd.viewParms.viewportWidth,
	                            backEnd.viewParms.viewportY, backEnd.viewParms.viewportY + backEnd.viewParms.viewportHeight,
	                            -99999, 99999 );

	// loop trough all light interactions and render the light quad for each last interaction
	for ( iaCount = 0, ia = &backEnd.viewParms.interactions[ 0 ]; iaCount < backEnd.viewParms.numInteractions; )
	{
		backEnd.currentLight = light = ia->light;

		if ( glConfig2.occlusionQueryBits && glConfig.driverType != GLDRV_MESA && r_dynamicLightOcclusionCulling->integer && !ia->occlusionQuerySamples )
		{
			// skip all interactions of this light because it failed the occlusion query
			goto skipInteraction;
		}

skipInteraction:

		if ( !ia->next )
		{
			if ( ( glConfig2.occlusionQueryBits && glConfig.driverType != GLDRV_MESA && r_dynamicLightOcclusionCulling->integer && ia->occlusionQuerySamples ) ||
			     !r_dynamicLightOcclusionCulling->integer )
			{
				GL_CheckErrors();

				GLimp_LogComment( "--- Rendering lighting ---\n" );

				// build world to light space matrix
				switch ( light->l.rlType )
				{
					case RL_OMNI:
					case RL_DIRECTIONAL:
						{
							// build the attenuation matrix
							MatrixSetupTranslation( light->attenuationMatrix, 0.5, 0.5, 0.5 );  // bias
							MatrixMultiplyScale( light->attenuationMatrix, 0.5, 0.5, 0.5 );  // scale
							MatrixMultiply2( light->attenuationMatrix, light->projectionMatrix );  // light projection (frustum)
							MatrixMultiply2( light->attenuationMatrix, light->viewMatrix );
							break;
						}

					case RL_PROJ:
						{
							// build the attenuation matrix
							MatrixSetupTranslation( light->attenuationMatrix, 0.5, 0.5, 0.0 );  // bias
							MatrixMultiplyScale( light->attenuationMatrix, 0.5, 0.5, Q_min( light->falloffLength, 1.0 ) );   // scale
							MatrixMultiply2( light->attenuationMatrix, light->projectionMatrix );
							MatrixMultiply2( light->attenuationMatrix, light->viewMatrix );
							break;
						}

					default:
						break;
				}

				if ( light->clipsNearPlane )
				{
					// set 2D virtual screen size
					GL_PushMatrix();
					GL_LoadProjectionMatrix( ortho );
					GL_LoadModelViewMatrix( matrixIdentity );

					for ( j = 0; j < 6; j++ )
					{
						VectorCopy( light->frustum[ j ].normal, lightFrustum[ j ] );
						lightFrustum[ j ][ 3 ] = light->frustum[ j ].dist;
					}
				}
				else
				{
					// prepare rendering of the light volume
					// either bind VBO or setup the tess struct
#if 0 // FIXME check why this does not work here
					if ( light->isStatic && light->frustumVBO && light->frustumIBO )
					{
						// render in world space
						backEnd.orientation = backEnd.viewParms.world;

						GL_LoadProjectionMatrix( backEnd.viewParms.projectionMatrix );
						GL_LoadModelViewMatrix( backEnd.viewParms.world.modelViewMatrix );

						R_BindVBO( light->frustumVBO );
						R_BindIBO( light->frustumIBO );

						GL_VertexAttribsState( ATTR_POSITION );

						tess.numVertexes = light->frustumVerts;
						tess.numIndexes = light->frustumIndexes;
					}
					else
#endif
					{
						tess.multiDrawPrimitives = 0;
						tess.numIndexes = 0;
						tess.numVertexes = 0;

						switch ( light->l.rlType )
						{
							case RL_OMNI:
							case RL_DIRECTIONAL:
								{
#if 1
									// render in light space
									R_RotateLightForViewParms( light, &backEnd.viewParms, &backEnd.orientation );

									GL_LoadProjectionMatrix( backEnd.viewParms.projectionMatrix );
									GL_LoadModelViewMatrix( backEnd.orientation.modelViewMatrix );

									Tess_AddCube( vec3_origin, light->localBounds[ 0 ], light->localBounds[ 1 ], colorWhite );

									Tess_UpdateVBOs( ATTR_POSITION | ATTR_COLOR );
#else
									matrix_t transform, scale, rot;
									axis_t   axis;

									MatrixSetupScale( scale, light->l.radius[ 0 ], light->l.radius[ 1 ], light->l.radius[ 2 ] );
									MatrixMultiply( scale, light->transformMatrix, transform );

									GL_LoadModelViewMatrix( transform );
									GL_LoadProjectionMatrix( backEnd.viewParms.projectionMatrix );

									R_BindVBO( tr.unitCubeVBO );
									R_BindIBO( tr.unitCubeIBO );

									GL_VertexAttribsState( ATTR_POSITION );

									tess.multiDrawPrimitives = 0;
									tess.numVertexes = tr.unitCubeVBO->vertexesNum;
									tess.numIndexes = tr.unitCubeIBO->indexesNum;
#endif
									break;
								}

							case RL_PROJ:
								{
									vec3_t farCorners[ 4 ];
									vec4_t *frustum = light->localFrustum;

									// render in light space
									R_RotateLightForViewParms( light, &backEnd.viewParms, &backEnd.orientation );

									GL_LoadProjectionMatrix( backEnd.viewParms.projectionMatrix );
									GL_LoadModelViewMatrix( backEnd.orientation.modelViewMatrix );

									PlanesGetIntersectionPoint( frustum[ FRUSTUM_LEFT ], frustum[ FRUSTUM_TOP ], frustum[ FRUSTUM_FAR ], farCorners[ 0 ] );
									PlanesGetIntersectionPoint( frustum[ FRUSTUM_RIGHT ], frustum[ FRUSTUM_TOP ], frustum[ FRUSTUM_FAR ], farCorners[ 1 ] );
									PlanesGetIntersectionPoint( frustum[ FRUSTUM_RIGHT ], frustum[ FRUSTUM_BOTTOM ], frustum[ FRUSTUM_FAR ], farCorners[ 2 ] );
									PlanesGetIntersectionPoint( frustum[ FRUSTUM_LEFT ], frustum[ FRUSTUM_BOTTOM ], frustum[ FRUSTUM_FAR ], farCorners[ 3 ] );

									if ( !VectorCompare( light->l.projStart, vec3_origin ) )
									{
										vec3_t nearCorners[ 4 ];

										// calculate the vertices defining the top area
										PlanesGetIntersectionPoint( frustum[ FRUSTUM_LEFT ], frustum[ FRUSTUM_TOP ], frustum[ FRUSTUM_NEAR ], nearCorners[ 0 ] );
										PlanesGetIntersectionPoint( frustum[ FRUSTUM_RIGHT ], frustum[ FRUSTUM_TOP ], frustum[ FRUSTUM_NEAR ], nearCorners[ 1 ] );
										PlanesGetIntersectionPoint( frustum[ FRUSTUM_RIGHT ], frustum[ FRUSTUM_BOTTOM ], frustum[ FRUSTUM_NEAR ], nearCorners[ 2 ] );
										PlanesGetIntersectionPoint( frustum[ FRUSTUM_LEFT ], frustum[ FRUSTUM_BOTTOM ], frustum[ FRUSTUM_NEAR ], nearCorners[ 3 ] );

										// draw outer surfaces
										for ( j = 0; j < 4; j++ )
										{
											Vector4Set( quadVerts[ 0 ], nearCorners[ j ][ 0 ], nearCorners[ j ][ 1 ], nearCorners[ j ][ 2 ], 1 );
											Vector4Set( quadVerts[ 1 ], farCorners[ j ][ 0 ], farCorners[ j ][ 1 ], farCorners[ j ][ 2 ], 1 );
											Vector4Set( quadVerts[ 2 ], farCorners[( j + 1 ) % 4 ][ 0 ], farCorners[( j + 1 ) % 4 ][ 1 ], farCorners[( j + 1 ) % 4 ][ 2 ], 1 );
											Vector4Set( quadVerts[ 3 ], nearCorners[( j + 1 ) % 4 ][ 0 ], nearCorners[( j + 1 ) % 4 ][ 1 ], nearCorners[( j + 1 ) % 4 ][ 2 ], 1 );
											Tess_AddQuadStamp2( quadVerts, colorCyan );
										}

										// draw far cap
										Vector4Set( quadVerts[ 0 ], farCorners[ 3 ][ 0 ], farCorners[ 3 ][ 1 ], farCorners[ 3 ][ 2 ], 1 );
										Vector4Set( quadVerts[ 1 ], farCorners[ 2 ][ 0 ], farCorners[ 2 ][ 1 ], farCorners[ 2 ][ 2 ], 1 );
										Vector4Set( quadVerts[ 2 ], farCorners[ 1 ][ 0 ], farCorners[ 1 ][ 1 ], farCorners[ 1 ][ 2 ], 1 );
										Vector4Set( quadVerts[ 3 ], farCorners[ 0 ][ 0 ], farCorners[ 0 ][ 1 ], farCorners[ 0 ][ 2 ], 1 );
										Tess_AddQuadStamp2( quadVerts, colorRed );

										// draw near cap
										Vector4Set( quadVerts[ 0 ], nearCorners[ 0 ][ 0 ], nearCorners[ 0 ][ 1 ], nearCorners[ 0 ][ 2 ], 1 );
										Vector4Set( quadVerts[ 1 ], nearCorners[ 1 ][ 0 ], nearCorners[ 1 ][ 1 ], nearCorners[ 1 ][ 2 ], 1 );
										Vector4Set( quadVerts[ 2 ], nearCorners[ 2 ][ 0 ], nearCorners[ 2 ][ 1 ], nearCorners[ 2 ][ 2 ], 1 );
										Vector4Set( quadVerts[ 3 ], nearCorners[ 3 ][ 0 ], nearCorners[ 3 ][ 1 ], nearCorners[ 3 ][ 2 ], 1 );
										Tess_AddQuadStamp2( quadVerts, colorGreen );
									}
									else
									{
										vec3_t top;

										// no light_start, just use the top vertex (doesn't need to be mirrored)
										PlanesGetIntersectionPoint( frustum[ FRUSTUM_LEFT ], frustum[ FRUSTUM_RIGHT ], frustum[ FRUSTUM_TOP ], top );

										// draw pyramid
										for ( j = 0; j < 4; j++ )
										{
											VectorCopy( top, tess.xyz[ tess.numVertexes ] );
											Vector4Copy( colorCyan, tess.colors[ tess.numVertexes ] );
											tess.indexes[ tess.numIndexes++ ] = tess.numVertexes;
											tess.numVertexes++;

											VectorCopy( farCorners[( j + 1 ) % 4 ], tess.xyz[ tess.numVertexes ] );
											Vector4Copy( colorCyan, tess.colors[ tess.numVertexes ] );
											tess.indexes[ tess.numIndexes++ ] = tess.numVertexes;
											tess.numVertexes++;

											VectorCopy( farCorners[ j ], tess.xyz[ tess.numVertexes ] );
											Vector4Copy( colorCyan, tess.colors[ tess.numVertexes ] );
											tess.indexes[ tess.numIndexes++ ] = tess.numVertexes;
											tess.numVertexes++;
										}

										Vector4Set( quadVerts[ 0 ], farCorners[ 0 ][ 0 ], farCorners[ 0 ][ 1 ], farCorners[ 0 ][ 2 ], 1 );
										Vector4Set( quadVerts[ 1 ], farCorners[ 1 ][ 0 ], farCorners[ 1 ][ 1 ], farCorners[ 1 ][ 2 ], 1 );
										Vector4Set( quadVerts[ 2 ], farCorners[ 2 ][ 0 ], farCorners[ 2 ][ 1 ], farCorners[ 2 ][ 2 ], 1 );
										Vector4Set( quadVerts[ 3 ], farCorners[ 3 ][ 0 ], farCorners[ 3 ][ 1 ], farCorners[ 3 ][ 2 ], 1 );
										Tess_AddQuadStamp2( quadVerts, colorRed );
									}

									Tess_UpdateVBOs( ATTR_POSITION | ATTR_COLOR );
									break;
								}

							default:
								break;
						}
					}
				}

				// last interaction of current light
				lightShader = light->shader;
				attenuationZStage = lightShader->stages[ 0 ];

				for ( j = 1; j < MAX_SHADER_STAGES; j++ )
				{
					attenuationXYStage = lightShader->stages[ j ];

					if ( !attenuationXYStage )
					{
						break;
					}

					if ( attenuationXYStage->type != ST_ATTENUATIONMAP_XY )
					{
						continue;
					}

					if ( !RB_EvalExpression( &attenuationXYStage->ifExp, 1.0 ) )
					{
						continue;
					}

					Tess_ComputeColor( attenuationXYStage );

					if ( VectorLength( tess.svars.color ) < 0.01 )
					{
						// don't render black lights
					}

					R_ComputeFinalAttenuation( attenuationXYStage, light );

					// set OpenGL state for additive lighting
					GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE );

					if ( light->clipsNearPlane )
					{
						GL_Cull( CT_TWO_SIDED );
					}
					else
					{
						GL_Cull( CT_FRONT_SIDED );
					}

					if ( light->l.rlType == RL_OMNI )
					{
						// choose right shader program ----------------------------------
						gl_deferredLightingShader_omniXYZ->SetPortalClipping( backEnd.viewParms.isPortal );
						gl_deferredLightingShader_omniXYZ->SetNormalMapping( r_normalMapping->integer );
						gl_deferredLightingShader_omniXYZ->SetShadowing( false );
						gl_deferredLightingShader_omniXYZ->SetFrustumClipping( light->clipsNearPlane );

						gl_deferredLightingShader_omniXYZ->BindProgram();

						// end choose right shader program ------------------------------

						gl_deferredLightingShader_omniXYZ->SetUniform_ViewOrigin( backEnd.viewParms.orientation.origin );  // in world space
						gl_deferredLightingShader_omniXYZ->SetUniform_LightOrigin( light->origin );
						gl_deferredLightingShader_omniXYZ->SetUniform_LightColor( tess.svars.color );
						gl_deferredLightingShader_omniXYZ->SetUniform_LightRadius( light->sphereRadius );
						gl_deferredLightingShader_omniXYZ->SetUniform_LightScale( light->l.scale );
						gl_deferredLightingShader_omniXYZ->SetUniform_LightAttenuationMatrix( light->attenuationMatrix2 );
						gl_deferredLightingShader_omniXYZ->SetUniform_LightFrustum( lightFrustum );

						gl_deferredLightingShader_omniXYZ->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );
						gl_deferredLightingShader_omniXYZ->SetUniform_UnprojectMatrix( backEnd.viewParms.unprojectionMatrix );

						if ( backEnd.viewParms.isPortal )
						{
							float plane[ 4 ];

							// clipping plane in world space
							plane[ 0 ] = backEnd.viewParms.portalPlane.normal[ 0 ];
							plane[ 1 ] = backEnd.viewParms.portalPlane.normal[ 1 ];
							plane[ 2 ] = backEnd.viewParms.portalPlane.normal[ 2 ];
							plane[ 3 ] = backEnd.viewParms.portalPlane.dist;

							gl_deferredLightingShader_omniXYZ->SetUniform_PortalPlane( plane );
						}

						// bind u_DiffuseMap
						GL_SelectTexture( 0 );
						GL_Bind( tr.deferredDiffuseFBOImage );

						// bind u_NormalMap
						GL_SelectTexture( 1 );
						GL_Bind( tr.deferredNormalFBOImage );

						if ( r_normalMapping->integer )
						{
							// bind u_SpecularMap
							GL_SelectTexture( 2 );
							GL_Bind( tr.deferredSpecularFBOImage );
						}

						// bind u_DepthMap
						GL_SelectTexture( 3 );
						GL_Bind( tr.depthRenderImage );

						// bind u_AttenuationMapXY
						GL_SelectTexture( 4 );
						BindAnimatedImage( &attenuationXYStage->bundle[ TB_COLORMAP ] );

						// bind u_AttenuationMapZ
						GL_SelectTexture( 5 );
						BindAnimatedImage( &attenuationZStage->bundle[ TB_COLORMAP ] );

						if ( light->clipsNearPlane )
						{
							// draw lighting with a fullscreen quad
							Tess_InstantQuad( backEnd.viewParms.viewportVerts );

							/*
							Vector4Set(quadVerts[0], ia->scissorX, ia->scissorY, 0, 1);
							Vector4Set(quadVerts[1], ia->scissorX + ia->scissorWidth - 1, ia->scissorY, 0, 1);
							Vector4Set(quadVerts[2], ia->scissorX + ia->scissorWidth - 1, ia->scissorY + ia->scissorHeight - 1, 0, 1);
							Vector4Set(quadVerts[3], ia->scissorX, ia->scissorY + ia->scissorHeight - 1, 0, 1);
							Tess_InstantQuad(quadVerts);
							*/
						}
						else
						{
							// draw the volume
							Tess_DrawElements();
						}
					}
					else if ( light->l.rlType == RL_PROJ )
					{
						// choose right shader program ----------------------------------
						gl_deferredLightingShader_projXYZ->SetPortalClipping( backEnd.viewParms.isPortal );
						gl_deferredLightingShader_projXYZ->SetNormalMapping( r_normalMapping->integer );
						gl_deferredLightingShader_projXYZ->SetShadowing( false );
						gl_deferredLightingShader_projXYZ->SetFrustumClipping( light->clipsNearPlane );

						gl_deferredLightingShader_projXYZ->BindProgram();

						// end choose right shader program ------------------------------

						gl_deferredLightingShader_projXYZ->SetUniform_ViewOrigin( backEnd.viewParms.orientation.origin );  // in world space
						gl_deferredLightingShader_projXYZ->SetUniform_LightOrigin( light->origin );
						gl_deferredLightingShader_projXYZ->SetUniform_LightColor( tess.svars.color );
						gl_deferredLightingShader_projXYZ->SetUniform_LightRadius( light->sphereRadius );
						gl_deferredLightingShader_projXYZ->SetUniform_LightScale( light->l.scale );
						gl_deferredLightingShader_projXYZ->SetUniform_LightAttenuationMatrix( light->attenuationMatrix2 );
						gl_deferredLightingShader_projXYZ->SetUniform_LightFrustum( lightFrustum );

						gl_deferredLightingShader_projXYZ->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );
						gl_deferredLightingShader_projXYZ->SetUniform_UnprojectMatrix( backEnd.viewParms.unprojectionMatrix );

						if ( backEnd.viewParms.isPortal )
						{
							float plane[ 4 ];

							// clipping plane in world space
							plane[ 0 ] = backEnd.viewParms.portalPlane.normal[ 0 ];
							plane[ 1 ] = backEnd.viewParms.portalPlane.normal[ 1 ];
							plane[ 2 ] = backEnd.viewParms.portalPlane.normal[ 2 ];
							plane[ 3 ] = backEnd.viewParms.portalPlane.dist;

							gl_deferredLightingShader_projXYZ->SetUniform_PortalPlane( plane );
						}

						// bind u_DiffuseMap
						GL_SelectTexture( 0 );
						GL_Bind( tr.deferredDiffuseFBOImage );

						// bind u_NormalMap
						GL_SelectTexture( 1 );
						GL_Bind( tr.deferredNormalFBOImage );

						if ( r_normalMapping->integer )
						{
							// bind u_SpecularMap
							GL_SelectTexture( 2 );
							GL_Bind( tr.deferredSpecularFBOImage );
						}

						// bind u_DepthMap
						GL_SelectTexture( 3 );
						GL_Bind( tr.depthRenderImage );

						// bind u_AttenuationMapXY
						GL_SelectTexture( 4 );
						BindAnimatedImage( &attenuationXYStage->bundle[ TB_COLORMAP ] );

						// bind u_AttenuationMapZ
						GL_SelectTexture( 5 );
						BindAnimatedImage( &attenuationZStage->bundle[ TB_COLORMAP ] );

						if ( light->clipsNearPlane )
						{
							// draw lighting with a fullscreen quad
							Tess_InstantQuad( backEnd.viewParms.viewportVerts );

							/*
							Vector4Set(quadVerts[0], ia->scissorX, ia->scissorY, 0, 1);
							Vector4Set(quadVerts[1], ia->scissorX + ia->scissorWidth - 1, ia->scissorY, 0, 1);
							Vector4Set(quadVerts[2], ia->scissorX + ia->scissorWidth - 1, ia->scissorY + ia->scissorHeight - 1, 0, 1);
							Vector4Set(quadVerts[3], ia->scissorX, ia->scissorY + ia->scissorHeight - 1, 0, 1);
							Tess_InstantQuad(quadVerts);
							*/
						}
						else
						{
							// draw the volume
							Tess_DrawElements();
						}
					}
					else if ( light->l.rlType == RL_DIRECTIONAL )
					{
						// choose right shader program ----------------------------------
						gl_deferredLightingShader_directionalSun->SetPortalClipping( backEnd.viewParms.isPortal );
						gl_deferredLightingShader_directionalSun->SetNormalMapping( r_normalMapping->integer );
						gl_deferredLightingShader_directionalSun->SetShadowing( false );
						gl_deferredLightingShader_directionalSun->SetFrustumClipping( light->clipsNearPlane );

						gl_deferredLightingShader_directionalSun->BindProgram();

						// end choose right shader program ------------------------------

						gl_deferredLightingShader_directionalSun->SetUniform_ViewOrigin( backEnd.viewParms.orientation.origin );  // in world space
						gl_deferredLightingShader_directionalSun->SetUniform_LightDir( tr.sunDirection );
						gl_deferredLightingShader_directionalSun->SetUniform_LightColor( tess.svars.color );
						gl_deferredLightingShader_directionalSun->SetUniform_LightRadius( light->sphereRadius );
						gl_deferredLightingShader_directionalSun->SetUniform_LightScale( light->l.scale );
						gl_deferredLightingShader_directionalSun->SetUniform_LightAttenuationMatrix( light->attenuationMatrix2 );
						gl_deferredLightingShader_directionalSun->SetUniform_LightFrustum( lightFrustum );

						gl_deferredLightingShader_directionalSun->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );
						gl_deferredLightingShader_directionalSun->SetUniform_UnprojectMatrix( backEnd.viewParms.unprojectionMatrix );

						if ( backEnd.viewParms.isPortal )
						{
							float plane[ 4 ];

							// clipping plane in world space
							plane[ 0 ] = backEnd.viewParms.portalPlane.normal[ 0 ];
							plane[ 1 ] = backEnd.viewParms.portalPlane.normal[ 1 ];
							plane[ 2 ] = backEnd.viewParms.portalPlane.normal[ 2 ];
							plane[ 3 ] = backEnd.viewParms.portalPlane.dist;

							gl_deferredLightingShader_directionalSun->SetUniform_PortalPlane( plane );
						}

						// bind u_DiffuseMap
						GL_SelectTexture( 0 );
						GL_Bind( tr.deferredDiffuseFBOImage );

						// bind u_NormalMap
						GL_SelectTexture( 1 );
						GL_Bind( tr.deferredNormalFBOImage );

						if ( r_normalMapping->integer )
						{
							// bind u_SpecularMap
							GL_SelectTexture( 2 );
							GL_Bind( tr.deferredSpecularFBOImage );
						}

						// bind u_DepthMap
						GL_SelectTexture( 3 );
						GL_Bind( tr.depthRenderImage );

						// bind u_AttenuationMapXY
						//GL_SelectTexture(4);
						//BindAnimatedImage(&attenuationXYStage->bundle[TB_COLORMAP]);

						// bind u_AttenuationMapZ
						//GL_SelectTexture(5);
						//BindAnimatedImage(&attenuationZStage->bundle[TB_COLORMAP]);

						if ( light->clipsNearPlane )
						{
							// draw lighting with a fullscreen quad
							Tess_InstantQuad( backEnd.viewParms.viewportVerts );

							/*
							Vector4Set(quadVerts[0], ia->scissorX, ia->scissorY, 0, 1);
							Vector4Set(quadVerts[1], ia->scissorX + ia->scissorWidth - 1, ia->scissorY, 0, 1);
							Vector4Set(quadVerts[2], ia->scissorX + ia->scissorWidth - 1, ia->scissorY + ia->scissorHeight - 1, 0, 1);
							Vector4Set(quadVerts[3], ia->scissorX, ia->scissorY + ia->scissorHeight - 1, 0, 1);
							Tess_InstantQuad(quadVerts);
							*/
						}
						else
						{
							// draw the volume
							Tess_DrawElements();
						}
					}
				}

				if ( light->clipsNearPlane )
				{
					GL_PopMatrix();
				}
			}

			if ( iaCount < ( backEnd.viewParms.numInteractions - 1 ) )
			{
				// jump to next interaction and continue
				ia++;
				iaCount++;
			}
			else
			{
				// increase last time to leave for loop
				iaCount++;
			}
		}
		else
		{
			// just continue
			ia = ia->next;
			iaCount++;
		}

//		oldLight = light;
	}

	// clear shader so we can tell we don't have any unclosed surfaces
	tess.multiDrawPrimitives = 0;
	tess.numIndexes = 0;
	tess.numVertexes = 0;

	R_BindNullVBO();
	R_BindNullIBO();

	// go back to the world modelview matrix
	backEnd.orientation = backEnd.viewParms.world;

	GL_LoadProjectionMatrix( backEnd.viewParms.projectionMatrix );
	GL_LoadModelViewMatrix( backEnd.viewParms.world.modelViewMatrix );

	// reset scissor
	GL_Scissor( backEnd.viewParms.viewportX, backEnd.viewParms.viewportY,
	            backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight );

	GL_CheckErrors();

	if ( r_speeds->integer == RSPEEDS_SHADING_TIMES )
	{
		glFinish();
		endTime = ri.Milliseconds();
		backEnd.pc.c_deferredLightingTime = endTime - startTime;
	}
}

static void RB_RenderInteractionsDeferredShadowMapped()
{
	interaction_t  *ia;
	int            iaCount;
	int            iaFirst;
	shader_t       *shader, *oldShader;
	trRefEntity_t  *entity, *oldEntity;
	trRefLight_t   *light, *oldLight;
	surfaceType_t  *surface;
	qboolean       depthRange, oldDepthRange;
	qboolean       alphaTest, oldAlphaTest;
	deformType_t   deformType, oldDeformType;
	qboolean       drawShadows;
	int            cubeSide;

	int            splitFrustumIndex;
	static const matrix_t bias = { 0.5, 0.0, 0.0, 0.0,
	                               0.0, 0.5, 0.0, 0.0,
	                               0.0, 0.0, 0.5, 0.0,
	                               0.5, 0.5, 0.5, 1.0
	                      };

	shader_t       *lightShader;
	shaderStage_t  *attenuationXYStage;
	shaderStage_t  *attenuationZStage;
	int            i, j;
	//vec3_t          viewOrigin;
	//vec3_t          lightOrigin;
	vec3_t         lightDirection;
//	vec4_t          lightColor;
	matrix_t       ortho;
	vec4_t         quadVerts[ 4 ];
	vec4_t         lightFrustum[ 6 ];
	int            startTime = 0, endTime = 0;

	GLimp_LogComment( "--- RB_RenderInteractionsDeferredShadowMapped ---\n" );

	if ( r_skipLightBuffer->integer )
	{
		return;
	}

	if ( r_speeds->integer == RSPEEDS_SHADING_TIMES )
	{
		glFinish();
		startTime = ri.Milliseconds();
	}

	oldLight = NULL;
	oldEntity = NULL;
	oldShader = NULL;
	oldDepthRange = depthRange = qfalse;
	oldAlphaTest = alphaTest = qfalse;
	oldDeformType = deformType = DEFORM_TYPE_NONE;
	drawShadows = qtrue;
	cubeSide = 0;
	splitFrustumIndex = 0;

	GL_State( GLS_DEFAULT );

	// set the window clipping
	GL_Viewport( backEnd.viewParms.viewportX, backEnd.viewParms.viewportY,
	             backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight );

	GL_Scissor( backEnd.viewParms.viewportX, backEnd.viewParms.viewportY,
	            backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight );

	// helper matrix for 2D rendering
	MatrixOrthogonalProjection( ortho, backEnd.viewParms.viewportX,
	                            backEnd.viewParms.viewportX + backEnd.viewParms.viewportWidth,
	                            backEnd.viewParms.viewportY, backEnd.viewParms.viewportY + backEnd.viewParms.viewportHeight,
	                            -99999, 99999 );

	// if we need to clear the FBO color buffers then it should be white
	GL_ClearColor( 1.0f, 1.0f, 1.0f, 1.0f );

	// render interactions
	for ( iaCount = 0, iaFirst = 0, ia = &backEnd.viewParms.interactions[ 0 ]; iaCount < backEnd.viewParms.numInteractions; )
	{
		backEnd.currentLight = light = ia->light;
		backEnd.currentEntity = entity = ia->entity;
		surface = ia->surface;
		shader = ia->surfaceShader;
		alphaTest = shader->alphaTest;

		if ( shader->numDeforms )
		{
			deformType = ShaderRequiresCPUDeforms( shader ) ? DEFORM_TYPE_CPU : DEFORM_TYPE_GPU;
		}
		else
		{
			deformType = DEFORM_TYPE_NONE;
		}

		if ( glConfig2.occlusionQueryBits && glConfig.driverType != GLDRV_MESA && r_dynamicLightOcclusionCulling->integer && !ia->occlusionQuerySamples )
		{
			// skip all interactions of this light because it failed the occlusion query
			goto skipInteraction;
		}

		// only iaCount == iaFirst if first iteration or counters were reset
		if ( iaCount == iaFirst )
		{
			if ( drawShadows )
			{
				// HACK: bring OpenGL into a safe state or strange FBO update problems will occur
				GL_BindProgram( NULL );
				GL_State( GLS_DEFAULT );
				//GL_VertexAttribsState(ATTR_POSITION);

				GL_SelectTexture( 0 );
				GL_Bind( tr.whiteImage );

				if ( light->l.noShadows || light->shadowLOD < 0 )
				{
					if ( r_logFile->integer )
					{
						// don't just call LogComment, or we will get
						// a call to va() every frame!
						GLimp_LogComment( va( "----- Skipping shadowCube side: %i -----\n", cubeSide ) );
					}

					goto skipInteraction;
				}
				else
				{
					switch ( light->l.rlType )
					{
						case RL_OMNI:
							{
								float    zNear, zFar;
								float    fovX, fovY;
								qboolean flipX, flipY;
								//float        *proj;
								vec3_t   angles;
								matrix_t rotationMatrix, transformMatrix, viewMatrix;

								if ( r_logFile->integer )
								{
									// don't just call LogComment, or we will get
									// a call to va() every frame!
									GLimp_LogComment( va( "----- Rendering shadowCube side: %i -----\n", cubeSide ) );
								}

								R_BindFBO( tr.shadowMapFBO[ light->shadowLOD ] );
								R_AttachFBOTexture2D( GL_TEXTURE_CUBE_MAP_POSITIVE_X + cubeSide,
								                      tr.shadowCubeFBOImage[ light->shadowLOD ]->texnum, 0 );

								if ( !r_ignoreGLErrors->integer )
								{
									R_CheckFBO( tr.shadowMapFBO[ light->shadowLOD ] );
								}

								// set the window clipping
								GL_Viewport( 0, 0, shadowMapResolutions[ light->shadowLOD ], shadowMapResolutions[ light->shadowLOD ] );
								GL_Scissor( 0, 0, shadowMapResolutions[ light->shadowLOD ], shadowMapResolutions[ light->shadowLOD ] );

								glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

								switch ( cubeSide )
								{
									case 0:
										{
											// view parameters
											VectorSet( angles, 0, 0, 90 );

											// projection parameters
											flipX = qfalse;
											flipY = qfalse;
											break;
										}

									case 1:
										{
											VectorSet( angles, 0, 180, 90 );
											flipX = qtrue;
											flipY = qtrue;
											break;
										}

									case 2:
										{
											VectorSet( angles, 0, 90, 0 );
											flipX = qfalse;
											flipY = qfalse;
											break;
										}

									case 3:
										{
											VectorSet( angles, 0, -90, 0 );
											flipX = qtrue;
											flipY = qtrue;
											break;
										}

									case 4:
										{
											VectorSet( angles, -90, 90, 0 );
											flipX = qfalse;
											flipY = qfalse;
											break;
										}

									case 5:
										{
											VectorSet( angles, 90, 90, 0 );
											flipX = qtrue;
											flipY = qtrue;
											break;
										}

									default:
										{
											// shut up compiler
											VectorSet( angles, 0, 0, 0 );
											flipX = qfalse;
											flipY = qfalse;
											break;
										}
								}

								// Quake -> OpenGL view matrix from light perspective
								MatrixFromAngles( rotationMatrix, angles[ PITCH ], angles[ YAW ], angles[ ROLL ] );
								MatrixSetupTransformFromRotation( transformMatrix, rotationMatrix, light->origin );
								MatrixAffineInverse( transformMatrix, viewMatrix );

								// convert from our coordinate system (looking down X)
								// to OpenGL's coordinate system (looking down -Z)
								MatrixMultiply( quakeToOpenGLMatrix, viewMatrix, light->viewMatrix );

								// OpenGL projection matrix
								fovX = 90;
								fovY = 90;

								zNear = 1.0;
								zFar = light->sphereRadius;

								if ( flipX )
								{
									fovX = -fovX;
								}

								if ( flipY )
								{
									fovY = -fovY;
								}

								MatrixPerspectiveProjectionFovXYRH( light->projectionMatrix, fovX, fovY, zNear, zFar );

								GL_LoadProjectionMatrix( light->projectionMatrix );
								break;
							}

						case RL_PROJ:
							{
								GLimp_LogComment( "--- Rendering projective shadowMap ---\n" );

								R_BindFBO( tr.shadowMapFBO[ light->shadowLOD ] );
								R_AttachFBOTexture2D( GL_TEXTURE_2D, tr.shadowMapFBOImage[ light->shadowLOD ]->texnum, 0 );

								if ( !r_ignoreGLErrors->integer )
								{
									R_CheckFBO( tr.shadowMapFBO[ light->shadowLOD ] );
								}

								// set the window clipping
								GL_Viewport( 0, 0, shadowMapResolutions[ light->shadowLOD ], shadowMapResolutions[ light->shadowLOD ] );
								GL_Scissor( 0, 0, shadowMapResolutions[ light->shadowLOD ], shadowMapResolutions[ light->shadowLOD ] );

								glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

								GL_LoadProjectionMatrix( light->projectionMatrix );
								break;
							}

						case RL_DIRECTIONAL:
							{
								vec3_t   angles;
								vec4_t   forward, side, up;
								vec3_t   viewOrigin, viewDirection;
								matrix_t rotationMatrix, transformMatrix, viewMatrix, projectionMatrix, viewProjectionMatrix;
								matrix_t cropMatrix;
								vec4_t   splitFrustum[ 6 ];
								vec3_t   splitFrustumCorners[ 8 ];
								vec3_t   splitFrustumBounds[ 2 ];
//							vec3_t     splitFrustumViewBounds[2];
								vec3_t   splitFrustumClipBounds[ 2 ];
								//float     splitFrustumRadius;
								int      numCasters;
								vec3_t   casterBounds[ 2 ];
								vec3_t   receiverBounds[ 2 ];
								vec3_t   cropBounds[ 2 ];
								vec4_t   point;
								vec4_t   transf;

								GLimp_LogComment( "--- Rendering directional shadowMap ---\n" );

								R_BindFBO( tr.sunShadowMapFBO[ splitFrustumIndex ] );
								R_AttachFBOTexture2D( GL_TEXTURE_2D, tr.sunShadowMapFBOImage[ splitFrustumIndex ]->texnum, 0 );

								if ( !r_ignoreGLErrors->integer )
								{
									R_CheckFBO( tr.sunShadowMapFBO[ splitFrustumIndex ] );
								}

								// set the window clipping
								GL_Viewport( 0, 0, sunShadowMapResolutions[ splitFrustumIndex ], sunShadowMapResolutions[ splitFrustumIndex ] );
								GL_Scissor( 0, 0, sunShadowMapResolutions[ splitFrustumIndex ], sunShadowMapResolutions[ splitFrustumIndex ] );

								glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

#if 1
								VectorCopy( tr.sunDirection, lightDirection );
#else
								VectorCopy( light->direction, lightDirection );
#endif

#if 0

								if ( r_lightSpacePerspectiveWarping->integer )
								{
									vec3_t         viewOrigin, viewDirection;
									vec4_t         forward, side, up;
									matrix_t       lispMatrix;
									matrix_t       postMatrix;
									matrix_t       projectionCenter;

									static const matrix_t switchToArticle =
									{
										1, 0,  0, 0,
										0, 0,  1, 0,
										0, -1, 0, 0,
										0, 0,  0, 1
									};

									static const matrix_t switchToGL =
									{
										1, 0, 0,  0,
										0, 0, -1, 0,
										0, 1, 0,  0,
										0, 0, 0,  1
									};

									// original light direction is from surface to light
									VectorInverse( lightDirection );
									VectorNormalize( lightDirection );

									VectorCopy( backEnd.viewParms.orientation.origin, viewOrigin );
									VectorCopy( backEnd.viewParms.orientation.axis[ 0 ], viewDirection );
									VectorNormalize( viewDirection );

									// calculate new up dir
									CrossProduct( lightDirection, viewDirection, side );
									VectorNormalize( side );

									CrossProduct( side, lightDirection, up );
									VectorNormalize( up );

#if 0
									VectorToAngles( lightDirection, angles );
									MatrixFromAngles( rotationMatrix, angles[ PITCH ], angles[ YAW ], angles[ ROLL ] );
									AngleVectors( angles, forward, side, up );
#endif

									MatrixLookAtRH( light->viewMatrix, viewOrigin, lightDirection, up );

#if 0
									ri.Printf( PRINT_ALL, "light = (%5.3f, %5.3f, %5.3f)\n", lightDirection[ 0 ], lightDirection[ 1 ], lightDirection[ 2 ] );
									ri.Printf( PRINT_ALL, "side = (%5.3f, %5.3f, %5.3f)\n", side[ 0 ], side[ 1 ], side[ 2 ] );
									ri.Printf( PRINT_ALL, "up = (%5.3f, %5.3f, %5.3f)\n", up[ 0 ], up[ 1 ], up[ 2 ] );
#endif

#if 0

									for ( j = 0; j < 6; j++ )
									{
										VectorCopy( backEnd.viewParms.frustums[ splitFrustumIndex ][ j ].normal, splitFrustum[ j ] );
										splitFrustum[ j ][ 3 ] = backEnd.viewParms.frustums[ splitFrustumIndex ][ j ].dist;
									}

									PlanesGetIntersectionPoint( splitFrustum[ FRUSTUM_LEFT ], splitFrustum[ FRUSTUM_TOP ], splitFrustum[ FRUSTUM_NEAR ], splitFrustumCorners[ 0 ] );
									PlanesGetIntersectionPoint( splitFrustum[ FRUSTUM_RIGHT ], splitFrustum[ FRUSTUM_TOP ], splitFrustum[ FRUSTUM_NEAR ], splitFrustumCorners[ 1 ] );
									PlanesGetIntersectionPoint( splitFrustum[ FRUSTUM_RIGHT ], splitFrustum[ FRUSTUM_BOTTOM ], splitFrustum[ FRUSTUM_NEAR ], splitFrustumCorners[ 2 ] );
									PlanesGetIntersectionPoint( splitFrustum[ FRUSTUM_LEFT ], splitFrustum[ FRUSTUM_BOTTOM ], splitFrustum[ FRUSTUM_NEAR ], splitFrustumCorners[ 3 ] );

#if 0
									ri.Printf( PRINT_ALL, "split frustum %i\n", splitFrustumIndex );
									ri.Printf( PRINT_ALL, "pyramid nearCorners\n" );

									for ( j = 0; j < 4; j++ )
									{
										ri.Printf( PRINT_ALL, "(%5.3f, %5.3f, %5.3f)\n", splitFrustumCorners[ j ][ 0 ], splitFrustumCorners[ j ][ 1 ], splitFrustumCorners[ j ][ 2 ] );
									}

#endif

									PlanesGetIntersectionPoint( splitFrustum[ FRUSTUM_LEFT ], splitFrustum[ FRUSTUM_TOP ], splitFrustum[ FRUSTUM_FAR ], splitFrustumCorners[ 4 ] );
									PlanesGetIntersectionPoint( splitFrustum[ FRUSTUM_RIGHT ], splitFrustum[ FRUSTUM_TOP ], splitFrustum[ FRUSTUM_FAR ], splitFrustumCorners[ 5 ] );
									PlanesGetIntersectionPoint( splitFrustum[ FRUSTUM_RIGHT ], splitFrustum[ FRUSTUM_BOTTOM ], splitFrustum[ FRUSTUM_FAR ], splitFrustumCorners[ 6 ] );
									PlanesGetIntersectionPoint( splitFrustum[ FRUSTUM_LEFT ], splitFrustum[ FRUSTUM_BOTTOM ], splitFrustum[ FRUSTUM_FAR ], splitFrustumCorners[ 7 ] );

#if 0
									ri.Printf( PRINT_ALL, "pyramid farCorners\n" );

									for ( j = 4; j < 8; j++ )
									{
										ri.Printf( PRINT_ALL, "(%5.3f, %5.3f, %5.3f)\n", splitFrustumCorners[ j ][ 0 ], splitFrustumCorners[ j ][ 1 ], splitFrustumCorners[ j ][ 2 ] );
									}

#endif
#endif

									ClearBounds( splitFrustumBounds[ 0 ], splitFrustumBounds[ 1 ] );
#if 0

									for ( i = 0; i < 8; i++ )
									{
										AddPointToBounds( splitFrustumCorners[ i ], splitFrustumBounds[ 0 ], splitFrustumBounds[ 1 ] );
									}

#endif
									//BoundsAdd(splitFrustumBounds[0], splitFrustumBounds[1], backEnd.viewParms.visBounds[0], backEnd.viewParms.visBounds[1]);
									BoundsAdd( splitFrustumBounds[ 0 ], splitFrustumBounds[ 1 ], light->worldBounds[ 0 ], light->worldBounds[ 1 ] );

									ClearBounds( cropBounds[ 0 ], cropBounds[ 1 ] );

									for ( j = 0; j < 8; j++ )
									{
										point[ 0 ] = splitFrustumBounds[ j & 1 ][ 0 ];
										point[ 1 ] = splitFrustumBounds[( j >> 1 ) & 1 ][ 1 ];
										point[ 2 ] = splitFrustumBounds[( j >> 2 ) & 1 ][ 2 ];
										point[ 3 ] = 1;

#if 1
										MatrixTransform4( light->viewMatrix, point, transf );
										transf[ 0 ] /= transf[ 3 ];
										transf[ 1 ] /= transf[ 3 ];
										transf[ 2 ] /= transf[ 3 ];
#else
										MatrixTransformPoint( light->viewMatrix, point, transf );
#endif
										AddPointToBounds( transf, cropBounds[ 0 ], cropBounds[ 1 ] );
									}

#if 0
									MatrixOrthogonalProjection( projectionMatrix, cropBounds[ 0 ][ 0 ], cropBounds[ 1 ][ 0 ], cropBounds[ 0 ][ 1 ], cropBounds[ 1 ][ 1 ], cropBounds[ 0 ][ 2 ], cropBounds[ 1 ][ 2 ] );
									MatrixMultiply( projectionMatrix, light->viewMatrix, viewProjectionMatrix );
									MatrixMultiply( viewProjectionMatrix, backEnd.viewParms.world.viewMatrix, postMatrix );

									VectorSet( viewOrigin, 0, 0, 0 );
									MatrixTransformPoint2( viewOrigin );

									VectorSet( viewDirection, 0, 0, -1 );
									MatrixTransformPoint2( viewDirection );

									VectorSet( up, 0, 1, 0 );
									MatrixTransformPoint2( viewDirection );
#endif

#if 0
									ri.Printf( PRINT_ALL, "light space crop bounds (%5.3f, %5.3f, %5.3f) (%5.3f, %5.3f, %5.3f)\n",
									           cropBounds[ 0 ][ 0 ], cropBounds[ 0 ][ 1 ], cropBounds[ 0 ][ 2 ],
									           cropBounds[ 1 ][ 0 ], cropBounds[ 1 ][ 1 ], cropBounds[ 1 ][ 2 ] );
#endif

#if 0
									ri.Printf( PRINT_ALL, "cropMatrix =\n(%5.3f, %5.3f, %5.3f, %5.3f)\n"
									           "(%5.3f, %5.3f, %5.3f, %5.3f)\n"
									           "(%5.3f, %5.3f, %5.3f, %5.3f)\n"
									           "(%5.3f, %5.3f, %5.3f, %5.3f)\n\n",
									           cropMatrix[ 0 ], cropMatrix[ 4 ], cropMatrix[ 8 ], cropMatrix[ 12 ],
									           cropMatrix[ 1 ], cropMatrix[ 5 ], cropMatrix[ 9 ], cropMatrix[ 13 ],
									           cropMatrix[ 2 ], cropMatrix[ 6 ], cropMatrix[ 10 ], cropMatrix[ 14 ],
									           cropMatrix[ 3 ], cropMatrix[ 7 ], cropMatrix[ 11 ], cropMatrix[ 15 ] );
#endif

									{
										float  gamma;
										float  cosGamma;
										float  sinGamma;
										float  zNear, zFar;
										float  depth;
										float  n, f;
										vec3_t viewOriginLS, Cstart_lp, C;

										// use the formulas of the paper to get n (and f)
#if 0
										cosGamma = DotProduct( viewDirection, lightDirection );
										sinGamma = sqrt( 1.0f - cosGamma * cosGamma );
#else
										gamma = AngleBetweenVectors( viewDirection, lightDirection );
										sinGamma = sin( DEG2RAD( gamma ) );
#endif

										depth = fabs( cropBounds[ 1 ][ 1 ] - cropBounds[ 0 ][ 1 ] );  //perspective transform depth //light space y extents
										//depth = fabs(cropBounds[0][2]) + fabs(cropBounds[1][2]);

#if 1
										zNear = backEnd.viewParms.zNear / sinGamma;
										zFar = zNear + depth * sinGamma;
										n = ( zNear + sqrt( zFar * zNear ) ) / sinGamma;
#elif 0
										zNear = backEnd.viewParms.zNear;
										zFar = backEnd.viewParms.zFar;

										n = ( zNear + sqrt( zFar * zNear ) ) / sinGamma;
#else
										zNear = backEnd.viewParms.zNear;
										zFar = zNear + depth * sinGamma;
										n = ( zNear + sqrt( zFar * zNear ) ) / sinGamma;
#endif
										f = n + depth;

										ri.Printf( PRINT_ALL, "gamma = %5.3f, sin(gamma) = %5.3f, n = %5.3f, f = %5.3f\n", gamma, sinGamma, n, f );

										// new observer point n-1 behind eye position:  pos = eyePos-up*(n-nearDist)
#if 1
										VectorMA( viewOrigin, - ( n - zNear ), up, C );
										//VectorMA(C, depth * 0.5f, lightDirection, C);
#else
										// get the coordinates of the near camera point in light space
										MatrixTransformPoint( light->viewMatrix, viewOrigin, viewOriginLS );

										// c start has the x and y coordinate of e, the z coord of B.min()
										VectorSet( Cstart_lp, viewOriginLS[ 0 ], viewOriginLS[ 1 ], depth * 0.5f );

										// calc C the projection center
										// new projection center C, n behind the near plane of P
										VectorMA( Cstart_lp, -n, axisDefault[ 1 ], C );
										MatrixAffineInverse( light->viewMatrix, transformMatrix );
										MatrixTransformPoint2( transformMatrix, C );
#endif

#if 1
										MatrixLookAtRH( light->viewMatrix, C, lightDirection, up );
#else
										VectorInverse( up );
										MatrixLookAtRH( light->viewMatrix, C, up, lightDirection );
										VectorInverse( up );

										//MatrixLookAtRH(light->viewMatrix, viewOrigin, viewDirection, backEnd.viewParms.orientation.axis[2]);
#endif

#if 0

										if ( n >= FLT_MAX )
										{
											// if n is infinite than we should do uniform shadow mapping
											MatrixIdentity( lispMatrix );
										}
										else
#endif
										{
											// one possibility for a simple perspective transformation matrix
											// with the two parameters n(near) and f(far) in y direction
											float a, b;
											float *m = lispMatrix;

											a = ( f + n ) / ( f - n );
											b = -2 * f * n / ( f - n );

											//a = f / (n - f);
											//b = (n * f) / (n - f);

											m[ 0 ] = 1;
											m[ 4 ] = 0;
											m[ 8 ] = 0;
											m[ 12 ] = 0;
											m[ 1 ] = 0;
											m[ 5 ] = a;
											m[ 9 ] = 0;
											m[ 13 ] = b;
											m[ 2 ] = 0;
											m[ 6 ] = 0;
											m[ 10 ] = 1;
											m[ 14 ] = 0;
											m[ 3 ] = 0;
											m[ 7 ] = 1;
											m[ 11 ] = 0;
											m[ 15 ] = 0;

											//MatrixPerspectiveProjectionRH(lispMatrix, -1, 1, n, f, -1, 1);
											//MatrixPerspectiveProjection(lispMatrix, -1, 1, -1, 1, n, f);
											//MatrixInverse(lispMatrix);

											//MatrixPerspectiveProjectionLH(lispMatrix, cropBounds[0][0], cropBounds[1][0], cropBounds[0][1], cropBounds[1][1], cropBounds[0][2], cropBounds[1][2]);
											//MatrixPerspectiveProjectionRH(lispMatrix, cropBounds[0][0], cropBounds[1][0], cropBounds[0][1], cropBounds[1][1], -f, -n);

#if 0
											ri.Printf( PRINT_ALL, "lispMatrix =\n(%5.3f, %5.3f, %5.3f, %5.3f)\n"
											           "(%5.3f, %5.3f, %5.3f, %5.3f)\n"
											           "(%5.3f, %5.3f, %5.3f, %5.3f)\n"
											           "(%5.3f, %5.3f, %5.3f, %5.3f)\n\n",
											           m[ 0 ], m[ 4 ], m[ 8 ], m[ 12 ],
											           m[ 1 ], m[ 5 ], m[ 9 ], m[ 13 ],
											           m[ 2 ], m[ 6 ], m[ 10 ], m[ 14 ],
											           m[ 3 ], m[ 7 ], m[ 11 ], m[ 15 ] );
#endif
										}

										//MatrixIdentity(lispMatrix);

										// temporal arrangement for the transformation of the points to post-perspective space
#if 0
										MatrixCopy( flipZMatrix, viewProjectionMatrix );
										//MatrixMultiply2(viewProjectionMatrix, switchToGL);
										MatrixMultiply2( viewProjectionMatrix, lispMatrix );
										//MatrixMultiply2(viewProjectionMatrix, switchToGL);
										//MatrixMultiplyScale(viewProjectionMatrix, 1, 1, -1);
										//MatrixMultiply(flipZMatrix, lispMatrix, viewProjectionMatrix);
										//MatrixMultiply(lispMatrix, light->viewMatrix, viewProjectionMatrix);
										//MatrixMultiply2(viewProjectionMatrix, cropMatrix);
										MatrixMultiply2( viewProjectionMatrix, light->viewMatrix );
										//MatrixMultiply2(viewProjectionMatrix, projectionCenter);
										//MatrixMultiply2(viewProjectionMatrix, transformMatrix);
#else
										MatrixMultiply( lispMatrix, light->viewMatrix, viewProjectionMatrix );
										//MatrixMultiply(flipZMatrix, lispMatrix, viewProjectionMatrix);
										//MatrixMultiply2(viewProjectionMatrix, light->viewMatrix);
#endif
										//MatrixMultiply(lispMatrix, light->viewMatrix, viewProjectionMatrix);
										//MatrixMultiply(light->viewMatrix, lispMatrix, viewProjectionMatrix);

										//transform the light volume points from world into the distorted light space
										//transformVecPoint(&Bcopy,lightProjection);

										//calculate the cubic hull (an AABB)
										//of the light space extents of the intersection body B
										//and save the two extreme points min and max
										//calcCubicHull(min,max,Bcopy.points,Bcopy.size);

#if 0

										VectorSet( forward, 1, 0, 0 );
										VectorSet( side, 0, 1, 0 );
										VectorSet( up, 0, 0, 1 );

										MatrixTransformNormal2( viewProjectionMatrix, forward );
										MatrixTransformNormal2( viewProjectionMatrix, side );
										MatrixTransformNormal2( viewProjectionMatrix, up );

										ri.Printf( PRINT_ALL, "forward = (%5.3f, %5.3f, %5.3f)\n", forward[ 0 ], forward[ 1 ], forward[ 2 ] );
										ri.Printf( PRINT_ALL, "side = (%5.3f, %5.3f, %5.3f)\n", side[ 0 ], side[ 1 ], side[ 2 ] );
										ri.Printf( PRINT_ALL, "up = (%5.3f, %5.3f, %5.3f)\n", up[ 0 ], up[ 1 ], up[ 2 ] );
#endif

#if 1
										ClearBounds( cropBounds[ 0 ], cropBounds[ 1 ] );

										for ( j = 0; j < 8; j++ )
										{
											point[ 0 ] = splitFrustumBounds[ j & 1 ][ 0 ];
											point[ 1 ] = splitFrustumBounds[( j >> 1 ) & 1 ][ 1 ];
											point[ 2 ] = splitFrustumBounds[( j >> 2 ) & 1 ][ 2 ];
											point[ 3 ] = 1;

											MatrixTransform4( viewProjectionMatrix, point, transf );
											transf[ 0 ] /= transf[ 3 ];
											transf[ 1 ] /= transf[ 3 ];
											transf[ 2 ] /= transf[ 3 ];

											AddPointToBounds( transf, cropBounds[ 0 ], cropBounds[ 1 ] );
										}

										MatrixScaleTranslateToUnitCube( cropMatrix, cropBounds[ 0 ], cropBounds[ 1 ] );
										//MatrixOrthogonalProjection(cropMatrix, cropBounds[0][0], cropBounds[1][0], cropBounds[0][1], cropBounds[1][1], cropBounds[0][2], cropBounds[1][2]);
#endif
									}

#if 0
									ri.Printf( PRINT_ALL, "light space post crop bounds (%5.3f, %5.3f, %5.3f) (%5.3f, %5.3f, %5.3f)\n",
									           cropBounds[ 0 ][ 0 ], cropBounds[ 0 ][ 1 ], cropBounds[ 0 ][ 2 ],
									           cropBounds[ 1 ][ 0 ], cropBounds[ 1 ][ 1 ], cropBounds[ 1 ][ 2 ] );
#endif

									//
									//MatrixInverse(cropMatrix);

#if 0
									ri.Printf( PRINT_ALL, "cropMatrix =\n(%5.3f, %5.3f, %5.3f, %5.3f)\n"
									           "(%5.3f, %5.3f, %5.3f, %5.3f)\n"
									           "(%5.3f, %5.3f, %5.3f, %5.3f)\n"
									           "(%5.3f, %5.3f, %5.3f, %5.3f)\n\n",
									           cropMatrix[ 0 ], cropMatrix[ 4 ], cropMatrix[ 8 ], cropMatrix[ 12 ],
									           cropMatrix[ 1 ], cropMatrix[ 5 ], cropMatrix[ 9 ], cropMatrix[ 13 ],
									           cropMatrix[ 2 ], cropMatrix[ 6 ], cropMatrix[ 10 ], cropMatrix[ 14 ],
									           cropMatrix[ 3 ], cropMatrix[ 7 ], cropMatrix[ 11 ], cropMatrix[ 15 ] );
#endif

									//MatrixOrthogonalProjectionLH(cropMatrix, -1024, 1024, -1024, 1024, cropBounds[0][2], cropBounds[1][2]);

									//MatrixIdentity(light->projectionMatrix);
									MatrixCopy( flipZMatrix, light->projectionMatrix );

									//MatrixMultiply2(light->projectionMatrix, switchToArticle);
									MatrixMultiply2( light->projectionMatrix, cropMatrix );
									MatrixMultiply2( light->projectionMatrix, lispMatrix );

									GL_LoadProjectionMatrix( light->projectionMatrix );
								}
								else // if(r_lightSpacePerspectiveWarping->integer)
#endif
									if ( r_parallelShadowSplits->integer )
									{
										// original light direction is from surface to light
										VectorInverse( lightDirection );
										VectorNormalize( lightDirection );

										VectorCopy( backEnd.viewParms.orientation.origin, viewOrigin );
										VectorCopy( backEnd.viewParms.orientation.axis[ 0 ], viewDirection );
										VectorNormalize( viewDirection );

#if 1
										// calculate new up dir
										CrossProduct( lightDirection, viewDirection, side );
										VectorNormalize( side );

										CrossProduct( side, lightDirection, up );
										VectorNormalize( up );

										VectorToAngles( lightDirection, angles );
										MatrixFromAngles( rotationMatrix, angles[ PITCH ], angles[ YAW ], angles[ ROLL ] );
										AngleVectors( angles, forward, side, up );

										MatrixLookAtRH( light->viewMatrix, viewOrigin, lightDirection, up );
#else
										MatrixLookAtRH( light->viewMatrix, viewOrigin, lightDirection, viewDirection );
#endif

										for ( j = 0; j < 6; j++ )
										{
											VectorCopy( backEnd.viewParms.frustums[ 1 + splitFrustumIndex ][ j ].normal, splitFrustum[ j ] );
											splitFrustum[ j ][ 3 ] = backEnd.viewParms.frustums[ 1 + splitFrustumIndex ][ j ].dist;
										}

										// calculate split frustum corner points
										PlanesGetIntersectionPoint( splitFrustum[ FRUSTUM_LEFT ], splitFrustum[ FRUSTUM_TOP ], splitFrustum[ FRUSTUM_NEAR ], splitFrustumCorners[ 0 ] );
										PlanesGetIntersectionPoint( splitFrustum[ FRUSTUM_RIGHT ], splitFrustum[ FRUSTUM_TOP ], splitFrustum[ FRUSTUM_NEAR ], splitFrustumCorners[ 1 ] );
										PlanesGetIntersectionPoint( splitFrustum[ FRUSTUM_RIGHT ], splitFrustum[ FRUSTUM_BOTTOM ], splitFrustum[ FRUSTUM_NEAR ], splitFrustumCorners[ 2 ] );
										PlanesGetIntersectionPoint( splitFrustum[ FRUSTUM_LEFT ], splitFrustum[ FRUSTUM_BOTTOM ], splitFrustum[ FRUSTUM_NEAR ], splitFrustumCorners[ 3 ] );

										PlanesGetIntersectionPoint( splitFrustum[ FRUSTUM_LEFT ], splitFrustum[ FRUSTUM_TOP ], splitFrustum[ FRUSTUM_FAR ], splitFrustumCorners[ 4 ] );
										PlanesGetIntersectionPoint( splitFrustum[ FRUSTUM_RIGHT ], splitFrustum[ FRUSTUM_TOP ], splitFrustum[ FRUSTUM_FAR ], splitFrustumCorners[ 5 ] );
										PlanesGetIntersectionPoint( splitFrustum[ FRUSTUM_RIGHT ], splitFrustum[ FRUSTUM_BOTTOM ], splitFrustum[ FRUSTUM_FAR ], splitFrustumCorners[ 6 ] );
										PlanesGetIntersectionPoint( splitFrustum[ FRUSTUM_LEFT ], splitFrustum[ FRUSTUM_BOTTOM ], splitFrustum[ FRUSTUM_FAR ], splitFrustumCorners[ 7 ] );

										if ( r_logFile->integer )
										{
											vec3_t rayIntersectionNear, rayIntersectionFar;
											float  zNear, zFar;

											// don't just call LogComment, or we will get
											// a call to va() every frame!
											//GLimp_LogComment(va("----- Skipping shadowCube side: %i -----\n", cubeSide));

											PlaneIntersectRay( viewOrigin, viewDirection, splitFrustum[ FRUSTUM_FAR ], rayIntersectionFar );
											zFar = Distance( viewOrigin, rayIntersectionFar );

											VectorInverse( viewDirection );

											PlaneIntersectRay( rayIntersectionFar, viewDirection, splitFrustum[ FRUSTUM_NEAR ], rayIntersectionNear );
											zNear = Distance( viewOrigin, rayIntersectionNear );

											VectorInverse( viewDirection );

											GLimp_LogComment( va( "split frustum %i: near = %5.3f, far = %5.3f\n", splitFrustumIndex, zNear, zFar ) );
											GLimp_LogComment( va( "pyramid nearCorners\n" ) );

											for ( j = 0; j < 4; j++ )
											{
												GLimp_LogComment( va( "(%5.3f, %5.3f, %5.3f)\n", splitFrustumCorners[ j ][ 0 ], splitFrustumCorners[ j ][ 1 ], splitFrustumCorners[ j ][ 2 ] ) );
											}

											GLimp_LogComment( va( "pyramid farCorners\n" ) );

											for ( j = 4; j < 8; j++ )
											{
												GLimp_LogComment( va( "(%5.3f, %5.3f, %5.3f)\n", splitFrustumCorners[ j ][ 0 ], splitFrustumCorners[ j ][ 1 ], splitFrustumCorners[ j ][ 2 ] ) );
											}
										}

										ClearBounds( splitFrustumBounds[ 0 ], splitFrustumBounds[ 1 ] );

										for ( i = 0; i < 8; i++ )
										{
											AddPointToBounds( splitFrustumCorners[ i ], splitFrustumBounds[ 0 ], splitFrustumBounds[ 1 ] );
										}

#if 0
										// find the bounding box of the current split in the light's view space
										ClearBounds( splitFrustumViewBounds[ 0 ], splitFrustumViewBounds[ 1 ] );
										numCasters = MergeInteractionBounds( light->viewMatrix, ia, iaCount, splitFrustumViewBounds, qtrue );

										for ( j = 0; j < 8; j++ )
										{
											VectorCopy( splitFrustumCorners[ j ], point );
											point[ 3 ] = 1;

											MatrixTransform4( light->viewMatrix, point, transf );
											transf[ 0 ] /= transf[ 3 ];
											transf[ 1 ] /= transf[ 3 ];
											transf[ 2 ] /= transf[ 3 ];

											AddPointToBounds( transf, splitFrustumViewBounds[ 0 ], splitFrustumViewBounds[ 1 ] );
										}

										//MatrixScaleTranslateToUnitCube(projectionMatrix, splitFrustumViewBounds[0], splitFrustumViewBounds[1]);
										//MatrixOrthogonalProjectionRH(projectionMatrix, -1, 1, -1, 1, -splitFrustumViewBounds[1][2], -splitFrustumViewBounds[0][2]);
										MatrixOrthogonalProjectionRH( projectionMatrix,  splitFrustumViewBounds[ 0 ][ 0 ],
										                              splitFrustumViewBounds[ 1 ][ 0 ],
										                              splitFrustumViewBounds[ 0 ][ 1 ],
										                              splitFrustumViewBounds[ 1 ][ 1 ],
										                              -splitFrustumViewBounds[ 1 ][ 2 ],
										                              -splitFrustumViewBounds[ 0 ][ 2 ] );

										MatrixMultiply( projectionMatrix, light->viewMatrix, viewProjectionMatrix );

										// find the bounding box of the current split in the light's clip space
										ClearBounds( splitFrustumClipBounds[ 0 ], splitFrustumClipBounds[ 1 ] );

										for ( j = 0; j < 8; j++ )
										{
											VectorCopy( splitFrustumCorners[ j ], point );
											point[ 3 ] = 1;

											MatrixTransform4( viewProjectionMatrix, point, transf );
											transf[ 0 ] /= transf[ 3 ];
											transf[ 1 ] /= transf[ 3 ];
											transf[ 2 ] /= transf[ 3 ];

											AddPointToBounds( transf, splitFrustumClipBounds[ 0 ], splitFrustumClipBounds[ 1 ] );
										}

										splitFrustumClipBounds[ 0 ][ 2 ] = 0;
										splitFrustumClipBounds[ 1 ][ 2 ] = 1;

										MatrixCrop( cropMatrix, splitFrustumClipBounds[ 0 ], splitFrustumClipBounds[ 1 ] );
										//MatrixIdentity(cropMatrix);

										if ( r_logFile->integer )
										{
											GLimp_LogComment( va( "split frustum light view space bounds (%5.3f, %5.3f, %5.3f) (%5.3f, %5.3f, %5.3f)\n",
											                      splitFrustumViewBounds[ 0 ][ 0 ], splitFrustumViewBounds[ 0 ][ 1 ], splitFrustumViewBounds[ 0 ][ 2 ],
											                      splitFrustumViewBounds[ 1 ][ 0 ], splitFrustumViewBounds[ 1 ][ 1 ], splitFrustumViewBounds[ 1 ][ 2 ] ) );

											GLimp_LogComment( va( "split frustum light clip space bounds (%5.3f, %5.3f, %5.3f) (%5.3f, %5.3f, %5.3f)\n",
											                      splitFrustumClipBounds[ 0 ][ 0 ], splitFrustumClipBounds[ 0 ][ 1 ], splitFrustumClipBounds[ 0 ][ 2 ],
											                      splitFrustumClipBounds[ 1 ][ 0 ], splitFrustumClipBounds[ 1 ][ 1 ], splitFrustumClipBounds[ 1 ][ 2 ] ) );
										}

#else

										// find the bounding box of the current split in the light's view space
										ClearBounds( cropBounds[ 0 ], cropBounds[ 1 ] );

										for ( j = 0; j < 8; j++ )
										{
											VectorCopy( splitFrustumCorners[ j ], point );
											point[ 3 ] = 1;

											MatrixTransform4( light->viewMatrix, point, transf );
											transf[ 0 ] /= transf[ 3 ];
											transf[ 1 ] /= transf[ 3 ];
											transf[ 2 ] /= transf[ 3 ];

											AddPointToBounds( transf, cropBounds[ 0 ], cropBounds[ 1 ] );
										}

										MatrixOrthogonalProjectionRH( projectionMatrix, cropBounds[ 0 ][ 0 ], cropBounds[ 1 ][ 0 ], cropBounds[ 0 ][ 1 ], cropBounds[ 1 ][ 1 ], -cropBounds[ 1 ][ 2 ], -cropBounds[ 0 ][ 2 ] );

										MatrixMultiply( projectionMatrix, light->viewMatrix, viewProjectionMatrix );

										numCasters = MergeInteractionBounds( viewProjectionMatrix, ia, iaCount, casterBounds, true );
										MergeInteractionBounds( viewProjectionMatrix, ia, iaCount, receiverBounds, false );

										// find the bounding box of the current split in the light's clip space
										ClearBounds( splitFrustumClipBounds[ 0 ], splitFrustumClipBounds[ 1 ] );

										for ( j = 0; j < 8; j++ )
										{
											VectorCopy( splitFrustumCorners[ j ], point );
											point[ 3 ] = 1;

											MatrixTransform4( viewProjectionMatrix, point, transf );
											transf[ 0 ] /= transf[ 3 ];
											transf[ 1 ] /= transf[ 3 ];
											transf[ 2 ] /= transf[ 3 ];

											AddPointToBounds( transf, splitFrustumClipBounds[ 0 ], splitFrustumClipBounds[ 1 ] );
										}

										//ri.Printf(PRINT_ALL, "shadow casters = %i\n", numCasters);

										if ( r_logFile->integer )
										{
											GLimp_LogComment( va( "shadow casters = %i\n", numCasters ) );

											GLimp_LogComment( va( "split frustum light space clip bounds (%5.3f, %5.3f, %5.3f) (%5.3f, %5.3f, %5.3f)\n",
											                      splitFrustumClipBounds[ 0 ][ 0 ], splitFrustumClipBounds[ 0 ][ 1 ], splitFrustumClipBounds[ 0 ][ 2 ],
											                      splitFrustumClipBounds[ 1 ][ 0 ], splitFrustumClipBounds[ 1 ][ 1 ], splitFrustumClipBounds[ 1 ][ 2 ] ) );

											GLimp_LogComment( va( "shadow caster light space clip bounds (%5.3f, %5.3f, %5.3f) (%5.3f, %5.3f, %5.3f)\n",
											                      casterBounds[ 0 ][ 0 ], casterBounds[ 0 ][ 1 ], casterBounds[ 0 ][ 2 ],
											                      casterBounds[ 1 ][ 0 ], casterBounds[ 1 ][ 1 ], casterBounds[ 1 ][ 2 ] ) );

											GLimp_LogComment( va( "light receiver light space clip bounds (%5.3f, %5.3f, %5.3f) (%5.3f, %5.3f, %5.3f)\n",
											                      receiverBounds[ 0 ][ 0 ], receiverBounds[ 0 ][ 1 ], receiverBounds[ 0 ][ 2 ],
											                      receiverBounds[ 1 ][ 0 ], receiverBounds[ 1 ][ 1 ], receiverBounds[ 1 ][ 2 ] ) );
										}

										// scene-dependent bounding volume
										cropBounds[ 0 ][ 0 ] = Q_max( Q_max( casterBounds[ 0 ][ 0 ], receiverBounds[ 0 ][ 0 ] ), splitFrustumClipBounds[ 0 ][ 0 ] );
										cropBounds[ 0 ][ 1 ] = Q_max( Q_max( casterBounds[ 0 ][ 1 ], receiverBounds[ 0 ][ 1 ] ), splitFrustumClipBounds[ 0 ][ 1 ] );

										cropBounds[ 1 ][ 0 ] = Q_min( Q_min( casterBounds[ 1 ][ 0 ], receiverBounds[ 1 ][ 0 ] ), splitFrustumClipBounds[ 1 ][ 0 ] );
										cropBounds[ 1 ][ 1 ] = Q_min( Q_min( casterBounds[ 1 ][ 1 ], receiverBounds[ 1 ][ 1 ] ), splitFrustumClipBounds[ 1 ][ 1 ] );

										cropBounds[ 0 ][ 2 ] = Q_min( casterBounds[ 0 ][ 2 ], splitFrustumClipBounds[ 0 ][ 2 ] );
										//cropBounds[0][2] = casterBounds[0][2];
										//cropBounds[0][2] = splitFrustumClipBounds[0][2];
										cropBounds[ 1 ][ 2 ] = Q_min( receiverBounds[ 1 ][ 2 ], splitFrustumClipBounds[ 1 ][ 2 ] );
										//cropBounds[1][2] = splitFrustumClipBounds[1][2];

										if ( numCasters == 0 )
										{
											VectorCopy( splitFrustumClipBounds[ 0 ], cropBounds[ 0 ] );
											VectorCopy( splitFrustumClipBounds[ 1 ], cropBounds[ 1 ] );
										}

										MatrixCrop( cropMatrix, cropBounds[ 0 ], cropBounds[ 1 ] );
#endif

										MatrixMultiply( cropMatrix, projectionMatrix, light->projectionMatrix );

										GL_LoadProjectionMatrix( light->projectionMatrix );
									}
									else
									{
										// original light direction is from surface to light
										VectorInverse( lightDirection );

										// Quake -> OpenGL view matrix from light perspective
#if 1
										VectorToAngles( lightDirection, angles );
										MatrixFromAngles( rotationMatrix, angles[ PITCH ], angles[ YAW ], angles[ ROLL ] );
										MatrixSetupTransformFromRotation( transformMatrix, rotationMatrix, backEnd.viewParms.orientation.origin );
										MatrixAffineInverse( transformMatrix, viewMatrix );
										MatrixMultiply( quakeToOpenGLMatrix, viewMatrix, light->viewMatrix );
#else
										MatrixLookAtRH( light->viewMatrix, backEnd.viewParms.orientation.origin, lightDirection, backEnd.viewParms.orientation.axis[ 0 ] );
#endif

										ClearBounds( splitFrustumBounds[ 0 ], splitFrustumBounds[ 1 ] );
										//BoundsAdd(splitFrustumBounds[0], splitFrustumBounds[1], backEnd.viewParms.visBounds[0], backEnd.viewParms.visBounds[1]);
										BoundsAdd( splitFrustumBounds[ 0 ], splitFrustumBounds[ 1 ], light->worldBounds[ 0 ], light->worldBounds[ 1 ] );

										ClearBounds( cropBounds[ 0 ], cropBounds[ 1 ] );

										for ( j = 0; j < 8; j++ )
										{
											point[ 0 ] = splitFrustumBounds[ j & 1 ][ 0 ];
											point[ 1 ] = splitFrustumBounds[( j >> 1 ) & 1 ][ 1 ];
											point[ 2 ] = splitFrustumBounds[( j >> 2 ) & 1 ][ 2 ];
											point[ 3 ] = 1;
#if 1
											MatrixTransform4( light->viewMatrix, point, transf );
											transf[ 0 ] /= transf[ 3 ];
											transf[ 1 ] /= transf[ 3 ];
											transf[ 2 ] /= transf[ 3 ];
#else
											MatrixTransformPoint( light->viewMatrix, point, transf );
#endif
											AddPointToBounds( transf, cropBounds[ 0 ], cropBounds[ 1 ] );
										}

										// transform from OpenGL's right handed into D3D's left handed coordinate system
#if 0
										MatrixScaleTranslateToUnitCube( projectionMatrix, cropBounds[ 0 ], cropBounds[ 1 ] );
										MatrixMultiply( flipZMatrix, projectionMatrix, light->projectionMatrix );
#else
										MatrixOrthogonalProjectionRH( light->projectionMatrix, cropBounds[ 0 ][ 0 ], cropBounds[ 1 ][ 0 ], cropBounds[ 0 ][ 1 ], cropBounds[ 1 ][ 1 ], -cropBounds[ 1 ][ 2 ], -cropBounds[ 0 ][ 2 ] );
#endif
										GL_LoadProjectionMatrix( light->projectionMatrix );
									}

								break;
							}

						default:
							{
								R_BindFBO( tr.shadowMapFBO[ light->shadowLOD ] );
								break;
							}
					}
				}

				if ( r_logFile->integer )
				{
					// don't just call LogComment, or we will get
					// a call to va() every frame!
					GLimp_LogComment( va( "----- First Shadow Interaction: %i -----\n", iaCount ) );
				}
			}
			else
			{
				GLimp_LogComment( "--- Rendering lighting ---\n" );

				if ( r_logFile->integer )
				{
					// don't just call LogComment, or we will get
					// a call to va() every frame!
					GLimp_LogComment( va( "----- First Light Interaction: %i -----\n", iaCount ) );
				}

				// finally draw light
				R_BindFBO( tr.geometricRenderFBO );
				glDrawBuffers( 1, geometricRenderTargets );

				// restore camera matrices
				GL_LoadProjectionMatrix( backEnd.viewParms.projectionMatrix );
				GL_LoadModelViewMatrix( backEnd.orientation.modelViewMatrix );

				// set the window clipping
				GL_Viewport( backEnd.viewParms.viewportX, backEnd.viewParms.viewportY,
				             backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight );

				GL_Scissor( backEnd.viewParms.viewportX, backEnd.viewParms.viewportY,
				            backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight );

				GL_CheckErrors();

				GLimp_LogComment( "--- Rendering lighting ---\n" );

				switch ( light->l.rlType )
				{
					case RL_OMNI:
						{
							// reset light view and projection matrices
							MatrixAffineInverse( light->transformMatrix, light->viewMatrix );
							MatrixSetupScale( light->projectionMatrix, 1.0 / light->l.radius[ 0 ], 1.0 / light->l.radius[ 1 ],
							                  1.0 / light->l.radius[ 2 ] );

							// build the attenuation matrix
							MatrixSetupTranslation( light->attenuationMatrix, 0.5, 0.5, 0.5 );  // bias
							MatrixMultiplyScale( light->attenuationMatrix, 0.5, 0.5, 0.5 );  // scale
							MatrixMultiply2( light->attenuationMatrix, light->projectionMatrix );  // light projection (frustum)
							MatrixMultiply2( light->attenuationMatrix, light->viewMatrix );

							MatrixCopy( light->attenuationMatrix, light->shadowMatrices[ 0 ] );
							break;
						}

					case RL_PROJ:
						{
							// build the attenuation matrix
							MatrixSetupTranslation( light->attenuationMatrix, 0.5, 0.5, 0.0 );
							MatrixMultiplyScale( light->attenuationMatrix, 0.5, 0.5, 1.0 / Q_min( light->falloffLength, 1.0 ) );
							MatrixMultiply2( light->attenuationMatrix, light->projectionMatrix );
							MatrixMultiply2( light->attenuationMatrix, light->viewMatrix );

							MatrixCopy( light->attenuationMatrix, light->shadowMatrices[ 0 ] );
							break;
						}

					case RL_DIRECTIONAL:
						{
							matrix_t viewMatrix, projectionMatrix;

							// build same attenuation matrix as for box lights so we can clip pixels outside of the light volume
							MatrixAffineInverse( light->transformMatrix, viewMatrix );
							MatrixSetupScale( projectionMatrix, 1.0 / light->l.radius[ 0 ], 1.0 / light->l.radius[ 1 ], 1.0 / light->l.radius[ 2 ] );

							MatrixCopy( bias, light->attenuationMatrix );
							MatrixMultiply2( light->attenuationMatrix, projectionMatrix );
							MatrixMultiply2( light->attenuationMatrix, viewMatrix );
							break;
						}

					default:
						break;
				}

				if ( light->clipsNearPlane )
				{
					// set 2D virtual screen size
					GL_PushMatrix();
					GL_LoadProjectionMatrix( ortho );
					GL_LoadModelViewMatrix( matrixIdentity );

					for ( j = 0; j < 6; j++ )
					{
						VectorCopy( light->frustum[ j ].normal, lightFrustum[ j ] );
						lightFrustum[ j ][ 3 ] = light->frustum[ j ].dist;
					}
				}
				else
				{
					// prepare rendering of the light volume
					// either bind VBO or setup the tess struct
#if 0 // FIXME check why this does not work here
					if ( light->isStatic && light->frustumVBO && light->frustumIBO )
					{
						// render in world space
						backEnd.orientation = backEnd.viewParms.world;

						GL_LoadProjectionMatrix( backEnd.viewParms.projectionMatrix );
						GL_LoadModelViewMatrix( backEnd.viewParms.world.modelViewMatrix );

						R_BindVBO( light->frustumVBO );
						R_BindIBO( light->frustumIBO );

						GL_VertexAttribsState( ATTR_POSITION );

						tess.numVertexes = light->frustumVerts;
						tess.numIndexes = light->frustumIndexes;
					}
					else
#endif
					{
						tess.multiDrawPrimitives = 0;
						tess.numIndexes = 0;
						tess.numVertexes = 0;

						switch ( light->l.rlType )
						{
							case RL_OMNI:
							case RL_DIRECTIONAL:
								{
#if 1
									// render in light space
									R_RotateLightForViewParms( light, &backEnd.viewParms, &backEnd.orientation );

									GL_LoadProjectionMatrix( backEnd.viewParms.projectionMatrix );
									GL_LoadModelViewMatrix( backEnd.orientation.modelViewMatrix );

									Tess_AddCube( vec3_origin, light->localBounds[ 0 ], light->localBounds[ 1 ], colorWhite );

									Tess_UpdateVBOs( ATTR_POSITION | ATTR_COLOR );
#else
									matrix_t transform, scale, rot;
									axis_t   axis;

									MatrixSetupScale( scale, light->l.radius[ 0 ], light->l.radius[ 1 ], light->l.radius[ 2 ] );
									MatrixMultiply( scale, light->transformMatrix, transform );

									GL_LoadModelViewMatrix( transform );
									GL_LoadProjectionMatrix( backEnd.viewParms.projectionMatrix );

									R_BindVBO( tr.unitCubeVBO );
									R_BindIBO( tr.unitCubeIBO );

									GL_VertexAttribsState( ATTR_POSITION );

									tess.multiDrawPrimitives = 0;
									tess.numVertexes = tr.unitCubeVBO->vertexesNum;
									tess.numIndexes = tr.unitCubeIBO->indexesNum;
#endif
									break;
								}

							case RL_PROJ:
								{
									vec3_t farCorners[ 4 ];
									vec4_t *frustum = light->localFrustum;

									// render in light space
									R_RotateLightForViewParms( light, &backEnd.viewParms, &backEnd.orientation );

									GL_LoadProjectionMatrix( backEnd.viewParms.projectionMatrix );
									GL_LoadModelViewMatrix( backEnd.orientation.modelViewMatrix );

									PlanesGetIntersectionPoint( frustum[ FRUSTUM_LEFT ], frustum[ FRUSTUM_TOP ], frustum[ FRUSTUM_FAR ], farCorners[ 0 ] );
									PlanesGetIntersectionPoint( frustum[ FRUSTUM_RIGHT ], frustum[ FRUSTUM_TOP ], frustum[ FRUSTUM_FAR ], farCorners[ 1 ] );
									PlanesGetIntersectionPoint( frustum[ FRUSTUM_RIGHT ], frustum[ FRUSTUM_BOTTOM ], frustum[ FRUSTUM_FAR ], farCorners[ 2 ] );
									PlanesGetIntersectionPoint( frustum[ FRUSTUM_LEFT ], frustum[ FRUSTUM_BOTTOM ], frustum[ FRUSTUM_FAR ], farCorners[ 3 ] );

									if ( !VectorCompare( light->l.projStart, vec3_origin ) )
									{
										vec3_t nearCorners[ 4 ];

										// calculate the vertices defining the top area
										PlanesGetIntersectionPoint( frustum[ FRUSTUM_LEFT ], frustum[ FRUSTUM_TOP ], frustum[ FRUSTUM_NEAR ], nearCorners[ 0 ] );
										PlanesGetIntersectionPoint( frustum[ FRUSTUM_RIGHT ], frustum[ FRUSTUM_TOP ], frustum[ FRUSTUM_NEAR ], nearCorners[ 1 ] );
										PlanesGetIntersectionPoint( frustum[ FRUSTUM_RIGHT ], frustum[ FRUSTUM_BOTTOM ], frustum[ FRUSTUM_NEAR ], nearCorners[ 2 ] );
										PlanesGetIntersectionPoint( frustum[ FRUSTUM_LEFT ], frustum[ FRUSTUM_BOTTOM ], frustum[ FRUSTUM_NEAR ], nearCorners[ 3 ] );

										// draw outer surfaces
										for ( j = 0; j < 4; j++ )
										{
											Vector4Set( quadVerts[ 0 ], nearCorners[ j ][ 0 ], nearCorners[ j ][ 1 ], nearCorners[ j ][ 2 ], 1 );
											Vector4Set( quadVerts[ 1 ], farCorners[ j ][ 0 ], farCorners[ j ][ 1 ], farCorners[ j ][ 2 ], 1 );
											Vector4Set( quadVerts[ 2 ], farCorners[( j + 1 ) % 4 ][ 0 ], farCorners[( j + 1 ) % 4 ][ 1 ], farCorners[( j + 1 ) % 4 ][ 2 ], 1 );
											Vector4Set( quadVerts[ 3 ], nearCorners[( j + 1 ) % 4 ][ 0 ], nearCorners[( j + 1 ) % 4 ][ 1 ], nearCorners[( j + 1 ) % 4 ][ 2 ], 1 );
											Tess_AddQuadStamp2( quadVerts, colorCyan );
										}

										// draw far cap
										Vector4Set( quadVerts[ 0 ], farCorners[ 3 ][ 0 ], farCorners[ 3 ][ 1 ], farCorners[ 3 ][ 2 ], 1 );
										Vector4Set( quadVerts[ 1 ], farCorners[ 2 ][ 0 ], farCorners[ 2 ][ 1 ], farCorners[ 2 ][ 2 ], 1 );
										Vector4Set( quadVerts[ 2 ], farCorners[ 1 ][ 0 ], farCorners[ 1 ][ 1 ], farCorners[ 1 ][ 2 ], 1 );
										Vector4Set( quadVerts[ 3 ], farCorners[ 0 ][ 0 ], farCorners[ 0 ][ 1 ], farCorners[ 0 ][ 2 ], 1 );
										Tess_AddQuadStamp2( quadVerts, colorRed );

										// draw near cap
										Vector4Set( quadVerts[ 0 ], nearCorners[ 0 ][ 0 ], nearCorners[ 0 ][ 1 ], nearCorners[ 0 ][ 2 ], 1 );
										Vector4Set( quadVerts[ 1 ], nearCorners[ 1 ][ 0 ], nearCorners[ 1 ][ 1 ], nearCorners[ 1 ][ 2 ], 1 );
										Vector4Set( quadVerts[ 2 ], nearCorners[ 2 ][ 0 ], nearCorners[ 2 ][ 1 ], nearCorners[ 2 ][ 2 ], 1 );
										Vector4Set( quadVerts[ 3 ], nearCorners[ 3 ][ 0 ], nearCorners[ 3 ][ 1 ], nearCorners[ 3 ][ 2 ], 1 );
										Tess_AddQuadStamp2( quadVerts, colorGreen );
									}
									else
									{
										vec3_t top;

										// no light_start, just use the top vertex (doesn't need to be mirrored)
										PlanesGetIntersectionPoint( frustum[ FRUSTUM_LEFT ], frustum[ FRUSTUM_RIGHT ], frustum[ FRUSTUM_TOP ], top );

										// draw pyramid
										for ( j = 0; j < 4; j++ )
										{
											VectorCopy( top, tess.xyz[ tess.numVertexes ] );
											Vector4Copy( colorCyan, tess.colors[ tess.numVertexes ] );
											tess.indexes[ tess.numIndexes++ ] = tess.numVertexes;
											tess.numVertexes++;

											VectorCopy( farCorners[( j + 1 ) % 4 ], tess.xyz[ tess.numVertexes ] );
											Vector4Copy( colorCyan, tess.colors[ tess.numVertexes ] );
											tess.indexes[ tess.numIndexes++ ] = tess.numVertexes;
											tess.numVertexes++;

											VectorCopy( farCorners[ j ], tess.xyz[ tess.numVertexes ] );
											Vector4Copy( colorCyan, tess.colors[ tess.numVertexes ] );
											tess.indexes[ tess.numIndexes++ ] = tess.numVertexes;
											tess.numVertexes++;
										}

										Vector4Set( quadVerts[ 0 ], farCorners[ 0 ][ 0 ], farCorners[ 0 ][ 1 ], farCorners[ 0 ][ 2 ], 1 );
										Vector4Set( quadVerts[ 1 ], farCorners[ 1 ][ 0 ], farCorners[ 1 ][ 1 ], farCorners[ 1 ][ 2 ], 1 );
										Vector4Set( quadVerts[ 2 ], farCorners[ 2 ][ 0 ], farCorners[ 2 ][ 1 ], farCorners[ 2 ][ 2 ], 1 );
										Vector4Set( quadVerts[ 3 ], farCorners[ 3 ][ 0 ], farCorners[ 3 ][ 1 ], farCorners[ 3 ][ 2 ], 1 );
										Tess_AddQuadStamp2( quadVerts, colorRed );
									}

									Tess_UpdateVBOs( ATTR_POSITION | ATTR_COLOR );
									break;
								}

							default:
								break;
						}
					}
				}

				// last interaction of current light
				lightShader = light->shader;
				attenuationZStage = lightShader->stages[ 0 ];

				for ( j = 1; j < MAX_SHADER_STAGES; j++ )
				{
					attenuationXYStage = lightShader->stages[ j ];

					if ( !attenuationXYStage )
					{
						break;
					}

					if ( attenuationXYStage->type != ST_ATTENUATIONMAP_XY )
					{
						continue;
					}

					if ( !RB_EvalExpression( &attenuationXYStage->ifExp, 1.0 ) )
					{
						continue;
					}

					Tess_ComputeColor( attenuationXYStage );

					if ( VectorLength( tess.svars.color ) < 0.01 )
					{
						// don't render black lights
					}

					R_ComputeFinalAttenuation( attenuationXYStage, light );

#if 0 // FIXME support darkening shadows as in HL2

					if ( light->l.inverseShadows )
					{
						GL_State( GLS_SRCBLEND_ZERO | GLS_DSTBLEND_ONE_MINUS_SRC_COLOR );
					}
					else
#endif
					{
						// set OpenGL state for additive lighting
						GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE );
					}

					if ( light->clipsNearPlane )
					{
						GL_Cull( CT_TWO_SIDED );
					}
					else
					{
						GL_Cull( CT_FRONT_SIDED );
					}

					bool  shadowCompare = ( r_shadows->integer >= SHADOWING_ESM16 && !light->l.noShadows && light->shadowLOD >= 0 );

					float shadowTexelSize = 1.0f;

					if ( shadowCompare )
					{
						shadowTexelSize = 1.0f / shadowMapResolutions[ light->shadowLOD ];
					}

					if ( light->l.rlType == RL_OMNI )
					{
						// choose right shader program ----------------------------------
						gl_deferredLightingShader_omniXYZ->SetPortalClipping( backEnd.viewParms.isPortal );
						gl_deferredLightingShader_omniXYZ->SetNormalMapping( r_normalMapping->integer );
						gl_deferredLightingShader_omniXYZ->SetShadowing( shadowCompare );
						gl_deferredLightingShader_omniXYZ->SetFrustumClipping( light->clipsNearPlane );

						gl_deferredLightingShader_omniXYZ->BindProgram();

						// end choose right shader program ------------------------------

						gl_deferredLightingShader_omniXYZ->SetUniform_ViewOrigin( backEnd.viewParms.orientation.origin );  // in world space
						gl_deferredLightingShader_omniXYZ->SetUniform_LightOrigin( light->origin );
						gl_deferredLightingShader_omniXYZ->SetUniform_LightColor( tess.svars.color );
						gl_deferredLightingShader_omniXYZ->SetUniform_LightRadius( light->sphereRadius );
						gl_deferredLightingShader_omniXYZ->SetUniform_LightScale( light->l.scale );
						gl_deferredLightingShader_omniXYZ->SetUniform_LightAttenuationMatrix( light->attenuationMatrix2 );
						gl_deferredLightingShader_omniXYZ->SetUniform_LightFrustum( lightFrustum );

						gl_deferredLightingShader_omniXYZ->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );
						gl_deferredLightingShader_omniXYZ->SetUniform_UnprojectMatrix( backEnd.viewParms.unprojectionMatrix );

						if ( backEnd.viewParms.isPortal )
						{
							float plane[ 4 ];

							// clipping plane in world space
							plane[ 0 ] = backEnd.viewParms.portalPlane.normal[ 0 ];
							plane[ 1 ] = backEnd.viewParms.portalPlane.normal[ 1 ];
							plane[ 2 ] = backEnd.viewParms.portalPlane.normal[ 2 ];
							plane[ 3 ] = backEnd.viewParms.portalPlane.dist;

							gl_deferredLightingShader_omniXYZ->SetUniform_PortalPlane( plane );
						}

						// bind u_DiffuseMap
						GL_SelectTexture( 0 );
						GL_Bind( tr.deferredDiffuseFBOImage );

						// bind u_NormalMap
						GL_SelectTexture( 1 );
						GL_Bind( tr.deferredNormalFBOImage );

						if ( r_normalMapping->integer )
						{
							// bind u_SpecularMap
							GL_SelectTexture( 2 );
							GL_Bind( tr.deferredSpecularFBOImage );
						}

						// bind u_DepthMap
						GL_SelectTexture( 3 );
						GL_Bind( tr.depthRenderImage );

						// bind u_AttenuationMapXY
						GL_SelectTexture( 4 );
						BindAnimatedImage( &attenuationXYStage->bundle[ TB_COLORMAP ] );

						// bind u_AttenuationMapZ
						GL_SelectTexture( 5 );
						BindAnimatedImage( &attenuationZStage->bundle[ TB_COLORMAP ] );

						// bind u_ShadowMap
						if ( shadowCompare )
						{
							GL_SelectTexture( 6 );
							GL_Bind( tr.shadowCubeFBOImage[ light->shadowLOD ] );
						}

						if ( light->clipsNearPlane )
						{
							// draw lighting with a fullscreen quad
							Tess_InstantQuad( backEnd.viewParms.viewportVerts );
						}
						else
						{
							// draw the volume
							Tess_DrawElements();
						}
					}
					else if ( light->l.rlType == RL_PROJ )
					{
						// choose right shader program ----------------------------------
						gl_deferredLightingShader_projXYZ->SetPortalClipping( backEnd.viewParms.isPortal );
						gl_deferredLightingShader_projXYZ->SetNormalMapping( r_normalMapping->integer );
						gl_deferredLightingShader_projXYZ->SetShadowing( shadowCompare );
						gl_deferredLightingShader_projXYZ->SetFrustumClipping( light->clipsNearPlane );

						gl_deferredLightingShader_projXYZ->BindProgram();

						// end choose right shader program ------------------------------

						gl_deferredLightingShader_projXYZ->SetUniform_ViewOrigin( backEnd.viewParms.orientation.origin );  // in world space
						gl_deferredLightingShader_projXYZ->SetUniform_LightOrigin( light->origin );
						gl_deferredLightingShader_projXYZ->SetUniform_LightColor( tess.svars.color );
						gl_deferredLightingShader_projXYZ->SetUniform_LightRadius( light->sphereRadius );
						gl_deferredLightingShader_projXYZ->SetUniform_LightScale( light->l.scale );
						gl_deferredLightingShader_projXYZ->SetUniform_LightAttenuationMatrix( light->attenuationMatrix2 );
						gl_deferredLightingShader_projXYZ->SetUniform_LightFrustum( lightFrustum );

						if ( shadowCompare )
						{
							gl_deferredLightingShader_projXYZ->SetUniform_ShadowTexelSize( shadowTexelSize );
							gl_deferredLightingShader_projXYZ->SetUniform_ShadowBlur( r_shadowBlur->value );
							gl_deferredLightingShader_projXYZ->SetUniform_ShadowMatrix( light->shadowMatrices );
						}

						gl_deferredLightingShader_projXYZ->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );
						gl_deferredLightingShader_projXYZ->SetUniform_UnprojectMatrix( backEnd.viewParms.unprojectionMatrix );

						if ( backEnd.viewParms.isPortal )
						{
							float plane[ 4 ];

							// clipping plane in world space
							plane[ 0 ] = backEnd.viewParms.portalPlane.normal[ 0 ];
							plane[ 1 ] = backEnd.viewParms.portalPlane.normal[ 1 ];
							plane[ 2 ] = backEnd.viewParms.portalPlane.normal[ 2 ];
							plane[ 3 ] = backEnd.viewParms.portalPlane.dist;

							gl_deferredLightingShader_projXYZ->SetUniform_PortalPlane( plane );
						}

						// bind u_DiffuseMap
						GL_SelectTexture( 0 );
						GL_Bind( tr.deferredDiffuseFBOImage );

						// bind u_NormalMap
						GL_SelectTexture( 1 );
						GL_Bind( tr.deferredNormalFBOImage );

						if ( r_normalMapping->integer )
						{
							// bind u_SpecularMap
							GL_SelectTexture( 2 );
							GL_Bind( tr.deferredSpecularFBOImage );
						}

						// bind u_DepthMap
						GL_SelectTexture( 3 );
						GL_Bind( tr.depthRenderImage );

						// bind u_AttenuationMapXY
						GL_SelectTexture( 4 );
						BindAnimatedImage( &attenuationXYStage->bundle[ TB_COLORMAP ] );

						// bind u_AttenuationMapZ
						GL_SelectTexture( 5 );
						BindAnimatedImage( &attenuationZStage->bundle[ TB_COLORMAP ] );

						// bind u_ShadowMap
						if ( shadowCompare )
						{
							GL_SelectTexture( 6 );
							GL_Bind( tr.shadowMapFBOImage[ light->shadowLOD ] );
						}

						if ( light->clipsNearPlane )
						{
							// draw lighting with a fullscreen quad
							Tess_InstantQuad( backEnd.viewParms.viewportVerts );
						}
						else
						{
							// draw the volume
							Tess_DrawElements();
						}
					}
					else if ( light->l.rlType == RL_DIRECTIONAL )
					{
						shadowCompare = ( r_shadows->integer >= SHADOWING_ESM16 && !light->l.noShadows ); // && light->shadowLOD >= 0);

						// choose right shader program ----------------------------------
						gl_deferredLightingShader_directionalSun->SetPortalClipping( backEnd.viewParms.isPortal );
						gl_deferredLightingShader_directionalSun->SetNormalMapping( r_normalMapping->integer );
						gl_deferredLightingShader_directionalSun->SetShadowing( shadowCompare );
						gl_deferredLightingShader_directionalSun->SetFrustumClipping( light->clipsNearPlane );

						gl_deferredLightingShader_directionalSun->BindProgram();

						// end choose right shader program ------------------------------

						gl_deferredLightingShader_directionalSun->SetUniform_ViewOrigin( backEnd.viewParms.orientation.origin );  // in world space
						gl_deferredLightingShader_directionalSun->SetUniform_LightDir( tr.sunDirection );
						gl_deferredLightingShader_directionalSun->SetUniform_LightColor( tess.svars.color );
						gl_deferredLightingShader_directionalSun->SetUniform_LightRadius( light->sphereRadius );
						gl_deferredLightingShader_directionalSun->SetUniform_LightScale( light->l.scale );
						gl_deferredLightingShader_directionalSun->SetUniform_LightAttenuationMatrix( light->attenuationMatrix2 );
						gl_deferredLightingShader_directionalSun->SetUniform_LightFrustum( lightFrustum );

						if ( shadowCompare )
						{
							gl_deferredLightingShader_directionalSun->SetUniform_ShadowMatrix( light->shadowMatricesBiased );
							gl_deferredLightingShader_directionalSun->SetUniform_ShadowParallelSplitDistances( backEnd.viewParms.parallelSplitDistances );
							gl_deferredLightingShader_directionalSun->SetUniform_ShadowTexelSize( shadowTexelSize );
							gl_deferredLightingShader_directionalSun->SetUniform_ShadowBlur( r_shadowBlur->value );
						}

						gl_deferredLightingShader_directionalSun->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );
						gl_deferredLightingShader_directionalSun->SetUniform_UnprojectMatrix( backEnd.viewParms.unprojectionMatrix );
						gl_deferredLightingShader_directionalSun->SetUniform_ViewMatrix( backEnd.viewParms.world.viewMatrix );

						if ( backEnd.viewParms.isPortal )
						{
							float plane[ 4 ];

							// clipping plane in world space
							plane[ 0 ] = backEnd.viewParms.portalPlane.normal[ 0 ];
							plane[ 1 ] = backEnd.viewParms.portalPlane.normal[ 1 ];
							plane[ 2 ] = backEnd.viewParms.portalPlane.normal[ 2 ];
							plane[ 3 ] = backEnd.viewParms.portalPlane.dist;

							gl_deferredLightingShader_directionalSun->SetUniform_PortalPlane( plane );
						}

						// bind u_DiffuseMap
						GL_SelectTexture( 0 );
						GL_Bind( tr.deferredDiffuseFBOImage );

						// bind u_NormalMap
						GL_SelectTexture( 1 );
						GL_Bind( tr.deferredNormalFBOImage );

						if ( r_normalMapping->integer )
						{
							// bind u_SpecularMap
							GL_SelectTexture( 2 );
							GL_Bind( tr.deferredSpecularFBOImage );
						}

						// bind u_DepthMap
						GL_SelectTexture( 3 );
						GL_Bind( tr.depthRenderImage );

						// bind u_AttenuationMapXY
						//GL_SelectTexture(4);
						//BindAnimatedImage(&attenuationXYStage->bundle[TB_COLORMAP]);

						// bind u_AttenuationMapZ
						//GL_SelectTexture(5);
						//BindAnimatedImage(&attenuationZStage->bundle[TB_COLORMAP]);

						// bind shadow maps
						if ( shadowCompare )
						{
							GL_SelectTexture( 6 );
							GL_Bind( tr.sunShadowMapFBOImage[ 0 ] );

							if ( r_parallelShadowSplits->integer >= 1 )
							{
								GL_SelectTexture( 7 );
								GL_Bind( tr.sunShadowMapFBOImage[ 1 ] );
							}

							if ( r_parallelShadowSplits->integer >= 2 )
							{
								GL_SelectTexture( 8 );
								GL_Bind( tr.sunShadowMapFBOImage[ 2 ] );
							}

							if ( r_parallelShadowSplits->integer >= 3 )
							{
								GL_SelectTexture( 9 );
								GL_Bind( tr.sunShadowMapFBOImage[ 3 ] );
							}

							if ( r_parallelShadowSplits->integer >= 4 )
							{
								GL_SelectTexture( 10 );
								GL_Bind( tr.sunShadowMapFBOImage[ 4 ] );
							}
						}

						if ( light->clipsNearPlane )
						{
							// draw lighting with a fullscreen quad
							Tess_InstantQuad( backEnd.viewParms.viewportVerts );
						}
						else
						{
							// draw the volume
							Tess_DrawElements();
						}
					}
				}

				if ( light->clipsNearPlane )
				{
					GL_PopMatrix();
				}

				// done with light post processing
				// clean up
				tess.multiDrawPrimitives = 0;
				tess.numIndexes = 0;
				tess.numVertexes = 0;

				// draw split frustum shadow maps
				if ( r_showShadowMaps->integer && light->l.rlType == RL_DIRECTIONAL )
				{
					int   frustumIndex;
					float x, y, w, h;

					GL_Viewport( backEnd.viewParms.viewportX, backEnd.viewParms.viewportY,
					             backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight );

					GL_Scissor( backEnd.viewParms.viewportX, backEnd.viewParms.viewportY,
					            backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight );

					// set 2D virtual screen size
					GL_PushMatrix();
					GL_LoadProjectionMatrix( ortho );
					GL_LoadModelViewMatrix( matrixIdentity );

					for ( frustumIndex = 0; frustumIndex <= r_parallelShadowSplits->integer; frustumIndex++ )
					{
						GL_Cull( CT_TWO_SIDED );
						GL_State( GLS_DEPTHTEST_DISABLE );

						gl_debugShadowMapShader->BindProgram();
						gl_debugShadowMapShader->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );

						// bind u_ColorMap
						GL_SelectTexture( 0 );
						GL_Bind( tr.sunShadowMapFBOImage[ frustumIndex ] );

						w = 200;
						h = 200;

						x = 205 * frustumIndex;
						y = 70;

						Vector4Set( quadVerts[ 0 ], x, y, 0, 1 );
						Vector4Set( quadVerts[ 1 ], x + w, y, 0, 1 );
						Vector4Set( quadVerts[ 2 ], x + w, y + h, 0, 1 );
						Vector4Set( quadVerts[ 3 ], x, y + h, 0, 1 );

						Tess_InstantQuad( quadVerts );

						{
							int    j;
							vec4_t splitFrustum[ 6 ];
							vec3_t farCorners[ 4 ];
							vec3_t nearCorners[ 4 ];

							GL_Viewport( x, y, w, h );
							GL_Scissor( x, y, w, h );

							GL_PushMatrix();

							GL_State( GLS_POLYMODE_LINE | GLS_DEPTHTEST_DISABLE );
							GL_Cull( CT_TWO_SIDED );

							gl_genericShader->DisableAlphaTesting();
							gl_genericShader->DisablePortalClipping();
							gl_genericShader->DisableVertexSkinning();
							gl_genericShader->DisableVertexAnimation();
							gl_genericShader->DisableDeformVertexes();
							gl_genericShader->DisableTCGenEnvironment();

							gl_genericShader->BindProgram();

							gl_genericShader->SetUniform_ColorModulate( CGEN_VERTEX, AGEN_VERTEX );
							gl_genericShader->SetUniform_Color( colorBlack );

							gl_genericShader->SetUniform_ModelViewProjectionMatrix( light->shadowMatrices[ frustumIndex ] );

							// bind u_ColorMap
							GL_SelectTexture( 0 );
							GL_Bind( tr.whiteImage );
							gl_genericShader->SetUniform_ColorTextureMatrix( matrixIdentity );

							tess.multiDrawPrimitives = 0;
							tess.numIndexes = 0;
							tess.numVertexes = 0;

#if 1

							for ( j = 0; j < 6; j++ )
							{
								VectorCopy( backEnd.viewParms.frustums[ 1 + frustumIndex ][ j ].normal, splitFrustum[ j ] );
								splitFrustum[ j ][ 3 ] = backEnd.viewParms.frustums[ 1 + frustumIndex ][ j ].dist;
							}

#else

							for ( j = 0; j < 6; j++ )
							{
								VectorCopy( backEnd.viewParms.frustums[ 0 ][ j ].normal, splitFrustum[ j ] );
								splitFrustum[ j ][ 3 ] = backEnd.viewParms.frustums[ 0 ][ j ].dist;
							}

#endif

							// calculate split frustum corner points
							PlanesGetIntersectionPoint( splitFrustum[ FRUSTUM_LEFT ], splitFrustum[ FRUSTUM_TOP ], splitFrustum[ FRUSTUM_NEAR ], nearCorners[ 0 ] );
							PlanesGetIntersectionPoint( splitFrustum[ FRUSTUM_RIGHT ], splitFrustum[ FRUSTUM_TOP ], splitFrustum[ FRUSTUM_NEAR ], nearCorners[ 1 ] );
							PlanesGetIntersectionPoint( splitFrustum[ FRUSTUM_RIGHT ], splitFrustum[ FRUSTUM_BOTTOM ], splitFrustum[ FRUSTUM_NEAR ], nearCorners[ 2 ] );
							PlanesGetIntersectionPoint( splitFrustum[ FRUSTUM_LEFT ], splitFrustum[ FRUSTUM_BOTTOM ], splitFrustum[ FRUSTUM_NEAR ], nearCorners[ 3 ] );

							PlanesGetIntersectionPoint( splitFrustum[ FRUSTUM_LEFT ], splitFrustum[ FRUSTUM_TOP ], splitFrustum[ FRUSTUM_FAR ], farCorners[ 0 ] );
							PlanesGetIntersectionPoint( splitFrustum[ FRUSTUM_RIGHT ], splitFrustum[ FRUSTUM_TOP ], splitFrustum[ FRUSTUM_FAR ], farCorners[ 1 ] );
							PlanesGetIntersectionPoint( splitFrustum[ FRUSTUM_RIGHT ], splitFrustum[ FRUSTUM_BOTTOM ], splitFrustum[ FRUSTUM_FAR ], farCorners[ 2 ] );
							PlanesGetIntersectionPoint( splitFrustum[ FRUSTUM_LEFT ], splitFrustum[ FRUSTUM_BOTTOM ], splitFrustum[ FRUSTUM_FAR ], farCorners[ 3 ] );

							// draw outer surfaces
#if 1

							for ( j = 0; j < 4; j++ )
							{
								Vector4Set( quadVerts[ 0 ], nearCorners[ j ][ 0 ], nearCorners[ j ][ 1 ], nearCorners[ j ][ 2 ], 1 );
								Vector4Set( quadVerts[ 1 ], farCorners[ j ][ 0 ], farCorners[ j ][ 1 ], farCorners[ j ][ 2 ], 1 );
								Vector4Set( quadVerts[ 2 ], farCorners[( j + 1 ) % 4 ][ 0 ], farCorners[( j + 1 ) % 4 ][ 1 ], farCorners[( j + 1 ) % 4 ][ 2 ], 1 );
								Vector4Set( quadVerts[ 3 ], nearCorners[( j + 1 ) % 4 ][ 0 ], nearCorners[( j + 1 ) % 4 ][ 1 ], nearCorners[( j + 1 ) % 4 ][ 2 ], 1 );
								Tess_AddQuadStamp2( quadVerts, colorCyan );
							}

							// draw far cap
							Vector4Set( quadVerts[ 0 ], farCorners[ 3 ][ 0 ], farCorners[ 3 ][ 1 ], farCorners[ 3 ][ 2 ], 1 );
							Vector4Set( quadVerts[ 1 ], farCorners[ 2 ][ 0 ], farCorners[ 2 ][ 1 ], farCorners[ 2 ][ 2 ], 1 );
							Vector4Set( quadVerts[ 2 ], farCorners[ 1 ][ 0 ], farCorners[ 1 ][ 1 ], farCorners[ 1 ][ 2 ], 1 );
							Vector4Set( quadVerts[ 3 ], farCorners[ 0 ][ 0 ], farCorners[ 0 ][ 1 ], farCorners[ 0 ][ 2 ], 1 );
							Tess_AddQuadStamp2( quadVerts, colorBlue );

							// draw near cap
							Vector4Set( quadVerts[ 0 ], nearCorners[ 0 ][ 0 ], nearCorners[ 0 ][ 1 ], nearCorners[ 0 ][ 2 ], 1 );
							Vector4Set( quadVerts[ 1 ], nearCorners[ 1 ][ 0 ], nearCorners[ 1 ][ 1 ], nearCorners[ 1 ][ 2 ], 1 );
							Vector4Set( quadVerts[ 2 ], nearCorners[ 2 ][ 0 ], nearCorners[ 2 ][ 1 ], nearCorners[ 2 ][ 2 ], 1 );
							Vector4Set( quadVerts[ 3 ], nearCorners[ 3 ][ 0 ], nearCorners[ 3 ][ 1 ], nearCorners[ 3 ][ 2 ], 1 );
							Tess_AddQuadStamp2( quadVerts, colorGreen );
#else

							// draw pyramid
							for ( j = 0; j < 4; j++ )
							{
								VectorCopy( farCorners[ j ], tess.xyz[ tess.numVertexes ] );
								Vector4Copy( colorCyan, tess.colors[ tess.numVertexes ] );
								tess.indexes[ tess.numIndexes++ ] = tess.numVertexes;
								tess.numVertexes++;

								VectorCopy( farCorners[( j + 1 ) % 4 ], tess.xyz[ tess.numVertexes ] );
								Vector4Copy( colorCyan, tess.colors[ tess.numVertexes ] );
								tess.indexes[ tess.numIndexes++ ] = tess.numVertexes;
								tess.numVertexes++;

								VectorCopy( backEnd.viewParms.orientation.origin, tess.xyz[ tess.numVertexes ] );
								Vector4Copy( colorCyan, tess.colors[ tess.numVertexes ] );
								tess.indexes[ tess.numIndexes++ ] = tess.numVertexes;
								tess.numVertexes++;
							}

							Vector4Set( quadVerts[ 0 ], farCorners[ 3 ][ 0 ], farCorners[ 3 ][ 1 ], farCorners[ 3 ][ 2 ], 1 );
							Vector4Set( quadVerts[ 1 ], farCorners[ 2 ][ 0 ], farCorners[ 2 ][ 1 ], farCorners[ 2 ][ 2 ], 1 );
							Vector4Set( quadVerts[ 2 ], farCorners[ 1 ][ 0 ], farCorners[ 1 ][ 1 ], farCorners[ 1 ][ 2 ], 1 );
							Vector4Set( quadVerts[ 3 ], farCorners[ 0 ][ 0 ], farCorners[ 0 ][ 1 ], farCorners[ 0 ][ 2 ], 1 );
							Tess_AddQuadStamp2( quadVerts, colorRed );
#endif

							Tess_UpdateVBOs( ATTR_POSITION | ATTR_COLOR );
							Tess_DrawElements();

							// draw light volume
							if ( light->isStatic && light->frustumVBO && light->frustumIBO )
							{
								gl_genericShader->SetUniform_ColorModulate( CGEN_CUSTOM_RGB, AGEN_CUSTOM );
								gl_genericShader->SetUniform_Color( colorYellow );

								R_BindVBO( light->frustumVBO );
								R_BindIBO( light->frustumIBO );

								GL_VertexAttribsState( ATTR_POSITION );

								tess.numVertexes = light->frustumVerts;
								tess.numIndexes = light->frustumIndexes;

								Tess_DrawElements();
							}

							tess.multiDrawPrimitives = 0;
							tess.numIndexes = 0;
							tess.numVertexes = 0;

							GL_PopMatrix();

							GL_Viewport( backEnd.viewParms.viewportX, backEnd.viewParms.viewportY,
							             backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight );

							GL_Scissor( backEnd.viewParms.viewportX, backEnd.viewParms.viewportY,
							            backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight );
						}
					} // end for

					// back to 3D or whatever
					GL_PopMatrix();
				}
			}
		} // end if(iaCount == iaFirst)

		if ( drawShadows )
		{
			if ( entity->e.renderfx & ( RF_NOSHADOW | RF_DEPTHHACK ) )
			{
				goto skipInteraction;
			}

			if ( shader->isSky )
			{
				goto skipInteraction;
			}

			if ( shader->sort > SS_OPAQUE )
			{
				goto skipInteraction;
			}

			if ( shader->noShadows || light->l.noShadows || light->shadowLOD < 0 )
			{
				goto skipInteraction;
			}

			if ( light->l.inverseShadows && ( entity == &tr.worldEntity ) )
			{
				// this light only casts shadows by its player and their items
				goto skipInteraction;
			}

			if ( ia->type == IA_LIGHTONLY )
			{
				goto skipInteraction;
			}

			if ( light->l.rlType == RL_OMNI && !( ia->cubeSideBits & ( 1 << cubeSide ) ) )
			{
				goto skipInteraction;
			}

			switch ( light->l.rlType )
			{
				case RL_OMNI:
				case RL_PROJ:
				case RL_DIRECTIONAL:
					{
#if 1

						if ( ( iaCount != iaFirst ) &&
						     light == oldLight &&
						     entity == oldEntity &&
						     ( alphaTest ? shader == oldShader : alphaTest == oldAlphaTest ) &&
						     ( deformType == oldDeformType ) )
						{
							if ( r_logFile->integer )
							{
								// don't just call LogComment, or we will get
								// a call to va() every frame!
								GLimp_LogComment( va( "----- Batching Shadow Interaction: %i -----\n", iaCount ) );
							}

							// fast path, same as previous
							rb_surfaceTable[ *surface ]( surface );
							goto nextInteraction;
						}
						else
#endif
						{
							if ( oldLight )
							{
								// draw the contents of the last shader batch
								Tess_End();
							}

							if ( r_logFile->integer )
							{
								// don't just call LogComment, or we will get
								// a call to va() every frame!
								GLimp_LogComment( va( "----- Beginning Shadow Interaction: %i -----\n", iaCount ) );
							}

							// we don't need tangent space calculations here
							Tess_Begin( Tess_StageIteratorShadowFill, NULL, shader, light->shader, qtrue, qfalse, -1, 0 );
						}

						break;
					}

				default:
					break;
			}
		}
		else
		{
			// jump to !ia->next
			goto nextInteraction;
		}

		// change the modelview matrix if needed
		if ( entity != oldEntity )
		{
			depthRange = qfalse;

			if ( entity != &tr.worldEntity )
			{
				// set up the transformation matrix
				if ( drawShadows )
				{
					R_RotateEntityForLight( entity, light, &backEnd.orientation );
				}
				else
				{
					R_RotateEntityForViewParms( entity, &backEnd.viewParms, &backEnd.orientation );
				}

				if ( entity->e.renderfx & RF_DEPTHHACK )
				{
					// hack the depth range to prevent view model from poking into walls
					depthRange = qtrue;
				}
			}
			else
			{
				// set up the transformation matrix
				if ( drawShadows )
				{
					Com_Memset( &backEnd.orientation, 0, sizeof( backEnd.orientation ) );

					backEnd.orientation.axis[ 0 ][ 0 ] = 1;
					backEnd.orientation.axis[ 1 ][ 1 ] = 1;
					backEnd.orientation.axis[ 2 ][ 2 ] = 1;
					VectorCopy( light->l.origin, backEnd.orientation.viewOrigin );

					MatrixIdentity( backEnd.orientation.transformMatrix );
					//MatrixAffineInverse(backEnd.orientation.transformMatrix, backEnd.orientation.viewMatrix);
					MatrixMultiply( light->viewMatrix, backEnd.orientation.transformMatrix, backEnd.orientation.viewMatrix );
					MatrixCopy( backEnd.orientation.viewMatrix, backEnd.orientation.modelViewMatrix );
				}
				else
				{
					// transform by the camera placement
					backEnd.orientation = backEnd.viewParms.world;
				}
			}

			GL_LoadModelViewMatrix( backEnd.orientation.modelViewMatrix );

			// change depthrange if needed
			if ( oldDepthRange != depthRange )
			{
				if ( depthRange )
				{
					glDepthRange( 0, 0.3 );
				}
				else
				{
					glDepthRange( 0, 1 );
				}

				oldDepthRange = depthRange;
			}
		}

		if ( drawShadows )
		{
			switch ( light->l.rlType )
			{
				case RL_OMNI:
				case RL_PROJ:
				case RL_DIRECTIONAL:
					{
						// add the triangles for this surface
						rb_surfaceTable[ *surface ]( surface );
						break;
					}

				default:
					break;
			}
		}
		else
		{
			// DO NOTHING
			//rb_surfaceTable[*surface] (surface, ia->numLightIndexes, ia->lightIndexes, 0, NULL);
		}

nextInteraction:

		// remember values
		oldLight = light;
		oldEntity = entity;
		oldShader = shader;
		oldAlphaTest = alphaTest;
		oldDeformType = deformType;

skipInteraction:

		if ( !ia->next )
		{
			// if ia->next does not point to any other interaction then
			// this is the last interaction of the current light

			if ( r_logFile->integer )
			{
				// don't just call LogComment, or we will get
				// a call to va() every frame!
				GLimp_LogComment( va( "----- Last Interaction: %i -----\n", iaCount ) );
			}

			if ( drawShadows )
			{
				// draw the contents of the last shader batch
				Tess_End();

				switch ( light->l.rlType )
				{
					case RL_OMNI:
						{
							if ( cubeSide == 5 )
							{
								cubeSide = 0;
								drawShadows = qfalse;
							}
							else
							{
								cubeSide++;
							}

							// jump back to first interaction of this light
							ia = &backEnd.viewParms.interactions[ iaFirst ];
							iaCount = iaFirst;
							break;
						}

					case RL_PROJ:
						{
							// jump back to first interaction of this light and start lighting
							ia = &backEnd.viewParms.interactions[ iaFirst ];
							iaCount = iaFirst;
							drawShadows = qfalse;
							break;
						}

					case RL_DIRECTIONAL:
						{
							// set shadow matrix including scale + offset
							MatrixCopy( bias, light->shadowMatricesBiased[ splitFrustumIndex ] );
							MatrixMultiply2( light->shadowMatricesBiased[ splitFrustumIndex ], light->projectionMatrix );
							MatrixMultiply2( light->shadowMatricesBiased[ splitFrustumIndex ], light->viewMatrix );

							MatrixMultiply( light->projectionMatrix, light->viewMatrix, light->shadowMatrices[ splitFrustumIndex ] );

							if ( r_parallelShadowSplits->integer )
							{
								if ( splitFrustumIndex == r_parallelShadowSplits->integer )
								{
									splitFrustumIndex = 0;
									drawShadows = qfalse;
								}
								else
								{
									splitFrustumIndex++;
								}

								// jump back to first interaction of this light
								ia = &backEnd.viewParms.interactions[ iaFirst ];
								iaCount = iaFirst;
							}
							else
							{
								// jump back to first interaction of this light and start lighting
								ia = &backEnd.viewParms.interactions[ iaFirst ];
								iaCount = iaFirst;
								drawShadows = qfalse;
							}

							break;
						}

					default:
						break;
				}
			}
			else
			{
#ifdef VOLUMETRIC_LIGHTING

				// draw the light volume if needed
				if ( light->shader->volumetricLight )
				{
					Render_lightVolume( ia );
				}

#endif

				if ( iaCount < ( backEnd.viewParms.numInteractions - 1 ) )
				{
					// jump to next interaction and start shadowing
					ia++;
					iaCount++;
					iaFirst = iaCount;
					drawShadows = qtrue;
				}
				else
				{
					// increase last time to leave for loop
					iaCount++;
				}
			}

			// force updates
			oldLight = NULL;
			oldEntity = NULL;
			oldShader = NULL;
		}
		else
		{
			// just continue
			ia = ia->next;
			iaCount++;
		}
	}

	// go back to the world modelview matrix
	GL_LoadModelViewMatrix( backEnd.viewParms.world.modelViewMatrix );

	if ( depthRange )
	{
		glDepthRange( 0, 1 );
	}

	// reset scissor clamping
	GL_Scissor( backEnd.viewParms.viewportX, backEnd.viewParms.viewportY,
	            backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight );

	// reset clear color
	GL_ClearColor( 0.0f, 0.0f, 0.0f, 1.0f );

	GL_CheckErrors();

	if ( r_speeds->integer == RSPEEDS_SHADING_TIMES )
	{
		glFinish();
		endTime = ri.Milliseconds();
		backEnd.pc.c_deferredLightingTime = endTime - startTime;
	}
}

#ifdef EXPERIMENTAL
void RB_RenderScreenSpaceAmbientOcclusion( qboolean deferred )
{
#if 0
//  int             i;
//  vec3_t          viewOrigin;
//  static vec3_t   jitter[32];
//  static qboolean jitterInit = qfalse;
//  matrix_t        projectMatrix;
	matrix_t ortho;

	GLimp_LogComment( "--- RB_RenderScreenSpaceAmbientOcclusion ---\n" );

	if ( backEnd.refdef.rdflags & RDF_NOWORLDMODEL )
	{
		return;
	}

	if ( !r_screenSpaceAmbientOcclusion->integer )
	{
		return;
	}

	// enable shader, set arrays
	gl_screenSpaceAmbientOcclusionShader->BindProgram();

	GL_State( GLS_DEPTHTEST_DISABLE );  // | GLS_DEPTHMASK_TRUE);
	GL_Cull( CT_TWO_SIDED );

	glVertexAttrib4fv( ATTR_INDEX_COLOR, colorWhite );

	// set uniforms

	/*
	   VectorCopy(backEnd.viewParms.orientation.origin, viewOrigin); // in world space

	   if(!jitterInit)
	   {
	   for(i = 0; i < 32; i++)
	   {
	   float *jit = &jitter[i][0];

	   float rad = crandom() * 1024.0f; // FIXME radius;
	   float a = crandom() * M_PI * 2;
	   float b = crandom() * M_PI * 2;

	   jit[0] = rad * sin(a) * cos(b);
	   jit[1] = rad * sin(a) * sin(b);
	   jit[2] = rad * cos(a);
	   }

	   jitterInit = qtrue;
	   }


	   MatrixCopy(backEnd.viewParms.projectionMatrix, projectMatrix);
	   MatrixInverse(projectMatrix);

	   glUniform3f(tr.screenSpaceAmbientOcclusionShader.u_ViewOrigin, viewOrigin[0], viewOrigin[1], viewOrigin[2]);
	   glUniform3fv(tr.screenSpaceAmbientOcclusionShader.u_SSAOJitter, 32, &jitter[0][0]);
	   glUniform1f(tr.screenSpaceAmbientOcclusionShader.u_SSAORadius, r_screenSpaceAmbientOcclusionRadius->value);

	   glUniformMatrix4fv(tr.screenSpaceAmbientOcclusionShader.u_UnprojectMatrix, 1, GL_FALSE, backEnd.viewParms.unprojectionMatrix);
	   glUniformMatrix4fv(tr.screenSpaceAmbientOcclusionShader.u_ProjectMatrix, 1, GL_FALSE, projectMatrix);
	 */

	// capture current color buffer for u_CurrentMap
	GL_SelectTexture( 0 );
	GL_Bind( tr.currentRenderImage );
	glCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, 0, tr.currentRenderImage->uploadWidth, tr.currentRenderImage->uploadHeight );

	// bind u_DepthMap
	GL_SelectTexture( 1 );

	if ( deferred )
	{
		GL_Bind( tr.deferredPositionFBOImage );
	}
	else
	{
		GL_Bind( tr.depthRenderImage );
		glCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, 0, tr.depthRenderImage->uploadWidth, tr.depthRenderImage->uploadHeight );
	}

	// set 2D virtual screen size
	GL_PushMatrix();
	MatrixOrthogonalProjection( ortho, backEnd.viewParms.viewportX,
	                            backEnd.viewParms.viewportX + backEnd.viewParms.viewportWidth,
	                            backEnd.viewParms.viewportY, backEnd.viewParms.viewportY + backEnd.viewParms.viewportHeight,
	                            -99999, 99999 );
	GL_LoadProjectionMatrix( ortho );
	GL_LoadModelViewMatrix( matrixIdentity );

	gl_screenSpaceAmbientOcclusionShader->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );
	// draw viewport
	Tess_InstantQuad( backEnd.viewParms.viewportVerts );

	// go back to 3D
	GL_PopMatrix();

	GL_CheckErrors();
#endif
}

#endif
#ifdef EXPERIMENTAL
void RB_RenderDepthOfField()
{
	matrix_t ortho;

	GLimp_LogComment( "--- RB_RenderDepthOfField ---\n" );

	if ( backEnd.refdef.rdflags & RDF_NOWORLDMODEL )
	{
		return;
	}

	if ( !r_depthOfField->integer )
	{
		return;
	}

	// enable shader, set arrays
	gl_depthOfFieldShader->BindProgram();

	GL_State( GLS_DEPTHTEST_DISABLE );  // | GLS_DEPTHMASK_TRUE);
	GL_Cull( CT_TWO_SIDED );

	glVertexAttrib4fv( ATTR_INDEX_COLOR, colorWhite );

	// set uniforms

	// capture current color buffer for u_CurrentMap
	GL_SelectTexture( 0 );

	if ( r_deferredShading->integer && glConfig2.framebufferObjectAvailable && glConfig2.textureFloatAvailable &&
	     glConfig2.drawBuffersAvailable && glConfig2.maxDrawBuffers >= 4 )
	{
		GL_Bind( tr.deferredRenderFBOImage );
	}
	else if ( r_hdrRendering->integer && glConfig2.framebufferObjectAvailable && glConfig2.textureFloatAvailable )
	{
		GL_Bind( tr.deferredRenderFBOImage );
	}
	else
	{
		GL_Bind( tr.currentRenderImage );
		glCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, 0, tr.currentRenderImage->uploadWidth, tr.currentRenderImage->uploadHeight );
	}

	// bind u_DepthMap
	GL_SelectTexture( 1 );

	if ( r_deferredShading->integer && glConfig2.framebufferObjectAvailable && glConfig2.textureFloatAvailable &&
	     glConfig2.drawBuffersAvailable && glConfig2.maxDrawBuffers >= 4 )
	{
		GL_Bind( tr.depthRenderImage );
	}
	else if ( r_hdrRendering->integer && glConfig2.framebufferObjectAvailable && glConfig2.textureFloatAvailable )
	{
		GL_Bind( tr.depthRenderImage );
	}
	else
	{
		// depth texture is not bound to a FBO
		GL_Bind( tr.depthRenderImage );
		glCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, 0, tr.depthRenderImage->uploadWidth, tr.depthRenderImage->uploadHeight );
	}

	// set 2D virtual screen size
	GL_PushMatrix();
	MatrixOrthogonalProjection( ortho, backEnd.viewParms.viewportX,
	                            backEnd.viewParms.viewportX + backEnd.viewParms.viewportWidth,
	                            backEnd.viewParms.viewportY, backEnd.viewParms.viewportY + backEnd.viewParms.viewportHeight,
	                            -99999, 99999 );
	GL_LoadProjectionMatrix( ortho );
	GL_LoadModelViewMatrix( matrixIdentity );

	gl_depthOfFieldShader->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );

	// draw viewport
	Tess_InstantQuad( backEnd.viewParms.viewportVerts );

	// go back to 3D
	GL_PopMatrix();

	GL_CheckErrors();
}

#endif

void RB_RenderGlobalFog()
{
	vec3_t   local;
	vec4_t   fogDistanceVector; //, fogDepthVector; //unused, uninitialized use
//	vec4_t          fogColor; //unused, uninitialized use
	matrix_t ortho;

	GLimp_LogComment( "--- RB_RenderGlobalFog ---\n" );

	if ( backEnd.refdef.rdflags & RDF_NOWORLDMODEL )
	{
		return;
	}

	if ( r_noFog->integer )
	{
		return;
	}

#if defined( COMPAT_ET )

	if ( !tr.world || tr.world->globalFog < 0 )
	{
		return;
	}

#else

	if ( r_forceFog->value <= 0 && VectorLength( tr.fogColor ) <= 0 )
	{
		return;
	}

	if ( r_forceFog->value <= 0 && tr.fogDensity <= 0 )
	{
		return;
	}

#endif

	GL_Cull( CT_TWO_SIDED );

	gl_fogGlobalShader->BindProgram();

	// go back to the world modelview matrix
	backEnd.orientation = backEnd.viewParms.world;

	gl_fogGlobalShader->SetUniform_ViewOrigin( backEnd.viewParms.orientation.origin );  // world space

#if defined( COMPAT_ET )
	{
		fog_t *fog;

		fog = &tr.world->fogs[ tr.world->globalFog ];

		if ( r_logFile->integer )
		{
			GLimp_LogComment( va( "--- RB_RenderGlobalFog( fogNum = %i, originalBrushNumber = %i ) ---\n", tr.world->globalFog, fog->originalBrushNumber ) );
		}

		GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );

		// all fogging distance is based on world Z units
		VectorSubtract( backEnd.orientation.origin, backEnd.viewParms.orientation.origin, local );
		fogDistanceVector[ 0 ] = -backEnd.orientation.modelViewMatrix[ 2 ];
		fogDistanceVector[ 1 ] = -backEnd.orientation.modelViewMatrix[ 6 ];
		fogDistanceVector[ 2 ] = -backEnd.orientation.modelViewMatrix[ 10 ];
		fogDistanceVector[ 3 ] = DotProduct( local, backEnd.viewParms.orientation.axis[ 0 ] );

		// scale the fog vectors based on the fog's thickness
		fogDistanceVector[ 0 ] *= fog->tcScale;
		fogDistanceVector[ 1 ] *= fog->tcScale;
		fogDistanceVector[ 2 ] *= fog->tcScale;
		fogDistanceVector[ 3 ] *= fog->tcScale;

		gl_fogGlobalShader->SetUniform_FogDistanceVector( fogDistanceVector );
		gl_fogGlobalShader->SetUniform_Color( fog->color );
	}
#else
	GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA | GLS_DSTBLEND_SRC_ALPHA );

	if ( r_forceFog->value )
	{
		Vector4Set( fogDepthVector, r_forceFog->value, 0, 0, 0 );
		VectorCopy( colorMdGrey, fogColor );
	}
	else
	{
		Vector4Set( fogDepthVector, tr.fogDensity, 0, 0, 0 );
		VectorCopy( tr.fogColor, fogColor );
	}

	gl_fogGlobalShader->SetUniform_FogDepthVector( fogDepthVector );
	gl_fogGlobalShader->SetUniform_Color( fogColor );
#endif

	gl_fogGlobalShader->SetUniform_ViewMatrix( backEnd.viewParms.world.viewMatrix );
	gl_fogGlobalShader->SetUniform_UnprojectMatrix( backEnd.viewParms.unprojectionMatrix );

	// bind u_ColorMap
	GL_SelectTexture( 0 );
	GL_Bind( tr.fogImage );

	// bind u_DepthMap
	GL_SelectTexture( 1 );

	if ( DS_STANDARD_ENABLED() || HDR_ENABLED() )
	{
		GL_Bind( tr.depthRenderImage );
	}
	else
	{
		// depth texture is not bound to a FBO
		GL_Bind( tr.depthRenderImage );
		glCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, 0, tr.depthRenderImage->uploadWidth, tr.depthRenderImage->uploadHeight );
	}

	// set 2D virtual screen size
	GL_PushMatrix();
	MatrixOrthogonalProjection( ortho, backEnd.viewParms.viewportX,
	                            backEnd.viewParms.viewportX + backEnd.viewParms.viewportWidth,
	                            backEnd.viewParms.viewportY, backEnd.viewParms.viewportY + backEnd.viewParms.viewportHeight,
	                            -99999, 99999 );
	GL_LoadProjectionMatrix( ortho );
	GL_LoadModelViewMatrix( matrixIdentity );

	gl_fogGlobalShader->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );

	// draw viewport
	Tess_InstantQuad( backEnd.viewParms.viewportVerts );

	// go back to 3D
	GL_PopMatrix();

	GL_CheckErrors();
}

void RB_RenderBloom()
{
	int      i, j;
	matrix_t ortho;

	GLimp_LogComment( "--- RB_RenderBloom ---\n" );

	if ( ( backEnd.refdef.rdflags & ( RDF_NOWORLDMODEL | RDF_NOBLOOM ) ) || !r_bloom->integer || backEnd.viewParms.isPortal || !glConfig2.framebufferObjectAvailable )
	{
		return;
	}

	// set 2D virtual screen size
	GL_PushMatrix();
	MatrixOrthogonalProjection( ortho, backEnd.viewParms.viewportX,
	                            backEnd.viewParms.viewportX + backEnd.viewParms.viewportWidth,
	                            backEnd.viewParms.viewportY, backEnd.viewParms.viewportY + backEnd.viewParms.viewportHeight,
	                            -99999, 99999 );
	GL_LoadProjectionMatrix( ortho );
	GL_LoadModelViewMatrix( matrixIdentity );

	// FIXME
	//if(glConfig.hardwareType != GLHW_ATI && glConfig.hardwareType != GLHW_ATI_DX10)
	{
		GL_State( GLS_DEPTHTEST_DISABLE );
		GL_Cull( CT_TWO_SIDED );

		GL_PushMatrix();
		GL_LoadModelViewMatrix( matrixIdentity );

#if 1
		MatrixOrthogonalProjection( ortho, 0, tr.contrastRenderFBO->width, 0, tr.contrastRenderFBO->height, -99999, 99999 );
		GL_LoadProjectionMatrix( ortho );
#endif

		if ( DS_STANDARD_ENABLED() )
		{
			if ( HDR_ENABLED() )
			{
				gl_toneMappingShader->EnableMacro_BRIGHTPASS_FILTER();
				gl_toneMappingShader->BindProgram();

				gl_toneMappingShader->SetUniform_HDRKey( backEnd.hdrKey );
				gl_toneMappingShader->SetUniform_HDRAverageLuminance( backEnd.hdrAverageLuminance );
				gl_toneMappingShader->SetUniform_HDRMaxLuminance( backEnd.hdrMaxLuminance );

				gl_toneMappingShader->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );
			}
			else
			{
				gl_contrastShader->BindProgram();

				gl_contrastShader->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );
			}

			GL_SelectTexture( 0 );
			GL_Bind( tr.downScaleFBOImage_quarter );
		}
		else if ( HDR_ENABLED() )
		{
			gl_toneMappingShader->EnableMacro_BRIGHTPASS_FILTER();
			gl_toneMappingShader->BindProgram();

			gl_toneMappingShader->SetUniform_HDRKey( backEnd.hdrKey );
			gl_toneMappingShader->SetUniform_HDRAverageLuminance( backEnd.hdrAverageLuminance );
			gl_toneMappingShader->SetUniform_HDRMaxLuminance( backEnd.hdrMaxLuminance );

			gl_toneMappingShader->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );

			GL_SelectTexture( 0 );
			GL_Bind( tr.downScaleFBOImage_quarter );
		}
		else
		{
			// render contrast downscaled to 1/4th of the screen
			gl_contrastShader->BindProgram();

			gl_contrastShader->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );

			GL_SelectTexture( 0 );
			//GL_Bind(tr.downScaleFBOImage_quarter);
			GL_Bind( tr.currentRenderImage );
			glCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, 0, tr.currentRenderImage->uploadWidth,
			                     tr.currentRenderImage->uploadHeight );
		}

		GL_PopMatrix(); // special 1/4th of the screen contrastRenderFBO ortho

		R_BindFBO( tr.contrastRenderFBO );
		GL_ClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
		glClear( GL_COLOR_BUFFER_BIT );

		// draw viewport
		Tess_InstantQuad( backEnd.viewParms.viewportVerts );

		// render bloom in multiple passes
		for ( i = 0; i < 2; i++ )
		{
			for ( j = 0; j < r_bloomPasses->integer; j++ )
			{
				R_BindFBO( tr.bloomRenderFBO[( j + 1 ) % 2 ] );

				GL_ClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
				glClear( GL_COLOR_BUFFER_BIT );

				GL_State( GLS_DEPTHTEST_DISABLE );

				GL_SelectTexture( 0 );

				if ( j == 0 )
				{
					GL_Bind( tr.contrastRenderFBOImage );
				}
				else
				{
					GL_Bind( tr.bloomRenderFBOImage[ j % 2 ] );
				}

				GL_PushMatrix();
				GL_LoadModelViewMatrix( matrixIdentity );

				MatrixOrthogonalProjection( ortho, 0, tr.bloomRenderFBO[ 0 ]->width, 0, tr.bloomRenderFBO[ 0 ]->height, -99999, 99999 );
				GL_LoadProjectionMatrix( ortho );

				if ( i == 0 )
				{
					gl_blurXShader->BindProgram();

					gl_blurXShader->SetUniform_DeformMagnitude( r_bloomBlur->value );
					gl_blurXShader->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );
				}
				else
				{
					gl_blurYShader->BindProgram();

					gl_blurYShader->SetUniform_DeformMagnitude( r_bloomBlur->value );
					gl_blurYShader->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );
				}

				GL_PopMatrix();

				Tess_InstantQuad( backEnd.viewParms.viewportVerts );
			}

			// add offscreen processed bloom to screen
			if ( DS_STANDARD_ENABLED() )
			{
				R_BindFBO( tr.geometricRenderFBO );
				glDrawBuffers( 1, geometricRenderTargets );

				gl_screenShader->BindProgram();
				GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE );
				glVertexAttrib4fv( ATTR_INDEX_COLOR, colorWhite );

				gl_screenShader->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );

				GL_SelectTexture( 0 );
				GL_Bind( tr.bloomRenderFBOImage[ j % 2 ] );
			}
			else if ( HDR_ENABLED() )
			{
				R_BindFBO( tr.deferredRenderFBO );

				gl_screenShader->BindProgram();
				GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE );
				glVertexAttrib4fv( ATTR_INDEX_COLOR, colorWhite );

				gl_screenShader->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );

				GL_SelectTexture( 0 );
				GL_Bind( tr.bloomRenderFBOImage[ j % 2 ] );
				//GL_Bind(tr.contrastRenderFBOImage);
			}
			else
			{
				R_BindNullFBO();

				gl_screenShader->BindProgram();
				GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE );
				glVertexAttrib4fv( ATTR_INDEX_COLOR, colorWhite );

				gl_screenShader->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );

				GL_SelectTexture( 0 );
				GL_Bind( tr.bloomRenderFBOImage[ j % 2 ] );
				//GL_Bind(tr.contrastRenderFBOImage);
			}

			Tess_InstantQuad( backEnd.viewParms.viewportVerts );
		}
	}

	// go back to 3D
	GL_PopMatrix();

	GL_CheckErrors();
}

void RB_RenderRotoscope( void )
{
#if 0 //!defined(GLSL_COMPILE_STARTUP_ONLY)
	matrix_t ortho;

	GLimp_LogComment( "--- RB_CameraPostFX ---\n" );

	if ( ( backEnd.refdef.rdflags & RDF_NOWORLDMODEL ) || !r_rotoscope->integer || backEnd.viewParms.isPortal )
	{
		return;
	}

	// set 2D virtual screen size
	GL_PushMatrix();
	MatrixOrthogonalProjection( ortho, backEnd.viewParms.viewportX,
	                            backEnd.viewParms.viewportX + backEnd.viewParms.viewportWidth,
	                            backEnd.viewParms.viewportY, backEnd.viewParms.viewportY + backEnd.viewParms.viewportHeight,
	                            -99999, 99999 );
	GL_LoadProjectionMatrix( ortho );
	GL_LoadModelViewMatrix( matrixIdentity );

	GL_State( GLS_DEPTHTEST_DISABLE );
	GL_Cull( CT_TWO_SIDED );

	// enable shader, set arrays
	GL_BindProgram( &tr.rotoscopeShader );

	GLSL_SetUniform_ModelViewProjectionMatrix( &tr.rotoscopeShader, glState.modelViewProjectionMatrix[ glState.stackIndex ] );
	glUniform1f( tr.rotoscopeShader.u_BlurMagnitude, r_bloomBlur->value );

	GL_SelectTexture( 0 );
	GL_Bind( tr.currentRenderImage );
	glCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, 0, tr.currentRenderImage->uploadWidth, tr.currentRenderImage->uploadHeight );

	// draw viewport
	Tess_InstantQuad( backEnd.viewParms.viewportVerts );

	// go back to 3D
	GL_PopMatrix();

	GL_CheckErrors();
#endif
}

void RB_CameraPostFX( void )
{
	matrix_t ortho;
	matrix_t grain;

	GLimp_LogComment( "--- RB_CameraPostFX ---\n" );

	if ( ( backEnd.refdef.rdflags & RDF_NOWORLDMODEL ) || !r_cameraPostFX->integer || backEnd.viewParms.isPortal ||
	     !tr.grainImage || !tr.vignetteImage )
	{
		return;
	}

	// set 2D virtual screen size
	GL_PushMatrix();
	MatrixOrthogonalProjection( ortho, backEnd.viewParms.viewportX,
	                            backEnd.viewParms.viewportX + backEnd.viewParms.viewportWidth,
	                            backEnd.viewParms.viewportY, backEnd.viewParms.viewportY + backEnd.viewParms.viewportHeight,
	                            -99999, 99999 );
	GL_LoadProjectionMatrix( ortho );
	GL_LoadModelViewMatrix( matrixIdentity );

	GL_State( GLS_DEPTHTEST_DISABLE );
	GL_Cull( CT_TWO_SIDED );

	// enable shader, set arrays
	gl_cameraEffectsShader->BindProgram();

	gl_cameraEffectsShader->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );
	//glUniform1f(tr.cameraEffectsShader.u_BlurMagnitude, r_bloomBlur->value);

	MatrixIdentity( grain );

	MatrixMultiplyScale( grain, r_cameraFilmGrainScale->value, r_cameraFilmGrainScale->value, 0 );
	MatrixMultiplyTranslation( grain, backEnd.refdef.floatTime * 10, backEnd.refdef.floatTime * 10, 0 );

	MatrixMultiplyTranslation( grain, 0.5, 0.5, 0.0 );
	MatrixMultiplyZRotation( grain, backEnd.refdef.floatTime * ( random() * 7 ) );
	MatrixMultiplyTranslation( grain, -0.5, -0.5, 0.0 );

	gl_cameraEffectsShader->SetUniform_ColorTextureMatrix( grain );

	// bind u_CurrentMap
	GL_SelectTexture( 0 );
	GL_Bind( tr.occlusionRenderFBOImage );

	/*
	if(glConfig.framebufferObjectAvailable && glConfig.textureFloatAvailable)
	{
	        // copy depth of the main context to deferredRenderFBO
	        glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, 0);
	        glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, tr.occlusionRenderFBO->frameBuffer);
	        glBlitFramebufferEXT(0, 0, glConfig.vidWidth, glConfig.vidHeight,
	                                                   0, 0, glConfig.vidWidth, glConfig.vidHeight,
	                                                   GL_COLOR_BUFFER_BIT,
	                                                   GL_NEAREST);
	}
	else
	*/
	{
		glCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, 0, tr.occlusionRenderFBOImage->uploadWidth, tr.occlusionRenderFBOImage->uploadHeight );
	}

	// bind u_GrainMap
	GL_SelectTexture( 1 );
	GL_Bind( tr.grainImage );
	//GL_Bind(tr.defaultImage);

	// bind u_VignetteMap
	GL_SelectTexture( 2 );

	if ( r_cameraVignette->integer )
	{
		GL_Bind( tr.vignetteImage );
	}
	else
	{
		GL_Bind( tr.whiteImage );
	}

	// draw viewport
	Tess_InstantQuad( backEnd.viewParms.viewportVerts );

	// go back to 3D
	GL_PopMatrix();

	GL_CheckErrors();
}

static void RB_CalculateAdaptation()
{
	int          i;
	static float image[ 64 * 64 * 4 ];
	float        curTime;
	float        deltaTime;
	float        luminance;
	float        avgLuminance;
	float        maxLuminance;
	double       sum;
	const vec3_t LUMINANCE_VECTOR = { 0.2125f, 0.7154f, 0.0721f };
	vec4_t       color;
	float        newAdaptation;
	float        newMaximum;

	curTime = ri.Milliseconds() / 1000.0f;

	// calculate the average scene luminance
	R_BindFBO( tr.downScaleFBO_64x64 );

	// read back the contents
//	glFinish();
	glReadPixels( 0, 0, 64, 64, GL_RGBA, GL_FLOAT, image );

	sum = 0.0f;
	maxLuminance = 0.0f;

	for ( i = 0; i < ( 64 * 64 * 4 ); i += 4 )
	{
		color[ 0 ] = image[ i + 0 ];
		color[ 1 ] = image[ i + 1 ];
		color[ 2 ] = image[ i + 2 ];
		color[ 3 ] = image[ i + 3 ];

		luminance = DotProduct( color, LUMINANCE_VECTOR ) + 0.0001f;

		if ( luminance > maxLuminance )
		{
			maxLuminance = luminance;
		}

		sum += log( luminance );
	}

	sum /= ( 64.0f * 64.0f );
	avgLuminance = exp( sum );

	// the user's adapted luminance level is simulated by closing the gap between
	// adapted luminance and current luminance by 2% every frame, based on a
	// 30 fps rate. This is not an accurate model of human adaptation, which can
	// take longer than half an hour.
	if ( backEnd.hdrTime > curTime )
	{
		backEnd.hdrTime = curTime;
	}

	deltaTime = curTime - backEnd.hdrTime;

	//if(r_hdrMaxLuminance->value)
	{
		Q_clamp( backEnd.hdrAverageLuminance, r_hdrMinLuminance->value, r_hdrMaxLuminance->value );
		Q_clamp( avgLuminance, r_hdrMinLuminance->value, r_hdrMaxLuminance->value );

		Q_clamp( backEnd.hdrMaxLuminance, r_hdrMinLuminance->value, r_hdrMaxLuminance->value );
		Q_clamp( maxLuminance, r_hdrMinLuminance->value, r_hdrMaxLuminance->value );
	}

	newAdaptation = backEnd.hdrAverageLuminance + ( avgLuminance - backEnd.hdrAverageLuminance ) * ( 1.0f - powf( 0.98f, 30.0f * deltaTime ) );
	newMaximum = backEnd.hdrMaxLuminance + ( maxLuminance - backEnd.hdrMaxLuminance ) * ( 1.0f - powf( 0.98f, 30.0f * deltaTime ) );

	if ( !Q_isnan( newAdaptation ) && !Q_isnan( newMaximum ) )
	{
#if 1
		backEnd.hdrAverageLuminance = newAdaptation;
		backEnd.hdrMaxLuminance = newMaximum;
#else
		backEnd.hdrAverageLuminance = avgLuminance;
		backEnd.hdrMaxLuminance = maxLuminance;
#endif
	}

	backEnd.hdrTime = curTime;

	// calculate HDR image key
	if ( r_hdrKey->value <= 0 )
	{
		// calculation from: Perceptual Effects in Real-time Tone Mapping - Krawczyk et al.
		backEnd.hdrKey = 1.03 - 2.0 / ( 2.0 + log10f( backEnd.hdrAverageLuminance + 1.0f ) );
	}
	else
	{
		backEnd.hdrKey = r_hdrKey->value;
	}

	if ( r_hdrDebug->integer )
	{
		ri.Printf( PRINT_ALL, "HDR luminance avg = %f, max = %f, key = %f\n", backEnd.hdrAverageLuminance, backEnd.hdrMaxLuminance, backEnd.hdrKey );
	}

	GL_CheckErrors();
}

void RB_RenderDeferredShadingResultToFrameBuffer()
{
	matrix_t ortho;

	GLimp_LogComment( "--- RB_RenderDeferredShadingResultToFrameBuffer ---\n" );

	R_BindNullFBO();

	/*
	   if(backEnd.refdef.rdflags & RDF_NOWORLDMODEL)
	   {
	   GL_State(GLS_DEPTHTEST_DISABLE | GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE);
	   }
	   else
	 */
	{
		GL_State( GLS_DEPTHTEST_DISABLE );  // | GLS_DEPTHMASK_TRUE);
	}

	GL_Cull( CT_TWO_SIDED );

	// set uniforms

	// set 2D virtual screen size
	GL_PushMatrix();
	MatrixOrthogonalProjection( ortho, backEnd.viewParms.viewportX,
	                            backEnd.viewParms.viewportX + backEnd.viewParms.viewportWidth,
	                            backEnd.viewParms.viewportY, backEnd.viewParms.viewportY + backEnd.viewParms.viewportHeight,
	                            -99999, 99999 );
	GL_LoadProjectionMatrix( ortho );
	GL_LoadModelViewMatrix( matrixIdentity );

	if ( !( backEnd.refdef.rdflags & RDF_NOWORLDMODEL ) && r_hdrRendering->integer )
	{
		R_BindNullFBO();

		gl_toneMappingShader->DisableMacro_BRIGHTPASS_FILTER();
		gl_toneMappingShader->BindProgram();

		gl_toneMappingShader->SetUniform_HDRKey( backEnd.hdrKey );
		gl_toneMappingShader->SetUniform_HDRAverageLuminance( backEnd.hdrAverageLuminance );
		gl_toneMappingShader->SetUniform_HDRMaxLuminance( backEnd.hdrMaxLuminance );

		gl_toneMappingShader->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );

		// bind u_ColorMap
		GL_SelectTexture( 0 );
		GL_Bind( tr.deferredRenderFBOImage );
	}
	else
	{
		gl_screenShader->BindProgram();
		glVertexAttrib4fv( ATTR_INDEX_COLOR, colorWhite );

		gl_screenShader->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );

		// bind u_ColorMap
		GL_SelectTexture( 0 );

		if ( r_showDeferredDiffuse->integer )
		{
			GL_Bind( tr.deferredDiffuseFBOImage );
		}
		else if ( r_showDeferredNormal->integer )
		{
			GL_Bind( tr.deferredNormalFBOImage );
		}
		else if ( r_showDeferredSpecular->integer )
		{
			GL_Bind( tr.deferredSpecularFBOImage );
		}
		else if ( r_showDeferredPosition->integer )
		{
			GL_Bind( tr.depthRenderImage );
		}
		else if ( r_showDeferredLight->integer )
		{
			GL_Bind( tr.lightRenderFBOImage );
		}
		else
		{
			GL_Bind( tr.deferredRenderFBOImage );
		}
	}

	GL_CheckErrors();

	Tess_InstantQuad( backEnd.viewParms.viewportVerts );

	GL_PopMatrix();
}

void RB_RenderDeferredHDRResultToFrameBuffer()
{
	matrix_t ortho;

	GLimp_LogComment( "--- RB_RenderDeferredHDRResultToFrameBuffer ---\n" );

	if ( !r_hdrRendering->integer || !glConfig2.framebufferObjectAvailable || !glConfig2.textureFloatAvailable )
	{
		return;
	}

	GL_CheckErrors();

	R_BindNullFBO();

	// bind u_CurrentMap
	GL_SelectTexture( 0 );
	GL_Bind( tr.deferredRenderFBOImage );

	GL_State( GLS_DEPTHTEST_DISABLE );
	GL_Cull( CT_TWO_SIDED );

	// set uniforms

	// set 2D virtual screen size
	GL_PushMatrix();
	MatrixOrthogonalProjection( ortho, backEnd.viewParms.viewportX,
	                            backEnd.viewParms.viewportX + backEnd.viewParms.viewportWidth,
	                            backEnd.viewParms.viewportY, backEnd.viewParms.viewportY + backEnd.viewParms.viewportHeight,
	                            -99999, 99999 );
	GL_LoadProjectionMatrix( ortho );
	GL_LoadModelViewMatrix( matrixIdentity );

	if ( backEnd.refdef.rdflags & RDF_NOWORLDMODEL )
	{
		gl_screenShader->BindProgram();

		glVertexAttrib4fv( ATTR_INDEX_COLOR, colorWhite );

		gl_screenShader->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );
	}
	else
	{
		gl_toneMappingShader->DisableMacro_BRIGHTPASS_FILTER();
		gl_toneMappingShader->BindProgram();

		gl_toneMappingShader->SetUniform_HDRKey( backEnd.hdrKey );
		gl_toneMappingShader->SetUniform_HDRAverageLuminance( backEnd.hdrAverageLuminance );
		gl_toneMappingShader->SetUniform_HDRMaxLuminance( backEnd.hdrMaxLuminance );

		gl_toneMappingShader->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );
	}

	GL_CheckErrors();

	Tess_InstantQuad( backEnd.viewParms.viewportVerts );

	GL_PopMatrix();
}

// ================================================================================================
//
// LIGHTS OCCLUSION CULLING
//
// ================================================================================================

static void RenderLightOcclusionVolume( trRefLight_t *light )
{
	int    j;
	vec4_t quadVerts[ 4 ];

	GL_CheckErrors();

#if 1

	if ( light->isStatic && light->frustumVBO && light->frustumIBO )
	{
		// render in world space
		backEnd.orientation = backEnd.viewParms.world;
		GL_LoadModelViewMatrix( backEnd.viewParms.world.modelViewMatrix );
		gl_genericShader->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );

		R_BindVBO( light->frustumVBO );
		R_BindIBO( light->frustumIBO );

		GL_VertexAttribsState( ATTR_POSITION );

		tess.numVertexes = light->frustumVerts;
		tess.numIndexes = light->frustumIndexes;

		Tess_DrawElements();
	}
	else
#endif
	{
		// render in light space
		R_RotateLightForViewParms( light, &backEnd.viewParms, &backEnd.orientation );
		GL_LoadModelViewMatrix( backEnd.orientation.modelViewMatrix );
		gl_genericShader->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );

		tess.multiDrawPrimitives = 0;
		tess.numIndexes = 0;
		tess.numVertexes = 0;

		switch ( light->l.rlType )
		{
			case RL_OMNI:
				{
					Tess_AddCube( vec3_origin, light->localBounds[ 0 ], light->localBounds[ 1 ], colorWhite );

					Tess_UpdateVBOs( ATTR_POSITION | ATTR_COLOR );
					Tess_DrawElements();
					break;
				}

			case RL_PROJ:
				{
					vec3_t farCorners[ 4 ];
					vec4_t *frustum = light->localFrustum;

					PlanesGetIntersectionPoint( frustum[ FRUSTUM_LEFT ], frustum[ FRUSTUM_TOP ], frustum[ FRUSTUM_FAR ], farCorners[ 0 ] );
					PlanesGetIntersectionPoint( frustum[ FRUSTUM_RIGHT ], frustum[ FRUSTUM_TOP ], frustum[ FRUSTUM_FAR ], farCorners[ 1 ] );
					PlanesGetIntersectionPoint( frustum[ FRUSTUM_RIGHT ], frustum[ FRUSTUM_BOTTOM ], frustum[ FRUSTUM_FAR ], farCorners[ 2 ] );
					PlanesGetIntersectionPoint( frustum[ FRUSTUM_LEFT ], frustum[ FRUSTUM_BOTTOM ], frustum[ FRUSTUM_FAR ], farCorners[ 3 ] );

					tess.multiDrawPrimitives = 0;
					tess.numVertexes = 0;
					tess.numIndexes = 0;

					if ( !VectorCompare( light->l.projStart, vec3_origin ) )
					{
						vec3_t nearCorners[ 4 ];

						// calculate the vertices defining the top area
						PlanesGetIntersectionPoint( frustum[ FRUSTUM_LEFT ], frustum[ FRUSTUM_TOP ], frustum[ FRUSTUM_NEAR ], nearCorners[ 0 ] );
						PlanesGetIntersectionPoint( frustum[ FRUSTUM_RIGHT ], frustum[ FRUSTUM_TOP ], frustum[ FRUSTUM_NEAR ], nearCorners[ 1 ] );
						PlanesGetIntersectionPoint( frustum[ FRUSTUM_RIGHT ], frustum[ FRUSTUM_BOTTOM ], frustum[ FRUSTUM_NEAR ], nearCorners[ 2 ] );
						PlanesGetIntersectionPoint( frustum[ FRUSTUM_LEFT ], frustum[ FRUSTUM_BOTTOM ], frustum[ FRUSTUM_NEAR ], nearCorners[ 3 ] );

						// draw outer surfaces
						for ( j = 0; j < 4; j++ )
						{
							Vector4Set( quadVerts[ 0 ], nearCorners[ j ][ 0 ], nearCorners[ j ][ 1 ], nearCorners[ j ][ 2 ], 1 );
							Vector4Set( quadVerts[ 1 ], farCorners[ j ][ 0 ], farCorners[ j ][ 1 ], farCorners[ j ][ 2 ], 1 );
							Vector4Set( quadVerts[ 2 ], farCorners[( j + 1 ) % 4 ][ 0 ], farCorners[( j + 1 ) % 4 ][ 1 ], farCorners[( j + 1 ) % 4 ][ 2 ], 1 );
							Vector4Set( quadVerts[ 3 ], nearCorners[( j + 1 ) % 4 ][ 0 ], nearCorners[( j + 1 ) % 4 ][ 1 ], nearCorners[( j + 1 ) % 4 ][ 2 ], 1 );
							Tess_AddQuadStamp2( quadVerts, colorCyan );
						}

						// draw far cap
						Vector4Set( quadVerts[ 0 ], farCorners[ 3 ][ 0 ], farCorners[ 3 ][ 1 ], farCorners[ 3 ][ 2 ], 1 );
						Vector4Set( quadVerts[ 1 ], farCorners[ 2 ][ 0 ], farCorners[ 2 ][ 1 ], farCorners[ 2 ][ 2 ], 1 );
						Vector4Set( quadVerts[ 2 ], farCorners[ 1 ][ 0 ], farCorners[ 1 ][ 1 ], farCorners[ 1 ][ 2 ], 1 );
						Vector4Set( quadVerts[ 3 ], farCorners[ 0 ][ 0 ], farCorners[ 0 ][ 1 ], farCorners[ 0 ][ 2 ], 1 );
						Tess_AddQuadStamp2( quadVerts, colorRed );

						// draw near cap
						Vector4Set( quadVerts[ 0 ], nearCorners[ 0 ][ 0 ], nearCorners[ 0 ][ 1 ], nearCorners[ 0 ][ 2 ], 1 );
						Vector4Set( quadVerts[ 1 ], nearCorners[ 1 ][ 0 ], nearCorners[ 1 ][ 1 ], nearCorners[ 1 ][ 2 ], 1 );
						Vector4Set( quadVerts[ 2 ], nearCorners[ 2 ][ 0 ], nearCorners[ 2 ][ 1 ], nearCorners[ 2 ][ 2 ], 1 );
						Vector4Set( quadVerts[ 3 ], nearCorners[ 3 ][ 0 ], nearCorners[ 3 ][ 1 ], nearCorners[ 3 ][ 2 ], 1 );
						Tess_AddQuadStamp2( quadVerts, colorGreen );
					}
					else
					{
						vec3_t top;

						// no light_start, just use the top vertex (doesn't need to be mirrored)
						PlanesGetIntersectionPoint( frustum[ FRUSTUM_LEFT ], frustum[ FRUSTUM_RIGHT ], frustum[ FRUSTUM_TOP ], top );

						// draw pyramid
						for ( j = 0; j < 4; j++ )
						{
							VectorCopy( farCorners[ j ], tess.xyz[ tess.numVertexes ] );
							Vector4Copy( colorCyan, tess.colors[ tess.numVertexes ] );
							tess.indexes[ tess.numIndexes++ ] = tess.numVertexes;
							tess.numVertexes++;

							VectorCopy( farCorners[( j + 1 ) % 4 ], tess.xyz[ tess.numVertexes ] );
							Vector4Copy( colorCyan, tess.colors[ tess.numVertexes ] );
							tess.indexes[ tess.numIndexes++ ] = tess.numVertexes;
							tess.numVertexes++;

							VectorCopy( top, tess.xyz[ tess.numVertexes ] );
							Vector4Copy( colorCyan, tess.colors[ tess.numVertexes ] );
							tess.indexes[ tess.numIndexes++ ] = tess.numVertexes;
							tess.numVertexes++;
						}

						Vector4Set( quadVerts[ 0 ], farCorners[ 3 ][ 0 ], farCorners[ 3 ][ 1 ], farCorners[ 3 ][ 2 ], 1 );
						Vector4Set( quadVerts[ 1 ], farCorners[ 2 ][ 0 ], farCorners[ 2 ][ 1 ], farCorners[ 2 ][ 2 ], 1 );
						Vector4Set( quadVerts[ 2 ], farCorners[ 1 ][ 0 ], farCorners[ 1 ][ 1 ], farCorners[ 1 ][ 2 ], 1 );
						Vector4Set( quadVerts[ 3 ], farCorners[ 0 ][ 0 ], farCorners[ 0 ][ 1 ], farCorners[ 0 ][ 2 ], 1 );
						Tess_AddQuadStamp2( quadVerts, colorRed );
					}

					Tess_UpdateVBOs( ATTR_POSITION | ATTR_COLOR );
					Tess_DrawElements();
					break;
				}

			default:
				break;
		}
	}

	tess.multiDrawPrimitives = 0;
	tess.numIndexes = 0;
	tess.numVertexes = 0;

	GL_CheckErrors();
}

static void IssueLightOcclusionQuery( link_t *queue, trRefLight_t *light, qboolean resetMultiQueryLink )
{
	GLimp_LogComment( "--- IssueLightOcclusionQuery ---\n" );

	//ri.Printf(PRINT_ALL, "--- IssueOcclusionQuery(%i) ---\n", node - tr.world->nodes);

	if ( tr.numUsedOcclusionQueryObjects < ( MAX_OCCLUSION_QUERIES - 1 ) )
	{
		light->occlusionQueryObject = tr.occlusionQueryObjects[ tr.numUsedOcclusionQueryObjects++ ];
	}
	else
	{
		light->occlusionQueryObject = 0;
	}

	EnQueue( queue, light );

	// tell GetOcclusionQueryResult that this is not a multi query
	if ( resetMultiQueryLink )
	{
		QueueInit( &light->multiQuery );
	}

	if ( light->occlusionQueryObject > 0 )
	{
		GL_CheckErrors();

		// begin the occlusion query
		glBeginQuery( GL_SAMPLES_PASSED, light->occlusionQueryObject );

		GL_CheckErrors();

		RenderLightOcclusionVolume( light );

		// end the query
		glEndQuery( GL_SAMPLES_PASSED );

#if 1

		if ( !glIsQuery( light->occlusionQueryObject ) )
		{
			ri.Error( ERR_FATAL, "IssueLightOcclusionQuery: light %i has no occlusion query object in slot %i: %i", ( int )( light - tr.world->lights ), backEnd.viewParms.viewCount, light->occlusionQueryObject );
		}

#endif

		//light->occlusionQueryNumbers[backEnd.viewParms.viewCount] = backEnd.pc.c_occlusionQueries;
		backEnd.pc.c_occlusionQueries++;
	}

	GL_CheckErrors();
}

static void IssueLightMultiOcclusionQueries( link_t *multiQueue, link_t *individualQueue )
{
	trRefLight_t *light;
	trRefLight_t *multiQueryLight;
	link_t       *l;

	GLimp_LogComment( "--- IssueLightMultiOcclusionQueries ---\n" );

#if 0
	ri.Printf( PRINT_ALL, "IssueLightMultiOcclusionQueries(" );

	for ( l = multiQueue->prev; l != multiQueue; l = l->prev )
	{
		light = ( trRefLight_t * ) l->data;

		ri.Printf( PRINT_ALL, "%i, ", light - backEnd.refdef.lights );
	}

	ri.Printf( PRINT_ALL, ")\n" );
#endif

	if ( QueueEmpty( multiQueue ) )
	{
		return;
	}

	multiQueryLight = ( trRefLight_t * ) QueueFront( multiQueue )->data;

	if ( tr.numUsedOcclusionQueryObjects < ( MAX_OCCLUSION_QUERIES - 1 ) )
	{
		multiQueryLight->occlusionQueryObject = tr.occlusionQueryObjects[ tr.numUsedOcclusionQueryObjects++ ];
	}
	else
	{
		multiQueryLight->occlusionQueryObject = 0;
	}

	if ( multiQueryLight->occlusionQueryObject > 0 )
	{
		// begin the occlusion query
		GL_CheckErrors();

		glBeginQuery( GL_SAMPLES_PASSED, multiQueryLight->occlusionQueryObject );

		GL_CheckErrors();

		//ri.Printf(PRINT_ALL, "rendering nodes:[");
		for ( l = multiQueue->prev; l != multiQueue; l = l->prev )
		{
			light = ( trRefLight_t * ) l->data;

			//ri.Printf(PRINT_ALL, "%i, ", light - backEnd.refdef.lights);

			RenderLightOcclusionVolume( light );
		}

		//ri.Printf(PRINT_ALL, "]\n");

		backEnd.pc.c_occlusionQueries++;
		backEnd.pc.c_occlusionQueriesMulti++;

		// end the query
		glEndQuery( GL_SAMPLES_PASSED );

		GL_CheckErrors();

#if 0

		if ( !glIsQuery( multiQueryNode->occlusionQueryObjects[ backEnd.viewParms.viewCount ] ) )
		{
			ri.Error( ERR_FATAL, "IssueMultiOcclusionQueries: node %i has no occlusion query object in slot %i: %i", multiQueryNode - tr.world->nodes, backEnd.viewParms.viewCount, multiQueryNode->occlusionQueryObjects[ backEnd.viewParms.viewCount ] );
		}

#endif
	}

	// move queue to node->multiQuery queue
	QueueInit( &multiQueryLight->multiQuery );
	DeQueue( multiQueue );

	while ( !QueueEmpty( multiQueue ) )
	{
		light = ( trRefLight_t * ) DeQueue( multiQueue );
		EnQueue( &multiQueryLight->multiQuery, light );
	}

	EnQueue( individualQueue, multiQueryLight );

	//ri.Printf(PRINT_ALL, "--- IssueMultiOcclusionQueries end ---\n");
}

static qboolean LightOcclusionResultAvailable( trRefLight_t *light )
{
	GLint available;

	if ( light->occlusionQueryObject > 0 )
	{
		glFinish();

		available = 0;
		//if(glIsQuery(light->occlusionQueryObjects[backEnd.viewParms.viewCount]))
		{
			glGetQueryObjectiv( light->occlusionQueryObject, GL_QUERY_RESULT_AVAILABLE, &available );
			GL_CheckErrors();
		}

		return ( qboolean ) available;
	}

	return qtrue;
}

static void GetLightOcclusionQueryResult( trRefLight_t *light )
{
	link_t *l, *sentinel;
	int    ocSamples;
	GLint  available;

	GLimp_LogComment( "--- GetLightOcclusionQueryResult ---\n" );

	if ( light->occlusionQueryObject > 0 )
	{
		glFinish();

#if 0

		if ( !glIsQuery( node->occlusionQueryObjects[ backEnd.viewParms.viewCount ] ) )
		{
			ri.Error( ERR_FATAL, "GetOcclusionQueryResult: node %i has no occlusion query object in slot %i: %i", node - tr.world->nodes, backEnd.viewParms.viewCount, node->occlusionQueryObjects[ backEnd.viewParms.viewCount ] );
		}

#endif

		available = 0;

		while ( !available )
		{
			//if(glIsQuery(node->occlusionQueryObjects[backEnd.viewParms.viewCount]))
			{
				glGetQueryObjectiv( light->occlusionQueryObject, GL_QUERY_RESULT_AVAILABLE, &available );
				//GL_CheckErrors();
			}
		}

		backEnd.pc.c_occlusionQueriesAvailable++;

		glGetQueryObjectiv( light->occlusionQueryObject, GL_QUERY_RESULT, &ocSamples );

		//ri.Printf(PRINT_ALL, "GetOcclusionQueryResult(%i): available = %i, samples = %i\n", node - tr.world->nodes, available, ocSamples);

		GL_CheckErrors();
	}
	else
	{
		ocSamples = 1;
	}

	light->occlusionQuerySamples = ocSamples;

	// copy result to all nodes that were linked to this multi query node
	sentinel = &light->multiQuery;

	for ( l = sentinel->prev; l != sentinel; l = l->prev )
	{
		light = ( trRefLight_t * ) l->data;

		light->occlusionQuerySamples = ocSamples;
	}
}

static int LightCompare( const void *a, const void *b )
{
	trRefLight_t *l1, *l2;
	float        d1, d2;

	l1 = ( trRefLight_t * ) * ( void ** ) a;
	l2 = ( trRefLight_t * ) * ( void ** ) b;

	d1 = DistanceSquared( backEnd.viewParms.orientation.origin, l1->l.origin );
	d2 = DistanceSquared( backEnd.viewParms.orientation.origin, l2->l.origin );

	if ( d1 < d2 )
	{
		return -1;
	}

	if ( d1 > d2 )
	{
		return 1;
	}

	return 0;
}

void RB_RenderLightOcclusionQueries()
{
	GLimp_LogComment( "--- RB_RenderLightOcclusionQueries ---\n" );

	if ( glConfig2.occlusionQueryBits && glConfig.driverType != GLDRV_MESA && r_dynamicLightOcclusionCulling->integer && !( backEnd.refdef.rdflags & RDF_NOWORLDMODEL ) )
	{
		int           i;
		interaction_t *ia;
		int           iaCount;
		int           iaFirst;
		trRefLight_t  *light, *oldLight, *multiQueryLight;
		GLint         ocSamples = 0;
		qboolean      queryObjects;
		link_t        occlusionQueryQueue;
		link_t        invisibleQueue;
		growList_t    invisibleList;
		int           startTime = 0, endTime = 0;

		glVertexAttrib4f( ATTR_INDEX_COLOR, 1.0f, 0.0f, 0.0f, 0.05f );

		if ( r_speeds->integer == RSPEEDS_OCCLUSION_QUERIES )
		{
			glFinish();
			startTime = ri.Milliseconds();
		}

		gl_genericShader->DisableAlphaTesting();
		gl_genericShader->DisablePortalClipping();
		gl_genericShader->DisableVertexSkinning();
		gl_genericShader->DisableVertexAnimation();
		gl_genericShader->DisableDeformVertexes();
		gl_genericShader->DisableTCGenEnvironment();

		gl_genericShader->BindProgram();
		gl_genericShader->SetRequiredVertexPointers();

		GL_Cull( CT_TWO_SIDED );

		GL_LoadProjectionMatrix( backEnd.viewParms.projectionMatrix );

		// set uniforms
		gl_genericShader->SetUniform_ColorModulate( CGEN_VERTEX, AGEN_VERTEX );
		gl_genericShader->SetUniform_Color( colorBlack );

		// bind u_ColorMap
		GL_SelectTexture( 0 );
		GL_Bind( tr.whiteImage );
		gl_genericShader->SetUniform_ColorTextureMatrix( matrixIdentity );

		// don't write to the color buffer or depth buffer
		if ( r_showOcclusionQueries->integer )
		{
			GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE );
		}
		else
		{
			GL_State( GLS_COLORMASK_BITS );
		}

		tr.numUsedOcclusionQueryObjects = 0;
		QueueInit( &occlusionQueryQueue );
		QueueInit( &invisibleQueue );
		Com_InitGrowList( &invisibleList, 1000 );

		// loop trough all light interactions and render the light OBB for each last interaction
		for ( iaCount = 0, ia = &backEnd.viewParms.interactions[ 0 ]; iaCount < backEnd.viewParms.numInteractions; )
		{
			backEnd.currentLight = light = ia->light;
			ia->occlusionQuerySamples = 1;

			if ( !ia->next )
			{
				// last interaction of current light
				if ( !ia->noOcclusionQueries )
				{
					Com_AddToGrowList( &invisibleList, light );
				}

				if ( iaCount < ( backEnd.viewParms.numInteractions - 1 ) )
				{
					// jump to next interaction and continue
					ia++;
					iaCount++;
				}
				else
				{
					// increase last time to leave for loop
					iaCount++;
				}
			}
			else
			{
				// just continue
				ia = ia->next;
				iaCount++;
			}
		}

		// sort lights by distance
		qsort( invisibleList.elements, invisibleList.currentElements, sizeof( void * ), LightCompare );

		for ( i = 0; i < invisibleList.currentElements; i++ )
		{
			light = ( trRefLight_t * ) Com_GrowListElement( &invisibleList, i );

			EnQueue( &invisibleQueue, light );

			if ( ( invisibleList.currentElements - i ) <= 100 )
			{
				if ( QueueSize( &invisibleQueue ) >= 10 )
				{
					IssueLightMultiOcclusionQueries( &invisibleQueue, &occlusionQueryQueue );
				}
			}
			else
			{
				if ( QueueSize( &invisibleQueue ) >= 50 )
				{
					IssueLightMultiOcclusionQueries( &invisibleQueue, &occlusionQueryQueue );
				}
			}
		}

		Com_DestroyGrowList( &invisibleList );

		if ( !QueueEmpty( &invisibleQueue ) )
		{
			// remaining previously invisible node queries
			IssueLightMultiOcclusionQueries( &invisibleQueue, &occlusionQueryQueue );

			//ri.Printf(PRINT_ALL, "occlusionQueryQueue.empty() = %i\n", QueueEmpty(&occlusionQueryQueue));
		}

		// go back to the world modelview matrix
		backEnd.orientation = backEnd.viewParms.world;
		GL_LoadModelViewMatrix( backEnd.viewParms.world.modelViewMatrix );

		while ( !QueueEmpty( &occlusionQueryQueue ) )
		{
			if ( LightOcclusionResultAvailable( ( trRefLight_t * ) QueueFront( &occlusionQueryQueue )->data ) )
			{
				light = ( trRefLight_t * ) DeQueue( &occlusionQueryQueue );

				// wait if result not available
				GetLightOcclusionQueryResult( light );

				if ( ( signed ) light->occlusionQuerySamples > r_chcVisibilityThreshold->integer )
				{
					// if a query of multiple previously invisible objects became visible, we need to
					// test all the individual objects ...
					if ( !QueueEmpty( &light->multiQuery ) )
					{
						multiQueryLight = light;

						IssueLightOcclusionQuery( &occlusionQueryQueue, multiQueryLight, qfalse );

						while ( !QueueEmpty( &multiQueryLight->multiQuery ) )
						{
							light = ( trRefLight_t * ) DeQueue( &multiQueryLight->multiQuery );

							IssueLightOcclusionQuery( &occlusionQueryQueue, light, qtrue );
						}
					}
				}
				else
				{
					if ( !QueueEmpty( &light->multiQuery ) )
					{
						backEnd.pc.c_occlusionQueriesLightsCulled++;

						multiQueryLight = light;

						while ( !QueueEmpty( &multiQueryLight->multiQuery ) )
						{
							light = ( trRefLight_t * ) DeQueue( &multiQueryLight->multiQuery );

							backEnd.pc.c_occlusionQueriesLightsCulled++;
							backEnd.pc.c_occlusionQueriesSaved++;
						}
					}
					else
					{
						backEnd.pc.c_occlusionQueriesLightsCulled++;
					}
				}
			}
		}

		if ( r_speeds->integer == RSPEEDS_OCCLUSION_QUERIES )
		{
			glFinish();
			endTime = ri.Milliseconds();
			backEnd.pc.c_occlusionQueriesResponseTime = endTime - startTime;

			startTime = ri.Milliseconds();
		}

		// go back to the world modelview matrix
		backEnd.orientation = backEnd.viewParms.world;
		GL_LoadModelViewMatrix( backEnd.viewParms.world.modelViewMatrix );

		// reenable writes to depth and color buffers
		GL_State( GLS_DEPTHMASK_TRUE );

		// loop trough all light interactions and fetch results for each last interaction
		// then copy result to all other interactions that belong to the same light
		iaFirst = 0;
		queryObjects = qtrue;
		oldLight = NULL;

		for ( iaCount = 0, ia = &backEnd.viewParms.interactions[ 0 ]; iaCount < backEnd.viewParms.numInteractions; )
		{
			backEnd.currentLight = light = ia->light;

			if ( light != oldLight )
			{
				iaFirst = iaCount;
			}

			if ( !queryObjects )
			{
				ia->occlusionQuerySamples = ocSamples;

				if ( ocSamples <= 0 )
				{
					backEnd.pc.c_occlusionQueriesInteractionsCulled++;
				}
			}

			if ( !ia->next )
			{
				if ( queryObjects )
				{
					if ( !ia->noOcclusionQueries )
					{
						ocSamples = ( signed ) light->occlusionQuerySamples > r_chcVisibilityThreshold->integer;
					}
					else
					{
						ocSamples = 1;
					}

					// jump back to first interaction of this light copy query result
					ia = &backEnd.viewParms.interactions[ iaFirst ];
					iaCount = iaFirst;
					queryObjects = qfalse;
				}
				else
				{
					if ( iaCount < ( backEnd.viewParms.numInteractions - 1 ) )
					{
						// jump to next interaction and start querying
						ia++;
						iaCount++;
						queryObjects = qtrue;
					}
					else
					{
						// increase last time to leave for loop
						iaCount++;
					}
				}
			}
			else
			{
				// just continue
				ia = ia->next;
				iaCount++;
			}

			oldLight = light;
		}

		if ( r_speeds->integer == RSPEEDS_OCCLUSION_QUERIES )
		{
			glFinish();
			endTime = ri.Milliseconds();
			backEnd.pc.c_occlusionQueriesFetchTime = endTime - startTime;
		}
	}

	GL_CheckErrors();
}

// ================================================================================================
//
// ENTITY OCCLUSION CULLING
//
// ================================================================================================

static void RenderEntityOcclusionVolume( trRefEntity_t *entity )
{
	GL_CheckErrors();

#if 0
	// render in entity space
	R_RotateEntityForViewParms( entity, &backEnd.viewParms, &backEnd.orientation );
	GL_LoadModelViewMatrix( backEnd.orientation.modelViewMatrix );
	gl_genericShader->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );

	tess.multiDrawPrimitives = 0;
	tess.numIndexes = 0;
	tess.numVertexes = 0;

	Tess_AddCube( vec3_origin, entity->localBounds[ 0 ], entity->localBounds[ 1 ], colorBlue );

	Tess_UpdateVBOs( ATTR_POSITION | ATTR_COLOR );
	Tess_DrawElements();

	tess.multiDrawPrimitives = 0;
	tess.numIndexes = 0;
	tess.numVertexes = 0;
#else

	vec3_t   boundsCenter;
	vec3_t   boundsSize;
	matrix_t rot;
	axis_t   axis;

#if 0
	VectorSubtract( entity->localBounds[ 1 ], entity->localBounds[ 0 ], boundsSize );
#else
	boundsSize[ 0 ] = Q_fabs( entity->localBounds[ 0 ][ 0 ] ) + Q_fabs( entity->localBounds[ 1 ][ 0 ] );
	boundsSize[ 1 ] = Q_fabs( entity->localBounds[ 0 ][ 1 ] ) + Q_fabs( entity->localBounds[ 1 ][ 1 ] );
	boundsSize[ 2 ] = Q_fabs( entity->localBounds[ 0 ][ 2 ] ) + Q_fabs( entity->localBounds[ 1 ][ 2 ] );
#endif

	VectorScale( entity->e.axis[ 0 ], boundsSize[ 0 ] * 0.5f, axis[ 0 ] );
	VectorScale( entity->e.axis[ 1 ], boundsSize[ 1 ] * 0.5f, axis[ 1 ] );
	VectorScale( entity->e.axis[ 2 ], boundsSize[ 2 ] * 0.5f, axis[ 2 ] );

	VectorAdd( entity->localBounds[ 0 ], entity->localBounds[ 1 ], boundsCenter );
	VectorScale( boundsCenter, 0.5f, boundsCenter );

	MatrixFromVectorsFLU( rot, entity->e.axis[ 0 ], entity->e.axis[ 1 ], entity->e.axis[ 2 ] );
	MatrixTransformNormal2( rot, boundsCenter );

	VectorAdd( entity->e.origin, boundsCenter, boundsCenter );

	MatrixSetupTransformFromVectorsFLU( backEnd.orientation.transformMatrix, axis[ 0 ], axis[ 1 ], axis[ 2 ], boundsCenter );

	MatrixAffineInverse( backEnd.orientation.transformMatrix, backEnd.orientation.viewMatrix );
	MatrixMultiply( backEnd.viewParms.world.viewMatrix, backEnd.orientation.transformMatrix, backEnd.orientation.modelViewMatrix );

	GL_LoadModelViewMatrix( backEnd.orientation.modelViewMatrix );
	gl_genericShader->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );

	R_BindVBO( tr.unitCubeVBO );
	R_BindIBO( tr.unitCubeIBO );

	GL_VertexAttribsState( ATTR_POSITION );

	tess.multiDrawPrimitives = 0;
	tess.numVertexes = tr.unitCubeVBO->vertexesNum;
	tess.numIndexes = tr.unitCubeIBO->indexesNum;

	Tess_DrawElements();

	tess.multiDrawPrimitives = 0;
	tess.numIndexes = 0;
	tess.numVertexes = 0;

#endif

	GL_CheckErrors();
}

static void IssueEntityOcclusionQuery( link_t *queue, trRefEntity_t *entity, qboolean resetMultiQueryLink )
{
	GLimp_LogComment( "--- IssueEntityOcclusionQuery ---\n" );

	//ri.Printf(PRINT_ALL, "--- IssueEntityOcclusionQuery(%i) ---\n", light - backEnd.refdef.lights);

	if ( tr.numUsedOcclusionQueryObjects < ( MAX_OCCLUSION_QUERIES - 1 ) )
	{
		entity->occlusionQueryObject = tr.occlusionQueryObjects[ tr.numUsedOcclusionQueryObjects++ ];
	}
	else
	{
		entity->occlusionQueryObject = 0;
	}

	EnQueue( queue, entity );

	// tell GetOcclusionQueryResult that this is not a multi query
	if ( resetMultiQueryLink )
	{
		QueueInit( &entity->multiQuery );
	}

	if ( entity->occlusionQueryObject > 0 )
	{
		GL_CheckErrors();

		// begin the occlusion query
		glBeginQuery( GL_SAMPLES_PASSED, entity->occlusionQueryObject );

		GL_CheckErrors();

		RenderEntityOcclusionVolume( entity );

		// end the query
		glEndQuery( GL_SAMPLES_PASSED );

#if 0

		if ( !glIsQuery( entity->occlusionQueryObject ) )
		{
			ri.Error( ERR_FATAL, "IssueOcclusionQuery: entity %i has no occlusion query object in slot %i: %i", light - tr.world->lights, backEnd.viewParms.viewCount, light->occlusionQueryObject );
		}

#endif
		backEnd.pc.c_occlusionQueries++;
	}

	GL_CheckErrors();
}

static void IssueEntityMultiOcclusionQueries( link_t *multiQueue, link_t *individualQueue )
{
	trRefEntity_t *entity;
	trRefEntity_t *multiQueryEntity;
	link_t        *l;

	GLimp_LogComment( "--- IssueEntityMultiOcclusionQueries ---\n" );

#if 0
	ri.Printf( PRINT_ALL, "IssueEntityMultiOcclusionQueries(" );

	for ( l = multiQueue->prev; l != multiQueue; l = l->prev )
	{
		light = ( trRefEntity_t * ) l->data;

		ri.Printf( PRINT_ALL, "%i, ", light - backEnd.refdef.entities );
	}

	ri.Printf( PRINT_ALL, ")\n" );
#endif

	if ( QueueEmpty( multiQueue ) )
	{
		return;
	}

	multiQueryEntity = ( trRefEntity_t * ) QueueFront( multiQueue )->data;

	if ( tr.numUsedOcclusionQueryObjects < ( MAX_OCCLUSION_QUERIES - 1 ) )
	{
		multiQueryEntity->occlusionQueryObject = tr.occlusionQueryObjects[ tr.numUsedOcclusionQueryObjects++ ];
	}
	else
	{
		multiQueryEntity->occlusionQueryObject = 0;
	}

	if ( multiQueryEntity->occlusionQueryObject > 0 )
	{
		// begin the occlusion query
		GL_CheckErrors();

		glBeginQuery( GL_SAMPLES_PASSED, multiQueryEntity->occlusionQueryObject );

		GL_CheckErrors();

		//ri.Printf(PRINT_ALL, "rendering nodes:[");
		for ( l = multiQueue->prev; l != multiQueue; l = l->prev )
		{
			entity = ( trRefEntity_t * ) l->data;

			//ri.Printf(PRINT_ALL, "%i, ", light - backEnd.refdef.lights);

			RenderEntityOcclusionVolume( entity );
		}

		//ri.Printf(PRINT_ALL, "]\n");

		backEnd.pc.c_occlusionQueries++;
		backEnd.pc.c_occlusionQueriesMulti++;

		// end the query
		glEndQuery( GL_SAMPLES_PASSED );

		GL_CheckErrors();

#if 0

		if ( !glIsQuery( multiQueryNode->occlusionQueryObjects[ backEnd.viewParms.viewCount ] ) )
		{
			ri.Error( ERR_FATAL, "IssueEntityMultiOcclusionQueries: node %i has no occlusion query object in slot %i: %i", multiQueryNode - tr.world->nodes, backEnd.viewParms.viewCount, multiQueryNode->occlusionQueryObjects[ backEnd.viewParms.viewCount ] );
		}

#endif
	}

	// move queue to node->multiQuery queue
	QueueInit( &multiQueryEntity->multiQuery );
	DeQueue( multiQueue );

	while ( !QueueEmpty( multiQueue ) )
	{
		entity = ( trRefEntity_t * ) DeQueue( multiQueue );
		EnQueue( &multiQueryEntity->multiQuery, entity );
	}

	EnQueue( individualQueue, multiQueryEntity );

	//ri.Printf(PRINT_ALL, "--- IssueMultiOcclusionQueries end ---\n");
}

static qboolean EntityOcclusionResultAvailable( trRefEntity_t *entity )
{
	GLint available;

	if ( entity->occlusionQueryObject > 0 )
	{
		glFinish();

		available = 0;
		//if(glIsQuery(light->occlusionQueryObjects[backEnd.viewParms.viewCount]))
		{
			glGetQueryObjectiv( entity->occlusionQueryObject, GL_QUERY_RESULT_AVAILABLE, &available );
			GL_CheckErrors();
		}

		return ( qboolean ) available;
	}

	return qtrue;
}

static void GetEntityOcclusionQueryResult( trRefEntity_t *entity )
{
	link_t *l, *sentinel;
	int    ocSamples;
	GLint  available;

	GLimp_LogComment( "--- GetEntityOcclusionQueryResult ---\n" );

	if ( entity->occlusionQueryObject > 0 )
	{
		glFinish();

		available = 0;

		while ( !available )
		{
			//if(glIsQuery(node->occlusionQueryObjects[backEnd.viewParms.viewCount]))
			{
				glGetQueryObjectiv( entity->occlusionQueryObject, GL_QUERY_RESULT_AVAILABLE, &available );
				//GL_CheckErrors();
			}
		}

		backEnd.pc.c_occlusionQueriesAvailable++;

		glGetQueryObjectiv( entity->occlusionQueryObject, GL_QUERY_RESULT, &ocSamples );

		//ri.Printf(PRINT_ALL, "GetOcclusionQueryResult(%i): available = %i, samples = %i\n", node - tr.world->nodes, available, ocSamples);

		GL_CheckErrors();
	}
	else
	{
		ocSamples = 1;
	}

	entity->occlusionQuerySamples = ocSamples;

	// copy result to all nodes that were linked to this multi query node
	sentinel = &entity->multiQuery;

	for ( l = sentinel->prev; l != sentinel; l = l->prev )
	{
		entity = ( trRefEntity_t * ) l->data;

		entity->occlusionQuerySamples = ocSamples;
	}
}

static int EntityCompare( const void *a, const void *b )
{
	trRefEntity_t *e1, *e2;
	float         d1, d2;

	e1 = ( trRefEntity_t * ) * ( void ** ) a;
	e2 = ( trRefEntity_t * ) * ( void ** ) b;

	d1 = DistanceSquared( backEnd.viewParms.orientation.origin, e1->e.origin );
	d2 = DistanceSquared( backEnd.viewParms.orientation.origin, e2->e.origin );

	if ( d1 < d2 )
	{
		return -1;
	}

	if ( d1 > d2 )
	{
		return 1;
	}

	return 0;
}

void RB_RenderEntityOcclusionQueries()
{
	GLimp_LogComment( "--- RB_RenderEntityOcclusionQueries ---\n" );

	if ( glConfig2.occlusionQueryBits && glConfig.driverType != GLDRV_MESA && !( backEnd.refdef.rdflags & RDF_NOWORLDMODEL ) )
	{
		int           i;
		trRefEntity_t *entity, *multiQueryEntity;
		link_t        occlusionQueryQueue;
		link_t        invisibleQueue;
		growList_t    invisibleList;
		int           startTime = 0, endTime = 0;

		glVertexAttrib4f( ATTR_INDEX_COLOR, 1.0f, 0.0f, 0.0f, 0.05f );

		if ( r_speeds->integer == RSPEEDS_OCCLUSION_QUERIES )
		{
			glFinish();
			startTime = ri.Milliseconds();
		}

		gl_genericShader->DisableAlphaTesting();
		gl_genericShader->DisablePortalClipping();
		gl_genericShader->DisableVertexSkinning();
		gl_genericShader->DisableVertexAnimation();
		gl_genericShader->DisableDeformVertexes();
		gl_genericShader->DisableTCGenEnvironment();

		gl_genericShader->BindProgram();
		gl_genericShader->SetRequiredVertexPointers();

		GL_Cull( CT_TWO_SIDED );

		GL_LoadProjectionMatrix( backEnd.viewParms.projectionMatrix );

		// set uniforms
		gl_genericShader->SetUniform_ColorModulate( CGEN_CONST, AGEN_CONST );
		gl_genericShader->SetUniform_Color( colorBlue );

		// bind u_ColorMap
		GL_SelectTexture( 0 );
		GL_Bind( tr.whiteImage );
		gl_genericShader->SetUniform_ColorTextureMatrix( matrixIdentity );

		// don't write to the color buffer or depth buffer
		if ( r_showOcclusionQueries->integer )
		{
			GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE );
		}
		else
		{
			GL_State( GLS_COLORMASK_BITS );
		}

		tr.numUsedOcclusionQueryObjects = 0;
		QueueInit( &occlusionQueryQueue );
		QueueInit( &invisibleQueue );
		Com_InitGrowList( &invisibleList, 1000 );

		// loop trough all entities and render the entity OBB
		for ( i = 0, entity = backEnd.refdef.entities; i < backEnd.refdef.numEntities; i++, entity++ )
		{
			if ( ( entity->e.renderfx & RF_THIRD_PERSON ) && !backEnd.viewParms.isPortal )
			{
				continue;
			}

			if ( entity->cull == CULL_OUT )
			{
				continue;
			}

			backEnd.currentEntity = entity;

			entity->occlusionQuerySamples = 1;
			entity->noOcclusionQueries = qfalse;

			// check if the entity volume clips against the near plane
			if ( BoxOnPlaneSide( entity->worldBounds[ 0 ], entity->worldBounds[ 1 ], &backEnd.viewParms.frustums[ 0 ][ FRUSTUM_NEAR ] ) == 3 )
			{
				entity->noOcclusionQueries = qtrue;
			}
			else
			{
				Com_AddToGrowList( &invisibleList, entity );
			}
		}

		// sort entities by distance
		qsort( invisibleList.elements, invisibleList.currentElements, sizeof( void * ), EntityCompare );

		for ( i = 0; i < invisibleList.currentElements; i++ )
		{
			entity = ( trRefEntity_t * ) Com_GrowListElement( &invisibleList, i );

			EnQueue( &invisibleQueue, entity );

			if ( ( invisibleList.currentElements - i ) <= 100 )
			{
				if ( QueueSize( &invisibleQueue ) >= 10 )
				{
					IssueEntityMultiOcclusionQueries( &invisibleQueue, &occlusionQueryQueue );
				}
			}
			else
			{
				if ( QueueSize( &invisibleQueue ) >= 50 )
				{
					IssueEntityMultiOcclusionQueries( &invisibleQueue, &occlusionQueryQueue );
				}
			}
		}

		Com_DestroyGrowList( &invisibleList );

		if ( !QueueEmpty( &invisibleQueue ) )
		{
			// remaining previously invisible node queries
			IssueEntityMultiOcclusionQueries( &invisibleQueue, &occlusionQueryQueue );

			//ri.Printf(PRINT_ALL, "occlusionQueryQueue.empty() = %i\n", QueueEmpty(&occlusionQueryQueue));
		}

		// go back to the world modelview matrix
		backEnd.orientation = backEnd.viewParms.world;
		GL_LoadModelViewMatrix( backEnd.viewParms.world.modelViewMatrix );

		while ( !QueueEmpty( &occlusionQueryQueue ) )
		{
			if ( EntityOcclusionResultAvailable( ( trRefEntity_t * ) QueueFront( &occlusionQueryQueue )->data ) )
			{
				entity = ( trRefEntity_t * ) DeQueue( &occlusionQueryQueue );

				// wait if result not available
				GetEntityOcclusionQueryResult( entity );

				if ( ( signed ) entity->occlusionQuerySamples > r_chcVisibilityThreshold->integer )
				{
					// if a query of multiple previously invisible objects became visible, we need to
					// test all the individual objects ...
					if ( !QueueEmpty( &entity->multiQuery ) )
					{
						multiQueryEntity = entity;

						IssueEntityOcclusionQuery( &occlusionQueryQueue, multiQueryEntity, qfalse );

						while ( !QueueEmpty( &multiQueryEntity->multiQuery ) )
						{
							entity = ( trRefEntity_t * ) DeQueue( &multiQueryEntity->multiQuery );

							IssueEntityOcclusionQuery( &occlusionQueryQueue, entity, qtrue );
						}
					}
				}
				else
				{
					if ( !QueueEmpty( &entity->multiQuery ) )
					{
						backEnd.pc.c_occlusionQueriesEntitiesCulled++;

						multiQueryEntity = entity;

						while ( !QueueEmpty( &multiQueryEntity->multiQuery ) )
						{
							entity = ( trRefEntity_t * ) DeQueue( &multiQueryEntity->multiQuery );

							backEnd.pc.c_occlusionQueriesEntitiesCulled++;
							backEnd.pc.c_occlusionQueriesSaved++;
						}
					}
					else
					{
						backEnd.pc.c_occlusionQueriesEntitiesCulled++;
					}
				}
			}
		}

		if ( r_speeds->integer == RSPEEDS_OCCLUSION_QUERIES )
		{
			glFinish();
			endTime = ri.Milliseconds();
			backEnd.pc.c_occlusionQueriesResponseTime = endTime - startTime;

			startTime = ri.Milliseconds();
		}

		// go back to the world modelview matrix
		backEnd.orientation = backEnd.viewParms.world;
		GL_LoadModelViewMatrix( backEnd.viewParms.world.modelViewMatrix );

		// reenable writes to depth and color buffers
		GL_State( GLS_DEPTHMASK_TRUE );
	}

	GL_CheckErrors();
}

// ================================================================================================
//
// BSP OCCLUSION CULLING
//
// ================================================================================================

#if 0
void RB_RenderBspOcclusionQueries()
{
	GLimp_LogComment( "--- RB_RenderBspOcclusionQueries ---\n" );

	if ( glConfig2.occlusionQueryBits && glConfig.driverType != GLDRV_MESA && r_dynamicBspOcclusionCulling->integer )
	{
		//int             j;
		bspNode_t *node;
		link_t    *l, *next, *sentinel;

		gl_genericShader->BindProgram();

		GL_Cull( CT_TWO_SIDED );

		GL_LoadProjectionMatrix( backEnd.viewParms.projectionMatrix );

		// set uniforms
		gl_genericShader->SetTCGenEnvironment( qfalse );
		gl_genericShader->SetUniform_ColorModulate( CGEN_VERTEX, AGEN_VERTEX );
		gl_genericShader->SetVertexSkinning( qfalse );

		gl_genericShader->SetUniform_AlphaTest( DGEN_NONE );
		gl_genericShader->SetUniform_AlphaTest( 0 );

		// set up the transformation matrix
		backEnd.orientation = backEnd.viewParms.world;
		GL_LoadModelViewMatrix( backEnd.orientation.modelViewMatrix );
		gl_genericShader->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );

		// bind u_ColorMap
		GL_SelectTexture( 0 );
		GL_Bind( tr.whiteImage );
		gl_genericShader->SetUniform_ColorTextureMatrix( matrixIdentity );

		// don't write to the color buffer or depth buffer
		GL_State( GLS_COLORMASK_BITS );

		sentinel = &tr.occlusionQueryList;

		for ( l = sentinel->next; l != sentinel; l = next )
		{
			next = l->next;
			node = ( bspNode_t * ) l->data;

			// begin the occlusion query
			glBeginQuery( GL_SAMPLES_PASSED, node->occlusionQueryObjects[ backEnd.viewParms.viewCount ] );

			R_BindVBO( node->volumeVBO );
			R_BindIBO( node->volumeIBO );

			GL_VertexAttribsState( ATTR_POSITION );

			tess.numVertexes = node->volumeVerts;
			tess.numIndexes = node->volumeIndexes;

			Tess_DrawElements();

			// end the query
			// don't read back immediately so that we give the query time to be ready
			glEndQuery( GL_SAMPLES_PASSED );

#if 0

			if ( !glIsQuery( node->occlusionQueryObjects[ backEnd.viewParms.viewCount ] ) )
			{
				ri.Error( ERR_FATAL, "node %i has no occlusion query object in slot %i: %i", j, 0, node->occlusionQueryObjects[ backEnd.viewParms.viewCount ] );
			}

#endif

			backEnd.pc.c_occlusionQueries++;

			tess.multiDrawPrimitives = 0;
			tess.numIndexes = 0;
			tess.numVertexes = 0;
		}
	}

	GL_CheckErrors();
}

void RB_CollectBspOcclusionQueries()
{
	GLimp_LogComment( "--- RB_CollectBspOcclusionQueries ---\n" );

	if ( glConfig2.occlusionQueryBits && glConfig.driverType != GLDRV_MESA && r_dynamicBspOcclusionCulling->integer )
	{
		//int             j;
		bspNode_t *node;
		link_t    *l, *next, *sentinel;

		int       ocCount;
		int       avCount;
		GLint     available;

		glFinish();

		ocCount = 0;
		sentinel = &tr.occlusionQueryList;

		for ( l = sentinel->next; l != sentinel; l = l->next )
		{
			node = ( bspNode_t * ) l->data;

			if ( glIsQuery( node->occlusionQueryObjects[ backEnd.viewParms.viewCount ] ) )
			{
				ocCount++;
			}
		}

		//ri.Printf(PRINT_ALL, "waiting for %i queries...\n", ocCount);

		avCount = 0;

		do
		{
			for ( l = sentinel->next; l != sentinel; l = l->next )
			{
				node = ( bspNode_t * ) l->data;

				if ( node->issueOcclusionQuery )
				{
					available = 0;

					if ( glIsQuery( node->occlusionQueryObjects[ backEnd.viewParms.viewCount ] ) )
					{
						glGetQueryObjectiv( node->occlusionQueryObjects[ backEnd.viewParms.viewCount ], GL_QUERY_RESULT_AVAILABLE, &available );
						GL_CheckErrors();
					}

					if ( available )
					{
						node->issueOcclusionQuery = qfalse;
						avCount++;

						//if(//avCount % oc)

						//ri.Printf(PRINT_ALL, "%i queries...\n", avCount);
					}
				}
			}
		}
		while ( avCount < ocCount );

		for ( l = sentinel->next; l != sentinel; l = l->next )
		{
			node = ( bspNode_t * ) l->data;

			available = 0;

			if ( glIsQuery( node->occlusionQueryObjects[ backEnd.viewParms.viewCount ] ) )
			{
				glGetQueryObjectiv( node->occlusionQueryObjects[ backEnd.viewParms.viewCount ], GL_QUERY_RESULT_AVAILABLE, &available );
				GL_CheckErrors();
			}

			if ( available )
			{
				backEnd.pc.c_occlusionQueriesAvailable++;

				// get the object and store it in the occlusion bits for the light
				glGetQueryObjectiv( node->occlusionQueryObjects[ backEnd.viewParms.viewCount ], GL_QUERY_RESULT, &node->occlusionQuerySamples[ backEnd.viewParms.viewCount ] );

				if ( node->occlusionQuerySamples[ backEnd.viewParms.viewCount ] <= 0 )
				{
					backEnd.pc.c_occlusionQueriesLeafsCulled++;
				}
			}
			else
			{
				node->occlusionQuerySamples[ backEnd.viewParms.viewCount ] = 1;
			}

			GL_CheckErrors();
		}

		//ri.Printf(PRINT_ALL, "done\n");
	}
}

#endif

static void RB_RenderDebugUtils()
{
	GLimp_LogComment( "--- RB_RenderDebugUtils ---\n" );

	if ( r_showLightTransforms->integer || r_showShadowLod->integer )
	{
		interaction_t *ia;
		int           iaCount, j;
		trRefLight_t  *light;
		vec3_t        forward, left, up;
		vec4_t        lightColor; //unused
		vec4_t        quadVerts[ 4 ];

		static const vec3_t minSize = { -2, -2, -2 };
		static const vec3_t maxSize = { 2,  2,  2 };

		gl_genericShader->DisableAlphaTesting();
		gl_genericShader->DisablePortalClipping();
		gl_genericShader->DisableVertexSkinning();
		gl_genericShader->DisableVertexAnimation();
		gl_genericShader->DisableDeformVertexes();
		gl_genericShader->DisableTCGenEnvironment();

		gl_genericShader->BindProgram();

		//GL_State(GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
		GL_State( GLS_POLYMODE_LINE | GLS_DEPTHTEST_DISABLE );
		GL_Cull( CT_TWO_SIDED );

		// set uniforms
		gl_genericShader->SetUniform_ColorModulate( CGEN_CUSTOM_RGB, AGEN_CUSTOM );

		// bind u_ColorMap
		GL_SelectTexture( 0 );
		GL_Bind( tr.whiteImage );
		gl_genericShader->SetUniform_ColorTextureMatrix( matrixIdentity );

		for ( iaCount = 0, ia = &backEnd.viewParms.interactions[ 0 ]; iaCount < backEnd.viewParms.numInteractions; )
		{
			light = ia->light;

			if ( !ia->next )
			{
				if ( r_showShadowLod->integer )
				{
					if ( light->shadowLOD == 0 )
					{
						Vector4Copy( colorRed, lightColor );
					}
					else if ( light->shadowLOD == 1 )
					{
						Vector4Copy( colorGreen, lightColor );
					}
					else if ( light->shadowLOD == 2 )
					{
						Vector4Copy( colorBlue, lightColor );
					}
					else if ( light->shadowLOD == 3 )
					{
						Vector4Copy( colorYellow, lightColor );
					}
					else if ( light->shadowLOD == 4 )
					{
						Vector4Copy( colorMagenta, lightColor );
					}
					else if ( light->shadowLOD == 5 )
					{
						Vector4Copy( colorCyan, lightColor );
					}
					else
					{
						Vector4Copy( colorMdGrey, lightColor );
					}
				}
				else if ( r_dynamicLightOcclusionCulling->integer )
				{
					if ( !ia->occlusionQuerySamples )
					{
						Vector4Copy( colorRed, lightColor );
					}
					else
					{
						Vector4Copy( colorGreen, lightColor );
					}
				}
				else
				{
					//Vector4Copy(g_color_table[iaCount % 8], lightColor);
					Vector4Copy( colorBlue, lightColor );
				}

				lightColor[ 3 ] = 0.2;

				gl_genericShader->SetUniform_Color( lightColor );

				MatrixToVectorsFLU( matrixIdentity, forward, left, up );
				VectorMA( vec3_origin, 16, forward, forward );
				VectorMA( vec3_origin, 16, left, left );
				VectorMA( vec3_origin, 16, up, up );

				/*
				// draw axis
				glBegin(GL_LINES);

				// draw orientation
				glVertexAttrib4fv(ATTR_INDEX_COLOR, colorRed);
				glVertex3fv(vec3_origin);
				glVertex3fv(forward);

				glVertexAttrib4fv(ATTR_INDEX_COLOR, colorGreen);
				glVertex3fv(vec3_origin);
				glVertex3fv(left);

				glVertexAttrib4fv(ATTR_INDEX_COLOR, colorBlue);
				glVertex3fv(vec3_origin);
				glVertex3fv(up);

				// draw special vectors
				glVertexAttrib4fv(ATTR_INDEX_COLOR, colorYellow);
				glVertex3fv(vec3_origin);
				VectorSubtract(light->origin, backEnd.orientation.origin, tmp);
				light->transformed[0] = DotProduct(tmp, backEnd.orientation.axis[0]);
				light->transformed[1] = DotProduct(tmp, backEnd.orientation.axis[1]);
				light->transformed[2] = DotProduct(tmp, backEnd.orientation.axis[2]);
				glVertex3fv(light->transformed);

				glEnd();
				*/

#if 1

				if ( light->isStatic && light->frustumVBO && light->frustumIBO )
				{
					// go back to the world modelview matrix
					backEnd.orientation = backEnd.viewParms.world;
					GL_LoadModelViewMatrix( backEnd.viewParms.world.modelViewMatrix );
					gl_genericShader->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );

					R_BindVBO( light->frustumVBO );
					R_BindIBO( light->frustumIBO );

					GL_VertexAttribsState( ATTR_POSITION );

					tess.numVertexes = light->frustumVerts;
					tess.numIndexes = light->frustumIndexes;

					Tess_DrawElements();

					tess.multiDrawPrimitives = 0;
					tess.numIndexes = 0;
					tess.numVertexes = 0;
				}
				else
#endif
				{
					tess.multiDrawPrimitives = 0;
					tess.numIndexes = 0;
					tess.numVertexes = 0;

					switch ( light->l.rlType )
					{
						case RL_OMNI:
						case RL_DIRECTIONAL:
							{
#if 1
								// set up the transformation matrix
								R_RotateLightForViewParms( light, &backEnd.viewParms, &backEnd.orientation );

								GL_LoadModelViewMatrix( backEnd.orientation.modelViewMatrix );
								gl_genericShader->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );

								Tess_AddCube( vec3_origin, light->localBounds[ 0 ], light->localBounds[ 1 ], lightColor );

								if ( !VectorCompare( light->l.center, vec3_origin ) )
								{
									Tess_AddCube( light->l.center, minSize, maxSize, colorYellow );
								}

								Tess_UpdateVBOs( ATTR_POSITION | ATTR_COLOR );
								Tess_DrawElements();
#else
								matrix_t transform, scale, rot;

								MatrixSetupScale( scale, light->l.radius[ 0 ], light->l.radius[ 1 ], light->l.radius[ 2 ] );
								MatrixMultiply( light->transformMatrix, scale, transform );

								GL_LoadModelViewMatrix( transform );
								//GL_LoadProjectionMatrix(backEnd.viewParms.projectionMatrix);

								gl_genericShader->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );

								R_BindVBO( tr.unitCubeVBO );
								R_BindIBO( tr.unitCubeIBO );

								GL_VertexAttribsState( ATTR_POSITION );

								tess.multiDrawPrimitives = 0;
								tess.numVertexes = tr.unitCubeVBO->vertexesNum;
								tess.numIndexes = tr.unitCubeIBO->indexesNum;

								Tess_DrawElements();
#endif
								break;
							}

						case RL_PROJ:
							{
								vec3_t farCorners[ 4 ];
								//vec4_t      frustum[6];
								vec4_t *frustum = light->localFrustum;

								// set up the transformation matrix
								R_RotateLightForViewParms( light, &backEnd.viewParms, &backEnd.orientation );
								GL_LoadModelViewMatrix( backEnd.orientation.modelViewMatrix );
								gl_genericShader->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );

#if 0

								// transform frustum from world space to local space
								for ( j = 0; j < 6; j++ )
								{
									MatrixTransformPlane( light->transformMatrix, light->localFrustum[ j ], frustum[ j ] );
									//Vector4Copy(light->localFrustum[j], frustum[j]);
									//MatrixTransformPlane2(light->viewMatrix, frustum[j]);
								}

								// go back to the world modelview matrix
								backEnd.orientation = backEnd.viewParms.world;
								GL_LoadModelViewMatrix( backEnd.viewParms.world.modelViewMatrix );
								GLSL_SetUniform_ModelViewProjectionMatrix( &tr.genericShader, glState.modelViewProjectionMatrix[ glState.stackIndex ] );
#endif

								PlanesGetIntersectionPoint( frustum[ FRUSTUM_LEFT ], frustum[ FRUSTUM_TOP ], frustum[ FRUSTUM_FAR ], farCorners[ 0 ] );
								PlanesGetIntersectionPoint( frustum[ FRUSTUM_RIGHT ], frustum[ FRUSTUM_TOP ], frustum[ FRUSTUM_FAR ], farCorners[ 1 ] );
								PlanesGetIntersectionPoint( frustum[ FRUSTUM_RIGHT ], frustum[ FRUSTUM_BOTTOM ], frustum[ FRUSTUM_FAR ], farCorners[ 2 ] );
								PlanesGetIntersectionPoint( frustum[ FRUSTUM_LEFT ], frustum[ FRUSTUM_BOTTOM ], frustum[ FRUSTUM_FAR ], farCorners[ 3 ] );

								// the planes of the frustum are measured at world 0,0,0 so we have to position the intersection points relative to the light origin
#if 0
								ri.Printf( PRINT_ALL, "pyramid farCorners\n" );

								for ( j = 0; j < 4; j++ )
								{
									ri.Printf( PRINT_ALL, "(%5.3f, %5.3f, %5.3f)\n", farCorners[ j ][ 0 ], farCorners[ j ][ 1 ], farCorners[ j ][ 2 ] );
								}

#endif

								tess.numVertexes = 0;
								tess.numIndexes = 0;
								tess.multiDrawPrimitives = 0;

								if ( !VectorCompare( light->l.projStart, vec3_origin ) )
								{
									vec3_t nearCorners[ 4 ];

									// calculate the vertices defining the top area
									PlanesGetIntersectionPoint( frustum[ FRUSTUM_LEFT ], frustum[ FRUSTUM_TOP ], frustum[ FRUSTUM_NEAR ], nearCorners[ 0 ] );
									PlanesGetIntersectionPoint( frustum[ FRUSTUM_RIGHT ], frustum[ FRUSTUM_TOP ], frustum[ FRUSTUM_NEAR ], nearCorners[ 1 ] );
									PlanesGetIntersectionPoint( frustum[ FRUSTUM_RIGHT ], frustum[ FRUSTUM_BOTTOM ], frustum[ FRUSTUM_NEAR ], nearCorners[ 2 ] );
									PlanesGetIntersectionPoint( frustum[ FRUSTUM_LEFT ], frustum[ FRUSTUM_BOTTOM ], frustum[ FRUSTUM_NEAR ], nearCorners[ 3 ] );

#if 0
									ri.Printf( PRINT_ALL, "pyramid nearCorners\n" );

									for ( j = 0; j < 4; j++ )
									{
										ri.Printf( PRINT_ALL, "(%5.3f, %5.3f, %5.3f)\n", nearCorners[ j ][ 0 ], nearCorners[ j ][ 1 ], nearCorners[ j ][ 2 ] );
									}

#endif

									// draw outer surfaces
									for ( j = 0; j < 4; j++ )
									{
										Vector4Set( quadVerts[ 3 ], nearCorners[ j ][ 0 ], nearCorners[ j ][ 1 ], nearCorners[ j ][ 2 ], 1 );
										Vector4Set( quadVerts[ 2 ], farCorners[ j ][ 0 ], farCorners[ j ][ 1 ], farCorners[ j ][ 2 ], 1 );
										Vector4Set( quadVerts[ 1 ], farCorners[( j + 1 ) % 4 ][ 0 ], farCorners[( j + 1 ) % 4 ][ 1 ], farCorners[( j + 1 ) % 4 ][ 2 ], 1 );
										Vector4Set( quadVerts[ 0 ], nearCorners[( j + 1 ) % 4 ][ 0 ], nearCorners[( j + 1 ) % 4 ][ 1 ], nearCorners[( j + 1 ) % 4 ][ 2 ], 1 );
										Tess_AddQuadStamp2( quadVerts, lightColor );
									}

									// draw far cap
									Vector4Set( quadVerts[ 0 ], farCorners[ 0 ][ 0 ], farCorners[ 0 ][ 1 ], farCorners[ 0 ][ 2 ], 1 );
									Vector4Set( quadVerts[ 1 ], farCorners[ 1 ][ 0 ], farCorners[ 1 ][ 1 ], farCorners[ 1 ][ 2 ], 1 );
									Vector4Set( quadVerts[ 2 ], farCorners[ 2 ][ 0 ], farCorners[ 2 ][ 1 ], farCorners[ 2 ][ 2 ], 1 );
									Vector4Set( quadVerts[ 3 ], farCorners[ 3 ][ 0 ], farCorners[ 3 ][ 1 ], farCorners[ 3 ][ 2 ], 1 );
									Tess_AddQuadStamp2( quadVerts, lightColor );

									// draw near cap
									Vector4Set( quadVerts[ 3 ], nearCorners[ 0 ][ 0 ], nearCorners[ 0 ][ 1 ], nearCorners[ 0 ][ 2 ], 1 );
									Vector4Set( quadVerts[ 2 ], nearCorners[ 1 ][ 0 ], nearCorners[ 1 ][ 1 ], nearCorners[ 1 ][ 2 ], 1 );
									Vector4Set( quadVerts[ 1 ], nearCorners[ 2 ][ 0 ], nearCorners[ 2 ][ 1 ], nearCorners[ 2 ][ 2 ], 1 );
									Vector4Set( quadVerts[ 0 ], nearCorners[ 3 ][ 0 ], nearCorners[ 3 ][ 1 ], nearCorners[ 3 ][ 2 ], 1 );
									Tess_AddQuadStamp2( quadVerts, lightColor );
								}
								else
								{
									vec3_t top;

									// no light_start, just use the top vertex (doesn't need to be mirrored)
									PlanesGetIntersectionPoint( frustum[ FRUSTUM_LEFT ], frustum[ FRUSTUM_RIGHT ], frustum[ FRUSTUM_TOP ], top );

									// draw pyramid
									for ( j = 0; j < 4; j++ )
									{
										VectorCopy( top, tess.xyz[ tess.numVertexes ] );
										Vector4Copy( lightColor, tess.colors[ tess.numVertexes ] );
										tess.indexes[ tess.numIndexes++ ] = tess.numVertexes;
										tess.numVertexes++;

										VectorCopy( farCorners[( j + 1 ) % 4 ], tess.xyz[ tess.numVertexes ] );
										Vector4Copy( lightColor, tess.colors[ tess.numVertexes ] );
										tess.indexes[ tess.numIndexes++ ] = tess.numVertexes;
										tess.numVertexes++;

										VectorCopy( farCorners[ j ], tess.xyz[ tess.numVertexes ] );
										Vector4Copy( lightColor, tess.colors[ tess.numVertexes ] );
										tess.indexes[ tess.numIndexes++ ] = tess.numVertexes;
										tess.numVertexes++;
									}

									// draw far cap
									Vector4Set( quadVerts[ 0 ], farCorners[ 0 ][ 0 ], farCorners[ 0 ][ 1 ], farCorners[ 0 ][ 2 ], 1 );
									Vector4Set( quadVerts[ 1 ], farCorners[ 1 ][ 0 ], farCorners[ 1 ][ 1 ], farCorners[ 1 ][ 2 ], 1 );
									Vector4Set( quadVerts[ 2 ], farCorners[ 2 ][ 0 ], farCorners[ 2 ][ 1 ], farCorners[ 2 ][ 2 ], 1 );
									Vector4Set( quadVerts[ 3 ], farCorners[ 3 ][ 0 ], farCorners[ 3 ][ 1 ], farCorners[ 3 ][ 2 ], 1 );
									Tess_AddQuadStamp2( quadVerts, lightColor );
								}

								// draw light_target
								Tess_AddCube( light->l.projTarget, minSize, maxSize, colorRed );
								Tess_AddCube( light->l.projRight, minSize, maxSize, colorGreen );
								Tess_AddCube( light->l.projUp, minSize, maxSize, colorBlue );

								if ( !VectorCompare( light->l.projStart, vec3_origin ) )
								{
									Tess_AddCube( light->l.projStart, minSize, maxSize, colorYellow );
								}

								if ( !VectorCompare( light->l.projEnd, vec3_origin ) )
								{
									Tess_AddCube( light->l.projEnd, minSize, maxSize, colorMagenta );
								}

								Tess_UpdateVBOs( ATTR_POSITION | ATTR_COLOR );
								Tess_DrawElements();
								break;
							}

						default:
							break;
					}

					tess.multiDrawPrimitives = 0;
					tess.numIndexes = 0;
					tess.numVertexes = 0;
				}

				if ( iaCount < ( backEnd.viewParms.numInteractions - 1 ) )
				{
					// jump to next interaction and continue
					ia++;
					iaCount++;
				}
				else
				{
					// increase last time to leave for loop
					iaCount++;
				}
			}
			else
			{
				// just continue
				ia = ia->next;
				iaCount++;
			}
		}

		// go back to the world modelview matrix
		backEnd.orientation = backEnd.viewParms.world;
		GL_LoadModelViewMatrix( backEnd.viewParms.world.modelViewMatrix );
	}

	if ( r_showLightInteractions->integer )
	{
		int           i;
		int           cubeSides;
		interaction_t *ia;
		int           iaCount;
		trRefLight_t  *light;
		trRefEntity_t *entity;
		surfaceType_t *surface;
		vec4_t        lightColor;

		static const vec3_t mins = { -1, -1, -1 };
		static const vec3_t maxs = { 1, 1, 1 };

		gl_genericShader->DisableAlphaTesting();
		gl_genericShader->DisablePortalClipping();
		gl_genericShader->DisableVertexSkinning();
		gl_genericShader->DisableVertexAnimation();
		gl_genericShader->DisableDeformVertexes();
		gl_genericShader->DisableTCGenEnvironment();

		gl_genericShader->BindProgram();

		GL_State( GLS_POLYMODE_LINE | GLS_DEPTHTEST_DISABLE );
		GL_Cull( CT_TWO_SIDED );

		// set uniforms
		gl_genericShader->SetUniform_ColorModulate( CGEN_VERTEX, AGEN_VERTEX );
		gl_genericShader->SetUniform_Color( colorBlack );

		// bind u_ColorMap
		GL_SelectTexture( 0 );
		GL_Bind( tr.whiteImage );
		gl_genericShader->SetUniform_ColorTextureMatrix( matrixIdentity );

		for ( iaCount = 0, ia = &backEnd.viewParms.interactions[ 0 ]; iaCount < backEnd.viewParms.numInteractions; )
		{
			backEnd.currentEntity = entity = ia->entity;
			light = ia->light;
			surface = ia->surface;

			if ( entity != &tr.worldEntity )
			{
				// set up the transformation matrix
				R_RotateEntityForViewParms( backEnd.currentEntity, &backEnd.viewParms, &backEnd.orientation );
			}
			else
			{
				backEnd.orientation = backEnd.viewParms.world;
			}

			GL_LoadModelViewMatrix( backEnd.orientation.modelViewMatrix );
			gl_genericShader->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );

			if ( r_shadows->integer >= SHADOWING_ESM16 && light->l.rlType == RL_OMNI )
			{
#if 0
				Vector4Copy( colorMdGrey, lightColor );

				if ( ia->cubeSideBits & CUBESIDE_PX )
				{
					Vector4Copy( colorBlack, lightColor );
				}

				if ( ia->cubeSideBits & CUBESIDE_PY )
				{
					Vector4Copy( colorRed, lightColor );
				}

				if ( ia->cubeSideBits & CUBESIDE_PZ )
				{
					Vector4Copy( colorGreen, lightColor );
				}

				if ( ia->cubeSideBits & CUBESIDE_NX )
				{
					Vector4Copy( colorYellow, lightColor );
				}

				if ( ia->cubeSideBits & CUBESIDE_NY )
				{
					Vector4Copy( colorBlue, lightColor );
				}

				if ( ia->cubeSideBits & CUBESIDE_NZ )
				{
					Vector4Copy( colorCyan, lightColor );
				}

				if ( ia->cubeSideBits == CUBESIDE_CLIPALL )
				{
					Vector4Copy( colorMagenta, lightColor );
				}

#else
				// count how many cube sides are in use for this interaction
				cubeSides = 0;

				for ( i = 0; i < 6; i++ )
				{
					if ( ia->cubeSideBits & ( 1 << i ) )
					{
						cubeSides++;
					}
				}

				Vector4Copy( g_color_table[ cubeSides ], lightColor );
#endif
			}
			else
			{
				Vector4Copy( colorMdGrey, lightColor );
			}

			lightColor[ 0 ] *= 0.5f;
			lightColor[ 1 ] *= 0.5f;
			lightColor[ 2 ] *= 0.5f;
			//lightColor[3] *= 0.2f;

			Vector4Copy( colorWhite, lightColor );

			tess.numVertexes = 0;
			tess.numIndexes = 0;
			tess.multiDrawPrimitives = 0;

			if ( *surface == SF_FACE || *surface == SF_GRID || *surface == SF_TRIANGLES )
			{
				srfGeneric_t *gen;

				gen = ( srfGeneric_t * ) surface;

				if ( *surface == SF_FACE )
				{
					Vector4Copy( colorMdGrey, lightColor );
				}
				else if ( *surface == SF_GRID )
				{
					Vector4Copy( colorCyan, lightColor );
				}
				else if ( *surface == SF_TRIANGLES )
				{
					Vector4Copy( colorMagenta, lightColor );
				}
				else
				{
					Vector4Copy( colorMdGrey, lightColor );
				}

				Tess_AddCube( vec3_origin, gen->bounds[ 0 ], gen->bounds[ 1 ], lightColor );

				Tess_AddCube( gen->origin, mins, maxs, colorWhite );
			}
			else if ( *surface == SF_VBO_MESH )
			{
				srfVBOMesh_t *srf = ( srfVBOMesh_t * ) surface;
				Tess_AddCube( vec3_origin, srf->bounds[ 0 ], srf->bounds[ 1 ], lightColor );
			}
			else if ( *surface == SF_MDV )
			{
				Tess_AddCube( vec3_origin, entity->localBounds[ 0 ], entity->localBounds[ 1 ], lightColor );
			}

			Tess_UpdateVBOs( ATTR_POSITION | ATTR_COLOR );
			Tess_DrawElements();

			tess.multiDrawPrimitives = 0;
			tess.numIndexes = 0;
			tess.numVertexes = 0;

			if ( !ia->next )
			{
				if ( iaCount < ( backEnd.viewParms.numInteractions - 1 ) )
				{
					// jump to next interaction and continue
					ia++;
					iaCount++;
				}
				else
				{
					// increase last time to leave for loop
					iaCount++;
				}
			}
			else
			{
				// just continue
				ia = ia->next;
				iaCount++;
			}
		}

		// go back to the world modelview matrix
		backEnd.orientation = backEnd.viewParms.world;
		GL_LoadModelViewMatrix( backEnd.viewParms.world.modelViewMatrix );
	}

	if ( r_showEntityTransforms->integer )
	{
		trRefEntity_t *ent;
		int           i;
		static const vec3_t mins = { -1, -1, -1 };
		static const vec3_t maxs = { 1, 1, 1 };

		gl_genericShader->DisableAlphaTesting();
		gl_genericShader->DisablePortalClipping();
		gl_genericShader->DisableVertexSkinning();
		gl_genericShader->DisableVertexAnimation();
		gl_genericShader->DisableDeformVertexes();
		gl_genericShader->DisableTCGenEnvironment();

		gl_genericShader->BindProgram();

		GL_State( GLS_POLYMODE_LINE | GLS_DEPTHTEST_DISABLE );
		GL_Cull( CT_TWO_SIDED );

		// set uniforms
		gl_genericShader->SetUniform_ColorModulate( CGEN_VERTEX, AGEN_VERTEX );
		gl_genericShader->SetUniform_Color( colorBlack );

		// bind u_ColorMap
		GL_SelectTexture( 0 );
		GL_Bind( tr.whiteImage );
		gl_genericShader->SetUniform_ColorTextureMatrix( matrixIdentity );

		ent = backEnd.refdef.entities;

		for ( i = 0; i < backEnd.refdef.numEntities; i++, ent++ )
		{
			if ( ( ent->e.renderfx & RF_THIRD_PERSON ) && !backEnd.viewParms.isPortal )
			{
				continue;
			}

			if ( ent->cull == CULL_OUT )
			{
				continue;
			}

			// set up the transformation matrix
			R_RotateEntityForViewParms( ent, &backEnd.viewParms, &backEnd.orientation );
			GL_LoadModelViewMatrix( backEnd.orientation.modelViewMatrix );
			gl_genericShader->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );

			//R_DebugAxis(vec3_origin, matrixIdentity);
			//R_DebugBoundingBox(vec3_origin, ent->localBounds[0], ent->localBounds[1], colorMagenta);

			tess.multiDrawPrimitives = 0;
			tess.numIndexes = 0;
			tess.numVertexes = 0;

			if ( r_dynamicEntityOcclusionCulling->integer )
			{
				if ( !ent->occlusionQuerySamples )
				{
					Tess_AddCube( vec3_origin, ent->localBounds[ 0 ], ent->localBounds[ 1 ], colorRed );
				}
				else
				{
					Tess_AddCube( vec3_origin, ent->localBounds[ 0 ], ent->localBounds[ 1 ], colorGreen );
				}
			}
			else
			{
				Tess_AddCube( vec3_origin, ent->localBounds[ 0 ], ent->localBounds[ 1 ], colorBlue );
			}

			Tess_AddCube( vec3_origin, mins, maxs, colorWhite );

			Tess_UpdateVBOs( ATTR_POSITION | ATTR_COLOR );
			Tess_DrawElements();

			tess.multiDrawPrimitives = 0;
			tess.numIndexes = 0;
			tess.numVertexes = 0;

			// go back to the world modelview matrix
			//backEnd.orientation = backEnd.viewParms.world;
			//GL_LoadModelViewMatrix(backEnd.viewParms.world.modelViewMatrix);

			//R_DebugBoundingBox(vec3_origin, ent->worldBounds[0], ent->worldBounds[1], colorCyan);
		}

		// go back to the world modelview matrix
		backEnd.orientation = backEnd.viewParms.world;
		GL_LoadModelViewMatrix( backEnd.viewParms.world.modelViewMatrix );
	}

#if defined( USE_REFENTITY_ANIMATIONSYSTEM )

	if ( r_showSkeleton->integer )
	{
		int                  i, j, k, parentIndex;
		trRefEntity_t        *ent;
		vec3_t               origin, offset;
		vec3_t               forward, right, up;
		vec3_t               diff, tmp, tmp2, tmp3;
		vec_t                length;
		vec4_t               tetraVerts[ 4 ];
		static refSkeleton_t skeleton;
		refSkeleton_t        *skel;

		gl_genericShader->DisableAlphaTesting();
		gl_genericShader->DisablePortalClipping();
		gl_genericShader->DisableVertexSkinning();
		gl_genericShader->DisableVertexAnimation();
		gl_genericShader->DisableDeformVertexes();
		gl_genericShader->DisableTCGenEnvironment();

		gl_genericShader->BindProgram();

		GL_Cull( CT_TWO_SIDED );

		// set uniforms
		gl_genericShader->SetUniform_ColorModulate( CGEN_VERTEX, AGEN_VERTEX );
		gl_genericShader->SetUniform_Color( colorBlack );

		// bind u_ColorMap
		GL_SelectTexture( 0 );
		GL_Bind( tr.charsetImage );
		gl_genericShader->SetUniform_ColorTextureMatrix( matrixIdentity );

		ent = backEnd.refdef.entities;

		for ( i = 0; i < backEnd.refdef.numEntities; i++, ent++ )
		{
			if ( ( ent->e.renderfx & RF_THIRD_PERSON ) && !backEnd.viewParms.isPortal )
			{
				continue;
			}

			// set up the transformation matrix
			R_RotateEntityForViewParms( ent, &backEnd.viewParms, &backEnd.orientation );
			GL_LoadModelViewMatrix( backEnd.orientation.modelViewMatrix );
			gl_genericShader->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );

			tess.multiDrawPrimitives = 0;
			tess.numVertexes = 0;
			tess.numIndexes = 0;

			skel = NULL;

			if ( ent->e.skeleton.type == SK_ABSOLUTE )
			{
				skel = &ent->e.skeleton;
			}
			else
			{
				model_t   *model;
				refBone_t *bone;

				model = R_GetModelByHandle( ent->e.hModel );

				if ( model )
				{
					switch ( model->type )
					{
						case MOD_MD5:
							{
								// copy absolute bones
								skeleton.numBones = model->md5->numBones;

								for ( j = 0, bone = &skeleton.bones[ 0 ]; j < skeleton.numBones; j++, bone++ )
								{
#if defined( REFBONE_NAMES )
									Q_strncpyz( bone->name, model->md5->bones[ j ].name, sizeof( bone->name ) );
#endif

									bone->parentIndex = model->md5->bones[ j ].parentIndex;
									VectorCopy( model->md5->bones[ j ].origin, bone->origin );
									VectorCopy( model->md5->bones[ j ].rotation, bone->rotation );
								}

								skel = &skeleton;
								break;
							}

						default:
							break;
					}
				}
			}

			if ( skel )
			{
				static vec3_t worldOrigins[ MAX_BONES ];

				GL_State( GLS_POLYMODE_LINE | GLS_DEPTHTEST_DISABLE );

				for ( j = 0; j < skel->numBones; j++ )
				{
					parentIndex = skel->bones[ j ].parentIndex;

					if ( parentIndex < 0 )
					{
						VectorClear( origin );
					}
					else
					{
						VectorCopy( skel->bones[ parentIndex ].origin, origin );
					}

					VectorCopy( skel->bones[ j ].origin, offset );
					QuatToVectorsFRU( skel->bones[ j ].rotation, forward, right, up );

					VectorSubtract( offset, origin, diff );

					if ( ( length = VectorNormalize( diff ) ) )
					{
						PerpendicularVector( tmp, diff );
						//VectorCopy(up, tmp);

						VectorScale( tmp, length * 0.1, tmp2 );
						VectorMA( tmp2, length * 0.2, diff, tmp2 );

						for ( k = 0; k < 3; k++ )
						{
							RotatePointAroundVector( tmp3, diff, tmp2, k * 120 );
							VectorAdd( tmp3, origin, tmp3 );
							VectorCopy( tmp3, tetraVerts[ k ] );
							tetraVerts[ k ][ 3 ] = 1;
						}

						VectorCopy( origin, tetraVerts[ 3 ] );
						tetraVerts[ 3 ][ 3 ] = 1;
						Tess_AddTetrahedron( tetraVerts, g_color_table[ ColorIndex( j ) ] );

						VectorCopy( offset, tetraVerts[ 3 ] );
						tetraVerts[ 3 ][ 3 ] = 1;
						Tess_AddTetrahedron( tetraVerts, g_color_table[ ColorIndex( j ) ] );
					}

					MatrixTransformPoint( backEnd.orientation.transformMatrix, skel->bones[ j ].origin, worldOrigins[ j ] );
				}

				Tess_UpdateVBOs( ATTR_POSITION | ATTR_TEXCOORD | ATTR_COLOR );

				Tess_DrawElements();

				tess.multiDrawPrimitives = 0;
				tess.numVertexes = 0;
				tess.numIndexes = 0;

#if defined( REFBONE_NAMES )
				{
					GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );

					// go back to the world modelview matrix
					backEnd.orientation = backEnd.viewParms.world;
					GL_LoadModelViewMatrix( backEnd.viewParms.world.modelViewMatrix );
					gl_genericShader->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );

					// draw names
					for ( j = 0; j < skel->numBones; j++ )
					{
						vec3_t left, up;
						float  radius;
						vec3_t origin;

						// calculate the xyz locations for the four corners
						radius = 0.4;
						VectorScale( backEnd.viewParms.orientation.axis[ 1 ], radius, left );
						VectorScale( backEnd.viewParms.orientation.axis[ 2 ], radius, up );

						if ( backEnd.viewParms.isMirror )
						{
							VectorSubtract( vec3_origin, left, left );
						}

						for ( k = 0; ( unsigned ) k < strlen( skel->bones[ j ].name ); k++ )
						{
							int   ch;
							int   row, col;
							float frow, fcol;
							float size;

							ch = skel->bones[ j ].name[ k ];
							ch &= 255;

							if ( ch == ' ' )
							{
								break;
							}

							row = ch >> 4;
							col = ch & 15;

							frow = row * 0.0625;
							fcol = col * 0.0625;
							size = 0.0625;

							VectorMA( worldOrigins[ j ], - ( k + 2.0f ), left, origin );
							Tess_AddQuadStampExt( origin, left, up, colorWhite, fcol, frow, fcol + size, frow + size );
						}

						Tess_UpdateVBOs( ATTR_POSITION | ATTR_TEXCOORD | ATTR_COLOR );

						Tess_DrawElements();

						tess.multiDrawPrimitives = 0;
						tess.numVertexes = 0;
						tess.numIndexes = 0;
					}
				}
#endif // REFBONE_NAMES
			}

			tess.multiDrawPrimitives = 0;
			tess.numVertexes = 0;
			tess.numIndexes = 0;
		}
	}

#endif

	if ( r_showLightScissors->integer )
	{
		interaction_t *ia;
		int           iaCount;
		matrix_t      ortho;
		vec4_t        quadVerts[ 4 ];

		gl_genericShader->DisableAlphaTesting();
		gl_genericShader->DisablePortalClipping();
		gl_genericShader->DisableVertexSkinning();
		gl_genericShader->DisableVertexAnimation();
		gl_genericShader->DisableDeformVertexes();
		gl_genericShader->DisableTCGenEnvironment();

		gl_genericShader->BindProgram();

		GL_State( GLS_POLYMODE_LINE | GLS_DEPTHTEST_DISABLE );
		GL_Cull( CT_TWO_SIDED );

		// set uniforms
		gl_genericShader->SetUniform_ColorModulate( CGEN_CUSTOM_RGB, AGEN_CUSTOM );

		// bind u_ColorMap
		GL_SelectTexture( 0 );
		GL_Bind( tr.whiteImage );
		gl_genericShader->SetUniform_ColorTextureMatrix( matrixIdentity );

		// set 2D virtual screen size
		GL_PushMatrix();
		MatrixOrthogonalProjection( ortho, backEnd.viewParms.viewportX,
		                            backEnd.viewParms.viewportX + backEnd.viewParms.viewportWidth,
		                            backEnd.viewParms.viewportY,
		                            backEnd.viewParms.viewportY + backEnd.viewParms.viewportHeight, -99999, 99999 );
		GL_LoadProjectionMatrix( ortho );
		GL_LoadModelViewMatrix( matrixIdentity );

		gl_genericShader->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );

		for ( iaCount = 0, ia = &backEnd.viewParms.interactions[ 0 ]; iaCount < backEnd.viewParms.numInteractions; )
		{
			if ( glConfig2.occlusionQueryBits && glConfig.driverType != GLDRV_MESA )
			{
				if ( !ia->occlusionQuerySamples )
				{
					gl_genericShader->SetUniform_Color( colorRed );
				}
				else
				{
					gl_genericShader->SetUniform_Color( colorGreen );
				}

				Vector4Set( quadVerts[ 0 ], ia->scissorX, ia->scissorY, 0, 1 );
				Vector4Set( quadVerts[ 1 ], ia->scissorX + ia->scissorWidth - 1, ia->scissorY, 0, 1 );
				Vector4Set( quadVerts[ 2 ], ia->scissorX + ia->scissorWidth - 1, ia->scissorY + ia->scissorHeight - 1, 0, 1 );
				Vector4Set( quadVerts[ 3 ], ia->scissorX, ia->scissorY + ia->scissorHeight - 1, 0, 1 );
				Tess_InstantQuad( quadVerts );
			}
			else
			{
				gl_genericShader->SetUniform_Color( colorWhite );

				Vector4Set( quadVerts[ 0 ], ia->scissorX, ia->scissorY, 0, 1 );
				Vector4Set( quadVerts[ 1 ], ia->scissorX + ia->scissorWidth - 1, ia->scissorY, 0, 1 );
				Vector4Set( quadVerts[ 2 ], ia->scissorX + ia->scissorWidth - 1, ia->scissorY + ia->scissorHeight - 1, 0, 1 );
				Vector4Set( quadVerts[ 3 ], ia->scissorX, ia->scissorY + ia->scissorHeight - 1, 0, 1 );
				Tess_InstantQuad( quadVerts );
			}

			if ( !ia->next )
			{
				if ( iaCount < ( backEnd.viewParms.numInteractions - 1 ) )
				{
					// jump to next interaction and continue
					ia++;
					iaCount++;
				}
				else
				{
					// increase last time to leave for loop
					iaCount++;
				}
			}
			else
			{
				// just continue
				ia = ia->next;
				iaCount++;
			}
		}

		GL_PopMatrix();
	}

	if ( r_showCubeProbes->integer )
	{
		cubemapProbe_t *cubeProbe;
		int            j;
//		vec4_t          quadVerts[4];
		static const vec3_t mins = { -8, -8, -8 };
		static const vec3_t maxs = { 8,  8,  8 };
		//vec3_t      viewOrigin;

		if ( backEnd.refdef.rdflags & ( RDF_NOWORLDMODEL | RDF_NOCUBEMAP ) )
		{
			return;
		}

		// choose right shader program ----------------------------------
		gl_reflectionShader->SetPortalClipping( backEnd.viewParms.isPortal );
		//  gl_reflectionShader->SetAlphaTesting((pStage->stateBits & GLS_ATEST_BITS) != 0);

		gl_reflectionShader->SetVertexSkinning( false );
		gl_reflectionShader->SetVertexAnimation( false );

		gl_reflectionShader->SetDeformVertexes( false );

		gl_reflectionShader->SetNormalMapping( false );
//		gl_reflectionShader->DisableMacro_TWOSIDED();

		gl_reflectionShader->BindProgram();

		// end choose right shader program ------------------------------

		gl_reflectionShader->SetUniform_ViewOrigin( backEnd.viewParms.orientation.origin );  // in world space

		GL_State( 0 );
		GL_Cull( CT_FRONT_SIDED );

		// set up the transformation matrix
		backEnd.orientation = backEnd.viewParms.world;
		GL_LoadModelViewMatrix( backEnd.orientation.modelViewMatrix );

		gl_reflectionShader->SetUniform_ModelMatrix( backEnd.orientation.transformMatrix );
		gl_reflectionShader->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );

		Tess_Begin( Tess_StageIteratorDebug, NULL, NULL, NULL, qtrue, qfalse, -1, 0 );

		for ( j = 0; j < tr.cubeProbes.currentElements; j++ )
		{
			cubeProbe = ( cubemapProbe_t * ) Com_GrowListElement( &tr.cubeProbes, j );

			// bind u_ColorMap
			GL_SelectTexture( 0 );
			GL_Bind( cubeProbe->cubemap );

			Tess_AddCubeWithNormals( cubeProbe->origin, mins, maxs, colorWhite );
		}

		Tess_End();

		{
			cubemapProbe_t *cubeProbeNearest;
			cubemapProbe_t *cubeProbeSecondNearest;

			gl_genericShader->DisableAlphaTesting();
			gl_genericShader->DisablePortalClipping();
			gl_genericShader->DisableVertexSkinning();
			gl_genericShader->DisableVertexAnimation();
			gl_genericShader->DisableDeformVertexes();
			gl_genericShader->DisableTCGenEnvironment();

			gl_genericShader->BindProgram();

			gl_genericShader->SetUniform_ColorModulate( CGEN_VERTEX, AGEN_VERTEX );
			gl_genericShader->SetUniform_Color( colorBlack );

			gl_genericShader->SetRequiredVertexPointers();

			GL_State( GLS_DEFAULT );
			GL_Cull( CT_TWO_SIDED );

			// set uniforms

			// set up the transformation matrix
			backEnd.orientation = backEnd.viewParms.world;
			GL_LoadModelViewMatrix( backEnd.orientation.modelViewMatrix );
			gl_genericShader->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );

			// bind u_ColorMap
			GL_SelectTexture( 0 );
			GL_Bind( tr.whiteImage );
			gl_genericShader->SetUniform_ColorTextureMatrix( matrixIdentity );

			GL_CheckErrors();

			R_FindTwoNearestCubeMaps( backEnd.viewParms.orientation.origin, &cubeProbeNearest, &cubeProbeSecondNearest );

			Tess_Begin( Tess_StageIteratorDebug, NULL, NULL, NULL, qtrue, qfalse, -1, 0 );

			if ( cubeProbeNearest == NULL && cubeProbeSecondNearest == NULL )
			{
				// bad
			}
			else if ( cubeProbeNearest == NULL )
			{
				Tess_AddCubeWithNormals( cubeProbeSecondNearest->origin, mins, maxs, colorBlue );
			}
			else if ( cubeProbeSecondNearest == NULL )
			{
				Tess_AddCubeWithNormals( cubeProbeNearest->origin, mins, maxs, colorYellow );
			}
			else
			{
				Tess_AddCubeWithNormals( cubeProbeNearest->origin, mins, maxs, colorGreen );
				Tess_AddCubeWithNormals( cubeProbeSecondNearest->origin, mins, maxs, colorRed );
			}

			Tess_End();
		}

		// go back to the world modelview matrix
		backEnd.orientation = backEnd.viewParms.world;
		GL_LoadModelViewMatrix( backEnd.viewParms.world.modelViewMatrix );
	}

	if ( r_showLightGrid->integer )
	{
		bspGridPoint_t *gridPoint;
		int            j, k;
		vec3_t         offset;
		vec3_t         lightDirection;
		vec3_t         tmp, tmp2, tmp3;
		vec_t          length;
		vec4_t         tetraVerts[ 4 ];

		if ( backEnd.refdef.rdflags & ( RDF_NOWORLDMODEL | RDF_NOCUBEMAP ) )
		{
			return;
		}

		GLimp_LogComment( "--- r_showLightGrid > 0: Rendering light grid\n" );

		gl_genericShader->DisableAlphaTesting();
		gl_genericShader->DisablePortalClipping();
		gl_genericShader->DisableVertexSkinning();
		gl_genericShader->DisableVertexAnimation();
		gl_genericShader->DisableDeformVertexes();
		gl_genericShader->DisableTCGenEnvironment();

		gl_genericShader->BindProgram();

		gl_genericShader->SetUniform_ColorModulate( CGEN_VERTEX, AGEN_VERTEX );
		gl_genericShader->SetUniform_Color( colorBlack );

		gl_genericShader->SetRequiredVertexPointers();

		GL_State( GLS_DEFAULT );
		GL_Cull( CT_TWO_SIDED );

		// set uniforms

		// set up the transformation matrix
		backEnd.orientation = backEnd.viewParms.world;
		GL_LoadModelViewMatrix( backEnd.orientation.modelViewMatrix );
		gl_genericShader->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );

		// bind u_ColorMap
		GL_SelectTexture( 0 );
		GL_Bind( tr.whiteImage );
		gl_genericShader->SetUniform_ColorTextureMatrix( matrixIdentity );

		GL_CheckErrors();

		Tess_Begin( Tess_StageIteratorDebug, NULL, NULL, NULL, qtrue, qfalse, -1, 0 );

		for ( j = 0; j < tr.world->numLightGridPoints; j++ )
		{
			gridPoint = &tr.world->lightGridData[ j ];

			if ( VectorDistanceSquared( gridPoint->origin, backEnd.viewParms.orientation.origin ) > Square( 1024 ) )
			{
				continue;
			}

			VectorNegate( gridPoint->direction, lightDirection );

			length = 8;
			VectorMA( gridPoint->origin, 8, lightDirection, offset );

			PerpendicularVector( tmp, lightDirection );
			//VectorCopy(up, tmp);

			VectorScale( tmp, length * 0.1, tmp2 );
			VectorMA( tmp2, length * 0.2, lightDirection, tmp2 );

			for ( k = 0; k < 3; k++ )
			{
				RotatePointAroundVector( tmp3, lightDirection, tmp2, k * 120 );
				VectorAdd( tmp3, gridPoint->origin, tmp3 );
				VectorCopy( tmp3, tetraVerts[ k ] );
				tetraVerts[ k ][ 3 ] = 1;
			}

			VectorCopy( gridPoint->origin, tetraVerts[ 3 ] );
			tetraVerts[ 3 ][ 3 ] = 1;
			Tess_AddTetrahedron( tetraVerts, gridPoint->directedColor );

			VectorCopy( offset, tetraVerts[ 3 ] );
			tetraVerts[ 3 ][ 3 ] = 1;
			Tess_AddTetrahedron( tetraVerts, gridPoint->directedColor );
		}

		Tess_End();

		// go back to the world modelview matrix
		backEnd.orientation = backEnd.viewParms.world;
		GL_LoadModelViewMatrix( backEnd.viewParms.world.modelViewMatrix );
	}

	if ( r_showBspNodes->integer )
	{
		bspNode_t *node;
		link_t    *l, *sentinel;

		if ( ( backEnd.refdef.rdflags & ( RDF_NOWORLDMODEL ) ) || !tr.world )
		{
			return;
		}

		gl_genericShader->DisableAlphaTesting();
		gl_genericShader->DisablePortalClipping();
		gl_genericShader->DisableVertexSkinning();
		gl_genericShader->DisableVertexAnimation();
		gl_genericShader->DisableDeformVertexes();
		gl_genericShader->DisableTCGenEnvironment();

		gl_genericShader->BindProgram();

		// set uniforms
		gl_genericShader->SetUniform_ColorModulate( CGEN_CUSTOM_RGB, AGEN_CUSTOM );

		// bind u_ColorMap
		GL_SelectTexture( 0 );
		GL_Bind( tr.whiteImage );
		gl_genericShader->SetUniform_ColorTextureMatrix( matrixIdentity );

		GL_CheckErrors();

		for ( int i = 0; i < 2; i++ )
		{
			float    x, y, w, h;
			matrix_t ortho;
			vec4_t   quadVerts[ 4 ];

			if ( i == 1 )
			{
				// set 2D virtual screen size
				GL_PushMatrix();
				MatrixOrthogonalProjection( ortho, backEnd.viewParms.viewportX,
				                            backEnd.viewParms.viewportX + backEnd.viewParms.viewportWidth,
				                            backEnd.viewParms.viewportY,
				                            backEnd.viewParms.viewportY + backEnd.viewParms.viewportHeight, -99999, 99999 );
				GL_LoadProjectionMatrix( ortho );
				GL_LoadModelViewMatrix( matrixIdentity );

				GL_Cull( CT_TWO_SIDED );
				GL_State( GLS_DEPTHTEST_DISABLE );

				gl_genericShader->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );
				gl_genericShader->SetUniform_Color( colorBlack );

				w = 300;
				h = 300;

				x = 20;
				y = 90;

				Vector4Set( quadVerts[ 0 ], x, y, 0, 1 );
				Vector4Set( quadVerts[ 1 ], x + w, y, 0, 1 );
				Vector4Set( quadVerts[ 2 ], x + w, y + h, 0, 1 );
				Vector4Set( quadVerts[ 3 ], x, y + h, 0, 1 );

				Tess_InstantQuad( quadVerts );

				{
					int    j;
					vec4_t splitFrustum[ 6 ];
					vec3_t farCorners[ 4 ];
					vec3_t nearCorners[ 4 ];
					vec3_t cropBounds[ 2 ];
					vec4_t point, transf;

					GL_Viewport( x, y, w, h );
					GL_Scissor( x, y, w, h );

					GL_PushMatrix();

					// calculate top down view projection matrix
					{
						vec3_t                                                       forward = { 0, 0, -1 };
						vec3_t                                                       up = { 1, 0, 0 };

						matrix_t /*rotationMatrix, transformMatrix,*/ viewMatrix, projectionMatrix;

						// Quake -> OpenGL view matrix from light perspective
#if 0
						VectorToAngles( lightDirection, angles );
						MatrixFromAngles( rotationMatrix, angles[ PITCH ], angles[ YAW ], angles[ ROLL ] );
						MatrixSetupTransformFromRotation( transformMatrix, rotationMatrix, backEnd.viewParms.orientation.origin );
						MatrixAffineInverse( transformMatrix, viewMatrix );
						MatrixMultiply( quakeToOpenGLMatrix, viewMatrix, light->viewMatrix );
#else
						MatrixLookAtRH( viewMatrix, backEnd.viewParms.orientation.origin, forward, up );
#endif

						//ClearBounds(splitFrustumBounds[0], splitFrustumBounds[1]);
						//BoundsAdd(splitFrustumBounds[0], splitFrustumBounds[1], backEnd.viewParms.visBounds[0], backEnd.viewParms.visBounds[1]);
						//BoundsAdd(splitFrustumBounds[0], splitFrustumBounds[1], light->worldBounds[0], light->worldBounds[1]);

						ClearBounds( cropBounds[ 0 ], cropBounds[ 1 ] );

						for ( j = 0; j < 8; j++ )
						{
							point[ 0 ] = tr.world->models[ 0 ].bounds[ j & 1 ][ 0 ];
							point[ 1 ] = tr.world->models[ 0 ].bounds[( j >> 1 ) & 1 ][ 1 ];
							point[ 2 ] = tr.world->models[ 0 ].bounds[( j >> 2 ) & 1 ][ 2 ];
							point[ 3 ] = 1;

							MatrixTransform4( viewMatrix, point, transf );
							transf[ 0 ] /= transf[ 3 ];
							transf[ 1 ] /= transf[ 3 ];
							transf[ 2 ] /= transf[ 3 ];

							AddPointToBounds( transf, cropBounds[ 0 ], cropBounds[ 1 ] );
						}

						MatrixOrthogonalProjectionRH( projectionMatrix, cropBounds[ 0 ][ 0 ], cropBounds[ 1 ][ 0 ], cropBounds[ 0 ][ 1 ], cropBounds[ 1 ][ 1 ], -cropBounds[ 1 ][ 2 ], -cropBounds[ 0 ][ 2 ] );

						GL_LoadModelViewMatrix( viewMatrix );
						GL_LoadProjectionMatrix( projectionMatrix );
					}

					// set uniforms
					gl_genericShader->SetUniform_ColorModulate( CGEN_VERTEX, AGEN_VERTEX );
					gl_genericShader->SetUniform_Color( colorBlack );

					GL_State( GLS_POLYMODE_LINE | GLS_DEPTHTEST_DISABLE );
					GL_Cull( CT_TWO_SIDED );

					// bind u_ColorMap
					GL_SelectTexture( 0 );
					GL_Bind( tr.whiteImage );
					gl_genericShader->SetUniform_ColorTextureMatrix( matrixIdentity );

					gl_genericShader->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );

					tess.multiDrawPrimitives = 0;
					tess.numIndexes = 0;
					tess.numVertexes = 0;

					for ( j = 0; j < 6; j++ )
					{
						VectorCopy( backEnd.viewParms.frustums[ 0 ][ j ].normal, splitFrustum[ j ] );
						splitFrustum[ j ][ 3 ] = backEnd.viewParms.frustums[ 0 ][ j ].dist;
					}

					// calculate split frustum corner points
					PlanesGetIntersectionPoint( splitFrustum[ FRUSTUM_LEFT ], splitFrustum[ FRUSTUM_TOP ], splitFrustum[ FRUSTUM_NEAR ], nearCorners[ 0 ] );
					PlanesGetIntersectionPoint( splitFrustum[ FRUSTUM_RIGHT ], splitFrustum[ FRUSTUM_TOP ], splitFrustum[ FRUSTUM_NEAR ], nearCorners[ 1 ] );
					PlanesGetIntersectionPoint( splitFrustum[ FRUSTUM_RIGHT ], splitFrustum[ FRUSTUM_BOTTOM ], splitFrustum[ FRUSTUM_NEAR ], nearCorners[ 2 ] );
					PlanesGetIntersectionPoint( splitFrustum[ FRUSTUM_LEFT ], splitFrustum[ FRUSTUM_BOTTOM ], splitFrustum[ FRUSTUM_NEAR ], nearCorners[ 3 ] );

					PlanesGetIntersectionPoint( splitFrustum[ FRUSTUM_LEFT ], splitFrustum[ FRUSTUM_TOP ], splitFrustum[ FRUSTUM_FAR ], farCorners[ 0 ] );
					PlanesGetIntersectionPoint( splitFrustum[ FRUSTUM_RIGHT ], splitFrustum[ FRUSTUM_TOP ], splitFrustum[ FRUSTUM_FAR ], farCorners[ 1 ] );
					PlanesGetIntersectionPoint( splitFrustum[ FRUSTUM_RIGHT ], splitFrustum[ FRUSTUM_BOTTOM ], splitFrustum[ FRUSTUM_FAR ], farCorners[ 2 ] );
					PlanesGetIntersectionPoint( splitFrustum[ FRUSTUM_LEFT ], splitFrustum[ FRUSTUM_BOTTOM ], splitFrustum[ FRUSTUM_FAR ], farCorners[ 3 ] );

					// draw outer surfaces
					for ( j = 0; j < 4; j++ )
					{
						Vector4Set( quadVerts[ 0 ], nearCorners[ j ][ 0 ], nearCorners[ j ][ 1 ], nearCorners[ j ][ 2 ], 1 );
						Vector4Set( quadVerts[ 1 ], farCorners[ j ][ 0 ], farCorners[ j ][ 1 ], farCorners[ j ][ 2 ], 1 );
						Vector4Set( quadVerts[ 2 ], farCorners[( j + 1 ) % 4 ][ 0 ], farCorners[( j + 1 ) % 4 ][ 1 ], farCorners[( j + 1 ) % 4 ][ 2 ], 1 );
						Vector4Set( quadVerts[ 3 ], nearCorners[( j + 1 ) % 4 ][ 0 ], nearCorners[( j + 1 ) % 4 ][ 1 ], nearCorners[( j + 1 ) % 4 ][ 2 ], 1 );
						Tess_AddQuadStamp2( quadVerts, colorCyan );
					}

					// draw far cap
					Vector4Set( quadVerts[ 0 ], farCorners[ 3 ][ 0 ], farCorners[ 3 ][ 1 ], farCorners[ 3 ][ 2 ], 1 );
					Vector4Set( quadVerts[ 1 ], farCorners[ 2 ][ 0 ], farCorners[ 2 ][ 1 ], farCorners[ 2 ][ 2 ], 1 );
					Vector4Set( quadVerts[ 2 ], farCorners[ 1 ][ 0 ], farCorners[ 1 ][ 1 ], farCorners[ 1 ][ 2 ], 1 );
					Vector4Set( quadVerts[ 3 ], farCorners[ 0 ][ 0 ], farCorners[ 0 ][ 1 ], farCorners[ 0 ][ 2 ], 1 );
					Tess_AddQuadStamp2( quadVerts, colorBlue );

					// draw near cap
					Vector4Set( quadVerts[ 0 ], nearCorners[ 0 ][ 0 ], nearCorners[ 0 ][ 1 ], nearCorners[ 0 ][ 2 ], 1 );
					Vector4Set( quadVerts[ 1 ], nearCorners[ 1 ][ 0 ], nearCorners[ 1 ][ 1 ], nearCorners[ 1 ][ 2 ], 1 );
					Vector4Set( quadVerts[ 2 ], nearCorners[ 2 ][ 0 ], nearCorners[ 2 ][ 1 ], nearCorners[ 2 ][ 2 ], 1 );
					Vector4Set( quadVerts[ 3 ], nearCorners[ 3 ][ 0 ], nearCorners[ 3 ][ 1 ], nearCorners[ 3 ][ 2 ], 1 );
					Tess_AddQuadStamp2( quadVerts, colorGreen );

					Tess_UpdateVBOs( ATTR_POSITION | ATTR_COLOR );
					Tess_DrawElements();

					gl_genericShader->SetUniform_ColorModulate( CGEN_CUSTOM_RGB, AGEN_CUSTOM );
				}
			} // i == 1
			else
			{
				GL_State( GLS_POLYMODE_LINE | GLS_DEPTHTEST_DISABLE );
				GL_Cull( CT_TWO_SIDED );

				// render in world space
				backEnd.orientation = backEnd.viewParms.world;

				GL_LoadProjectionMatrix( backEnd.viewParms.projectionMatrix );
				GL_LoadModelViewMatrix( backEnd.viewParms.world.modelViewMatrix );

				gl_genericShader->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );
			}

			// draw BSP nodes
			sentinel = &tr.traversalStack;

			for ( l = sentinel->next; l != sentinel; l = l->next )
			{
				node = ( bspNode_t * ) l->data;

				if ( !r_dynamicBspOcclusionCulling->integer )
				{
					if ( node->contents != -1 )
					{
						if ( r_showBspNodes->integer == 3 )
						{
							continue;
						}

						if ( node->numMarkSurfaces <= 0 )
						{
							continue;
						}

						//if(node->shrinkedAABB)
						//  gl_genericShader->SetUniform_Color(colorBlue);
						//else
						if ( node->visCounts[ tr.visIndex ] == tr.visCounts[ tr.visIndex ] )
						{
							gl_genericShader->SetUniform_Color( colorGreen );
						}
						else
						{
							gl_genericShader->SetUniform_Color( colorRed );
						}
					}
					else
					{
						if ( r_showBspNodes->integer == 2 )
						{
							continue;
						}

						if ( node->visCounts[ tr.visIndex ] == tr.visCounts[ tr.visIndex ] )
						{
							gl_genericShader->SetUniform_Color( colorYellow );
						}
						else
						{
							gl_genericShader->SetUniform_Color( colorBlue );
						}
					}
				}
				else
				{
					if ( node->lastVisited[ backEnd.viewParms.viewCount ] != backEnd.viewParms.frameCount )
					{
						continue;
					}

					if ( r_showBspNodes->integer == 5 && node->lastQueried[ backEnd.viewParms.viewCount ] != backEnd.viewParms.frameCount )
					{
						continue;
					}

					if ( node->contents != -1 )
					{
						if ( r_showBspNodes->integer == 3 )
						{
							continue;
						}

						//if(node->occlusionQuerySamples[backEnd.viewParms.viewCount] > 0)
						if ( node->visible[ backEnd.viewParms.viewCount ] )
						{
							gl_genericShader->SetUniform_Color( colorGreen );
						}
						else
						{
							gl_genericShader->SetUniform_Color( colorRed );
						}
					}
					else
					{
						if ( r_showBspNodes->integer == 2 )
						{
							continue;
						}

						//if(node->occlusionQuerySamples[backEnd.viewParms.viewCount] > 0)
						if ( node->visible[ backEnd.viewParms.viewCount ] )
						{
							gl_genericShader->SetUniform_Color( colorYellow );
						}
						else
						{
							gl_genericShader->SetUniform_Color( colorBlue );
						}
					}

					if ( r_showBspNodes->integer == 4 )
					{
						gl_genericShader->SetUniform_Color( g_color_table[ ColorIndex( node->occlusionQueryNumbers[ backEnd.viewParms.viewCount ] ) ] );
					}

					GL_CheckErrors();
				}

				if ( node->contents != -1 )
				{
					glEnable( GL_POLYGON_OFFSET_FILL );
					GL_PolygonOffset( r_offsetFactor->value, r_offsetUnits->value );
				}

				R_BindVBO( node->volumeVBO );
				R_BindIBO( node->volumeIBO );

				GL_VertexAttribsState( ATTR_POSITION );

				tess.multiDrawPrimitives = 0;
				tess.numVertexes = node->volumeVerts;
				tess.numIndexes = node->volumeIndexes;

				Tess_DrawElements();

				tess.numIndexes = 0;
				tess.numVertexes = 0;

				if ( node->contents != -1 )
				{
					glDisable( GL_POLYGON_OFFSET_FILL );
				}
			}

			if ( i == 1 )
			{
				tess.multiDrawPrimitives = 0;
				tess.numIndexes = 0;
				tess.numVertexes = 0;

				GL_PopMatrix();

				GL_Viewport( backEnd.viewParms.viewportX, backEnd.viewParms.viewportY,
				             backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight );

				GL_Scissor( backEnd.viewParms.viewportX, backEnd.viewParms.viewportY,
				            backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight );

				GL_PopMatrix();
			}
		}

		// go back to the world modelview matrix
		backEnd.orientation = backEnd.viewParms.world;

		GL_LoadProjectionMatrix( backEnd.viewParms.projectionMatrix );
		GL_LoadModelViewMatrix( backEnd.viewParms.world.modelViewMatrix );
	}

	if ( r_showDecalProjectors->integer )
	{
		int              i;
		decalProjector_t *dp;
		srfDecal_t       *srfDecal;
		static const vec3_t mins = { -1, -1, -1 };
		static const vec3_t maxs = { 1, 1, 1 };

		if ( backEnd.refdef.rdflags & ( RDF_NOWORLDMODEL ) )
		{
			return;
		}

		gl_genericShader->DisableAlphaTesting();
		gl_genericShader->DisablePortalClipping();
		gl_genericShader->DisableVertexSkinning();
		gl_genericShader->DisableVertexAnimation();
		gl_genericShader->DisableDeformVertexes();
		gl_genericShader->DisableTCGenEnvironment();

		gl_genericShader->BindProgram();

		GL_State( GLS_POLYMODE_LINE | GLS_DEPTHTEST_DISABLE );
		GL_Cull( CT_TWO_SIDED );

		// set uniforms
		gl_genericShader->SetUniform_ColorModulate( CGEN_VERTEX, AGEN_VERTEX );
		gl_genericShader->SetUniform_Color( colorBlack );

		// set up the transformation matrix
		backEnd.orientation = backEnd.viewParms.world;
		GL_LoadModelViewMatrix( backEnd.orientation.modelViewMatrix );
		gl_genericShader->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );

		// bind u_ColorMap
		GL_SelectTexture( 0 );
		GL_Bind( tr.whiteImage );
		gl_genericShader->SetUniform_ColorTextureMatrix( matrixIdentity );

		GL_CheckErrors();

		Tess_Begin( Tess_StageIteratorDebug, NULL, NULL, NULL, qtrue, qfalse, -1, 0 );

		for ( i = 0, dp = backEnd.refdef.decalProjectors; i < backEnd.refdef.numDecalProjectors; i++, dp++ )
		{
			if ( VectorDistanceSquared( dp->center, backEnd.viewParms.orientation.origin ) > Square( 1024 ) )
			{
				continue;
			}

			Tess_AddCube( dp->center, mins, maxs, colorRed );
			Tess_AddCube( vec3_origin, dp->mins, dp->maxs, colorBlue );
		}

		glEnable( GL_POLYGON_OFFSET_FILL );
		GL_PolygonOffset( r_offsetFactor->value, r_offsetUnits->value );

		for ( i = 0, srfDecal = backEnd.refdef.decals; i < backEnd.refdef.numDecals; i++, srfDecal++ )
		{
			rb_surfaceTable[ SF_DECAL ]( srfDecal );
		}

		glDisable( GL_POLYGON_OFFSET_FILL );

		Tess_End();

		// go back to the world modelview matrix
		//backEnd.orientation = backEnd.viewParms.world;
		//GL_LoadModelViewMatrix(backEnd.viewParms.world.modelViewMatrix);
	}

	GL_CheckErrors();
}

/*
==================
RB_RenderView
==================
*/
static void RB_RenderView( void )
{
	if ( r_logFile->integer )
	{
		// don't just call LogComment, or we will get a call to va() every frame!
		GLimp_LogComment( va
		                  ( "--- RB_RenderView( %i surfaces, %i interactions ) ---\n", backEnd.viewParms.numDrawSurfs,
		                    backEnd.viewParms.numInteractions ) );
	}

	//ri.Error(ERR_FATAL, "test");

	GL_CheckErrors();

	backEnd.pc.c_surfaces += backEnd.viewParms.numDrawSurfs;

	if ( DS_STANDARD_ENABLED() )
	{
		//
		// Deferred shading path
		//

		int clearBits = 0;
		int startTime = 0, endTime = 0;

		// sync with gl if needed
		if ( r_finish->integer == 1 && !glState.finishCalled )
		{
			glFinish();
			glState.finishCalled = qtrue;
		}

		if ( r_finish->integer == 0 )
		{
			glState.finishCalled = qtrue;
		}

		// we will need to change the projection matrix before drawing
		// 2D images again
		backEnd.projection2D = qfalse;

		// set the modelview matrix for the viewer
		SetViewportAndScissor();

		// ensures that depth writes are enabled for the depth clear
		GL_State( GLS_DEFAULT );

		// clear frame buffer objects
		R_BindNullFBO();
		//R_BindFBO(tr.deferredRenderFBO);
		//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		clearBits = GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;

		/*
		   if(r_measureOverdraw->integer || r_shadows->integer == SHADOWING_STENCIL)
		   {
		   clearBits |= GL_STENCIL_BUFFER_BIT;
		   }
		 */
		if ( !( backEnd.refdef.rdflags & RDF_NOWORLDMODEL ) )
		{
			clearBits |= GL_COLOR_BUFFER_BIT;
			GL_ClearColor( 0.0f, 0.0f, 0.0f, 1.0f );  // FIXME: get color of sky
		}

		glClear( clearBits );

		R_BindFBO( tr.geometricRenderFBO );

		if ( !( backEnd.refdef.rdflags & RDF_NOWORLDMODEL ) )
		{
			//clearBits = GL_COLOR_BUFFER_BIT;
			GL_ClearColor( 0.0f, 0.0f, 0.0f, 1.0f );  // FIXME: get color of sky
		}
		else
		{
			/*
			if(glConfig2.framebufferBlitAvailable)
			{
			        glDrawBuffers(1, geometricRenderTargets);

			        // copy color of the main context to geometricRenderFBO

			        glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, 0);
			        glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, tr.deferredRenderFBO->frameBuffer);
			        glBlitFramebufferEXT(0, 0, glConfig.vidWidth, glConfig.vidHeight,
			                                                   0, 0, glConfig.vidWidth, glConfig.vidHeight,
			                                                   GL_COLOR_BUFFER_BIT,
			                                                   GL_NEAREST);
			}
			 */
		}

		glClear( clearBits );

		if ( ( backEnd.refdef.rdflags & RDF_HYPERSPACE ) )
		{
			RB_Hyperspace();
			return;
		}
		else
		{
			backEnd.isHyperspace = qfalse;
		}

		glState.faceCulling = -1; // force face culling to set next time

		// we will only draw a sun if there was sky rendered in this view
		backEnd.skyRenderedThisView = qfalse;

		GL_CheckErrors();

		//RB_RenderDrawSurfacesIntoGeometricBuffer();

		if ( r_speeds->integer == RSPEEDS_SHADING_TIMES )
		{
			glFinish();
			startTime = ri.Milliseconds();
		}

		// draw everything that is opaque
		R_BindFBO( tr.geometricRenderFBO );
		RB_RenderDrawSurfaces( true, true, DRAWSURFACES_ALL );

		if ( r_speeds->integer == RSPEEDS_SHADING_TIMES )
		{
			glFinish();
			endTime = ri.Milliseconds();
			backEnd.pc.c_deferredGBufferTime = endTime - startTime;
		}

		// try to cull lights using hardware occlusion queries
		R_BindFBO( tr.geometricRenderFBO );
		RB_RenderLightOcclusionQueries();

		if ( r_speeds->integer == RSPEEDS_SHADING_TIMES )
		{
			glFinish();
			startTime = ri.Milliseconds();
		}

		if ( !r_showDeferredRender->integer )
		{
			if ( r_shadows->integer >= SHADOWING_ESM16 )
			{
				// render dynamic shadowing and lighting using shadow mapping
				RB_RenderInteractionsDeferredShadowMapped();
			}
			else
			{
				// render dynamic lighting
				RB_RenderInteractionsDeferred();
			}
		}

		if ( r_speeds->integer == RSPEEDS_SHADING_TIMES )
		{
			glFinish();
			endTime = ri.Milliseconds();
			backEnd.pc.c_deferredLightingTime = endTime - startTime;
		}

		R_BindFBO( tr.geometricRenderFBO );
		glDrawBuffers( 1, geometricRenderTargets );

		// render global fog
		RB_RenderGlobalFog();

		if ( r_speeds->integer == RSPEEDS_SHADING_TIMES )
		{
			glFinish();
			startTime = ri.Milliseconds();
		}

		// draw everything that is translucent
		R_BindFBO( tr.geometricRenderFBO );
		RB_RenderDrawSurfaces( false, false, DRAWSURFACES_ALL );

		if ( r_speeds->integer == RSPEEDS_SHADING_TIMES )
		{
			glFinish();
			endTime = ri.Milliseconds();
			backEnd.pc.c_forwardTranslucentTime = endTime - startTime;
		}

		// render debug information
		R_BindFBO( tr.geometricRenderFBO );
		RB_RenderDebugUtils();

		// scale down rendered HDR scene to 1 / 4th
		if ( r_hdrRendering->integer )
		{
			// FIXME

			/*
			if(glConfig2.framebufferBlitAvailable)
			{
			        glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, tr.geometricRenderFBO->frameBuffer);
			        glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, tr.downScaleFBO_quarter->frameBuffer);
			        glBlitFramebufferEXT(0, 0, glConfig.vidWidth, glConfig.vidHeight,
			                                                        0, 0, glConfig.vidWidth * 0.25f, glConfig.vidHeight * 0.25f,
			                                                        GL_COLOR_BUFFER_BIT,
			                                                        GL_LINEAR);

			        glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, tr.geometricRenderFBO->frameBuffer);
			        glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, tr.downScaleFBO_64x64->frameBuffer);
			        glBlitFramebufferEXT(0, 0, glConfig.vidWidth, glConfig.vidHeight,
			                                                   0, 0, 64, 64,
			                                                   GL_COLOR_BUFFER_BIT,
			                                                   GL_LINEAR);
			}
			else
			{
			        // FIXME add non EXT_framebuffer_blit code
			}
			*/

			RB_CalculateAdaptation();
		}
		else
		{
			/*
			if(glConfig2.framebufferBlitAvailable)
			{
			        // copy deferredRenderFBO to downScaleFBO_quarter
			        glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, tr.deferredRenderFBO->frameBuffer);
			        glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, tr.downScaleFBO_quarter->frameBuffer);
			        glBlitFramebufferEXT(0, 0, glConfig.vidWidth, glConfig.vidHeight,
			                                                        0, 0, glConfig.vidWidth * 0.25f, glConfig.vidHeight * 0.25f,
			                                                        GL_COLOR_BUFFER_BIT,
			                                                        GL_LINEAR);
			}
			else
			{
			        // FIXME add non EXT_framebuffer_blit code
			}
			*/
		}

		GL_CheckErrors();

		// render bloom post process effect
		//RB_RenderBloom();

		// copy offscreen rendered scene to the current OpenGL context
		RB_RenderDeferredShadingResultToFrameBuffer();

		if ( backEnd.viewParms.isPortal )
		{
			/*
			if(glConfig2.framebufferBlitAvailable)
			{
			        // copy deferredRenderFBO to portalRenderFBO
			        glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, tr.deferredRenderFBO->frameBuffer);
			        glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, tr.portalRenderFBO->frameBuffer);
			        glBlitFramebufferEXT(0, 0, tr.deferredRenderFBO->width, tr.deferredRenderFBO->height,
			                                                   0, 0, tr.portalRenderFBO->width, tr.portalRenderFBO->height,
			                                                   GL_COLOR_BUFFER_BIT,
			                                                   GL_NEAREST);
			}
			else
			{
			        // capture current color buffer
			        GL_SelectTexture(0);
			        GL_Bind(tr.portalRenderImage);
			        glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, tr.portalRenderImage->uploadWidth, tr.portalRenderImage->uploadHeight);
			}
			*/
			backEnd.pc.c_portals++;
		}
	}
	else
	{
		//
		// Forward shading path
		//

		int clearBits = 0;
		int startTime = 0, endTime = 0;

		// sync with gl if needed
		if ( r_finish->integer == 1 && !glState.finishCalled )
		{
			glFinish();
			glState.finishCalled = qtrue;
		}

		if ( r_finish->integer == 0 )
		{
			glState.finishCalled = qtrue;
		}

		// disable offscreen rendering
		if ( glConfig2.framebufferObjectAvailable )
		{
			if ( r_hdrRendering->integer && glConfig2.textureFloatAvailable )
			{
				R_BindFBO( tr.deferredRenderFBO );
			}
			else
			{
				R_BindNullFBO();
			}
		}

		// we will need to change the projection matrix before drawing
		// 2D images again
		backEnd.projection2D = qfalse;

		// set the modelview matrix for the viewer
		SetViewportAndScissor();

		// ensures that depth writes are enabled for the depth clear
		GL_State( GLS_DEFAULT );

		// clear relevant buffers
		clearBits = GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;

#if defined( COMPAT_ET )
		// ydnar: global q3 fog volume
		if ( tr.world && tr.world->globalFog >= 0 )
		{
			if ( !( backEnd.refdef.rdflags & RDF_NOWORLDMODEL ) )
			{
				clearBits |= GL_COLOR_BUFFER_BIT;

				GL_ClearColor( tr.world->fogs[ tr.world->globalFog ].color[ 0 ],
				               tr.world->fogs[ tr.world->globalFog ].color[ 1 ],
				               tr.world->fogs[ tr.world->globalFog ].color[ 2 ], 1.0 );
			}
		}
		else if ( tr.world && tr.world->hasSkyboxPortal )
		{
			if ( backEnd.refdef.rdflags & RDF_SKYBOXPORTAL )
			{
				// portal scene, clear whatever is necessary

				if ( r_fastsky->integer || backEnd.refdef.rdflags & RDF_NOWORLDMODEL )
				{
					// fastsky: clear color

					// try clearing first with the portal sky fog color, then the world fog color, then finally a default
					clearBits |= GL_COLOR_BUFFER_BIT;

					if ( tr.glfogsettings[ FOG_PORTALVIEW ].registered )
					{
						GL_ClearColor( tr.glfogsettings[ FOG_PORTALVIEW ].color[ 0 ], tr.glfogsettings[ FOG_PORTALVIEW ].color[ 1 ],
						               tr.glfogsettings[ FOG_PORTALVIEW ].color[ 2 ], tr.glfogsettings[ FOG_PORTALVIEW ].color[ 3 ] );
					}
					else if ( tr.glfogNum > FOG_NONE && tr.glfogsettings[ FOG_CURRENT ].registered )
					{
						GL_ClearColor( tr.glfogsettings[ FOG_CURRENT ].color[ 0 ], tr.glfogsettings[ FOG_CURRENT ].color[ 1 ],
						               tr.glfogsettings[ FOG_CURRENT ].color[ 2 ], tr.glfogsettings[ FOG_CURRENT ].color[ 3 ] );
					}
					else
					{
						//                  GL_ClearColor ( 1.0, 0.0, 0.0, 1.0 );   // red clear for testing portal sky clear
						GL_ClearColor( 0.5, 0.5, 0.5, 1.0 );
					}
				}
				else
				{
					// rendered sky (either clear color or draw quake sky)
					if ( tr.glfogsettings[ FOG_PORTALVIEW ].registered )
					{
						GL_ClearColor( tr.glfogsettings[ FOG_PORTALVIEW ].color[ 0 ], tr.glfogsettings[ FOG_PORTALVIEW ].color[ 1 ],
						               tr.glfogsettings[ FOG_PORTALVIEW ].color[ 2 ], tr.glfogsettings[ FOG_PORTALVIEW ].color[ 3 ] );

						if ( tr.glfogsettings[ FOG_PORTALVIEW ].clearscreen )
						{
							// portal fog requests a screen clear (distance fog rather than quake sky)
							clearBits |= GL_COLOR_BUFFER_BIT;
						}
					}
				}
			}
			else
			{
				// world scene with portal sky, don't clear any buffers, just set the fog color if there is one

				if ( tr.glfogNum > FOG_NONE && tr.glfogsettings[ FOG_CURRENT ].registered )
				{
					if ( backEnd.refdef.rdflags & RDF_UNDERWATER )
					{
						if ( tr.glfogsettings[ FOG_CURRENT ].mode == GL_LINEAR )
						{
							clearBits |= GL_COLOR_BUFFER_BIT;
						}
					}
					else if ( !r_portalSky->integer )
					{
						// portal skies have been manually turned off, clear bg color
						clearBits |= GL_COLOR_BUFFER_BIT;
					}

					GL_ClearColor( tr.glfogsettings[ FOG_CURRENT ].color[ 0 ], tr.glfogsettings[ FOG_CURRENT ].color[ 1 ],
					               tr.glfogsettings[ FOG_CURRENT ].color[ 2 ], tr.glfogsettings[ FOG_CURRENT ].color[ 3 ] );
				}
				else if ( !r_portalSky->integer )
				{
					// ydnar: portal skies have been manually turned off, clear bg color
					clearBits |= GL_COLOR_BUFFER_BIT;
					GL_ClearColor( 0.5, 0.5, 0.5, 1.0 );
				}
			}
		}
		else
		{
			// world scene with no portal sky

			// NERVE - SMF - we don't want to clear the buffer when no world model is specified
			if ( backEnd.refdef.rdflags & RDF_NOWORLDMODEL )
			{
				clearBits &= ~GL_COLOR_BUFFER_BIT;
			}
			// -NERVE - SMF
			else if ( r_fastsky->integer || backEnd.refdef.rdflags & RDF_NOWORLDMODEL )
			{
				clearBits |= GL_COLOR_BUFFER_BIT;

				if ( tr.glfogsettings[ FOG_CURRENT ].registered )
				{
					// try to clear fastsky with current fog color
					GL_ClearColor( tr.glfogsettings[ FOG_CURRENT ].color[ 0 ], tr.glfogsettings[ FOG_CURRENT ].color[ 1 ],
					               tr.glfogsettings[ FOG_CURRENT ].color[ 2 ], tr.glfogsettings[ FOG_CURRENT ].color[ 3 ] );
				}
				else
				{
					//              GL_ClearColor ( 0.0, 0.0, 1.0, 1.0 );   // blue clear for testing world sky clear
					GL_ClearColor( 0.05, 0.05, 0.05, 1.0 );  // JPW NERVE changed per id req was 0.5s
				}
			}
			else
			{
				// world scene, no portal sky, not fastsky, clear color if fog says to, otherwise, just set the clearcolor
				if ( tr.glfogsettings[ FOG_CURRENT ].registered )
				{
					// try to clear fastsky with current fog color
					GL_ClearColor( tr.glfogsettings[ FOG_CURRENT ].color[ 0 ], tr.glfogsettings[ FOG_CURRENT ].color[ 1 ],
					               tr.glfogsettings[ FOG_CURRENT ].color[ 2 ], tr.glfogsettings[ FOG_CURRENT ].color[ 3 ] );

					if ( tr.glfogsettings[ FOG_CURRENT ].clearscreen )
					{
						// world fog requests a screen clear (distance fog rather than quake sky)
						clearBits |= GL_COLOR_BUFFER_BIT;
					}
				}
			}

			if ( HDR_ENABLED() )
			{
				// copy color of the main context to deferredRenderFBO
				glBindFramebufferEXT( GL_READ_FRAMEBUFFER_EXT, 0 );
				glBindFramebufferEXT( GL_DRAW_FRAMEBUFFER_EXT, tr.deferredRenderFBO->frameBuffer );
				glBlitFramebufferEXT( 0, 0, glConfig.vidWidth, glConfig.vidHeight,
				                      0, 0, glConfig.vidWidth, glConfig.vidHeight,
				                      GL_COLOR_BUFFER_BIT,
				                      GL_NEAREST );
			}
		}

#else

		if ( !( backEnd.refdef.rdflags & RDF_NOWORLDMODEL ) )
		{
			clearBits |= GL_COLOR_BUFFER_BIT; // FIXME: only if sky shaders have been used
			GL_ClearColor( 0.0f, 0.0f, 0.0f, 1.0f );  // FIXME: get color of sky
		}
		else
		{
			if ( HDR_ENABLED() )
			{
				// copy color of the main context to deferredRenderFBO
				glBindFramebufferEXT( GL_READ_FRAMEBUFFER_EXT, 0 );
				glBindFramebufferEXT( GL_DRAW_FRAMEBUFFER_EXT, tr.deferredRenderFBO->frameBuffer );
				glBlitFramebufferEXT( 0, 0, glConfig.vidWidth, glConfig.vidHeight,
				                      0, 0, glConfig.vidWidth, glConfig.vidHeight,
				                      GL_COLOR_BUFFER_BIT,
				                      GL_NEAREST );
			}
		}

#endif

		glClear( clearBits );

		if ( ( backEnd.refdef.rdflags & RDF_HYPERSPACE ) )
		{
			RB_Hyperspace();
			return;
		}
		else
		{
			backEnd.isHyperspace = qfalse;
		}

		glState.faceCulling = -1; // force face culling to set next time

		// we will only draw a sun if there was sky rendered in this view
		backEnd.skyRenderedThisView = qfalse;

		GL_CheckErrors();

		if ( r_speeds->integer == RSPEEDS_SHADING_TIMES )
		{
			glFinish();
			startTime = ri.Milliseconds();
		}

		if ( r_dynamicEntityOcclusionCulling->integer )
		{
			// draw everything from world that is opaque into black so we can benefit from early-z rejections later
			//RB_RenderOpaqueSurfacesIntoDepth(true);
			RB_RenderDrawSurfaces( true, false, DRAWSURFACES_WORLD_ONLY );

			// try to cull entities using hardware occlusion queries
			RB_RenderEntityOcclusionQueries();

			// draw everything that is opaque
			RB_RenderDrawSurfaces( true, false, DRAWSURFACES_ENTITIES_ONLY );
		}
		else
		{
			// draw everything that is opaque
			RB_RenderDrawSurfaces( true, false, DRAWSURFACES_ALL );

			// try to cull entities using hardware occlusion queries
			//RB_RenderEntityOcclusionQueries();
		}

		// try to cull bsp nodes for the next frame using hardware occlusion queries
		//RB_RenderBspOcclusionQueries();

		if ( r_speeds->integer == RSPEEDS_SHADING_TIMES )
		{
			glFinish();
			endTime = ri.Milliseconds();
			backEnd.pc.c_forwardAmbientTime = endTime - startTime;
		}

		// try to cull lights using hardware occlusion queries
		RB_RenderLightOcclusionQueries();

		if ( r_shadows->integer >= SHADOWING_ESM16 )
		{
			// render dynamic shadowing and lighting using shadow mapping
			RB_RenderInteractionsShadowMapped();

			// render player shadows if any
			//RB_RenderInteractionsDeferredInverseShadows();
		}
		else
		{
			// render dynamic lighting
			RB_RenderInteractions();
		}

		// render ambient occlusion process effect
		// Tr3B: needs way more work RB_RenderScreenSpaceAmbientOcclusion(qfalse);

		if ( HDR_ENABLED() )
		{
			R_BindFBO( tr.deferredRenderFBO );
		}

		// render global fog post process effect
		RB_RenderGlobalFog();

		// draw everything that is translucent
		RB_RenderDrawSurfaces( false, false, DRAWSURFACES_ALL );

		// scale down rendered HDR scene to 1 / 4th
		if ( HDR_ENABLED() )
		{
			if ( glConfig2.framebufferBlitAvailable )
			{
				glBindFramebufferEXT( GL_READ_FRAMEBUFFER_EXT, tr.deferredRenderFBO->frameBuffer );
				glBindFramebufferEXT( GL_DRAW_FRAMEBUFFER_EXT, tr.downScaleFBO_quarter->frameBuffer );
				glBlitFramebufferEXT( 0, 0, glConfig.vidWidth, glConfig.vidHeight,
				                      0, 0, glConfig.vidWidth * 0.25f, glConfig.vidHeight * 0.25f,
				                      GL_COLOR_BUFFER_BIT,
				                      GL_LINEAR );

				glBindFramebufferEXT( GL_READ_FRAMEBUFFER_EXT, tr.deferredRenderFBO->frameBuffer );
				glBindFramebufferEXT( GL_DRAW_FRAMEBUFFER_EXT, tr.downScaleFBO_64x64->frameBuffer );
				glBlitFramebufferEXT( 0, 0, glConfig.vidWidth, glConfig.vidHeight,
				                      0, 0, 64, 64,
				                      GL_COLOR_BUFFER_BIT,
				                      GL_LINEAR );
			}
			else
			{
				// FIXME add non EXT_framebuffer_blit code
			}

			RB_CalculateAdaptation();
		}
		else
		{
			/*
			Tr3B: FIXME this causes: caught OpenGL error:
			GL_INVALID_OPERATION in file code/renderer/tr_backend.c line 6479

			if(glConfig2.framebufferBlitAvailable)
			{
			        // copy deferredRenderFBO to downScaleFBO_quarter
			        glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, 0);
			        glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, tr.downScaleFBO_quarter->frameBuffer);
			        glBlitFramebufferEXT(0, 0, glConfig.vidWidth, glConfig.vidHeight,
			                                                        0, 0, glConfig.vidWidth * 0.25f, glConfig.vidHeight * 0.25f,
			                                                        GL_COLOR_BUFFER_BIT,
			                                                        GL_NEAREST);
			}
			else
			{
			        // FIXME add non EXT_framebuffer_blit code
			}
			*/
		}

		GL_CheckErrors();
#ifdef EXPERIMENTAL
		// render depth of field post process effect
		RB_RenderDepthOfField();
#endif
		// render bloom post process effect
		RB_RenderBloom();

		// copy offscreen rendered HDR scene to the current OpenGL context
		RB_RenderDeferredHDRResultToFrameBuffer();

		// render rotoscope post process effect
		RB_RenderRotoscope();

#if 0
		// add the sun flare
		RB_DrawSun();
#endif

#if 0
		// add light flares on lights that aren't obscured
		RB_RenderFlares();
#endif

		// wait until all bsp node occlusion queries are back
		//RB_CollectBspOcclusionQueries();

		// render debug information
		RB_RenderDebugUtils();

		if ( backEnd.viewParms.isPortal )
		{
#if 0

			if ( r_hdrRendering->integer && glConfig.textureFloatAvailable && glConfig.framebufferObjectAvailable && glConfig.framebufferBlitAvailable )
			{
				// copy deferredRenderFBO to portalRenderFBO
				glBindFramebufferEXT( GL_READ_FRAMEBUFFER_EXT, tr.deferredRenderFBO->frameBuffer );
				glBindFramebufferEXT( GL_DRAW_FRAMEBUFFER_EXT, tr.portalRenderFBO->frameBuffer );
				glBlitFramebufferEXT( 0, 0, tr.deferredRenderFBO->width, tr.deferredRenderFBO->height,
				                      0, 0, tr.portalRenderFBO->width, tr.portalRenderFBO->height,
				                      GL_COLOR_BUFFER_BIT,
				                      GL_NEAREST );
			}

#endif
#if 0

			// FIXME: this trashes the OpenGL context for an unknown reason
			if ( glConfig2.framebufferObjectAvailable && glConfig2.framebufferBlitAvailable )
			{
				// copy main context to portalRenderFBO
				glBindFramebufferEXT( GL_READ_FRAMEBUFFER_EXT, 0 );
				glBindFramebufferEXT( GL_DRAW_FRAMEBUFFER_EXT, tr.portalRenderFBO->frameBuffer );
				glBlitFramebufferEXT( 0, 0, glConfig.vidWidth, glConfig.vidHeight,
				                      0, 0, glConfig.vidWidth, glConfig.vidHeight,
				                      GL_COLOR_BUFFER_BIT,
				                      GL_NEAREST );
			}

#endif
			//else
			{
				// capture current color buffer
				GL_SelectTexture( 0 );
				GL_Bind( tr.portalRenderImage );
				glCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, 0, tr.portalRenderImage->uploadWidth, tr.portalRenderImage->uploadHeight );
			}
			backEnd.pc.c_portals++;
		}

#if 0

		if ( r_dynamicBspOcclusionCulling->integer )
		{
			// copy depth of the main context to deferredRenderFBO
			glBindFramebufferEXT( GL_READ_FRAMEBUFFER_EXT, 0 );
			glBindFramebufferEXT( GL_DRAW_FRAMEBUFFER_EXT, tr.occlusionRenderFBO->frameBuffer );
			glBlitFramebufferEXT( 0, 0, glConfig.vidWidth, glConfig.vidHeight,
			                      0, 0, glConfig.vidWidth, glConfig.vidHeight,
			                      GL_DEPTH_BUFFER_BIT,
			                      GL_NEAREST );
		}

#endif
	}

	// render chromatric aberration
	RB_CameraPostFX();

	// copy to given byte buffer that is NOT a FBO
	if ( tr.refdef.pixelTarget != NULL )
	{
		int i;

		// need to convert Y axis
#if 0
		glReadPixels( 0, 0, tr.refdef.pixelTargetWidth, tr.refdef.pixelTargetHeight, GL_RGBA, GL_UNSIGNED_BYTE, tr.refdef.pixelTarget );
#else
		// Bugfix: drivers absolutely hate running in high res and using glReadPixels near the top or bottom edge.
		// Sooo... let's do it in the middle.
		glReadPixels( glConfig.vidWidth / 2, glConfig.vidHeight / 2, tr.refdef.pixelTargetWidth, tr.refdef.pixelTargetHeight, GL_RGBA,
		              GL_UNSIGNED_BYTE, tr.refdef.pixelTarget );
#endif

		for ( i = 0; i < tr.refdef.pixelTargetWidth * tr.refdef.pixelTargetHeight; i++ )
		{
			tr.refdef.pixelTarget[( i * 4 ) + 3 ] = 255;  //set the alpha pure white
		}
	}

	GL_CheckErrors();

	backEnd.pc.c_views++;
}

/*
============================================================================

RENDER BACK END THREAD FUNCTIONS

============================================================================
*/

/*
=============
RE_StretchRaw

FIXME: not exactly backend
Stretches a raw 32 bit power of 2 bitmap image over the given screen rectangle.
Used for cinematics.
=============
*/
void RE_StretchRaw( int x, int y, int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty )
{
	int i, j;
	int start, end;

	if ( !tr.registered )
	{
		return;
	}

	R_SyncRenderThread();

	// we definitely want to sync every frame for the cinematics
	glFinish();

	start = end = 0;

	if ( r_speeds->integer )
	{
		glFinish();
		start = ri.Milliseconds();
	}

	// make sure rows and cols are powers of 2
	for ( i = 0; ( 1 << i ) < cols; i++ )
	{
	}

	for ( j = 0; ( 1 << j ) < rows; j++ )
	{
	}

	if ( ( 1 << i ) != cols || ( 1 << j ) != rows )
	{
		ri.Error( ERR_DROP, "Draw_StretchRaw: size not a power of 2: %i by %i", cols, rows );
	}

	RB_SetGL2D();

	glVertexAttrib4f( ATTR_INDEX_NORMAL, 0, 0, 1, 1 );
	glVertexAttrib4f( ATTR_INDEX_COLOR, tr.identityLight, tr.identityLight, tr.identityLight, 1 );

	gl_genericShader->DisableAlphaTesting();
	gl_genericShader->DisablePortalClipping();
	gl_genericShader->DisableVertexSkinning();
	gl_genericShader->DisableVertexAnimation();
	gl_genericShader->DisableDeformVertexes();
	gl_genericShader->DisableTCGenEnvironment();

	gl_genericShader->BindProgram();

	// set uniforms
	gl_genericShader->SetUniform_ColorModulate( CGEN_VERTEX, AGEN_VERTEX );
	gl_genericShader->SetUniform_Color( colorBlack );

	gl_genericShader->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );

	// bind u_ColorMap
	GL_SelectTexture( 0 );
	GL_Bind( tr.scratchImage[ client ] );
	gl_genericShader->SetUniform_ColorTextureMatrix( matrixIdentity );

	// if the scratchImage isn't in the format we want, specify it as a new texture
	if ( cols != tr.scratchImage[ client ]->width || rows != tr.scratchImage[ client ]->height )
	{
		tr.scratchImage[ client ]->width = tr.scratchImage[ client ]->uploadWidth = cols;
		tr.scratchImage[ client ]->height = tr.scratchImage[ client ]->uploadHeight = rows;

		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB8, cols, rows, 0, GL_RGBA, GL_UNSIGNED_BYTE, data );

		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	}
	else
	{
		if ( dirty )
		{
			// otherwise, just subimage upload it so that drivers can tell we are going to be changing
			// it and don't try and do a texture compression
			glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, cols, rows, GL_RGBA, GL_UNSIGNED_BYTE, data );
		}
	}

	if ( r_speeds->integer )
	{
		glFinish();
		end = ri.Milliseconds();
		ri.Printf( PRINT_DEVELOPER, "glTexSubImage2D %i, %i: %i msec\n", cols, rows, end - start );
	}

	/*
	   glBegin(GL_QUADS);
	   glVertexAttrib4f(ATTR_INDEX_TEXCOORD0, 0.5f / cols, 0.5f / rows, 0, 1);
	   glVertexAttrib4f(ATTR_INDEX_POSITION, x, y, 0, 1);
	   glVertexAttrib4f(ATTR_INDEX_TEXCOORD0, (cols - 0.5f) / cols, 0.5f / rows, 0, 1);
	   glVertexAttrib4f(ATTR_INDEX_POSITION, x + w, y, 0, 1);
	   glVertexAttrib4f(ATTR_INDEX_TEXCOORD0, (cols - 0.5f) / cols, (rows - 0.5f) / rows, 0, 1);
	   glVertexAttrib4f(ATTR_INDEX_POSITION, x + w, y + h, 0, 1);
	   glVertexAttrib4f(ATTR_INDEX_TEXCOORD0, 0.5f / cols, (rows - 0.5f) / rows, 0, 1);
	   glVertexAttrib4f(ATTR_INDEX_POSITION, x, y + h, 0, 1);
	   glEnd();
	 */

	tess.multiDrawPrimitives = 0;
	tess.numVertexes = 0;
	tess.numIndexes = 0;

	tess.xyz[ tess.numVertexes ][ 0 ] = x;
	tess.xyz[ tess.numVertexes ][ 1 ] = y;
	tess.xyz[ tess.numVertexes ][ 2 ] = 0;
	tess.xyz[ tess.numVertexes ][ 3 ] = 1;
	tess.texCoords[ tess.numVertexes ][ 0 ] = 0.5f / cols;
	tess.texCoords[ tess.numVertexes ][ 1 ] = 0.5f / rows;
	tess.texCoords[ tess.numVertexes ][ 2 ] = 0;
	tess.texCoords[ tess.numVertexes ][ 3 ] = 1;
	tess.numVertexes++;

	tess.xyz[ tess.numVertexes ][ 0 ] = x + w;
	tess.xyz[ tess.numVertexes ][ 1 ] = y;
	tess.xyz[ tess.numVertexes ][ 2 ] = 0;
	tess.xyz[ tess.numVertexes ][ 3 ] = 1;
	tess.texCoords[ tess.numVertexes ][ 0 ] = ( cols - 0.5f ) / cols;
	tess.texCoords[ tess.numVertexes ][ 1 ] = 0.5f / rows;
	tess.texCoords[ tess.numVertexes ][ 2 ] = 0;
	tess.texCoords[ tess.numVertexes ][ 3 ] = 1;
	tess.numVertexes++;

	tess.xyz[ tess.numVertexes ][ 0 ] = x + w;
	tess.xyz[ tess.numVertexes ][ 1 ] = y + h;
	tess.xyz[ tess.numVertexes ][ 2 ] = 0;
	tess.xyz[ tess.numVertexes ][ 3 ] = 1;
	tess.texCoords[ tess.numVertexes ][ 0 ] = ( cols - 0.5f ) / cols;
	tess.texCoords[ tess.numVertexes ][ 1 ] = ( rows - 0.5f ) / rows;
	tess.texCoords[ tess.numVertexes ][ 2 ] = 0;
	tess.texCoords[ tess.numVertexes ][ 3 ] = 1;
	tess.numVertexes++;

	tess.xyz[ tess.numVertexes ][ 0 ] = x;
	tess.xyz[ tess.numVertexes ][ 1 ] = y + h;
	tess.xyz[ tess.numVertexes ][ 2 ] = 0;
	tess.xyz[ tess.numVertexes ][ 3 ] = 1;
	tess.texCoords[ tess.numVertexes ][ 0 ] = 0.5f / cols;
	tess.texCoords[ tess.numVertexes ][ 1 ] = ( rows - 0.5f ) / rows;
	tess.texCoords[ tess.numVertexes ][ 2 ] = 0;
	tess.texCoords[ tess.numVertexes ][ 3 ] = 1;
	tess.numVertexes++;

	tess.indexes[ tess.numIndexes++ ] = 0;
	tess.indexes[ tess.numIndexes++ ] = 1;
	tess.indexes[ tess.numIndexes++ ] = 2;
	tess.indexes[ tess.numIndexes++ ] = 0;
	tess.indexes[ tess.numIndexes++ ] = 2;
	tess.indexes[ tess.numIndexes++ ] = 3;

	Tess_UpdateVBOs( ATTR_POSITION | ATTR_TEXCOORD );

	Tess_DrawElements();

	tess.multiDrawPrimitives = 0;
	tess.numVertexes = 0;
	tess.numIndexes = 0;

	GL_CheckErrors();
}

void RE_UploadCinematic( int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty )
{
	GL_Bind( tr.scratchImage[ client ] );

	// if the scratchImage isn't in the format we want, specify it as a new texture
	if ( cols != tr.scratchImage[ client ]->width || rows != tr.scratchImage[ client ]->height )
	{
		tr.scratchImage[ client ]->width = tr.scratchImage[ client ]->uploadWidth = cols;
		tr.scratchImage[ client ]->height = tr.scratchImage[ client ]->uploadHeight = rows;

		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB8, cols, rows, 0, GL_RGBA, GL_UNSIGNED_BYTE, data );

		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER );
		glTexParameterfv( GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, colorBlack );
	}
	else
	{
		if ( dirty )
		{
			// otherwise, just subimage upload it so that drivers can tell we are going to be changing
			// it and don't try and do a texture compression
			glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, cols, rows, GL_RGBA, GL_UNSIGNED_BYTE, data );
		}
	}

	GL_CheckErrors();
}

/*
=============
RB_SetColor
=============
*/
const void     *RB_SetColor( const void *data )
{
	const setColorCommand_t *cmd;

	GLimp_LogComment( "--- RB_SetColor ---\n" );

	cmd = ( const setColorCommand_t * ) data;

	backEnd.color2D[ 0 ] = cmd->color[ 0 ];
	backEnd.color2D[ 1 ] = cmd->color[ 1 ];
	backEnd.color2D[ 2 ] = cmd->color[ 2 ];
	backEnd.color2D[ 3 ] = cmd->color[ 3 ];

	return ( const void * )( cmd + 1 );
}

/*
=============
RB_StretchPic
=============
*/
const void     *RB_StretchPic( const void *data )
{
	int                       i;
	const stretchPicCommand_t *cmd;
	shader_t                  *shader;
	int                       numVerts, numIndexes;

	GLimp_LogComment( "--- RB_StretchPic ---\n" );

	cmd = ( const stretchPicCommand_t * ) data;

	if ( !backEnd.projection2D )
	{
		RB_SetGL2D();
	}

	shader = cmd->shader;

	if ( shader != tess.surfaceShader )
	{
		if ( tess.numIndexes )
		{
			Tess_End();
		}

		backEnd.currentEntity = &backEnd.entity2D;
		Tess_Begin( Tess_StageIteratorGeneric, NULL, shader, NULL, qfalse, qfalse, -1, 0 );
	}

	Tess_CheckOverflow( 4, 6 );
	numVerts = tess.numVertexes;
	numIndexes = tess.numIndexes;

	tess.numVertexes += 4;
	tess.numIndexes += 6;

	tess.indexes[ numIndexes ] = numVerts + 3;
	tess.indexes[ numIndexes + 1 ] = numVerts + 0;
	tess.indexes[ numIndexes + 2 ] = numVerts + 2;
	tess.indexes[ numIndexes + 3 ] = numVerts + 2;
	tess.indexes[ numIndexes + 4 ] = numVerts + 0;
	tess.indexes[ numIndexes + 5 ] = numVerts + 1;

	for ( i = 0; i < 4; i++ )
	{
		tess.colors[ numVerts + i ][ 0 ] = backEnd.color2D[ 0 ];
		tess.colors[ numVerts + i ][ 1 ] = backEnd.color2D[ 1 ];
		tess.colors[ numVerts + i ][ 2 ] = backEnd.color2D[ 2 ];
		tess.colors[ numVerts + i ][ 3 ] = backEnd.color2D[ 3 ];
	}

	tess.xyz[ numVerts ][ 0 ] = cmd->x;
	tess.xyz[ numVerts ][ 1 ] = cmd->y;
	tess.xyz[ numVerts ][ 2 ] = 0;
	tess.xyz[ numVerts ][ 3 ] = 1;

	tess.texCoords[ numVerts ][ 0 ] = cmd->s1;
	tess.texCoords[ numVerts ][ 1 ] = cmd->t1;
	tess.texCoords[ numVerts ][ 2 ] = 0;
	tess.texCoords[ numVerts ][ 3 ] = 1;

	tess.xyz[ numVerts + 1 ][ 0 ] = cmd->x + cmd->w;
	tess.xyz[ numVerts + 1 ][ 1 ] = cmd->y;
	tess.xyz[ numVerts + 1 ][ 2 ] = 0;
	tess.xyz[ numVerts + 1 ][ 3 ] = 1;

	tess.texCoords[ numVerts + 1 ][ 0 ] = cmd->s2;
	tess.texCoords[ numVerts + 1 ][ 1 ] = cmd->t1;
	tess.texCoords[ numVerts + 1 ][ 2 ] = 0;
	tess.texCoords[ numVerts + 1 ][ 3 ] = 1;

	tess.xyz[ numVerts + 2 ][ 0 ] = cmd->x + cmd->w;
	tess.xyz[ numVerts + 2 ][ 1 ] = cmd->y + cmd->h;
	tess.xyz[ numVerts + 2 ][ 2 ] = 0;
	tess.xyz[ numVerts + 2 ][ 3 ] = 1;

	tess.texCoords[ numVerts + 2 ][ 0 ] = cmd->s2;
	tess.texCoords[ numVerts + 2 ][ 1 ] = cmd->t2;
	tess.texCoords[ numVerts + 2 ][ 2 ] = 0;
	tess.texCoords[ numVerts + 2 ][ 3 ] = 1;

	tess.xyz[ numVerts + 3 ][ 0 ] = cmd->x;
	tess.xyz[ numVerts + 3 ][ 1 ] = cmd->y + cmd->h;
	tess.xyz[ numVerts + 3 ][ 2 ] = 0;
	tess.xyz[ numVerts + 3 ][ 3 ] = 1;

	tess.texCoords[ numVerts + 3 ][ 0 ] = cmd->s1;
	tess.texCoords[ numVerts + 3 ][ 1 ] = cmd->t2;
	tess.texCoords[ numVerts + 3 ][ 2 ] = 0;
	tess.texCoords[ numVerts + 3 ][ 3 ] = 1;

	return ( const void * )( cmd + 1 );
}

const void     *RB_Draw2dPolys( const void *data )
{
	const poly2dCommand_t *cmd;
	shader_t              *shader;
	int                   i;

	cmd = ( const poly2dCommand_t * ) data;

	if ( !backEnd.projection2D )
	{
		RB_SetGL2D();
	}

	shader = cmd->shader;

	if ( shader != tess.surfaceShader )
	{
		if ( tess.numIndexes )
		{
			Tess_End();
		}

		backEnd.currentEntity = &backEnd.entity2D;
		Tess_Begin( Tess_StageIteratorGeneric, NULL, shader, NULL, qfalse, qfalse, -1, 0 );
	}

	Tess_CheckOverflow( cmd->numverts, ( cmd->numverts - 2 ) * 3 );

	for ( i = 0; i < cmd->numverts - 2; i++ )
	{
		tess.indexes[ tess.numIndexes + 0 ] = tess.numVertexes;
		tess.indexes[ tess.numIndexes + 1 ] = tess.numVertexes + i + 1;
		tess.indexes[ tess.numIndexes + 2 ] = tess.numVertexes + i + 2;
		tess.numIndexes += 3;
	}

	for ( i = 0; i < cmd->numverts; i++ )
	{
		tess.xyz[ tess.numVertexes ][ 0 ] = cmd->verts[ i ].xyz[ 0 ];
		tess.xyz[ tess.numVertexes ][ 1 ] = cmd->verts[ i ].xyz[ 1 ];
		tess.xyz[ tess.numVertexes ][ 2 ] = 0;
		tess.xyz[ tess.numVertexes ][ 3 ] = 1;

		tess.texCoords[ tess.numVertexes ][ 0 ] = cmd->verts[ i ].st[ 0 ];
		tess.texCoords[ tess.numVertexes ][ 1 ] = cmd->verts[ i ].st[ 1 ];

		tess.colors[ tess.numVertexes ][ 0 ] = cmd->verts[ i ].modulate[ 0 ] * ( 1.0 / 255.0f );
		tess.colors[ tess.numVertexes ][ 1 ] = cmd->verts[ i ].modulate[ 1 ] * ( 1.0 / 255.0f );
		tess.colors[ tess.numVertexes ][ 2 ] = cmd->verts[ i ].modulate[ 2 ] * ( 1.0 / 255.0f );
		tess.colors[ tess.numVertexes ][ 3 ] = cmd->verts[ i ].modulate[ 3 ] * ( 1.0 / 255.0f );
		tess.numVertexes++;
	}

	return ( const void * )( cmd + 1 );
}

// NERVE - SMF

/*
=============
RB_RotatedPic
=============
*/
const void     *RB_RotatedPic( const void *data )
{
	const stretchPicCommand_t *cmd;
	shader_t                  *shader;
	int                       numVerts, numIndexes;
	float                     mx, my, cosA, sinA, cw, ch, sw, sh;

	cmd = ( const stretchPicCommand_t * ) data;

	if ( !backEnd.projection2D )
	{
		RB_SetGL2D();
	}

	shader = cmd->shader;

	if ( shader != tess.surfaceShader )
	{
		if ( tess.numIndexes )
		{
			Tess_End();
		}

		backEnd.currentEntity = &backEnd.entity2D;
		Tess_Begin( Tess_StageIteratorGeneric, NULL, shader, NULL, qfalse, qfalse, -1, 0 );
	}

	Tess_CheckOverflow( 4, 6 );
	numVerts = tess.numVertexes;
	numIndexes = tess.numIndexes;

	tess.numVertexes += 4;
	tess.numIndexes += 6;

	tess.indexes[ numIndexes ] = numVerts + 3;
	tess.indexes[ numIndexes + 1 ] = numVerts + 0;
	tess.indexes[ numIndexes + 2 ] = numVerts + 2;
	tess.indexes[ numIndexes + 3 ] = numVerts + 2;
	tess.indexes[ numIndexes + 4 ] = numVerts + 0;
	tess.indexes[ numIndexes + 5 ] = numVerts + 1;

	Vector4Copy( backEnd.color2D, tess.colors[ numVerts + 0 ] );
	Vector4Copy( backEnd.color2D, tess.colors[ numVerts + 1 ] );
	Vector4Copy( backEnd.color2D, tess.colors[ numVerts + 2 ] );
	Vector4Copy( backEnd.color2D, tess.colors[ numVerts + 3 ] );

	mx = cmd->x + ( cmd->w / 2 );
	my = cmd->y + ( cmd->h / 2 );
	cosA = cos( DEG2RAD( cmd->angle ) );
	sinA = sin( DEG2RAD( cmd->angle ) );
	cw = cosA * ( cmd->w / 2 );
	ch = cosA * ( cmd->h / 2 );
	sw = sinA * ( cmd->w / 2 );
	sh = sinA * ( cmd->h / 2 );

	tess.xyz[ numVerts ][ 0 ] = mx - cw - sh;
	tess.xyz[ numVerts ][ 1 ] = my + sw - ch;
	tess.xyz[ numVerts ][ 2 ] = 0;
	tess.xyz[ numVerts ][ 3 ] = 1;

	tess.texCoords[ numVerts ][ 0 ] = cmd->s1;
	tess.texCoords[ numVerts ][ 1 ] = cmd->t1;

	tess.xyz[ numVerts + 1 ][ 0 ] = mx + cw - sh;
	tess.xyz[ numVerts + 1 ][ 1 ] = my - sw - ch;
	tess.xyz[ numVerts + 1 ][ 2 ] = 0;
	tess.xyz[ numVerts + 1 ][ 3 ] = 1;

	tess.texCoords[ numVerts + 1 ][ 0 ] = cmd->s2;
	tess.texCoords[ numVerts + 1 ][ 1 ] = cmd->t1;

	tess.xyz[ numVerts + 2 ][ 0 ] = mx + cw + sh;
	tess.xyz[ numVerts + 2 ][ 1 ] = my - sw + ch;
	tess.xyz[ numVerts + 2 ][ 2 ] = 0;
	tess.xyz[ numVerts + 2 ][ 3 ] = 1;

	tess.texCoords[ numVerts + 2 ][ 0 ] = cmd->s2;
	tess.texCoords[ numVerts + 2 ][ 1 ] = cmd->t2;

	tess.xyz[ numVerts + 3 ][ 0 ] = mx - cw + sh;
	tess.xyz[ numVerts + 3 ][ 1 ] = my + sw + ch;
	tess.xyz[ numVerts + 3 ][ 2 ] = 0;
	tess.xyz[ numVerts + 3 ][ 3 ] = 1;

	tess.texCoords[ numVerts + 3 ][ 0 ] = cmd->s1;
	tess.texCoords[ numVerts + 3 ][ 1 ] = cmd->t2;

	return ( const void * )( cmd + 1 );
}

// -NERVE - SMF

/*
==============
RB_StretchPicGradient
==============
*/
const void     *RB_StretchPicGradient( const void *data )
{
	const stretchPicCommand_t *cmd;
	shader_t                  *shader;
	int                       numVerts, numIndexes;
	int                       i;

	cmd = ( const stretchPicCommand_t * ) data;

	if ( !backEnd.projection2D )
	{
		RB_SetGL2D();
	}

	shader = cmd->shader;

	if ( shader != tess.surfaceShader )
	{
		if ( tess.numIndexes )
		{
			Tess_End();
		}

		backEnd.currentEntity = &backEnd.entity2D;
		Tess_Begin( Tess_StageIteratorGeneric, NULL, shader, NULL, qfalse, qfalse, -1, 0 );
	}

	Tess_CheckOverflow( 4, 6 );
	numVerts = tess.numVertexes;
	numIndexes = tess.numIndexes;

	tess.numVertexes += 4;
	tess.numIndexes += 6;

	tess.indexes[ numIndexes ] = numVerts + 3;
	tess.indexes[ numIndexes + 1 ] = numVerts + 0;
	tess.indexes[ numIndexes + 2 ] = numVerts + 2;
	tess.indexes[ numIndexes + 3 ] = numVerts + 2;
	tess.indexes[ numIndexes + 4 ] = numVerts + 0;
	tess.indexes[ numIndexes + 5 ] = numVerts + 1;

	//*(int *)tess.vertexColors[numVerts].v = *(int *)tess.vertexColors[numVerts + 1].v = *(int *)backEnd.color2D;
	//*(int *)tess.vertexColors[numVerts + 2].v = *(int *)tess.vertexColors[numVerts + 3].v = *(int *)cmd->gradientColor;

	Vector4Copy( backEnd.color2D, tess.colors[ numVerts + 0 ] );
	Vector4Copy( backEnd.color2D, tess.colors[ numVerts + 1 ] );

	for ( i = 0; i < 4; i++ )
	{
		tess.colors[ numVerts + 2 ][ i ] = cmd->gradientColor[ i ] * ( 1.0f / 255.0f );
		tess.colors[ numVerts + 3 ][ i ] = cmd->gradientColor[ i ] * ( 1.0f / 255.0f );
	}

	tess.xyz[ numVerts ][ 0 ] = cmd->x;
	tess.xyz[ numVerts ][ 1 ] = cmd->y;
	tess.xyz[ numVerts ][ 2 ] = 0;
	tess.xyz[ numVerts ][ 3 ] = 1;

	tess.texCoords[ numVerts ][ 0 ] = cmd->s1;
	tess.texCoords[ numVerts ][ 1 ] = cmd->t1;

	tess.xyz[ numVerts + 1 ][ 0 ] = cmd->x + cmd->w;
	tess.xyz[ numVerts + 1 ][ 1 ] = cmd->y;
	tess.xyz[ numVerts + 1 ][ 2 ] = 0;
	tess.xyz[ numVerts + 1 ][ 3 ] = 1;

	tess.texCoords[ numVerts + 1 ][ 0 ] = cmd->s2;
	tess.texCoords[ numVerts + 1 ][ 1 ] = cmd->t1;

	tess.xyz[ numVerts + 2 ][ 0 ] = cmd->x + cmd->w;
	tess.xyz[ numVerts + 2 ][ 1 ] = cmd->y + cmd->h;
	tess.xyz[ numVerts + 2 ][ 2 ] = 0;
	tess.xyz[ numVerts + 2 ][ 3 ] = 1;

	tess.texCoords[ numVerts + 2 ][ 0 ] = cmd->s2;
	tess.texCoords[ numVerts + 2 ][ 1 ] = cmd->t2;

	tess.xyz[ numVerts + 3 ][ 0 ] = cmd->x;
	tess.xyz[ numVerts + 3 ][ 1 ] = cmd->y + cmd->h;
	tess.xyz[ numVerts + 3 ][ 2 ] = 0;
	tess.xyz[ numVerts + 3 ][ 3 ] = 1;

	tess.texCoords[ numVerts + 3 ][ 0 ] = cmd->s1;
	tess.texCoords[ numVerts + 3 ][ 1 ] = cmd->t2;

	return ( const void * )( cmd + 1 );
}

/*
=============
RB_DrawView
=============
*/
const void     *RB_DrawView( const void *data )
{
	const drawViewCommand_t *cmd;

	GLimp_LogComment( "--- RB_DrawView ---\n" );

	// finish any 2D drawing if needed
	if ( tess.numIndexes )
	{
		Tess_End();
	}

	cmd = ( const drawViewCommand_t * ) data;

	backEnd.refdef = cmd->refdef;
	backEnd.viewParms = cmd->viewParms;

	RB_RenderView();

	return ( const void * )( cmd + 1 );
}

/*
=============
RB_DrawBuffer
=============
*/
const void     *RB_DrawBuffer( const void *data )
{
	const drawBufferCommand_t *cmd;

	GLimp_LogComment( "--- RB_DrawBuffer ---\n" );

	cmd = ( const drawBufferCommand_t * ) data;

	GL_DrawBuffer( cmd->buffer );

	// clear screen for debugging
	if ( r_clear->integer )
	{
//      GL_ClearColor(1, 0, 0.5, 1);
		GL_ClearColor( 0, 0, 0, 1 );
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	}

	return ( const void * )( cmd + 1 );
}

/*
===============
RB_ShowImages

Draw all the images to the screen, on top of whatever
was there.  This is used to test for texture thrashing.

Also called by RE_EndRegistration
===============
*/
void RB_ShowImages( void )
{
	int     i;
	image_t *image;
	float   x, y, w, h;
	vec4_t  quadVerts[ 4 ];
	int     start, end;

	GLimp_LogComment( "--- RB_ShowImages ---\n" );

	if ( !backEnd.projection2D )
	{
		RB_SetGL2D();
	}

	glClear( GL_COLOR_BUFFER_BIT );

	glFinish();

	gl_genericShader->DisableAlphaTesting();
	gl_genericShader->DisablePortalClipping();
	gl_genericShader->DisableVertexSkinning();
	gl_genericShader->DisableVertexAnimation();
	gl_genericShader->DisableDeformVertexes();
	gl_genericShader->DisableTCGenEnvironment();

	gl_genericShader->BindProgram();

	GL_Cull( CT_TWO_SIDED );

	// set uniforms
	gl_genericShader->SetUniform_ColorModulate( CGEN_VERTEX, AGEN_VERTEX );
	gl_genericShader->SetUniform_ColorTextureMatrix( matrixIdentity );

	GL_SelectTexture( 0 );

	start = ri.Milliseconds();

	for ( i = 0; i < tr.images.currentElements; i++ )
	{
		image = ( image_t * ) Com_GrowListElement( &tr.images, i );

		/*
		   if(image->bits & (IF_RGBA16F | IF_RGBA32F | IF_LA16F | IF_LA32F))
		   {
		   // don't render float textures using FFP
		   continue;
		   }
		 */

		w = glConfig.vidWidth / 20;
		h = glConfig.vidHeight / 15;
		x = i % 20 * w;
		y = i / 20 * h;

		// show in proportional size in mode 2
		if ( r_showImages->integer == 2 )
		{
			w *= image->uploadWidth / 512.0f;
			h *= image->uploadHeight / 512.0f;
		}

		// bind u_ColorMap
		GL_Bind( image );

		Vector4Set( quadVerts[ 0 ], x, y, 0, 1 );
		Vector4Set( quadVerts[ 1 ], x + w, y, 0, 1 );
		Vector4Set( quadVerts[ 2 ], x + w, y + h, 0, 1 );
		Vector4Set( quadVerts[ 3 ], x, y + h, 0, 1 );

		Tess_InstantQuad( quadVerts );

		/*
		   glBegin(GL_QUADS);
		   glVertexAttrib4f(ATTR_INDEX_TEXCOORD0, 0, 0, 0, 1);
		   glVertexAttrib4f(ATTR_INDEX_POSITION, x, y, 0, 1);
		   glVertexAttrib4f(ATTR_INDEX_TEXCOORD0, 1, 0, 0, 1);
		   glVertexAttrib4f(ATTR_INDEX_POSITION, x + w, y, 0, 1);
		   glVertexAttrib4f(ATTR_INDEX_TEXCOORD0, 1, 1, 0, 1);
		   glVertexAttrib4f(ATTR_INDEX_POSITION, x + w, y + h, 0, 1);
		   glVertexAttrib4f(ATTR_INDEX_TEXCOORD0, 0, 1, 0, 1);
		   glVertexAttrib4f(ATTR_INDEX_POSITION, x, y + h, 0, 1);
		   glEnd();
		 */
	}

	glFinish();

	end = ri.Milliseconds();
	ri.Printf( PRINT_DEVELOPER, "%i msec to draw all images\n", end - start );

	GL_CheckErrors();
}

/*
=============
RB_SwapBuffers
=============
*/
const void     *RB_SwapBuffers( const void *data )
{
	const swapBuffersCommand_t *cmd;

	// finish any 2D drawing if needed
	if ( tess.numIndexes )
	{
		Tess_End();
	}

	// texture swapping test
	if ( r_showImages->integer )
	{
		RB_ShowImages();
	}

	cmd = ( const swapBuffersCommand_t * ) data;

	// we measure overdraw by reading back the stencil buffer and
	// counting up the number of increments that have happened
	if ( r_measureOverdraw->integer )
	{
		int           i;
		long          sum = 0;
		unsigned char *stencilReadback;

		stencilReadback = ( unsigned char * ) ri.Hunk_AllocateTempMemory( glConfig.vidWidth * glConfig.vidHeight );
		glReadPixels( 0, 0, glConfig.vidWidth, glConfig.vidHeight, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, stencilReadback );

		for ( i = 0; i < glConfig.vidWidth * glConfig.vidHeight; i++ )
		{
			sum += stencilReadback[ i ];
		}

		backEnd.pc.c_overDraw += sum;
		ri.Hunk_FreeTempMemory( stencilReadback );
	}

	if ( !glState.finishCalled )
	{
		glFinish();
	}

	GLimp_LogComment( "***************** RB_SwapBuffers *****************\n\n\n" );

	GLimp_EndFrame();

	backEnd.projection2D = qfalse;

	return ( const void * )( cmd + 1 );
}

//bani

/*
=============
RB_RenderToTexture
=============
*/
const void     *RB_RenderToTexture( const void *data )
{
	const renderToTextureCommand_t *cmd;

//  ri.Printf( PRINT_ALL, "RB_RenderToTexture\n" );

	cmd = ( const renderToTextureCommand_t * ) data;

	GL_Bind( cmd->image );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_GENERATE_MIPMAP_SGIS, GL_TRUE );
	glCopyTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, cmd->x, cmd->y, cmd->w, cmd->h, 0 );
//  glCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, cmd->x, cmd->y, cmd->w, cmd->h );

	return ( const void * )( cmd + 1 );
}

//bani

/*
=============
RB_Finish
=============
*/
const void     *RB_Finish( const void *data )
{
	const renderFinishCommand_t *cmd;

//  ri.Printf( PRINT_ALL, "RB_Finish\n" );

	cmd = ( const renderFinishCommand_t * ) data;

	glFinish();

	return ( const void * )( cmd + 1 );
}

/*
====================
RB_ExecuteRenderCommands

This function will be called synchronously if running without
smp extensions, or asynchronously by another thread.
====================
*/
void RB_ExecuteRenderCommands( const void *data )
{
	int t1, t2;

	GLimp_LogComment( "--- RB_ExecuteRenderCommands ---\n" );

	t1 = ri.Milliseconds();

	if ( !r_smp->integer || data == backEndData[ 0 ]->commands.cmds )
	{
		backEnd.smpFrame = 0;
	}
	else
	{
		backEnd.smpFrame = 1;
	}

	while ( 1 )
	{
		switch ( * ( const int * ) data )
		{
			case RC_SET_COLOR:
				data = RB_SetColor( data );
				break;

			case RC_STRETCH_PIC:
				data = RB_StretchPic( data );
				break;

			case RC_2DPOLYS:
				data = RB_Draw2dPolys( data );
				break;

			case RC_ROTATED_PIC:
				data = RB_RotatedPic( data );
				break;

			case RC_STRETCH_PIC_GRADIENT:
				data = RB_StretchPicGradient( data );
				break;

			case RC_DRAW_VIEW:
				data = RB_DrawView( data );
				break;

			case RC_DRAW_BUFFER:
				data = RB_DrawBuffer( data );
				break;

			case RC_SWAP_BUFFERS:
				data = RB_SwapBuffers( data );
				break;

			case RC_SCREENSHOT:
				data = RB_TakeScreenshotCmd( data );
				break;

			case RC_VIDEOFRAME:
				data = RB_TakeVideoFrameCmd( data );
				break;

			case RC_RENDERTOTEXTURE:
				data = RB_RenderToTexture( data );
				break;

			case RC_FINISH:
				data = RB_Finish( data );
				break;

			case RC_END_OF_LIST:
			default:
				// stop rendering on this thread
				t2 = ri.Milliseconds();
				backEnd.pc.msec = t2 - t1;
				return;
		}
	}
}

/*
================
RB_RenderThread
================
*/
void RB_RenderThread( void )
{
	const void *data;

	// wait for either a rendering command or a quit command
	while ( 1 )
	{
		// sleep until we have work to do
		data = GLimp_RendererSleep();

		if ( !data )
		{
			return; // all done, renderer is shutting down
		}

		renderThreadActive = qtrue;

		RB_ExecuteRenderCommands( data );

		renderThreadActive = qfalse;
	}
}
