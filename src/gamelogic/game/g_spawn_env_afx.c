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

#include "g_local.h"
#include "g_spawn.h"

void InitEnvAFXEntity( gentity_t *self )
{
	if ( !VectorCompare( self->s.angles, vec3_origin ) )
	{
		G_SetMovedir( self->s.angles, self->movedir );
	}

	trap_SetBrushModel( self, self->model );
	self->r.contents = CONTENTS_TRIGGER; // replaces the -1 from trap_SetBrushModel
	self->r.svFlags = SVF_NOCLIENT;
}

void env_afx_toggle( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	if ( self->r.linked )
	{
		trap_UnlinkEntity( self );
	}
	else
	{
		trap_LinkEntity( self );
	}
}

/*
==============================================================================

trigger_push

==============================================================================
*/

void env_afx_push_touch( gentity_t *self, gentity_t *other, trace_t *trace )
{
}

void SP_env_afx_push( gentity_t *self )
{
	InitEnvAFXEntity( self );

	// unlike other triggers, we need to send this one to the client
	self->r.svFlags &= ~SVF_NOCLIENT;

	self->s.eType = ET_PUSH_TRIGGER;
	self->touch = env_afx_push_touch;
	self->think = think_aimAtTarget;
	self->nextthink = level.time + FRAMETIME;
	trap_LinkEntity( self );
}

/*
==============================================================================

trigger_teleport

==============================================================================
*/

void env_afx_teleporter_touch( gentity_t *self, gentity_t *other, trace_t *trace )
{
	gentity_t *dest;

	if ( self->s.eFlags & EF_NODRAW )
	{
		return;
	}

	if ( !other->client )
	{
		return;
	}

	if ( other->client->ps.pm_type == PM_DEAD )
	{
		return;
	}

	// Spectators only?
	if ( ( self->spawnflags & 1 ) &&
	     other->client->sess.spectatorState == SPECTATOR_NOT )
	{
		return;
	}

	dest = G_PickRandomTargetFor( self );

	if ( !dest )
		return;

	G_TeleportPlayer( other, dest->s.origin, dest->s.angles, self->config.speed );
}

void env_afx_teleporter_use( gentity_t *ent, gentity_t *other, gentity_t *activator )
{
	ent->s.eFlags ^= EF_NODRAW;
}

void SP_env_afx_teleport( gentity_t *self )
{
	InitEnvAFXEntity( self );

	if( !self->config.speed )
		self->config.speed = 400;

	// unlike other triggers, we need to send this one to the client
	// unless is a spectator trigger
	if ( self->spawnflags & 1 )
	{
		self->r.svFlags |= SVF_NOCLIENT;
	}
	else
	{
		self->r.svFlags &= ~SVF_NOCLIENT;
	}

	// SPAWN_DISABLED
	if ( self->spawnflags & 2 )
	{
		self->s.eFlags |= EF_NODRAW;
	}

	self->s.eType = ET_TELEPORT_TRIGGER;
	self->touch = env_afx_teleporter_touch;
	self->use = env_afx_teleporter_use;

	trap_LinkEntity( self );
}

/*
==============================================================================

trigger_hurt

==============================================================================
*/

void env_afx_hurt_touch( gentity_t *self, gentity_t *other, trace_t *trace )
{
	int dflags;

	if ( !other->takedamage )
	{
		return;
	}

	if ( self->timestamp > level.time )
	{
		return;
	}

	if ( self->spawnflags & 16 )
	{
		self->timestamp = level.time + 1000;
	}
	else
	{
		self->timestamp = level.time + FRAMETIME;
	}

	// play sound
	if ( !( self->spawnflags & 4 ) )
	{
		G_Sound( other, CHAN_AUTO, self->soundIndex );
	}

	if ( self->spawnflags & 8 )
	{
		dflags = DAMAGE_NO_PROTECTION;
	}
	else
	{
		dflags = 0;
	}

	G_Damage( other, self, self, NULL, NULL, self->damage, dflags, MOD_TRIGGER_HURT );
}

void SP_env_afx_hurt( gentity_t *self )
{
	InitEnvAFXEntity( self );

	self->soundIndex = G_SoundIndex( "sound/misc/electro.wav" );
	self->touch = env_afx_hurt_touch;

	reset_intField(&self->damage, self->config.damage, self->eclass->config.damage, 5);
	if ( self->damage < 0 )
	{
		self->damage = 5;
	}

	self->use = env_afx_toggle;

	// link in to the world if starting active
	if ( self->spawnflags & 1 )
	{
		trap_UnlinkEntity( self );
	}
	else
	{
		trap_LinkEntity( self );
	}
}

/*
=================================================================================

trigger_gravity

=================================================================================
*/
void env_afx_gravity_touch( gentity_t *ent, gentity_t *other, trace_t *trace )
{
	//only triggered by clients
	if ( !other->client )
	{
		return;
	}

	other->client->ps.gravity = ent->triggerGravity;
}

/*
===============
SP_trigger_gravity
===============
*/
void SP_env_afx_gravity( gentity_t *self )
{
	G_SpawnInt( "gravity", "800", &self->triggerGravity );

	self->touch = env_afx_gravity_touch;
	self->use = env_afx_toggle;

	InitEnvAFXEntity( self );
	trap_LinkEntity( self );
}

/*
=================================================================================

trigger_heal

=================================================================================
*/

void env_afx_heal_touch( gentity_t *self, gentity_t *other, trace_t *trace )
{
	int max;

	if ( !other->client )
	{
		return;
	}

	if ( self->timestamp > level.time )
	{
		return;
	}

	if ( self->spawnflags & 2 )
	{
		self->timestamp = level.time + 1000;
	}
	else
	{
		self->timestamp = level.time + FRAMETIME;
	}

	max = other->client->ps.stats[ STAT_MAX_HEALTH ];

	other->health += self->damage;

	if ( other->health > max )
	{
		other->health = max;
	}

	other->client->ps.stats[ STAT_HEALTH ] = other->health;
}

/*
===============
SP_trigger_heal
===============
*/
void SP_env_afx_heal( gentity_t *self )
{
	G_SpawnInt( "heal", "5", &self->damage );

	if ( self->damage <= 0 )
	{
		self->damage = 1;
		G_Printf( S_COLOR_YELLOW "WARNING: trigger_heal with negative damage key\n" );
	}

	self->touch = env_afx_heal_touch;
	self->use = env_afx_toggle;

	InitEnvAFXEntity( self );

	// link in to the world if starting active
	if ( self->spawnflags & 1 )
	{
		trap_UnlinkEntity( self );
	}
	else
	{
		trap_LinkEntity( self );
	}
}

/*
=================================================================================

trigger_ammo

=================================================================================
*/
void env_afx_ammo_touch( gentity_t *self, gentity_t *other, trace_t *trace )
{
	int      maxClips, maxAmmo;
	weapon_t weapon;

	if ( !other->client )
	{
		return;
	}

	if ( other->client->ps.stats[ STAT_TEAM ] != TEAM_HUMANS )
	{
		return;
	}

	if ( self->timestamp > level.time )
	{
		return;
	}

	if ( other->client->ps.weaponstate != WEAPON_READY )
	{
		return;
	}

	weapon = BG_PrimaryWeapon( other->client->ps.stats );

	if ( BG_Weapon( weapon )->usesEnergy && ( self->spawnflags & 2 ) )
	{
		return;
	}

	if ( !BG_Weapon( weapon )->usesEnergy && ( self->spawnflags & 4 ) )
	{
		return;
	}

	if ( self->spawnflags & 1 )
	{
		self->timestamp = level.time + 1000;
	}
	else
	{
		self->timestamp = level.time + FRAMETIME;
	}

	maxAmmo = BG_Weapon( weapon )->maxAmmo;
	maxClips = BG_Weapon( weapon )->maxClips;

	if ( ( other->client->ps.ammo + self->damage ) > maxAmmo )
	{
		if ( other->client->ps.clips < maxClips )
		{
			other->client->ps.clips++;
			other->client->ps.ammo = 1;
		}
		else
		{
			other->client->ps.ammo = maxAmmo;
		}
	}
	else
	{
		other->client->ps.ammo += self->damage;
	}
}

/*
===============
SP_trigger_ammo
===============
*/
void SP_env_afx_ammo( gentity_t *self )
{
	G_SpawnInt( "ammo", "1", &self->damage );

	if ( self->damage <= 0 )
	{
		self->damage = 1;
		G_Printf( S_COLOR_YELLOW "WARNING: trigger_ammo with negative ammo key\n" );
	}

	self->touch = env_afx_ammo_touch;

	InitEnvAFXEntity( self );
	trap_LinkEntity( self );
}
