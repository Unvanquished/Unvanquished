/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
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
// tr_shade_calc.c
#include "tr_local.h"

#define WAVEVALUE( table, base, amplitude, phase, freq ) (( base ) + table[ Q_ftol( ( ( ( phase ) + backEnd.refdef.floatTime * ( freq ) ) * FUNCTABLE_SIZE ) ) & FUNCTABLE_MASK ] * ( amplitude ))

static float   *TableForFunc( genFunc_t func )
{
	switch ( func )
	{
		case GF_SIN:
			return tr.sinTable;

		case GF_TRIANGLE:
			return tr.triangleTable;

		case GF_SQUARE:
			return tr.squareTable;

		case GF_SAWTOOTH:
			return tr.sawToothTable;

		case GF_INVERSE_SAWTOOTH:
			return tr.inverseSawToothTable;

		case GF_NONE:
		default:
			break;
	}

	return tr.sinTable;
}

/*
** EvalWaveForm
**
** Evaluates a given waveForm_t, referencing backEnd.refdef.time directly
*/
float RB_EvalWaveForm( const waveForm_t *wf )
{
	float *table;

	table = TableForFunc( wf->func );

	return WAVEVALUE( table, wf->base, wf->amplitude, wf->phase, wf->frequency );
}

float RB_EvalWaveFormClamped( const waveForm_t *wf )
{
	float glow = RB_EvalWaveForm( wf );

	if ( glow < 0 )
	{
		return 0;
	}

	if ( glow > 1 )
	{
		return 1;
	}

	return glow;
}

static float GetOpValue( const expOperation_t *op )
{
	float value;
	float inv255 = 1.0f / 255.0f;

	switch ( op->type )
	{
		case OP_NUM:
			value = op->value;
			break;

		case OP_TIME:
			value = backEnd.refdef.floatTime;
			break;

		case OP_PARM0:
			if ( backEnd.currentLight )
			{
				value = backEnd.currentLight->l.color[ 0 ];
				break;
			}
			else if ( backEnd.currentEntity )
			{
				value = backEnd.currentEntity->e.shaderRGBA.Red() * inv255;
			}
			else
			{
				value = 1.0;
			}

			break;

		case OP_PARM1:
			if ( backEnd.currentLight )
			{
				value = backEnd.currentLight->l.color[ 1 ];
				break;
			}
			else if ( backEnd.currentEntity )
			{
				value = backEnd.currentEntity->e.shaderRGBA.Green() * inv255;
			}
			else
			{
				value = 1.0;
			}

			break;

		case OP_PARM2:
			if ( backEnd.currentLight )
			{
				value = backEnd.currentLight->l.color[ 2 ];
				break;
			}
			else if ( backEnd.currentEntity )
			{
				value = backEnd.currentEntity->e.shaderRGBA.Blue() * inv255;
			}
			else
			{
				value = 1.0;
			}

			break;

		case OP_PARM3:
			if ( backEnd.currentLight )
			{
				value = 1.0;
				break;
			}
			else if ( backEnd.currentEntity )
			{
				value = backEnd.currentEntity->e.shaderRGBA.Alpha() * inv255;
			}
			else
			{
				value = 1.0;
			}

			break;

		case OP_PARM4:
			if ( backEnd.currentEntity )
			{
				value = -backEnd.currentEntity->e.shaderTime;
			}
			else
			{
				value = 0.0;
			}

			break;

		case OP_PARM5:
		case OP_PARM6:
		case OP_PARM7:
		case OP_PARM8:
		case OP_PARM9:
		case OP_PARM10:
		case OP_PARM11:
		case OP_GLOBAL0:
		case OP_GLOBAL1:
		case OP_GLOBAL2:
		case OP_GLOBAL3:
		case OP_GLOBAL4:
		case OP_GLOBAL5:
		case OP_GLOBAL6:
		case OP_GLOBAL7:
			value = 1.0;
			break;

		case OP_FRAGMENTSHADERS:
			value = 1.0;
			break;

		case OP_FRAMEBUFFEROBJECTS:
			value = glConfig2.framebufferObjectAvailable;
			break;

		case OP_SOUND:
			value = 0.5;
			break;

		case OP_DISTANCE:
			value = 0.0; // FIXME ?
			break;

		default:
			value = 0.0;
			break;
	}

	return value;
}

const char* GetOpName(opcode_t type);

float RB_EvalExpression( const expression_t *exp, float defaultValue )
{
	int                     i;
	expOperation_t          op;
	expOperation_t          ops[ MAX_EXPRESSION_OPS ];
	int                     numOps;
	float                   value;
	float                   value1;
	float                   value2;

	numOps = 0;
	value = 0;
	value1 = 0;
	value2 = 0;

	if ( !exp || !exp->active )
	{
		return defaultValue;
	}

	// http://www.qiksearch.com/articles/cs/postfix-evaluation/
	// http://www.kyz.uklinux.net/evaluate/

	for ( i = 0; i < exp->numOps; i++ )
	{
		op = exp->ops[ i ];

		switch ( op.type )
		{
			case OP_BAD:
				return defaultValue;

			case OP_NEG:
				{
					if ( numOps < 1 )
					{
						ri.Printf( PRINT_ALL, "WARNING: shader %s has numOps < 1 for unary - operator\n", tess.surfaceShader->name );
						return defaultValue;
					}

					value1 = GetOpValue( &ops[ numOps - 1 ] );
					numOps--;

					value = -value1;

					// push result
					op.type = OP_NUM;
					op.value = value;
					ops[ numOps++ ] = op;
					break;
				}

			case OP_NUM:
			case OP_TIME:
			case OP_PARM0:
			case OP_PARM1:
			case OP_PARM2:
			case OP_PARM3:
			case OP_PARM4:
			case OP_PARM5:
			case OP_PARM6:
			case OP_PARM7:
			case OP_PARM8:
			case OP_PARM9:
			case OP_PARM10:
			case OP_PARM11:
			case OP_GLOBAL0:
			case OP_GLOBAL1:
			case OP_GLOBAL2:
			case OP_GLOBAL3:
			case OP_GLOBAL4:
			case OP_GLOBAL5:
			case OP_GLOBAL6:
			case OP_GLOBAL7:
			case OP_FRAGMENTSHADERS:
			case OP_FRAMEBUFFEROBJECTS:
			case OP_SOUND:
			case OP_DISTANCE:
				ops[ numOps++ ] = op;
				break;

			case OP_TABLE:
				{
					shaderTable_t *table;
					int           numValues;
					float         index;
					float         lerp;
					int           oldIndex;
					int           newIndex;

					if ( numOps < 1 )
					{
						ri.Printf( PRINT_ALL, "WARNING: shader %s has numOps < 1 for table operator\n", tess.surfaceShader->name );
						return defaultValue;
					}

					value1 = GetOpValue( &ops[ numOps - 1 ] );
					numOps--;

					table = tr.shaderTables[( int ) op.value ];

					numValues = table->numValues;

					index = value1 * numValues; // float index into the table?s elements
					lerp = index - floor( index );  // being inbetween two elements of the table

					oldIndex = ( int ) index;
					newIndex = ( int ) index + 1;

					if ( table->clamp )
					{
						// clamp indices to table-range
						oldIndex = Math::Clamp( oldIndex, 0, numValues - 1 );
						newIndex = Math::Clamp( newIndex, 0, numValues - 1 );
					}
					else
					{
						// wrap around indices
						oldIndex %= numValues;
						newIndex %= numValues;
					}

					if ( table->snap )
					{
						// use fixed value
						value = table->values[ oldIndex ];
					}
					else
					{
						// lerp value
						value = table->values[ oldIndex ] + ( ( table->values[ newIndex ] - table->values[ oldIndex ] ) * lerp );
					}

					//ri.Printf(PRINT_ALL, "%s: %i %i %f\n", table->name, oldIndex, newIndex, value);

					// push result
					op.type = OP_NUM;
					op.value = value;
					ops[ numOps++ ] = op;
					break;
				}

			default:
				{
					if ( numOps < 2 )
					{
						ri.Printf( PRINT_ALL, "WARNING: shader %s has numOps < 2 for binary operator %s\n", tess.surfaceShader->name,
						           GetOpName( op.type ) );
						return defaultValue;
					}

					value2 = GetOpValue( &ops[ numOps - 1 ] );
					numOps--;

					value1 = GetOpValue( &ops[ numOps - 1 ] );
					numOps--;

					switch ( op.type )
					{
						case OP_LAND:
							value = value1 && value2;
							break;

						case OP_LOR:
							value = value1 || value2;
							break;

						case OP_GE:
							value = value1 >= value2;
							break;

						case OP_LE:
							value = value1 <= value2;
							break;

						case OP_LEQ:
							value = value1 == value2;
							break;

						case OP_LNE:
							value = value1 != value2;
							break;

						case OP_ADD:
							value = value1 + value2;
							break;

						case OP_SUB:
							value = value1 - value2;
							break;

						case OP_DIV:
							if ( value2 == 0 )
							{
								// don't divide by zero
								value = value1;
							}
							else
							{
								value = value1 / value2;
							}

							break;

						case OP_MOD:
							value = ( float )( ( int ) value1 % ( int ) value2 );
							break;

						case OP_MUL:
							value = value1 * value2;
							break;

						case OP_LT:
							value = value1 < value2;
							break;

						case OP_GT:
							value = value1 > value2;
							break;

						default:
							value = value1 = value2 = 0;
							break;
					}

					// push result
					op.type = OP_NUM;
					op.value = value;
					ops[ numOps++ ] = op;
					break;
				}
		}
	}

	return GetOpValue( &ops[ 0 ] );
}

/*
====================================================================

DEFORMATIONS

====================================================================
*/

/*
=====================
ReorderQuadVerts

Reorder the quad vertices so that they are sorted by their texcoords
=====================
*/
static void ReorderQuadVerts( shaderVertex_t *vert, glIndex_t *index,
			      glIndex_t minIndex )
{
	int newIndex[4];
	shaderVertex_t vertexCopy[4];
	vec2_t midTexCoord;
	int i;

	Com_Memcpy( vertexCopy, vert, 4 * sizeof(shaderVertex_t) );

	midTexCoord[ 0 ] = midTexCoord[ 1 ] = 0.0f;
	for( i = 0; i < 4; i++ ) {
		midTexCoord[ 0 ] += halfToFloat( vert[ i ].texCoords[ 0 ] );
		midTexCoord[ 1 ] += halfToFloat( vert[ i ].texCoords[ 1 ] );
	}
	midTexCoord[ 0 ] *= 0.25f;
	midTexCoord[ 1 ] *= 0.25f;

	for( i = 0; i < 4; i++ ) {
		newIndex[ i ] = 0;
		if( halfToFloat( vert[ i ].texCoords[ 0 ] ) > midTexCoord[ 0 ] )
			newIndex[ i ] |= 3;
		if( halfToFloat( vert[ i ].texCoords[ 1 ] ) > midTexCoord[ 1 ] )
			newIndex[ i ] ^= 1;

		newIndex[ i ] = (newIndex[ i ] - minIndex) & 3;
	}

	for( i = 0; i < 4; i++ ) {
		Com_Memcpy( &vert[ newIndex[ i ] ], &vertexCopy[ i ],
			    sizeof(shaderVertex_t) );
	}
	for( i = 0; i < 6; i++ ) {
		index[ i ] = minIndex + newIndex[ index[ i ] - minIndex ];
	}
}

/*
=====================
AutospriteDeform

Assuming all the triangles for this shader are independent
quads, rebuild them as forward facing sprites
=====================
*/
static void AutospriteDeform( int firstVertex, int numVertexes, int numIndexes )
{
	int    i, j;
	shaderVertex_t *v;
	vec3_t mid, delta;
	float  radius;

	if ( numVertexes & 3 )
	{
		ri.Printf( PRINT_WARNING, "Autosprite shader %s had odd vertex count\n", tess.surfaceShader->name );
	}

	if ( numIndexes != ( numVertexes >> 2 ) * 6 )
	{
		ri.Printf( PRINT_WARNING, "Autosprite shader %s had odd index count\n", tess.surfaceShader->name );
	}

	for ( i = 0; i < numVertexes; i += 4 )
	{
		// find the midpoint
		v = &tess.verts[ firstVertex + i ];

		mid[ 0 ] = 0.25f * ( v[ 0 ].xyz[ 0 ] + v[ 1 ].xyz[ 0 ] + v[ 2 ].xyz[ 0 ] + v[ 3 ].xyz[ 0 ] );
		mid[ 1 ] = 0.25f * ( v[ 0 ].xyz[ 1 ] + v[ 1 ].xyz[ 1 ] + v[ 2 ].xyz[ 1 ] + v[ 3 ].xyz[ 1 ] );
		mid[ 2 ] = 0.25f * ( v[ 0 ].xyz[ 2 ] + v[ 1 ].xyz[ 2 ] + v[ 2 ].xyz[ 2 ] + v[ 3 ].xyz[ 2 ] );

		VectorSubtract( v[ 0 ].xyz, mid, delta );
		radius = VectorLength( delta ) * 0.5f * M_SQRT2;

		// add 4 identical vertices
		for ( j = 0; j < 4; j++ ) {
			VectorCopy( mid, v[ j ].xyz );
			Vector4Set( v[ j ].spriteOrientation,
				    0, 0, 0, floatToHalf( radius ) );
		}
	}
}

/*
=====================
Autosprite2Deform

Autosprite2 will pivot a rectangular quad along the center of its long axis
=====================
*/
static const int edgeVerts[ 6 ][ 2 ] =
{
	{ 0, 1 },
	{ 0, 2 },
	{ 0, 3 },
	{ 1, 2 },
	{ 1, 3 },
	{ 2, 3 }
};

static void Autosprite2Deform( int firstVertex, int numVertexes, int firstIndex, int numIndexes )
{
	int    i, j, k;
	int    indexes;

	if ( numVertexes & 3 )
	{
		ri.Printf( PRINT_WARNING, "Autosprite2 shader %s had odd vertex count\n", tess.surfaceShader->name );
	}

	if ( numIndexes != ( numVertexes >> 2 ) * 6 )
	{
		ri.Printf( PRINT_WARNING, "Autosprite2 shader %s had odd index count\n", tess.surfaceShader->name );
	}

	// this is a lot of work for two triangles...
	// we could precalculate a lot of it is an issue, but it would mess up
	// the shader abstraction
	for ( i = 0, indexes = 0; i < numVertexes; i += 4, indexes += 6 )
	{
		float  lengths[ 2 ];
		int    nums[ 2 ];
		vec3_t mid[ 2 ];
		vec3_t normal, cross;
		vec3_t major, minor;
		shaderVertex_t *v1, *v2;

		ReorderQuadVerts( tess.verts + firstVertex + i,
				  tess.indexes + firstIndex + indexes,
				  firstVertex );
		R_QtangentsToNormal( tess.verts[ firstVertex + i ].qtangents, normal );

		// find the midpoint

		// identify the two shortest edges
		nums[ 0 ] = nums[ 1 ] = 0;
		lengths[ 0 ] = lengths[ 1 ] = 999999;

		for ( j = 0; j < 6; j++ )
		{
			float  l;
			vec3_t temp;

			v1 = tess.verts + firstVertex + i + edgeVerts[ j ][ 0 ];
			v2 = tess.verts + firstVertex + i + edgeVerts[ j ][ 1 ];

			VectorSubtract( v1->xyz, v2->xyz, temp );

			l = DotProduct( temp, temp );

			if ( l < lengths[ 0 ] )
			{
				nums[ 1 ] = nums[ 0 ];
				lengths[ 1 ] = lengths[ 0 ];
				nums[ 0 ] = j;
				lengths[ 0 ] = l;
			}
			else if ( l < lengths[ 1 ] )
			{
				nums[ 1 ] = j;
				lengths[ 1 ] = l;
			}
		}

		for ( j = 0; j < 2; j++ )
		{
			v1 = tess.verts + firstVertex + i + edgeVerts[ nums[ j ] ][ 0 ];
			v2 = tess.verts + firstVertex + i + edgeVerts[ nums[ j ] ][ 1 ];

			mid[ j ][ 0 ] = 0.5f * ( v1->xyz[ 0 ] + v2->xyz[ 0 ] );
			mid[ j ][ 1 ] = 0.5f * ( v1->xyz[ 1 ] + v2->xyz[ 1 ] );
			mid[ j ][ 2 ] = 0.5f * ( v1->xyz[ 2 ] + v2->xyz[ 2 ] );
		}

		// find the vector of the major axis
		VectorSubtract( mid[ 1 ], mid[ 0 ], major );
		CrossProduct( major, normal, cross );

		// update the vertices
		for ( j = 0; j < 4; j++ )
		{
			vec4_t orientation;

			v1 = &tess.verts[ firstVertex + i + j ];
			lengths[ 0 ] = Distance( mid[ 0 ], v1->xyz );
			lengths[ 1 ] = Distance( mid[ 1 ], v1->xyz );

			// pick the closer midpoint
			if ( lengths[ 0 ] <= lengths[ 1 ] )
				k = 0;
			else
				k = 1;

			VectorSubtract( v1->xyz, mid[ k ], minor );
			if ( DotProduct( cross, minor ) * (k ? -1.0f : 1.0f) < 0.0f ) {
				VectorNegate( major, orientation );
			} else {
				VectorCopy( major, orientation );
			}
			orientation[ 3 ] = -lengths[ k ];

			floatToHalf( orientation, v1->spriteOrientation );
			VectorCopy( mid[ k ], v1->xyz );
		}
	}
}

/*
=====================
Tess_AutospriteDeform
=====================
*/
void Tess_AutospriteDeform( int mode, int firstVertex, int numVertexes,
			    int firstIndex, int numIndexes )
{
	switch( mode ) {
	case 1:
		AutospriteDeform( firstVertex, numVertexes, numIndexes );
		break;
	case 2:
		Autosprite2Deform( firstVertex, numVertexes, firstIndex, numIndexes );
		break;
	}
	GL_CheckErrors();
}

/*
====================================================================

TEX COORDS

====================================================================
*/

/*
===============
RB_CalcTexMatrix
===============
*/
void RB_CalcTexMatrix( const textureBundle_t *bundle, matrix_t matrix )
{
	int   j;
	float x, y;

	MatrixIdentity( matrix );

	for ( j = 0; j < bundle->numTexMods; j++ )
	{
		switch ( bundle->texMods[ j ].type )
		{
			case TMOD_NONE:
				j = TR_MAX_TEXMODS; // break out of for loop
				break;

			case TMOD_TURBULENT:
				{
					waveForm_t *wf;

					wf = &bundle->texMods[ j ].wave;

					x = ( 1.0 / 4.0 );
					y = ( wf->phase + backEnd.refdef.floatTime * wf->frequency );

					MatrixMultiplyScale( matrix, 1 + ( wf->amplitude * sin( y ) + wf->base ) * x,
					                     1 + ( wf->amplitude * sin( y + 0.25 ) + wf->base ) * x, 0.0 );
					break;
				}

			case TMOD_ENTITY_TRANSLATE:
				{
					x = backEnd.currentEntity->e.shaderTexCoord[ 0 ] * backEnd.refdef.floatTime;
					y = backEnd.currentEntity->e.shaderTexCoord[ 1 ] * backEnd.refdef.floatTime;

					// clamp so coordinates don't continuously get larger, causing problems
					// with hardware limits
					x = x - floor( x );
					y = y - floor( y );

					MatrixMultiplyTranslation( matrix, x, y, 0.0 );
					break;
				}

			case TMOD_SCROLL:
				{
					x = bundle->texMods[ j ].scroll[ 0 ] * backEnd.refdef.floatTime;
					y = bundle->texMods[ j ].scroll[ 1 ] * backEnd.refdef.floatTime;

					// clamp so coordinates don't continuously get larger, causing problems
					// with hardware limits
					x = x - floor( x );
					y = y - floor( y );

					MatrixMultiplyTranslation( matrix, x, y, 0.0 );
					break;
				}

			case TMOD_SCALE:
				{
					x = bundle->texMods[ j ].scale[ 0 ];
					y = bundle->texMods[ j ].scale[ 1 ];

					MatrixMultiplyScale( matrix, x, y, 0.0 );
					break;
				}

			case TMOD_STRETCH:
				{
					float p;

					p = 1.0f / RB_EvalWaveForm( &bundle->texMods[ j ].wave );

					MatrixMultiplyTranslation( matrix, 0.5, 0.5, 0.0 );
					MatrixMultiplyScale( matrix, p, p, 0.0 );
					MatrixMultiplyTranslation( matrix, -0.5, -0.5, 0.0 );
					break;
				}

			case TMOD_TRANSFORM:
				{
					const texModInfo_t *tmi = &bundle->texMods[ j ];

					MatrixMultiply2( matrix, tmi->matrix );
					break;
				}

			case TMOD_ROTATE:
				{
					x = -bundle->texMods[ j ].rotateSpeed * backEnd.refdef.floatTime;

					MatrixMultiplyTranslation( matrix, 0.5, 0.5, 0.0 );
					MatrixMultiplyZRotation( matrix, x );
					MatrixMultiplyTranslation( matrix, -0.5, -0.5, 0.0 );
					break;
				}

			case TMOD_SCROLL2:
				{
					x = RB_EvalExpression( &bundle->texMods[ j ].sExp, 0 );
					y = RB_EvalExpression( &bundle->texMods[ j ].tExp, 0 );

					// clamp so coordinates don't continuously get larger, causing problems
					// with hardware limits
					x = x - floor( x );
					y = y - floor( y );

					MatrixMultiplyTranslation( matrix, x, y, 0.0 );
					break;
				}

			case TMOD_SCALE2:
				{
					x = RB_EvalExpression( &bundle->texMods[ j ].sExp, 0 );
					y = RB_EvalExpression( &bundle->texMods[ j ].tExp, 0 );

					MatrixMultiplyScale( matrix, x, y, 0.0 );
					break;
				}

			case TMOD_CENTERSCALE:
				{
					x = RB_EvalExpression( &bundle->texMods[ j ].sExp, 0 );
					y = RB_EvalExpression( &bundle->texMods[ j ].tExp, 0 );

					MatrixMultiplyTranslation( matrix, 0.5, 0.5, 0.0 );
					MatrixMultiplyScale( matrix, x, y, 0.0 );
					MatrixMultiplyTranslation( matrix, -0.5, -0.5, 0.0 );
					break;
				}

			case TMOD_SHEAR:
				{
					x = RB_EvalExpression( &bundle->texMods[ j ].sExp, 0 );
					y = RB_EvalExpression( &bundle->texMods[ j ].tExp, 0 );

					MatrixMultiplyTranslation( matrix, 0.5, 0.5, 0.0 );
					MatrixMultiplyShear( matrix, x, y );
					MatrixMultiplyTranslation( matrix, -0.5, -0.5, 0.0 );
					break;
				}

			case TMOD_ROTATE2:
				{
					x = RB_EvalExpression( &bundle->texMods[ j ].rExp, 0 );

					MatrixMultiplyTranslation( matrix, 0.5, 0.5, 0.0 );
					MatrixMultiplyZRotation( matrix, x );
					MatrixMultiplyTranslation( matrix, -0.5, -0.5, 0.0 );
					break;
				}

			default:
				break;
		}
	}
}
