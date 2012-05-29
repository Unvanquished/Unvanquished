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
static void CG_DrawBoxFace( vec3_t a, vec3_t b, vec3_t c, vec3_t d )
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

	trap_R_AddPolyToScene( cgs.media.outlineShader, 4, verts );
}

/*
======================
CG_DrawBoundingBox

Draws a bounding box
======================
*/
void CG_DrawBoundingBox( vec3_t origin, vec3_t mins, vec3_t maxs )
{
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

	CG_DrawBoxFace( ppp, mpp, mmp, pmp );
	CG_DrawBoxFace( ppp, pmp, pmm, ppm );
	CG_DrawBoxFace( mpp, ppp, ppm, mpm );
	CG_DrawBoxFace( mmp, mpp, mpm, mmm );
	CG_DrawBoxFace( pmp, mmp, mmm, pmm );
	CG_DrawBoxFace( mmm, mpm, ppm, pmm );
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
void CG_TransformSkeleton( refSkeleton_t *skel, const vec3_t scale )
{
	int       i;
	int       j;
	refBone_t *bone;

	switch ( skel->type )
	{
		case SK_INVALID:
		case SK_ABSOLUTE:
			return;

		default:
			break;
	}

	skel->bounds[0][0] = skel->bounds[0][1] = skel->bounds[0][2] =
	skel->bounds[1][0] = skel->bounds[1][1] = skel->bounds[1][2] = 0;

	// calculate absolute transforms
	for ( i = 0, bone = &skel->bones[ 0 ]; i < skel->numBones; i++, bone++ )
	{
		if ( bone->parentIndex >= 0 )
		{
			vec3_t    rotated;
			quat_t    quat;

			refBone_t *parent;

			parent = &skel->bones[ bone->parentIndex ];

			QuatTransformVector( parent->rotation, bone->origin, rotated );

			if ( scale )
			{
				rotated[ 0 ] *= scale[ 0 ];
				rotated[ 1 ] *= scale[ 1 ];
				rotated[ 2 ] *= scale[ 2 ];
			}

			VectorAdd( parent->origin, rotated, bone->origin );

			// use the origin to adjust the bbox, but do we also need to use the other end of the bone?
			for ( j = 0; j < 3; ++j )
			{
				if ( bone->origin[j] < skel->bounds[0][j] ) { skel->bounds[0][j] = bone->origin[j]; }
				if ( bone->origin[j] > skel->bounds[1][j] ) { skel->bounds[1][j] = bone->origin[j]; }
			}

			QuatMultiply1( parent->rotation, bone->rotation, quat );
			QuatCopy( quat, bone->rotation );
		}
	}

	skel->type = SK_ABSOLUTE;

	if ( scale )
	{
		VectorCopy( scale, skel->scale );
	}
	else
	{
		VectorSet( skel->scale, 1, 1, 1 );
	}
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
	if ( cent->currentState.constantLight )
	{
		int cl;
		int i, r, g, b;

		cl = cent->currentState.constantLight;
		r = cl & 255;
		g = ( cl >> 8 ) & 255;
		b = ( cl >> 16 ) & 255;
		i = ( ( cl >> 24 ) & 255 ) * 4;
		trap_R_AddLightToScene( cent->lerpOrigin, i, i, r, g, b, 0, 0 );
	}

	if ( CG_IsTrailSystemValid( &cent->muzzleTS ) )
	{
		//FIXME hack to prevent tesla trails reaching too far
		if ( cent->currentState.eType == ET_BUILDABLE )
		{
			vec3_t front, back;

			CG_AttachmentPoint( &cent->muzzleTS->frontAttachment, front );
			CG_AttachmentPoint( &cent->muzzleTS->backAttachment, back );

			if ( Distance( front, back ) > ( TESLAGEN_RANGE * M_ROOT3 ) )
			{
				CG_DestroyTrailSystem( &cent->muzzleTS );
			}
		}

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
	refEntity_t   ent;
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

	trap_S_StartSound( NULL, cent->currentState.number, CHAN_ITEM, cgs.gameSounds[ cent->currentState.eventParm ] );

	//  ent->s.frame = ent->wait * 10;
	//  ent->s.clientNum = ent->random * 10;
	cent->miscTime = cg.time + cent->currentState.frame * 100 + cent->currentState.clientNum * 100 * crandom();
}

//============================================================================

/*
===============
CG_LaunchMissile
===============
*/
static void CG_LaunchMissile( centity_t *cent )
{
	entityState_t      *es;
	const weaponInfo_t *wi;
	particleSystem_t   *ps;
	trailSystem_t      *ts;
	weapon_t           weapon;
	weaponMode_t       weaponMode;

	es = &cent->currentState;

	weapon = es->weapon;

	if ( weapon > WP_NUM_WEAPONS )
	{
		weapon = WP_NONE;
	}

	wi = &cg_weapons[ weapon ];
	weaponMode = es->generic1;

	if ( wi->wim[ weaponMode ].missileParticleSystem )
	{
		ps = CG_SpawnNewParticleSystem( wi->wim[ weaponMode ].missileParticleSystem );

		if ( CG_IsParticleSystemValid( &ps ) )
		{
			CG_SetAttachmentCent( &ps->attachment, cent );
			CG_AttachToCent( &ps->attachment );
			ps->charge = es->torsoAnim;
		}
	}

	if ( wi->wim[ weaponMode ].missileTrailSystem )
	{
		ts = CG_SpawnNewTrailSystem( wi->wim[ weaponMode ].missileTrailSystem );

		if ( CG_IsTrailSystemValid( &ts ) )
		{
			CG_SetAttachmentCent( &ts->frontAttachment, cent );
			CG_AttachToCent( &ts->frontAttachment );
		}
	}
}

/*
===============
CG_Missile
===============
*/
static void CG_Missile( centity_t *cent )
{
	refEntity_t            ent;
	entityState_t          *es;
	const weaponInfo_t     *wi;
	weapon_t               weapon;
	weaponMode_t           weaponMode;
	const weaponInfoMode_t *wim;

	es = &cent->currentState;

	weapon = es->weapon;

	if ( weapon > WP_NUM_WEAPONS )
	{
		weapon = WP_NONE;
	}

	wi = &cg_weapons[ weapon ];
	weaponMode = es->generic1;

	wim = &wi->wim[ weaponMode ];

	// calculate the axis
	VectorCopy( es->angles, cent->lerpAngles );

	// add dynamic light
	if ( wim->missileDlight )
	{
		trap_R_AddLightToScene( cent->lerpOrigin, wim->missileDlight, wim->missileDlightIntensity,
		                        wim->missileDlightColor[ 0 ],
		                        wim->missileDlightColor[ 1 ],
		                        wim->missileDlightColor[ 2 ], 0, 0 );
	}

	// add missile sound
	if ( wim->missileSound )
	{
		vec3_t velocity;

		BG_EvaluateTrajectoryDelta( &cent->currentState.pos, cg.time, velocity );

		trap_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, velocity, wim->missileSound );
	}

	// create the render entity
	memset( &ent, 0, sizeof( ent ) );
	VectorCopy( cent->lerpOrigin, ent.origin );
	VectorCopy( cent->lerpOrigin, ent.oldorigin );

	if ( wim->usesSpriteMissle )
	{
		ent.reType = RT_SPRITE;
		ent.radius = wim->missileSpriteSize +
		             wim->missileSpriteCharge * es->torsoAnim;
		ent.rotation = 0;
		ent.customShader = wim->missileSprite;
		ent.shaderRGBA[ 0 ] = 0xFF;
		ent.shaderRGBA[ 1 ] = 0xFF;
		ent.shaderRGBA[ 2 ] = 0xFF;
		ent.shaderRGBA[ 3 ] = 0xFF;
	}
	else
	{
		ent.hModel = wim->missileModel;
		ent.renderfx = wim->missileRenderfx | RF_NOSHADOW;

		// convert direction of travel into axis
		if ( VectorNormalize2( es->pos.trDelta, ent.axis[ 0 ] ) == 0 )
		{
			ent.axis[ 0 ][ 2 ] = 1;
		}

		// spin as it moves
		if ( es->pos.trType != TR_STATIONARY && wim->missileRotates )
		{
			RotateAroundDirection( ent.axis, cg.time / 4 );
		}
		else
		{
			RotateAroundDirection( ent.axis, es->time );
		}

		if ( wim->missileAnimates )
		{
			int timeSinceStart = cg.time - es->time;

			if ( wim->missileAnimLooping )
			{
				ent.frame = wim->missileAnimStartFrame +
				            ( int )( ( timeSinceStart / 1000.0f ) * wim->missileAnimFrameRate ) %
				            wim->missileAnimNumFrames;
			}
			else
			{
				ent.frame = wim->missileAnimStartFrame +
				            ( int )( ( timeSinceStart / 1000.0f ) * wim->missileAnimFrameRate );

				if ( ent.frame > ( wim->missileAnimStartFrame + wim->missileAnimNumFrames ) )
				{
					ent.frame = wim->missileAnimStartFrame + wim->missileAnimNumFrames;
				}
			}
		}
	}

	//only refresh if there is something to display
	if ( wim->missileSprite || wim->missileModel )
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
	refEntity_t   ent;
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
CG_Beam

Also called as an event
===============
*/
void CG_Beam( centity_t *cent )
{
	refEntity_t   ent;
	entityState_t *s1;

	s1 = &cent->currentState;

	// create the render entity
	memset( &ent, 0, sizeof( ent ) );
	VectorCopy( s1->pos.trBase, ent.origin );
	VectorCopy( s1->origin2, ent.oldorigin );
	AxisClear( ent.axis );
	ent.reType = RT_BEAM;

	ent.renderfx = RF_NOSHADOW;

	// add to refresh list
	trap_R_AddRefEntityToScene( &ent );
}

/*
===============
CG_Portal
===============
*/
static void CG_Portal( centity_t *cent )
{
	refEntity_t   ent;
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
	refEntity_t   flare;
	entityState_t *es;
	vec3_t        forward, delta;
	float         len;
	trace_t       tr;
	float         maxAngle;
	vec3_t        mins, maxs, start, end;
	float         srcRadius, srLocal, ratio = 1.0f;
	int           entityNum;

	es = &cent->currentState;

	if ( cg.renderingThirdPerson )
	{
		entityNum = MAGIC_TRACE_HACK;
	}
	else
	{
		entityNum = cg.predictedPlayerState.clientNum;
	}

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

	CG_Trace( &tr, cg.refdef.vieworg, NULL, NULL, es->angles2,
	          entityNum, MASK_SHOT );

	//if there is no los between the view and the flare source
	//it definately cannot be seen
	if ( tr.fraction < 1.0f || tr.allsolid )
	{
		return;
	}

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
	AngleVectors( es->angles, forward, NULL, NULL );
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

	//only recalculate radius and ratio every three frames
	if ( !( cg.clientFrame % 2 ) )
	{
		//can only see the flare when in front of it
		flare.radius = len / es->origin2[ 0 ];

		if ( es->origin2[ 2 ] == 0 )
		{
			srcRadius = srLocal = flare.radius / 2.0f;
		}
		else
		{
			srcRadius = srLocal = len / es->origin2[ 2 ];
		}

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
			//"correct" flares
			CG_BiSphereTrace( &tr, cg.refdef.vieworg, end,
			                  1.0f, srcRadius, entityNum, MASK_SHOT );

			if ( tr.fraction < 1.0f )
			{
				ratio = tr.lateralFraction;
			}
			else
			{
				ratio = 1.0f;
			}
		}
		else if ( cg_lightFlare.integer == FLARE_TIMEFADE )
		{
			//draw timed flares
			SETBOUNDS( mins, maxs, srcRadius );
			CG_Trace( &tr, start, mins, maxs, end,
			          entityNum, MASK_SHOT );

			if ( ( tr.fraction < 1.0f || tr.startsolid ) && cent->lfs.status )
			{
				cent->lfs.status = qfalse;
				cent->lfs.lastTime = cg.time;
			}
			else if ( ( tr.fraction == 1.0f && !tr.startsolid ) && !cent->lfs.status )
			{
				cent->lfs.status = qtrue;
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
			//draw nofade flares
			SETBOUNDS( mins, maxs, srcRadius );
			CG_Trace( &tr, start, mins, maxs, end,
			          entityNum, MASK_SHOT );

			//flare source occluded
			if ( ( tr.fraction < 1.0f || tr.startsolid ) )
			{
				ratio = 0.0f;
			}
		}
	}
	else
	{
		ratio = cent->lfs.lastRatio;
		flare.radius = cent->lfs.lastRadius;
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
	centity_t     *source = NULL, *target = NULL;
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
void CG_AdjustPositionForMover( const vec3_t in, int moverNum, int fromTime, int toTime, vec3_t out )
{
	centity_t *cent;
	vec3_t    oldOrigin, origin, deltaOrigin;
	vec3_t    oldAngles, angles, deltaAngles;

	if ( moverNum <= 0 || moverNum >= ENTITYNUM_MAX_NORMAL )
	{
		VectorCopy( in, out );
		return;
	}

	cent = &cg_entities[ moverNum ];

	if ( cent->currentState.eType != ET_MOVER )
	{
		VectorCopy( in, out );
		return;
	}

	BG_EvaluateTrajectory( &cent->currentState.pos, fromTime, oldOrigin );
	BG_EvaluateTrajectory( &cent->currentState.apos, fromTime, oldAngles );

	BG_EvaluateTrajectory( &cent->currentState.pos, toTime, origin );
	BG_EvaluateTrajectory( &cent->currentState.apos, toTime, angles );

	VectorSubtract( origin, oldOrigin, deltaOrigin );
	VectorSubtract( angles, oldAngles, deltaAngles );

	VectorAdd( in, deltaOrigin, out );

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
	if ( cg.nextSnap == NULL )
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
		          cent->currentState.number, MASK_SHOT );

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
		                           cg.snap->serverTime, cg.time, cent->lerpOrigin );
	}
}

/*
================
CG_RangeMarker
================
*/
void CG_RangeMarker( centity_t *cent )
{
	qboolean drawS, drawI, drawF;
	float    so, lo, th;
	int      rmType;
	float    range;
	vec3_t   rgb;

	if ( CG_GetRangeMarkerPreferences( &drawS, &drawI, &drawF, &so, &lo, &th ) &&
	     CG_GetBuildableRangeMarkerProperties( cent->currentState.modelindex, &rmType, &range, rgb ) )
	{
		CG_DrawRangeMarker( rmType, cent->lerpOrigin, ( rmType > 0 ? cent->lerpAngles : NULL ),
		                    range, drawS, drawI, drawF, rgb, so, lo, th );
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
		case ET_MISSILE:
			CG_LaunchMissile( cent );
			break;

		case ET_BUILDABLE:
			cent->lastBuildableHealth = es->generic1;
			break;

		default:
			break;
	}

	//clear any particle systems from previous uses of this centity_t
	cent->muzzlePS = NULL;
	cent->muzzlePsTrigger = qfalse;
	cent->jetPackPS = NULL;
	cent->jetPackState = JPS_OFF;
	cent->buildablePS = NULL;
	cent->entityPS = NULL;
	cent->entityPSMissing = qfalse;

	//make sure that the buildable animations are in a consistent state
	//when a buildable enters the PVS
	cent->buildableAnim = cent->lerpFrame.animationNumber = BANIM_NONE;
	cent->oldBuildableAnim = es->legsAnim;
}

/*
===============
CG_CEntityPVSLeave

===============
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

		default:
			break;
	}
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
			CG_Error( "Bad entity type: %i\n", cent->currentState.eType );
			break;

		case ET_INVISIBLE:
		case ET_PUSH_TRIGGER:
		case ET_TELEPORT_TRIGGER:
		case ET_LOCATION:
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

		case ET_RANGE_MARKER:
			CG_RangeMarker( cent );
			break;

		case ET_MISSILE:
			CG_Missile( cent );
			break;

		case ET_MOVER:
			CG_Mover( cent );
			break;

		case ET_BEAM:
			CG_Beam( cent );
			break;

		case ET_PORTAL:
			CG_Portal( cent );
			break;

		case ET_SPEAKER:
			CG_Speaker( cent );
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
void CG_AddPacketEntities( void )
{
	int           num;
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
	BG_PlayerStateToEntityState( ps, &cg.predictedPlayerEntity.currentState, qfalse );
	cg.predictedPlayerEntity.valid = qtrue;
	CG_AddCEntity( &cg.predictedPlayerEntity );

	// lerp the non-predicted value for lightning gun origins
	CG_CalcEntityLerpPositions( &cg_entities[ cg.snap->ps.clientNum ] );

	// scanner
	CG_UpdateEntityPositions();

	for ( num = 0; num < MAX_GENTITIES; num++ )
	{
		cg_entities[ num ].valid = qfalse;
	}

	// add each entity sent over by the server
	for ( num = 0; num < cg.snap->numEntities; num++ )
	{
		cent = &cg_entities[ cg.snap->entities[ num ].number ];
		cent->valid = qtrue;
	}

	for ( num = 0; num < MAX_GENTITIES; num++ )
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
	for ( num = 0; num < cg.snap->numEntities; num++ )
	{
		cent = &cg_entities[ cg.snap->entities[ num ].number ];
		CG_AddCEntity( cent );
	}

	//make an attempt at drawing bounding boxes of selected entity types
	if ( cg_drawBBOX.integer )
	{
		for ( num = 0; num < cg.snap->numEntities; num++ )
		{
			float         x, zd, zu;
			vec3_t        mins, maxs;
			entityState_t *es;

			cent = &cg_entities[ cg.snap->entities[ num ].number ];
			es = &cent->currentState;

			switch ( es->eType )
			{
				case ET_BUILDABLE:
				case ET_MISSILE:
				case ET_CORPSE:
					x = ( es->solid & 255 );
					zd = ( ( es->solid >> 8 ) & 255 );
					zu = ( ( es->solid >> 16 ) & 255 ) - 32;

					mins[ 0 ] = mins[ 1 ] = -x;
					maxs[ 0 ] = maxs[ 1 ] = x;
					mins[ 2 ] = -zd;
					maxs[ 2 ] = zu;

					CG_DrawBoundingBox( cent->lerpOrigin, mins, maxs );
					break;

				default:
					break;
			}
		}
	}
}
