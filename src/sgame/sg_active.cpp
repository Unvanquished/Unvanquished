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

#include "sg_local.h"
#include "CBSE.h"

bool ClientInactivityTimer( gentity_t *ent, bool active );

/*
===============
G_DamageFeedback

Called just before a snapshot is sent to the given player.
Totals up all damage and generates both the player_state_t
damage values to that client for pain blends and kicks, and
global pain sound events for all clients.
===============
*/
void P_DamageFeedback( gentity_t *player )
{
	gclient_t *client;
	float     count;
	vec3_t    angles;

	client = player->client;

	if ( !PM_Live( client->ps.pm_type ) )
	{
		return;
	}

	// total points of damage shot at the player this frame
	count = client->damage_received;

	if ( count == 0 )
	{
		return; // didn't take any damage
	}

	if ( count > 255 )
	{
		count = 255;
	}

	// send the information to the client

	// world damage (falling, slime, etc) uses a special code
	// to make the blend blob centered instead of positional
	if ( client->damage_fromWorld )
	{
		client->ps.damagePitch = 255;
		client->ps.damageYaw = 255;

		client->damage_fromWorld = false;
	}
	else
	{
		vectoangles( client->damage_from, angles );
		client->ps.damagePitch = angles[ PITCH ] / 360.0 * 256;
		client->ps.damageYaw = angles[ YAW ] / 360.0 * 256;
	}

	// play an appropriate pain sound
	if ( ( level.time > player->pain_debounce_time ) && !( player->flags & FL_GODMODE ) )
	{
		player->pain_debounce_time = level.time + 700;
		int transmittedHalth = (int)std::ceil(player->entity->Get<HealthComponent>()->Health());
		transmittedHalth = Math::Clamp(transmittedHalth, 0, 255);
		G_AddEvent( player, EV_PAIN, transmittedHalth );
		client->ps.damageEvent++;
	}

	client->ps.damageCount = count;

	//
	// clear totals
	//
	client->damage_received = 0;
}

/*
=============
P_WorldEffects

Check for lava / slime contents and drowning
=============
*/
void P_WorldEffects( gentity_t *ent )
{
	int waterlevel;

	if ( ent->client->noclip )
	{
		ent->client->airOutTime = level.time + 12000; // don't need air
		return;
	}

	waterlevel = ent->waterlevel;

	//
	// check for drowning
	//
	if ( waterlevel == 3 )
	{
		// if out of air, start drowning
		if ( ent->client->airOutTime < level.time )
		{
			// drown!
			ent->client->airOutTime += 1000;

			if ( G_Alive( ent ) )
			{
				// take more damage the longer underwater
				ent->damage += 2;

				if ( ent->damage > 15 )
				{
					ent->damage = 15;
				}

				// play a gurp sound instead of a general pain sound
				if ( ent->entity->Get<HealthComponent>()->Health() <= (float)ent->damage )
				{
					G_Sound( ent, CHAN_VOICE, G_SoundIndex( "*drown.wav" ) );
				}
				else if ( rand() < RAND_MAX / 2 + 1 )
				{
					G_Sound( ent, CHAN_VOICE, G_SoundIndex( "sound/player/gurp1.wav" ) );
				}
				else
				{
					G_Sound( ent, CHAN_VOICE, G_SoundIndex( "sound/player/gurp2.wav" ) );
				}

				// don't play a general pain sound
				ent->pain_debounce_time = level.time + 200;

				ent->entity->Damage((float)ent->damage, nullptr, Util::nullopt, Util::nullopt,
				                    DAMAGE_PURE, MOD_WATER);
			}
		}
	}
	else
	{
		ent->client->airOutTime = level.time + 12000;
		ent->damage = 2;
	}

	//
	// check for sizzle damage (move to pmove?)
	//
	if ( waterlevel &&
	     ( ent->watertype & ( CONTENTS_LAVA | CONTENTS_SLIME ) ) )
	{
		if ( G_Alive( ent ) && ent->pain_debounce_time <= level.time )
		{
			if ( ent->watertype & CONTENTS_LAVA )
			{
				ent->entity->Damage(30.0f * waterlevel, nullptr, Util::nullopt, Util::nullopt, 0, MOD_LAVA);
			}

			if ( ent->watertype & CONTENTS_SLIME )
			{
				ent->entity->Damage(10.0f * waterlevel, nullptr, Util::nullopt, Util::nullopt, 0, MOD_SLIME);
			}
		}
	}
}

/*
===============
G_SetClientSound
===============
*/
void G_SetClientSound( gentity_t *ent )
{
	if ( ent->waterlevel && ( ent->watertype & ( CONTENTS_LAVA | CONTENTS_SLIME ) ) )
	{
		ent->client->ps.loopSound = level.snd_fry;
	}
	else
	{
		ent->client->ps.loopSound = 0;
	}
}

//==============================================================

/*
==============
GetClientMass

TODO: Define player class masses in config files
==============
*/
static int GetClientMass( gentity_t *ent )
{
	int entMass = 100;

	if ( ent->client->pers.team == TEAM_ALIENS )
	{
		entMass = BG_Class( ent->client->pers.classSelection )->health;
	}
	else if ( ent->client->pers.team == TEAM_HUMANS )
	{
		if ( BG_InventoryContainsUpgrade( UP_BATTLESUIT, ent->client->ps.stats ) )
		{
			entMass *= 2;
		}
	}
	else
	{
		return 0;
	}

	return entMass;
}

/*
==============
ClientShove
==============
*/
static void ClientShove( gentity_t *ent, gentity_t *victim )
{
	vec3_t dir, push;
	float  force;
	int    entMass, vicMass;

	// Don't push if the entity is not trying to move
	if ( !ent->client->pers.cmd.rightmove && !ent->client->pers.cmd.forwardmove &&
	     !ent->client->pers.cmd.upmove )
	{
		return;
	}

	// Cannot push enemy players unless they are walking on the player
	if ( !G_OnSameTeam( ent, victim ) &&
	     victim->client->ps.groundEntityNum != ent - g_entities )
	{
		return;
	}

	// Shove force is scaled by relative mass
	entMass = GetClientMass( ent );
	vicMass = GetClientMass( victim );

	if ( vicMass <= 0 || entMass <= 0 )
	{
		return;
	}

	force = g_shove.value * entMass / vicMass;

	if ( force < 0 )
	{
		force = 0;
	}

	if ( force > 150 )
	{
		force = 150;
	}

	// Give the victim some shove velocity
	VectorSubtract( victim->r.currentOrigin, ent->r.currentOrigin, dir );
	VectorNormalizeFast( dir );
	VectorScale( dir, force, push );
	VectorAdd( victim->client->ps.velocity, push, victim->client->ps.velocity );

	// Set the pmove timer so that the other client can't cancel
	// out the movement immediately
	if ( !victim->client->ps.pm_time )
	{
		int time;

		time = force * 2 + 0.5f;

		if ( time < 50 )
		{
			time = 50;
		}

		if ( time > 200 )
		{
			time = 200;
		}

		victim->client->ps.pm_time = time;
		victim->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
	}
}
void PushBot(gentity_t * ent, gentity_t * other)
{
	vec3_t dir, ang, f, r;
	float oldspeed;

	oldspeed = VectorLength(other->client->ps.velocity);
	if(oldspeed < 200)
	{
		oldspeed = 200;
	}

	VectorSubtract(other->r.currentOrigin, ent->r.currentOrigin, dir);
	VectorNormalize(dir);
	vectoangles(dir, ang);
	AngleVectors(ang, f, r, nullptr);
	f[2] = 0;
	r[2] = 0;

	VectorMA(other->client->ps.velocity, 200, f, other->client->ps.velocity);
	VectorMA(other->client->ps.velocity, 100 * ((level.time + (ent->s.number * 1000)) % 4000 < 2000 ? 1.0 : -1.0), r,
	other->client->ps.velocity);

	if(VectorLengthSquared(other->client->ps.velocity) > oldspeed * oldspeed)
	{
		VectorNormalize(other->client->ps.velocity);
		VectorScale(other->client->ps.velocity, oldspeed, other->client->ps.velocity);
	}
}
/*
==============
ClientImpacts
==============
*/
void ClientImpacts( gentity_t *ent, pmove_t *pm )
{
	int       i;
	trace_t   trace;
	gentity_t *other;

	if( !ent || !ent->client )
	{
		return;
	}

	// clear a fake trace struct for touch function
	memset( &trace, 0, sizeof( trace ) );

	for ( i = 0; i < pm->numtouch; i++ )
	{
		other = &g_entities[ pm->touchents[ i ] ];

		// see G_UnlaggedDetectCollisions(), this is the inverse of that.
		// if our movement is blocked by another player's real position,
		// don't use the unlagged position for them because they are
		// blocking or server-side Pmove() from reaching it
		if ( other->client && other->client->unlaggedCalc.used )
		{
			other->client->unlaggedCalc.used = false;
		}

		// deal impact and weight damage
		G_ImpactAttack( ent, other );
		G_WeightAttack( ent, other );

		// tyrant trample
		if ( ent->client->ps.weapon == WP_ALEVEL4 )
		{
			G_ChargeAttack( ent, other );
		}

		// shove players
		if ( other->client )
		{
			ClientShove( ent, other );

			//bot should get pushed out the way
			if( (other->r.svFlags & SVF_BOT) && ent->client->pers.team == other->client->pers.team)
			{
				PushBot(ent, other);
			}

			// if we are standing on their head, then we should be pushed also
			if( (ent->r.svFlags & SVF_BOT) && ent->s.groundEntityNum == other->s.number &&
			    other->client && ent->client->pers.team == other->client->pers.team)
			{
				PushBot(other, ent);
			}
		}

		// touch triggers
		if ( other->touch )
		{
			other->touch( other, ent, &trace );
		}
	}
}

/*
============
G_TouchTriggers

Find all trigger entities that ent's current position touches.
Spectators will only interact with teleporters.
============
*/
void  G_TouchTriggers( gentity_t *ent )
{
	int              i, num;
	int              touch[ MAX_GENTITIES ];
	gentity_t        *hit;
	trace_t          trace;
	vec3_t           mins, maxs;
	vec3_t           pmins, pmaxs;
	static const     vec3_t range = { 10, 10, 10 };

	if ( !ent->client )
	{
		return;
	}

	// noclipping clients don't activate triggers!
	if ( ent->client->noclip )
	{
		return;
	}

	// dead clients don't activate triggers!
	if ( G_Dead( ent ) )
	{
		return;
	}

	BG_ClassBoundingBox( ent->client->ps.stats[ STAT_CLASS ],
	                     pmins, pmaxs, nullptr, nullptr, nullptr );

	VectorAdd( ent->client->ps.origin, pmins, mins );
	VectorAdd( ent->client->ps.origin, pmaxs, maxs );

	VectorSubtract( mins, range, mins );
	VectorAdd( maxs, range, maxs );

	num = trap_EntitiesInBox( mins, maxs, touch, MAX_GENTITIES );

	// can't use ent->absmin, because that has a one unit pad
	VectorAdd( ent->client->ps.origin, ent->r.mins, mins );
	VectorAdd( ent->client->ps.origin, ent->r.maxs, maxs );

	for ( i = 0; i < num; i++ )
	{
		hit = &g_entities[ touch[ i ] ];

		if ( !hit->touch && !ent->touch )
		{
			continue;
		}

		if ( !( hit->r.contents & CONTENTS_SENSOR ) )
		{
			continue;
		}

		if ( !hit->enabled )
		{
			continue;
		}

		// ignore most entities if a spectator
		if ( ent->client->sess.spectatorState != SPECTATOR_NOT && hit->s.eType != ET_TELEPORTER )
		{
			continue;
		}

		if ( !trap_EntityContact( mins, maxs, hit ) )
		{
			continue;
		}

		memset( &trace, 0, sizeof( trace ) );

		if ( hit->touch )
		{
			hit->touch( hit, ent, &trace );
		}
	}
}

/*
=================
SpectatorThink
=================
*/
void SpectatorThink( gentity_t *ent, usercmd_t *ucmd )
{
	pmove_t   pm;
	gclient_t *client;
	int       clientNum;
	bool  attack1, following, queued, attackReleased;
	team_t    team;

	client = ent->client;

	usercmdCopyButtons( client->oldbuttons, client->buttons );
	usercmdCopyButtons( client->buttons, ucmd->buttons );

	attack1 = usercmdButtonPressed( client->buttons, BUTTON_ATTACK ) &&
	          !usercmdButtonPressed( client->oldbuttons, BUTTON_ATTACK );

	attackReleased = !usercmdButtonPressed( client->buttons, BUTTON_ATTACK ) &&
		  usercmdButtonPressed( client->oldbuttons, BUTTON_ATTACK );

	//if bot
	if( ent->r.svFlags & SVF_BOT ) {
		G_BotSpectatorThink( ent );
		return;
	}

	// We are in following mode only if we are following a non-spectating client
	following = client->sess.spectatorState == SPECTATOR_FOLLOW;

	if ( following )
	{
		clientNum = client->sess.spectatorClient;

		if ( clientNum < 0 || clientNum > level.maxclients ||
		     !g_entities[ clientNum ].client ||
		     g_entities[ clientNum ].client->sess.spectatorState != SPECTATOR_NOT )
		{
			following = false;
		}
	}

	team = (team_t) client->pers.team;

	// Check to see if we are in the spawn queue
	// Also, do some other checks and updates which players need while spectating
	if ( team == TEAM_ALIENS || team == TEAM_HUMANS )
	{
		client->ps.persistant[ PERS_UNLOCKABLES ] = BG_UnlockablesMask( client->pers.team );
		queued = G_SearchSpawnQueue( &level.team[ team ].spawnQueue, ent - g_entities );

		if ( !ClientInactivityTimer( ent, queued || !level.team[ team ].numSpawns ) )
		{
			return;
		}
	}
	else
	{
		queued = false;
	}

	// Wants to get out of spawn queue
	if ( attack1 && queued )
	{
		if ( client->sess.spectatorState == SPECTATOR_FOLLOW )
		{
			G_StopFollowing( ent );
		}

		//be sure that only valid team "numbers" can be used.
		assert(team == TEAM_ALIENS || team == TEAM_HUMANS);
		G_RemoveFromSpawnQueue( &level.team[ team ].spawnQueue, client->ps.clientNum );

		client->pers.classSelection = PCL_NONE;
		client->pers.humanItemSelection = WP_NONE;
		client->ps.stats[ STAT_CLASS ] = PCL_NONE;
		client->ps.pm_flags &= ~PMF_QUEUED;
		queued = false;
	}
	else if ( attack1 )
	{
		// Wants to get into spawn queue
		if ( client->sess.spectatorState == SPECTATOR_FOLLOW )
		{
			G_StopFollowing( ent );
		}
	}
	else if ( attackReleased )
	{
		if ( team == TEAM_NONE )
		{
			G_TriggerMenu( client->ps.clientNum, MN_TEAM );
		}
		else if ( team == TEAM_ALIENS )
		{
			G_TriggerMenu( client->ps.clientNum, MN_A_CLASS );
		}
		else if ( team == TEAM_HUMANS )
		{
			G_TriggerMenu( client->ps.clientNum, MN_H_SPAWN );
		}
	}


	// We are either not following anyone or following a spectator
	if ( !following )
	{
		if ( client->sess.spectatorState == SPECTATOR_LOCKED ||
		     client->sess.spectatorState == SPECTATOR_FOLLOW )
		{
			client->ps.pm_type = PM_FREEZE;
		}
		else if ( client->noclip )
		{
			client->ps.pm_type = PM_NOCLIP;
		}
		else
		{
			client->ps.pm_type = PM_SPECTATOR;
		}

		if ( queued )
		{
			client->ps.pm_flags |= PMF_QUEUED;
		}

		client->ps.speed = client->pers.flySpeed;
		client->ps.stats[ STAT_STAMINA ] = 0;
		client->ps.stats[ STAT_FUEL ] = 0;
		client->ps.stats[ STAT_MISC ] = 0;
		client->ps.stats[ STAT_BUILDABLE ] = BA_NONE;
		client->ps.stats[ STAT_CLASS ] = PCL_NONE;
		client->ps.weapon = WP_NONE;

		// Set up for pmove
		memset( &pm, 0, sizeof( pm ) );
		pm.ps = &client->ps;
		pm.pmext = &client->pmext;
		pm.cmd = *ucmd;
		pm.tracemask = MASK_DEADSOLID; // spectators can fly through bodies
		pm.trace = trap_Trace;
		pm.pointcontents = trap_PointContents;

		// Perform a pmove
		Pmove( &pm );

		// Save results of pmove
		VectorCopy( client->ps.origin, ent->s.origin );

		G_TouchTriggers( ent );
		trap_UnlinkEntity( ent );

		// Set the queue position and spawn count for the client side
		if ( client->ps.pm_flags & PMF_QUEUED )
		{
			/* team must exist, or there will be a sigsegv */
			assert(team == TEAM_HUMANS || team == TEAM_ALIENS);
			client->ps.persistant[ PERS_SPAWNQUEUE ] = level.team[ team ].numSpawns;
			client->ps.persistant[ PERS_SPAWNQUEUE ] |= G_GetPosInSpawnQueue( &level.team[ team ].spawnQueue,
			                                                                  client->ps.clientNum ) << 8;
		}
	}
}

/*
=================
ClientInactivityTimer

Returns false if the client is dropped
=================
*/
bool ClientInactivityTimer( gentity_t *ent, bool active )
{
	gclient_t *client = ent->client;

	if ( !g_inactivity.integer )
	{
		// give everyone some time, so if the operator sets g_inactivity during
		// gameplay, everyone isn't kicked
		client->inactivityTime = level.time + 60 * 1000;
		client->inactivityWarning = false;
	}
	else if ( active ||
	          client->pers.cmd.forwardmove ||
	          client->pers.cmd.rightmove ||
	          client->pers.cmd.upmove ||
	          usercmdButtonPressed( client->pers.cmd.buttons, BUTTON_ATTACK ) )
	{
		client->inactivityTime = level.time + g_inactivity.integer * 1000;
		client->inactivityWarning = false;
	}
	else if ( !client->pers.localClient )
	{
		if ( level.time > client->inactivityTime &&
		     !G_admin_permission( ent, ADMF_ACTIVITY ) )
		{
			if( strchr( g_inactivity.string, 's' ) )
			{
				trap_SendServerCommand( -1,
				                        va( "print_tr %s %s %s", QQ( N_("$1$^7 moved from $2$ to spectators due to inactivity\n") ),
				                            Quote( client->pers.netname ), Quote( BG_TeamName( client->pers.team ) ) ) );
				G_LogPrintf( "Inactivity: %d\n", (int)( client - level.clients ) );
				G_ChangeTeam( ent, TEAM_NONE );
			}
			else
			{
				trap_DropClient( client - level.clients, "Dropped due to inactivity" );
				return false;
			}
		}

		if ( level.time > client->inactivityTime - 10000 &&
		     !client->inactivityWarning &&
		     !G_admin_permission( ent, ADMF_ACTIVITY ) )
		{
			client->inactivityWarning = true;
			trap_SendServerCommand( client - level.clients,
			                        va( "cp_tr %s", strchr( g_inactivity.string, 's' ) ? N_("\"Ten seconds until inactivity spectate!\n\"") : N_("\"Ten seconds until inactivity drop!\n\"") ) );
		}
	}

	return true;
}

// TODO: Move to MedikitComponent.
static void G_ReplenishHumanHealth( gentity_t *self )
{
	gclient_t *client;
	int       remainingStartupTime;

	if ( !self )
	{
		return;
	}

	client = self->client;

	if ( !client || client->pers.team != TEAM_HUMANS )
	{
		return;
	}

	// check if medikit is active
	if ( !( client->ps.stats[ STAT_STATE ] & SS_HEALING_4X ) )
	{
		return;
	}

	// stop if client is fully healed
	if ( self->entity->Get<HealthComponent>()->FullHealth() )
	{
		client->medKitHealthToRestore = 0;
		client->ps.stats[ STAT_STATE ] &= ~SS_HEALING_4X;

		return;
	}

	// stop if client is dead or medikit is depleted
	if ( client->medKitHealthToRestore <= 0 || client->ps.pm_type == PM_DEAD )
	{
		client->medKitHealthToRestore = 0;
		client->ps.stats[ STAT_STATE ] &= ~SS_HEALING_4X;

		return;
	}

	remainingStartupTime = MEDKIT_STARTUP_TIME - ( level.time - client->lastMedKitTime );

	// increase heal rate during startup
	if ( remainingStartupTime > 0 )
	{
		if ( level.time < client->medKitIncrementTime )
		{
			return;
		}
		else
		{
			client->medKitIncrementTime = level.time + ( remainingStartupTime / MEDKIT_STARTUP_SPEED );
		}
	}

	// heal
	self->entity->Heal(1.0f, nullptr);
	client->medKitHealthToRestore --;
}

static void BeaconAutoTag( gentity_t *self, int timePassed )
{
	gentity_t *traceEnt, *target;
	gclient_s *client;
	team_t    team;
	vec3_t viewOrigin, forward, end;

	if ( !( client = self->client ) ) return;

	team = (team_t)client->pers.team;

	BG_GetClientViewOrigin( &self->client->ps, viewOrigin );
	AngleVectors( self->client->ps.viewangles, forward, nullptr, nullptr );
	VectorMA( viewOrigin, 65536, forward, end );

	G_UnlaggedOn( self, viewOrigin, 65536 );
	traceEnt = Beacon::TagTrace( viewOrigin, end, self->s.number, MASK_SHOT, team, true );
	G_UnlaggedOff( );

	client->ps.stats[ STAT_TAGSCORE ] = 0;

	for ( target = nullptr; ( target = G_IterateEntities( target ) ); )
	{
		// Tag entity directly hit and entities in human radar range, make sure the latter are also
		// in vis and, for buildables, are in a line of sight.
		if( ( target == traceEnt ) ||
		    ( team == TEAM_HUMANS &&
		      BG_InventoryContainsUpgrade( UP_RADAR, client->ps.stats ) &&
		      Distance( self->s.origin, target->s.origin ) < RADAR_RANGE &&
		      Beacon::EntityTaggable( target->s.number, team, false ) &&
		      trap_InPVSIgnorePortals( self->s.origin, target->s.origin ) &&
		      ( target->s.eType != ET_BUILDABLE ||
		        G_LineOfSight( self, target, MASK_SOLID, false ) ) ) )
		{
			target->tagScore     += timePassed;
			target->tagScoreTime  = level.time;

			if( target->tagScore > 1000 )
				Beacon::Tag( target, team, ( target->s.eType == ET_BUILDABLE ) );

			client->ps.stats[ STAT_TAGSCORE ] = Math::Clamp(
				target->tagScore, client->ps.stats[ STAT_TAGSCORE ], 1000 );
		}
	}
}

/*
==================
ClientTimerActions

Actions that happen once a second
==================
*/
void ClientTimerActions( gentity_t *ent, int msec )
{
	playerState_t *ps;
	gclient_t     *client;
	int           i;
	buildable_t   buildable;

	if ( !ent || !ent->client )
	{
		return;
	}

	client = ent->client;
	ps     = &client->ps;

	client->time100 += msec;
	client->time1000 += msec;
	client->time10000 += msec;

	if( ent->r.svFlags & SVF_BOT )
	{
		G_BotThink( ent );
	}

	while ( client->time100 >= 100 )
	{
		weapon_t weapon = BG_GetPlayerWeapon( &client->ps );

		client->time100 -= 100;

		// Update build timer
		if ( weapon == WP_ABUILD || weapon == WP_ABUILD2 ||
		     BG_InventoryContainsWeapon( WP_HBUILD, client->ps.stats ) )
		{
			if ( client->ps.stats[ STAT_MISC ] > 0 )
			{
				client->ps.stats[ STAT_MISC ] -= 100;
			}

			if ( client->ps.stats[ STAT_MISC ] < 0 )
			{
				client->ps.stats[ STAT_MISC ] = 0;
			}
		}

		// Building related
		switch ( weapon )
		{
			case WP_ABUILD:
			case WP_ABUILD2:
			case WP_HBUILD:
				buildable = (buildable_t) ( client->ps.stats[ STAT_BUILDABLE ] & SB_BUILDABLE_MASK );

				// Set validity bit on buildable
				if ( buildable > BA_NONE )
				{
					vec3_t forward, aimDir, normal;
					vec3_t dummy, dummy2;
					int dummy3;
					int dist;

					BG_GetClientNormal( &client->ps,normal );
					AngleVectors( client->ps.viewangles, aimDir, nullptr, nullptr );
					ProjectPointOnPlane( forward, aimDir, normal );
					VectorNormalize( forward );

					dist = BG_Class( ent->client->ps.stats[ STAT_CLASS ] )->buildDist * DotProduct( forward, aimDir );

					client->ps.stats[ STAT_BUILDABLE ] &= ~SB_BUILDABLE_STATE_MASK;
					client->ps.stats[ STAT_BUILDABLE ] |= SB_BUILDABLE_FROM_IBE( G_CanBuild( ent, buildable, dist, dummy, dummy2, &dummy3 ) );

					if ( buildable == BA_H_DRILL || buildable == BA_A_LEECH )
					{
						client->ps.stats[ STAT_PREDICTION ] =
							( int )( G_RGSPredictEfficiencyDelta( dummy, ( team_t )ps->persistant[ PERS_TEAM ] ) * 100.0f );
					}

					// Let the client know which buildables will be removed by building
					for ( i = 0; i < MAX_MISC; i++ )
					{
						if ( i < level.numBuildablesForRemoval )
						{
							client->ps.misc[ i ] = level.markedBuildables[ i ]->s.number;
						}
						else
						{
							client->ps.misc[ i ] = 0;
						}
					}
				}
				else
				{
					for ( i = 0; i < MAX_MISC; i++ )
					{
						client->ps.misc[ i ] = 0;
					}
				}

				break;

			default:
				break;
		}

		BeaconAutoTag( ent, 100 );

		// replenish human health
		G_ReplenishHumanHealth( ent );

		// refill weapon ammo
		if ( ent->client->lastAmmoRefillTime + HUMAN_AMMO_REFILL_PERIOD < level.time &&
		     ps->weaponTime == 0 )
		{
			G_FindAmmo( ent );
		}

		// refill jetpack fuel
		if ( client->ps.stats[ STAT_FUEL ] < JETPACK_FUEL_REFUEL )
		{
			G_FindFuel( ent );
		}

		// remove orphaned tags for enemy structures when the structure's death was confirmed
		{
			gentity_t *other = nullptr;
			while ( ( other = G_IterateEntities( other ) ) )
			{
				if ( other->s.eType == ET_BEACON &&
				     other->s.modelindex == BCT_TAG &&
				     ( other->s.eFlags & EF_BC_ENEMY ) &&
				     !other->tagAttachment &&
				     ent->client->pers.team == other->s.generic1 &&
				     G_LineOfSight( ent, other, CONTENTS_SOLID, true ) )
				{
					Beacon::Delete( other, true );
				}
			}
		}
	}

	while ( client->time1000 >= 1000 )
	{
		client->time1000 -= 1000;

		// deal poison damage
		if ( client->ps.stats[ STAT_STATE ] & SS_POISONED )
		{
			ent->entity->Damage(ALIEN_POISON_DMG, client->lastPoisonClient, Util::nullopt,
			                    Util::nullopt, DAMAGE_NO_LOCDAMAGE, MOD_POISON);
		}

		// turn off life support when a team admits defeat
		if ( client->pers.team == TEAM_ALIENS &&
		     level.surrenderTeam == TEAM_ALIENS )
		{
			ent->entity->Damage((float)BG_Class(client->ps.stats[STAT_CLASS])->regenRate,
			                    nullptr, Util::nullopt, Util::nullopt, DAMAGE_PURE, MOD_SUICIDE);
		}
		else if ( client->pers.team == TEAM_HUMANS &&
		          level.surrenderTeam == TEAM_HUMANS )
		{
			ent->entity->Damage(5.0f, nullptr, Util::nullopt, Util::nullopt, DAMAGE_PURE, MOD_SUICIDE);
		}

		// lose some voice enthusiasm
		if ( client->voiceEnthusiasm > 0.0f )
		{
			client->voiceEnthusiasm -= VOICE_ENTHUSIASM_DECAY;
		}
		else
		{
			client->voiceEnthusiasm = 0.0f;
		}

		client->pers.aliveSeconds++;

		if ( g_freeFundPeriod.integer > 0 &&
		     client->pers.aliveSeconds % g_freeFundPeriod.integer == 0 )
		{
			// Give clients some credit periodically
			if ( client->pers.team == TEAM_ALIENS )
			{
				G_AddCreditToClient( client, PLAYER_BASE_VALUE, true );
			}
			else if ( client->pers.team == TEAM_HUMANS )
			{
				G_AddCreditToClient( client, PLAYER_BASE_VALUE, true );
			}
		}
	}

	while ( client->time10000 >= 10000 )
	{
		client->time10000 -= 10000;

		if ( ent->client->ps.weapon == WP_ABUILD ||
		     ent->client->ps.weapon == WP_ABUILD2 )
		{
			G_AddCreditsToScore( ent, ALIEN_BUILDER_SCOREINC );
		}
		else if ( ent->client->ps.weapon == WP_HBUILD )
		{
			G_AddCreditsToScore( ent, HUMAN_BUILDER_SCOREINC );
		}
	}

	// Regenerate Adv. Dragoon barbs
	if ( client->ps.weapon == WP_ALEVEL3_UPG )
	{
		if ( client->ps.ammo < BG_Weapon( WP_ALEVEL3_UPG )->maxAmmo )
		{
			if ( ent->timestamp + LEVEL3_BOUNCEBALL_REGEN < level.time )
			{
				client->ps.ammo++;
				ent->timestamp = level.time;
			}
		}
		else
		{
			ent->timestamp = level.time;
		}
	}
}

/*
====================
ClientIntermissionThink
====================
*/
void ClientIntermissionThink( gclient_t *client )
{
	client->ps.eFlags &= ~EF_FIRING;
	client->ps.eFlags &= ~EF_FIRING2;

	// the level will exit when everyone wants to or after timeouts

	// swap and latch button actions

	usercmdCopyButtons( client->oldbuttons, client->buttons );
	usercmdCopyButtons( client->buttons, client->pers.cmd.buttons );

	if ( ( usercmdButtonPressed( client->buttons, BUTTON_ATTACK ) ||
	       usercmdButtonPressed( client->buttons, BUTTON_USE_HOLDABLE ) ) &&
	     usercmdButtonsDiffer( client->oldbuttons, client->buttons ) )
	{
		client->readyToExit = 1;
	}
}

/*
================
ClientEvents

Events will be passed on to the clients for presentation,
but any server game effects are handled here
================
*/
void ClientEvents( gentity_t *ent, int oldEventSequence )
{
	int       i;
	int       event;
	gclient_t *client;
	int       damage;
	vec3_t    dir;
	vec3_t    point, mins;
	float     fallDistance;
	class_t   pcl;

	client = ent->client;
	pcl = (class_t) client->ps.stats[ STAT_CLASS ];

	if ( oldEventSequence < client->ps.eventSequence - MAX_EVENTS )
	{
		oldEventSequence = client->ps.eventSequence - MAX_EVENTS;
	}

	for ( i = oldEventSequence; i < client->ps.eventSequence; i++ )
	{
		event = client->ps.events[ i & ( MAX_EVENTS - 1 ) ];

		switch ( event )
		{
			case EV_FALL_MEDIUM:
			case EV_FALL_FAR:
				if ( ent->s.eType != ET_PLAYER )
				{
					break; // not in the player model
				}

				fallDistance = ( ( float ) client->ps.stats[ STAT_FALLDIST ] - MIN_FALL_DISTANCE ) /
				               ( MAX_FALL_DISTANCE - MIN_FALL_DISTANCE );

				if ( fallDistance < 0.0f )
				{
					fallDistance = 0.0f;
				}
				else if ( fallDistance > 1.0f )
				{
					fallDistance = 1.0f;
				}

				damage = ( int )( ( float ) BG_Class( pcl )->health *
				                  BG_Class( pcl )->fallDamage * fallDistance );

				VectorSet( dir, 0, 0, 1 );
				BG_ClassBoundingBox( pcl, mins, nullptr, nullptr, nullptr, nullptr );
				mins[ 0 ] = mins[ 1 ] = 0.0f;
				VectorAdd( client->ps.origin, mins, point );

				ent->pain_debounce_time = level.time + 200; // no general pain sound
				ent->entity->Damage((float)damage, nullptr, Vec3::Load(point), Vec3::Load(dir),
				                    DAMAGE_NO_LOCDAMAGE, MOD_FALLING);
				break;

			case EV_FIRE_WEAPON:
				G_FireWeapon( ent, ( weapon_t )ent->s.weapon, WPM_PRIMARY );
				break;

			case EV_FIRE_WEAPON2:
				G_FireWeapon( ent, ( weapon_t )ent->s.weapon, WPM_SECONDARY );
				break;

			case EV_FIRE_WEAPON3:
				G_FireWeapon( ent, ( weapon_t )ent->s.weapon, WPM_TERTIARY );
				break;

			case EV_NOAMMO:
				break;

			default:
				break;
		}
	}
}

/*
==============
SendPendingPredictableEvents
==============
*/
void SendPendingPredictableEvents( playerState_t *ps )
{
	gentity_t *t;
	int       event, seq;
	int       extEvent, number;

	// if there are still events pending
	if ( ps->entityEventSequence < ps->eventSequence )
	{
		// create a temporary entity for this event which is sent to everyone
		// except the client who generated the event
		seq = ps->entityEventSequence & ( MAX_EVENTS - 1 );
		event = ps->events[ seq ] | ( ( ps->entityEventSequence & 3 ) << 8 );
		// set external event to zero before calling BG_PlayerStateToEntityState
		extEvent = ps->externalEvent;
		ps->externalEvent = 0;
		// create temporary entity for event
		t = G_NewTempEntity( ps->origin, event );
		number = t->s.number;
		BG_PlayerStateToEntityState( ps, &t->s, true );
		t->s.number = number;
		t->s.eType = (entityType_t) ( ET_EVENTS + event );
		t->s.eFlags |= EF_PLAYER_EVENT;
		t->s.otherEntityNum = ps->clientNum;
		// send to everyone except the client who generated the event
		t->r.svFlags |= SVF_NOTSINGLECLIENT;
		t->r.singleClient = ps->clientNum;
		// set back external event
		ps->externalEvent = extEvent;
	}
}

/*
==============
 G_UnlaggedStore

 Called on every server frame.  Stores position data for the client at that
 into client->unlaggedHist[] and the time into level.unlaggedTimes[].
 This data is used by G_UnlaggedCalc()
==============
*/
void G_UnlaggedStore()
{
	int        i = 0;
	gentity_t  *ent;
	unlagged_t *save;

	if ( !g_unlagged.integer )
	{
		return;
	}

	level.unlaggedIndex++;

	if ( level.unlaggedIndex >= MAX_UNLAGGED_MARKERS )
	{
		level.unlaggedIndex = 0;
	}

	level.unlaggedTimes[ level.unlaggedIndex ] = level.time;

	for ( i = 0; i < level.maxclients; i++ )
	{
		ent = &g_entities[ i ];
		save = &ent->client->unlaggedHist[ level.unlaggedIndex ];
		save->used = false;

		if ( !ent->r.linked || !( ent->r.contents & CONTENTS_BODY ) )
		{
			continue;
		}

		if ( ent->client->pers.connected != CON_CONNECTED )
		{
			continue;
		}

		VectorCopy( ent->r.mins, save->mins );
		VectorCopy( ent->r.maxs, save->maxs );
		VectorCopy( ent->s.pos.trBase, save->origin );
		save->used = true;
	}
}

/*
==============
 G_UnlaggedClear

 Mark all unlaggedHist[] markers for this client invalid.  Useful for
 preventing teleporting and death.
==============
*/
void G_UnlaggedClear( gentity_t *ent )
{
	int i;

	for ( i = 0; i < MAX_UNLAGGED_MARKERS; i++ )
	{
		ent->client->unlaggedHist[ i ].used = false;
	}
}

/*
==============
 G_UnlaggedCalc

 Loops through all active clients and calculates their predicted position
 for time then stores it in client->unlaggedCalc
==============
*/
void G_UnlaggedCalc( int time, gentity_t *rewindEnt )
{
	int       i = 0;
	gentity_t *ent;
	int       startIndex = level.unlaggedIndex;
	int       stopIndex = -1;
	int       frameMsec = 0;
	float     lerp = 0.5f;

	if ( !g_unlagged.integer )
	{
		return;
	}

	// clear any calculated values from a previous run
	for ( i = 0; i < level.maxclients; i++ )
	{
		ent = &g_entities[ i ];

		if ( !ent->inuse )
		{
			continue;
		}

		ent->client->unlaggedCalc.used = false;
	}

	for ( i = 0; i < MAX_UNLAGGED_MARKERS; i++ )
	{
		if ( level.unlaggedTimes[ startIndex ] <= time )
		{
			break;
		}

		stopIndex = startIndex;

		if ( --startIndex < 0 )
		{
			startIndex = MAX_UNLAGGED_MARKERS - 1;
		}
	}

	if ( i == MAX_UNLAGGED_MARKERS )
	{
		// if we searched all markers and the oldest one still isn't old enough
		// just use the oldest marker with no lerping
		lerp = 0.0f;
	}

	// client is on the current frame, no need for unlagged
	if ( stopIndex == -1 )
	{
		return;
	}

	// lerp between two markers
	frameMsec = level.unlaggedTimes[ stopIndex ] -
	            level.unlaggedTimes[ startIndex ];

	if ( frameMsec > 0 )
	{
		lerp = ( float )( time - level.unlaggedTimes[ startIndex ] ) /
		       ( float ) frameMsec;
	}

	for ( i = 0; i < level.maxclients; i++ )
	{
		ent = &g_entities[ i ];

		if ( ent == rewindEnt )
		{
			continue;
		}

		if ( !ent->inuse )
		{
			continue;
		}

		if ( !ent->r.linked || !( ent->r.contents & CONTENTS_BODY ) )
		{
			continue;
		}

		if ( ent->client->pers.connected != CON_CONNECTED )
		{
			continue;
		}

		if ( !ent->client->unlaggedHist[ startIndex ].used )
		{
			continue;
		}

		if ( !ent->client->unlaggedHist[ stopIndex ].used )
		{
			continue;
		}

		// between two unlagged markers
		VectorLerpTrem( lerp, ent->client->unlaggedHist[ startIndex ].mins,
		                ent->client->unlaggedHist[ stopIndex ].mins,
		                ent->client->unlaggedCalc.mins );
		VectorLerpTrem( lerp, ent->client->unlaggedHist[ startIndex ].maxs,
		                ent->client->unlaggedHist[ stopIndex ].maxs,
		                ent->client->unlaggedCalc.maxs );
		VectorLerpTrem( lerp, ent->client->unlaggedHist[ startIndex ].origin,
		                ent->client->unlaggedHist[ stopIndex ].origin,
		                ent->client->unlaggedCalc.origin );

		ent->client->unlaggedCalc.used = true;
	}
}

/*
==============
 G_UnlaggedOff

 Reverses the changes made to all active clients by G_UnlaggedOn()
==============
*/
void G_UnlaggedOff()
{
	int       i = 0;
	gentity_t *ent;

	if ( !g_unlagged.integer )
	{
		return;
	}

	for ( i = 0; i < level.maxclients; i++ )
	{
		ent = &g_entities[ i ];

		if ( !ent->client->unlaggedBackup.used )
		{
			continue;
		}

		VectorCopy( ent->client->unlaggedBackup.mins, ent->r.mins );
		VectorCopy( ent->client->unlaggedBackup.maxs, ent->r.maxs );
		VectorCopy( ent->client->unlaggedBackup.origin, ent->r.currentOrigin );
		ent->client->unlaggedBackup.used = false;
		trap_LinkEntity( ent );
	}
}

/*
==============
 G_UnlaggedOn

 Called after G_UnlaggedCalc() to apply the calculated values to all active
 clients.  Once finished tracing, G_UnlaggedOff() must be called to restore
 the clients' position data

 As an optimization, all clients that have an unlagged position that is
 not touchable at "range" from "muzzle" will be ignored.  This is required
 to prevent a huge amount of trap_LinkEntity() calls per user cmd.
==============
*/

void G_UnlaggedOn( gentity_t *attacker, vec3_t muzzle, float range )
{
	int        i = 0;
	gentity_t  *ent;
	unlagged_t *calc;

	if ( !g_unlagged.integer )
	{
		return;
	}

	if ( !attacker->client->pers.useUnlagged )
	{
		return;
	}

	for ( i = 0; i < level.maxclients; i++ )
	{
		ent = &g_entities[ i ];
		calc = &ent->client->unlaggedCalc;

		if ( !calc->used )
		{
			continue;
		}

		if ( ent->client->unlaggedBackup.used )
		{
			continue;
		}

		if ( !ent->r.linked || !( ent->r.contents & CONTENTS_BODY ) )
		{
			continue;
		}

		if ( VectorCompare( ent->r.currentOrigin, calc->origin ) )
		{
			continue;
		}

		if ( muzzle )
		{
			float r1 = Distance( calc->origin, calc->maxs );
			float r2 = Distance( calc->origin, calc->mins );
			float maxRadius = ( r1 > r2 ) ? r1 : r2;

			if ( Distance( muzzle, calc->origin ) > range + maxRadius )
			{
				continue;
			}
		}

		// create a backup of the real positions
		VectorCopy( ent->r.mins, ent->client->unlaggedBackup.mins );
		VectorCopy( ent->r.maxs, ent->client->unlaggedBackup.maxs );
		VectorCopy( ent->r.currentOrigin, ent->client->unlaggedBackup.origin );
		ent->client->unlaggedBackup.used = true;

		// move the client to the calculated unlagged position
		VectorCopy( calc->mins, ent->r.mins );
		VectorCopy( calc->maxs, ent->r.maxs );
		VectorCopy( calc->origin, ent->r.currentOrigin );
		trap_LinkEntity( ent );
	}
}

/*
==============
 G_UnlaggedDetectCollisions

 cgame prediction will predict a client's own position all the way up to
 the current time, but only updates other player's positions up to the
 postition sent in the most recent snapshot.

 This allows player X to essentially "move through" the position of player Y
 when player X's cmd is processed with Pmove() on the server.  This is because
 player Y was clipping player X's Pmove() on his client, but when the same
 cmd is processed with Pmove on the server it is not clipped.

 Long story short (too late): don't use unlagged positions for players who
 were blocking this player X's client-side Pmove().  This makes the assumption
 that if player X's movement was blocked in the client he's going to still
 be up against player Y when the Pmove() is run on the server with the
 same cmd.

 NOTE: this must be called after Pmove() and G_UnlaggedCalc()
==============
*/
static void G_UnlaggedDetectCollisions( gentity_t *ent )
{
	unlagged_t *calc;
	trace_t    tr;
	float      r1, r2;
	float      range;

	if ( !g_unlagged.integer )
	{
		return;
	}

	if ( !ent->client->pers.useUnlagged )
	{
		return;
	}

	calc = &ent->client->unlaggedCalc;

	// if the client isn't moving, this is not necessary
	if ( VectorCompare( ent->client->oldOrigin, ent->client->ps.origin ) )
	{
		return;
	}

	range = Distance( ent->client->oldOrigin, ent->client->ps.origin );

	// increase the range by the player's largest possible radius since it's
	// the players bounding box that collides, not their origin
	r1 = Distance( calc->origin, calc->mins );
	r2 = Distance( calc->origin, calc->maxs );
	range += ( r1 > r2 ) ? r1 : r2;

	G_UnlaggedOn( ent, ent->client->oldOrigin, range );

	trap_Trace( &tr, ent->client->oldOrigin, ent->r.mins, ent->r.maxs,
	            ent->client->ps.origin, ent->s.number, MASK_PLAYERSOLID, 0 );

	if ( tr.entityNum >= 0 && tr.entityNum < MAX_CLIENTS )
	{
		g_entities[ tr.entityNum ].client->unlaggedCalc.used = false;
	}

	G_UnlaggedOff();
}

/**
 * @brief Attempt to find a health source for an alien.
 * @return A mask of SS_HEALING_* flags.
 */
static int FindAlienHealthSource( gentity_t *self )
{
	int       ret = 0, closeTeammates = 0;
	float     distance, minBoosterDistance = FLT_MAX;
	bool  needsHealing;
	gentity_t *ent;

	if ( !self || !self->client )
	{
		return 0;
	}

	needsHealing = !self->entity->Get<HealthComponent>()->FullHealth();

	self->boosterUsed = nullptr;

	for ( ent = nullptr; ( ent = G_IterateEntities( ent, nullptr, true, 0, nullptr ) ); )
	{
		if ( !G_OnSameTeam( self, ent ) ) continue;
		if ( G_Dead( ent ) )              continue;

		distance = Distance( ent->s.origin, self->s.origin );

		if ( ent->client && self != ent && distance < REGEN_TEAMMATE_RANGE &&
		     G_LineOfSight( self, ent, MASK_SOLID, false ) )
		{
			closeTeammates++;
			ret |= ( closeTeammates > 1 ) ? SS_HEALING_4X : SS_HEALING_2X;
		}
		else if ( ent->s.eType == ET_BUILDABLE && ent->spawned && ent->powered )
		{
			if ( ent->s.modelindex == BA_A_BOOSTER && ent->powered &&
			     distance < REGEN_BOOSTER_RANGE )
			{
				// Booster healing
				ret |= SS_HEALING_8X;

				// The closest booster used will play an effect
				if ( needsHealing && distance < minBoosterDistance )
				{
					minBoosterDistance = distance;
					self->boosterUsed  = ent;
				}
			}
			else if ( distance < BG_Buildable( ent->s.modelindex )->creepSize )
			{
				// Creep healing
				ret |= SS_HEALING_4X;
			}
			else if ( ( ent->s.modelindex == BA_A_OVERMIND || ent->s.modelindex == BA_A_SPAWN ) &&
			          distance < CREEP_BASESIZE )
			{
				// Base healing
				ret |= SS_HEALING_2X;
			}
		}
	}

	if ( ret & SS_HEALING_8X ) ret |= SS_HEALING_4X;
	if ( ret & SS_HEALING_4X ) ret |= SS_HEALING_2X;

	if ( ret )
	{
		self->healthSourceTime = level.time;

		if ( self->boosterUsed )
		{
			self->boosterTime = level.time;
		}
	}

	return ret;
}

// TODO: Synchronize
// TODO: Move to HealthRegenComponent.
static void G_ReplenishAlienHealth( gentity_t *self )
{
	gclient_t *client;
	float     regenBaseRate, modifier;
	int       count, interval;
	bool  wasHealing;

	client = self->client;

	// Check if client is an alien and has the healing ability
	if ( !client || client->pers.team != TEAM_ALIENS || G_Dead( self )
	     || level.surrenderTeam == client->pers.team )
	{
		return;
	}

	regenBaseRate = BG_Class( client->ps.stats[ STAT_CLASS ] )->regenRate;

	if ( regenBaseRate <= 0 )
	{
		return;
	}

	wasHealing = client->ps.stats[ STAT_STATE ] & SS_HEALING_2X;

	// Check for health sources
	client->ps.stats[ STAT_STATE ] &= ~( SS_HEALING_2X | SS_HEALING_4X | SS_HEALING_8X );
	client->ps.stats[ STAT_STATE ] |= FindAlienHealthSource( self );

	if ( self->nextRegenTime < level.time )
	{
		if      ( client->ps.stats[ STAT_STATE ] & SS_HEALING_8X )
		{
			modifier = 8.0f;
		}
		else if ( client->ps.stats[ STAT_STATE ] & SS_HEALING_4X )
		{
			modifier = 4.0f;
		}
		else if ( client->ps.stats[ STAT_STATE ] & SS_HEALING_2X )
		{
			modifier = 2.0f;
		}
		else
		{
			if ( g_alienOffCreepRegenHalfLife.value < 1 )
			{
				modifier = 1.0f;
			}
			else
			{
				// Exponentially decrease healing rate when not on creep. ln(2) ~= 0.6931472
				modifier = exp( ( 0.6931472f / ( 1000.0f * g_alienOffCreepRegenHalfLife.value ) ) *
				                ( self->healthSourceTime - level.time ) );
				modifier = MAX( modifier, ALIEN_REGEN_NOCREEP_MIN );
			}
		}

		interval = ( int )( 1000.0f / ( regenBaseRate * modifier ) );

		// If recovery interval is less than frametime, compensate by healing more
		count = 1 + ( level.time - self->nextRegenTime ) / interval;

		self->entity->Heal((float)count, nullptr);

		self->nextRegenTime = level.time + count * interval;
	}
	else if ( !wasHealing && client->ps.stats[ STAT_STATE ] & SS_HEALING_2X )
	{
		// Don't immediately start regeneration to prevent players from quickly
		// hopping in and out of a creep area to increase their heal rate
		self->nextRegenTime = level.time + ( 1000 / regenBaseRate );
	}
}

/*
==============
ClientThink_real

This will be called once for each client frame, which will
usually be a couple times for each server frame on fast clients.

If "g_synchronousClients 1" is set, this will be called exactly
once for each server frame, which makes for smooth demo recording.
==============
*/
void ClientThink_real( gentity_t *self )
{
	gclient_t *client;
	pmove_t   pm;
	int       oldEventSequence;
	int       msec;
	usercmd_t *ucmd;

	client = self->client;

	// don't think if the client is not yet connected (and thus not yet spawned in)
	if ( client->pers.connected != CON_CONNECTED )
	{
		return;
	}

	// mark the time, so the connection sprite can be removed
	ucmd = &client->pers.cmd;

	// sanity check the command time to prevent speedup cheating
	if ( ucmd->serverTime > level.time + 200 )
	{
		ucmd->serverTime = level.time + 200;
//    G_Printf("serverTime <<<<<\n" );
	}

	if ( ucmd->serverTime < level.time - 1000 )
	{
		ucmd->serverTime = level.time - 1000;
//    G_Printf("serverTime >>>>>\n" );
	}

	msec = ucmd->serverTime - client->ps.commandTime;

	// following others may result in bad times, but we still want
	// to check for follow toggles
	if ( msec < 1 && client->sess.spectatorState != SPECTATOR_FOLLOW )
	{
		return;
	}

	if ( msec > 200 )
	{
		msec = 200;
	}

	client->unlaggedTime = ucmd->serverTime;

	if ( level.pmoveParams.fixed || client->pers.pmoveFixed )
	{
		ucmd->serverTime = ( ( ucmd->serverTime + level.pmoveParams.msec - 1 ) / level.pmoveParams.msec ) * level.pmoveParams.msec;
		//if (ucmd->serverTime - client->ps.commandTime <= 0)
		//  return;
	}

	// client is admin but program hasn't responded to challenge? Resend
	ClientAdminChallenge( self - g_entities );

	//
	// check for exiting intermission
	//
	if ( level.intermissiontime )
	{
		if( self->r.svFlags & SVF_BOT )
			G_BotIntermissionThink( client );
		else
			ClientIntermissionThink( client );
		return;
	}

	// spectators don't do much
	if ( client->sess.spectatorState != SPECTATOR_NOT )
	{
		if ( client->sess.spectatorState == SPECTATOR_SCOREBOARD )
		{
			return;
		}

		SpectatorThink( self, ucmd );
		return;
	}

	G_namelog_update_score( client );

	// check for inactivity timer, but never drop the local client of a non-dedicated server
	if ( !ClientInactivityTimer( self, false ) )
	{
		return;
	}

	// calculate where ent is currently seeing all the other active clients
	G_UnlaggedCalc( client->unlaggedTime, self );

	if ( client->noclip )
	{
		client->ps.pm_type = PM_NOCLIP;
	}
	else if ( G_Dead( self ) )
	{
		client->ps.pm_type = PM_DEAD;
	}
	else if ( client->ps.stats[ STAT_STATE ] & SS_BLOBLOCKED )
	{
		client->ps.pm_type = PM_GRABBED;
	}
	else
	{
		client->ps.pm_type = PM_NORMAL;
	}

	if ( ( client->ps.stats[ STAT_STATE ] & SS_BLOBLOCKED ) &&
	     client->lastLockTime + LOCKBLOB_LOCKTIME < level.time )
	{
		client->ps.stats[ STAT_STATE ] &= ~SS_BLOBLOCKED;
	}

	if ( ( client->ps.stats[ STAT_STATE ] & SS_SLOWLOCKED ) &&
	     client->lastSlowTime + ABUILDER_BLOB_TIME < level.time )
	{
		client->ps.stats[ STAT_STATE ] &= ~SS_SLOWLOCKED;
	}

	// Is power/creep available for the client's team?
	if ( client->pers.team == TEAM_HUMANS && G_ActiveReactor() )
	{
		client->ps.eFlags |= EF_POWER_AVAILABLE;
	}
	else if ( client->pers.team == TEAM_ALIENS && G_ActiveOvermind() )
	{
		client->ps.eFlags |= EF_POWER_AVAILABLE;
	}
	else
	{
		client->ps.eFlags &= ~EF_POWER_AVAILABLE;
	}

	// Update boosted state flags
	client->ps.stats[ STAT_STATE ] &= ~SS_BOOSTEDWARNING;

	if ( client->ps.stats[ STAT_STATE ] & SS_BOOSTED )
	{
		if ( level.time - client->boostedTime != BOOST_TIME )
		{
			client->ps.stats[ STAT_STATE ] &= ~SS_BOOSTEDNEW;
		}
		if ( level.time - client->boostedTime >= BOOST_TIME )
		{
			client->ps.stats[ STAT_STATE ] &= ~SS_BOOSTED;
		}
		else if ( level.time - client->boostedTime >= BOOST_WARN_TIME )
		{
			client->ps.stats[ STAT_STATE ] |= SS_BOOSTEDWARNING;
		}
	}

	if ( (client->ps.stats[ STAT_STATE ] & SS_POISONED) &&
	     client->lastPoisonTime + ALIEN_POISON_TIME < level.time )
	{
		client->ps.stats[ STAT_STATE ] &= ~SS_POISONED;
	}

	// copy global gravity to playerstate
	client->ps.gravity = g_gravity.value;

	HealthComponent *healthComponent = self->entity->Get<HealthComponent>();

	// handle medkit (TODO: move into helper function)
	if ( BG_InventoryContainsUpgrade( UP_MEDKIT, client->ps.stats ) &&
	     BG_UpgradeIsActive( UP_MEDKIT, client->ps.stats ) )
	{
		//if currently using a medkit or have no need for a medkit now
		if ( (client->ps.stats[ STAT_STATE ] & SS_HEALING_4X) ||
		     ( healthComponent->FullHealth() &&
		       !( client->ps.stats[ STAT_STATE ] & SS_POISONED ) ) )
		{
			BG_DeactivateUpgrade( UP_MEDKIT, client->ps.stats );
		}
		else if ( G_Alive( self ) )
		{
			//remove anti toxin
			BG_DeactivateUpgrade( UP_MEDKIT, client->ps.stats );
			BG_RemoveUpgradeFromInventory( UP_MEDKIT, client->ps.stats );

			client->ps.stats[ STAT_STATE ] &= ~SS_POISONED;
			client->poisonImmunityTime = level.time + MEDKIT_POISON_IMMUNITY_TIME;

			client->ps.stats[ STAT_STATE ] |= SS_HEALING_4X;
			client->lastMedKitTime = level.time;
			client->medKitHealthToRestore = healthComponent->MaxHealth() - healthComponent->Health();
			client->medKitIncrementTime = level.time +
			                              ( MEDKIT_STARTUP_TIME / MEDKIT_STARTUP_SPEED );

			G_AddEvent( self, EV_MEDKIT_USED, 0 );
		}
	}

	// Replenish alien health
	G_ReplenishAlienHealth( self );

	// Throw human grenade
	if ( BG_InventoryContainsUpgrade( UP_GRENADE, client->ps.stats ) &&
	     BG_UpgradeIsActive( UP_GRENADE, client->ps.stats ) )
	{
		// Remove from inventory
		BG_DeactivateUpgrade( UP_GRENADE, client->ps.stats );
		BG_RemoveUpgradeFromInventory( UP_GRENADE, client->ps.stats );

		G_FireUpgrade( self, UP_GRENADE );
	}

	// Throw human firebomb
	if ( BG_InventoryContainsUpgrade( UP_FIREBOMB, client->ps.stats ) &&
	     BG_UpgradeIsActive( UP_FIREBOMB, client->ps.stats ) )
	{
		// Remove from inventory
		BG_DeactivateUpgrade( UP_FIREBOMB, client->ps.stats );
		BG_RemoveUpgradeFromInventory( UP_FIREBOMB, client->ps.stats );

		G_FireUpgrade( self, UP_FIREBOMB );
	}

	// set speed
	if ( client->ps.pm_type == PM_NOCLIP )
	{
		client->ps.speed = client->pers.flySpeed;
	}
	else
	{
		client->ps.speed = g_speed.value *
		                   BG_Class( client->ps.stats[ STAT_CLASS ] )->speed;
	}

	// unset creepslowed flag if it's time
	if ( client->lastCreepSlowTime + CREEP_TIMEOUT < level.time )
	{
		client->ps.stats[ STAT_STATE ] &= ~SS_CREEPSLOWED;
	}

	// unset level1slow flag if it's time
	if ( client->lastLevel1SlowTime + LEVEL1_SLOW_TIME < level.time )
	{
		client->ps.stats[ STAT_STATE2 ] &= ~SS2_LEVEL1SLOW;
	}

	// set up for pmove
	oldEventSequence = client->ps.eventSequence;

	memset( &pm, 0, sizeof( pm ) );

	if ( self->flags & FL_FORCE_GESTURE )
	{
		self->flags &= ~FL_FORCE_GESTURE;
		usercmdPressButton( client->pers.cmd.buttons, BUTTON_GESTURE );
	}

	// clear fall impact velocity before every pmove
	VectorSet( client->pmext.fallImpactVelocity, 0.0f, 0.0f, 0.0f );

	pm.ps    = &client->ps;
	pm.pmext = &client->pmext;
	pm.cmd   = *ucmd;

	if ( pm.ps->pm_type == PM_DEAD )
	{
		pm.tracemask = MASK_DEADSOLID;
	}
	else
	{
		pm.tracemask = MASK_PLAYERSOLID;
	}

	pm.trace          = trap_Trace;
	pm.pointcontents  = trap_PointContents;
	pm.debugLevel     = g_debugMove.integer;
	pm.noFootsteps    = 0;
	pm.pmove_fixed    = level.pmoveParams.fixed || client->pers.pmoveFixed;
	pm.pmove_msec     = level.pmoveParams.msec;
	pm.pmove_accurate = level.pmoveParams.accurate;

	VectorCopy( client->ps.origin, client->oldOrigin );

	// moved from after Pmove -- potentially the cause of future triggering bugs
	G_TouchTriggers( self );

	// Do this before Pmove because it is shared code and accesses networked fields.
	G_PrepareEntityNetCode();

	Pmove( &pm );

	G_UnlaggedDetectCollisions( self );

	// save results of pmove
	if ( client->ps.eventSequence != oldEventSequence )
	{
		self->eventTime = level.time;
	}

	if ( g_smoothClients.integer )
	{
		BG_PlayerStateToEntityStateExtraPolate( &client->ps, &self->s, client->ps.commandTime, true );
	}
	else
	{
		BG_PlayerStateToEntityState( &client->ps, &self->s, true );
	}

	// update attached tags right after evaluating movement
	Beacon::UpdateTags( self );

	switch ( client->ps.weapon )
	{
		case WP_ALEVEL0:
			if ( !G_CheckVenomAttack( self ) )
			{
				client->ps.weaponstate = WEAPON_READY;
			}
			else
			{
				client->ps.generic1 = WPM_PRIMARY;
				G_AddEvent( self, EV_FIRE_WEAPON, 0 );
			}

			break;

		case WP_ALEVEL3:
		case WP_ALEVEL3_UPG:
			if ( !G_CheckPounceAttack( self ) )
			{
				client->ps.weaponstate = WEAPON_READY;
			}
			else
			{
				client->ps.generic1 = WPM_SECONDARY;
				G_AddEvent( self, EV_FIRE_WEAPON2, 0 );
			}

			break;

		case WP_ALEVEL4:

			// If not currently in a trample, reset the trample bookkeeping data
			if ( !( client->ps.pm_flags & PMF_CHARGE ) && client->trampleBuildablesHitPos )
			{
				client->trampleBuildablesHitPos = 0;
				memset( client->trampleBuildablesHit, 0, sizeof( client->trampleBuildablesHit ) );
			}

			break;

		case WP_HBUILD:
			G_CheckCkitRepair( self );
			break;

		default:
			break;
	}

	SendPendingPredictableEvents( &client->ps );

	// use the snapped origin for linking so it matches client predicted versions
	VectorCopy( self->s.pos.trBase, self->r.currentOrigin );

	VectorCopy( pm.mins, self->r.mins );
	VectorCopy( pm.maxs, self->r.maxs );

	self->waterlevel = pm.waterlevel;
	self->watertype = pm.watertype;

	// touch other objects
	ClientImpacts( self, &pm );

	// execute client events
	ClientEvents( self, oldEventSequence );

	// link entity now, after any personal teleporters have been used
	trap_LinkEntity( self );

	// NOTE: now copy the exact origin over otherwise clients can be snapped into solid
	VectorCopy( client->ps.origin, self->r.currentOrigin );
	VectorCopy( client->ps.origin, self->s.origin );

	// save results of triggers and client events
	if ( client->ps.eventSequence != oldEventSequence )
	{
		self->eventTime = level.time;
	}

	// inform client about the state of unlockable items
	client->ps.persistant[ PERS_UNLOCKABLES ] = BG_UnlockablesMask( client->pers.team );

	// Don't think anymore if dead
	if ( G_Dead( self ) )
	{
		return;
	}

	// swap and latch button actions
	usercmdCopyButtons( client->oldbuttons, client->buttons );
	usercmdCopyButtons( client->buttons, ucmd->buttons );
	usercmdLatchButtons( client->latched_buttons, client->buttons, client->oldbuttons );

	if ( usercmdButtonPressed( client->buttons, BUTTON_ACTIVATE ) &&
	     !usercmdButtonPressed( client->oldbuttons, BUTTON_ACTIVATE ) &&
	     G_Alive( self ) )
	{
		trace_t   trace;
		vec3_t    view, point;
		gentity_t *ent;

		// look for object infront of player
		AngleVectors( client->ps.viewangles, view, nullptr, nullptr );
		VectorMA( client->ps.origin, ENTITY_USE_RANGE, view, point );
		trap_Trace( &trace, client->ps.origin, nullptr, nullptr, point, self->s.number, MASK_SHOT, 0 );

		ent = &g_entities[ trace.entityNum ];

		if ( ent && ent->use &&
		     ( !ent->buildableTeam   || ent->buildableTeam   == client->pers.team ) &&
		     ( !ent->conditions.team || ent->conditions.team == client->pers.team ) )
		{
			if ( g_debugEntities.integer > 1 )
			{
				G_Printf("Debug: Calling entity->use for player facing %s\n", etos(ent));
			}

			ent->use( ent, self, self ); // other and activator are the same in this context
		}
		else
		{
			// no entity in front of player - do a small area search
			for ( ent = nullptr; ( ent = G_IterateEntitiesWithinRadius( ent, client->ps.origin, ENTITY_USE_RANGE ) ); )
			{
				if ( ent && ent->use && ent->buildableTeam == client->pers.team)
				{
					if ( g_debugEntities.integer > 1 )
					{
						G_Printf("Debug: Calling entity->use after an area-search for %s\n", etos(ent));
					}

					ent->use( ent, self, self ); // other and activator are the same in this context

					break;
				}
			}

			if ( !ent && client->pers.team == TEAM_ALIENS )
			{
				if ( BG_AlienCanEvolve( client->ps.stats[ STAT_CLASS ], client->pers.credit ) )
				{
					// no nearby objects and alien - show class menu
					G_TriggerMenu( client->ps.clientNum, MN_A_INFEST );
				}
				else
				{
					// flash frags
					G_AddEvent( self, EV_ALIEN_EVOLVE_FAILED, 0 );
				}
			}
		}
	}

	client->ps.persistant[ PERS_BP ] = G_GetBuildPointsInt( (team_t) client->pers.team );
	client->ps.persistant[ PERS_MARKEDBP ] = G_GetMarkedBuildPointsInt( (team_t) client->pers.team );

	if ( client->ps.persistant[ PERS_BP ] < 0 )
	{
		client->ps.persistant[ PERS_BP ] = 0;
	}

	// perform once-a-second actions
	ClientTimerActions( self, msec );

	if ( self->suicideTime > 0 && self->suicideTime < level.time )
	{
		G_Kill(self, nullptr, MOD_SUICIDE);
		self->suicideTime = 0;
	}
}

/*
==================
ClientThink

A new command has arrived from the client
==================
*/
void ClientThink( int clientNum )
{
	gentity_t *ent;

	ent = g_entities + clientNum;
	trap_GetUsercmd( clientNum, &ent->client->pers.cmd );

	// mark the time we got info, so we can display the phone jack if we don't get any for a while
	ent->client->lastCmdTime = level.time;

	if(!( ent->r.svFlags & SVF_BOT ) && !level.pmoveParams.synchronous )
	{
		ClientThink_real( ent );
	}
}

void G_RunClient( gentity_t *ent )
{
	if(!( ent->r.svFlags & SVF_BOT ) && !level.pmoveParams.synchronous )
	{
		return;
	}

	ent->client->pers.cmd.serverTime = level.time;
	ClientThink_real( ent );
}

/*
==================
SpectatorClientEndFrame

==================
*/
void SpectatorClientEndFrame( gentity_t *ent )
{
	gclient_t *cl;
	int       clientNum;
	int       score, ping;

	// if we are doing a chase cam or a remote view, grab the latest info
	if ( ent->client->sess.spectatorState == SPECTATOR_FOLLOW )
	{
		clientNum = ent->client->sess.spectatorClient;

		if ( clientNum >= 0 && clientNum < level.maxclients )
		{
			cl = &level.clients[ clientNum ];

			if ( cl->pers.connected == CON_CONNECTED )
			{
				// Save
				score = ent->client->ps.persistant[ PERS_SCORE ];
				ping = ent->client->ps.ping;

				// Copy
				ent->client->ps = cl->ps;

				// Restore
				ent->client->ps.persistant[ PERS_SCORE ] = score;
				ent->client->ps.ping = ping;

				ent->client->ps.pm_flags |= PMF_FOLLOW;
				ent->client->ps.pm_flags &= ~PMF_QUEUED;
			}
		}
	}
}

/*
==============
ClientEndFrame

Called at the end of each server frame for each connected client
A fast client will have multiple ClientThink for each ClientEdFrame,
while a slow client may have multiple ClientEndFrame between ClientThink.
==============
*/
void ClientEndFrame( gentity_t *ent )
{
	if ( ent->client->sess.spectatorState != SPECTATOR_NOT )
	{
		SpectatorClientEndFrame( ent );
		return;
	}

	//
	// If the end of unit layout is displayed, don't give
	// the player any normal movement attributes
	//
	if ( level.intermissiontime )
	{
		return;
	}

	// burn from lava, etc
	P_WorldEffects( ent );

	// apply all the damage taken this frame
	P_DamageFeedback( ent );

	// add the EF_CONNECTION flag if we haven't gotten commands recently
	if ( level.time - ent->client->lastCmdTime > 1000 && !( ent->r.svFlags & SVF_BOT ) )
	{
		ent->client->ps.eFlags |= EF_CONNECTION;
	}
	else
	{
		ent->client->ps.eFlags &= ~EF_CONNECTION;
	}

	// respawn if dead
	if ( G_Dead( ent ) && level.time >= ent->client->respawnTime )
	{
		respawn( ent );
	}

	G_SetClientSound( ent );

	// set the latest infor
	if ( g_smoothClients.integer )
	{
		BG_PlayerStateToEntityStateExtraPolate( &ent->client->ps, &ent->s, ent->client->ps.commandTime, true );
	}
	else
	{
		BG_PlayerStateToEntityState( &ent->client->ps, &ent->s, true );
	}

	SendPendingPredictableEvents( &ent->client->ps );
}
