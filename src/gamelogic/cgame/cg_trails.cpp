/*
===========================================================================
Copyright (C) 2000-2009 Darklegion Development

This file is part of Daemon.

Daemon is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Daemon is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

// cg_trails.c -- the trail system

#include "cg_local.h"

static baseTrailSystem_t baseTrailSystems[ MAX_BASETRAIL_SYSTEMS ];
static baseTrailBeam_t   baseTrailBeams[ MAX_BASETRAIL_BEAMS ];
static int               numBaseTrailSystems = 0;
static int               numBaseTrailBeams = 0;

static trailSystem_t     trailSystems[ MAX_TRAIL_SYSTEMS ];
static trailBeam_t       trailBeams[ MAX_TRAIL_BEAMS ];

/*
===============
CG_CalculateBeamNodeProperties

Fills in trailBeamNode_t.textureCoord
===============
*/
static void CG_CalculateBeamNodeProperties( trailBeam_t *tb )
{
	trailBeamNode_t *i = NULL;
	trailSystem_t   *ts;
	baseTrailBeam_t *btb;
	float           nodeDistances[ MAX_TRAIL_BEAM_NODES ];
	float           totalDistance = 0.0f, position = 0.0f;
	int             j, numNodes = 0;
	float           TCRange, widthRange, alphaRange;
	vec3_t          colorRange;
	float           fadeAlpha = 1.0f;

	if ( !tb || !tb->nodes )
	{
		return;
	}

	ts = tb->parent;
	btb = tb->class_;

	if ( ts->destroyTime > 0 && btb->fadeOutTime )
	{
		fadeAlpha -= ( float )( cg.time - ts->destroyTime ) / btb->fadeOutTime;

		if ( fadeAlpha < 0.0f )
		{
			fadeAlpha = 0.0f;
		}
	}

	TCRange = tb->class_->backTextureCoord -
			          tb->class_->frontTextureCoord;
widthRange = tb->class_->backWidth -
			             tb->class_->frontWidth;
alphaRange = tb->class_->backAlpha -
			             tb->class_->frontAlpha;
	VectorSubtract( tb->class_->backColor,
	                tb->class_->frontColor, colorRange );

	for ( i = tb->nodes; i && i->next; i = i->next )
{
		nodeDistances[ numNodes++ ] =
		  Distance( i->position, i->next->position );
	}

	for ( j = 0; j < numNodes; j++ )
	{
		totalDistance += nodeDistances[ j ];
	}

	for ( j = 0, i = tb->nodes; i; i = i->next, j++ )
	{
		if ( tb->class_->textureType == TBTT_STRETCH )
		{
			i->textureCoord = tb->class_->frontTextureCoord +
					                  ( ( position / totalDistance ) * TCRange );
		}
		else if ( tb->class_->textureType == TBTT_REPEAT )
	{
			if ( tb->class_->clampToBack )
			{
				i->textureCoord = ( totalDistance - position ) /
				                  tb->class_->repeatLength;
			}
			else
			{
				i->textureCoord = position / tb->class_->repeatLength;
			}
		}

		i->halfWidth = ( tb->class_->frontWidth +
		                 ( ( position / totalDistance ) * widthRange ) ) / 2.0f;
		i->alpha = ( byte )( ( float ) 0xFF * ( tb->class_->frontAlpha +
		                                        ( ( position / totalDistance ) * alphaRange ) ) * fadeAlpha );
		VectorMA( tb->class_->frontColor, ( position / totalDistance ),
		          colorRange, i->color );

		position += nodeDistances[ j ];
	}
}

/*
===============
CG_LightVertex

Lights a particular vertex
===============
*/
static void CG_LightVertex( vec3_t point, byte alpha, byte *rgba )
{
	int    i;
	vec3_t alight, dlight, lightdir;

	trap_R_LightForPoint( point, alight, dlight, lightdir );

	for ( i = 0; i <= 2; i++ )
	{
		rgba[ i ] = ( int ) alight[ i ];
	}

	rgba[ 3 ] = alpha;
}

/*
===============
CG_RenderBeam

Renders a beam
===============
*/
static void CG_RenderBeam( trailBeam_t *tb )
{
	trailBeamNode_t   *i = NULL;
	trailBeamNode_t   *prev = NULL;
	trailBeamNode_t   *next = NULL;
	vec3_t            up;
	polyVert_t        verts[( MAX_TRAIL_BEAM_NODES - 1 ) * 4 ];
	int               numVerts = 0;
	baseTrailBeam_t   *btb;
	trailSystem_t     *ts;
	baseTrailSystem_t *bts;

	if ( !tb || !tb->nodes )
	{
		return;
	}

	btb = tb->class_;
	ts = tb->parent;
	bts = ts->class_;

	if ( bts->thirdPersonOnly &&
	     ( CG_AttachmentCentNum( &ts->frontAttachment ) == cg.snap->ps.clientNum ||
	       CG_AttachmentCentNum( &ts->backAttachment ) == cg.snap->ps.clientNum ) &&
	     !cg.renderingThirdPerson )
	{
		return;
	}

	CG_CalculateBeamNodeProperties( tb );

	i = tb->nodes;

	do
	{
		prev = i->prev;
		next = i->next;

		if ( prev && next )
		{
			//this node has two neighbours
			GetPerpendicularViewVector( cg.refdef.vieworg, next->position, prev->position, up );
		}
		else if ( !prev && next )
		{
			//this is the front
			GetPerpendicularViewVector( cg.refdef.vieworg, next->position, i->position, up );
		}
		else if ( prev && !next )
		{
			//this is the back
			GetPerpendicularViewVector( cg.refdef.vieworg, i->position, prev->position, up );
		}
		else
		{
			break;
		}

		if ( prev )
		{
			VectorMA( i->position, i->halfWidth, up, verts[ numVerts ].xyz );
			verts[ numVerts ].st[ 0 ] = i->textureCoord;
			verts[ numVerts ].st[ 1 ] = 1.0f;

			if ( btb->realLight )
			{
				CG_LightVertex( verts[ numVerts ].xyz, i->alpha, verts[ numVerts ].modulate );
			}
			else
			{
				VectorCopy( i->color, verts[ numVerts ].modulate );
				verts[ numVerts ].modulate[ 3 ] = i->alpha;
			}

			numVerts++;

			VectorMA( i->position, -i->halfWidth, up, verts[ numVerts ].xyz );
			verts[ numVerts ].st[ 0 ] = i->textureCoord;
			verts[ numVerts ].st[ 1 ] = 0.0f;

			if ( btb->realLight )
			{
				CG_LightVertex( verts[ numVerts ].xyz, i->alpha, verts[ numVerts ].modulate );
			}
			else
			{
				VectorCopy( i->color, verts[ numVerts ].modulate );
				verts[ numVerts ].modulate[ 3 ] = i->alpha;
			}

			numVerts++;
		}

		if ( next )
		{
			VectorMA( i->position, -i->halfWidth, up, verts[ numVerts ].xyz );
			verts[ numVerts ].st[ 0 ] = i->textureCoord;
			verts[ numVerts ].st[ 1 ] = 0.0f;

			if ( btb->realLight )
			{
				CG_LightVertex( verts[ numVerts ].xyz, i->alpha, verts[ numVerts ].modulate );
			}
			else
			{
				VectorCopy( i->color, verts[ numVerts ].modulate );
				verts[ numVerts ].modulate[ 3 ] = i->alpha;
			}

			numVerts++;

			VectorMA( i->position, i->halfWidth, up, verts[ numVerts ].xyz );
			verts[ numVerts ].st[ 0 ] = i->textureCoord;
			verts[ numVerts ].st[ 1 ] = 1.0f;

			if ( btb->realLight )
			{
				CG_LightVertex( verts[ numVerts ].xyz, i->alpha, verts[ numVerts ].modulate );
			}
			else
			{
				VectorCopy( i->color, verts[ numVerts ].modulate );
				verts[ numVerts ].modulate[ 3 ] = i->alpha;
			}

			numVerts++;
		}

		i = i->next;
	}
	while ( i );

	trap_R_AddPolysToScene( tb->class_->shader, 4, &verts[ 0 ], numVerts / 4 );
}

/*
===============
CG_AllocateBeamNode

Allocates a trailBeamNode_t from a trailBeam_t's nodePool
===============
*/
static trailBeamNode_t *CG_AllocateBeamNode( trailBeam_t *tb )
{
	baseTrailBeam_t *btb = tb->class_;
	int             i;
	trailBeamNode_t *tbn;

	for ( i = 0; i < MAX_TRAIL_BEAM_NODES; i++ )
	{
		tbn = &tb->nodePool[ i ];

		if ( !tbn->used )
		{
			tbn->timeLeft = btb->segmentTime;
			tbn->prev = NULL;
			tbn->next = NULL;
			tbn->used = qtrue;
			return tbn;
		}
	}

	// no space left
	return NULL;
}

/*
===============
CG_DestroyBeamNode

Removes a node from a beam
Returns the new head
===============
*/
static trailBeamNode_t *CG_DestroyBeamNode( trailBeamNode_t *tbn )
{
	trailBeamNode_t *newHead = NULL;

	if ( tbn->prev )
	{
		if ( tbn->next )
		{
			// node is in the middle
			tbn->prev->next = tbn->next;
			tbn->next->prev = tbn->prev;
		}
		else // node is at the back
		{
			tbn->prev->next = NULL;
		}

		// find the new head (shouldn't have changed)
		newHead = tbn->prev;

		while ( newHead->prev )
		{
			newHead = newHead->prev;
		}
	}
	else if ( tbn->next )
	{
		//node is at the front
		tbn->next->prev = NULL;
		newHead = tbn->next;
	}

	tbn->prev = NULL;
	tbn->next = NULL;
	tbn->used = qfalse;

	return newHead;
}

/*
===============
CG_FindLastBeamNode

Returns the last beam node in a beam
===============
*/
static trailBeamNode_t *CG_FindLastBeamNode( trailBeam_t *tb )
{
	trailBeamNode_t *i = tb->nodes;

	while ( i && i->next )
	{
		i = i->next;
	}

	return i;
}

/*
===============
CG_CountBeamNodes

Returns the number of nodes in a beam
===============
*/
static int CG_CountBeamNodes( trailBeam_t *tb )
{
	trailBeamNode_t *i = tb->nodes;
	int             numNodes = 0;

	while ( i )
	{
		numNodes++;
		i = i->next;
	}

	return numNodes;
}

/*
===============
CG_PrependBeamNode

Prepend a new beam node to the front of a beam
Returns the new node
===============
*/
static trailBeamNode_t *CG_PrependBeamNode( trailBeam_t *tb )
{
	trailBeamNode_t *i;

	if ( tb->nodes )
	{
		// prepend another node
		i = CG_AllocateBeamNode( tb );

		if ( i )
		{
			i->next = tb->nodes;
			tb->nodes->prev = i;
			tb->nodes = i;
		}
	}
	else //add first node
	{
		i = CG_AllocateBeamNode( tb );

		if ( i )
		{
			tb->nodes = i;
		}
	}

	return i;
}

/*
===============
CG_AppendBeamNode

Append a new beam node to the back of a beam
Returns the new node
===============
*/
static trailBeamNode_t *CG_AppendBeamNode( trailBeam_t *tb )
{
	trailBeamNode_t *last, *i;

	if ( tb->nodes )
	{
		// append another node
		last = CG_FindLastBeamNode( tb );
		i = CG_AllocateBeamNode( tb );

		if ( i )
		{
			last->next = i;
			i->prev = last;
			i->next = NULL;
		}
	}
	else //add first node
	{
		i = CG_AllocateBeamNode( tb );

		if ( i )
		{
			tb->nodes = i;
		}
	}

	return i;
}

/*
===============
CG_ApplyJitters
===============
*/
static void CG_ApplyJitters( trailBeam_t *tb )
{
	trailBeamNode_t *i = NULL;
	int             j;
	baseTrailBeam_t *btb;
	trailSystem_t   *ts;
	trailBeamNode_t *start;
	trailBeamNode_t *end;

	if ( !tb || !tb->nodes )
	{
		return;
	}

	btb = tb->class_;
	ts = tb->parent;

	for ( j = 0; j < btb->numJitters; j++ )
	{
		if ( tb->nextJitterTimes[ j ] <= cg.time )
		{
			for ( i = tb->nodes; i; i = i->next )
			{
				i->jitters[ j ][ 0 ] = ( crandom() * btb->jitters[ j ].magnitude );
				i->jitters[ j ][ 1 ] = ( crandom() * btb->jitters[ j ].magnitude );
			}

			tb->nextJitterTimes[ j ] = cg.time + btb->jitters[ j ].period;
		}
	}

	start = tb->nodes;
	end = CG_FindLastBeamNode( tb );

	if ( !btb->jitterAttachments )
	{
		if ( CG_Attached( &ts->frontAttachment ) && start->next )
		{
			start = start->next;
		}

		if ( CG_Attached( &ts->backAttachment ) && end->prev )
		{
			end = end->prev;
		}
	}

	for ( i = start; i; i = i->next )
	{
		vec3_t          forward, right, up;
		trailBeamNode_t *prev;
		trailBeamNode_t *next;
		float           upJitter = 0.0f, rightJitter = 0.0f;

		prev = i->prev;
		next = i->next;

		if ( prev && next )
		{
			//this node has two neighbours
			GetPerpendicularViewVector( cg.refdef.vieworg, next->position, prev->position, up );
			VectorSubtract( next->position, prev->position, forward );
		}
		else if ( !prev && next )
		{
			//this is the front
			GetPerpendicularViewVector( cg.refdef.vieworg, next->position, i->position, up );
			VectorSubtract( next->position, i->position, forward );
		}
		else if ( prev && !next )
		{
			//this is the back
			GetPerpendicularViewVector( cg.refdef.vieworg, i->position, prev->position, up );
			VectorSubtract( i->position, prev->position, forward );
		}

		VectorNormalize( forward );
		CrossProduct( forward, up, right );
		VectorNormalize( right );

		for ( j = 0; j < btb->numJitters; j++ )
		{
			upJitter += i->jitters[ j ][ 0 ];
			rightJitter += i->jitters[ j ][ 1 ];
		}

		VectorMA( i->position, upJitter, up, i->position );
		VectorMA( i->position, rightJitter, right, i->position );

		if ( i == end )
		{
			break;
		}
	}
}

/*
===============
CG_UpdateBeam

Updates a beam
===============
*/
static void CG_UpdateBeam( trailBeam_t *tb )
{
	baseTrailBeam_t *btb;
	trailSystem_t   *ts;
	trailBeamNode_t *i;
	int             deltaTime;
	int             nodesToAdd;
	int             j;
	int             numNodes;

	if ( !tb )
	{
		return;
	}

	btb = tb->class_;
	ts = tb->parent;

	deltaTime = cg.time - tb->lastEvalTime;
	tb->lastEvalTime = cg.time;

	// first make sure this beam has enough nodes
	if ( ts->destroyTime <= 0 )
	{
		nodesToAdd = btb->numSegments - CG_CountBeamNodes( tb ) + 1;

		while ( nodesToAdd-- )
		{
			i = CG_AppendBeamNode( tb );

			if ( !tb->nodes->next && CG_Attached( &ts->frontAttachment ) )
			{
				// this is the first node to be added
				if ( !CG_AttachmentPoint( &ts->frontAttachment, i->refPosition ) )
				{
					CG_DestroyTrailSystem( &ts );
				}
			}
			else
			{
				VectorCopy( i->prev->refPosition, i->refPosition );
			}
		}
	}

	numNodes = CG_CountBeamNodes( tb );

	for ( i = tb->nodes; i; i = i->next )
	{
		VectorCopy( i->refPosition, i->position );
	}

	if ( CG_Attached( &ts->frontAttachment ) && CG_Attached( &ts->backAttachment ) )
	{
		// beam between two attachments
		vec3_t dir, front, back;

		if ( ts->destroyTime > 0 && ( cg.time - ts->destroyTime ) >= btb->fadeOutTime )
		{
			tb->valid = qfalse;
			return;
		}

		if ( !CG_AttachmentPoint( &ts->frontAttachment, front ) )
		{
			CG_DestroyTrailSystem( &ts );
		}

		if ( !CG_AttachmentPoint( &ts->backAttachment, back ) )
		{
			CG_DestroyTrailSystem( &ts );
		}

		VectorSubtract( back, front, dir );

		for ( j = 0, i = tb->nodes; i; i = i->next, j++ )
		{
			float scale = ( float ) j / ( float )( numNodes - 1 );

			VectorMA( front, scale, dir, i->position );
		}
	}
	else if ( CG_Attached( &ts->frontAttachment ) )
	{
		// beam from one attachment

		// cull the trail tail
		i = CG_FindLastBeamNode( tb );

		if ( i && i->timeLeft >= 0 )
		{
			i->timeLeft -= deltaTime;

			if ( i->timeLeft < 0 )
			{
				tb->nodes = CG_DestroyBeamNode( i );

				if ( !tb->nodes )
				{
					tb->valid = qfalse;
					return;
				}

				// if the ts has been destroyed, stop creating new nodes
				if ( ts->destroyTime <= 0 )
				{
					CG_PrependBeamNode( tb );
				}
			}
			else if ( i->timeLeft >= 0 && i->prev )
			{
				vec3_t dir;
				float  length;

				VectorSubtract( i->refPosition, i->prev->refPosition, dir );
				length = VectorNormalize( dir ) *
				         ( ( float ) i->timeLeft / ( float ) tb->class_->segmentTime );

				VectorMA( i->prev->refPosition, length, dir, i->position );
			}
		}

		if ( tb->nodes )
		{
			if ( !CG_AttachmentPoint( &ts->frontAttachment, tb->nodes->refPosition ) )
			{
				CG_DestroyTrailSystem( &ts );
			}

			VectorCopy( tb->nodes->refPosition, tb->nodes->position );
		}
	}

	CG_ApplyJitters( tb );
}

/*
===============
CG_ParseTrailBeam

Parse a trail beam
===============
*/
static qboolean CG_ParseTrailBeam( baseTrailBeam_t *btb, char **text_p )
{
	char *token;

	// read optional parameters
	while ( 1 )
	{
		token = COM_Parse( text_p );

		if ( !*token )
		{
			return qfalse;
		}

		if ( !Q_stricmp( token, "segments" ) )
		{
			token = COM_Parse( text_p );

			if ( !*token )
			{
				break;
			}

			btb->numSegments = atoi_neg( token, qfalse );

			if ( btb->numSegments >= MAX_TRAIL_BEAM_NODES )
			{
				btb->numSegments = MAX_TRAIL_BEAM_NODES - 1;
				CG_Printf( S_WARNING "too many segments in trail beam\n" );
			}

			continue;
		}
		else if ( !Q_stricmp( token, "width" ) )
		{
			token = COM_Parse( text_p );

			if ( !*token )
			{
				break;
			}

			btb->frontWidth = atof_neg( token, qfalse );

			token = COM_Parse( text_p );

			if ( !*token )
			{
				break;
			}

			if ( !Q_stricmp( token, "-" ) )
			{
				btb->backWidth = btb->frontWidth;
			}
			else
			{
				btb->backWidth = atof_neg( token, qfalse );
			}

			continue;
		}
		else if ( !Q_stricmp( token, "alpha" ) )
		{
			token = COM_Parse( text_p );

			if ( !*token )
			{
				break;
			}

			btb->frontAlpha = atof_neg( token, qfalse );

			token = COM_Parse( text_p );

			if ( !*token )
			{
				break;
			}

			if ( !Q_stricmp( token, "-" ) )
			{
				btb->backAlpha = btb->frontAlpha;
			}
			else
			{
				btb->backAlpha = atof_neg( token, qfalse );
			}

			continue;
		}
		else if ( !Q_stricmp( token, "color" ) )
		{
			token = COM_Parse( text_p );

			if ( !*token )
			{
				break;
			}

			if ( !Q_stricmp( token, "{" ) )
			{
				if ( !CG_ParseColor( btb->frontColor, text_p ) )
				{
					break;
				}

				token = COM_Parse( text_p );

				if ( !*token )
				{
					break;
				}

				if ( !Q_stricmp( token, "-" ) )
				{
					btb->backColor[ 0 ] = btb->frontColor[ 0 ];
					btb->backColor[ 1 ] = btb->frontColor[ 1 ];
					btb->backColor[ 2 ] = btb->frontColor[ 2 ];
				}
				else if ( !Q_stricmp( token, "{" ) )
				{
					if ( !CG_ParseColor( btb->backColor, text_p ) )
					{
						break;
					}
				}
				else
				{
					CG_Printf( S_ERROR "missing '{'\n" );
					break;
				}
			}
			else
			{
				CG_Printf( S_ERROR "missing '{'\n" );
				break;
			}

			continue;
		}
		else if ( !Q_stricmp( token, "segmentTime" ) )
		{
			token = COM_Parse( text_p );

			if ( !*token )
			{
				break;
			}

			btb->segmentTime = atoi_neg( token, qfalse );
			continue;
		}
		else if ( !Q_stricmp( token, "fadeOutTime" ) )
		{
			token = COM_Parse( text_p );

			if ( !*token )
			{
				break;
			}

			btb->fadeOutTime = atoi_neg( token, qfalse );
			continue;
		}
		else if ( !Q_stricmp( token, "shader" ) )
		{
			token = COM_Parse( text_p );

			if ( !*token )
			{
				break;
			}

			Q_strncpyz( btb->shaderName, token, MAX_QPATH );

			continue;
		}
		else if ( !Q_stricmp( token, "textureType" ) )
		{
			token = COM_Parse( text_p );

			if ( !*token )
			{
				break;
			}

			if ( !Q_stricmp( token, "stretch" ) )
			{
				btb->textureType = TBTT_STRETCH;

				token = COM_Parse( text_p );

				if ( !*token )
				{
					break;
				}

				btb->frontTextureCoord = atof_neg( token, qfalse );

				token = COM_Parse( text_p );

				if ( !*token )
				{
					break;
				}

				btb->backTextureCoord = atof_neg( token, qfalse );
			}
			else if ( !Q_stricmp( token, "repeat" ) )
			{
				btb->textureType = TBTT_REPEAT;

				token = COM_Parse( text_p );

				if ( !*token )
				{
					break;
				}

				if ( !Q_stricmp( token, "front" ) )
				{
					btb->clampToBack = qfalse;
				}
				else if ( !Q_stricmp( token, "back" ) )
				{
					btb->clampToBack = qtrue;
				}
				else
				{
					CG_Printf( S_ERROR "unknown textureType clamp \"%s\"\n", token );
					break;
				}

				token = COM_Parse( text_p );

				if ( !*token )
				{
					break;
				}

				btb->repeatLength = atof_neg( token, qfalse );
			}
			else
			{
				CG_Printf( S_ERROR "unknown textureType \"%s\"\n", token );
				break;
			}

			continue;
		}
		else if ( !Q_stricmp( token, "realLight" ) )
		{
			btb->realLight = qtrue;

			continue;
		}
		else if ( !Q_stricmp( token, "jitter" ) )
		{
			if ( btb->numJitters == MAX_TRAIL_BEAM_JITTERS )
			{
				CG_Printf( S_ERROR "too many jitters\n" );
				break;
			}

			token = COM_Parse( text_p );

			if ( !*token )
			{
				break;
			}

			btb->jitters[ btb->numJitters ].magnitude = atof_neg( token, qfalse );

			token = COM_Parse( text_p );

			if ( !*token )
			{
				break;
			}

			btb->jitters[ btb->numJitters ].period = atoi_neg( token, qfalse );

			btb->numJitters++;

			continue;
		}
		else if ( !Q_stricmp( token, "jitterAttachments" ) )
		{
			btb->jitterAttachments = qtrue;

			continue;
		}
		else if ( !Q_stricmp( token, "}" ) )
		{
			return qtrue; //reached the end of this trail beam
		}
		else
		{
			CG_Printf( S_ERROR "unknown token '%s' in trail beam\n", token );
			return qfalse;
		}
	}

	return qfalse;
}

/*
===============
CG_InitialiseBaseTrailBeam
===============
*/
static void CG_InitialiseBaseTrailBeam( baseTrailBeam_t *btb )
{
	memset( btb, 0, sizeof( baseTrailBeam_t ) );

	btb->numSegments = 1;
	btb->frontWidth = btb->backWidth = 1.0f;
	btb->frontAlpha = btb->backAlpha = 1.0f;
	memset( btb->frontColor, 0xFF, sizeof( btb->frontColor ) );
	memset( btb->backColor, 0xFF, sizeof( btb->backColor ) );

	btb->segmentTime = 100;

	btb->textureType = TBTT_STRETCH;
	btb->frontTextureCoord = 0.0f;
	btb->backTextureCoord = 1.0f;
}

/*
===============
CG_ParseTrailSystem

Parse a trail system section
===============
*/
static qboolean CG_ParseTrailSystem( baseTrailSystem_t *bts, char **text_p, const char *name )
{
	char *token;

	// read optional parameters
	while ( 1 )
	{
		token = COM_Parse( text_p );

		if ( !*token )
		{
			return qfalse;
		}

		if ( !Q_stricmp( token, "{" ) )
		{
			CG_InitialiseBaseTrailBeam( &baseTrailBeams[ numBaseTrailBeams ] );

			if ( !CG_ParseTrailBeam( &baseTrailBeams[ numBaseTrailBeams ], text_p ) )
			{
				CG_Printf( S_ERROR "failed to parse trail beam\n" );
				return qfalse;
			}

			if ( bts->numBeams == MAX_BEAMS_PER_SYSTEM )
			{
				CG_Printf( S_ERROR "trail system has > %d beams\n", MAX_BEAMS_PER_SYSTEM );
				return qfalse;
			}
			else if ( numBaseTrailBeams == MAX_BASETRAIL_BEAMS )
			{
				CG_Printf( S_ERROR "maximum number of trail beams (%d) reached\n",
				           MAX_BASETRAIL_BEAMS );
				return qfalse;
			}
			else
			{
				//start parsing beams again
				bts->beams[ bts->numBeams ] = &baseTrailBeams[ numBaseTrailBeams ];
				bts->numBeams++;
				numBaseTrailBeams++;
			}

			continue;
		}
		else if ( !Q_stricmp( token, "thirdPersonOnly" ) )
		{
			bts->thirdPersonOnly = qtrue;
		}
		else if ( !Q_stricmp( token, "lifeTime" ) )
		{
			token = COM_Parse( text_p );

			if ( !*token )
			{
				break;
			}

			bts->lifeTime = atoi_neg( token, qfalse );
			continue;
		}
		else if ( !Q_stricmp( token, "beam" ) )  //acceptable text
		{
			continue;
		}
		else if ( !Q_stricmp( token, "}" ) )
		{
			if ( cg_debugTrails.integer >= 1 )
			{
				CG_Printf( "Parsed trail system %s\n", name );
			}

			return qtrue; //reached the end of this trail system
		}
		else
		{
			CG_Printf( "ERROR: unknown token '%s' in trail system %s\n", token, bts->name );
			return qfalse;
		}
	}

	return qfalse;
}

/*
===============
CG_ParseTrailFile

Load the trail systems from a trail file
===============
*/
static qboolean CG_ParseTrailFile( const char *fileName )
{
	char         *text_p;
	int          i;
	int          len;
	char         *token;
	char         text[ 32000 ];
	char         tsName[ MAX_QPATH ];
	qboolean     tsNameSet = qfalse;
	fileHandle_t f;

	// load the file
	len = trap_FS_FOpenFile( fileName, &f, FS_READ );

	if ( len <= 0 )
	{
		return qfalse;
	}

	if ( len == 0 || len + 1 >= (int) sizeof( text ) )
	{
		trap_FS_FCloseFile( f );
		CG_Printf( len ? S_ERROR "trail file %s is too long\n" : S_ERROR "trail file %s is empty\n", fileName );
		return qfalse;
	}

	trap_FS_Read( text, len, f );
	text[ len ] = 0;
	trap_FS_FCloseFile( f );

	// parse the text
	text_p = text;

	// read optional parameters
	while ( 1 )
	{
		token = COM_Parse( &text_p );

		if ( !*token )
		{
			break;
		}

		if ( !Q_stricmp( token, "{" ) )
		{
			if ( tsNameSet )
			{
				//check for name space clashes
				for ( i = 0; i < numBaseTrailSystems; i++ )
				{
					if ( !Q_stricmp( baseTrailSystems[ i ].name, tsName ) )
					{
						CG_Printf( S_ERROR "a trail system is already named %s\n", tsName );
						return qfalse;
					}
				}

				Q_strncpyz( baseTrailSystems[ numBaseTrailSystems ].name, tsName, MAX_QPATH );

				if ( !CG_ParseTrailSystem( &baseTrailSystems[ numBaseTrailSystems ], &text_p, tsName ) )
				{
					CG_Printf( S_ERROR "%s: failed to parse trail system %s\n", fileName, tsName );
					return qfalse;
				}

				//start parsing trail systems again
				tsNameSet = qfalse;

				if ( numBaseTrailSystems == MAX_BASETRAIL_SYSTEMS )
				{
					CG_Printf( S_ERROR "maximum number of trail systems (%d) reached\n",
					           MAX_BASETRAIL_SYSTEMS );
					return qfalse;
				}
				else
				{
					numBaseTrailSystems++;
				}

				continue;
			}
			else
			{
				CG_Printf( S_ERROR "unnamed trail system\n" );
				return qfalse;
			}
		}

		if ( !tsNameSet )
		{
			Q_strncpyz( tsName, token, sizeof( tsName ) );
			tsNameSet = qtrue;
		}
		else
		{
			CG_Printf( S_ERROR "trail system already named\n" );
			return qfalse;
		}
	}

	return qtrue;
}

/*
===============
CG_LoadTrailSystems

Load trail system templates
===============
*/
void CG_LoadTrailSystems( void )
{
	int  i, numFiles, fileLen;
	char fileList[ MAX_TRAIL_FILES * MAX_QPATH ];
	char fileName[ MAX_QPATH ];
	char *filePtr;

	//clear out the old
	numBaseTrailSystems = 0;
	numBaseTrailBeams = 0;

	for ( i = 0; i < MAX_BASETRAIL_SYSTEMS; i++ )
	{
		baseTrailSystem_t *bts = &baseTrailSystems[ i ];
		memset( bts, 0, sizeof( baseTrailSystem_t ) );
	}

	for ( i = 0; i < MAX_BASETRAIL_BEAMS; i++ )
	{
		baseTrailBeam_t *btb = &baseTrailBeams[ i ];
		memset( btb, 0, sizeof( baseTrailBeam_t ) );
	}

	//and bring in the new
	numFiles = trap_FS_GetFileList( "scripts", ".trail",
	                                fileList, MAX_TRAIL_FILES * MAX_QPATH );
	filePtr = fileList;

	for ( i = 0; i < numFiles; i++, filePtr += fileLen + 1 )
	{
		fileLen = strlen( filePtr );
		strcpy( fileName, "scripts/" );
		strcat( fileName, filePtr );
		// CG_Printf(_( "...loading '%s'\n"), fileName );
		CG_ParseTrailFile( fileName );
	}
}

/*
===============
CG_RegisterTrailSystem

Load the media that a trail system needs
===============
*/
qhandle_t CG_RegisterTrailSystem( const char *name )
{
	int               i, j;
	baseTrailSystem_t *bts;
	baseTrailBeam_t   *btb;

	for ( i = 0; i < MAX_BASETRAIL_SYSTEMS; i++ )
	{
		bts = &baseTrailSystems[ i ];

		if ( !Q_stricmp( bts->name, name ) )
		{
			//already registered
			if ( bts->registered )
			{
				return i + 1;
			}

			for ( j = 0; j < bts->numBeams; j++ )
			{
				btb = bts->beams[ j ];

				btb->shader = trap_R_RegisterShader(btb->shaderName,
								    RSF_DEFAULT);
			}

			if ( cg_debugTrails.integer >= 1 )
			{
				CG_Printf( "Registered trail system %s\n", name );
			}

			bts->registered = qtrue;

			//avoid returning 0
			return i + 1;
		}
	}

	CG_Printf( S_ERROR "failed to register trail system %s\n", name );
	return 0;
}

/*
===============
CG_SpawnNewTrailBeam

Allocate a new trail beam
===============
*/
static trailBeam_t *CG_SpawnNewTrailBeam( baseTrailBeam_t *btb,
    trailSystem_t *parent )
{
	int           i;
	trailBeam_t   *tb = NULL;
	trailSystem_t *ts = parent;

	for ( i = 0; i < MAX_TRAIL_BEAMS; i++ )
	{
		tb = &trailBeams[ i ];

		if ( !tb->valid )
		{
			memset( tb, 0, sizeof( trailBeam_t ) );

			//found a free slot
			tb->class_ = btb;
			tb->parent = ts;

			tb->valid = qtrue;

			if ( cg_debugTrails.integer >= 1 )
			{
				CG_Printf( "TB %s created\n", ts->class_->name );
			}

			return tb;
		}
	}

	if ( cg_debugTrails.integer >= 1 )
	{
		CG_Printf( "MAX_TRAIL_BEAMS\n" );
	}

	return NULL;
}

/*
===============
CG_SpawnNewTrailSystem

Spawns a new trail system
===============
*/
trailSystem_t *CG_SpawnNewTrailSystem( qhandle_t psHandle )
{
	int               i, j;
	trailSystem_t     *ts = NULL;
	baseTrailSystem_t *bts = &baseTrailSystems[ psHandle - 1 ];

	if ( !bts->registered )
	{
		CG_Printf( S_ERROR "a trail system has not been registered yet\n" );
		return NULL;
	}

	for ( i = 0; i < MAX_TRAIL_SYSTEMS; i++ )
	{
		ts = &trailSystems[ i ];

		if ( !ts->valid )
		{
			memset( ts, 0, sizeof( trailSystem_t ) );

			//found a free slot
			ts->class_ = bts;

			ts->valid = qtrue;
			ts->destroyTime = -1;
			ts->birthTime = cg.time;

			for ( j = 0; j < bts->numBeams; j++ )
			{
				CG_SpawnNewTrailBeam( bts->beams[ j ], ts );
			}

			if ( cg_debugTrails.integer >= 1 )
			{
				CG_Printf( "TS %s created\n", bts->name );
			}

			return ts;
		}
	}

	if ( cg_debugTrails.integer >= 1 )
	{
		CG_Printf( "MAX_TRAIL_SYSTEMS\n" );
	}

	return NULL;
}

/*
===============
CG_DestroyTrailSystem

Destroy a trail system
===============
*/
void CG_DestroyTrailSystem( trailSystem_t **ts )
{
	( *ts )->destroyTime = cg.time;

	if ( CG_Attached( & ( *ts )->frontAttachment ) &&
	     !CG_Attached( & ( *ts )->backAttachment ) )
	{
		vec3_t v;

		// attach the trail head to a static point
		CG_AttachmentPoint( & ( *ts )->frontAttachment, v );
		CG_SetAttachmentPoint( & ( *ts )->frontAttachment, v );
		CG_AttachToPoint( & ( *ts )->frontAttachment );

		( *ts )->frontAttachment.centValid = qfalse; // a bit naughty
	}

	ts = NULL;
}

/*
===============
CG_IsTrailSystemValid

Test a trail system for validity
===============
*/
qboolean CG_IsTrailSystemValid( trailSystem_t **ts )
{
	if ( *ts == NULL || ( *ts && !( *ts )->valid ) )
	{
		if ( *ts && !( *ts )->valid )
		{
			*ts = NULL;
		}

		return qfalse;
	}

	return qtrue;
}

/*
===============
CG_GarbageCollectTrailSystems

Destroy inactive trail systems
===============
*/
static void CG_GarbageCollectTrailSystems( void )
{
	int           i, j, count;
	trailSystem_t *ts;
	trailBeam_t   *tb;
	int           centNum;

	for ( i = 0; i < MAX_TRAIL_SYSTEMS; i++ )
	{
		ts = &trailSystems[ i ];
		count = 0;

		//don't bother checking already invalid systems
		if ( !ts->valid )
		{
			continue;
		}

		for ( j = 0; j < MAX_TRAIL_BEAMS; j++ )
		{
			tb = &trailBeams[ j ];

			if ( tb->valid && tb->parent == ts )
			{
				count++;
			}
		}

		if ( !count )
		{
			ts->valid = qfalse;
		}

		//check systems where the parent cent has left the PVS
		//( local player entity is always valid )
		if ( ( centNum = CG_AttachmentCentNum( &ts->frontAttachment ) ) >= 0 &&
		     centNum != cg.snap->ps.clientNum )
		{
			trailSystem_t *tempTS = ts;

			if ( !cg_entities[ centNum ].valid )
			{
				CG_DestroyTrailSystem( &tempTS );
			}
		}

		if ( ( centNum = CG_AttachmentCentNum( &ts->backAttachment ) ) >= 0 &&
		     centNum != cg.snap->ps.clientNum )
		{
			trailSystem_t *tempTS = ts;

			if ( !cg_entities[ centNum ].valid )
			{
				CG_DestroyTrailSystem( &tempTS );
			}
		}

		// lifetime expired
		if ( ts->destroyTime <= 0 && ts->class_->lifeTime &&
		     ts->birthTime + ts->class_->lifeTime < cg.time )
		{
			trailSystem_t *tempTS = ts;

			CG_DestroyTrailSystem( &tempTS );

			if ( cg_debugTrails.integer >= 1 )
			{
				CG_Printf( "TS %s expired (born %d, lives %d, now %d)\n",
				           ts->class_->name, ts->birthTime, ts->class_->lifeTime,
				           cg.time );
			}
		}

		if ( cg_debugTrails.integer >= 1 && !ts->valid )
		{
			CG_Printf( "TS %s garbage collected\n", ts->class_->name );
		}
	}
}

/*
===============
CG_AddTrails

Add trails to the scene
===============
*/
void CG_AddTrails( void )
{
	int         i;
	trailBeam_t *tb;
	int         numTS = 0, numTB = 0;

	//remove expired trail systems
	CG_GarbageCollectTrailSystems();

	for ( i = 0; i < MAX_TRAIL_BEAMS; i++ )
	{
		tb = &trailBeams[ i ];

		if ( tb->valid )
		{
			CG_UpdateBeam( tb );
			CG_RenderBeam( tb );
		}
	}

	if ( cg_debugTrails.integer >= 2 )
	{
		for ( i = 0; i < MAX_TRAIL_SYSTEMS; i++ )
		{
			if ( trailSystems[ i ].valid )
			{
				numTS++;
			}
		}

		for ( i = 0; i < MAX_TRAIL_BEAMS; i++ )
		{
			if ( trailBeams[ i ].valid )
			{
				numTB++;
			}
		}

		CG_Printf( "TS: %d  TB: %d\n", numTS, numTB );
	}
}

static trailSystem_t *testTS;
static qhandle_t     testTSHandle;

/*
===============
CG_DestroyTestTS_f

Destroy the test a trail system
===============
*/
void CG_DestroyTestTS_f( void )
{
	if ( CG_IsTrailSystemValid( &testTS ) )
	{
		CG_DestroyTrailSystem( &testTS );
	}
}

/*
===============
CG_TestTS_f

Test a trail system
===============
*/
void CG_TestTS_f( void )
{
	char tsName[ MAX_QPATH ];

	if ( trap_Argc() < 2 )
	{
		return;
	}

	Q_strncpyz( tsName, CG_Argv( 1 ), MAX_QPATH );
	testTSHandle = CG_RegisterTrailSystem( tsName );

	if ( testTSHandle )
	{
		CG_DestroyTestTS_f();

		testTS = CG_SpawnNewTrailSystem( testTSHandle );

		if ( CG_IsTrailSystemValid( &testTS ) )
		{
			CG_SetAttachmentCent( &testTS->frontAttachment, &cg_entities[ 0 ] );
			CG_AttachToCent( &testTS->frontAttachment );
		}
	}
}
