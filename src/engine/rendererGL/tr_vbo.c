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

/*
============
R_CreateVBO
============
*/
VBO_t          *R_CreateVBO( const char *name, byte *vertexes, int vertexesSize, vboUsage_t usage )
{
	VBO_t *vbo;
	int   glUsage;

	switch ( usage )
	{
		case VBO_USAGE_STATIC:
			glUsage = GL_STATIC_DRAW_ARB;
			break;

		case VBO_USAGE_DYNAMIC:
			glUsage = GL_DYNAMIC_DRAW_ARB;
			break;

		default:
			glUsage = 0; //Prevents warning
			Com_Error( ERR_FATAL, "bad vboUsage_t given: %i", usage );
	}

	if ( strlen( name ) >= MAX_QPATH )
	{
		ri.Error( ERR_DROP, "R_CreateVBO: \"%s\" is too long\n", name );
	}

	// make sure the render thread is stopped
	R_SyncRenderThread();

	vbo = ri.Hunk_Alloc( sizeof( *vbo ), h_low );
	Com_AddToGrowList( &tr.vbos, vbo );

	Q_strncpyz( vbo->name, name, sizeof( vbo->name ) );

	vbo->ofsXYZ = 0;
	vbo->ofsTexCoords = 0;
	vbo->ofsLightCoords = 0;
	vbo->ofsBinormals = 0;
	vbo->ofsTangents = 0;
	vbo->ofsNormals = 0;
	vbo->ofsColors = 0;
	vbo->ofsPaintColors = 0;
	vbo->ofsLightDirections = 0;
	vbo->ofsBoneIndexes = 0;
	vbo->ofsBoneWeights = 0;

	vbo->sizeXYZ = 0;
	vbo->sizeTangents = 0;
	vbo->sizeBinormals = 0;
	vbo->sizeNormals = 0;

	vbo->vertexesSize = vertexesSize;

	glGenBuffersARB( 1, &vbo->vertexesVBO );

	glBindBufferARB( GL_ARRAY_BUFFER_ARB, vbo->vertexesVBO );
	glBufferDataARB( GL_ARRAY_BUFFER_ARB, vertexesSize, vertexes, glUsage );

	glBindBufferARB( GL_ARRAY_BUFFER_ARB, 0 );

	GL_CheckErrors();

	return vbo;
}

/*
============
R_CreateVBO2

RB: OPTIMIZE rewrite to not use memcpy
============
*/
VBO_t          *R_CreateVBO2( const char *name, int numVertexes, srfVert_t *verts, unsigned int stateBits, vboUsage_t usage )
{
	VBO_t  *vbo;
	int    i, j;

	byte   *data;
	int    dataSize;
	int    dataOfs;

	vec4_t tmp;
	int    glUsage;

	switch ( usage )
	{
		case VBO_USAGE_STATIC:
			glUsage = GL_STATIC_DRAW_ARB;
			break;

		case VBO_USAGE_DYNAMIC:
			glUsage = GL_DYNAMIC_DRAW_ARB;
			break;

		default:
			glUsage = 0;
			Com_Error( ERR_FATAL, "bad vboUsage_t given: %i", usage );
	}

	if ( !numVertexes )
	{
		return NULL;
	}

	if ( strlen( name ) >= MAX_QPATH )
	{
		ri.Error( ERR_DROP, "R_CreateVBO2: \"%s\" is too long\n", name );
	}

	// make sure the render thread is stopped
	R_SyncRenderThread();

	vbo = ri.Hunk_Alloc( sizeof( *vbo ), h_low );
	Com_AddToGrowList( &tr.vbos, vbo );

	Q_strncpyz( vbo->name, name, sizeof( vbo->name ) );

	vbo->ofsXYZ = 0;
	vbo->ofsTexCoords = 0;
	vbo->ofsLightCoords = 0;
	vbo->ofsBinormals = 0;
	vbo->ofsTangents = 0;
	vbo->ofsNormals = 0;
	vbo->ofsColors = 0;
	vbo->ofsPaintColors = 0;
	vbo->ofsLightDirections = 0;
	vbo->ofsBoneIndexes = 0;
	vbo->ofsBoneWeights = 0;

	vbo->sizeXYZ = 0;
	vbo->sizeTangents = 0;
	vbo->sizeBinormals = 0;
	vbo->sizeNormals = 0;

	// create VBO
	dataSize = numVertexes * ( sizeof( vec4_t ) * 9 );
	data = ri.Hunk_AllocateTempMemory( dataSize );
	dataOfs = 0;

	// set up xyz array
	for ( i = 0; i < numVertexes; i++ )
	{
		for ( j = 0; j < 3; j++ )
		{
			tmp[ j ] = verts[ i ].xyz[ j ];
		}

		tmp[ 3 ] = 1;

		memcpy( data + dataOfs, ( vec_t * ) tmp, sizeof( vec4_t ) );
		dataOfs += sizeof( vec4_t );
	}

	// feed vertex texcoords
	if ( stateBits & ATTR_TEXCOORD )
	{
		vbo->ofsTexCoords = dataOfs;

		for ( i = 0; i < numVertexes; i++ )
		{
			for ( j = 0; j < 2; j++ )
			{
				tmp[ j ] = verts[ i ].st[ j ];
			}

			tmp[ 2 ] = 0;
			tmp[ 3 ] = 1;

			memcpy( data + dataOfs, ( vec_t * ) tmp, sizeof( vec4_t ) );
			dataOfs += sizeof( vec4_t );
		}
	}

	// feed vertex lightmap texcoords
	if ( stateBits & ATTR_LIGHTCOORD )
	{
		vbo->ofsLightCoords = dataOfs;

		for ( i = 0; i < numVertexes; i++ )
		{
			for ( j = 0; j < 2; j++ )
			{
				tmp[ j ] = verts[ i ].lightmap[ j ];
			}

			tmp[ 2 ] = 0;
			tmp[ 3 ] = 1;

			memcpy( data + dataOfs, ( vec_t * ) tmp, sizeof( vec4_t ) );
			dataOfs += sizeof( vec4_t );
		}
	}

	// feed vertex tangents
	if ( stateBits & ATTR_TANGENT )
	{
		vbo->ofsTangents = dataOfs;

		for ( i = 0; i < numVertexes; i++ )
		{
			for ( j = 0; j < 3; j++ )
			{
				tmp[ j ] = verts[ i ].tangent[ j ];
			}

			tmp[ 3 ] = 1;

			memcpy( data + dataOfs, ( vec_t * ) tmp, sizeof( vec4_t ) );
			dataOfs += sizeof( vec4_t );
		}
	}

	// feed vertex binormals
	if ( stateBits & ATTR_BINORMAL )
	{
		vbo->ofsBinormals = dataOfs;

		for ( i = 0; i < numVertexes; i++ )
		{
			for ( j = 0; j < 3; j++ )
			{
				tmp[ j ] = verts[ i ].binormal[ j ];
			}

			tmp[ 3 ] = 1;

			memcpy( data + dataOfs, ( vec_t * ) tmp, sizeof( vec4_t ) );
			dataOfs += sizeof( vec4_t );
		}
	}

	// feed vertex normals
	if ( stateBits & ATTR_NORMAL )
	{
		vbo->ofsNormals = dataOfs;

		for ( i = 0; i < numVertexes; i++ )
		{
			for ( j = 0; j < 3; j++ )
			{
				tmp[ j ] = verts[ i ].normal[ j ];
			}

			tmp[ 3 ] = 1;

			memcpy( data + dataOfs, ( vec_t * ) tmp, sizeof( vec4_t ) );
			dataOfs += sizeof( vec4_t );
		}
	}

	// feed vertex colors
	if ( stateBits & ATTR_COLOR )
	{
		vbo->ofsColors = dataOfs;

		for ( i = 0; i < numVertexes; i++ )
		{
			for ( j = 0; j < 4; j++ )
			{
				tmp[ j ] = verts[ i ].lightColor[ j ];
			}

			memcpy( data + dataOfs, ( vec_t * ) tmp, sizeof( vec4_t ) );
			dataOfs += sizeof( vec4_t );
		}
	}

#if !defined( COMPAT_Q3A ) && !defined( COMPAT_ET )

	// feed vertex paint colors
	if ( stateBits & ATTR_PAINTCOLOR )
	{
		vbo->ofsPaintColors = dataOfs;

		for ( i = 0; i < numVertexes; i++ )
		{
			for ( j = 0; j < 4; j++ )
			{
				tmp[ j ] = verts[ i ].paintColor[ j ];
			}

			memcpy( data + dataOfs, ( vec_t * ) tmp, sizeof( vec4_t ) );
			dataOfs += sizeof( vec4_t );
		}
	}

	// feed vertex light directions
	if ( stateBits & ATTR_LIGHTDIRECTION )
	{
		vbo->ofsLightDirections = dataOfs;

		for ( i = 0; i < numVertexes; i++ )
		{
			for ( j = 0; j < 3; j++ )
			{
				tmp[ j ] = verts[ i ].lightDirection[ j ];
			}

			tmp[ 3 ] = 1;

			memcpy( data + dataOfs, ( vec_t * ) tmp, sizeof( vec4_t ) );
			dataOfs += sizeof( vec4_t );
		}
	}

#endif

	vbo->vertexesSize = dataSize;
	vbo->vertexesNum = numVertexes;

	glGenBuffersARB( 1, &vbo->vertexesVBO );

	glBindBufferARB( GL_ARRAY_BUFFER_ARB, vbo->vertexesVBO );
	glBufferDataARB( GL_ARRAY_BUFFER_ARB, dataSize, data, glUsage );

	glBindBufferARB( GL_ARRAY_BUFFER_ARB, 0 );

	GL_CheckErrors();

	ri.Hunk_FreeTempMemory( data );

	return vbo;
}

/*
============
R_CreateIBO
============
*/
IBO_t          *R_CreateIBO( const char *name, byte *indexes, int indexesSize, vboUsage_t usage )
{
	IBO_t *ibo;
	int   glUsage;

	switch ( usage )
	{
		case VBO_USAGE_STATIC:
			glUsage = GL_STATIC_DRAW_ARB;
			break;

		case VBO_USAGE_DYNAMIC:
			glUsage = GL_DYNAMIC_DRAW_ARB;
			break;

		default:
			glUsage = 0;
			Com_Error( ERR_FATAL, "bad vboUsage_t given: %i", usage );
	}

	if ( strlen( name ) >= MAX_QPATH )
	{
		ri.Error( ERR_DROP, "R_CreateIBO: \"%s\" is too long\n", name );
	}

	// make sure the render thread is stopped
	R_SyncRenderThread();

	ibo = ri.Hunk_Alloc( sizeof( *ibo ), h_low );
	Com_AddToGrowList( &tr.ibos, ibo );

	Q_strncpyz( ibo->name, name, sizeof( ibo->name ) );

	ibo->indexesSize = indexesSize;

	glGenBuffersARB( 1, &ibo->indexesVBO );

	glBindBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB, ibo->indexesVBO );
	glBufferDataARB( GL_ELEMENT_ARRAY_BUFFER_ARB, indexesSize, indexes, glUsage );

	glBindBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB, 0 );

	GL_CheckErrors();

	return ibo;
}

/*
============
R_CreateIBO2
============
*/
IBO_t          *R_CreateIBO2( const char *name, int numTriangles, srfTriangle_t *triangles, vboUsage_t usage )
{
	IBO_t         *ibo;
	int           i, j;

	byte          *indexes;
	int           indexesSize;
	int           indexesOfs;

	srfTriangle_t *tri;
	glIndex_t     index;
	int           glUsage;

	switch ( usage )
	{
		case VBO_USAGE_STATIC:
			glUsage = GL_STATIC_DRAW_ARB;
			break;

		case VBO_USAGE_DYNAMIC:
			glUsage = GL_DYNAMIC_DRAW_ARB;
			break;

		default:
			glUsage = 0;
			Com_Error( ERR_FATAL, "bad vboUsage_t given: %i", usage );
	}

	if ( !numTriangles )
	{
		return NULL;
	}

	if ( strlen( name ) >= MAX_QPATH )
	{
		ri.Error( ERR_DROP, "R_CreateIBO2: \"%s\" is too long\n", name );
	}

	// make sure the render thread is stopped
	R_SyncRenderThread();

	ibo = ri.Hunk_Alloc( sizeof( *ibo ), h_low );
	Com_AddToGrowList( &tr.ibos, ibo );

	Q_strncpyz( ibo->name, name, sizeof( ibo->name ) );

	indexesSize = numTriangles * 3 * sizeof( glIndex_t );
	indexes = ri.Hunk_AllocateTempMemory( indexesSize );
	indexesOfs = 0;

	//ri.Printf(PRINT_ALL, "sizeof(glIndex_t) = %i\n", sizeof(glIndex_t));

	for ( i = 0, tri = triangles; i < numTriangles; i++, tri++ )
	{
		for ( j = 0; j < 3; j++ )
		{
			index = tri->indexes[ j ];
			memcpy( indexes + indexesOfs, &index, sizeof( glIndex_t ) );
			indexesOfs += sizeof( glIndex_t );
		}
	}

	ibo->indexesSize = indexesSize;
	ibo->indexesNum = numTriangles * 3;

	glGenBuffersARB( 1, &ibo->indexesVBO );

	glBindBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB, ibo->indexesVBO );
	glBufferDataARB( GL_ELEMENT_ARRAY_BUFFER_ARB, indexesSize, indexes, glUsage );

	glBindBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB, 0 );

	GL_CheckErrors();

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
		return;
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

		glState.vertexAttribsInterpolation = 0;
		glState.vertexAttribsOldFrame = 0;
		glState.vertexAttribsNewFrame = 0;

		glBindBufferARB( GL_ARRAY_BUFFER_ARB, vbo->vertexesVBO );

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
		glBindBufferARB( GL_ARRAY_BUFFER_ARB, 0 );
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
		return;
	}

	if ( r_logFile->integer )
	{
		// don't just call LogComment, or we will get a call to va() every frame!
		GLimp_LogComment( va( "--- R_BindIBO( %s ) ---\n", ibo->name ) );
	}

	if ( glState.currentIBO != ibo )
	{
		glBindBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB, ibo->indexesVBO );

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
		glBindBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB, 0 );
		glState.currentIBO = NULL;
		glState.vertexAttribPointersSet = 0;
	}
}

static void R_InitUnitCubeVBO()
{
	vec3_t        mins = { -1, -1, -1 };
	vec3_t        maxs = { 1,  1,  1 };

	int           i;
//	vec4_t          quadVerts[4];
	srfVert_t     *verts;
	srfTriangle_t *triangles;

	if ( glConfig.smpActive )
	{
		ri.Error( ERR_FATAL, "R_InitUnitCubeVBO: FIXME SMP" );
	}

	tess.multiDrawPrimitives = 0;
	tess.numIndexes = 0;
	tess.numVertexes = 0;

	Tess_AddCube( vec3_origin, mins, maxs, colorWhite );

	verts = ri.Hunk_AllocateTempMemory( tess.numVertexes * sizeof( srfVert_t ) );
	triangles = ri.Hunk_AllocateTempMemory( ( tess.numIndexes / 3 ) * sizeof( srfTriangle_t ) );

	for ( i = 0; i < tess.numVertexes; i++ )
	{
		VectorCopy( tess.xyz[ i ], verts[ i ].xyz );
	}

	for ( i = 0; i < ( tess.numIndexes / 3 ); i++ )
	{
		triangles[ i ].indexes[ 0 ] = tess.indexes[ i * 3 + 0 ];
		triangles[ i ].indexes[ 1 ] = tess.indexes[ i * 3 + 1 ];
		triangles[ i ].indexes[ 2 ] = tess.indexes[ i * 3 + 2 ];
	}

	tr.unitCubeVBO = R_CreateVBO2( "unitCube_VBO", tess.numVertexes, verts, ATTR_POSITION, VBO_USAGE_STATIC );
	tr.unitCubeIBO = R_CreateIBO2( "unitCube_IBO", tess.numIndexes / 3, triangles, VBO_USAGE_STATIC );

	ri.Hunk_FreeTempMemory( triangles );
	ri.Hunk_FreeTempMemory( verts );

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
	int  dataSize;
	byte *data;

	ri.Printf( PRINT_ALL, "------- R_InitVBOs -------\n" );

	Com_InitGrowList( &tr.vbos, 100 );
	Com_InitGrowList( &tr.ibos, 100 );

	dataSize = sizeof( vec4_t ) * SHADER_MAX_VERTEXES * 11;
	data = Com_Allocate( dataSize );
	memset( data, 0, dataSize );

	tess.vbo = R_CreateVBO( "tessVertexArray_VBO", data, dataSize, VBO_USAGE_DYNAMIC );
	tess.vbo->ofsXYZ = 0;
	tess.vbo->ofsTexCoords = tess.vbo->ofsXYZ + sizeof( tess.xyz );
	tess.vbo->ofsLightCoords = tess.vbo->ofsTexCoords + sizeof( tess.texCoords );
	tess.vbo->ofsTangents = tess.vbo->ofsLightCoords + sizeof( tess.lightCoords );
	tess.vbo->ofsBinormals = tess.vbo->ofsTangents + sizeof( tess.tangents );
	tess.vbo->ofsNormals = tess.vbo->ofsBinormals + sizeof( tess.binormals );
	tess.vbo->ofsColors = tess.vbo->ofsNormals + sizeof( tess.normals );

#if !defined( COMPAT_Q3A ) && !defined( COMPAT_ET )
	tess.vbo->ofsPaintColors = tess.vbo->ofsColors + sizeof( tess.colors );
	tess.vbo->ofsLightDirections = tess.vbo->ofsPaintColors + sizeof( tess.paintColors );
#endif

	tess.vbo->sizeXYZ = sizeof( tess.xyz );
	tess.vbo->sizeTangents = sizeof( tess.tangents );
	tess.vbo->sizeBinormals = sizeof( tess.binormals );
	tess.vbo->sizeNormals = sizeof( tess.normals );

	Com_Dealloc( data );

	dataSize = sizeof( tess.indexes );
	data = Com_Allocate( dataSize );
	memset( data, 0, dataSize );

	tess.ibo = R_CreateIBO( "tessVertexArray_IBO", data, dataSize, VBO_USAGE_DYNAMIC );

	Com_Dealloc( data );

	R_InitUnitCubeVBO();

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

	ri.Printf( PRINT_ALL, "------- R_ShutdownVBOs -------\n" );

	R_BindNullVBO();
	R_BindNullIBO();

	for ( i = 0; i < tr.vbos.currentElements; i++ )
	{
		vbo = ( VBO_t * ) Com_GrowListElement( &tr.vbos, i );

		if ( vbo->vertexesVBO )
		{
			glDeleteBuffersARB( 1, &vbo->vertexesVBO );
		}
	}

	for ( i = 0; i < tr.ibos.currentElements; i++ )
	{
		ibo = ( IBO_t * ) Com_GrowListElement( &tr.ibos, i );

		if ( ibo->indexesVBO )
		{
			glDeleteBuffersARB( 1, &ibo->indexesVBO );
		}
	}

#if defined( USE_BSP_CLUSTERSURFACE_MERGING )

	if ( tr.world )
	{
		int j;

		for ( j = 0; j < MAX_VISCOUNTS; j++ )
		{
			// FIXME: clean up this code
			for ( i = 0; i < tr.world->clusterVBOSurfaces[ j ].currentElements; i++ )
			{
				srfVBOMesh_t *vboSurf;

				vboSurf = ( srfVBOMesh_t * ) Com_GrowListElement( &tr.world->clusterVBOSurfaces[ j ], i );
				ibo = vboSurf->ibo;

				if ( ibo->indexesVBO )
				{
					glDeleteBuffersARB( 1, &ibo->indexesVBO );
				}
			}

			Com_DestroyGrowList( &tr.world->clusterVBOSurfaces[ j ] );
		}
	}

#endif // #if defined(USE_BSP_CLUSTERSURFACE_MERGING)

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

#if defined( USE_BSP_CLUSTERSURFACE_MERGING )

	if ( tr.world )
	{
		int j;

		for ( j = 0; j < MAX_VISCOUNTS; j++ )
		{
			// FIXME: clean up this code
			for ( i = 0; i < tr.world->clusterVBOSurfaces[ j ].currentElements; i++ )
			{
				srfVBOMesh_t *vboSurf;

				vboSurf = ( srfVBOMesh_t * ) Com_GrowListElement( &tr.world->clusterVBOSurfaces[ j ], i );
				ibo = vboSurf->ibo;

				ri.Printf( PRINT_ALL, "%d.%02d MB %s\n", ibo->indexesSize / ( 1024 * 1024 ),
				           ( ibo->indexesSize % ( 1024 * 1024 ) ) * 100 / ( 1024 * 1024 ), ibo->name );

				indexesSize += ibo->indexesSize;
			}
		}
	}

#endif // #if defined(USE_BSP_CLUSTERSURFACE_MERGING)

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
