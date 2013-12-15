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

// cg_particles.c -- the particle system

#include "cg_local.h"

static baseParticleSystem_t  baseParticleSystems[ MAX_BASEPARTICLE_SYSTEMS ];
static baseParticleEjector_t baseParticleEjectors[ MAX_BASEPARTICLE_EJECTORS ];
static baseParticle_t        baseParticles[ MAX_BASEPARTICLES ];
static int                   numBaseParticleSystems = 0;
static int                   numBaseParticleEjectors = 0;
static int                   numBaseParticles = 0;

static particleSystem_t      particleSystems[ MAX_PARTICLE_SYSTEMS ];
static particleEjector_t     particleEjectors[ MAX_PARTICLE_EJECTORS ];
static particle_t            particles[ MAX_PARTICLES ];
static particle_t            *sortedParticles[ MAX_PARTICLES ];
static particle_t            *radixBuffer[ MAX_PARTICLES ];

/*
===============
CG_LerpValues

Lerp between two values
===============
*/
static float CG_LerpValues( float a, float b, float f )
{
	if ( b == PARTICLES_SAME_AS_INITIAL )
	{
		return a;
	}
	else
	{
		return ( ( a ) + ( f ) * ( ( b ) - ( a ) ) );
	}
}

/*
===============
CG_RandomiseValue

Randomise some value by some variance
===============
*/
static float CG_RandomiseValue( float value, float variance )
{
	if ( value != 0.0f )
	{
		return value * ( 1.0f + ( random() * variance ) );
	}
	else
	{
		return random() * variance;
	}
}

/*
===============
CG_SpreadVector

Randomly spread a vector by some amount
===============
*/
static void CG_SpreadVector( vec3_t v, float spread )
{
	vec3_t p, r1, r2;
	float  randomSpread = crandom() * spread;
	float  randomRotation = random() * 360.0f;

	PerpendicularVector( p, v );

	RotatePointAroundVector( r1, p, v, randomSpread );
	RotatePointAroundVector( r2, v, r1, randomRotation );

	VectorCopy( r2, v );
}

/*
===============
CG_DestroyParticle

Destroy an individual particle
===============
*/
static void CG_DestroyParticle( particle_t *p, vec3_t impactNormal )
{
	//this particle has an onDeath particle system attached
	if ( p->class_->onDeathSystemName[ 0 ] != '\0' )
	{
		particleSystem_t *ps;

		ps = CG_SpawnNewParticleSystem( p->class_->onDeathSystemHandle );

		if ( CG_IsParticleSystemValid( &ps ) )
		{
			if ( impactNormal )
			{
				CG_SetParticleSystemNormal( ps, impactNormal );
			}

			CG_SetAttachmentPoint( &ps->attachment, p->origin );
			CG_AttachToPoint( &ps->attachment );
		}
	}

	p->valid = qfalse;

	//this gives other systems a couple of
	//frames to realise the particle is gone
	p->frameWhenInvalidated = cg.clientFrame;
}

/*
===============
CG_SpawnNewParticle

Introduce a new particle into the world
===============
*/
static particle_t *CG_SpawnNewParticle( baseParticle_t *bp, particleEjector_t *parent )
{
	int               i, j;
	particle_t        *p = NULL;
	particleEjector_t *pe = parent;
	particleSystem_t  *ps = parent->parent;
	vec3_t            attachmentPoint, attachmentVelocity;
	vec3_t            transform[ 3 ];

	for ( i = 0; i < MAX_PARTICLES; i++ )
	{
		p = &particles[ i ];

		//FIXME: the + 1 may be unnecessary
		if ( !p->valid && cg.clientFrame > p->frameWhenInvalidated + 1 )
		{
			memset( p, 0, sizeof( particle_t ) );

			//found a free slot
			p->class_ = bp;
			p->parent = pe;

			p->birthTime = cg.time;
			p->lifeTime = ( int ) CG_RandomiseValue( ( float ) bp->lifeTime, bp->lifeTimeRandFrac );

			p->radius.delay = ( int ) CG_RandomiseValue( ( float ) bp->radius.delay, bp->radius.delayRandFrac );
			p->radius.initial = CG_RandomiseValue( bp->radius.initial, bp->radius.initialRandFrac );
			p->radius.final = CG_RandomiseValue( bp->radius.final, bp->radius.finalRandFrac );

			p->radius.initial += bp->scaleWithCharge * pe->parent->charge;

			p->alpha.delay = ( int ) CG_RandomiseValue( ( float ) bp->alpha.delay, bp->alpha.delayRandFrac );
			p->alpha.initial = CG_RandomiseValue( bp->alpha.initial, bp->alpha.initialRandFrac );
			p->alpha.final = CG_RandomiseValue( bp->alpha.final, bp->alpha.finalRandFrac );

			p->rotation.delay = ( int ) CG_RandomiseValue( ( float ) bp->rotation.delay, bp->rotation.delayRandFrac );
			p->rotation.initial = CG_RandomiseValue( bp->rotation.initial, bp->rotation.initialRandFrac );
			p->rotation.final = CG_RandomiseValue( bp->rotation.final, bp->rotation.finalRandFrac );

			p->dLightRadius.delay =
			  ( int ) CG_RandomiseValue( ( float ) bp->dLightRadius.delay, bp->dLightRadius.delayRandFrac );
			p->dLightRadius.initial =
			  CG_RandomiseValue( bp->dLightRadius.initial, bp->dLightRadius.initialRandFrac );
			p->dLightRadius.final =
			  CG_RandomiseValue( bp->dLightRadius.final, bp->dLightRadius.finalRandFrac );

			p->colorDelay = CG_RandomiseValue( bp->colorDelay, bp->colorDelayRandFrac );

			p->bounceMarkRadius = CG_RandomiseValue( bp->bounceMarkRadius, bp->bounceMarkRadiusRandFrac );
			p->bounceMarkCount =
			  rint( CG_RandomiseValue( ( float ) bp->bounceMarkCount, bp->bounceMarkCountRandFrac ) );
			p->bounceSoundCount =
			  rint( CG_RandomiseValue( ( float ) bp->bounceSoundCount, bp->bounceSoundCountRandFrac ) );

			if ( bp->numModels )
			{
				p->model = bp->models[ rand() % bp->numModels ];

				if ( bp->modelAnimation.frameLerp < 0 )
				{
					bp->modelAnimation.frameLerp = p->lifeTime / bp->modelAnimation.numFrames;
					bp->modelAnimation.initialLerp = p->lifeTime / bp->modelAnimation.numFrames;
				}
			}

			if ( !CG_AttachmentPoint( &ps->attachment, attachmentPoint ) )
			{
				return NULL;
			}

			VectorCopy( attachmentPoint, p->origin );

			if ( CG_AttachmentAxis( &ps->attachment, transform ) )
			{
				vec3_t transDisplacement;

				VectorMatrixMultiply( bp->displacement, transform, transDisplacement );
				VectorAdd( p->origin, transDisplacement, p->origin );
			}
			else
			{
				VectorAdd( p->origin, bp->displacement, p->origin );
			}

			for ( j = 0; j <= 2; j++ )
			{
				p->origin[ j ] += ( crandom() * bp->randDisplacement[ j ] );
			}

			switch ( bp->velMoveType )
			{
				case PMT_STATIC:
					if ( bp->velMoveValues.dirType == PMD_POINT )
					{
						VectorSubtract( bp->velMoveValues.point, p->origin, p->velocity );
					}
					else if ( bp->velMoveValues.dirType == PMD_LINEAR )
					{
						VectorCopy( bp->velMoveValues.dir, p->velocity );
					}

					break;

				case PMT_STATIC_TRANSFORM:
					if ( !CG_AttachmentAxis( &ps->attachment, transform ) )
					{
						return NULL;
					}

					if ( bp->velMoveValues.dirType == PMD_POINT )
					{
						vec3_t transPoint;

						VectorMatrixMultiply( bp->velMoveValues.point, transform, transPoint );
						VectorSubtract( transPoint, p->origin, p->velocity );
					}
					else if ( bp->velMoveValues.dirType == PMD_LINEAR )
					{
						VectorMatrixMultiply( bp->velMoveValues.dir, transform, p->velocity );
					}

					break;

				case PMT_TAG:
				case PMT_CENT_ANGLES:
					if ( bp->velMoveValues.dirType == PMD_POINT )
					{
						VectorSubtract( attachmentPoint, p->origin, p->velocity );
					}
					else if ( bp->velMoveValues.dirType == PMD_LINEAR )
					{
						if ( !CG_AttachmentDir( &ps->attachment, p->velocity ) )
						{
							return NULL;
						}
					}

					break;

				case PMT_NORMAL:
					if ( !ps->normalValid )
					{
						CG_Printf( S_ERROR "a particle with velocityType "
						           "normal has no normal\n" );
						return NULL;
					}

					VectorCopy( ps->normal, p->velocity );

					//normal displacement
					VectorNormalize( p->velocity );
					VectorMA( p->origin, bp->normalDisplacement, p->velocity, p->origin );
					break;

				case PMT_LAST_NORMAL:
					VectorCopy( ps->lastNormal, p->velocity );
					VectorNormalize( p->velocity );
					VectorMA( p->origin, bp->normalDisplacement, p->velocity, p->origin );
					break;

				case PMT_OPPORTUNISTIC_NORMAL:
					if ( ps->lastNormalIsCurrent )
					{
						VectorCopy( ps->lastNormal, p->velocity );
						VectorNormalize( p->velocity );
						VectorMA( p->origin, bp->normalDisplacement, p->velocity, p->origin );
					}
					break;
			}

			VectorNormalize( p->velocity );
			CG_SpreadVector( p->velocity, bp->velMoveValues.dirRandAngle );
			VectorScale( p->velocity,
			             CG_RandomiseValue( bp->velMoveValues.mag, bp->velMoveValues.magRandFrac ),
			             p->velocity );

			if ( CG_AttachmentVelocity( &ps->attachment, attachmentVelocity ) )
			{
				VectorMA( p->velocity,
				          CG_RandomiseValue( bp->velMoveValues.parentVelFrac,
				                             bp->velMoveValues.parentVelFracRandFrac ), attachmentVelocity, p->velocity );
			}

			p->lastEvalTime = cg.time;

			p->valid = qtrue;

			//this particle has a child particle system attached
			if ( bp->childSystemName[ 0 ] != '\0' )
			{
				particleSystem_t *chps = CG_SpawnNewParticleSystem( bp->childSystemHandle );

				if ( CG_IsParticleSystemValid( &chps ) )
				{
					CG_SetAttachmentParticle( &chps->attachment, p );
					CG_AttachToParticle( &chps->attachment );
					p->childParticleSystem = chps;

					if ( ps->lastNormalIsCurrent )
						CG_SetParticleSystemLastNormal( chps, ps->lastNormal );
					else
						VectorCopy( ps->lastNormal, chps->lastNormal );
				}
			}

			//this particle has a child trail system attached
			if ( bp->childTrailSystemName[ 0 ] != '\0' )
			{
				trailSystem_t *ts = CG_SpawnNewTrailSystem( bp->childTrailSystemHandle );

				if ( CG_IsTrailSystemValid( &ts ) )
				{
					CG_SetAttachmentParticle( &ts->frontAttachment, p );
					CG_AttachToParticle( &ts->frontAttachment );
				}
			}

			break;
		}
	}

	return p;
}

/*
===============
CG_SpawnNewParticles

Check if there are any ejectors that should be
introducing new particles
===============
*/
static void CG_SpawnNewParticles( void )
{
	int                   i, j;
	particle_t            *p;
	particleSystem_t      *ps;
	particleEjector_t     *pe;
	baseParticleEjector_t *bpe;
	float                 lerpFrac;
	int                   count;

	for ( i = 0; i < MAX_PARTICLE_EJECTORS; i++ )
	{
		pe = &particleEjectors[ i ];
		ps = pe->parent;

		if ( pe->valid )
		{
			//a non attached particle system can't make particles
			if ( !CG_Attached( &ps->attachment ) )
			{
				continue;
			}

			bpe = particleEjectors[ i ].class_;

			//if this system is scheduled for removal don't make any new particles
			if ( !ps->lazyRemove )
			{
				while ( pe->nextEjectionTime <= cg.time &&
				        ( pe->count > 0 || pe->totalParticles == PARTICLES_INFINITE ) )
				{
					for ( j = 0; j < bpe->numParticles; j++ )
					{
						CG_SpawnNewParticle( bpe->particles[ j ], pe );
					}

					if ( pe->count > 0 )
					{
						pe->count--;
					}

					//calculate next ejection time
					lerpFrac = 1.0 - ( ( float ) pe->count / ( float ) pe->totalParticles );
					pe->nextEjectionTime = cg.time + ( int ) CG_RandomiseValue(
					                         CG_LerpValues( pe->ejectPeriod.initial,
					                                        pe->ejectPeriod.final,
					                                        lerpFrac ),
					                         pe->ejectPeriod.randFrac );
				}
			}

			if ( pe->count == 0 || ps->lazyRemove )
			{
				count = 0;

				//wait for child particles to die before declaring this pe invalid
				for ( j = 0; j < MAX_PARTICLES; j++ )
				{
					p = &particles[ j ];

					if ( p->valid && p->parent == pe )
					{
						count++;
					}
				}

				if ( !count )
				{
					pe->valid = qfalse;
				}
			}
		}
	}
}

/*
===============
CG_SpawnNewParticleEjector

Allocate a new particle ejector
===============
*/
static particleEjector_t *CG_SpawnNewParticleEjector( baseParticleEjector_t *bpe,
    particleSystem_t *parent )
{
	int               i;
	particleEjector_t *pe = NULL;
	particleSystem_t  *ps = parent;

	for ( i = 0; i < MAX_PARTICLE_EJECTORS; i++ )
	{
		pe = &particleEjectors[ i ];

		if ( !pe->valid )
		{
			memset( pe, 0, sizeof( particleEjector_t ) );

			//found a free slot
			pe->class_ = bpe;
			pe->parent = ps;

			pe->ejectPeriod.initial = bpe->eject.initial;
			pe->ejectPeriod.final = bpe->eject.final;
			pe->ejectPeriod.randFrac = bpe->eject.randFrac;

			pe->nextEjectionTime = cg.time +
			                       ( int ) CG_RandomiseValue( ( float ) bpe->eject.delay, bpe->eject.delayRandFrac );
			pe->count = pe->totalParticles =
			              ( int ) rint( CG_RandomiseValue( ( float ) bpe->totalParticles, bpe->totalParticlesRandFrac ) );

			pe->valid = qtrue;

			if ( cg_debugParticles.integer >= 1 )
			{
				CG_Printf( "PE %s created\n", ps->class_->name );
			}

			break;
		}
	}

	return pe;
}

/*
===============
CG_SpawnNewParticleSystem

Allocate a new particle system
===============
*/
particleSystem_t *CG_SpawnNewParticleSystem( qhandle_t psHandle )
{
	int                  i, j;
	particleSystem_t     *ps = NULL;
	baseParticleSystem_t *bps = &baseParticleSystems[ psHandle - 1 ];

	if ( !bps->registered )
	{
		CG_Printf( S_ERROR "a particle system has not been registered yet\n" );
		return NULL;
	}

	for ( i = 0; i < MAX_PARTICLE_SYSTEMS; i++ )
	{
		ps = &particleSystems[ i ];

		if ( !ps->valid )
		{
			memset( ps, 0, sizeof( particleSystem_t ) );

			//found a free slot
			ps->class_ = bps;

			ps->valid = qtrue;
			ps->lazyRemove = qfalse;

			// use "up" as an arbitrary (non-null) "last" normal
			VectorSet( ps->lastNormal, 0, 0, 1 );

			for ( j = 0; j < bps->numEjectors; j++ )
			{
				CG_SpawnNewParticleEjector( bps->ejectors[ j ], ps );
			}

			if ( cg_debugParticles.integer >= 1 )
			{
				CG_Printf( "PS %s created\n", bps->name );
			}

			break;
		}
	}

	return ps;
}

/*
===============
CG_RegisterParticleSystem

Load the shaders required for a particle system
===============
*/
qhandle_t CG_RegisterParticleSystem( const char *name )
{
	int                   i, j, k, l;
	baseParticleSystem_t  *bps;
	baseParticleEjector_t *bpe;
	baseParticle_t        *bp;

	for ( i = 0; i < MAX_BASEPARTICLE_SYSTEMS; i++ )
	{
		bps = &baseParticleSystems[ i ];

		if ( !Q_strnicmp( bps->name, name, MAX_QPATH ) )
		{
			//already registered
			if ( bps->registered )
			{
				return i + 1;
			}

			for ( j = 0; j < bps->numEjectors; j++ )
			{
				bpe = bps->ejectors[ j ];

				for ( l = 0; l < bpe->numParticles; l++ )
				{
					bp = bpe->particles[ l ];

					for ( k = 0; k < bp->numFrames; k++ )
					{
						bp->shaders[ k ] = trap_R_RegisterShader(bp->shaderNames[k],
											 RSF_DEFAULT);
					}

					for ( k = 0; k < bp->numModels; k++ )
					{
						bp->models[ k ] = trap_R_RegisterModel( bp->modelNames[ k ] );
					}

					if ( bp->bounceMarkName[ 0 ] != '\0' )
					{
						bp->bounceMark = trap_R_RegisterShader(bp->bounceMarkName,
										       RSF_DEFAULT);
					}

					if ( bp->bounceSoundName[ 0 ] != '\0' )
					{
						bp->bounceSound = trap_S_RegisterSound( bp->bounceSoundName, qfalse );
					}

					//recursively register any children
					if ( bp->childSystemName[ 0 ] != '\0' )
					{
						//don't care about a handle for children since
						//the system deals with it
						CG_RegisterParticleSystem( bp->childSystemName );
					}

					if ( bp->onDeathSystemName[ 0 ] != '\0' )
					{
						//don't care about a handle for children since
						//the system deals with it
						CG_RegisterParticleSystem( bp->onDeathSystemName );
					}

					if ( bp->childTrailSystemName[ 0 ] != '\0' )
					{
						bp->childTrailSystemHandle = CG_RegisterTrailSystem( bp->childTrailSystemName );
					}
				}
			}

			if ( cg_debugParticles.integer >= 1 )
			{
				CG_Printf( "Registered particle system %s\n", name );
			}

			bps->registered = qtrue;

			//avoid returning 0
			return i + 1;
		}
	}

	CG_Printf( S_ERROR "failed to register particle system %s\n", name );
	return 0;
}

/*
===============
CG_ParseValueAndVariance

Parse a value and its random variance
===============
*/
static void CG_ParseValueAndVariance( char *token, float *value, float *variance, qboolean allowNegative )
{
	char  valueBuffer[ 16 ];
	char  *variancePtr = NULL, *varEndPointer = NULL;
	float localValue = 0.0f;
	float localVariance = 0.0f;

	Q_strncpyz( valueBuffer, token, sizeof( valueBuffer ) );

	variancePtr = strchr( valueBuffer, '~' );

	//variance included
	if ( variancePtr )
	{
		variancePtr[ 0 ] = '\0';
		variancePtr++;

		localValue = atof_neg( valueBuffer, allowNegative );

		varEndPointer = strchr( variancePtr, '%' );

		if ( varEndPointer )
		{
			varEndPointer[ 0 ] = '\0';
			localVariance = atof_neg( variancePtr, qfalse ) / 100.0f;
		}
		else
		{
			if ( localValue != 0.0f )
			{
				localVariance = atof_neg( variancePtr, qfalse ) / localValue;
			}
			else
			{
				localVariance = atof_neg( variancePtr, qfalse );
			}
		}
	}
	else
	{
		localValue = atof_neg( valueBuffer, allowNegative );
	}

	if ( value != NULL )
	{
		*value = localValue;
	}

	if ( variance != NULL )
	{
		*variance = localVariance;
	}
}

/*
CG_ParseParticle helpers
*/
static void CG_CopyLine( int *i, char *toks, int num, size_t size, char **text_p )
{
	char *token;

	while( *i < num )
	{
		token = COM_ParseExt( text_p, qfalse );

		if ( !*token )
		{
			break;
		}

		Q_strncpyz( toks, token, size );
		(*i)++;

		toks += size;
	}
}

static qboolean CG_ParseType( pMoveType_t *pmt, char **text_p )
{
	char *token = COM_Parse( text_p );

	if( !*token )
	{
		return qfalse;
	}

	if ( !Q_stricmp( token, "static" ) )
	{
		*pmt = PMT_STATIC;
	}
	else if ( !Q_stricmp( token, "static_transform" ) )
	{
		*pmt = PMT_STATIC_TRANSFORM;
	}
	else if ( !Q_stricmp( token, "tag" ) )
	{
		*pmt = PMT_TAG;
	}
	else if ( !Q_stricmp( token, "cent" ) )
	{
		*pmt = PMT_CENT_ANGLES;
	}
	else if ( !Q_stricmp( token, "normal" ) )
	{
		*pmt = PMT_NORMAL;
	}
	else if ( !Q_stricmp( token, "last_normal" ) )
	{
		*pmt = PMT_LAST_NORMAL;
	}
	else if ( !Q_stricmp( token, "opportunistic_normal" ) )
	{
		*pmt = PMT_OPPORTUNISTIC_NORMAL;
	}

	return qtrue;
}

static qboolean CG_ParseDir( pMoveValues_t *pmv, char **text_p )
{
	char *token = COM_Parse( text_p );

	if ( !*token )
	{
		return qfalse;
	}

	if ( !Q_stricmp( token, "linear" ) )
	{
		pmv->dirType = PMD_LINEAR;
	}
	else if ( !Q_stricmp( token, "point" ) )
	{
		pmv->dirType = PMD_POINT;
	}

	return qtrue;
}

static qboolean CG_ParseFinal( pLerpValues_t *plv, char **text_p, qboolean allowNegative )
{
	char *token = COM_Parse( text_p );

	if( !*token )
	{
		return qfalse;
	}

	if ( !Q_stricmp( token, "-" ) )
	{
		plv->final = PARTICLES_SAME_AS_INITIAL;
		plv->finalRandFrac = 0.0f;
	}
	else
	{
		CG_ParseValueAndVariance( token, &plv->final, &plv->finalRandFrac, allowNegative );
	}

	return qtrue;
}

/*
===============
CG_ParseParticle

Parse a particle section
===============
*/
static qboolean CG_ParseParticle( baseParticle_t *bp, char **text_p )
{
	char  *token;
	float number, randFrac;
	int   i;

	// read optional parameters
	while ( 1 )
	{
		token = COM_Parse( text_p );

		if ( !*token )
		{
			return qfalse;
		}

		if ( !Q_stricmp( token, "bounce" ) )
		{
			token = COM_Parse( text_p );

			if ( !*token )
			{
				break;
			}

			if ( !Q_stricmp( token, "cull" ) )
			{
				bp->bounceCull = qtrue;

				bp->bounceFrac = -1.0f;
				bp->bounceFracRandFrac = 0.0f;
			}
			else
			{
				CG_ParseValueAndVariance( token, &bp->bounceFrac, &bp->bounceFracRandFrac, qfalse );
			}
		}
		else if ( !Q_stricmp( token, "bounceMark" ) )
		{
			token = COM_Parse( text_p );

			if ( !*token )
			{
				break;
			}

			CG_ParseValueAndVariance( token, &bp->bounceMarkCount, &bp->bounceMarkCountRandFrac, qfalse );

			token = COM_Parse( text_p );

			if ( !*token )
			{
				break;
			}

			CG_ParseValueAndVariance( token, &bp->bounceMarkRadius, &bp->bounceMarkRadiusRandFrac, qfalse );

			token = COM_ParseExt( text_p, qfalse );

			if ( !*token )
			{
				break;
			}

			Q_strncpyz( bp->bounceMarkName, token, MAX_QPATH );
		}
		else if ( !Q_stricmp( token, "bounceSound" ) )
		{
			token = COM_Parse( text_p );

			if ( !*token )
			{
				break;
			}

			CG_ParseValueAndVariance( token, &bp->bounceSoundCount, &bp->bounceSoundCountRandFrac, qfalse );

			token = COM_Parse( text_p );

			if ( !*token )
			{
				break;
			}

			Q_strncpyz( bp->bounceSoundName, token, MAX_QPATH );
		}
		else if ( !Q_stricmp( token, "shader" ) )
		{
			if ( bp->numModels > 0 )
			{
				CG_Printf( S_ERROR "'shader' not allowed in "
				           "conjunction with 'model'\n" );
				break;
			}

			token = COM_Parse( text_p );

			if ( !*token )
			{
				break;
			}

			if ( !Q_stricmp( token, "sync" ) )
			{
				bp->framerate = 0.0f;
			}
			else
			{
				bp->framerate = atof_neg( token, qfalse );
			}

			CG_CopyLine( &bp->numFrames, bp->shaderNames[ 0 ], ARRAY_LEN( bp->shaderNames ), MAX_QPATH, text_p );
		}
		else if ( !Q_stricmp( token, "model" ) )
		{
			if ( bp->numFrames > 0 )
			{
				CG_Printf( S_ERROR "'model' not allowed in "
				           "conjunction with 'shader'\n" );
				break;
			}

			CG_CopyLine( &bp->numModels, bp->modelNames[ 0 ], ARRAY_LEN( bp->modelNames ), MAX_QPATH, text_p );
		}
		else if ( !Q_stricmp( token, "modelAnimation" ) )
		{
			token = COM_Parse( text_p );

			if ( !*token )
			{
				break;
			}

			bp->modelAnimation.firstFrame = atoi_neg( token, qfalse );

			token = COM_Parse( text_p );

			if ( !*token )
			{
				break;
			}

			bp->modelAnimation.numFrames = atoi( token );
			bp->modelAnimation.reversed = qfalse;
			bp->modelAnimation.flipflop = qfalse;

			// if numFrames is negative the animation is reversed
			if ( bp->modelAnimation.numFrames < 0 )
			{
				bp->modelAnimation.numFrames = -bp->modelAnimation.numFrames;
				bp->modelAnimation.reversed = qtrue;
			}

			token = COM_Parse( text_p );

			if ( !*token )
			{
				break;
			}

			bp->modelAnimation.loopFrames = atoi( token );

			token = COM_Parse( text_p );

			if ( !*token )
			{
				break;
			}

			if ( !Q_stricmp( token, "sync" ) )
			{
				bp->modelAnimation.frameLerp = -1;
				bp->modelAnimation.initialLerp = -1;
			}
			else
			{
				float fps = atof_neg( token, qfalse );

				if ( fps == 0.0f )
				{
					fps = 1.0f;
				}

				bp->modelAnimation.frameLerp = 1000 / fps;
				bp->modelAnimation.initialLerp = 1000 / fps;
			}
		}
		///
		else if ( !Q_stricmp( token, "velocityType" ) )
		{
			if ( !CG_ParseType( &bp->velMoveType, text_p ) )
			{
				break;
			}
		}
		else if ( !Q_stricmp( token, "velocityDir" ) )
		{
			if ( ! CG_ParseDir( &bp->velMoveValues, text_p ) )
			{
				break;
			}
		}
		else if ( !Q_stricmp( token, "velocityMagnitude" ) )
		{
			token = COM_Parse( text_p );

			if ( !*token )
			{
				break;
			}

			CG_ParseValueAndVariance( token, &bp->velMoveValues.mag, &bp->velMoveValues.magRandFrac, qtrue );
		}
		else if ( !Q_stricmp( token, "parentVelocityFraction" ) )
		{
			token = COM_Parse( text_p );

			if ( !*token )
			{
				break;
			}

			CG_ParseValueAndVariance( token, &bp->velMoveValues.parentVelFrac, &bp->velMoveValues.parentVelFracRandFrac, qfalse );
		}
		else if ( !Q_stricmp( token, "velocity" ) )
		{
			for ( i = 0; i <= 2; i++ )
			{
				token = COM_Parse( text_p );

				if ( !*token )
				{
					break;
				}

				bp->velMoveValues.dir[ i ] = atof_neg( token, qtrue );
			}

			token = COM_Parse( text_p );

			if ( !*token )
			{
				break;
			}

			CG_ParseValueAndVariance( token, NULL, &bp->velMoveValues.dirRandAngle, qfalse );
		}
		else if ( !Q_stricmp( token, "velocityPoint" ) )
		{
			for ( i = 0; i <= 2; i++ )
			{
				token = COM_Parse( text_p );

				if ( !*token )
				{
					break;
				}

				bp->velMoveValues.point[ i ] = atof_neg( token, qtrue );
			}

			token = COM_Parse( text_p );

			if ( !*token )
			{
				break;
			}

			CG_ParseValueAndVariance( token, NULL, &bp->velMoveValues.pointRandAngle, qfalse );
		}
		///
		else if ( !Q_stricmp( token, "accelerationType" ) )
		{
			if ( !CG_ParseType( &bp->accMoveType, text_p ) )
			{
				break;
			}
		}
		else if ( !Q_stricmp( token, "accelerationDir" ) )
		{
			if ( !CG_ParseDir( &bp->accMoveValues, text_p ) )
			{
				break;
			}
		}
		else if ( !Q_stricmp( token, "accelerationMagnitude" ) )
		{
			token = COM_Parse( text_p );

			if ( !*token )
			{
				break;
			}

			CG_ParseValueAndVariance( token, &bp->accMoveValues.mag, &bp->accMoveValues.magRandFrac, qtrue );
		}
		else if ( !Q_stricmp( token, "acceleration" ) )
		{
			for ( i = 0; i <= 2; i++ )
			{
				token = COM_Parse( text_p );

				if ( !*token )
				{
					break;
				}

				bp->accMoveValues.dir[ i ] = atof_neg( token, qtrue );
			}

			token = COM_Parse( text_p );

			if ( !*token )
			{
				break;
			}

			CG_ParseValueAndVariance( token, NULL, &bp->accMoveValues.dirRandAngle, qfalse );
		}
		else if ( !Q_stricmp( token, "accelerationPoint" ) )
		{
			for ( i = 0; i <= 2; i++ )
			{
				token = COM_Parse( text_p );

				if ( !*token )
				{
					break;
				}

				bp->accMoveValues.point[ i ] = atof_neg( token, qtrue );
			}

			token = COM_Parse( text_p );

			if ( !*token )
			{
				break;
			}

			CG_ParseValueAndVariance( token, NULL, &bp->accMoveValues.pointRandAngle, qfalse );
		}
		///
		else if ( !Q_stricmp( token, "displacement" ) )
		{
			for ( i = 0; i <= 2; i++ )
			{
				token = COM_Parse( text_p );

				if ( !*token )
				{
					break;
				}

				CG_ParseValueAndVariance( token, &bp->displacement[ i ],
				                          &bp->randDisplacement[ i ], qtrue );
			}

			// if there is another token on the same line interpret it as an
			// additional displacement in all three directions, for compatibility
			// with the old scripts where this was the only option
			randFrac = 0;
			token = COM_ParseExt( text_p, qfalse );

			if ( token )
			{
				CG_ParseValueAndVariance( token, NULL, &randFrac, qtrue );
			}

			for ( i = 0; i < 3; i++ )
			{
				// convert randDisplacement from proportions to absolute values
				if ( bp->displacement[ i ] != 0 )
				{
					bp->randDisplacement[ i ] *= bp->displacement[ i ];
				}

				bp->randDisplacement[ i ] += randFrac;
			}
		}
		else if ( !Q_stricmp( token, "normalDisplacement" ) )
		{
			token = COM_Parse( text_p );

			if ( !*token )
			{
				break;
			}

			bp->normalDisplacement = atof_neg( token, qtrue );
		}
		else if ( !Q_stricmp( token, "overdrawProtection" ) )
		{
			bp->overdrawProtection = qtrue;
		}
		else if ( !Q_stricmp( token, "realLight" ) )
		{
			bp->realLight = qtrue;
		}
		else if ( !Q_stricmp( token, "dynamicLight" ) )
		{
			bp->dynamicLight = qtrue;

			token = COM_Parse( text_p );

			if ( !*token )
			{
				break;
			}

			CG_ParseValueAndVariance( token, &number, &bp->dLightRadius.delayRandFrac, qfalse );
			bp->dLightRadius.delay = ( int ) number;

			token = COM_Parse( text_p );

			if ( !*token )
			{
				break;
			}

			CG_ParseValueAndVariance( token, &bp->dLightRadius.initial, &bp->dLightRadius.initialRandFrac, qfalse );

			if ( !CG_ParseFinal( &bp->dLightRadius, text_p, qfalse ) )
			{
				break;
			}

			token = COM_Parse( text_p );

			if ( !*token )
			{
				break;
			}

			if ( !Q_stricmp( token, "{" ) )
			{
				if ( !CG_ParseColor( bp->dLightColor, text_p ) )
				{
					break;
				}
			}
		}
		else if ( !Q_stricmp( token, "cullOnStartSolid" ) )
		{
			bp->cullOnStartSolid = qtrue;
		}
		else if ( !Q_stricmp( token, "radius" ) )
		{
			token = COM_Parse( text_p );

			if ( !*token )
			{
				break;
			}

			CG_ParseValueAndVariance( token, &number, &bp->radius.delayRandFrac, qfalse );
			bp->radius.delay = ( int ) number;

			token = COM_Parse( text_p );

			if ( !*token )
			{
				break;
			}

			CG_ParseValueAndVariance( token, &bp->radius.initial, &bp->radius.initialRandFrac, qfalse );

			if ( !CG_ParseFinal( &bp->radius, text_p, qfalse ) )
			{
				break;
			}
		}
		else if ( !Q_stricmp( token, "physicsRadius" ) )
		{
			token = COM_Parse( text_p );

			if ( !*token )
			{
				break;
			}

			bp->physicsRadius = atoi( token );
		}
		else if ( !Q_stricmp( token, "alpha" ) )
		{
			token = COM_Parse( text_p );

			if ( !*token )
			{
				break;
			}

			CG_ParseValueAndVariance( token, &number, &bp->alpha.delayRandFrac, qfalse );
			bp->alpha.delay = ( int ) number;

			token = COM_Parse( text_p );

			if ( !*token )
			{
				break;
			}

			CG_ParseValueAndVariance( token, &bp->alpha.initial, &bp->alpha.initialRandFrac, qfalse );

			if ( !CG_ParseFinal( &bp->alpha, text_p, qfalse ) )
			{
				break;
			}
		}
		else if ( !Q_stricmp( token, "color" ) )
		{
			token = COM_Parse( text_p );

			if ( !*token )
			{
				break;
			}

			CG_ParseValueAndVariance( token, &number, &bp->colorDelayRandFrac, qfalse );
			bp->colorDelay = ( int ) number;

			token = COM_Parse( text_p );

			if ( !*token )
			{
				break;
			}

			if ( !Q_stricmp( token, "{" ) )
			{
				if ( !CG_ParseColor( bp->initialColor, text_p ) )
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
					bp->finalColor[ 0 ] = bp->initialColor[ 0 ];
					bp->finalColor[ 1 ] = bp->initialColor[ 1 ];
					bp->finalColor[ 2 ] = bp->initialColor[ 2 ];
				}
				else if ( !Q_stricmp( token, "{" ) )
				{
					if ( !CG_ParseColor( bp->finalColor, text_p ) )
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
		}
		else if ( !Q_stricmp( token, "rotation" ) )
		{
			token = COM_Parse( text_p );

			if ( !*token )
			{
				break;
			}

			CG_ParseValueAndVariance( token, &number, &bp->rotation.delayRandFrac, qfalse );
			bp->rotation.delay = ( int ) number;

			token = COM_Parse( text_p );

			if ( !*token )
			{
				break;
			}

			CG_ParseValueAndVariance( token, &bp->rotation.initial, &bp->rotation.initialRandFrac, qtrue );

			if ( !CG_ParseFinal( &bp->rotation, text_p, qtrue ) )
			{
				break;
			}
		}
		else if ( !Q_stricmp( token, "lifeTime" ) )
		{
			token = COM_Parse( text_p );

			if ( !*token )
			{
				break;
			}

			CG_ParseValueAndVariance( token, &number, &bp->lifeTimeRandFrac, qfalse );
			bp->lifeTime = ( int ) number;
		}
		else if ( !Q_stricmp( token, "childSystem" ) )
		{
			token = COM_Parse( text_p );

			if ( !*token )
			{
				break;
			}

			Q_strncpyz( bp->childSystemName, token, MAX_QPATH );
		}
		else if ( !Q_stricmp( token, "onDeathSystem" ) )
		{
			token = COM_Parse( text_p );

			if ( !*token )
			{
				break;
			}

			Q_strncpyz( bp->onDeathSystemName, token, MAX_QPATH );
		}
		else if ( !Q_stricmp( token, "childTrailSystem" ) )
		{
			token = COM_Parse( text_p );

			if ( !*token )
			{
				break;
			}

			Q_strncpyz( bp->childTrailSystemName, token, MAX_QPATH );
		}
		else if ( !Q_stricmp( token, "scaleWithCharge" ) )
		{
			token = COM_Parse( text_p );

			if ( !*token )
			{
				break;
			}

			bp->scaleWithCharge = atof( token );
		}
		else if ( !Q_stricmp( token, "}" ) )
		{
			return qtrue; //reached the end of this particle
		}
		else
		{
			CG_Printf( S_ERROR "unknown token '%s' in particle\n", token );
			return qfalse;
		}
	}

	return qfalse;
}

/*
===============
CG_InitialiseBaseParticle
===============
*/
static void CG_InitialiseBaseParticle( baseParticle_t *bp )
{
	memset( bp, 0, sizeof( baseParticle_t ) );

	memset( bp->initialColor, 0xFF, sizeof( bp->initialColor ) );
	memset( bp->finalColor, 0xFF, sizeof( bp->finalColor ) );
}

/*
===============
CG_ParseParticleEjector

Parse a particle ejector section
===============
*/
static qboolean CG_ParseParticleEjector( baseParticleEjector_t *bpe, char **text_p )
{
	char  *token;
	float number;

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
			CG_InitialiseBaseParticle( &baseParticles[ numBaseParticles ] );

			if ( !CG_ParseParticle( &baseParticles[ numBaseParticles ], text_p ) )
			{
				CG_Printf( S_ERROR "failed to parse particle\n" );
				return qfalse;
			}

			if ( bpe->numParticles == MAX_PARTICLES_PER_EJECTOR )
			{
				CG_Printf( S_ERROR "ejector has > %d particles\n", MAX_PARTICLES_PER_EJECTOR );
				return qfalse;
			}

			if ( numBaseParticles == MAX_BASEPARTICLES )
			{
				CG_Printf( S_ERROR "maximum number of particles (%d) reached\n", MAX_BASEPARTICLES );
				return qfalse;
			}

			//start parsing particles again
			bpe->particles[ bpe->numParticles ] = &baseParticles[ numBaseParticles ];
			bpe->numParticles++;
			numBaseParticles++;
		}
		else if ( !Q_stricmp( token, "delay" ) )
		{
			token = COM_Parse( text_p );

			if ( !*token )
			{
				break;
			}

			CG_ParseValueAndVariance( token, &number, &bpe->eject.delayRandFrac, qfalse );
			bpe->eject.delay = ( int ) number;
		}
		else if ( !Q_stricmp( token, "period" ) )
		{
			token = COM_Parse( text_p );

			if ( !*token )
			{
				break;
			}

			bpe->eject.initial = atoi_neg( token, qfalse );

			token = COM_Parse( text_p );

			if ( !*token )
			{
				break;
			}

			if ( !Q_stricmp( token, "-" ) )
			{
				bpe->eject.final = PARTICLES_SAME_AS_INITIAL;
			}
			else
			{
				bpe->eject.final = atoi_neg( token, qfalse );
			}

			token = COM_Parse( text_p );

			if ( !*token )
			{
				break;
			}

			CG_ParseValueAndVariance( token, NULL, &bpe->eject.randFrac, qfalse );
		}
		else if ( !Q_stricmp( token, "count" ) )
		{
			token = COM_Parse( text_p );

			if ( !*token )
			{
				break;
			}

			if ( !Q_stricmp( token, "infinite" ) )
			{
				bpe->totalParticles = PARTICLES_INFINITE;
				bpe->totalParticlesRandFrac = 0.0f;
			}
			else
			{
				CG_ParseValueAndVariance( token, &number, &bpe->totalParticlesRandFrac, qfalse );
				bpe->totalParticles = ( int ) number;
			}
		}
		else if ( !Q_stricmp( token, "particle" ) )  //acceptable text
		{
			continue;
		}
		else if ( !Q_stricmp( token, "}" ) )
		{
			return qtrue; //reached the end of this particle ejector
		}
		else
		{
			CG_Printf( S_ERROR "unknown token '%s' in particle ejector\n", token );
			return qfalse;
		}
	}

	return qfalse;
}

/*
===============
CG_ParseParticleSystem

Parse a particle system section
===============
*/
static qboolean CG_ParseParticleSystem( baseParticleSystem_t *bps, char **text_p, const char *name )
{
	char                  *token;
	baseParticleEjector_t *bpe;

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
			if ( !CG_ParseParticleEjector( &baseParticleEjectors[ numBaseParticleEjectors ], text_p ) )
			{
				CG_Printf( S_ERROR "failed to parse particle ejector\n" );
				return qfalse;
			}

			bpe = &baseParticleEjectors[ numBaseParticleEjectors ];

			//check for infinite count + zero period
			if ( bpe->totalParticles == PARTICLES_INFINITE &&
			     ( bpe->eject.initial == 0.0f || bpe->eject.final == 0.0f ) )
			{
				CG_Printf( S_ERROR "ejector with 'count infinite' potentially has zero period\n" );
				return qfalse;
			}

			if ( bps->numEjectors == MAX_EJECTORS_PER_SYSTEM )
			{
				CG_Printf( S_ERROR "particle system has > %d ejectors\n", MAX_EJECTORS_PER_SYSTEM );
				return qfalse;
			}

			if ( numBaseParticleEjectors == MAX_BASEPARTICLE_EJECTORS )
			{
				CG_Printf( S_ERROR "maximum number of particle ejectors (%d) reached\n",
				           MAX_BASEPARTICLE_EJECTORS );
				return qfalse;
			}

			//start parsing ejectors again
			bps->ejectors[ bps->numEjectors ] = &baseParticleEjectors[ numBaseParticleEjectors ];
			bps->numEjectors++;
			numBaseParticleEjectors++;
		}
		else if ( !Q_stricmp( token, "thirdPersonOnly" ) )
		{
			bps->thirdPersonOnly = qtrue;
		}
		else if ( !Q_stricmp( token, "ejector" ) )  //acceptable text
		{
			continue;
		}
		else if ( !Q_stricmp( token, "}" ) )
		{
			if ( cg_debugParticles.integer >= 1 )
			{
				CG_Printf( "Parsed particle system %s\n", name );
			}

			return qtrue; //reached the end of this particle system
		}
		else
		{
			CG_Printf( S_ERROR "unknown token '%s' in particle system %s\n", token, bps->name );
			return qfalse;
		}
	}

	return qfalse;
}

/*
===============
CG_ParseParticleFile

Load the particle systems from a particle file
===============
*/
static qboolean CG_ParseParticleFile( const char *fileName )
{
	char         *text_p;
	int          i;
	int          len;
	char         *token;
	char         text[ 32000 ];
	char         psName[ MAX_QPATH ];
	qboolean     psNameSet = qfalse;
	fileHandle_t f;

	// load the file
	len = trap_FS_FOpenFile( fileName, &f, FS_READ );

	if ( len < 0 )
	{
		return qfalse;
	}

	if ( len == 0 || len + 1 >= (int) sizeof( text ) )
	{
		trap_FS_FCloseFile( f );
		CG_Printf( len ? S_ERROR "particle file %s is too long\n" : S_ERROR "particle file %s is empty\n", fileName );
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
			if ( psNameSet )
			{
				Q_strncpyz( baseParticleSystems[ numBaseParticleSystems ].name, psName, MAX_QPATH );

				if ( !CG_ParseParticleSystem( &baseParticleSystems[ numBaseParticleSystems ], &text_p, psName ) )
				{
					CG_Printf( S_ERROR "%s: failed to parse particle system %s\n", fileName, psName );
					return qfalse;
				}

				//start parsing particle systems again
				psNameSet = qfalse;

				if ( numBaseParticleSystems == MAX_BASEPARTICLE_SYSTEMS )
				{
					CG_Printf( S_ERROR "maximum number of particle systems (%d) reached\n",
					           MAX_BASEPARTICLE_SYSTEMS );
					return qfalse;
				}

				numBaseParticleSystems++;
			}
			else
			{
				CG_Printf( S_ERROR "unnamed particle system\n" );
				return qfalse;
			}
		}
		else if ( !psNameSet )
		{
			Q_strncpyz( psName, token, sizeof( psName ) );

			//check for name space clashes
			for ( i = 0; i < numBaseParticleSystems; i++ )
			{
				if ( !Q_stricmp( baseParticleSystems[ i ].name, psName ) )
				{
					CG_Printf( S_ERROR "a particle system is already named %s\n", psName );
					break;
				}
			}

			if ( i < numBaseParticleSystems )
			{
				SkipBracedSection( &text_p );
				continue;
			}

			psNameSet = qtrue;
		}
		else
		{
			CG_Printf( S_ERROR "particle system already named\n" );
			return qfalse;
		}
	}

	return qtrue;
}

/*
===============
CG_LoadParticleSystems

Load particle systems from .particle files
===============
*/
void CG_LoadParticleSystems( void )
{
	int  i, j, numFiles, fileLen;
	char fileList[ MAX_PARTICLE_FILES * MAX_QPATH ];
	char fileName[ MAX_QPATH ];
	char *filePtr;

	//clear out the old
	numBaseParticleSystems = 0;
	numBaseParticleEjectors = 0;
	numBaseParticles = 0;

	for ( i = 0; i < MAX_BASEPARTICLE_SYSTEMS; i++ )
	{
		baseParticleSystem_t *bps = &baseParticleSystems[ i ];
		memset( bps, 0, sizeof( baseParticleSystem_t ) );
	}

	for ( i = 0; i < MAX_BASEPARTICLE_EJECTORS; i++ )
	{
		baseParticleEjector_t *bpe = &baseParticleEjectors[ i ];
		memset( bpe, 0, sizeof( baseParticleEjector_t ) );
	}

	for ( i = 0; i < MAX_BASEPARTICLES; i++ )
	{
		baseParticle_t *bp = &baseParticles[ i ];
		memset( bp, 0, sizeof( baseParticle_t ) );
	}

	//and bring in the new
	numFiles = trap_FS_GetFileList( "scripts", ".particle",
	                                fileList, MAX_PARTICLE_FILES * MAX_QPATH );
	filePtr = fileList;

	for ( i = 0; i < numFiles; i++, filePtr += fileLen + 1 )
	{
		fileLen = strlen( filePtr );
		strcpy( fileName, "scripts/" );
		strcat( fileName, filePtr );
		// CG_Printf(_( "...loading '%s'\n"), fileName );
		CG_ParseParticleFile( fileName );
	}

	//connect any child systems to their psHandle
	for ( i = 0; i < numBaseParticles; i++ )
	{
		baseParticle_t *bp = &baseParticles[ i ];

		if ( bp->childSystemName[ 0 ] )
		{
			//particle class has a child, resolve the name
			for ( j = 0; j < numBaseParticleSystems; j++ )
			{
				baseParticleSystem_t *bps = &baseParticleSystems[ j ];

				if ( !Q_stricmp( bps->name, bp->childSystemName ) )
				{
					//FIXME: add checks for cycles and infinite children

					bp->childSystemHandle = j + 1;

					break;
				}
			}

			if ( j == numBaseParticleSystems )
			{
				//couldn't find named particle system
				CG_Printf( S_WARNING "failed to find child %s\n", bp->childSystemName );
				bp->childSystemName[ 0 ] = '\0';
			}
		}

		if ( bp->onDeathSystemName[ 0 ] )
		{
			//particle class has a child, resolve the name
			for ( j = 0; j < numBaseParticleSystems; j++ )
			{
				baseParticleSystem_t *bps = &baseParticleSystems[ j ];

				if ( !Q_stricmp( bps->name, bp->onDeathSystemName ) )
				{
					//FIXME: add checks for cycles and infinite children

					bp->onDeathSystemHandle = j + 1;

					break;
				}
			}

			if ( j == numBaseParticleSystems )
			{
				//couldn't find named particle system
				CG_Printf( S_WARNING "failed to find onDeath system %s\n", bp->onDeathSystemName );
				bp->onDeathSystemName[ 0 ] = '\0';
			}
		}
	}
}

/*
===============
CG_SetParticleSystemNormal
===============
*/
void CG_SetParticleSystemNormal( particleSystem_t *ps, vec3_t normal )
{
	if ( ps == NULL || !ps->valid )
	{
		CG_Printf( S_WARNING "tried to modify a NULL particle system\n" );
		return;
	}

	ps->normalValid = qtrue;
	VectorCopy( normal, ps->normal );
	VectorNormalize( ps->normal );

	CG_SetParticleSystemLastNormal( ps, normal );
}

/*
===============
CG_SetParticleSystemLastNormal
===============
*/
void CG_SetParticleSystemLastNormal( particleSystem_t *ps, const vec3_t normal )
{
	if ( ps == NULL || !ps->valid )
	{
		CG_Printf( S_WARNING "tried to modify a NULL particle system\n" );
		return;
	}

	if ( normal )
	{
		ps->lastNormalIsCurrent = qtrue;
		VectorCopy( normal, ps->lastNormal );
		VectorNormalize( ps->lastNormal );
	}
	else
		ps->lastNormalIsCurrent = qfalse;
}

/*
===============
CG_DestroyParticleSystem

Destroy a particle system

This doesn't actually invalidate anything, it just stops
particle ejectors from producing new particles so the
garbage collector will eventually remove this system.
However is does set the pointer to NULL so the user is
unable to manipulate this particle system any longer.
===============
*/
void CG_DestroyParticleSystem( particleSystem_t **ps )
{
	int               i;
	particleEjector_t *pe;

	if ( *ps == NULL || !( *ps )->valid )
	{
		CG_Printf( S_WARNING "tried to destroy a NULL particle system\n" );
		return;
	}

	if ( cg_debugParticles.integer >= 1 )
	{
		CG_Printf( "PS destroyed\n" );
	}

	for ( i = 0; i < MAX_PARTICLE_EJECTORS; i++ )
	{
		pe = &particleEjectors[ i ];

		if ( pe->valid && pe->parent == *ps )
		{
			pe->totalParticles = pe->count = 0;
		}
	}

	*ps = NULL;
}

/*
===============
CG_IsParticleSystemInfinite

Test a particle system for 'count infinite' ejectors
===============
*/
qboolean CG_IsParticleSystemInfinite( particleSystem_t *ps )
{
	int               i;
	particleEjector_t *pe;

	if ( ps == NULL )
	{
		CG_Printf( S_WARNING "tried to test a NULL particle system\n" );
		return qfalse;
	}

	if ( !ps->valid )
	{
		CG_Printf( S_WARNING "tried to test an invalid particle system\n" );
		return qfalse;
	}

	//don't bother checking already invalid systems
	if ( !ps->valid )
	{
		return qfalse;
	}

	for ( i = 0; i < MAX_PARTICLE_EJECTORS; i++ )
	{
		pe = &particleEjectors[ i ];

		if ( pe->valid && pe->parent == ps )
		{
			if ( pe->totalParticles == PARTICLES_INFINITE )
			{
				return qtrue;
			}
		}
	}

	return qfalse;
}

/*
===============
CG_IsParticleSystemValid

Test a particle system for validity
===============
*/
qboolean CG_IsParticleSystemValid( particleSystem_t **ps )
{
	if ( *ps == NULL || ( *ps && !( *ps )->valid ) )
	{
		if ( *ps && !( *ps )->valid )
		{
			*ps = NULL;
		}

		return qfalse;
	}

	return qtrue;
}

/*
===============
CG_GarbageCollectParticleSystems

Destroy inactive particle systems
===============
*/
static void CG_GarbageCollectParticleSystems( void )
{
	int               i, j, count;
	particleSystem_t  *ps;
	particleEjector_t *pe;
	int               centNum;

	for ( i = 0; i < MAX_PARTICLE_SYSTEMS; i++ )
	{
		ps = &particleSystems[ i ];
		count = 0;

		//don't bother checking already invalid systems
		if ( !ps->valid )
		{
			continue;
		}

		for ( j = 0; j < MAX_PARTICLE_EJECTORS; j++ )
		{
			pe = &particleEjectors[ j ];

			if ( pe->valid && pe->parent == ps )
			{
				count++;
			}
		}

		if ( !count )
		{
			ps->valid = qfalse;
		}

		//check systems where the parent cent has left the PVS
		//( local player entity is always valid )
		if ( ( centNum = CG_AttachmentCentNum( &ps->attachment ) ) >= 0 &&
		     centNum != cg.snap->ps.clientNum )
		{
			if ( !cg_entities[ centNum ].valid )
			{
				ps->lazyRemove = qtrue;
			}
		}

		if ( cg_debugParticles.integer >= 1 && !ps->valid )
		{
			CG_Printf( "PS %s garbage collected\n", ps->class_->name );
		}
	}
}

/*
===============
CG_CalculateTimeFrac

Calculate the fraction of time passed
===============
*/
static float CG_CalculateTimeFrac( int birth, int life, int delay )
{
	float frac;

	frac = ( ( float ) cg.time - ( float )( birth + delay ) ) / ( float )( life - delay );

	if ( frac < 0.0f )
	{
		frac = 0.0f;
	}
	else if ( frac > 1.0f )
	{
		frac = 1.0f;
	}

	return frac;
}

/*
===============
CG_EvaluateParticlePhysics

Compute the physics on a specific particle
===============
*/
static void CG_EvaluateParticlePhysics( particle_t *p )
{
	particleSystem_t *ps = p->parent->parent;
	baseParticle_t   *bp = p->class_;
	vec3_t           acceleration, newOrigin;
	vec3_t           mins, maxs;
	float            deltaTime, bounce, radius, dot;
	trace_t          trace;
	vec3_t           transform[ 3 ];

	if ( p->atRest )
	{
		VectorClear( p->velocity );
		return;
	}

	switch ( bp->accMoveType )
	{
		case PMT_STATIC:
			if ( bp->accMoveValues.dirType == PMD_POINT )
			{
				VectorSubtract( bp->accMoveValues.point, p->origin, acceleration );
			}
			else if ( bp->accMoveValues.dirType == PMD_LINEAR )
			{
				VectorCopy( bp->accMoveValues.dir, acceleration );
			}

			break;

		case PMT_STATIC_TRANSFORM:
			if ( !CG_AttachmentAxis( &ps->attachment, transform ) )
			{
				return;
			}

			if ( bp->accMoveValues.dirType == PMD_POINT )
			{
				vec3_t transPoint;

				VectorMatrixMultiply( bp->accMoveValues.point, transform, transPoint );
				VectorSubtract( transPoint, p->origin, acceleration );
			}
			else if ( bp->accMoveValues.dirType == PMD_LINEAR )
			{
				VectorMatrixMultiply( bp->accMoveValues.dir, transform, acceleration );
			}

			break;

		case PMT_TAG:
		case PMT_CENT_ANGLES:
			if ( bp->accMoveValues.dirType == PMD_POINT )
			{
				vec3_t point;

				if ( !CG_AttachmentPoint( &ps->attachment, point ) )
				{
					return;
				}

				VectorSubtract( point, p->origin, acceleration );
			}
			else if ( bp->accMoveValues.dirType == PMD_LINEAR )
			{
				if ( !CG_AttachmentDir( &ps->attachment, acceleration ) )
				{
					return;
				}
			}

			break;

		case PMT_NORMAL:
			if ( !ps->normalValid )
			{
				return;
			}

			VectorCopy( ps->normal, acceleration );

			break;

		case PMT_LAST_NORMAL:
			VectorCopy( ps->lastNormal, acceleration );
			break;

		case PMT_OPPORTUNISTIC_NORMAL:
			if ( ps->lastNormalIsCurrent )
				VectorCopy( ps->lastNormal, acceleration );
			else
				VectorClear( acceleration );
			break;
		default:
			VectorClear( acceleration );
			break;
	}

#define MAX_ACC_RADIUS 1000.0f

	if ( bp->accMoveValues.dirType == PMD_POINT )
	{
		//FIXME: so this fall off is a bit... odd -- it works..
		float r2 = DotProduct( acceleration, acceleration );  // = radius^2
		float scale = ( MAX_ACC_RADIUS - r2 ) / MAX_ACC_RADIUS;

		if ( scale > 1.0f )
		{
			scale = 1.0f;
		}
		else if ( scale < 0.1f )
		{
			scale = 0.1f;
		}

		scale *= CG_RandomiseValue( bp->accMoveValues.mag, bp->accMoveValues.magRandFrac );

		VectorNormalize( acceleration );
		CG_SpreadVector( acceleration, bp->accMoveValues.dirRandAngle );
		VectorScale( acceleration, scale, acceleration );
	}
	else if ( bp->accMoveValues.dirType == PMD_LINEAR )
	{
		VectorNormalize( acceleration );
		CG_SpreadVector( acceleration, bp->accMoveValues.dirRandAngle );
		VectorScale( acceleration,
		             CG_RandomiseValue( bp->accMoveValues.mag, bp->accMoveValues.magRandFrac ),
		             acceleration );
	}

	// Some particles have a visual radius that differs from their collision radius
	if ( bp->physicsRadius )
	{
		radius = bp->physicsRadius;
	}
	else
	{
		radius = CG_LerpValues( p->radius.initial, p->radius.final,
		                        CG_CalculateTimeFrac( p->birthTime, p->lifeTime,
		                            p->radius.delay ) );
	}

	VectorSet( mins, -radius, -radius, -radius );
	VectorSet( maxs, radius, radius, radius );

	bounce = CG_RandomiseValue( bp->bounceFrac, bp->bounceFracRandFrac );

	deltaTime = ( float )( cg.time - p->lastEvalTime ) * 0.001;
	VectorMA( p->velocity, deltaTime, acceleration, p->velocity );
	VectorMA( p->origin, deltaTime, p->velocity, newOrigin );
	p->lastEvalTime = cg.time;

	// we're not doing particle physics, but at least cull them in solids
	if ( !cg_bounceParticles.integer )
	{
		int contents = trap_CM_PointContents( newOrigin, 0 );

		if ( ( contents & CONTENTS_SOLID ) || ( contents & CONTENTS_NODROP ) )
		{
			CG_DestroyParticle( p, NULL );
		}
		else
		{
			VectorCopy( newOrigin, p->origin );
		}

		return;
	}

	CG_Trace( &trace, p->origin, mins, maxs, newOrigin,
	          CG_AttachmentCentNum( &ps->attachment ), CONTENTS_SOLID );

	//not hit anything or not a collider
	if ( trace.fraction == 1.0f || bounce == 0.0f )
	{
		VectorCopy( newOrigin, p->origin );
		if ( CG_IsParticleSystemValid( &p->childParticleSystem ) )
			CG_SetParticleSystemLastNormal( p->childParticleSystem, NULL );
		return;
	}

	//remove particles that get into a CONTENTS_NODROP brush
	if ( ( trap_CM_PointContents( trace.endpos, 0 ) & CONTENTS_NODROP ) ||
	     ( bp->cullOnStartSolid && trace.startsolid ) )
	{
		CG_DestroyParticle( p, NULL );
		return;
	}
	else if ( bp->bounceCull )
	{
		CG_DestroyParticle( p, trace.plane.normal );
		return;
	}

	//reflect the velocity on the trace plane
	dot = DotProduct( p->velocity, trace.plane.normal );
	VectorMA( p->velocity, -2.0f * dot, trace.plane.normal, p->velocity );

	VectorScale( p->velocity, bounce, p->velocity );

	if ( trace.plane.normal[ 2 ] > 0.5f &&
	     ( p->velocity[ 2 ] < 40.0f ||
	       p->velocity[ 2 ] < -cg.frametime * p->velocity[ 2 ] ) )
	{
		p->atRest = qtrue;
	}

	if ( bp->bounceMarkName[ 0 ] && p->bounceMarkCount > 0 )
	{
		CG_ImpactMark( bp->bounceMark, trace.endpos, trace.plane.normal,
		               random() * 360, 1, 1, 1, 1, qtrue, bp->bounceMarkRadius, qfalse );
		p->bounceMarkCount--;
	}

	if ( bp->bounceSoundName[ 0 ] && p->bounceSoundCount > 0 )
	{
		trap_S_StartSound( trace.endpos, ENTITYNUM_WORLD, CHAN_AUTO, bp->bounceSound );
		p->bounceSoundCount--;
	}

	VectorCopy( trace.endpos, p->origin );

	if ( !trace.allsolid )
	{
		if ( CG_IsParticleSystemValid( &p->childParticleSystem ) )
			CG_SetParticleSystemLastNormal( p->childParticleSystem, trace.plane.normal );
	}
}

#define GETKEY(x,y) ((( x ) >> y ) & 0xFF )

/*
===============
CG_Radix
===============
*/
static void CG_Radix( int bits, int size, particle_t **source, particle_t **dest )
{
	int count[ 256 ];
	int index[ 256 ];
	int i;

	memset( count, 0, sizeof( count ) );

	for ( i = 0; i < size; i++ )
	{
		count[ GETKEY( source[ i ]->sortKey, bits ) ]++;
	}

	index[ 0 ] = 0;

	for ( i = 1; i < 256; i++ )
	{
		index[ i ] = index[ i - 1 ] + count[ i - 1 ];
	}

	for ( i = 0; i < size; i++ )
	{
		dest[ index[ GETKEY( source[ i ]->sortKey, bits ) ]++ ] = source[ i ];
	}
}

/*
===============
CG_RadixSort

Radix sort with 4 byte size buckets
===============
*/
static void CG_RadixSort( particle_t **source, particle_t **temp, int size )
{
	CG_Radix( 0,   size, source, temp );
	CG_Radix( 8,   size, temp, source );
	CG_Radix( 16,  size, source, temp );
	CG_Radix( 24,  size, temp, source );
}

/*
===============
CG_CompactAndSortParticles

Depth sort the particles
===============
*/
static void CG_CompactAndSortParticles( void )
{
	int    i, j = 0;
	int    numParticles;
	vec3_t delta;

	for ( i = 0; i < MAX_PARTICLES; i++ )
	{
		sortedParticles[ i ] = &particles[ i ];
	}

	if ( !cg_depthSortParticles.integer )
	{
		return;
	}

	for ( i = MAX_PARTICLES - 1; i >= 0; i-- )
	{
		if ( sortedParticles[ i ]->valid )
		{
			//find the first hole
			while ( j < MAX_PARTICLES && sortedParticles[ j ]->valid )
			{
				j++;
			}

			//no more holes
			if ( j >= i )
			{
				break;
			}

			sortedParticles[ j ] = sortedParticles[ i ];
		}
	}

	numParticles = i;

	//set sort keys
	for ( i = 0; i < numParticles; i++ )
	{
		VectorSubtract( sortedParticles[ i ]->origin, cg.refdef.vieworg, delta );
		sortedParticles[ i ]->sortKey = ( int ) DotProduct( delta, delta );
	}

	CG_RadixSort( sortedParticles, radixBuffer, numParticles );

	//FIXME: wtf?
	//reverse order of particles array
	for ( i = 0; i < numParticles; i++ )
	{
		radixBuffer[ i ] = sortedParticles[ numParticles - i - 1 ];
	}

	for ( i = 0; i < numParticles; i++ )
	{
		sortedParticles[ i ] = radixBuffer[ i ];
	}
}

/*
===============
CG_RenderParticle

Actually render a particle
===============
*/
static void CG_RenderParticle( particle_t *p )
{
	refEntity_t          re;
	float                timeFrac, scale;
	int                  index;
	baseParticle_t       *bp = p->class_;
	particleSystem_t     *ps = p->parent->parent;
	baseParticleSystem_t *bps = ps->class_;
	vec3_t               alight, dlight, lightdir;
	int                  i;
	vec3_t               up = { 0.0f, 0.0f, 1.0f };

	memset( &re, 0, sizeof( refEntity_t ) );

	timeFrac = CG_CalculateTimeFrac( p->birthTime, p->lifeTime, 0 );

	scale = CG_LerpValues( p->radius.initial,
	                       p->radius.final,
	                       CG_CalculateTimeFrac( p->birthTime,
	                           p->lifeTime,
	                           p->radius.delay ) );

	re.shaderTime = p->birthTime / 1000.0f;

	if ( bp->numFrames ) //shader based
	{
		re.reType = RT_SPRITE;

		//apply environmental lighting to the particle
		if ( bp->realLight )
		{
			trap_R_LightForPoint( p->origin, alight, dlight, lightdir );

			for ( i = 0; i <= 2; i++ )
			{
				re.shaderRGBA[ i ] = ( byte ) alight[ i ];
			}
		}
		else
		{
			vec3_t colorRange;

			VectorSubtract( bp->finalColor,
			                bp->initialColor, colorRange );

			VectorMA( bp->initialColor,
			          CG_CalculateTimeFrac( p->birthTime,
			                                p->lifeTime,
			                                p->colorDelay ),
			          colorRange, re.shaderRGBA );
		}

		re.shaderRGBA[ 3 ] = ( byte )( ( float ) 0xFF *
		                               CG_LerpValues( p->alpha.initial,
		                                   p->alpha.final,
		                                   CG_CalculateTimeFrac( p->birthTime,
		                                       p->lifeTime,
		                                       p->alpha.delay ) ) );

		re.radius = scale;

		re.rotation = CG_LerpValues( p->rotation.initial,
		                             p->rotation.final,
		                             CG_CalculateTimeFrac( p->birthTime,
		                                 p->lifeTime,
		                                 p->rotation.delay ) );

		// if the view would be "inside" the sprite, kill the sprite
		// so it doesn't add too much overdraw
		if ( Distance( p->origin, cg.refdef.vieworg ) < re.radius && bp->overdrawProtection )
		{
			return;
		}

		if ( bp->framerate == 0.0f )
		{
			//sync animation time to lifeTime of particle
			index = ( int )( timeFrac * ( bp->numFrames + 1 ) );

			if ( index >= bp->numFrames )
			{
				index = bp->numFrames - 1;
			}

			re.customShader = bp->shaders[ index ];
		}
		else
		{
			//looping animation
			index = ( int )( bp->framerate * timeFrac * p->lifeTime * 0.001 ) % bp->numFrames;
			re.customShader = bp->shaders[ index ];
		}
	}
	else if ( bp->numModels ) //model based
	{
		re.reType = RT_MODEL;

		re.hModel = p->model;

		if ( p->atRest )
		{
			AxisCopy( p->lastAxis, re.axis );
		}
		else
		{
			// convert direction of travel into axis
			VectorNormalize2( p->velocity, re.axis[ 0 ] );

			if ( re.axis[ 0 ][ 0 ] == 0.0f && re.axis[ 0 ][ 1 ] == 0.0f )
			{
				AxisCopy( axisDefault, re.axis );
			}
			else
			{
				ProjectPointOnPlane( re.axis[ 2 ], up, re.axis[ 0 ] );
				VectorNormalize( re.axis[ 2 ] );
				CrossProduct( re.axis[ 2 ], re.axis[ 0 ], re.axis[ 1 ] );
			}

			AxisCopy( re.axis, p->lastAxis );
		}

		if ( scale != 1.0f )
		{
			VectorScale( re.axis[ 0 ], scale, re.axis[ 0 ] );
			VectorScale( re.axis[ 1 ], scale, re.axis[ 1 ] );
			VectorScale( re.axis[ 2 ], scale, re.axis[ 2 ] );
			re.nonNormalizedAxes = qtrue;
		}
		else
		{
			re.nonNormalizedAxes = qfalse;
		}

		p->lf.animation = &bp->modelAnimation;

		//run animation
		CG_RunLerpFrame( &p->lf, 1.0f );

		re.oldframe = p->lf.oldFrame;
		re.frame = p->lf.frame;
		re.backlerp = p->lf.backlerp;
	}

	if ( bps->thirdPersonOnly &&
	     CG_AttachmentCentNum( &ps->attachment ) == cg.snap->ps.clientNum &&
	     !cg.renderingThirdPerson )
	{
		re.renderfx |= RF_THIRD_PERSON;
	}

	if ( bp->dynamicLight && !( re.renderfx & RF_THIRD_PERSON ) )
	{
		trap_R_AddLightToScene( p->origin,
		                        CG_LerpValues( p->dLightRadius.initial, p->dLightRadius.final,
		                            CG_CalculateTimeFrac( p->birthTime, p->lifeTime, p->dLightRadius.delay ) ),
		                        3,
		                        ( float ) bp->dLightColor[ 0 ] / ( float ) 0xFF,
		                        ( float ) bp->dLightColor[ 1 ] / ( float ) 0xFF,
		                        ( float ) bp->dLightColor[ 2 ] / ( float ) 0xFF, 0, 0 );
	}

	VectorCopy( p->origin, re.origin );

	trap_R_AddRefEntityToScene( &re );
}

/*
===============
CG_AddParticles

Add particles to the scene
===============
*/
void CG_AddParticles( void )
{
	int        i;
	particle_t *p;
	int        numPS = 0, numPE = 0, numP = 0;

	//remove expired particle systems
	CG_GarbageCollectParticleSystems();

	//check each ejector and introduce any new particles
	CG_SpawnNewParticles();

	//sorting
	CG_CompactAndSortParticles();

	for ( i = 0; i < MAX_PARTICLES; i++ )
	{
		p = sortedParticles[ i ];

		if ( p->valid )
		{
			if ( p->birthTime + p->lifeTime > cg.time )
			{
				//particle is active
				CG_EvaluateParticlePhysics( p );
				CG_RenderParticle( p );
			}
			else
			{
				CG_DestroyParticle( p, NULL );
			}
		}
	}

	if ( cg_debugParticles.integer >= 2 )
	{
		for ( i = 0; i < MAX_PARTICLE_SYSTEMS; i++ )
		{
			if ( particleSystems[ i ].valid )
			{
				numPS++;
			}
		}

		for ( i = 0; i < MAX_PARTICLE_EJECTORS; i++ )
		{
			if ( particleEjectors[ i ].valid )
			{
				numPE++;
			}
		}

		for ( i = 0; i < MAX_PARTICLES; i++ )
		{
			if ( particles[ i ].valid )
			{
				numP++;
			}
		}

		CG_Printf( "PS: %d  PE: %d  P: %d\n", numPS, numPE, numP );
	}
}

/*
===============
CG_ParticleSystemEntity

Particle system entity client code
===============
*/
void CG_ParticleSystemEntity( centity_t *cent )
{
	entityState_t *es;

	es = &cent->currentState;

	if ( es->eFlags & EF_NODRAW )
	{
		if ( CG_IsParticleSystemValid( &cent->entityPS ) && CG_IsParticleSystemInfinite( cent->entityPS ) )
		{
			CG_DestroyParticleSystem( &cent->entityPS );
		}

		return;
	}

	if ( !CG_IsParticleSystemValid( &cent->entityPS ) && !cent->entityPSMissing )
	{
		cent->entityPS = CG_SpawnNewParticleSystem( cgs.gameParticleSystems[ es->modelindex ] );

		if ( CG_IsParticleSystemValid( &cent->entityPS ) )
		{
			CG_SetAttachmentCent( &cent->entityPS->attachment, cent );
			CG_AttachToCent( &cent->entityPS->attachment );
		}
		else
		{
			cent->entityPSMissing = qtrue;
		}
	}
}

static particleSystem_t *testPS;
static qhandle_t        testPSHandle;

/*
===============
CG_DestroyTestPS_f

Destroy the test a particle system
===============
*/
void CG_DestroyTestPS_f( void )
{
	if ( CG_IsParticleSystemValid( &testPS ) )
	{
		CG_DestroyParticleSystem( &testPS );
	}
}

/*
===============
CG_TestPS_f

Test a particle system
===============
*/
void CG_TestPS_f( void )
{
	vec3_t origin;
	vec3_t up = { 0.0f, 0.0f, 1.0f };
	char   psName[ MAX_QPATH ];

	if ( trap_Argc() < 2 )
	{
		return;
	}

	Q_strncpyz( psName, CG_Argv( 1 ), MAX_QPATH );
	testPSHandle = CG_RegisterParticleSystem( psName );

	if ( testPSHandle )
	{
		CG_DestroyTestPS_f();

		testPS = CG_SpawnNewParticleSystem( testPSHandle );

		VectorMA( cg.refdef.vieworg, 100, cg.refdef.viewaxis[ 0 ], origin );

		if ( CG_IsParticleSystemValid( &testPS ) )
		{
			CG_SetAttachmentPoint( &testPS->attachment, origin );
			CG_SetParticleSystemNormal( testPS, up );
			CG_AttachToPoint( &testPS->attachment );
		}
	}
}
