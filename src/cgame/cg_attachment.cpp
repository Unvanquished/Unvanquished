/*
===========================================================================

Unvanquished GPL Source Code
Copyright (C) 2000-2009 Darklegion Development

This file is part of the Unvanquished GPL Source Code (Unvanquished Source Code).

Unvanquished is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Unvanquished is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Unvanquished; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

===========================================================================
*/

// cg_attachment.c -- an abstract attachment system

#include "cg_local.h"

/*
===============
CG_AttachmentPoint

Return the attachment point
===============
*/
bool CG_AttachmentPoint( attachment_t *a, vec3_t v )
{
	centity_t *cent;

	if ( !a )
	{
		return false;
	}

	// if it all breaks, then use the last point we know was correct
	VectorCopy( a->lastValidAttachmentPoint, v );

	switch ( a->type )
	{
		case AT_STATIC:
			if ( !a->staticValid )
			{
				return false;
			}

			VectorCopy( a->origin, v );
			break;

		case AT_TAG:
			if ( !a->tagValid )
			{
				return false;
			}

			AxisCopy( axisDefault, a->re.axis );
			CG_PositionRotatedEntityOnTag( &a->re, &a->parent,
			                               a->model, a->tagName );
			VectorCopy( a->re.origin, v );
			break;

		case AT_CENT:
			if ( !a->centValid )
			{
				return false;
			}

			if ( a->centNum == cg.predictedPlayerState.clientNum )
			{
				// this is smoother if it's the local client
				VectorCopy( cg.predictedPlayerState.origin, v );
			}
			else
			{
				cent = &cg_entities[ a->centNum ];
				VectorCopy( cent->lerpOrigin, v );
			}

			break;

		case AT_PARTICLE:
			if ( !a->particleValid )
			{
				return false;
			}

			if ( !a->particle->valid )
			{
				a->particleValid = false;
				return false;
			}
			else
			{
				VectorCopy( a->particle->origin, v );
			}

			break;

		default:
			Log::Warn( "Invalid attachmentType_t in attachment" );
			break;
	}

	if ( a->hasOffset )
	{
		VectorAdd( v, a->offset, v );
	}

	VectorCopy( v, a->lastValidAttachmentPoint );

	return true;
}

/*
===============
CG_AttachmentDir

Return the attachment direction
===============
*/
bool CG_AttachmentDir( attachment_t *a, vec3_t v )
{
	vec3_t    forward;
	centity_t *cent;

	if ( !a )
	{
		return false;
	}

	switch ( a->type )
	{
		case AT_STATIC:
			return false;

		case AT_TAG:
			if ( !a->tagValid )
			{
				return false;
			}

			VectorCopy( a->re.axis[ 0 ], v );
			break;

		case AT_CENT:
			if ( !a->centValid )
			{
				return false;
			}

			cent = &cg_entities[ a->centNum ];
			AngleVectors( cent->lerpAngles, forward, nullptr, nullptr );
			VectorCopy( forward, v );
			break;

		case AT_PARTICLE:
			if ( !a->particleValid )
			{
				return false;
			}

			if ( !a->particle->valid )
			{
				a->particleValid = false;
				return false;
			}
			else
			{
				VectorCopy( a->particle->velocity, v );
			}

			break;

		default:
			Log::Warn( "Invalid attachmentType_t in attachment" );
			break;
	}

	VectorNormalize( v );
	return true;
}

/*
===============
CG_AttachmentAxis

Return the attachment axis
===============
*/
bool CG_AttachmentAxis( attachment_t *a, vec3_t axis[ 3 ] )
{
	centity_t *cent;

	if ( !a )
	{
		return false;
	}

	switch ( a->type )
	{
		case AT_STATIC:
			return false;

		case AT_TAG:
			if ( !a->tagValid )
			{
				return false;
			}

			AxisCopy( a->re.axis, axis );
			break;

		case AT_CENT:
			if ( !a->centValid )
			{
				return false;
			}

			cent = &cg_entities[ a->centNum ];
			AnglesToAxis( cent->lerpAngles, axis );
			break;

		case AT_PARTICLE:
			return false;

		default:
			Log::Warn( "Invalid attachmentType_t in attachment" );
			break;
	}

	return true;
}

/*
===============
CG_AttachmentVelocity

If the attachment can have velocity, return it
===============
*/
bool CG_AttachmentVelocity( attachment_t *a, vec3_t v )
{
	if ( !a )
	{
		return false;
	}

	if ( a->particleValid && a->particle->valid )
	{
		VectorCopy( a->particle->velocity, v );
		return true;
	}
	else if ( a->centValid )
	{
		centity_t *cent = &cg_entities[ a->centNum ];

		VectorCopy( cent->currentState.pos.trDelta, v );
		return true;
	}

	return false;
}

/*
===============
CG_AttachmentCentNum

If the attachment has a centNum, return it
===============
*/
int CG_AttachmentCentNum( attachment_t *a )
{
	if ( !a || !a->centValid )
	{
		return -1;
	}

	return a->centNum;
}

/*
===============
CG_Attached

If the attachment is valid, return true
===============
*/
bool CG_Attached( attachment_t *a )
{
	if ( !a )
	{
		return false;
	}

	return a->attached;
}

/*
===============
CG_AttachToPoint

Attach to a point in space
===============
*/
void CG_AttachToPoint( attachment_t *a )
{
	if ( !a || !a->staticValid )
	{
		return;
	}

	a->type = AT_STATIC;
	a->attached = true;
}

/*
===============
CG_AttachToCent

Attach to a centity_t
===============
*/
void CG_AttachToCent( attachment_t *a )
{
	if ( !a || !a->centValid )
	{
		return;
	}

	a->type = AT_CENT;
	a->attached = true;
}

/*
===============
CG_AttachToTag

Attach to a model tag
===============
*/
void CG_AttachToTag( attachment_t *a )
{
	if ( !a || !a->tagValid )
	{
		return;
	}

	a->type = AT_TAG;
	a->attached = true;
}

/*
===============
CG_AttachToParticle

Attach to a particle
===============
*/
void CG_AttachToParticle( attachment_t *a )
{
	if ( !a || !a->particleValid )
	{
		return;
	}

	a->type = AT_PARTICLE;
	a->attached = true;
}

/*
===============
CG_SetAttachmentPoint
===============
*/
void CG_SetAttachmentPoint( attachment_t *a, vec3_t v )
{
	if ( !a )
	{
		return;
	}

	VectorCopy( v, a->origin );
	a->staticValid = true;
}

/*
===============
CG_SetAttachmentCent
===============
*/
void CG_SetAttachmentCent( attachment_t *a, centity_t *cent )
{
	if ( !a || !cent )
	{
		return;
	}

	a->centNum = cent->currentState.number;
	a->centValid = true;
}

/*
===============
CG_SetAttachmentTag
===============
*/
void CG_SetAttachmentTag( attachment_t *a, refEntity_t *parent,
                          qhandle_t model, const char *tagName )
{
	if ( !a )
	{
		return;
	}

	a->parent = *parent;
	a->model = model;
	Q_strncpyz( a->tagName, tagName, MAX_STRING_CHARS );
	a->tagValid = true;
}

/*
===============
CG_SetAttachmentParticle
===============
*/
void CG_SetAttachmentParticle( attachment_t *a, particle_t *p )
{
	if ( !a )
	{
		return;
	}

	a->particle = p;
	a->particleValid = true;
}

/*
===============
CG_SetAttachmentOffset
===============
*/
void CG_SetAttachmentOffset( attachment_t *a, vec3_t v )
{
	if ( !a )
	{
		return;
	}

	VectorCopy( v, a->offset );
	a->hasOffset = true;
}
