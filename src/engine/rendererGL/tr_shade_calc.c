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

#define WAVEVALUE( table, base, amplitude, phase, freq ) (( base ) + table[ XreaL_Q_ftol( ( ( ( phase ) + backEnd.refdef.floatTime * ( freq ) ) * FUNCTABLE_SIZE ) ) & FUNCTABLE_MASK ] * ( amplitude ))

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

#if 0
	ri.Error( ERR_DROP, "TableForFunc called with invalid function '%d' in shader '%s'\n", func, tess.surfaceShader->name );
	return NULL;
#else
	// FIXME ri.Printf(PRINT_WARNING, "TableForFunc called with invalid function '%d' in shader '%s'\n", func, tess.surfaceShader->name);
	return tr.sinTable;
#endif
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
				value = backEnd.currentEntity->e.shaderRGBA[ 0 ] * inv255;
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
				value = backEnd.currentEntity->e.shaderRGBA[ 1 ] * inv255;
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
				value = backEnd.currentEntity->e.shaderRGBA[ 2 ] * inv255;
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
				value = backEnd.currentEntity->e.shaderRGBA[ 3 ] * inv255;
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

float RB_EvalExpression( const expression_t *exp, float defaultValue )
{
#if 1
	int                     i;
	expOperation_t          op;
	expOperation_t          ops[ MAX_EXPRESSION_OPS ];
	int                     numOps;
	float                   value;
	float                   value1;
	float                   value2;
	extern const opstring_t opStrings[];

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
						Q_clamp( oldIndex, 0, numValues - 1 );
						Q_clamp( newIndex, 0, numValues - 1 );
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
						           opStrings[ op.type ].s );
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

					//ri.Printf(PRINT_ALL, "%s: %f %f %f\n", opStrings[op.type].s, value, value1, value2);

					// push result
					op.type = OP_NUM;
					op.value = value;
					ops[ numOps++ ] = op;
					break;
				}
		}
	}

	return GetOpValue( &ops[ 0 ] );
#else
	return defaultValue;
#endif
}

/*
====================================================================

DEFORMATIONS

====================================================================
*/

/*
========================
RB_CalcDeformVertexes
========================
*/
void RB_CalcDeformVertexes( deformStage_t *ds )
{
	int    i;
	vec3_t offset;
	float  scale;
	float  *xyz = ( float * ) tess.xyz;
	float  *normal = ( float * ) tess.normals;
	float  *table;

#if defined( COMPAT_ET )

	if ( ds->deformationWave.frequency < 0 )
	{
		qboolean inverse = qfalse;
		vec3_t   worldUp;

		//static vec3_t up = {0,0,1};

		if ( VectorCompare( backEnd.currentEntity->e.fireRiseDir, vec3_origin ) )
		{
			VectorSet( backEnd.currentEntity->e.fireRiseDir, 0, 0, 1 );
		}

		// get the world up vector in local coordinates
		if ( backEnd.currentEntity->e.hModel )
		{
			// world surfaces dont have an axis
			VectorRotate( backEnd.currentEntity->e.fireRiseDir, backEnd.currentEntity->e.axis, worldUp );
		}
		else
		{
			VectorCopy( backEnd.currentEntity->e.fireRiseDir, worldUp );
		}

		// don't go so far if sideways, since they must be moving
		VectorScale( worldUp, 0.4 + 0.6 * Q_fabs( backEnd.currentEntity->e.fireRiseDir[ 2 ] ), worldUp );

		ds->deformationWave.frequency *= -1;

		if ( ds->deformationWave.frequency > 999 )
		{
			// hack for negative Z deformation (ack)
			inverse = qtrue;
			ds->deformationWave.frequency -= 999;
		}

		table = TableForFunc( ds->deformationWave.func );

		for ( i = 0; i < tess.numVertexes; i++, xyz += 4, normal += 4 )
		{
			float off = ( xyz[ 0 ] + xyz[ 1 ] + xyz[ 2 ] ) * ds->deformationSpread;
			float dot;

			scale = WAVEVALUE( table, ds->deformationWave.base,
			                   ds->deformationWave.amplitude, ds->deformationWave.phase + off, ds->deformationWave.frequency );

			dot = DotProduct( worldUp, normal );

			if ( dot * scale > 0 )
			{
				if ( inverse )
				{
					scale *= -1;
				}

				VectorMA( xyz, dot * scale, worldUp, xyz );
			}
		}

		if ( inverse )
		{
			ds->deformationWave.frequency += 999;
		}

		ds->deformationWave.frequency *= -1;
	}
	else
#endif // #if defined(COMPAT_ET)
		if ( ds->deformationWave.frequency == 0 )
		{
			scale = RB_EvalWaveForm( &ds->deformationWave );

			for ( i = 0; i < tess.numVertexes; i++, xyz += 4, normal += 4 )
			{
				VectorScale( normal, scale, offset );

				xyz[ 0 ] += offset[ 0 ];
				xyz[ 1 ] += offset[ 1 ];
				xyz[ 2 ] += offset[ 2 ];
			}
		}
		else
		{
			table = TableForFunc( ds->deformationWave.func );

			for ( i = 0; i < tess.numVertexes; i++, xyz += 4, normal += 4 )
			{
				float off = ( xyz[ 0 ] + xyz[ 1 ] + xyz[ 2 ] ) * ds->deformationSpread;

				scale = WAVEVALUE( table, ds->deformationWave.base,
				                   ds->deformationWave.amplitude, ds->deformationWave.phase + off, ds->deformationWave.frequency );

				VectorScale( normal, scale, offset );

				xyz[ 0 ] += offset[ 0 ];
				xyz[ 1 ] += offset[ 1 ];
				xyz[ 2 ] += offset[ 2 ];
			}
		}
}

/*
=========================
RB_CalcDeformNormals

Wiggle the normals for wavy environment mapping
=========================
*/
void RB_CalcDeformNormals( deformStage_t *ds )
{
	int   i;
	float scale;
	float *xyz = ( float * ) tess.xyz;
	float *normal = ( float * ) tess.normals;

	for ( i = 0; i < tess.numVertexes; i++, xyz += 4, normal += 4 )
	{
		scale = 0.98f;
		scale =
		  R_NoiseGet4f( xyz[ 0 ] * scale, xyz[ 1 ] * scale, xyz[ 2 ] * scale,
		                backEnd.refdef.floatTime * ds->deformationWave.frequency );
		normal[ 0 ] += ds->deformationWave.amplitude * scale;

		scale = 0.98f;
		scale = R_NoiseGet4f( 100 + xyz[ 0 ] * scale, xyz[ 1 ] * scale, xyz[ 2 ] * scale,
		                      backEnd.refdef.floatTime * ds->deformationWave.frequency );
		normal[ 1 ] += ds->deformationWave.amplitude * scale;

		scale = 0.98f;
		scale = R_NoiseGet4f( 200 + xyz[ 0 ] * scale, xyz[ 1 ] * scale, xyz[ 2 ] * scale,
		                      backEnd.refdef.floatTime * ds->deformationWave.frequency );
		normal[ 2 ] += ds->deformationWave.amplitude * scale;

		VectorNormalizeFast( normal );
	}
}

/*
========================
RB_CalcBulgeVertexes
========================
*/
void RB_CalcBulgeVertexes( deformStage_t *ds )
{
	int         i;
	const float *st = ( const float * ) tess.texCoords[ 0 ];
	float       *xyz = ( float * ) tess.xyz;
	float       *normal = ( float * ) tess.normals;
	float       now;

	now = backEnd.refdef.time * ds->bulgeSpeed * 0.001f;

	for ( i = 0; i < tess.numVertexes; i++, xyz += 4, st += 4, normal += 4 )
	{
		int   off;
		float scale;

		off = ( float )( FUNCTABLE_SIZE / ( M_PI * 2 ) ) * ( st[ 0 ] * ds->bulgeWidth + now );

		scale = tr.sinTable[ off & FUNCTABLE_MASK ] * ds->bulgeHeight;

		xyz[ 0 ] += normal[ 0 ] * scale;
		xyz[ 1 ] += normal[ 1 ] * scale;
		xyz[ 2 ] += normal[ 2 ] * scale;
	}
}

/*
======================
RB_CalcMoveVertexes

A deformation that can move an entire surface along a wave path
======================
*/
void RB_CalcMoveVertexes( deformStage_t *ds )
{
	int    i;
	float  *xyz;
	float  *table;
	float  scale;
	vec3_t offset;

	table = TableForFunc( ds->deformationWave.func );

	scale = WAVEVALUE( table, ds->deformationWave.base,
	                   ds->deformationWave.amplitude, ds->deformationWave.phase, ds->deformationWave.frequency );

	VectorScale( ds->moveVector, scale, offset );

	xyz = ( float * ) tess.xyz;

	for ( i = 0; i < tess.numVertexes; i++, xyz += 4 )
	{
		VectorAdd( xyz, offset, xyz );
	}
}

/*
=============
DeformText

Change a polygon into a bunch of text polygons
=============
*/
void DeformText( const char *text )
{
	int    i;
	vec3_t origin, width, height;
	int    len;
	int    ch;
	float  bottom, top;
	vec3_t mid;

	height[ 0 ] = 0;
	height[ 1 ] = 0;
	height[ 2 ] = -1;
	CrossProduct( tess.normals[ 0 ], height, width );

	// find the midpoint of the box
	VectorClear( mid );
	bottom = 999999;
	top = -999999;

	for ( i = 0; i < 4; i++ )
	{
		VectorAdd( tess.xyz[ i ], mid, mid );

		if ( tess.xyz[ i ][ 2 ] < bottom )
		{
			bottom = tess.xyz[ i ][ 2 ];
		}

		if ( tess.xyz[ i ][ 2 ] > top )
		{
			top = tess.xyz[ i ][ 2 ];
		}
	}

	VectorScale( mid, 0.25f, origin );

	// determine the individual character size
	height[ 0 ] = 0;
	height[ 1 ] = 0;
	height[ 2 ] = ( top - bottom ) * 0.5f;

	VectorScale( width, height[ 2 ] * -0.75f, width );

	// determine the starting position
	len = strlen( text );
	VectorMA( origin, ( len - 1 ), width, origin );

	// clear the shader indexes
	tess.multiDrawPrimitives = 0;
	tess.numIndexes = 0;
	tess.numVertexes = 0;

	// draw each character
	for ( i = 0; i < len; i++ )
	{
		ch = text[ i ];
		ch &= 255;

		if ( ch != ' ' )
		{
			int   row, col;
			float frow, fcol, size;

			row = ch >> 4;
			col = ch & 15;

			frow = row * 0.0625f;
			fcol = col * 0.0625f;
			size = 0.0625f;

			Tess_AddQuadStampExt( origin, width, height, colorWhite, fcol, frow, fcol + size, frow + size );
		}

		VectorMA( origin, -2, width, origin );
	}
}

/*
==================
GlobalVectorToLocal
==================
*/
static void GlobalVectorToLocal( const vec3_t in, vec3_t out )
{
	out[ 0 ] = DotProduct( in, backEnd.orientation.axis[ 0 ] );
	out[ 1 ] = DotProduct( in, backEnd.orientation.axis[ 1 ] );
	out[ 2 ] = DotProduct( in, backEnd.orientation.axis[ 2 ] );
}

/*
=====================
AutospriteDeform

Assuming all the triangles for this shader are independant
quads, rebuild them as forward facing sprites
=====================
*/
static void AutospriteDeform( void )
{
	int    i;
	int    oldVerts;
	float  *xyz;
	vec3_t mid, delta;
	float  radius;
	vec3_t left, up;
	vec3_t leftDir, upDir;

	if ( tess.numVertexes & 3 )
	{
		ri.Printf( PRINT_WARNING, "Autosprite shader %s had odd vertex count", tess.surfaceShader->name );
	}

	if ( tess.numIndexes != ( tess.numVertexes >> 2 ) * 6 )
	{
		ri.Printf( PRINT_WARNING, "Autosprite shader %s had odd index count", tess.surfaceShader->name );
	}

	oldVerts = tess.numVertexes;
	tess.numVertexes = 0;
	tess.numIndexes = 0;
	tess.multiDrawPrimitives = 0;

	if ( backEnd.currentEntity != &tr.worldEntity )
	{
		GlobalVectorToLocal( backEnd.viewParms.orientation.axis[ 1 ], leftDir );
		GlobalVectorToLocal( backEnd.viewParms.orientation.axis[ 2 ], upDir );
	}
	else
	{
		VectorCopy( backEnd.viewParms.orientation.axis[ 1 ], leftDir );
		VectorCopy( backEnd.viewParms.orientation.axis[ 2 ], upDir );
	}

	for ( i = 0; i < oldVerts; i += 4 )
	{
		// find the midpoint
		xyz = tess.xyz[ i ];

		mid[ 0 ] = 0.25f * ( xyz[ 0 ] + xyz[ 4 ] + xyz[ 8 ] + xyz[ 12 ] );
		mid[ 1 ] = 0.25f * ( xyz[ 1 ] + xyz[ 5 ] + xyz[ 9 ] + xyz[ 13 ] );
		mid[ 2 ] = 0.25f * ( xyz[ 2 ] + xyz[ 6 ] + xyz[ 10 ] + xyz[ 14 ] );

		VectorSubtract( xyz, mid, delta );
		radius = VectorLength( delta ) * 0.707f;  // / sqrt(2)

		VectorScale( leftDir, radius, left );
		VectorScale( upDir, radius, up );

		if ( backEnd.viewParms.isMirror )
		{
			VectorSubtract( vec3_origin, left, left );
		}

		// compensate for scale in the axes if necessary
		if ( backEnd.currentEntity->e.nonNormalizedAxes )
		{
			float axisLength;

			axisLength = VectorLength( backEnd.currentEntity->e.axis[ 0 ] );

			if ( !axisLength )
			{
				axisLength = 0;
			}
			else
			{
				axisLength = 1.0f / axisLength;
			}

			VectorScale( left, axisLength, left );
			VectorScale( up, axisLength, up );
		}

		Tess_AddQuadStamp( mid, left, up, tess.colors[ i ] );
	}
}

/*
=====================
Autosprite2Deform

Autosprite2 will pivot a rectangular quad along the center of its long axis
=====================
*/
int edgeVerts[ 6 ][ 2 ] =
{
	{ 0, 1 },
	{ 0, 2 },
	{ 0, 3 },
	{ 1, 2 },
	{ 1, 3 },
	{ 2, 3 }
};

static void Autosprite2Deform( void )
{
	int    i, j, k;
	int    indexes;
	float  *xyz;
	vec3_t forward;

	if ( tess.numVertexes & 3 )
	{
		ri.Printf( PRINT_WARNING, "Autosprite2 shader %s had odd vertex count", tess.surfaceShader->name );
	}

	if ( tess.numIndexes != ( tess.numVertexes >> 2 ) * 6 )
	{
		ri.Printf( PRINT_WARNING, "Autosprite2 shader %s had odd index count", tess.surfaceShader->name );
	}

	if ( backEnd.currentEntity != &tr.worldEntity )
	{
		GlobalVectorToLocal( backEnd.viewParms.orientation.axis[ 0 ], forward );
	}
	else
	{
		VectorCopy( backEnd.viewParms.orientation.axis[ 0 ], forward );
	}

	// this is a lot of work for two triangles...
	// we could precalculate a lot of it is an issue, but it would mess up
	// the shader abstraction
	for ( i = 0, indexes = 0; i < tess.numVertexes; i += 4, indexes += 6 )
	{
		float  lengths[ 2 ];
		int    nums[ 2 ];
		vec3_t mid[ 2 ];
		vec3_t major, minor;
		float  *v1, *v2;

		// find the midpoint
		xyz = tess.xyz[ i ];

		// identify the two shortest edges
		nums[ 0 ] = nums[ 1 ] = 0;
		lengths[ 0 ] = lengths[ 1 ] = 999999;

		for ( j = 0; j < 6; j++ )
		{
			float  l;
			vec3_t temp;

			v1 = xyz + 4 * edgeVerts[ j ][ 0 ];
			v2 = xyz + 4 * edgeVerts[ j ][ 1 ];

			VectorSubtract( v1, v2, temp );

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
			v1 = xyz + 4 * edgeVerts[ nums[ j ] ][ 0 ];
			v2 = xyz + 4 * edgeVerts[ nums[ j ] ][ 1 ];

			mid[ j ][ 0 ] = 0.5f * ( v1[ 0 ] + v2[ 0 ] );
			mid[ j ][ 1 ] = 0.5f * ( v1[ 1 ] + v2[ 1 ] );
			mid[ j ][ 2 ] = 0.5f * ( v1[ 2 ] + v2[ 2 ] );
		}

		// find the vector of the major axis
		VectorSubtract( mid[ 1 ], mid[ 0 ], major );

		// cross this with the view direction to get minor axis
		CrossProduct( major, forward, minor );
		VectorNormalize( minor );

		// re-project the points
		for ( j = 0; j < 2; j++ )
		{
			float l;

			v1 = xyz + 4 * edgeVerts[ nums[ j ] ][ 0 ];
			v2 = xyz + 4 * edgeVerts[ nums[ j ] ][ 1 ];

			l = 0.5 * sqrt( lengths[ j ] );

			// we need to see which direction this edge
			// is used to determine direction of projection
			for ( k = 0; k < 5; k++ )
			{
				if ( tess.indexes[ indexes + k ] == i + edgeVerts[ nums[ j ] ][ 0 ]
				     && tess.indexes[ indexes + k + 1 ] == i + edgeVerts[ nums[ j ] ][ 1 ] )
				{
					break;
				}
			}

			if ( k == 5 )
			{
				VectorMA( mid[ j ], l, minor, v1 );
				VectorMA( mid[ j ], -l, minor, v2 );
			}
			else
			{
				VectorMA( mid[ j ], -l, minor, v1 );
				VectorMA( mid[ j ], l, minor, v2 );
			}
		}
	}
}

qboolean ShaderRequiresCPUDeforms( const shader_t *shader )
{
	if ( shader->numDeforms )
	{
		int      i;
		qboolean cpuDeforms = qfalse;

		if ( glConfig.driverType != GLDRV_OPENGL3 || !r_vboDeformVertexes->integer )
		{
			return qtrue;
		}

		for ( i = 0; i < shader->numDeforms; i++ )
		{
			const deformStage_t *ds = &shader->deforms[ 0 ];

			switch ( ds->deformation )
			{
				case DEFORM_WAVE:
				case DEFORM_BULGE:
				case DEFORM_MOVE:
					break;

				default:
					cpuDeforms = qtrue;
			}
		}

		return cpuDeforms;
	}

	return qfalse;
}

/*
=====================
Tess_DeformGeometry
=====================
*/
void Tess_DeformGeometry( void )
{
	int           i;
	deformStage_t *ds;

#if defined( USE_D3D10 )
	// TODO
#else

	if ( glState.currentVBO != tess.vbo || glState.currentIBO != tess.ibo )
	{
		// static VBOs are incompatible with deformVertexes
		return;
	}

#endif

	if ( !ShaderRequiresCPUDeforms( tess.surfaceShader ) )
	{
		// we don't need the following CPU deforms
		return;
	}

	for ( i = 0; i < tess.surfaceShader->numDeforms; i++ )
	{
		ds = &tess.surfaceShader->deforms[ i ];

		switch ( ds->deformation )
		{
			case DEFORM_NONE:
				break;

			case DEFORM_NORMALS:
				RB_CalcDeformNormals( ds );
				break;

			case DEFORM_WAVE:
				RB_CalcDeformVertexes( ds );
				break;

			case DEFORM_BULGE:
				RB_CalcBulgeVertexes( ds );
				break;

			case DEFORM_MOVE:
				RB_CalcMoveVertexes( ds );
				break;

			case DEFORM_PROJECTION_SHADOW:
				RB_ProjectionShadowDeform();
				break;

			case DEFORM_AUTOSPRITE:
				AutospriteDeform();
				break;

			case DEFORM_AUTOSPRITE2:
				Autosprite2Deform();
				break;

			case DEFORM_TEXT0:
			case DEFORM_TEXT1:
			case DEFORM_TEXT2:
			case DEFORM_TEXT3:
			case DEFORM_TEXT4:
			case DEFORM_TEXT5:
			case DEFORM_TEXT6:
			case DEFORM_TEXT7:
				DeformText( backEnd.refdef.text[ ds->deformation - DEFORM_TEXT0 ] );
				break;

			case DEFORM_SPRITE:
				//AutospriteDeform();
				break;

			case DEFORM_FLARE:
				//Autosprite2Deform();
				break;
		}
	}

#if !defined( USE_D3D10 )
	GL_CheckErrors();
#endif
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
