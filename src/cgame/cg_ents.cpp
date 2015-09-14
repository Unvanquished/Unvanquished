/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
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

// cg_ents.c -- present snapshot entities, happens every single frame

#include "cg_local.h"

/*
======================
CG_DrawBoxFace

Draws a bounding box face
======================
*/
static void CG_DrawBoxFace( bool solid, vec3_t a, vec3_t b, vec3_t c, vec3_t d )
{
	polyVert_t verts[ 4 ];
	vec4_t     color = { 255.0f, 0.0f, 0.0f, 128.0f };

	VectorCopy( d, verts[ 0 ].xyz );
	verts[ 0 ].st[ 0 ] = 1;
	verts[ 0 ].st[ 1 ] = 1;
	Vector4Copy( color, verts[ 0 ].modulate );

	VectorCopy( c, verts[ 1 ].xyz );
	verts[ 1 ].st[ 0 ] = 1;
	verts[ 1 ].st[ 1 ] = 0;
	Vector4Copy( color, verts[ 1 ].modulate );

	VectorCopy( b, verts[ 2 ].xyz );
	verts[ 2 ].st[ 0 ] = 0;
	verts[ 2 ].st[ 1 ] = 0;
	Vector4Copy( color, verts[ 2 ].modulate );

	VectorCopy( a, verts[ 3 ].xyz );
	verts[ 3 ].st[ 0 ] = 0;
	verts[ 3 ].st[ 1 ] = 1;
	Vector4Copy( color, verts[ 3 ].modulate );

	trap_R_AddPolyToScene( solid ? cgs.media.whiteShader : cgs.media.outlineShader, 4, verts );
}

/*
======================
CG_DrawBoundingBox

Draws a bounding box
======================
*/
void CG_DrawBoundingBox( int type, vec3_t origin, vec3_t mins, vec3_t maxs )
{
	bool solid = (type > 1);

	vec3_t ppp, mpp, mmp, pmp;
	vec3_t mmm, pmm, ppm, mpm;

	ppp[ 0 ] = origin[ 0 ] + maxs[ 0 ];
	ppp[ 1 ] = origin[ 1 ] + maxs[ 1 ];
	ppp[ 2 ] = origin[ 2 ] + maxs[ 2 ];

	mpp[ 0 ] = origin[ 0 ] + mins[ 0 ];
	mpp[ 1 ] = origin[ 1 ] + maxs[ 1 ];
	mpp[ 2 ] = origin[ 2 ] + maxs[ 2 ];

	mmp[ 0 ] = origin[ 0 ] + mins[ 0 ];
	mmp[ 1 ] = origin[ 1 ] + mins[ 1 ];
	mmp[ 2 ] = origin[ 2 ] + maxs[ 2 ];

	pmp[ 0 ] = origin[ 0 ] + maxs[ 0 ];
	pmp[ 1 ] = origin[ 1 ] + mins[ 1 ];
	pmp[ 2 ] = origin[ 2 ] + maxs[ 2 ];

	ppm[ 0 ] = origin[ 0 ] + maxs[ 0 ];
	ppm[ 1 ] = origin[ 1 ] + maxs[ 1 ];
	ppm[ 2 ] = origin[ 2 ] + mins[ 2 ];

	mpm[ 0 ] = origin[ 0 ] + mins[ 0 ];
	mpm[ 1 ] = origin[ 1 ] + maxs[ 1 ];
	mpm[ 2 ] = origin[ 2 ] + mins[ 2 ];

	mmm[ 0 ] = origin[ 0 ] + mins[ 0 ];
	mmm[ 1 ] = origin[ 1 ] + mins[ 1 ];
	mmm[ 2 ] = origin[ 2 ] + mins[ 2 ];

	pmm[ 0 ] = origin[ 0 ] + maxs[ 0 ];
	pmm[ 1 ] = origin[ 1 ] + mins[ 1 ];
	pmm[ 2 ] = origin[ 2 ] + mins[ 2 ];

	//phew!

	CG_DrawBoxFace( solid, ppp, mpp, mmp, pmp );
	CG_DrawBoxFace( solid, ppp, pmp, pmm, ppm );
	CG_DrawBoxFace( solid, mpp, ppp, ppm, mpm );
	CG_DrawBoxFace( solid, mmp, mpp, mpm, mmm );
	CG_DrawBoxFace( solid, pmp, mmp, mmm, pmm );
	CG_DrawBoxFace( solid, mmm, mpm, ppm, pmm );
}

/*
======================
CG_PositionEntityOnTag

Modifies the entities position and axis by the given
tag location
======================
*/
void CG_PositionEntityOnTag( refEntity_t *entity, const refEntity_t *parent,
                             qhandle_t parentModel, const char *tagName )
{
	int           i;
	orientation_t lerped;

	Q_UNUSED(parentModel);

	// lerp the tag
	trap_R_LerpTag( &lerped, parent, tagName, 0 );

	// FIXME: allow origin offsets along tag?
	VectorCopy( parent->origin, entity->origin );

	for ( i = 0; i < 3; i++ )
	{
		VectorMA( entity->origin, lerped.origin[ i ], parent->axis[ i ], entity->origin );
	}

	// had to cast away the const to avoid compiler problems...
	AxisMultiply( lerped.axis, ( ( refEntity_t * ) parent )->axis, entity->axis );
	entity->backlerp = parent->backlerp;
}

/*
======================
CG_PositionRotatedEntityOnTag

Modifies the entities position and axis by the given
tag location
======================
*/
void CG_PositionRotatedEntityOnTag( refEntity_t *entity, const refEntity_t *parent,
                                    qhandle_t parentModel, const char *tagName )
{
	int           i;
	orientation_t lerped;
	vec3_t        tempAxis[ 3 ];

	Q_UNUSED(parentModel);
//AxisClear( entity->axis );
	// lerp the tag
	trap_R_LerpTag( &lerped, parent, tagName, 0 );

	// FIXME: allow origin offsets along tag?
	VectorCopy( parent->origin, entity->origin );

	for ( i = 0; i < 3; i++ )
	{
		VectorMA( entity->origin, lerped.origin[ i ], parent->axis[ i ], entity->origin );
	}

	// had to cast away the const to avoid compiler problems...
	AxisMultiply( entity->axis, lerped.axis, tempAxis );
	AxisMultiply( tempAxis, ( ( refEntity_t * ) parent )->axis, entity->axis );
}

/*
=================
CG_TransformSkeleton

transform relative bones to absolute ones required for vertex skinning
=================
*/
void CG_TransformSkeleton( refSkeleton_t *skel, const vec_t scale )
{
	int       i;
	refBone_t *bone;

	skel->scale = scale;

	switch ( skel->type )
	{
		case SK_INVALID:
		case SK_ABSOLUTE:
			return;

		default:
			break;
	}

	// calculate absolute transforms
	for ( i = 0, bone = &skel->bones[ 0 ]; i < skel->numBones; i++, bone++ )
	{
		if ( bone->parentIndex >= 0 )
		{
			refBone_t *parent;

			parent = &skel->bones[ bone->parentIndex ];

			TransCombine( &bone->t, &parent->t, &bone->t );
		}
	}

	skel->type = SK_ABSOLUTE;
}

/*
==========================================================================

FUNCTIONS CALLED EACH FRAME

==========================================================================
*/

/*
======================
CG_SetEntitySoundPosition

Also called by event processing code
======================
*/
void CG_SetEntitySoundPosition( centity_t *cent )
{
	if ( cent->currentState.solid == SOLID_BMODEL )
	{
		vec3_t origin;
		float  *v;

		v = cgs.inlineModelMidpoints[ cent->currentState.modelindex ];
		VectorAdd( cent->lerpOrigin, v, origin );
		trap_S_UpdateEntityPosition( cent->currentState.number, origin );
	}
	else
	{
		trap_S_UpdateEntityPosition( cent->currentState.number, cent->lerpOrigin );
		trap_S_UpdateEntityVelocity( cent->currentState.number, cent->currentState.pos.trDelta );
	}
}

/*
==================
CG_EntityEffects

Add continuous entity effects, like local entity emission and lighting
==================
*/
static void CG_EntityEffects( centity_t *cent )
{
	// update sound origins
	CG_SetEntitySoundPosition( cent );

	// add loop sound
	if ( cent->currentState.loopSound )
	{
		if ( cent->currentState.eType != ET_SPEAKER )
		{
			trap_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin,
			                        cgs.gameSounds[ cent->currentState.loopSound ] );
		}
		else
		{
			trap_S_AddRealLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin,
			                            cgs.gameSounds[ cent->currentState.loopSound ] );
		}
	}

	// constant light glow
	if ( cent->currentState.constantLight && ( cent->currentState.eType == ET_MOVER || cent->currentState.eType == ET_MODELDOOR ) )
	{
		int cl;
		int i, r, g, b;

		cl = cent->currentState.constantLight;
		r = cl & 255;
		g = ( cl >> 8 ) & 255;
		b = ( cl >> 16 ) & 255;
		i = ( ( cl >> 24 ) & 255 ) * 4;

		trap_R_AddAdditiveLightToScene( cent->lerpOrigin, i, r, g, b );
	}

	if ( CG_IsTrailSystemValid( &cent->muzzleTS ) )
	{
		if ( cg.time > cent->muzzleTSDeathTime && CG_IsTrailSystemValid( &cent->muzzleTS ) )
		{
			CG_DestroyTrailSystem( &cent->muzzleTS );
		}
	}
}

/*
==================
CG_General
==================
*/
static void CG_General( centity_t *cent )
{
	static refEntity_t ent; // static for proper alignment in QVMs
	entityState_t *s1;

	s1 = &cent->currentState;

	// if set to invisible, skip
	if ( !s1->modelindex )
	{
		return;
	}

	memset( &ent, 0, sizeof( ent ) );

	// set frame

	ent.frame = s1->frame;
	ent.oldframe = ent.frame;
	ent.backlerp = 0;

	VectorCopy( cent->lerpOrigin, ent.origin );
	VectorCopy( cent->lerpOrigin, ent.oldorigin );

	ent.hModel = cgs.gameModels[ s1->modelindex ];

	// player model
	if ( s1->number == cg.snap->ps.clientNum )
	{
		ent.renderfx |= RF_THIRD_PERSON; // only draw from mirrors
	}

	// convert angles to axis
	AnglesToAxis( cent->lerpAngles, ent.axis );

	// add to refresh list
	trap_R_AddRefEntityToScene( &ent );
}

/*
==================
CG_Speaker

Speaker entities can automatically play sounds
==================
*/
static void CG_Speaker( centity_t *cent )
{
	if ( !cent->currentState.clientNum )
	{
		// FIXME: use something other than clientNum...
		return; // not auto triggering
	}

	if ( cg.time < cent->miscTime )
	{
		return;
	}

	trap_S_StartSound( nullptr, cent->currentState.number, CHAN_ITEM, cgs.gameSounds[ cent->currentState.eventParm ] );

	//  ent->s.frame = ent->wait * 10;
	//  ent->s.clientNum = ent->random * 10;
	cent->miscTime = cg.time + cent->currentState.frame * 100 + cent->currentState.clientNum * 100 * crandom();
}

//============================================================================

/*
===============
CG_Missile
===============
*/
static void CG_Missile( centity_t *cent )
{
	static refEntity_t        ent; // static for proper alignment in QVMs
	entityState_t             *es;
	const missileAttributes_t *ma;

	es = &cent->currentState;
	ma = BG_Missile( es->weapon );

	// calculate the axis
	VectorCopy( es->angles, cent->lerpAngles );

	// add dynamic light
	if ( ma->usesDlight )
	{
		trap_R_AddLightToScene( cent->lerpOrigin, ma->dlight, ma->dlightIntensity,
		                        ma->dlightColor[ 0 ], ma->dlightColor[ 1 ], ma->dlightColor[ 2 ], 0, 0 );
	}

	// add missile sound
	if ( ma->sound )
	{
		vec3_t velocity;

		BG_EvaluateTrajectoryDelta( &cent->currentState.pos, cg.time, velocity );

		trap_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, velocity, ma->sound );
	}

	// create the render entity
	memset( &ent, 0, sizeof( ent ) );
	VectorCopy( cent->lerpOrigin, ent.origin );
	VectorCopy( cent->lerpOrigin, ent.oldorigin );

	if ( ma->usesSprite )
	{
		ent.reType = RT_SPRITE;
		ent.radius = ma->spriteSize + ma->spriteCharge * es->torsoAnim;
		ent.rotation = 0;
		ent.customShader = ma->sprite;
		ent.shaderRGBA[ 0 ] = 0xFF;
		ent.shaderRGBA[ 1 ] = 0xFF;
		ent.shaderRGBA[ 2 ] = 0xFF;
		ent.shaderRGBA[ 3 ] = 0xFF;
	}
	else if ( ma->model )
	{
		vec3_t velocity;

		ent.hModel = ma->model;
		ent.renderfx = ma->renderfx | RF_NOSHADOW;

		if( es->weapon == MIS_GRENADE || es->weapon == MIS_FIREBOMB )
		{
			VectorCopy( es->pos.trDelta, velocity );
		}
		else
		{
			BG_EvaluateTrajectoryDelta( &es->pos, cg.time, velocity );
		}

		// convert direction of travel into axis
		if ( VectorNormalize2( velocity, ent.axis[ 0 ] ) == 0 )
		{
			ent.axis[ 0 ][ 2 ] = 1.0f;
		}

		MakeNormalVectors( ent.axis[ 0 ], ent.axis[ 1 ], ent.axis[ 2 ] );

		// apply rotation from config
		{
			matrix_t axisMat;
			quat_t   axisQuat, rotQuat;

			QuatFromAngles( rotQuat, ma->modelRotation[ PITCH ], ma->modelRotation[ YAW ],
			                         ma->modelRotation[ ROLL ] );

			// FRU because that's what MakeNormalVectors produces (?)
			MatrixFromVectorsFRU( axisMat, ent.axis[ 0 ], ent.axis[ 1 ], ent.axis[ 2 ] );
			QuatFromMatrix( axisQuat, axisMat );
			QuatMultiply0( axisQuat, rotQuat );
			MatrixFromQuat( axisMat, axisQuat );
			MatrixToVectorsFRU( axisMat, ent.axis[ 0 ], ent.axis[ 1 ], ent.axis[ 2 ] );
		}

		// spin as it moves
		if ( es->pos.trType != TR_STATIONARY && ma->rotates )
		{
			RotateAroundDirection( ent.axis, cg.time / 4 );
		}

		// apply scale from config
		if ( ma->modelScale != 1.0f )
		{
			VectorScale( ent.axis[ 0 ], ma->modelScale, ent.axis[ 0 ] );
			VectorScale( ent.axis[ 1 ], ma->modelScale, ent.axis[ 1 ] );
			VectorScale( ent.axis[ 2 ], ma->modelScale, ent.axis[ 2 ] );

			ent.nonNormalizedAxes = true;
		}
		else
		{
			ent.nonNormalizedAxes = false;
		}

		if ( ma->usesAnim )
		{
			int timeSinceStart = cg.time - es->time;

			if ( ma->animLooping )
			{
				ent.frame = ma->animStartFrame +
				            ( int )( ( timeSinceStart / 1000.0f ) * ma->animFrameRate ) % ma->animNumFrames;
			}
			else
			{
				ent.frame = ma->animStartFrame +
				            ( int )( ( timeSinceStart / 1000.0f ) * ma->animFrameRate );

				if ( ent.frame > ( ma->animStartFrame + ma->animNumFrames ) )
				{
					ent.frame = ma->animStartFrame +ma->animNumFrames;
				}
			}
		}

		ent.skeleton.scale = ma->modelScale;
	}

	// Add particle system.
	if ( ma->particleSystem && !CG_IsParticleSystemValid( &cent->missilePS ) )
	{
		cent->missilePS = CG_SpawnNewParticleSystem( ma->particleSystem );

		if ( CG_IsParticleSystemValid( &cent->missilePS ) )
		{
			CG_SetAttachmentCent( &cent->missilePS->attachment, cent );
			CG_AttachToCent( &cent->missilePS->attachment );
			cent->missilePS->charge = es->torsoAnim;
		}
	}

	// Add trail system.
	if ( ma->trailSystem && !CG_IsTrailSystemValid( &cent->missileTS ) )
	{
		cent->missileTS = CG_SpawnNewTrailSystem( ma->trailSystem );

		if ( CG_IsTrailSystemValid( &cent->missileTS ) )
		{
			// TODO: Make attachment to tags on missile models work.
			/*const char *tag;
			bool       attachToTag = true;

			// TODO: Move attachment name to config files.
			switch ( es->modelindex )
			{
				case MIS_ROCKET:
					tag = "Bone_nozzle";
					break;

				default:
					attachToTag = false;
					break;
			}

			if ( attachToTag )
			{
				CG_SetAttachmentTag( &cent->missileTS->frontAttachment, &ent, ent.hModel, tag );
				CG_SetAttachmentCent( &cent->missileTS->frontAttachment, cent );
				CG_AttachToTag( &cent->missileTS->frontAttachment );
			}
			else
			{*/
				CG_SetAttachmentCent( &cent->missileTS->frontAttachment, cent );
				CG_AttachToCent( &cent->missileTS->frontAttachment );
			//}
		}
	}

	// Only refresh if there is something to display.
	if ( ma->sprite || ma->model )
	{
		trap_R_AddRefEntityToScene( &ent );
	}
}

/*
===============
CG_Mover
===============
*/
static void CG_Mover( centity_t *cent )
{
	static refEntity_t ent; // static for proper alignment in QVMs
	entityState_t *s1;

	s1 = &cent->currentState;

	// create the render entity
	memset( &ent, 0, sizeof( ent ) );
	VectorCopy( cent->lerpOrigin, ent.origin );
	VectorCopy( cent->lerpOrigin, ent.oldorigin );
	AnglesToAxis( cent->lerpAngles, ent.axis );

	ent.renderfx = RF_NOSHADOW;

	// flicker between two skins (FIXME?)
	ent.skinNum = ( cg.time >> 6 ) & 1;

	// get the model, either as a bmodel or a modelindex
	if ( s1->solid == SOLID_BMODEL )
	{
		ent.hModel = cgs.inlineDrawModel[ s1->modelindex ];
	}
	else
	{
		ent.hModel = cgs.gameModels[ s1->modelindex ];
	}

	// add to refresh list
	trap_R_AddRefEntityToScene( &ent );

	// add the secondary model
	if ( s1->modelindex2 )
	{
		ent.skinNum = 0;
		ent.hModel = cgs.gameModels[ s1->modelindex2 ];
		trap_R_AddRefEntityToScene( &ent );
	}
}

/*
===============
CG_Portal
===============
*/
static void CG_Portal( centity_t *cent )
{
	static refEntity_t ent; // static for proper alignment in QVMs
	entityState_t *s1;

	s1 = &cent->currentState;

	// create the render entity
	memset( &ent, 0, sizeof( ent ) );
	VectorCopy( cent->lerpOrigin, ent.origin );
	VectorCopy( s1->origin2, ent.oldorigin );
	ByteToDir( s1->eventParm, ent.axis[ 0 ] );
	PerpendicularVector( ent.axis[ 1 ], ent.axis[ 0 ] );

	// negating this tends to get the directions like they want
	// we really should have a camera roll value
	VectorSubtract( vec3_origin, ent.axis[ 1 ], ent.axis[ 1 ] );

	CrossProduct( ent.axis[ 0 ], ent.axis[ 1 ], ent.axis[ 2 ] );
	ent.reType = RT_PORTALSURFACE;
	ent.oldframe = s1->misc;
	ent.frame = s1->frame; // rotation speed
	ent.skinNum = s1->clientNum / 256.0 * 360; // roll offset

	// add to refresh list
	trap_R_AddRefEntityToScene( &ent );
}

//============================================================================

/*
===============
CG_Fire
===============
*/
void CG_Fire( centity_t *cent )
{
	// TODO: Add burning sound
	// trap_S_StartSound( origin, ENTITYNUM_WORLD, CHAN_AUTO, cgs.media. );

	if ( !CG_IsParticleSystemValid( &cent->entityPS ) )
	{
		// TODO: Use different particle systems for different normals
		cent->entityPS = CG_SpawnNewParticleSystem( cgs.media.floorFirePS );

		if ( CG_IsParticleSystemValid( &cent->entityPS ) )
		{
			CG_SetParticleSystemNormal( cent->entityPS, cent->currentState.origin2 );
			CG_SetAttachmentPoint( &cent->entityPS->attachment, cent->currentState.origin );
			CG_AttachToPoint( &cent->entityPS->attachment );
		}
	}
}

#define SETBOUNDS(v1,v2,r) (( v1 )[ 0 ] = ( -r / 2 ),( v1 )[ 1 ] = ( -r / 2 ),( v1 )[ 2 ] = ( -r / 2 ), \
                            ( v2 )[ 0 ] = ( r / 2 ),( v2 )[ 1 ] = ( r / 2 ),( v2 )[ 2 ] = ( r / 2 ))
#define RADIUSSTEP     0.5f

#define FLARE_OFF      0
#define FLARE_NOFADE   1
#define FLARE_TIMEFADE 2
#define FLARE_REALFADE 3

/*
=========================
CG_LightFlare
=========================
*/
static void CG_LightFlare( centity_t *cent )
{
	static refEntity_t flare; // static for proper alignment in QVMs
	entityState_t *es;
	vec3_t        forward, delta;
	float         len;
	float         maxAngle;
	vec3_t        start, end;
	float         ratio = 1.0f;
	float         newStatus;

	es = &cent->currentState;

	//don't draw light flares
	if ( cg_lightFlare.integer == FLARE_OFF )
	{
		return;
	}

	//flare is "off"
	if ( es->eFlags & EF_NODRAW )
	{
		return;
	}

	newStatus = trap_CheckVisibility( cent->lfs.hTest );

	trap_AddVisTestToScene( cent->lfs.hTest, es->origin, 16.0f, 8.0f );

	memset( &flare, 0, sizeof( flare ) );

	flare.reType = RT_SPRITE;
	flare.customShader = cgs.gameShaders[ es->modelindex ];
	flare.shaderRGBA[ 0 ] = 0xFF;
	flare.shaderRGBA[ 1 ] = 0xFF;
	flare.shaderRGBA[ 2 ] = 0xFF;
	flare.shaderRGBA[ 3 ] = 0xFF;

	//flares always drawn before the rest of the scene
	flare.renderfx |= RF_DEPTHHACK;

	//bunch of geometry
	AngleVectors( es->angles, forward, nullptr, nullptr );
	VectorCopy( cent->lerpOrigin, flare.origin );
	VectorSubtract( flare.origin, cg.refdef.vieworg, delta );
	len = VectorLength( delta );
	VectorNormalize( delta );

	//flare is too close to camera to be drawn
	if ( len < es->generic1 )
	{
		return;
	}

	//don't bother for flares behind the view plane
	if ( DotProduct( delta, cg.refdef.viewaxis[ 0 ] ) < 0.0 )
	{
		return;
	}

	//can only see the flare when in front of it
	flare.radius = len / es->origin2[ 0 ];

	maxAngle = es->origin2[ 1 ];

	if ( maxAngle > 0.0f )
	{
		float radiusMod = 1.0f - ( 180.0f - RAD2DEG(
						   acos( DotProduct( delta, forward ) ) ) ) / maxAngle;

		if ( radiusMod < 0.0f )
		{
			radiusMod = 0.0f;
		}

		flare.radius *= radiusMod;
	}

	if ( flare.radius < 0.0f )
	{
		flare.radius = 0.0f;
	}

	VectorMA( flare.origin, -flare.radius, delta, end );
	VectorMA( cg.refdef.vieworg, flare.radius, delta, start );

	if ( cg_lightFlare.integer == FLARE_REALFADE )
	{
		ratio = newStatus;
	}
	else if ( cg_lightFlare.integer == FLARE_TIMEFADE )
	{
		//draw timed flares
		if ( newStatus <= 0.5f && cent->lfs.status )
		{
			cent->lfs.status = false;
			cent->lfs.lastTime = cg.time;
		}
		else if ( newStatus > 0.5f && !cent->lfs.status )
		{
			cent->lfs.status = true;
			cent->lfs.lastTime = cg.time;
		}

		//fade flare up
		if ( cent->lfs.status )
		{
			if ( cent->lfs.lastTime + es->time > cg.time )
			{
				ratio = ( float )( cg.time - cent->lfs.lastTime ) / es->time;
			}
		}

		//fade flare down
		if ( !cent->lfs.status )
		{
			if ( cent->lfs.lastTime + es->time > cg.time )
			{
				ratio = ( float )( cg.time - cent->lfs.lastTime ) / es->time;
				ratio = 1.0f - ratio;
			}
			else
			{
				ratio = 0.0f;
			}
		}
	}
	else if ( cg_lightFlare.integer == FLARE_NOFADE )
	{
		//flare source occluded
		if ( newStatus <= 0.5f )
		{
			ratio = 0.0f;
		}
	}

	cent->lfs.lastRatio = ratio;
	cent->lfs.lastRadius = flare.radius;

	if ( ratio < 1.0f )
	{
		flare.radius *= ratio;
		flare.shaderRGBA[ 3 ] = ( byte )( ( float ) flare.shaderRGBA[ 3 ] * ratio );
	}

	if ( flare.radius <= 0.0f )
	{
		return;
	}

	trap_R_AddRefEntityToScene( &flare );
}

/*
=========================
CG_Lev2ZapChain
=========================
*/
static void CG_Lev2ZapChain( centity_t *cent )
{
	int           i;
	entityState_t *es;
	centity_t     *source = nullptr, *target = nullptr;
	int           entityNums[ LEVEL2_AREAZAP_MAX_TARGETS + 1 ];
	int           count;

	es = &cent->currentState;

	count = BG_UnpackEntityNumbers( es, entityNums, LEVEL2_AREAZAP_MAX_TARGETS + 1 );

	for ( i = 1; i < count; i++ )
	{
		if ( i == 1 )
		{
			// First entity is the attacker
			source = &cg_entities[ entityNums[ 0 ] ];
		}
		else
		{
			// Subsequent zaps come from the first target
			source = &cg_entities[ entityNums[ 1 ] ];
		}

		target = &cg_entities[ entityNums[ i ] ];

		if ( !CG_IsTrailSystemValid( &cent->level2ZapTS[ i ] ) )
		{
			cent->level2ZapTS[ i ] = CG_SpawnNewTrailSystem( cgs.media.level2ZapTS );
		}

		if ( CG_IsTrailSystemValid( &cent->level2ZapTS[ i ] ) )
		{
			CG_SetAttachmentCent( &cent->level2ZapTS[ i ]->frontAttachment, source );
			CG_SetAttachmentCent( &cent->level2ZapTS[ i ]->backAttachment, target );
			CG_AttachToCent( &cent->level2ZapTS[ i ]->frontAttachment );
			CG_AttachToCent( &cent->level2ZapTS[ i ]->backAttachment );
		}
	}
}

/*
=========================
CG_AdjustPositionForMover

Also called by client movement prediction code
=========================
*/
void CG_AdjustPositionForMover( const vec3_t in, int moverNum, int fromTime, int toTime, vec3_t out, vec3_t angles_in, vec3_t angles_out )
{
	centity_t *cent;
	vec3_t    oldOrigin, origin, deltaOrigin;
	vec3_t    oldAngles, angles, deltaAngles;

	if ( moverNum <= 0 || moverNum >= ENTITYNUM_MAX_NORMAL )
	{
		VectorCopy( in, out );
		VectorCopy( angles_in, angles_out );
		return;
	}

	cent = &cg_entities[ moverNum ];

	if ( cent->currentState.eType != ET_MOVER )
	{
		VectorCopy( in, out );
		VectorCopy( angles_in, angles_out );
		return;
	}

	BG_EvaluateTrajectory( &cent->currentState.pos, fromTime, oldOrigin );
	BG_EvaluateTrajectory( &cent->currentState.apos, fromTime, oldAngles );

	BG_EvaluateTrajectory( &cent->currentState.pos, toTime, origin );
	BG_EvaluateTrajectory( &cent->currentState.apos, toTime, angles );

	VectorSubtract( origin, oldOrigin, deltaOrigin );
	VectorSubtract( angles, oldAngles, deltaAngles );

	VectorAdd( in, deltaOrigin, out );
	VectorAdd( angles_in, deltaAngles, angles_out );

	// FIXME: origin change when on a rotating object
}

/*
=============================
CG_InterpolateEntityPosition
=============================
*/
static void CG_InterpolateEntityPosition( centity_t *cent )
{
	vec3_t current, next;
	float  f;

	// it would be an internal error to find an entity that interpolates without
	// a snapshot ahead of the current one
	if ( cg.nextSnap == nullptr )
	{
		CG_Error( "CG_InterpoateEntityPosition: cg.nextSnap == NULL" );
	}

	f = cg.frameInterpolation;

	// this will linearize a sine or parabolic curve, but it is important
	// to not extrapolate player positions if more recent data is available
	BG_EvaluateTrajectory( &cent->currentState.pos, cg.snap->serverTime, current );
	BG_EvaluateTrajectory( &cent->nextState.pos, cg.nextSnap->serverTime, next );

	cent->lerpOrigin[ 0 ] = current[ 0 ] + f * ( next[ 0 ] - current[ 0 ] );
	cent->lerpOrigin[ 1 ] = current[ 1 ] + f * ( next[ 1 ] - current[ 1 ] );
	cent->lerpOrigin[ 2 ] = current[ 2 ] + f * ( next[ 2 ] - current[ 2 ] );

	BG_EvaluateTrajectory( &cent->currentState.apos, cg.snap->serverTime, current );
	BG_EvaluateTrajectory( &cent->nextState.apos, cg.nextSnap->serverTime, next );

	cent->lerpAngles[ 0 ] = LerpAngle( current[ 0 ], next[ 0 ], f );
	cent->lerpAngles[ 1 ] = LerpAngle( current[ 1 ], next[ 1 ], f );
	cent->lerpAngles[ 2 ] = LerpAngle( current[ 2 ], next[ 2 ], f );
}

/*
===============
CG_CalcEntityLerpPositions

===============
*/
static void CG_CalcEntityLerpPositions( centity_t *cent )
{
	// this will be set to how far forward projectiles will be extrapolated
	int timeshift = 0;

	// if this player does not want to see extrapolated players
	if ( !cg_smoothClients.integer )
	{
		// make sure the clients use TR_INTERPOLATE
		if ( cent->currentState.number < MAX_CLIENTS )
		{
			cent->currentState.pos.trType = TR_INTERPOLATE;
			cent->nextState.pos.trType = TR_INTERPOLATE;
		}
	}

	if ( cent->interpolate && cent->currentState.pos.trType == TR_INTERPOLATE )
	{
		CG_InterpolateEntityPosition( cent );
		return;
	}

	// first see if we can interpolate between two snaps for
	// linear extrapolated clients
	if ( cent->interpolate && cent->currentState.pos.trType == TR_LINEAR_STOP &&
	     cent->currentState.number < MAX_CLIENTS )
	{
		CG_InterpolateEntityPosition( cent );
		return;
	}

	if ( cg_projectileNudge.integer &&
	     !cg.demoPlayback &&
	     cent->currentState.eType == ET_MISSILE &&
	     !( cg.snap->ps.pm_flags & PMF_FOLLOW ) )
	{
		timeshift = cg.ping;
	}

	// just use the current frame and evaluate as best we can
	BG_EvaluateTrajectory( &cent->currentState.pos,
	                       ( cg.time + timeshift ), cent->lerpOrigin );
	BG_EvaluateTrajectory( &cent->currentState.apos,
	                       ( cg.time + timeshift ), cent->lerpAngles );

	if ( timeshift )
	{
		trace_t tr;
		vec3_t  lastOrigin;

		BG_EvaluateTrajectory( &cent->currentState.pos, cg.time, lastOrigin );

		CG_Trace( &tr, lastOrigin, vec3_origin, vec3_origin, cent->lerpOrigin,
		          cent->currentState.number, MASK_SHOT, 0 );

		// don't let the projectile go through the floor
		if ( tr.fraction < 1.0f )
		{
			VectorLerpTrem( tr.fraction, lastOrigin, cent->lerpOrigin, cent->lerpOrigin );
		}
	}

	// adjust for riding a mover if it wasn't rolled into the predicted
	// player state
	if ( cent != &cg.predictedPlayerEntity )
	{
		CG_AdjustPositionForMover( cent->lerpOrigin, cent->currentState.groundEntityNum,
		                           cg.snap->serverTime, cg.time, cent->lerpOrigin, cent->lerpAngles, cent->lerpAngles );
	}
}

/*
===============
CG_CEntityPVSEnter

===============
*/
static void CG_CEntityPVSEnter( centity_t *cent )
{
	entityState_t *es = &cent->currentState;

	if ( cg_debugPVS.integer )
	{
		CG_Printf( "Entity %d entered PVS\n", cent->currentState.number );
	}

	switch ( es->eType )
	{
		case ET_BUILDABLE:
			cent->lastBuildableHealth = es->generic1;
			break;

		case ET_LIGHTFLARE:
			cent->lfs.hTest = trap_RegisterVisTest();
			break;

		default:
			break;
	}

	//clear any particle systems from previous uses of this centity_t
	cent->muzzlePS = nullptr;
	cent->muzzlePsTrigger = false;
	cent->jetPackPS[ 0 ] = nullptr;
	cent->jetPackPS[ 1 ] = nullptr;
	cent->jetPackState = JPS_INACTIVE;
	cent->buildablePS = nullptr;
	cent->buildableStatusPS = nullptr;
	cent->entityPS = nullptr;
	cent->entityPSMissing = false;
	cent->missilePS = nullptr;
	cent->missileTS = nullptr;

	//make sure that the buildable animations are in a consistent state
	//when a buildable enters the PVS
	cent->buildableAnim = BANIM_NONE;
	cent->lerpFrame.animationNumber = BANIM_NONE;
	cent->oldBuildableAnim = (buildableAnimNumber_t) es->legsAnim;
	cent->radarVisibility = 0.0f;

	cent->pvsEnterTime = cg.time;
}

/**
 * @brief Callend when a client entity leaves the potentially visible set.
 *        Performs cleanup operations, e.g. destroying attached particle effects.
 * @param cent
 */
static void CG_CEntityPVSLeave( centity_t *cent )
{
	int           i;
	entityState_t *es = &cent->currentState;

	if ( cg_debugPVS.integer )
	{
		CG_Printf( "Entity %d left PVS\n", cent->currentState.number );
	}

	switch ( es->eType )
	{
		case ET_LEV2_ZAP_CHAIN:
			for ( i = 0; i <= LEVEL2_AREAZAP_MAX_TARGETS; i++ )
			{
				if ( CG_IsTrailSystemValid( &cent->level2ZapTS[ i ] ) )
				{
					CG_DestroyTrailSystem( &cent->level2ZapTS[ i ] );
				}
			}
			break;

		case ET_LIGHTFLARE:
			trap_UnregisterVisTest( cent->lfs.hTest );
			cent->lfs.hTest = 0;
			break;

		case ET_FIRE:
			if ( CG_IsParticleSystemValid( &cent->entityPS ) )
			{
				CG_DestroyParticleSystem( &cent->entityPS );
			}
			break;

		default:
			break;
	}

	// some muzzle PS are infinite, destroy them here
	if ( CG_IsParticleSystemValid( &cent->muzzlePS ) )
	{
		CG_DestroyParticleSystem( &cent->muzzlePS );
	}

	// destroy the jetpack PS
	if ( CG_IsParticleSystemValid( &cent->jetPackPS[ 0 ] ) )
	{
	     CG_DestroyParticleSystem( &cent->jetPackPS[ 0 ] );
	}

	if ( CG_IsParticleSystemValid( &cent->jetPackPS[ 1 ] ) )
	{
		CG_DestroyParticleSystem( &cent->jetPackPS[ 1 ] );
	}

	// Destroy missile PS.
	if ( CG_IsParticleSystemValid( &cent->missilePS ) )
	{
		CG_DestroyParticleSystem( &cent->missilePS );
	}

	// Destroy missile TS.
	if ( CG_IsTrailSystemValid( &cent->missileTS ) )
	{
		CG_DestroyTrailSystem( &cent->missileTS );
	}

	// Lazy TODO: Destroy more PS/TS here
	// Better TODO: Make two groups cent->temporaryPS[NUM_TMPPS], cent->persistentPS[NUM_PERSPS]
	//              and use an enumerator each for different types of mutually non-exclusive PS.

	// NOTE: Don't destroy cent->entityPS here since it is supposed to survive for certain
	//       entity types (e.g. particle entities)
}

/*
===============
CG_AddCEntity

===============
*/
static void CG_AddCEntity( centity_t *cent )
{
	// event-only entities will have been dealt with already
	if ( cent->currentState.eType >= ET_EVENTS )
	{
		return;
	}

	// calculate the current origin
	CG_CalcEntityLerpPositions( cent );

	// add automatic effects
	CG_EntityEffects( cent );

	switch ( cent->currentState.eType )
	{
		default:
			CG_Error( "Bad entity type: %i", cent->currentState.eType );

		case ET_INVISIBLE:
		case ET_PUSHER:
		case ET_TELEPORTER:
		case ET_LOCATION:
		case ET_BEACON:
			break;

		case ET_GENERAL:
			CG_General( cent );
			break;

		case ET_CORPSE:
			CG_Corpse( cent );
			break;

		case ET_PLAYER:
			CG_Player( cent );
			break;

		case ET_BUILDABLE:
			CG_Buildable( cent );
			break;

		case ET_MISSILE:
			CG_Missile( cent );
			break;

		case ET_MOVER:
			CG_Mover( cent );
			break;

		case ET_PORTAL:
			CG_Portal( cent );
			break;

		case ET_SPEAKER:
			CG_Speaker( cent );
			break;

		case ET_FIRE:
			CG_Fire( cent );
			break;

		case ET_PARTICLE_SYSTEM:
			CG_ParticleSystemEntity( cent );
			break;

		case ET_ANIMMAPOBJ:
			CG_AnimMapObj( cent );
			break;

		case ET_MODELDOOR:
			CG_ModelDoor( cent );
			break;

		case ET_LIGHTFLARE:
			CG_LightFlare( cent );
			break;

		case ET_LEV2_ZAP_CHAIN:
			CG_Lev2ZapChain( cent );
			break;
	}
}

/*
===============
CG_AddPacketEntities

===============
*/
void CG_AddPacketEntities()
{
	centity_t     *cent;
	playerState_t *ps;

	// set cg.frameInterpolation
	if ( cg.nextSnap )
	{
		int delta;

		delta = ( cg.nextSnap->serverTime - cg.snap->serverTime );

		if ( delta == 0 )
		{
			cg.frameInterpolation = 0;
		}
		else
		{
			cg.frameInterpolation = ( float )( cg.time - cg.snap->serverTime ) / delta;
		}
	}
	else
	{
		cg.frameInterpolation = 0; // actually, it should never be used, because
		// no entities should be marked as interpolating
	}

	// the auto-rotating items will all have the same axis
	cg.autoAngles[ 0 ] = 0;
	cg.autoAngles[ 1 ] = ( cg.time & 2047 ) * 360 / 2048.0;
	cg.autoAngles[ 2 ] = 0;

	cg.autoAnglesFast[ 0 ] = 0;
	cg.autoAnglesFast[ 1 ] = ( cg.time & 1023 ) * 360 / 1024.0f;
	cg.autoAnglesFast[ 2 ] = 0;

	AnglesToAxis( cg.autoAngles, cg.autoAxis );
	AnglesToAxis( cg.autoAnglesFast, cg.autoAxisFast );

	// generate and add the entity from the playerstate
	ps = &cg.predictedPlayerState;
	BG_PlayerStateToEntityState( ps, &cg.predictedPlayerEntity.currentState, false );
	cg.predictedPlayerEntity.valid = true;
	CG_AddCEntity( &cg.predictedPlayerEntity );

	// lerp the non-predicted value for lightning gun origins
	CG_CalcEntityLerpPositions( &cg_entities[ cg.snap->ps.clientNum ] );

	for ( unsigned num = 0; num < MAX_GENTITIES; num++ )
	{
		cg_entities[ num ].valid = false;
	}

	// add each entity sent over by the server
	for ( unsigned num = 0; num < cg.snap->entities.size(); num++ )
	{
		cent = &cg_entities[ cg.snap->entities[ num ].number ];
		cent->valid = true;
	}

	for ( unsigned num = 0; num < MAX_GENTITIES; num++ )
	{
		cent = &cg_entities[ num ];

		if ( cent->valid && !cent->oldValid )
		{
			CG_CEntityPVSEnter( cent );
		}
		else if ( !cent->valid && cent->oldValid )
		{
			CG_CEntityPVSLeave( cent );
		}

		cent->oldValid = cent->valid;
	}

	// add each entity sent over by the server
	for ( unsigned num = 0; num < cg.snap->entities.size(); num++ )
	{
		cent = &cg_entities[ cg.snap->entities[ num ].number ];
		CG_AddCEntity( cent );
	}

	//make an attempt at drawing bounding boxes of selected entity types
	if ( cg_drawBBOX.integer )
	{
		for ( unsigned num = 0; num < cg.snap->entities.size(); num++ )
		{
			float         x, zd, zu;
			vec3_t        mins, maxs;
			entityState_t *es;

			cent = &cg_entities[ cg.snap->entities[ num ].number ];
			es = &cent->currentState;

			switch ( es->eType )
			{
				case ET_MISSILE:
				case ET_CORPSE:
					x = ( es->solid & 255 );
					zd = ( ( es->solid >> 8 ) & 255 );
					zu = ( ( es->solid >> 16 ) & 255 ) - 32;

					mins[ 0 ] = mins[ 1 ] = -x;
					maxs[ 0 ] = maxs[ 1 ] = x;
					mins[ 2 ] = -zd;
					maxs[ 2 ] = zu;

					CG_DrawBoundingBox( cg_drawBBOX.integer, cent->lerpOrigin, mins, maxs );
					break;

				default:
					break;
			}
		}
	}
}
