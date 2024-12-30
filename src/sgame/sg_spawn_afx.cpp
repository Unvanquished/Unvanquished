/*
===========================================================================

Unvanquished GPL Source Code
Copyright (C) 2012 Unvanquished Developers

This file is part of the Unvanquished GPL Source Code (Unvanquished Source Code).

Unvanquished Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Unvanquished Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Unvanquished Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Unvanquished Source Code is also subject to certain additional terms.
You should have received a copy of these additional terms immediately following the
terms and conditions of the GNU General Public License which accompanied the Unvanquished
Source Code.  If not, please request a copy in writing from id Software at the address
below.

If you have questions concerning this license or the applicable additional terms, you
may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville,
Maryland 20850 USA.

===========================================================================
*/

#include "common/Common.h"
#include "sg_local.h"
#include "sg_spawn.h"
#include "CBSE.h"

static void InitEnvAFXEntity( gentity_t *self, bool link )
{
	if ( !VectorCompare( self->s.angles, vec3_origin ) )
	{
		glm::vec3 angles = VEC2GLM( self->s.angles );
		glm::vec3 movedir = VEC2GLM( self->mapEntity.movedir );
		G_SetMovedir( angles, movedir );
		VectorCopy( angles, self->s.angles );
		VectorCopy( movedir, self->mapEntity.movedir );
	}

	trap_SetBrushModel( self, self->mapEntity.model );
	self->r.contents = CONTENTS_TRIGGER; // replaces the -1 from trap_SetBrushModel
	self->r.svFlags = SVF_NOCLIENT;

	if( link )
	{
		trap_LinkEntity( self );
	}
}

static void env_afx_toggle( gentity_t *self, gentity_t*, gentity_t* )
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

static void env_afx_push_touch( gentity_t *self, gentity_t *activator )
{
	//only triggered by clients
	if ( !activator || !activator->client )
	{
		return;
	}

	if ( activator->client->ps.pm_type != PM_NORMAL )
	{
		return;
	}

	if ( self->nextthink > level.time )
	{
		return;
	}
	self->nextthink = VariatedLevelTime( self->mapEntity.config.wait );

	VectorCopy( self->s.origin2, activator->client->ps.velocity );
}

void SP_env_afx_push( gentity_t *self )
{
	SP_WaitFields(self, 0.5f, 0);

	self->s.eType = entityType_t::ET_PUSHER;
	self->touch = env_afx_push_touch;
	self->think = think_aimAtTarget;
	self->nextthink = level.time + FRAMETIME;
	self->act = env_afx_toggle;

	InitEnvAFXEntity( self, !(self->mapEntity.spawnflags & SPF_SPAWN_DISABLED ) );

	// unlike other afx, we need to send this one to the client
	self->r.svFlags &= ~SVF_NOCLIENT;
}

/*
==============================================================================

trigger_teleport

==============================================================================
*/

static void env_afx_teleporter_touch( gentity_t *self, gentity_t *other )
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
	if ( ( self->mapEntity.spawnflags & 1 ) &&
	     other->client->sess.spectatorState == SPECTATOR_NOT )
	{
		return;
	}

	dest = G_PickRandomTargetFor( self );

	if ( !dest )
		return;

	G_TeleportPlayer( other, VEC2GLM( dest->s.origin ), VEC2GLM( dest->s.angles ), self->mapEntity.config.speed );
}

static void env_afx_teleporter_act( gentity_t *ent, gentity_t*, gentity_t* )
{
	ent->s.eFlags ^= EF_NODRAW;
}

void SP_env_afx_teleport( gentity_t *self )
{

	if( !self->mapEntity.config.speed )
		self->mapEntity.config.speed = 400;

	// SPAWN_DISABLED
	if ( self->mapEntity.spawnflags & 2 )
	{
		self->s.eFlags |= EF_NODRAW;
	}

	self->s.eType = entityType_t::ET_TELEPORTER;
	self->touch = env_afx_teleporter_touch;
	self->act = env_afx_teleporter_act;

	InitEnvAFXEntity( self, true );
	// unlike other afx, we need to send this one to the client
	// unless is a spectator trigger
	if ( self->mapEntity.spawnflags & 1 )
	{
		self->r.svFlags |= SVF_NOCLIENT;
	}
	else
	{
		self->r.svFlags &= ~SVF_NOCLIENT;
	}
}

/*
==============================================================================

trigger_hurt

==============================================================================
*/

static void env_afx_hurt_touch( gentity_t *self, gentity_t *other )
{
	int dflags;

	if ( self->timestamp > level.time )
	{
		return;
	}

	if ( self->mapEntity.spawnflags & 16 )
	{
		self->timestamp = level.time + 1000;
	}
	else
	{
		self->timestamp = level.time + FRAMETIME;
	}

	// play sound
	if ( !( self->mapEntity.spawnflags & 4 ) )
	{
		G_Sound( other, soundChannel_t::CHAN_AUTO, self->mapEntity.soundIndex );
	}

	if ( self->mapEntity.spawnflags & 8 )
	{
		dflags = DAMAGE_NO_PROTECTION;
	}
	else
	{
		dflags = 0;
	}

	other->Damage((float)self->damage, self, Util::nullopt, Util::nullopt, dflags,
	                      MOD_TRIGGER_HURT);
}

void SP_env_afx_hurt( gentity_t *self )
{

	self->mapEntity.soundIndex = G_SoundIndex( "sound/misc/electro" );
	self->touch = env_afx_hurt_touch;

	G_ResetIntField(&self->damage, true, self->mapEntity.config.damage, self->mapEntity.eclass->config.damage, 5);

	self->act = env_afx_toggle;

	InitEnvAFXEntity( self, !(self->mapEntity.spawnflags & SPF_SPAWN_DISABLED ) );
}

/*
=================================================================================

trigger_gravity

=================================================================================
*/

static void env_afx_gravity_reset( gentity_t *self )
{
	G_ResetIntField(&self->mapEntity.amount, false, self->mapEntity.config.amount, self->mapEntity.eclass->config.amount, g_gravity.Get());
}

static void env_afx_gravity_touch( gentity_t *ent, gentity_t *other )
{
	//only triggered by clients
	if ( !other->client )
	{
		return;
	}

	other->client->ps.gravity = ent->mapEntity.amount;
}

/*
===============
SP_trigger_gravity
===============
*/
void SP_env_afx_gravity( gentity_t *self )
{
	int* amount = &self->mapEntity.config.amount;
	self->mapEntity.amount = *amount;
	if ( !( G_SpawnInt( "amount", "0", amount ) || G_SpawnInt( "gravity", "0", amount ) ) )
	{
		//TODO: it is highly possible that other situations are broken.
		// So, this entity needs maps which implements several cases, for
		// both unv and trem formats:
		//
		// * negative gravity/amount
		// * positive gravity/amount
		// * null/0 gravity/amount
		// * missing gravity/amuont
		self->reset = env_afx_gravity_reset;
	}

	self->touch = env_afx_gravity_touch;
	self->act = env_afx_toggle;

	// Remove CONTENTS_SOLID flag.
	self->r.contents &= ~CONTENTS_SOLID;

	InitEnvAFXEntity( self, true );
}

/*
=================================================================================

trigger_heal

=================================================================================
*/

static void env_afx_heal_touch( gentity_t *self, gentity_t *other )
{
	if ( !other->client )
	{
		return;
	}

	if ( self->timestamp > level.time )
	{
		return;
	}

	if ( self->mapEntity.spawnflags & 2 )
	{
		self->timestamp = level.time + 1000;
	}
	else
	{
		self->timestamp = level.time + FRAMETIME;
	}

	other->entity->Heal((float)self->damage, nullptr);
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
		Log::Warn( "trigger_heal with negative damage key" );
	}

	self->touch = env_afx_heal_touch;
	self->act = env_afx_toggle;

	InitEnvAFXEntity( self, !( self->mapEntity.spawnflags & SPF_SPAWN_DISABLED ) );
}

/*
=================================================================================

trigger_ammo

=================================================================================
*/
static void env_afx_ammo_touch( gentity_t *self, gentity_t *other )
{
	int      maxClips, maxAmmo;
	weapon_t weapon;

	if ( !other->client )
	{
		return;
	}

	if ( other->client->pers.team != TEAM_HUMANS )
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

	if ( BG_Weapon( weapon )->usesEnergy && ( self->mapEntity.spawnflags & 2 ) )
	{
		return;
	}

	if ( !BG_Weapon( weapon )->usesEnergy && ( self->mapEntity.spawnflags & 4 ) )
	{
		return;
	}

	if ( self->mapEntity.spawnflags & 1 )
	{
		self->timestamp = level.time + 1000;
	}
	else
	{
		self->timestamp = level.time + FRAMETIME;
	}

	maxAmmo = BG_Weapon( weapon )->maxAmmo;
	maxClips = BG_Weapon( weapon )->maxClips;

	if ( ( other->client->ps.ammo + self->mapEntity.config.amount ) > maxAmmo )
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
		other->client->ps.ammo += self->mapEntity.config.amount;
	}
}

/*
===============
SP_trigger_ammo
===============
*/
void SP_env_afx_ammo( gentity_t *self )
{
	G_SpawnInt( "ammo", "1", &self->mapEntity.config.amount );

	if ( self->mapEntity.config.amount <= 0 )
	{
		self->mapEntity.config.amount = 1;
		Log::Warn( "%s with negative or unset ammo amount key", etos(self) );
	}

	self->touch = env_afx_ammo_touch;
	self->act = env_afx_toggle;

	InitEnvAFXEntity( self, true );
}
