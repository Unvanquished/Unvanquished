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

static uint32_t R_DeriveAttrBits( vboData_t data )
{
	uint32_t stateBits = 0;

	if ( data.xyz )
	{
		stateBits |= ATTR_POSITION;
	}

	if ( data.binormal )
	{
		stateBits |= ATTR_BINORMAL;
	}

	if ( data.tangent )
	{
		stateBits |= ATTR_TANGENT;
	}

	if ( data.normal )
	{
		stateBits |= ATTR_NORMAL;
	}

	if ( data.color )
	{
		stateBits |= ATTR_COLOR;
	}

	if ( data.lightDir )
	{
		stateBits |= ATTR_LIGHTDIRECTION;
	}

	if ( data.directedLight )
	{
		stateBits |= ATTR_DIRECTEDLIGHT;
	}

	if ( data.ambientLight )
	{
		stateBits |= ATTR_AMBIENTLIGHT;
	}

	if ( data.lightCoord )
	{
		stateBits |= ATTR_LIGHTCOORD;
	}

	if ( data.st )
	{
		stateBits |= ATTR_TEXCOORD;
	}

	if ( data.boneWeights )
	{
		stateBits |= ATTR_BONE_WEIGHTS;
	}

	if ( data.boneIndexes )
	{
		stateBits |= ATTR_BONE_INDEXES;
	}

	if ( data.numFrames )
	{
		if ( data.xyz )
		{
			stateBits |= ATTR_POSITION2;
		}

		if ( data.normal )
		{
			stateBits |= ATTR_NORMAL2;
		}

		if ( data.binormal )
		{
			stateBits |= ATTR_BINORMAL2;
		}

		if ( data.tangent )
		{
			stateBits |= ATTR_TANGENT2;
		}
	}

	return stateBits;
}

static void R_SetVBOAttributeComponentType( VBO_t *vbo, uint32_t i )
{
	if ( i == ATTR_INDEX_BONE_INDEXES )
	{
		vbo->attribs[ i ].componentType = GL_INT;
	}
	else
	{
		vbo->attribs[ i ].componentType = GL_FLOAT;
	}

	vbo->attribs[ i ].normalize = GL_FALSE;

	if ( i == ATTR_INDEX_TEXCOORD || i == ATTR_INDEX_LIGHTCOORD )
	{
		vbo->attribs[ i ].numComponents = 2;
	}
	else if ( i == ATTR_INDEX_POSITION || i == ATTR_INDEX_POSITION2 ||
	     i == ATTR_INDEX_TANGENT2 || i == ATTR_INDEX_NORMAL2 || i == ATTR_INDEX_BINORMAL2 ||
	     i == ATTR_INDEX_TANGENT || i == ATTR_INDEX_NORMAL || i == ATTR_INDEX_BINORMAL ||
	     i == ATTR_INDEX_AMBIENTLIGHT || i == ATTR_INDEX_DIRECTEDLIGHT || i == ATTR_INDEX_LIGHTDIRECTION )
	{
		vbo->attribs[ i ].numComponents = 3;
	}
	else
	{
		vbo->attribs[ i ].numComponents = 4;
	}
}

static size_t R_GetSizeForType( GLenum type )
{
	switch( type )
	{
		case GL_INT:
			return sizeof( int );
		case GL_FLOAT:
			return sizeof( float );
		case GL_UNSIGNED_BYTE:
			return sizeof( unsigned char );
		case GL_BYTE:
			return sizeof( signed char );
		default:
			Com_Error( ERR_FATAL, "R_GetSizeForType: ERROR unknown type\n" );
			return 0;
	}
}

static size_t R_GetAttribStorageSize( const VBO_t *vbo, uint32_t attribute )
{
	if ( vbo->usage == GL_STATIC_DRAW )
	{
		return vbo->attribs[ attribute ].numComponents * R_GetSizeForType( vbo->attribs[ attribute ].componentType );
	}
	else
	{
		if ( attribute == ATTR_INDEX_TEXCOORD || attribute == ATTR_INDEX_LIGHTCOORD )
		{
			return sizeof( vec2_t );
		}
		else
		{
			return sizeof( vec4_t );
		}
	}
}

static qboolean R_AttributeTightlyPacked( const VBO_t *vbo, uint32_t attribute )
{
	const vboAttributeLayout_t *layout = &vbo->attribs[ attribute ];

	return layout->numComponents * R_GetSizeForType( layout->componentType ) == layout->realStride;
}

static uint32_t R_FindVertexSize( VBO_t *vbo, uint32_t attribBits )
{
	uint32_t bits = attribBits & ~ATTR_INTERP_BITS;
	uint32_t size = 0;
	uint32_t attribute = 0;

	while ( bits )
	{
		if ( ( bits & 1 ) )
		{
			size += R_GetAttribStorageSize( vbo, attribute );
		}
		bits >>= 1;
		attribute++;
	}

	return size;
}

static void R_SetAttributeLayoutsInterleaved( VBO_t *vbo )
{
	uint32_t i;
	uint32_t offset = 0;
	uint32_t vertexSize = R_FindVertexSize( vbo, vbo->attribBits );

	for ( i = 0; i < ATTR_INDEX_MAX; i++ )
	{
		vbo->attribs[ i ].ofs = offset;
		vbo->attribs[ i ].realStride = vertexSize;

		if ( R_AttributeTightlyPacked( vbo, i ) )
		{
			vbo->attribs[ i ].stride = 0;
		}
		else
		{
			vbo->attribs[ i ].stride = vbo->attribs[ i ].realStride;
		}

		vbo->attribs[ i ].frameOffset = 0;

		if ( ( vbo->attribBits & BIT( i ) ) )
		{
			offset += R_GetAttribStorageSize( vbo, i );
		}
	}

	vbo->vertexesSize = vertexSize * vbo->vertexesNum;
}

static void R_SetAttributeLayoutsSeperate( VBO_t *vbo )
{
	uint32_t i;
	uint32_t offset = 0;

	for ( i = 0; i < ATTR_INDEX_MAX; i++ )
	{
		vbo->attribs[ i ].ofs = offset;
		vbo->attribs[ i ].realStride = R_GetAttribStorageSize( vbo, i );

		if ( R_AttributeTightlyPacked( vbo, i ) )
		{
			vbo->attribs[ i ].stride = 0;
		}
		else
		{
			vbo->attribs[ i ].stride = vbo->attribs[ i ].realStride;
		}

		vbo->attribs[ i ].frameOffset = 0;

		if ( ( vbo->attribBits & BIT( i ) ) )
		{
			offset += vbo->vertexesNum * vbo->attribs[ i ].realStride;
		}
	}

	vbo->vertexesSize = offset;
}

static void R_SetAttributeLayoutsVertexAnimation( VBO_t *vbo )
{
	int32_t i;
	uint32_t stride = 0;
	uint32_t offset = 0;
	uint32_t positionBits = ATTR_POSITION | ATTR_NORMAL | ATTR_TANGENT | ATTR_BINORMAL;

	// seperate the position attributes for animation purposes
	for ( i = 0; i < ATTR_INDEX_MAX; i++ )
	{
		uint32_t bit = BIT( i );
		if ( !( positionBits & bit ) )
		{
			continue;
		}

		if ( !( vbo->attribBits & bit ) )
		{
			continue;
		}

		vbo->attribs[ i ].ofs = offset;
		vbo->attribs[ i ].realStride = R_GetAttribStorageSize( vbo, i );

		if ( R_AttributeTightlyPacked( vbo, i ) )
		{
			vbo->attribs[ i ].stride = 0;
		}
		else
		{
			vbo->attribs[ i ].stride = vbo->attribs[ i ].realStride;
		}

		vbo->attribs[ i ].frameOffset = vbo->vertexesNum * vbo->attribs[ i ].realStride;

		offset += vbo->framesNum * vbo->attribs[ i ].frameOffset;
	}

	// these use the same layout
	vbo->attribs[ ATTR_INDEX_POSITION2 ] = vbo->attribs[ ATTR_INDEX_POSITION ];
	vbo->attribs[ ATTR_INDEX_NORMAL2 ] = vbo->attribs[ ATTR_INDEX_NORMAL ];
	vbo->attribs[ ATTR_INDEX_TANGENT2 ] = vbo->attribs[ ATTR_INDEX_TANGENT ];
	vbo->attribs[ ATTR_INDEX_BINORMAL2 ] = vbo->attribs[ ATTR_INDEX_BINORMAL ];

	// add the rest of the vertex attributes
	for ( i = 0; i < ATTR_INDEX_MAX; i++ )
	{
		uint32_t bit = BIT( i );
		if ( ( ( positionBits | ATTR_INTERP_BITS ) & bit ) )
		{
			continue;
		}

		vbo->attribs[ i ].ofs = offset;
		vbo->attribs[ i ].realStride = R_GetAttribStorageSize( vbo, i );

		if ( R_AttributeTightlyPacked( vbo, i ) )
		{
			vbo->attribs[ i ].stride = 0;
		}
		else
		{
			vbo->attribs[ i ].stride = vbo->attribs[ i ].realStride;
		}

		vbo->attribs[ i ].frameOffset = 0;

		if ( ( vbo->attribBits & bit ) )
		{
			offset += vbo->attribs[ i ].realStride * vbo->vertexesNum;
		}
	}

	vbo->vertexesSize = offset;
}

static void R_SetVBOAttributeLayouts( VBO_t *vbo )
{
	uint32_t i;
	for ( i = 0; i < ATTR_INDEX_MAX; i++ )
	{
		R_SetVBOAttributeComponentType( vbo, i );
	}

	if ( vbo->layout == VBO_LAYOUT_VERTEX_ANIMATION )
	{
		R_SetAttributeLayoutsVertexAnimation( vbo );
	}
	else if ( vbo->layout == VBO_LAYOUT_INTERLEAVED )
	{
		R_SetAttributeLayoutsInterleaved( vbo );
	}
	else if ( vbo->layout == VBO_LAYOUT_SEPERATE )
	{
		R_SetAttributeLayoutsSeperate( vbo );
	}
	else
	{
		ri.Error( ERR_DROP, S_COLOR_YELLOW "Unknown attribute layout for vbo: %s\n", vbo->name );
	}
}

static void R_CopyVertexData( VBO_t *vbo, byte *outData, vboData_t inData )
{
	uint32_t v;

#define VERTEXCOPY( v, attr, index, type ) \
	do \
	{ \
		uint32_t j; \
		type *tmp = ( type * ) ( outData + vbo->attribs[ index ].ofs + v * vbo->attribs[ index ].realStride ); \
		const type *vert = inData.attr[ v ]; \
		uint32_t len = ARRAY_LEN( *inData.attr ); \
		for ( j = 0; j < len; j++ ) { tmp[ j ] = vert[ j ]; } \
	} while ( 0 )

	if ( vbo->layout == VBO_LAYOUT_VERTEX_ANIMATION )
	{
		for ( v = 0; v < vbo->framesNum * vbo->vertexesNum; v++ )
		{
			if ( ( vbo->attribBits & ATTR_POSITION ) )
			{
				VERTEXCOPY( v, xyz, ATTR_INDEX_POSITION, float );
			}

			if ( ( vbo->attribBits & ATTR_NORMAL ) )
			{
				VERTEXCOPY( v, normal, ATTR_INDEX_NORMAL, float );
			}

			if ( ( vbo->attribBits & ATTR_BINORMAL ) )
			{
				VERTEXCOPY( v, binormal, ATTR_INDEX_BINORMAL, float );
			}

			if ( ( vbo->attribBits & ATTR_TANGENT ) )
			{
				VERTEXCOPY( v, tangent, ATTR_INDEX_TANGENT, float );
			}
		}
	}

	for ( v = 0; v < vbo->vertexesNum; v++ )
	{
		if ( vbo->layout != VBO_LAYOUT_VERTEX_ANIMATION )
		{
			if ( ( vbo->attribBits & ATTR_POSITION ) )
			{
				VERTEXCOPY( v, xyz, ATTR_INDEX_POSITION, float );
			}

			if ( ( vbo->attribBits & ATTR_NORMAL ) )
			{
				VERTEXCOPY( v, normal, ATTR_INDEX_NORMAL, float );
			}

			if ( ( vbo->attribBits & ATTR_BINORMAL ) )
			{
				VERTEXCOPY( v, binormal, ATTR_INDEX_BINORMAL, float );
			}

			if ( ( vbo->attribBits & ATTR_TANGENT ) )
			{
				VERTEXCOPY( v, tangent, ATTR_INDEX_TANGENT, float );
			}
		}

		if ( ( vbo->attribBits & ATTR_TEXCOORD ) )
		{
			VERTEXCOPY( v, st, ATTR_INDEX_TEXCOORD, float );
		}

		if ( ( vbo->attribBits & ATTR_LIGHTCOORD ) )
		{
			VERTEXCOPY( v, lightCoord, ATTR_INDEX_LIGHTCOORD, float );
		}

		if ( ( vbo->attribBits & ATTR_COLOR ) )
		{
			VERTEXCOPY( v, color, ATTR_INDEX_COLOR, float );
		}

		if ( ( vbo->attribBits & ATTR_AMBIENTLIGHT ) )
		{
			VERTEXCOPY( v, ambientLight, ATTR_INDEX_AMBIENTLIGHT, float );
		}

		if ( ( vbo->attribBits & ATTR_DIRECTEDLIGHT ) )
		{
			VERTEXCOPY( v, directedLight, ATTR_INDEX_DIRECTEDLIGHT, float );
		}

		if ( ( vbo->attribBits & ATTR_LIGHTDIRECTION ) )
		{
			VERTEXCOPY( v, lightDir, ATTR_INDEX_LIGHTDIRECTION, float );
		}

		if ( ( vbo->attribBits & ATTR_BONE_INDEXES ) )
		{
			VERTEXCOPY( v, boneIndexes, ATTR_INDEX_BONE_INDEXES, int );
		}

		if ( ( vbo->attribBits & ATTR_BONE_WEIGHTS ) )
		{
			VERTEXCOPY( v, boneWeights, ATTR_INDEX_BONE_WEIGHTS, float );
		}
	}

}

VBO_t *R_CreateDynamicVBO( const char *name, int numVertexes, uint32_t stateBits, vboLayout_t layout )
{
	VBO_t *vbo;

	if ( !numVertexes )
	{
		return NULL;
	}

	if ( strlen( name ) >= MAX_QPATH )
	{
		ri.Error( ERR_DROP, "R_CreateDynamicVBO: \"%s\" is too long", name );
	}

	// make sure the render thread is stopped
	R_SyncRenderThread();

	vbo = (VBO_t*) ri.Hunk_Alloc( sizeof( *vbo ), h_low );
	memset( vbo, 0, sizeof( *vbo ) );

	Com_AddToGrowList( &tr.vbos, vbo );

	Q_strncpyz( vbo->name, name, sizeof( vbo->name ) );

	vbo->layout = layout;
	vbo->framesNum = 0;
	vbo->vertexesNum = numVertexes;
	vbo->attribBits = stateBits;
	vbo->usage = GL_DYNAMIC_DRAW;

	R_SetVBOAttributeLayouts( vbo );

	glGenBuffers( 1, &vbo->vertexesVBO );

	glBindBuffer( GL_ARRAY_BUFFER, vbo->vertexesVBO );
	glBufferData( GL_ARRAY_BUFFER, vbo->vertexesSize, NULL, vbo->usage );
	glBindBuffer( GL_ARRAY_BUFFER, 0 );

	GL_CheckErrors();

	return vbo;
}

/*
============
R_CreateVBO
============
*/
VBO_t *R_CreateStaticVBO( const char *name, vboData_t data, vboLayout_t layout )
{
	VBO_t *vbo;
	byte *outData;

	if ( strlen( name ) >= MAX_QPATH )
	{
		ri.Error( ERR_DROP, "R_CreateVBO: \"%s\" is too long", name );
	}

	// make sure the render thread is stopped
	R_SyncRenderThread();

	vbo = (VBO_t*) ri.Hunk_Alloc( sizeof( *vbo ), h_low );
	memset( vbo, 0, sizeof( *vbo ) );

	Com_AddToGrowList( &tr.vbos, vbo );

	Q_strncpyz( vbo->name, name, sizeof( vbo->name ) );

	vbo->layout = layout;
	vbo->vertexesNum = data.numVerts;
	vbo->framesNum = data.numFrames;
	vbo->attribBits = R_DeriveAttrBits( data );
	vbo->usage = GL_STATIC_DRAW;

	R_SetVBOAttributeLayouts( vbo );

	outData = ( byte * ) ri.Hunk_AllocateTempMemory( vbo->vertexesSize );

	R_CopyVertexData( vbo, outData, data );

	glGenBuffers( 1, &vbo->vertexesVBO );

	glBindBuffer( GL_ARRAY_BUFFER, vbo->vertexesVBO );
	glBufferData( GL_ARRAY_BUFFER, vbo->vertexesSize, outData, vbo->usage );
	glBindBuffer( GL_ARRAY_BUFFER, 0 );

	GL_CheckErrors();

	ri.Hunk_FreeTempMemory( outData );

	return vbo;
}

static vboData_t R_CreateVBOData( const VBO_t *vbo, const srfVert_t *verts )
{
	uint32_t v;
	vboData_t data;
	memset( &data, 0, sizeof( data ) );
	data.numVerts = vbo->vertexesNum;
	data.numFrames = vbo->framesNum;

	for ( v = 0; v < vbo->vertexesNum; v++ )
	{
		const srfVert_t *vert = verts + v;
		if ( ( vbo->attribBits & ATTR_POSITION ) )
		{
			if ( !data.xyz )
			{
				data.xyz = ( vec3_t * ) ri.Hunk_AllocateTempMemory( sizeof( *data.xyz ) * data.numVerts );
			}
			VectorCopy( vert->xyz, data.xyz[ v ] );
		}

		if ( ( vbo->attribBits & ATTR_TEXCOORD ) )
		{
			if ( !data.st )
			{
				data.st = ( vec2_t * ) ri.Hunk_AllocateTempMemory( sizeof( *data.st ) * data.numVerts );
			}
			data.st[ v ][ 0 ] = vert->st[ 0 ];
			data.st[ v ][ 1 ] = vert->st[ 1 ];
		}

		if ( ( vbo->attribBits & ATTR_NORMAL ) )
		{
			if ( !data.normal )
			{
				data.normal = ( vec3_t * ) ri.Hunk_AllocateTempMemory( sizeof( *data.normal ) * data.numVerts );
			}
			VectorCopy( vert->normal, data.normal[ v ] );
		}

		if ( ( vbo->attribBits & ATTR_BINORMAL ) )
		{
			if ( !data.binormal )
			{
				data.binormal = ( vec3_t * ) ri.Hunk_AllocateTempMemory( sizeof( *data.binormal ) * data.numVerts );
			}
			VectorCopy( vert->binormal, data.binormal[ v ] );
		}

		if ( ( vbo->attribBits & ATTR_TANGENT ) )
		{
			if ( !data.tangent )
			{
				data.tangent = ( vec3_t * ) ri.Hunk_AllocateTempMemory( sizeof( *data.tangent ) * data.numVerts );
			}
			VectorCopy( vert->tangent, data.tangent[ v ] );
		}

		if ( ( vbo->attribBits & ATTR_LIGHTCOORD ) )
		{
			if ( !data.lightCoord )
			{
				data.lightCoord = ( vec2_t * ) ri.Hunk_AllocateTempMemory( sizeof( *data.lightCoord ) * data.numVerts );
			}
			data.lightCoord[ v ][ 0 ] = vert->lightmap[ 0 ];
			data.lightCoord[ v ][ 1 ] = vert->lightmap[ 1 ];
		}

		if ( ( vbo->attribBits & ATTR_COLOR ) )
		{
			if ( !data.color )
			{
				data.color = ( vec4_t * ) ri.Hunk_AllocateTempMemory( sizeof( *data.color ) * data.numVerts );
			}
			Vector4Copy( vert->lightColor, data.color[ v ] );
		}
	}

	return data;
}

static void R_FreeVBOData( vboData_t data )
{
	if ( data.color )
	{
		ri.Hunk_FreeTempMemory( data.color );
	}

	if ( data.lightCoord )
	{
		ri.Hunk_FreeTempMemory( data.lightCoord );
	}

	if ( data.tangent )
	{
		ri.Hunk_FreeTempMemory( data.tangent );
	}

	if ( data.binormal )
	{
		ri.Hunk_FreeTempMemory( data.binormal );
	}

	if ( data.normal )
	{
		ri.Hunk_FreeTempMemory( data.normal );
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

/*
============
R_CreateVBO2
============
*/
VBO_t *R_CreateStaticVBO2( const char *name, int numVertexes, srfVert_t *verts, unsigned int stateBits )
{
	VBO_t  *vbo;

	byte   *data;
	vboData_t vboData;

	if ( !numVertexes )
	{
		return NULL;
	}

	if ( strlen( name ) >= MAX_QPATH )
	{
		ri.Error( ERR_DROP, "R_CreateVBO2: \"%s\" is too long", name );
	}

	// make sure the render thread is stopped
	R_SyncRenderThread();

	vbo = ( VBO_t * ) ri.Hunk_Alloc( sizeof( *vbo ), h_low );
	memset( vbo, 0, sizeof( *vbo ) );

	Com_AddToGrowList( &tr.vbos, vbo );

	Q_strncpyz( vbo->name, name, sizeof( vbo->name ) );

	vbo->layout = VBO_LAYOUT_SEPERATE;
	vbo->framesNum = 0;
	vbo->vertexesNum = numVertexes;
	vbo->attribBits = stateBits;
	vbo->usage = GL_STATIC_DRAW;

	vboData = R_CreateVBOData( vbo, verts );

	R_SetVBOAttributeLayouts( vbo );
	
	data = ( byte * ) ri.Hunk_AllocateTempMemory( vbo->vertexesSize );

	R_CopyVertexData( vbo, data, vboData );

	glGenBuffers( 1, &vbo->vertexesVBO );

	glBindBuffer( GL_ARRAY_BUFFER, vbo->vertexesVBO );
	glBufferData( GL_ARRAY_BUFFER, vbo->vertexesSize, data, vbo->usage );
	glBindBuffer( GL_ARRAY_BUFFER, 0 );

	GL_CheckErrors();

	ri.Hunk_FreeTempMemory( data );
	R_FreeVBOData( vboData );

	return vbo;
}

/*
============
R_CreateIBO
============
*/
IBO_t *R_CreateDynamicIBO( const char *name, int numIndexes )
{
	IBO_t *ibo;

	if ( strlen( name ) >= MAX_QPATH )
	{
		ri.Error( ERR_DROP, "R_CreateIBO: \"%s\" is too long", name );
	}

	// make sure the render thread is stopped
	R_SyncRenderThread();

	ibo = (IBO_t*) ri.Hunk_Alloc( sizeof( *ibo ), h_low );
	Com_AddToGrowList( &tr.ibos, ibo );

	Q_strncpyz( ibo->name, name, sizeof( ibo->name ) );

	ibo->indexesSize = numIndexes * sizeof( glIndex_t );
	ibo->indexesNum = numIndexes;

	glGenBuffers( 1, &ibo->indexesVBO );

	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ibo->indexesVBO );
	glBufferData( GL_ELEMENT_ARRAY_BUFFER, ibo->indexesSize, NULL, GL_DYNAMIC_DRAW );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );

	GL_CheckErrors();

	return ibo;
}

/*
============
R_CreateIBO2
============
*/
IBO_t *R_CreateStaticIBO( const char *name, glIndex_t *indexes, int numIndexes )
{
	IBO_t         *ibo;

	if ( !numIndexes )
	{
		return NULL;
	}

	if ( strlen( name ) >= MAX_QPATH )
	{
		ri.Error( ERR_DROP, "R_CreateIBO: \"%s\" is too long", name );
	}

	// make sure the render thread is stopped
	R_SyncRenderThread();

	ibo = ( IBO_t * ) ri.Hunk_Alloc( sizeof( *ibo ), h_low );
	Com_AddToGrowList( &tr.ibos, ibo );

	Q_strncpyz( ibo->name, name, sizeof( ibo->name ) );

	ibo->indexesSize = numIndexes * sizeof( glIndex_t );
	ibo->indexesNum = numIndexes;

	glGenBuffers( 1, &ibo->indexesVBO );

	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ibo->indexesVBO );
	glBufferData( GL_ELEMENT_ARRAY_BUFFER, ibo->indexesSize, indexes, GL_STATIC_DRAW );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );

	GL_CheckErrors();

	return ibo;
}

IBO_t *R_CreateStaticIBO2( const char *name, int numTriangles, srfTriangle_t *triangles )
{
	IBO_t         *ibo;
	int           i, j;

	glIndex_t     *indexes;

	srfTriangle_t *tri;

	if ( !numTriangles )
	{
		return NULL;
	}

	if ( strlen( name ) >= MAX_QPATH )
	{
		ri.Error( ERR_DROP, "R_CreateIBO2: \"%s\" is too long", name );
	}

	// make sure the render thread is stopped
	R_SyncRenderThread();

	indexes = ( glIndex_t * ) ri.Hunk_AllocateTempMemory( numTriangles * 3 * sizeof( glIndex_t ) );

	for ( i = 0, tri = triangles; i < numTriangles; i++, tri++ )
	{
		for ( j = 0; j < 3; j++ )
		{
			indexes[ i * 3 + j ] = tri->indexes[ j ];
		}
	}

	ibo = R_CreateStaticIBO( name, indexes, numTriangles * 3 );

	ri.Hunk_FreeTempMemory( indexes );

	return ibo;
}

/*
============
R_BindVBO
============
*/
void R_BindVBO( VBO_t *vbo )
{
	if ( !vbo )
	{
		//R_BindNullVBO();
		ri.Error( ERR_DROP, "R_BindNullVBO: NULL vbo" );
	}

	if ( r_logFile->integer )
	{
		// don't just call LogComment, or we will get a call to va() every frame!
		GLimp_LogComment( va( "--- R_BindVBO( %s ) ---\n", vbo->name ) );
	}

	if ( glState.currentVBO != vbo )
	{
		glState.currentVBO = vbo;
		glState.vertexAttribPointersSet = 0;

		glState.vertexAttribsInterpolation = -1;
		glState.vertexAttribsOldFrame = 0;
		glState.vertexAttribsNewFrame = 0;

		glBindBuffer( GL_ARRAY_BUFFER, vbo->vertexesVBO );

		backEnd.pc.c_vboVertexBuffers++;

		//GL_VertexAttribPointers(ATTR_BITS);
	}
}

/*
============
R_BindNullVBO
============
*/
void R_BindNullVBO( void )
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
void R_BindIBO( IBO_t *ibo )
{
	if ( !ibo )
	{
		//R_BindNullIBO();
		ri.Error( ERR_DROP, "R_BindIBO: NULL ibo" );
	}

	if ( r_logFile->integer )
	{
		// don't just call LogComment, or we will get a call to va() every frame!
		GLimp_LogComment( va( "--- R_BindIBO( %s ) ---\n", ibo->name ) );
	}

	if ( glState.currentIBO != ibo )
	{
		glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ibo->indexesVBO );

		glState.currentIBO = ibo;

		backEnd.pc.c_vboIndexBuffers++;
	}
}

/*
============
R_BindNullIBO
============
*/
void R_BindNullIBO( void )
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

	Tess_AddCube( vec3_origin, mins, maxs, colorWhite );

	memset( &data, 0, sizeof( data ) );
	data.xyz = ( vec3_t * ) ri.Hunk_AllocateTempMemory( tess.numVertexes * sizeof( *data.xyz ) );

	data.numVerts = tess.numVertexes;

	for ( i = 0; i < tess.numVertexes; i++ )
	{
		VectorCopy( tess.xyz[ i ], data.xyz[ i ] );
	}

	tr.unitCubeVBO = R_CreateStaticVBO( "unitCube_VBO", data, VBO_LAYOUT_SEPERATE );
	tr.unitCubeIBO = R_CreateStaticIBO( "unitCube_IBO", tess.indexes, tess.numIndexes );

	ri.Hunk_FreeTempMemory( data.xyz );

	tess.multiDrawPrimitives = 0;
	tess.numIndexes = 0;
	tess.numVertexes = 0;
}

/*
============
R_InitVBOs
============
*/
void R_InitVBOs( void )
{
	uint32_t attribs = ATTR_POSITION | ATTR_TEXCOORD | ATTR_BINORMAL 
	                   | ATTR_TANGENT | ATTR_NORMAL  | ATTR_LIGHTCOORD 
#if !defined( COMPAT_Q3A ) && !defined( COMPAT_ET )
	                   | ATTR_PAINTCOLOR
#endif
	                   | ATTR_COLOR | ATTR_AMBIENTLIGHT| ATTR_DIRECTEDLIGHT | ATTR_LIGHTDIRECTION;

	ri.Printf( PRINT_DEVELOPER, "------- R_InitVBOs -------\n" );

	Com_InitGrowList( &tr.vbos, 100 );
	Com_InitGrowList( &tr.ibos, 100 );

	tess.vbo = R_CreateDynamicVBO( "tessVertexArray_VBO", SHADER_MAX_VERTEXES, attribs, VBO_LAYOUT_SEPERATE );

	tess.vbo->attribs[ ATTR_INDEX_POSITION ].frameOffset = sizeof( tess.xyz );
	tess.vbo->attribs[ ATTR_INDEX_TANGENT ].frameOffset = sizeof( tess.tangents );
	tess.vbo->attribs[ ATTR_INDEX_BINORMAL ].frameOffset = sizeof( tess.binormals );
	tess.vbo->attribs[ ATTR_INDEX_NORMAL ].frameOffset = sizeof( tess.normals );
	tess.vbo->attribs[ ATTR_INDEX_POSITION2 ].frameOffset = sizeof( tess.xyz );
	tess.vbo->attribs[ ATTR_INDEX_TANGENT2 ].frameOffset = sizeof( tess.tangents );
	tess.vbo->attribs[ ATTR_INDEX_BINORMAL2 ].frameOffset = sizeof( tess.binormals );
	tess.vbo->attribs[ ATTR_INDEX_NORMAL2 ].frameOffset = sizeof( tess.normals );

	tess.ibo = R_CreateDynamicIBO( "tessVertexArray_IBO", SHADER_MAX_INDEXES );

	R_InitUnitCubeVBO();

	// allocate a PBO for color grade map transfers
	glGenBuffers( 1, &tr.colorGradePBO );
	glBindBuffer( GL_PIXEL_PACK_BUFFER, tr.colorGradePBO );
	glBufferData( GL_PIXEL_PACK_BUFFER,
		      REF_COLORGRADEMAP_STORE_SIZE * sizeof(color4ub_t),
		      NULL, GL_STREAM_COPY );
	glBindBuffer( GL_PIXEL_PACK_BUFFER, 0 );

	R_BindNullVBO();
	R_BindNullIBO();

	GL_CheckErrors();
}

/*
============
R_ShutdownVBOs
============
*/
void R_ShutdownVBOs( void )
{
	int   i;
	VBO_t *vbo;
	IBO_t *ibo;

	ri.Printf( PRINT_DEVELOPER, "------- R_ShutdownVBOs -------\n" );

	R_BindNullVBO();
	R_BindNullIBO();

	glDeleteBuffers( 1, &tr.colorGradePBO );

	for ( i = 0; i < tr.vbos.currentElements; i++ )
	{
		vbo = ( VBO_t * ) Com_GrowListElement( &tr.vbos, i );

		if ( vbo->vertexesVBO )
		{
			glDeleteBuffers( 1, &vbo->vertexesVBO );
		}
	}

	for ( i = 0; i < tr.ibos.currentElements; i++ )
	{
		ibo = ( IBO_t * ) Com_GrowListElement( &tr.ibos, i );

		if ( ibo->indexesVBO )
		{
			glDeleteBuffers( 1, &ibo->indexesVBO );
		}
	}

	Com_DestroyGrowList( &tr.vbos );
	Com_DestroyGrowList( &tr.ibos );
}

/*
============
R_VBOList_f
============
*/
void R_VBOList_f( void )
{
	int   i;
	VBO_t *vbo;
	IBO_t *ibo;
	int   vertexesSize = 0;
	int   indexesSize = 0;

	ri.Printf( PRINT_ALL, " size          name\n" );
	ri.Printf( PRINT_ALL, "----------------------------------------------------------\n" );

	for ( i = 0; i < tr.vbos.currentElements; i++ )
	{
		vbo = ( VBO_t * ) Com_GrowListElement( &tr.vbos, i );

		ri.Printf( PRINT_ALL, "%d.%02d MB %s\n", vbo->vertexesSize / ( 1024 * 1024 ),
		           ( vbo->vertexesSize % ( 1024 * 1024 ) ) * 100 / ( 1024 * 1024 ), vbo->name );

		vertexesSize += vbo->vertexesSize;
	}

	for ( i = 0; i < tr.ibos.currentElements; i++ )
	{
		ibo = ( IBO_t * ) Com_GrowListElement( &tr.ibos, i );

		ri.Printf( PRINT_ALL, "%d.%02d MB %s\n", ibo->indexesSize / ( 1024 * 1024 ),
		           ( ibo->indexesSize % ( 1024 * 1024 ) ) * 100 / ( 1024 * 1024 ), ibo->name );

		indexesSize += ibo->indexesSize;
	}

	ri.Printf( PRINT_ALL, " %i total VBOs\n", tr.vbos.currentElements );
	ri.Printf( PRINT_ALL, " %d.%02d MB total vertices memory\n", vertexesSize / ( 1024 * 1024 ),
	           ( vertexesSize % ( 1024 * 1024 ) ) * 100 / ( 1024 * 1024 ) );

	ri.Printf( PRINT_ALL, " %i total IBOs\n", tr.ibos.currentElements );
	ri.Printf( PRINT_ALL, " %d.%02d MB total triangle indices memory\n", indexesSize / ( 1024 * 1024 ),
	           ( indexesSize % ( 1024 * 1024 ) ) * 100 / ( 1024 * 1024 ) );
}
