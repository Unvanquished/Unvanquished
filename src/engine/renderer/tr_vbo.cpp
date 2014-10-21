/*
===========================================================================
Copyright (C) 2007-2011 Robert Beckebans <trebor_7@users.sourceforge.net>

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
// tr_vbo.c
#include "tr_local.h"

// "templates" for VBO vertex data layouts

// interleaved position and qtangents per frame/vertex in part 1
struct fmtVertexAnim1 {
	i16vec4_t position;
	i16vec4_t qtangents;
};
const GLsizei sizeVertexAnim1 = sizeof( struct fmtVertexAnim1 );
// interleaved texcoords and colour in part 2
struct fmtVertexAnim2 {
	i16vec2_t texcoord;
	u8vec4_t  colour;
};
const GLsizei sizeVertexAnim2 = sizeof( struct fmtVertexAnim2 );

// interleaved data: position, texcoord, colour, qtangent, bonefactors
struct fmtSkeletal {
	i16vec4_t position;
	i16vec2_t texcoord;
	u8vec4_t  colour;
	i16vec4_t qtangents;
	u16vec4_t boneFactors;
};
const GLsizei sizeSkeletal = sizeof( struct fmtSkeletal );

// interleaved data: position, colour, qtangent, texcoord
// -> struct shaderVertex_t in tr_local.h
const GLsizei sizeShaderVertex = sizeof( shaderVertex_t );


static uint32_t R_DeriveAttrBits( vboData_t data )
{
	uint32_t stateBits = 0;

	if ( data.xyz )
	{
		stateBits |= ATTR_POSITION;
	}

	if ( data.qtangent )
	{
		stateBits |= ATTR_QTANGENT;
	}

	if ( data.color )
	{
		stateBits |= ATTR_COLOR;
	}

	if ( data.st )
	{
		stateBits |= ATTR_TEXCOORD;
	}

	if ( data.boneIndexes && data.boneWeights )
	{
		stateBits |= ATTR_BONE_FACTORS;
	}

	if ( data.numFrames )
	{
		if ( data.xyz )
		{
			stateBits |= ATTR_POSITION2;
		}

		if ( data.qtangent )
		{
			stateBits |= ATTR_QTANGENT2;
		}
	}

	return stateBits;
}

void VBO::SetAttributeComponentType( uint32_t i, qboolean noLightCoords )
{
	if ( i == ATTR_INDEX_BONE_FACTORS )
	{
		attribs[ i ].componentType = GL_UNSIGNED_SHORT;
	}
	else if ( i == ATTR_INDEX_COLOR )
	{
		attribs[ i ].componentType = GL_UNSIGNED_BYTE;
	}
	else if ( i == ATTR_INDEX_TEXCOORD )
	{
		attribs[ i ].componentType = GL_HALF_FLOAT;
	}
	else if ( i == ATTR_INDEX_QTANGENT || i == ATTR_INDEX_QTANGENT2 )
	{
		attribs[ i ].componentType = GL_SHORT;
	}
	else
	{
		attribs[ i ].componentType = GL_FLOAT;
	}

	if ( i == ATTR_INDEX_COLOR || i == ATTR_INDEX_QTANGENT || i == ATTR_INDEX_QTANGENT2 )
	{
		attribs[ i ].normalize = GL_TRUE;
	}
	else
	{
		attribs[ i ].normalize = GL_FALSE;
	}
	
	if ( i == ATTR_INDEX_TEXCOORD && noLightCoords )
	{
		attribs[ i ].numComponents = 2;
	}
	else if ( i == ATTR_INDEX_POSITION || i == ATTR_INDEX_POSITION2 )
	{
		attribs[ i ].numComponents = 3;
	}
	else
	{
		attribs[ i ].numComponents = 4;
	}
}

void VBO::SetAttributeLayoutsVertexAnimation( )
{
	// part 1 is repeated for every frame
	GLsizei sizePart1 = sizeVertexAnim1 * vertexesNum * framesNum;
	GLsizei sizePart2 = sizeVertexAnim2 * vertexesNum;

	attribs[ ATTR_INDEX_POSITION ].numComponents = 4;
	attribs[ ATTR_INDEX_POSITION ].componentType = GL_SHORT;
	attribs[ ATTR_INDEX_POSITION ].normalize     = GL_TRUE;
	attribs[ ATTR_INDEX_POSITION ].ofs           = offsetof( struct fmtVertexAnim1, position );
	attribs[ ATTR_INDEX_POSITION ].realStride    = sizeVertexAnim1;
	attribs[ ATTR_INDEX_POSITION ].stride        = sizeVertexAnim1;
	attribs[ ATTR_INDEX_POSITION ].frameOffset   = sizeVertexAnim1 * vertexesNum;

	attribs[ ATTR_INDEX_QTANGENT ].numComponents = 4;
	attribs[ ATTR_INDEX_QTANGENT ].componentType = GL_SHORT;
	attribs[ ATTR_INDEX_QTANGENT ].normalize     = GL_TRUE;
	attribs[ ATTR_INDEX_QTANGENT ].ofs          = offsetof( struct fmtVertexAnim1, qtangents );
	attribs[ ATTR_INDEX_QTANGENT ].realStride   = sizeVertexAnim1;
	attribs[ ATTR_INDEX_QTANGENT ].stride       = sizeVertexAnim1;
	attribs[ ATTR_INDEX_QTANGENT ].frameOffset  = sizeVertexAnim1 * vertexesNum;

	// these use the same layout
	attribs[ ATTR_INDEX_POSITION2 ] = attribs[ ATTR_INDEX_POSITION ];
	attribs[ ATTR_INDEX_QTANGENT2 ] = attribs[ ATTR_INDEX_QTANGENT ];

	attribs[ ATTR_INDEX_TEXCOORD ].numComponents = 2;
	attribs[ ATTR_INDEX_TEXCOORD ].componentType = GL_HALF_FLOAT;
	attribs[ ATTR_INDEX_TEXCOORD ].normalize     = GL_FALSE;
	attribs[ ATTR_INDEX_TEXCOORD ].ofs          = sizePart1 + offsetof( struct fmtVertexAnim2, texcoord );
	attribs[ ATTR_INDEX_TEXCOORD ].realStride   = sizeVertexAnim2;
	attribs[ ATTR_INDEX_TEXCOORD ].stride       = sizeVertexAnim2;
	attribs[ ATTR_INDEX_TEXCOORD ].frameOffset  = 0;

	attribs[ ATTR_INDEX_COLOR ].numComponents   = 4;
	attribs[ ATTR_INDEX_COLOR ].componentType   = GL_UNSIGNED_BYTE;
	attribs[ ATTR_INDEX_COLOR ].normalize       = GL_TRUE;
	attribs[ ATTR_INDEX_COLOR ].ofs             = sizePart1 + offsetof( struct fmtVertexAnim2, colour );
	attribs[ ATTR_INDEX_COLOR ].realStride      = sizeVertexAnim2;
	attribs[ ATTR_INDEX_COLOR ].stride          = sizeVertexAnim2;
	attribs[ ATTR_INDEX_COLOR ].frameOffset     = 0;

	// total size
	size = sizePart1 + sizePart2;
}

void VBO::SetAttributeLayoutsSkeletal( )
{
	attribs[ ATTR_INDEX_POSITION ].numComponents = 4;
	attribs[ ATTR_INDEX_POSITION ].componentType = GL_SHORT;
	attribs[ ATTR_INDEX_POSITION ].normalize     = GL_TRUE;
	attribs[ ATTR_INDEX_POSITION ].ofs           = offsetof( struct fmtSkeletal, position );
	attribs[ ATTR_INDEX_POSITION ].realStride    = sizeSkeletal;
	attribs[ ATTR_INDEX_POSITION ].stride        = sizeSkeletal;
	attribs[ ATTR_INDEX_POSITION ].frameOffset   = 0;

	attribs[ ATTR_INDEX_TEXCOORD ].numComponents = 2;
	attribs[ ATTR_INDEX_TEXCOORD ].componentType = GL_HALF_FLOAT;
	attribs[ ATTR_INDEX_TEXCOORD ].normalize     = GL_FALSE;
	attribs[ ATTR_INDEX_TEXCOORD ].ofs          = offsetof( struct fmtSkeletal, texcoord );
	attribs[ ATTR_INDEX_TEXCOORD ].realStride   = sizeSkeletal;
	attribs[ ATTR_INDEX_TEXCOORD ].stride       = sizeSkeletal;
	attribs[ ATTR_INDEX_TEXCOORD ].frameOffset  = 0;

	attribs[ ATTR_INDEX_COLOR ].numComponents   = 4;
	attribs[ ATTR_INDEX_COLOR ].componentType   = GL_UNSIGNED_BYTE;
	attribs[ ATTR_INDEX_COLOR ].normalize       = GL_TRUE;
	attribs[ ATTR_INDEX_COLOR ].ofs             = offsetof( struct fmtSkeletal, colour );
	attribs[ ATTR_INDEX_COLOR ].realStride      = sizeSkeletal;
	attribs[ ATTR_INDEX_COLOR ].stride          = sizeSkeletal;
	attribs[ ATTR_INDEX_COLOR ].frameOffset     = 0;

	attribs[ ATTR_INDEX_QTANGENT ].numComponents = 4;
	attribs[ ATTR_INDEX_QTANGENT ].componentType = GL_SHORT;
	attribs[ ATTR_INDEX_QTANGENT ].normalize     = GL_TRUE;
	attribs[ ATTR_INDEX_QTANGENT ].ofs          = offsetof( struct fmtSkeletal, qtangents );
	attribs[ ATTR_INDEX_QTANGENT ].realStride   = sizeSkeletal;
	attribs[ ATTR_INDEX_QTANGENT ].stride       = sizeSkeletal;
	attribs[ ATTR_INDEX_QTANGENT ].frameOffset  = 0;

	attribs[ ATTR_INDEX_BONE_FACTORS ].numComponents = 4;
	attribs[ ATTR_INDEX_BONE_FACTORS ].componentType = GL_UNSIGNED_SHORT;
	attribs[ ATTR_INDEX_BONE_FACTORS ].normalize     = GL_FALSE;
	attribs[ ATTR_INDEX_BONE_FACTORS ].ofs           = offsetof( struct fmtSkeletal, boneFactors );
	attribs[ ATTR_INDEX_BONE_FACTORS ].realStride    = sizeSkeletal;
	attribs[ ATTR_INDEX_BONE_FACTORS ].stride        = sizeSkeletal;
	attribs[ ATTR_INDEX_BONE_FACTORS ].frameOffset   = 0;

	// total size
	size = sizeSkeletal * vertexesNum;
}

void VBO::SetAttributeLayoutsStatic( )
{
	attribs[ ATTR_INDEX_POSITION ].numComponents = 3;
	attribs[ ATTR_INDEX_POSITION ].componentType = GL_FLOAT;
	attribs[ ATTR_INDEX_POSITION ].normalize     = GL_FALSE;
	attribs[ ATTR_INDEX_POSITION ].ofs           = offsetof( shaderVertex_t, xyz );
	attribs[ ATTR_INDEX_POSITION ].realStride    = sizeShaderVertex;
	attribs[ ATTR_INDEX_POSITION ].stride        = sizeShaderVertex;
	attribs[ ATTR_INDEX_POSITION ].frameOffset   = 0;

	attribs[ ATTR_INDEX_COLOR ].numComponents   = 4;
	attribs[ ATTR_INDEX_COLOR ].componentType   = GL_UNSIGNED_BYTE;
	attribs[ ATTR_INDEX_COLOR ].normalize       = GL_TRUE;
	attribs[ ATTR_INDEX_COLOR ].ofs             = offsetof( shaderVertex_t, color );
	attribs[ ATTR_INDEX_COLOR ].realStride      = sizeShaderVertex;
	attribs[ ATTR_INDEX_COLOR ].stride          = sizeShaderVertex;
	attribs[ ATTR_INDEX_COLOR ].frameOffset     = 0;

	attribs[ ATTR_INDEX_QTANGENT ].numComponents = 4;
	attribs[ ATTR_INDEX_QTANGENT ].componentType = GL_SHORT;
	attribs[ ATTR_INDEX_QTANGENT ].normalize     = GL_TRUE;
	attribs[ ATTR_INDEX_QTANGENT ].ofs           = offsetof( shaderVertex_t, qtangents );
	attribs[ ATTR_INDEX_QTANGENT ].realStride    = sizeShaderVertex;
	attribs[ ATTR_INDEX_QTANGENT ].stride        = sizeShaderVertex;
	attribs[ ATTR_INDEX_QTANGENT ].frameOffset   = 0;

	attribs[ ATTR_INDEX_TEXCOORD ].numComponents = 4;
	attribs[ ATTR_INDEX_TEXCOORD ].componentType = GL_HALF_FLOAT;
	attribs[ ATTR_INDEX_TEXCOORD ].normalize     = GL_FALSE;
	attribs[ ATTR_INDEX_TEXCOORD ].ofs           = offsetof( shaderVertex_t, texCoords );
	attribs[ ATTR_INDEX_TEXCOORD ].realStride    = sizeShaderVertex;
	attribs[ ATTR_INDEX_TEXCOORD ].stride        = sizeShaderVertex;
	attribs[ ATTR_INDEX_TEXCOORD ].frameOffset   = 0;

	// total size
	size = sizeShaderVertex * vertexesNum;
}

void VBO::SetAttributeLayoutsPosition( )
{
	attribs[ ATTR_INDEX_POSITION ].numComponents = 3;
	attribs[ ATTR_INDEX_POSITION ].componentType = GL_FLOAT;
	attribs[ ATTR_INDEX_POSITION ].normalize     = GL_FALSE;
	attribs[ ATTR_INDEX_POSITION ].ofs           = 0;
	attribs[ ATTR_INDEX_POSITION ].realStride    = sizeof( vec3_t );
	attribs[ ATTR_INDEX_POSITION ].stride        = sizeof( vec3_t );
	attribs[ ATTR_INDEX_POSITION ].frameOffset   = 0;

	// total size
	size = sizeof( vec3_t ) * vertexesNum;
}

void VBO::SetAttributeLayouts( qboolean noLightCoords )
{
	uint32_t i;
	for ( i = 0; i < ATTR_INDEX_MAX; i++ )
	{
		SetAttributeComponentType( i, noLightCoords );
	}

	if ( layout == VBO_LAYOUT_VERTEX_ANIMATION )
	{
		SetAttributeLayoutsVertexAnimation( );
	}
	else if ( layout == VBO_LAYOUT_SKELETAL )
	{
		SetAttributeLayoutsSkeletal( );
	}
	else if ( layout == VBO_LAYOUT_STATIC )
	{
		SetAttributeLayoutsStatic( );
	}
	else if ( layout == VBO_LAYOUT_POSITION )
	{
		SetAttributeLayoutsPosition( );
	}
	else
	{
		ri.Error( ERR_DROP, S_COLOR_YELLOW "Unknown attribute layout for vbo: %s\n", name );
	}
}

// index has to be in range 0-255, weight has to be >= 0 and < 1
static inline unsigned short
boneFactor( int index, float weight ) {
	int scaledWeight = lrintf( weight * 256.0 );
	return (unsigned short)( ( index << 8 ) | MIN( scaledWeight, 255 ) );
}

void VBO::CopyVertexData( byte *outData, vboData_t inData )
{
	uint32_t v;

	if ( layout == VBO_LAYOUT_VERTEX_ANIMATION )
	{
		struct fmtVertexAnim1 *ptr = ( struct fmtVertexAnim1 * )outData;

		for ( v = 0; v < framesNum * vertexesNum; v++ )
		{

			if ( ( attribBits & ATTR_POSITION ) )
			{
				vec4_t tmp;
				VectorScale( inData.xyz[ v ], 1.0f / 512.0f, tmp);
				tmp[ 3 ] = 1.0f; // unused

				floatToSnorm16( tmp, ptr[ v ].position );
			}

			if ( ( attribBits & ATTR_QTANGENT ) )
			{
				Vector4Copy( inData.qtangent[ v ], ptr[ v ].qtangents );
			}
		}
	}

	for ( v = 0; v < vertexesNum; v++ )
	{
		if ( layout == VBO_LAYOUT_SKELETAL ) {
			struct fmtSkeletal *ptr = ( struct fmtSkeletal * )outData;
			if ( ( attribBits & ATTR_POSITION ) )
			{
				vec4_t tmp;
				VectorCopy( inData.xyz[ v ], tmp);
				tmp[ 3 ] = 1.0f; // unused

				floatToSnorm16( tmp, ptr[ v ].position );
			}

			if ( ( attribBits & ATTR_TEXCOORD ) )
			{
				Vector2Copy( inData.st[ v ], ptr[ v ].texcoord );
			}

			if ( ( attribBits & ATTR_COLOR ) )
			{
				Vector4Copy( inData.color[ v ], ptr[ v ].colour );
			}

			if ( ( attribBits & ATTR_QTANGENT ) )
			{
				Vector4Copy( inData.qtangent[ v ], ptr[ v ].qtangents );
			}

			if ( ( attribBits & ATTR_BONE_FACTORS ) )
			{
				uint32_t j;

				ptr[ v ].boneFactors[ 0 ] = boneFactor( inData.boneIndexes[ v ][ 0 ],
									1.0f - inData.boneWeights[ v ][ 0 ]);
				for ( j = 1; j < 4; j++ ) {
					ptr[ v ].boneFactors[ j ] = boneFactor( inData.boneIndexes[ v ][ j ],
										inData.boneWeights[ v ][ j ] );
				}
			}
		} else if ( layout == VBO_LAYOUT_STATIC ) {
			shaderVertex_t *ptr = ( shaderVertex_t * )outData;
			if ( ( attribBits & ATTR_POSITION ) )
			{
				VectorCopy( inData.xyz[ v ], ptr[ v ].xyz );
			}

			if ( ( attribBits & ATTR_COLOR ) )
			{
				Vector4Copy( inData.color[ v ], ptr[ v ].color );
			}

			if ( ( attribBits & ATTR_QTANGENT ) )
			{
				Vector4Copy( inData.qtangent[ v ], ptr[ v ].qtangents );
			}

			if ( ( attribBits & ATTR_TEXCOORD ) )
			{
				Vector4Copy( inData.stpq[ v ], ptr[ v ].texCoords );
			}
		} else if ( layout == VBO_LAYOUT_POSITION ) {
			vec3_t *ptr = ( vec3_t * )outData;
			if ( ( attribBits & ATTR_POSITION ) )
			{
				VectorCopy( inData.xyz[ v ], ptr[ v ] );
			}
		} else if ( layout == VBO_LAYOUT_VERTEX_ANIMATION ) {
			struct fmtVertexAnim2 *ptr = ( struct fmtVertexAnim2 * )( outData + ( framesNum * vertexesNum ) * sizeVertexAnim1 );

			if ( ( attribBits & ATTR_TEXCOORD ) )
			{
				Vector2Copy( inData.st[ v ], ptr[ v ].texcoord );
			}

			if ( ( attribBits & ATTR_COLOR ) )
			{
				Vector4Copy( inData.color[ v ], ptr[ v ].colour );
			}
		}
	}

}

#if defined( GLEW_ARB_buffer_storage ) && defined( GLEW_ARB_sync )
/*
============
R_InitRingbuffer
============
*/
static void R_InitRingbuffer( GLenum target, GLsizei elementSize,
			      GLsizei segmentElements, glRingbuffer_t *rb ) {
	GLsizei totalSize = elementSize * segmentElements * DYN_BUFFER_SEGMENTS;
	int i;

	glBufferStorage( target, totalSize, NULL,
			 GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT );
	rb->baseAddr = glMapBufferRange( target, 0, totalSize,
					 GL_MAP_WRITE_BIT |
					 GL_MAP_PERSISTENT_BIT |
					 GL_MAP_FLUSH_EXPLICIT_BIT );
	rb->elementSize = elementSize;
	rb->segmentElements = segmentElements;
	rb->activeSegment = 0;
	for( i = 1; i < DYN_BUFFER_SEGMENTS; i++ ) {
		rb->syncs[ i ] = glFenceSync( GL_SYNC_GPU_COMMANDS_COMPLETE, 0 );
	}
}

/*
============
R_RotateRingbuffer
============
*/
static GLsizei R_RotateRingbuffer( glRingbuffer_t *rb ) {
	rb->syncs[ rb->activeSegment ] = glFenceSync( GL_SYNC_GPU_COMMANDS_COMPLETE, 0 );

	rb->activeSegment++;
	if( rb->activeSegment >= DYN_BUFFER_SEGMENTS )
		rb->activeSegment = 0;

	// wait until next segment is ready in 1 sec intervals
	while( glClientWaitSync( rb->syncs[ rb->activeSegment ], GL_SYNC_FLUSH_COMMANDS_BIT,
				 10000000 ) == GL_TIMEOUT_EXPIRED ) {
		ri.Printf( PRINT_WARNING, "long wait for GL buffer" );
	};

	return rb->activeSegment * rb->segmentElements;
}

/*
============
R_ShutdownRingbuffer
============
*/
static void R_ShutdownRingbuffer( GLenum target, glRingbuffer_t *rb ) {
	int i;

	glUnmapBuffer( target );
	rb->baseAddr = NULL;

	for( i = 0; i < DYN_BUFFER_SEGMENTS; i++ ) {
		if( i == rb->activeSegment )
			continue;

		glDeleteSync( rb->syncs[ i ] );
	}
}
#endif

static void R_FreeVBOData( vboData_t data )
{
	if ( data.color )
	{
		ri.Hunk_FreeTempMemory( data.color );
	}

	if ( data.qtangent )
	{
		ri.Hunk_FreeTempMemory( data.qtangent );
	}

	if ( data.st )
	{
		ri.Hunk_FreeTempMemory( data.st );
	}

	if ( data.xyz )
	{
		ri.Hunk_FreeTempMemory( data.xyz );
	}
}

// without these fails at linking
std::list<VBO*> VBO::all;
std::list<IBO*> IBO::all;

VBO_IBO_Map vbo_ibo_pairs;

void VBO::AddToList()
{
	all.push_back( this );
	all_iter = all.end();
	all_iter--;
}

void IBO::AddToList()
{
	all.push_back( this );
	all_iter = all.end();
	all_iter--;
}

// CreateDynamicVBO
VBO::VBO( const char *name, int numVertexes, uint32_t stateBits, vboLayout_t layout )
{
	if ( !numVertexes )
	{
		// TODO: was possible to return NULL
		ri.Error( ERR_DROP, "CreateDynamicVBO: numVertexes is 0" );
	}

	if ( strlen( name ) >= MAX_QPATH )
	{
		ri.Error( ERR_DROP, "R_CreateDynamicVBO: \"%s\" is too long", name );
	}

	// make sure the render thread is stopped
	R_SyncRenderThread();

	AddToList();

	Q_strncpyz( this->name, name, sizeof( this->name ) );

	this->layout = layout;
	framesNum = 0;
	vertexesNum = numVertexes;
	attribBits = stateBits;
	usage = GL_DYNAMIC_DRAW;

	SetAttributeLayouts( qfalse );

	glGenBuffers( 1, &handle );
	Bind();

#if defined( GLEW_ARB_buffer_storage ) && defined( GLEW_ARB_sync )
	if( glConfig2.bufferStorageAvailable &&
	    glConfig2.syncAvailable ) {
		R_InitRingbuffer( GL_ARRAY_BUFFER, sizeof( shaderVertex_t ),
				  numVertexes, &tess.vertexRB );
	} else
#endif
	{
		glBufferData( GL_ARRAY_BUFFER, size, NULL, usage );
	}

	BindNull();

	GL_CheckErrors();
}

void VBO::LoadStatic( vboData_t vboData )
{
	byte   *data;

	glGenBuffers( 1, &handle );
	Bind();

#ifdef GLEW_ARB_buffer_storage
	if( glConfig2.bufferStorageAvailable ) {
		data = ( byte * ) ri.Hunk_AllocateTempMemory( size );
		CopyVertexData( data, vboData );
		glBufferStorage( GL_ARRAY_BUFFER, size, data, 0 );
		ri.Hunk_FreeTempMemory( data );
	} else
#endif
	{
		glBufferData( GL_ARRAY_BUFFER, size, NULL, usage );
		data = (byte *)glMapBuffer( GL_ARRAY_BUFFER, GL_WRITE_ONLY );
		CopyVertexData( data, vboData );
		glUnmapBuffer( GL_ARRAY_BUFFER );
	}

	BindNull();
	GL_CheckErrors();
}

// CreateStaticVBO
VBO::VBO( const char *name, vboData_t data, vboLayout_t layout )
{
	if ( strlen( name ) >= MAX_QPATH )
	{
		ri.Error( ERR_DROP, "R_CreateVBO: \"%s\" is too long", name );
	}
	
	AddToList();
	
	Q_strncpyz( this->name, name, sizeof( this->name ) );

	// make sure the render thread is stopped
	R_SyncRenderThread();

	this->layout = layout;
	vertexesNum = data.numVerts;
	framesNum = data.numFrames;
	attribBits = R_DeriveAttrBits( data );
	usage = GL_STATIC_DRAW;

	SetAttributeLayouts( data.noLightCoords );

	LoadStatic( data );
}

/*
============
R_CreateVBO2
============
*/
VBO::VBO( const char *name, int numVertexes, srfVert_t *verts, unsigned int stateBits )
{
	vboData_t vboData;

	if ( !numVertexes )
	{
		// TODO: was possible to return NULL
		ri.Error( ERR_DROP, "CreateVBO2: numVertexes is 0" );
	}

	if ( strlen( name ) >= MAX_QPATH )
	{
		ri.Error( ERR_DROP, "R_CreateVBO2: \"%s\" is too long", name );
	}

	// make sure the render thread is stopped
	R_SyncRenderThread();

	AddToList();

	Q_strncpyz( this->name, name, sizeof( this->name ) );

	layout = VBO_LAYOUT_STATIC;
	framesNum = 0;
	vertexesNum = numVertexes;
	attribBits = stateBits;
	usage = GL_STATIC_DRAW;

	vboData = CreateVBOData( verts, qfalse );

	SetAttributeLayouts( qfalse );
	
	LoadStatic( vboData );

	R_FreeVBOData( vboData );
}

VBO::~VBO()
{
	R_SyncRenderThread();
	all.erase(all_iter);
	if ( handle )
	{
		glDeleteBuffers( 1, &handle );
	}
}

IBO::~IBO()
{

	R_SyncRenderThread();
	all.erase(all_iter);
	if ( handle )
	{
		glDeleteBuffers( 1, &handle );
	}
}

void VBO::KillAll()
{
	std::list<VBO*>::iterator it;
	for ( it = all.begin() ; it != all.end(); it = all.begin() )
	{
		delete (*it);
	}
}

void IBO::KillAll()
{
	std::list<IBO*>::iterator it;
	for ( it = all.begin() ; it != all.end(); it = all.begin() )
	{
		delete (*it);
	}
}

vboData_t VBO::CreateVBOData( const srfVert_t *verts, qboolean noLightCoords )
{
	uint32_t v;
	vboData_t data;
	memset( &data, 0, sizeof( data ) );
	data.numVerts = vertexesNum;
	data.numFrames = framesNum;
	data.noLightCoords = noLightCoords;

	for ( v = 0; v < vertexesNum; v++ )
	{
		const srfVert_t *vert = verts + v;
		if ( ( attribBits & ATTR_POSITION ) )
		{
			if ( !data.xyz )
			{
				data.xyz = ( vec3_t * ) ri.Hunk_AllocateTempMemory( sizeof( *data.xyz ) * data.numVerts );
			}
			VectorCopy( vert->xyz, data.xyz[ v ] );
		}

		if ( ( attribBits & ATTR_TEXCOORD ) && noLightCoords )
		{
			if ( !data.st )
			{
				data.st = ( i16vec2_t * ) ri.Hunk_AllocateTempMemory( sizeof( i16vec2_t ) * data.numVerts );
			}
			data.st[ v ][ 0 ] = floatToHalf( vert->st[ 0 ] );
			data.st[ v ][ 1 ] = floatToHalf( vert->st[ 1 ] );
		}

		if ( ( attribBits & ATTR_TEXCOORD ) && !noLightCoords )
		{
			if ( !data.stpq )
			{
				data.stpq = ( i16vec4_t * ) ri.Hunk_AllocateTempMemory( sizeof( i16vec4_t ) * data.numVerts );
			}
			data.stpq[ v ][ 0 ] = floatToHalf( vert->st[ 0 ] );
			data.stpq[ v ][ 1 ] = floatToHalf( vert->st[ 1 ] );
			data.stpq[ v ][ 2 ] = floatToHalf( vert->lightmap[ 0 ] );
			data.stpq[ v ][ 3 ] = floatToHalf( vert->lightmap[ 1 ] );
		}

		if ( ( attribBits & ATTR_QTANGENT ) )
		{
			if ( !data.qtangent )
			{
				data.qtangent = ( i16vec4_t * ) ri.Hunk_AllocateTempMemory( sizeof( *data.qtangent ) * data.numVerts );
			}
			Vector4Copy( vert->qtangent, data.qtangent[ v ] );
		}

		if ( ( attribBits & ATTR_COLOR ) )
		{
			if ( !data.color )
			{
				data.color = ( u8vec4_t * ) ri.Hunk_AllocateTempMemory( sizeof( *data.color ) * data.numVerts );
			}
			Vector4Copy( vert->lightColor, data.color[ v ] );
		}
	}

	return data;
}

/*
============
R_CreateIBO
============
*/
IBO::IBO( const char *name, int numIndexes )
{
	if ( strlen( name ) >= MAX_QPATH )
	{
		ri.Error( ERR_DROP, "R_CreateIBO: \"%s\" is too long", name );
	}

	// make sure the render thread is stopped
	R_SyncRenderThread();

	AddToList();

	Q_strncpyz( this->name, name, sizeof( this->name ) );

	size = numIndexes * sizeof( glIndex_t );
	indexesNum = numIndexes;

	glGenBuffers( 1, &handle );
	Bind();

#if defined( GLEW_ARB_buffer_storage ) && defined( GLEW_ARB_sync )
	if( glConfig2.bufferStorageAvailable &&
	    glConfig2.syncAvailable ) {
		R_InitRingbuffer( GL_ELEMENT_ARRAY_BUFFER, sizeof( glIndex_t ),
				  numIndexes, &tess.indexRB );
	} else
#endif
	{
		glBufferData( GL_ELEMENT_ARRAY_BUFFER, size, NULL, GL_DYNAMIC_DRAW );
	}

	BindNull();
	GL_CheckErrors();
}

void IBO::LoadStatic( glIndex_t *indexes )
{
	glGenBuffers( 1, &handle );
	Bind();

#ifdef GLEW_ARB_buffer_storage
	if( glConfig2.bufferStorageAvailable ) {
		glBufferStorage( GL_ELEMENT_ARRAY_BUFFER, size, indexes, 0 );
	} else
#endif
	{
		glBufferData( GL_ELEMENT_ARRAY_BUFFER, size, indexes, GL_STATIC_DRAW );
	}

	BindNull();
	GL_CheckErrors();
}

/*
============
R_CreateIBO2
============
*/
IBO::IBO( const char *name, glIndex_t *indexes, int numIndexes )
{
	if ( !numIndexes )
	{
		// TODO: was possible to return NULL
		ri.Error( ERR_DROP, "CreateIBO2: numIndexes is 0" );
	}

	if ( strlen( name ) >= MAX_QPATH )
	{
		ri.Error( ERR_DROP, "R_CreateIBO: \"%s\" is too long", name );
	}

	// make sure the render thread is stopped
	R_SyncRenderThread();

	AddToList();

	Q_strncpyz( this->name, name, sizeof( this->name ) );

	size = numIndexes * sizeof( glIndex_t );
	indexesNum = numIndexes;

	LoadStatic( indexes );
}

IBO::IBO( const char *name, int numTriangles, srfTriangle_t *triangles )
{
	int           i, j;
	glIndex_t     *indexes;
	srfTriangle_t *tri;

	if ( !numTriangles )
	{
		// TODO: was possible to return NULL
		ri.Error( ERR_DROP, "CreateIBO2: numIndexes is 0" );
	}

	if ( strlen( name ) >= MAX_QPATH )
	{
		ri.Error( ERR_DROP, "R_CreateIBO2: \"%s\" is too long", name );
	}

	// make sure the render thread is stopped
	R_SyncRenderThread();

	AddToList();
	Q_strncpyz( this->name, name, sizeof( this->name ) );

	indexes = ( glIndex_t * ) ri.Hunk_AllocateTempMemory( numTriangles * 3 * sizeof( glIndex_t ) );

	for ( i = 0, tri = triangles; i < numTriangles; i++, tri++ )
	{
		for ( j = 0; j < 3; j++ )
		{
			indexes[ i * 3 + j ] = tri->indexes[ j ];
		}
	}

	size = numTriangles * 3 * sizeof( glIndex_t );
	indexesNum = numTriangles * 3;

	LoadStatic( indexes );

	ri.Hunk_FreeTempMemory( indexes );
}

/*
============
R_BindVBO
============
*/
void VBO::Bind()
{
	if ( r_logFile->integer )
	{
		// don't just call LogComment, or we will get a call to va() every frame!
		GLimp_LogComment( va( "--- R_BindVBO( %s ) ---\n", name ) );
	}

	if ( glState.currentVBO != this )
	{
		glState.currentVBO = this;
		glState.vertexAttribPointersSet = 0;

		glState.vertexAttribsInterpolation = -1;
		glState.vertexAttribsOldFrame = 0;
		glState.vertexAttribsNewFrame = 0;

		glBindBuffer( GL_ARRAY_BUFFER, handle );

		backEnd.pc.c_vboVertexBuffers++;
	}
}

/*
============
R_BindNullVBO
============
*/
void VBO::BindNull()
{
	GLimp_LogComment( "--- R_BindNullVBO ---\n" );

	if ( glState.currentVBO )
	{
		glBindBuffer( GL_ARRAY_BUFFER, 0 );
		glState.currentVBO = NULL;
	}

	GL_CheckErrors();
}

/*
============
R_BindIBO
============
*/
void IBO::Bind()
{
	if ( r_logFile->integer )
	{
		// don't just call LogComment, or we will get a call to va() every frame!
		GLimp_LogComment( va( "--- R_BindIBO( %s ) ---\n", name ) );
	}

	if ( glState.currentIBO != this )
	{
		glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, handle );

		glState.currentIBO = this;

		backEnd.pc.c_vboIndexBuffers++;
	}
}

/*
============
R_BindNullIBO
============
*/
void IBO::BindNull()
{
	GLimp_LogComment( "--- R_BindNullIBO ---\n" );

	if ( glState.currentIBO )
	{
		glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
		glState.currentIBO = NULL;
		glState.vertexAttribPointersSet = 0;
	}
}

static void R_InitUnitCubeVBO( void )
{
	vec3_t        mins = { -1, -1, -1 };
	vec3_t        maxs = { 1,  1,  1 };
	vboData_t     data;
	int           i;

	R_SyncRenderThread();

	tess.multiDrawPrimitives = 0;
	tess.numIndexes = 0;
	tess.numVertexes = 0;

	Tess_MapVBOs( qtrue );

	Tess_AddCube( vec3_origin, mins, maxs, colorWhite );

	memset( &data, 0, sizeof( data ) );
	data.xyz = ( vec3_t * ) ri.Hunk_AllocateTempMemory( tess.numVertexes * sizeof( *data.xyz ) );

	data.numVerts = tess.numVertexes;

	for ( i = 0; i < tess.numVertexes; i++ )
	{
		VectorCopy( tess.verts[ i ].xyz, data.xyz[ i ] );
	}

	tr.unitCubeVBO = new VBO( "unitCube_VBO", data, VBO_LAYOUT_POSITION );
	tr.unitCubeIBO = new IBO( "unitCube_IBO", tess.indexes, tess.numIndexes );

	ri.Hunk_FreeTempMemory( data.xyz );

	tess.multiDrawPrimitives = 0;
	tess.numIndexes = 0;
	tess.numVertexes = 0;
	tess.verts = NULL;
	tess.indexes = NULL;
}

const int vertexCapacity = DYN_BUFFER_SIZE / sizeof( shaderVertex_t );
const int indexCapacity = DYN_BUFFER_SIZE / sizeof( glIndex_t );

/*
============
R_InitVBOs
============
*/
void R_InitVBOs( void )
{
	uint32_t attribs = ATTR_POSITION | ATTR_TEXCOORD | ATTR_QTANGENT | ATTR_COLOR;

	ri.Printf( PRINT_DEVELOPER, "------- R_InitVBOs -------\n" );

	vbo_ibo_pairs.clear();
	VBO::KillAll();
	IBO::KillAll();

	tess.vertsBuffer = ( shaderVertex_t * ) Com_Allocate_Aligned( 64, SHADER_MAX_VERTEXES * sizeof( shaderVertex_t ) );
	tess.indexesBuffer = ( glIndex_t * ) Com_Allocate_Aligned( 64, SHADER_MAX_INDEXES * sizeof( glIndex_t ) );

	if( glConfig2.mapBufferRangeAvailable ) {
		tess.vbo = new VBO( "tessVertexArray_VBO", vertexCapacity, attribs, VBO_LAYOUT_STATIC );
		tess.ibo = new IBO( "tessVertexArray_IBO", indexCapacity );
		tess.vertsWritten = tess.indexesWritten = 0;
	} else {
		// use glBufferSubData to update VBO
		tess.vbo = new VBO( "tessVertexArray_VBO", SHADER_MAX_VERTEXES, attribs, VBO_LAYOUT_STATIC );
		tess.ibo = new IBO( "tessVertexArray_IBO", SHADER_MAX_INDEXES );
	}


	R_InitUnitCubeVBO();

	// allocate a PBO for color grade map transfers
	glGenBuffers( 1, &tr.colorGradePBO );
	glBindBuffer( GL_PIXEL_PACK_BUFFER, tr.colorGradePBO );
	glBufferData( GL_PIXEL_PACK_BUFFER,
		      REF_COLORGRADEMAP_STORE_SIZE * sizeof(u8vec4_t),
		      NULL, GL_STREAM_COPY );
	glBindBuffer( GL_PIXEL_PACK_BUFFER, 0 );

	GL_CheckErrors();
}

/*
============
R_ShutdownVBOs
============
*/
void R_ShutdownVBOs( void )
{
	ri.Printf( PRINT_DEVELOPER, "------- R_ShutdownVBOs -------\n" );

	if( !glConfig2.mapBufferRangeAvailable ) {
		// nothing
	}
#if defined( GLEW_ARB_buffer_storage ) && defined( GLEW_ARB_sync )
	else if( glConfig2.bufferStorageAvailable &&
		 glConfig2.syncAvailable ) {
		tess.vbo->Bind();
		R_ShutdownRingbuffer( GL_ARRAY_BUFFER, &tess.vertexRB );
		tess.ibo->Bind();
		R_ShutdownRingbuffer( GL_ELEMENT_ARRAY_BUFFER, &tess.indexRB );
	}
#endif
	else {
		if( tess.verts != NULL && tess.verts != tess.vertsBuffer ) {
			tess.vbo->Bind();
			glUnmapBuffer( GL_ARRAY_BUFFER );
		}

		if( tess.indexes != NULL && tess.indexes != tess.indexesBuffer ) {
			tess.ibo->Bind();
			glUnmapBuffer( GL_ELEMENT_ARRAY_BUFFER );
		}
	}

	VBO::BindNull();
	IBO::BindNull();

	glDeleteBuffers( 1, &tr.colorGradePBO );

	VBO::KillAll();
	IBO::KillAll();
	vbo_ibo_pairs.clear(); // all vbo and ibo already deleted

	Com_Free_Aligned( tess.vertsBuffer );
	Com_Free_Aligned( tess.indexesBuffer );

	tess.verts = tess.vertsBuffer = NULL;
	tess.indexes = tess.indexesBuffer = NULL;
}

/*
==============
Tess_MapVBOs

Map the default VBOs
==============
*/
void Tess_MapVBOs( qboolean forceCPU ) {
	if( forceCPU || !glConfig2.mapBufferRangeAvailable ) {
		// use host buffers
		tess.verts = tess.vertsBuffer;
		tess.indexes = tess.indexesBuffer;

		return;
	}

	if( tess.verts == NULL ) {
		tess.vbo->Bind();

#if defined( GLEW_ARB_buffer_storage ) && defined( GL_ARB_sync )
		if( glConfig2.bufferStorageAvailable &&
		    glConfig2.syncAvailable ) {
			GLsizei segmentEnd = (tess.vertexRB.activeSegment + 1) * tess.vertexRB.segmentElements;
			if( tess.vertsWritten + SHADER_MAX_VERTEXES > segmentEnd ) {
				tess.vertsWritten = R_RotateRingbuffer( &tess.vertexRB );
			}
			tess.verts = ( shaderVertex_t * )tess.vertexRB.baseAddr + tess.vertsWritten;
		} else
#endif
		{
			if( vertexCapacity - tess.vertsWritten < SHADER_MAX_VERTEXES ) {
				// buffer is full, allocate a new one
				glBufferData( GL_ARRAY_BUFFER, vertexCapacity * sizeof( shaderVertex_t ), NULL, GL_DYNAMIC_DRAW );
				tess.vertsWritten = 0;
			}
			tess.verts = ( shaderVertex_t *) glMapBufferRange( 
				GL_ARRAY_BUFFER, tess.vertsWritten * sizeof( shaderVertex_t ),
				SHADER_MAX_VERTEXES * sizeof( shaderVertex_t ),
				GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT |
				GL_MAP_UNSYNCHRONIZED_BIT | GL_MAP_FLUSH_EXPLICIT_BIT );
		}
	}

	if( tess.indexes == NULL ) {
		tess.ibo->Bind();

#if defined( GLEW_ARB_buffer_storage ) && defined( GL_ARB_sync )
		if( glConfig2.bufferStorageAvailable &&
		    glConfig2.syncAvailable ) {
			GLsizei segmentEnd = (tess.indexRB.activeSegment + 1) * tess.indexRB.segmentElements;
			if( tess.indexesWritten + SHADER_MAX_INDEXES > segmentEnd ) {
				tess.indexesWritten = R_RotateRingbuffer( &tess.indexRB );
			}
			tess.indexes = ( glIndex_t * )tess.indexRB.baseAddr + tess.indexesWritten;
		} else
#endif
		{
			if( indexCapacity - tess.indexesWritten < SHADER_MAX_INDEXES ) {
				// buffer is full, allocate a new one
				glBufferData( GL_ELEMENT_ARRAY_BUFFER, indexCapacity * sizeof( glIndex_t ), NULL, GL_DYNAMIC_DRAW );
				tess.indexesWritten = 0;
			}
			tess.indexes = ( glIndex_t *) glMapBufferRange( 
				GL_ELEMENT_ARRAY_BUFFER, tess.indexesWritten * sizeof( glIndex_t ),
				SHADER_MAX_INDEXES * sizeof( glIndex_t ),
				GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT |
				GL_MAP_UNSYNCHRONIZED_BIT | GL_MAP_FLUSH_EXPLICIT_BIT );
		}
	}
}

/*
==============
Tess_UpdateVBOs

Tr3B: update the default VBO to replace the client side vertex arrays
==============
*/
void Tess_UpdateVBOs( void )
{
	GLimp_LogComment( "--- Tess_UpdateVBOs( ) ---\n" );

	GL_CheckErrors();

	// update the default VBO
	if ( tess.numVertexes > 0 && tess.numVertexes <= SHADER_MAX_VERTEXES )
	{
		GLsizei size = tess.numVertexes * sizeof( shaderVertex_t );

		GL_CheckErrors();

		if ( r_logFile->integer )
		{
			GLimp_LogComment( va( "glBufferSubData( vbo = '%s', numVertexes = %i )\n", tess.vbo->GetName(), tess.numVertexes ) );
		}

		if( !glConfig2.mapBufferRangeAvailable ) {
			tess.vbo->Bind();
			glBufferSubData( GL_ARRAY_BUFFER, 0, size, tess.verts );
		} else {
			tess.vbo->Bind();
			if( glConfig2.bufferStorageAvailable &&
			    glConfig2.syncAvailable ) {
				GLsizei offset = tess.vertexBase * sizeof( shaderVertex_t );

				glFlushMappedBufferRange( GL_ARRAY_BUFFER,
							  offset, size );
			} else {
				glFlushMappedBufferRange( GL_ARRAY_BUFFER,
							  0, size );
				glUnmapBuffer( GL_ARRAY_BUFFER );
			}
			tess.vertexBase = tess.vertsWritten;
			tess.vertsWritten += tess.numVertexes;

			tess.verts = NULL;
		}
	}

	GL_CheckErrors();

	// update the default IBO
	if ( tess.numIndexes > 0 && tess.numIndexes <= SHADER_MAX_INDEXES )
	{
		GLsizei size = tess.numIndexes * sizeof( glIndex_t );

		if( !glConfig2.mapBufferRangeAvailable ) {
			tess.ibo->Bind();
			glBufferSubData( GL_ELEMENT_ARRAY_BUFFER, 0, size,
					 tess.indexes );
		} else {
			tess.ibo->Bind();

			if( glConfig2.bufferStorageAvailable &&
			    glConfig2.syncAvailable ) {
				GLsizei offset = tess.indexBase * sizeof( glIndex_t );

				glFlushMappedBufferRange( GL_ELEMENT_ARRAY_BUFFER,
							  offset, size );
			} else {
				glFlushMappedBufferRange( GL_ELEMENT_ARRAY_BUFFER,
							  0, size );
				glUnmapBuffer( GL_ELEMENT_ARRAY_BUFFER );
			}
			tess.indexBase = tess.indexesWritten;
			tess.indexesWritten += tess.numIndexes;

			tess.indexes = NULL;
		}
	}

	GL_CheckErrors();
}

void VBO::ReportList()
{
	int size = 0;
	int count = 0;
	for ( std::list<VBO*>::iterator it = all.begin(); it != all.end(); ++it )
	{
		ri.Printf( PRINT_ALL, "%d.%02d MB %s\n", (*it)->size / ( 1024 * 1024 ),
		           ( (*it)->size % ( 1024 * 1024 ) ) * 100 / ( 1024 * 1024 ), (*it)->name );
		size += (*it)->size;
		count++;
	}
	ri.Printf( PRINT_ALL, " %i total VBOs\n", count );
	ri.Printf( PRINT_ALL, " %d.%02d MB total vertices memory\n", size / ( 1024 * 1024 ),
	           ( size % ( 1024 * 1024 ) ) * 100 / ( 1024 * 1024 ) );
}

void IBO::ReportList()
{
	int size = 0;
	int count = 0;
	for ( std::list<IBO*>::iterator it = all.begin(); it != all.end(); ++it )
	{
		ri.Printf( PRINT_ALL, "%d.%02d MB %s\n", (*it)->size / ( 1024 * 1024 ),
		           ( (*it)->size % ( 1024 * 1024 ) ) * 100 / ( 1024 * 1024 ), (*it)->name );
		size += (*it)->size;
		count++;
	}
	ri.Printf( PRINT_ALL, " %i total IBOs\n", count );
	ri.Printf( PRINT_ALL, " %d.%02d MB total triangle indices memory\n", size / ( 1024 * 1024 ),
	           ( size % ( 1024 * 1024 ) ) * 100 / ( 1024 * 1024 ) );
}

/*
============
R_VBOList_f
============
*/
void R_VBOList_f( void )
{
	ri.Printf( PRINT_ALL, " size          name\n" );
	ri.Printf( PRINT_ALL, "----------------------------------------------------------\n" );

	VBO::ReportList();
	IBO::ReportList();
}
