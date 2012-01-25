/*
===========================================================================

OpenWolf GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the OpenWolf GPL Source Code (OpenWolf Source Code).  

OpenWolf Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

OpenWolf Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with OpenWolf Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the OpenWolf Source Code is also subject to certain additional terms. 
You should have received a copy of these additional terms immediately following the 
terms and conditions of the GNU General Public License which accompanied the OpenWolf 
Source Code.  If not, please request a copy in writing from id Software at the address 
below.

If you have questions concerning this license or the applicable additional terms, you 
may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, 
Maryland 20850 USA.

===========================================================================
*/

/*
 * name:		g_mover.c
 *
 * desc:
 *
*/

#include "g_local.h"
#ifdef OMNIBOT
#include "../../omnibot/et/g_etbot_interface.h"
#endif

char           *hintStrings[HINT_NUM_HINTS] = {
	"",							// HINT_NONE
	"HINT_NONE",				// actually HINT_FORCENONE, but since this is being specified in the ent, the designer actually means HINT_FORCENONE
	"HINT_PLAYER",
	"HINT_ACTIVATE",
	"HINT_DOOR",
	"HINT_DOOR_ROTATING",
	"HINT_DOOR_LOCKED",
	"HINT_DOOR_ROTATING_LOCKED",
	"HINT_MG42",
	"HINT_BREAKABLE",
	"HINT_BREAKABLE_BIG",
	"HINT_CHAIR",
	"HINT_ALARM",
	"HINT_HEALTH",
	"HINT_TREASURE",
	"HINT_KNIFE",
	"HINT_LADDER",
	"HINT_BUTTON",
	"HINT_WATER",
	"HINT_CAUTION",
	"HINT_DANGER",
	"HINT_SECRET",
	"HINT_QUESTION",
	"HINT_EXCLAMATION",
	"HINT_CLIPBOARD",
	"HINT_WEAPON",
	"HINT_AMMO",
	"HINT_ARMOR",
	"HINT_POWERUP",
	"HINT_HOLDABLE",
	"HINT_INVENTORY",
	"HINT_SCENARIC",
	"HINT_EXIT",
	"HINT_NOEXIT",
	"HINT_PLYR_FRIEND",
	"HINT_PLYR_NEUTRAL",
	"HINT_PLYR_ENEMY",
	"HINT_PLYR_UNKNOWN",
	"HINT_BUILD",				// DHM - Nerve
	"HINT_DISARM",				// DHM - Nerve
	"HINT_REVIVE",				// DHM - Nerve
	"HINT_DYNAMITE",			// DHM - Nerve

	"HINT_CONSTRUCTIBLE",
	"HINT_UNIFORM",
	"HINT_LANDMINE",
	"HINT_TANK",
	"HINT_SATCHELCHARGE",
	// START Mad Doc - TDF
	"HINT_LOCKPICK"
		// END Mad Doc - TDF
		"",						// HINT_BAD_USER
};

/*
===============================================================================

PUSHMOVE

===============================================================================
*/

void            MatchTeam(gentity_t * teamLeader, int moverState, int time);
void            Reached_Train(gentity_t * ent);
void            Think_BeginMoving(gentity_t * ent);
void            Use_Func_Rotate(gentity_t * ent, gentity_t * other, gentity_t * activator);
void            Blocked_Door(gentity_t * ent, gentity_t * other);
void            Blocked_DoorRotate(gentity_t * ent, gentity_t * other);

#define PUSH_STACK_DEPTH 3
int             pushedStackDepth = 0;

typedef struct
{
	gentity_t      *ent;
	vec3_t          origin;
	vec3_t          angles;
	float           deltayaw;
} pushed_t;
pushed_t        pushed[MAX_GENTITIES * PUSH_STACK_DEPTH], *pushed_p;	// Arnout *PUSH_STACK_DEPTH to prevent overflows


/*
============
G_TestEntityPosition
============
*/
gentity_t      *G_TestEntityPosition(gentity_t * ent)
{
	trace_t         tr;
	int             mask;

	if(ent->clipmask)
	{
//      if ( ent->r.contents == CONTENTS_CORPSE && ent->health <= 0 ) { // Arnout: players waiting to be revived are important
//      if ( ent->r.contents == CONTENTS_CORPSE ) {
		// corpse aren't important
		//G_Damage( ent, NULL, NULL, NULL, NULL, 99999, 0, MOD_CRUSH );
//          return NULL;
//      } else {
		mask = ent->clipmask;
//      }

		/*if ( ent->r.contents == CONTENTS_CORPSE ) {
		   return NULL;
		   } else {
		   mask = ent->clipmask;
		   } */
	}
	else
	{
		mask = MASK_SOLID;
	}

	if(ent->client)
	{
		trap_TraceCapsule(&tr, ent->client->ps.origin, ent->r.mins, ent->r.maxs, ent->client->ps.origin, ent->s.number, mask);

		if(!tr.startsolid && ent->client->ps.eFlags & EF_PRONE)
		{
			vec3_t          org, flatforward, point;

			AngleVectors(ent->client->ps.viewangles, flatforward, NULL, NULL);
			flatforward[2] = 0;
			VectorNormalizeFast(flatforward);

			org[0] = ent->client->ps.origin[0] + flatforward[0] * -32;
			org[1] = ent->client->ps.origin[1] + flatforward[1] * -32;
			//org[2] = ent->client->ps.origin[2] + 12;  // 12 units to play with
			org[2] = ent->client->ps.origin[2] + 24.f;	// 12 units to play with

			//VectorSet( point, org[0], org[1], org[2] - 9.6f - 24.f ); // diff between playerlegsMins and playerlegsMaxs z + 24 units to play with
			VectorSet(point, org[0], org[1], org[2] - (24.f - 2.4f) - 24.f);	// diff between playerlegsMins and playerlegsMaxs z + 24 units to play with

			trap_TraceCapsule(&tr, org, playerlegsProneMins, playerlegsProneMaxs, point, ent->s.number, mask);

			if(!tr.startsolid || tr.entityNum < MAX_CLIENTS)
			{
				VectorCopy(tr.endpos, org);
				//VectorSet( point, org[0], org[1], org[2] + 9.6f );
				VectorSet(point, org[0], org[1], org[2] + (24.f - 2.4f));

				trap_TraceCapsule(&tr, org, playerlegsProneMins, playerlegsProneMaxs, point, ent->s.number, mask);

				if(tr.startsolid && tr.entityNum < MAX_CLIENTS)
				{
					tr.startsolid = qfalse;
				}
			}
		}
	}
	else if(ent->s.eType == ET_CORPSE)
	{
		vec3_t          pos;

		VectorCopy(ent->s.pos.trBase, pos);
		pos[2] += 4;			// move up a bit - corpses normally got their origin slightly in the ground
		trap_Trace(&tr, pos, ent->r.mins, ent->r.maxs, pos, ent->s.number, mask);
		// don't crush corpses against players
//      if( tr.startsolid && g_entities[ tr.entityNum ].client )
//          return NULL;
	}
	else if(ent->s.eType == ET_MISSILE)
	{
		trap_Trace(&tr, ent->s.pos.trBase, ent->r.mins, ent->r.maxs, ent->s.pos.trBase, ent->r.ownerNum, mask);
	}
	else
	{
		trap_Trace(&tr, ent->s.pos.trBase, ent->r.mins, ent->r.maxs, ent->s.pos.trBase, ent->s.number, mask);
	}

	if(tr.startsolid)
	{
		return &g_entities[tr.entityNum];
	}

	return NULL;
}

/*
============
G_TestEntityDropToFloor

============
*/
void G_TestEntityDropToFloor(gentity_t * ent, float maxdrop)
{
	trace_t         tr;
	int             mask;
	vec3_t          endpos;

	if(ent->clipmask)
	{
		mask = ent->clipmask;
	}
	else
	{
		mask = MASK_SOLID;
	}
	if(ent->client)
	{
		VectorCopy(ent->client->ps.origin, endpos);
	}
	else
	{
		VectorCopy(ent->s.pos.trBase, endpos);
	}

	endpos[2] -= maxdrop;
	if(ent->client)
	{
		trap_TraceCapsule(&tr, ent->client->ps.origin, ent->r.mins, ent->r.maxs, endpos, ent->s.number, mask);
	}
	else
	{
		trap_Trace(&tr, ent->s.pos.trBase, ent->r.mins, ent->r.maxs, endpos, ent->s.number, mask);
	}

	VectorCopy(tr.endpos, ent->s.pos.trBase);
	if(ent->client)
	{
		VectorCopy(tr.endpos, ent->client->ps.origin);
	}
}

/*
============
G_TestEntityMoveTowardsPos

============
*/
void G_TestEntityMoveTowardsPos(gentity_t * ent, vec3_t pos)
{
	trace_t         tr;
	int             mask;

	if(ent->clipmask)
	{
		mask = ent->clipmask;
	}
	else
	{
		mask = MASK_SOLID;
	}
	if(ent->client)
	{
		trap_TraceCapsule(&tr, ent->client->ps.origin, ent->r.mins, ent->r.maxs, pos, ent->s.number, mask);
	}
	else
	{
		trap_Trace(&tr, ent->s.pos.trBase, ent->r.mins, ent->r.maxs, pos, ent->s.number, mask);
	}

	VectorCopy(tr.endpos, ent->s.pos.trBase);
	if(ent->client)
	{
		VectorCopy(tr.endpos, ent->client->ps.origin);
	}
}

/*
==================
G_TryPushingEntity

Returns qfalse if the move is blocked
==================
*/
qboolean G_TryPushingEntity(gentity_t * check, gentity_t * pusher, vec3_t move, vec3_t amove)
{
	vec3_t          org, org2, move2;
	gentity_t      *block;
	vec3_t          matrix[3], transpose[3];
	float           x, fx, y, fy, z, fz;

#define JITTER_INC  4
#define JITTER_MAX  ( check->r.maxs[0] / 2.0 )

	// EF_MOVER_STOP will just stop when contacting another entity
	// instead of pushing it, but entities can still ride on top of it
	if((pusher->s.eFlags & EF_MOVER_STOP) && check->s.groundEntityNum != pusher->s.number)
	{
		//pusher->s.eFlags |= EF_MOVER_BLOCKED;
		return qfalse;
	}

	// save off the old position
	if(pushed_p > &pushed[MAX_GENTITIES])
	{
		G_Error("pushed_p > &pushed[MAX_GENTITIES]");
	}
	pushed_p->ent = check;
	VectorCopy(check->s.pos.trBase, pushed_p->origin);
	VectorCopy(check->s.apos.trBase, pushed_p->angles);
	if(check->client)
	{
		pushed_p->deltayaw = check->client->ps.delta_angles[YAW];
		VectorCopy(check->client->ps.origin, pushed_p->origin);
	}
	pushed_p++;

	// try moving the contacted entity
	VectorAdd(check->s.pos.trBase, move, check->s.pos.trBase);
	if(check->client)
	{
		// make sure the client's view rotates when on a rotating mover
		// RF, this is done client-side now
		// ydnar: only do this if player is prone or using set mortar
		if((check->client->ps.eFlags & EF_PRONE) || check->s.weapon == WP_MORTAR_SET)
		{
			check->client->ps.delta_angles[YAW] += ANGLE2SHORT(amove[YAW]);
		}
	}

	// figure movement due to the pusher's amove
	BG_CreateRotationMatrix(amove, transpose);
	BG_TransposeMatrix((const vec3_t *)transpose, matrix);
	if(check->client)
	{
		VectorSubtract(check->client->ps.origin, pusher->r.currentOrigin, org);
	}
	else
	{
		VectorSubtract(check->s.pos.trBase, pusher->r.currentOrigin, org);
	}
	VectorCopy(org, org2);
	BG_RotatePoint(org2, (const vec3_t *)matrix);
	VectorSubtract(org2, org, move2);
	VectorAdd(check->s.pos.trBase, move2, check->s.pos.trBase);
	if(check->client)
	{
		VectorAdd(check->client->ps.origin, move, check->client->ps.origin);
		VectorAdd(check->client->ps.origin, move2, check->client->ps.origin);
	}

	// may have pushed them off an edge
	if(check->s.groundEntityNum != pusher->s.number)
	{
		check->s.groundEntityNum = -1;
	}

	block = G_TestEntityPosition(check);
	if(!block)
	{
		// pushed ok
		if(check->client)
		{
			VectorCopy(check->client->ps.origin, check->r.currentOrigin);
		}
		else
		{
			VectorCopy(check->s.pos.trBase, check->r.currentOrigin);
		}
		return qtrue;
	}

	// Arnout, if blocking entity is a player, try to move this player first.

	if(block->client)
	{
		pushedStackDepth++;
		if(pushedStackDepth < PUSH_STACK_DEPTH && G_TryPushingEntity(block, pusher, move, amove))
		{
			// pushed ok
			if(check->client)
			{
				VectorCopy(check->client->ps.origin, check->r.currentOrigin);
			}
			else
			{
				VectorCopy(check->s.pos.trBase, check->r.currentOrigin);
			}
			return qtrue;
		}
		pushedStackDepth--;
	}

	// RF, if still not valid, move them around to see if we can find a good spot
	if(JITTER_MAX > JITTER_INC)
	{
		VectorCopy(check->s.pos.trBase, org);
		if(check->client)
		{
			VectorCopy(check->client->ps.origin, org);
		}
		for(z = 0; z < JITTER_MAX; z += JITTER_INC)
			for(fz = -z; fz <= z; fz += 2 * z)
			{
				for(x = JITTER_INC; x < JITTER_MAX; x += JITTER_INC)
					for(fx = -x; fx <= x; fx += 2 * x)
					{
						for(y = JITTER_INC; y < JITTER_MAX; y += JITTER_INC)
							for(fy = -y; fy <= y; fy += 2 * y)
							{
								VectorSet(move2, fx, fy, fz);
								VectorAdd(org, move2, org2);
								VectorCopy(org2, check->s.pos.trBase);
								if(check->client)
								{
									VectorCopy(org2, check->client->ps.origin);
								}

								//
								// do the test
								block = G_TestEntityPosition(check);
								if(!block)
								{
									// pushed ok
									if(check->client)
									{
										VectorCopy(check->client->ps.origin, check->r.currentOrigin);
									}
									else
									{
										VectorCopy(check->s.pos.trBase, check->r.currentOrigin);
									}
									return qtrue;
								}
							}
					}
				if(!fz)
				{
					break;
				}
			}
		// didnt work, so set the position back
		VectorCopy(org, check->s.pos.trBase);
		if(check->client)
		{
			VectorCopy(org, check->client->ps.origin);
		}
	}

	// if it is ok to leave in the old position, do it
	// this is only relevent for riding entities, not pushed
	// Sliding trapdoors can cause this.
	VectorCopy((pushed_p - 1)->origin, check->s.pos.trBase);
	if(check->client)
	{
		VectorCopy((pushed_p - 1)->origin, check->client->ps.origin);
	}
	VectorCopy((pushed_p - 1)->angles, check->s.apos.trBase);
	block = G_TestEntityPosition(check);
	if(!block)
	{
		check->s.groundEntityNum = -1;
		pushed_p--;
		return qtrue;
	}

	// blocked
	return qfalse;
}

//bani - referenced in G_MoverPush()
extern void     LandMineTrigger(gentity_t * self);

/*
============
G_MoverPush

Objects need to be moved back on a failed push,
otherwise riders would continue to slide.
If qfalse is returned, *obstacle will be the blocking entity
============
*/
qboolean G_MoverPush(gentity_t * pusher, vec3_t move, vec3_t amove, gentity_t ** obstacle)
{
	int             i, e;
	gentity_t      *check;
	vec3_t          mins, maxs;
	pushed_t       *p;
	pushed_t       *work;
	int             entityList[MAX_GENTITIES];
	int             moveList[MAX_GENTITIES];
	int             listedEntities, moveEntities;
	vec3_t          totalMins, totalMaxs;

	*obstacle = NULL;


	// mins/maxs are the bounds at the destination
	// totalMins / totalMaxs are the bounds for the entire move
	if(pusher->r.currentAngles[0] || pusher->r.currentAngles[1] || pusher->r.currentAngles[2] || amove[0] || amove[1] || amove[2])
	{
		float           radius;

		radius = RadiusFromBounds(pusher->r.mins, pusher->r.maxs);
		for(i = 0; i < 3; i++)
		{
			mins[i] = pusher->r.currentOrigin[i] - radius + move[i];
			maxs[i] = pusher->r.currentOrigin[i] + radius + move[i];
			totalMins[i] = pusher->r.currentOrigin[i] - radius;
			totalMaxs[i] = pusher->r.currentOrigin[i] + radius;
		}
	}
	else
	{
		for(i = 0; i < 3; i++)
		{
			mins[i] = pusher->r.absmin[i] + move[i];
			maxs[i] = pusher->r.absmax[i] + move[i];
		}

		VectorCopy(pusher->r.absmin, totalMins);
		VectorCopy(pusher->r.absmax, totalMaxs);
	}
	for(i = 0; i < 3; i++)
	{
		if(move[i] > 0)
		{
			totalMaxs[i] += move[i];
		}
		else
		{
			totalMins[i] += move[i];
		}
	}

	// unlink the pusher so we don't get it in the entityList
	trap_UnlinkEntity(pusher);

	listedEntities = trap_EntitiesInBox(totalMins, totalMaxs, entityList, MAX_GENTITIES);

	// move the pusher to it's final position
	VectorAdd(pusher->r.currentOrigin, move, pusher->r.currentOrigin);
	VectorAdd(pusher->r.currentAngles, amove, pusher->r.currentAngles);
	trap_LinkEntity(pusher);

	moveEntities = 0;
	// see if any solid entities are inside the final position
	for(e = 0; e < listedEntities; e++)
	{
		check = &g_entities[entityList[e]];

		if(check->s.eType == ET_ALARMBOX)
		{
			continue;
		}

		if(check->isProp && check->s.eType == ET_PROP)
		{
			continue;
		}

		// only push items and players
		if(check->s.eType != ET_MISSILE && check->s.eType != ET_ITEM && check->s.eType != ET_PLAYER && !check->physicsObject)
		{
			continue;
		}

		if(check->s.eType == ET_MISSILE && check->s.groundEntityNum != pusher->s.number)
		{
			if(check->methodOfDeath == MOD_LANDMINE)
			{
				if(check->s.teamNum >= 0 && check->s.teamNum < 4)
				{
					LandMineTrigger(check);
				}
			}
			continue;
		}

		if(check->s.eType == ET_PLAYER && check->client &&
		   ((check->client->ps.eFlags & EF_TAGCONNECT) || (check->client->ps.pm_type == PM_NOCLIP)))
		{
			continue;
		}

		//if ( check->s.eType == ET_MISSILE && VectorLengthSquared( check->s.pos.trDelta ) ) {
		//  continue;   // it's moving
		//}

		// if the entity is standing on the pusher, it will definitely be moved
		if(check->s.groundEntityNum != pusher->s.number)
		{
			// see if the ent needs to be tested
			if(check->r.absmin[0] >= maxs[0]
			   || check->r.absmin[1] >= maxs[1]
			   || check->r.absmin[2] >= maxs[2]
			   || check->r.absmax[0] <= mins[0] || check->r.absmax[1] <= mins[1] || check->r.absmax[2] <= mins[2])
			{
				continue;
			}
			// see if the ent's bbox is inside the pusher's final position
			// this does allow a fast moving object to pass through a thin entity...
			if(G_TestEntityPosition(check) != pusher)
			{
				continue;
			}
		}

		moveList[moveEntities++] = entityList[e];
	}

	// unlink all to be moved entities so they cannot get stuck in each other
	for(e = 0; e < moveEntities; e++)
	{
		check = &g_entities[moveList[e]];

		trap_UnlinkEntity(check);
	}

	for(e = 0; e < moveEntities; e++)
	{
		check = &g_entities[moveList[e]];

		// the entity needs to be pushed
		pushedStackDepth = 0;	// Arnout: new push, reset stack depth
		if(G_TryPushingEntity(check, pusher, move, amove))
		{

			// link it in now so nothing else tries to clip into us
			trap_LinkEntity(check);
			continue;
		}

		// the move was blocked an entity

		// bobbing entities are instant-kill and never get blocked
		if(pusher->s.pos.trType == TR_SINE || pusher->s.apos.trType == TR_SINE)
		{
			G_Damage(check, pusher, pusher, NULL, NULL, 99999, 0, MOD_CRUSH);
			continue;
		}


		// save off the obstacle so we can call the block function (crush, etc)
		*obstacle = check;

		// move back any entities we already moved
		// go backwards, so if the same entity was pushed
		// twice, it goes back to the original position
		// rain - changed the loop slightly to avoid checking an invalid
		// pointer (do the -1 inside the loop, > instead of >=)
		for(p = pushed_p; p > pushed; p--)
		{
			work = p - 1;

			VectorCopy(work->origin, work->ent->s.pos.trBase);
			VectorCopy(work->angles, work->ent->s.apos.trBase);
			if(work->ent->client)
			{
				work->ent->client->ps.delta_angles[YAW] = work->deltayaw;
				VectorCopy(work->origin, work->ent->client->ps.origin);
			}
		}

		// link all entities at their original position
		for(e = 0; e < moveEntities; e++)
		{
			check = &g_entities[moveList[e]];

			trap_LinkEntity(check);
		}
		// movement failed
		return qfalse;
	}
	// link all entities at their final position
	for(e = 0; e < moveEntities; e++)
	{
		check = &g_entities[moveList[e]];

		trap_LinkEntity(check);
	}
	// movement was successfull
	return qtrue;
}


/*
=================
G_MoverTeam
=================
*/
void G_MoverTeam(gentity_t * ent)
{
	vec3_t          move, amove;
	gentity_t      *part, *obstacle;
	vec3_t          origin, angles;

	obstacle = NULL;

	// make sure all team slaves can move before commiting
	// any moves or calling any think functions
	// if the move is blocked, all moved objects will be backed out
	pushed_p = pushed;
	for(part = ent; part; part = part->teamchain)
	{
		// get current position
		BG_EvaluateTrajectory(&part->s.pos, level.time, origin, qfalse, ent->s.effect2Time);
		BG_EvaluateTrajectory(&part->s.apos, level.time, angles, qtrue, ent->s.effect2Time);
		VectorSubtract(origin, part->r.currentOrigin, move);
		VectorSubtract(angles, part->r.currentAngles, amove);

		//if (part->s.eFlags == EF_MOVER_STOP)
		//  part->s.eFlags &= ~EF_MOVER_BLOCKED;

		if(part->s.eFlags == EF_MOVER_STOP)
		{
			part->s.eFlags &= ~EF_MOVER_BLOCKED;
		}

		if(!G_MoverPush(part, move, amove, &obstacle))
		{
			break;				// move was blocked
		}
	}

	if(part)
	{
		// go back to the previous position
		for(part = ent; part; part = part->teamchain)
		{
			part->s.pos.trTime += level.time - level.previousTime;
			part->s.apos.trTime += level.time - level.previousTime;
			BG_EvaluateTrajectory(&part->s.pos, level.time, part->r.currentOrigin, qfalse, ent->s.effect2Time);
			BG_EvaluateTrajectory(&part->s.apos, level.time, part->r.currentAngles, qtrue, ent->s.effect2Time);
			trap_LinkEntity(part);
		}

		// if the pusher has a "blocked" function, call it
		if(ent->blocked)
		{
			ent->blocked(ent, obstacle);
		}
		return;
	}

	// the move succeeded
	for(part = ent; part; part = part->teamchain)
	{
		// call the reached function if time is at or past end point

		// opening/closing sliding door
		if(part->s.pos.trType == TR_LINEAR_STOP)
		{
			if(level.time >= part->s.pos.trTime + part->s.pos.trDuration)
			{
				if(part->reached)
				{
					part->reached(part);
				}
			}
		}
//----(SA)  removed
		// opening or closing rotating door
		else if(part->s.apos.trType == TR_LINEAR_STOP)
		{
			if(level.time >= part->s.apos.trTime + part->s.apos.trDuration)
			{
				if(part->reached)
				{
					part->reached(part);
				}
			}
		}
	}
}

/*
================
G_RunMover

================
*/
void G_RunMover(gentity_t * ent)
{
	// if not a team captain, don't do anything, because
	// the captain will handle everything
	if(ent->flags & FL_TEAMSLAVE)
	{
		// FIXME
		// hack to fix problem of tram car slaves being linked
		// after being unlinked in G_FindTeams
		if(ent->r.linked && !Q_stricmp(ent->classname, "func_tramcar"))
		{
			trap_UnlinkEntity(ent);
		}
		// Sigh... need to figure out why re links in
		else if(ent->r.linked && !Q_stricmp(ent->classname, "func_rotating"))
		{
			trap_UnlinkEntity(ent);
		}
		return;
	}

	// if stationary at one of the positions, don't move anything
	if(ent->s.pos.trType != TR_STATIONARY || ent->s.apos.trType != TR_STATIONARY)
	{
		// OSP - pausing
		if(level.match_pause == PAUSE_NONE)
		{
			G_MoverTeam(ent);
		}
		else
		{
			ent->s.pos.trTime += level.time - level.previousTime;
		}
	}

	// check think function
	G_RunThink(ent);
}

/*
============================================================================

GENERAL MOVERS

Doors, plats, and buttons are all binary (two position) movers
Pos1 is "at rest", pos2 is "activated"
============================================================================
*/

/*
===============
SetMoverState
===============
*/
void SetMoverState(gentity_t * ent, moverState_t moverState, int time)
{
	vec3_t          delta;
	float           f;
	qboolean        kicked = qfalse, soft = qfalse;

	kicked = (qboolean) (ent->flags & FL_KICKACTIVATE);
	soft = (qboolean) (ent->flags & FL_SOFTACTIVATE);	//----(SA)    added

	ent->moverState = moverState;
	ent->s.pos.trTime = time;
	ent->s.apos.trTime = time;
	switch (moverState)
	{
		case MOVER_POS1:
			VectorCopy(ent->pos1, ent->s.pos.trBase);
			ent->s.pos.trType = TR_STATIONARY;
			ent->active = qfalse;
			break;
		case MOVER_POS2:
			VectorCopy(ent->pos2, ent->s.pos.trBase);
			ent->s.pos.trType = TR_STATIONARY;
			break;

			// JOSEPH 1-26-00
		case MOVER_POS3:
			VectorCopy(ent->pos3, ent->s.pos.trBase);
			ent->s.pos.trType = TR_STATIONARY;
			break;

		case MOVER_2TO3:
			VectorCopy(ent->pos2, ent->s.pos.trBase);
			VectorSubtract(ent->pos3, ent->pos2, delta);
			f = 1000.0 / ent->s.pos.trDuration;
			VectorScale(delta, f, ent->s.pos.trDelta);
			ent->s.pos.trType = TR_LINEAR_STOP;
			break;
		case MOVER_3TO2:
			VectorCopy(ent->pos3, ent->s.pos.trBase);
			VectorSubtract(ent->pos2, ent->pos3, delta);
			f = 1000.0 / ent->s.pos.trDuration;
			VectorScale(delta, f, ent->s.pos.trDelta);
			ent->s.pos.trType = TR_LINEAR_STOP;
			break;
			// END JOSEPH

		case MOVER_1TO2:		// opening
			VectorCopy(ent->pos1, ent->s.pos.trBase);
			VectorSubtract(ent->pos2, ent->pos1, delta);
//----(SA)  numerous changes start here
			ent->s.pos.trDuration = ent->gDuration;
			f = 1000.0 / ent->s.pos.trDuration;
			VectorScale(delta, f, ent->s.pos.trDelta);
			ent->s.pos.trType = TR_LINEAR_STOP;
#ifdef OMNIBOT
			{
				const char *pName = _GetEntityName(ent);
				if (Q_stricmp(pName, ""))
				{
					Bot_Util_SendTrigger(ent, NULL, va("%s_Moving", pName), "opening");
				}
			}
#endif
			break;

		case MOVER_2TO1:		// closing
			VectorCopy(ent->pos2, ent->s.pos.trBase);
			VectorSubtract(ent->pos1, ent->pos2, delta);
			if(ent->closespeed)
			{					//----(SA)  handle doors with different close speeds
				ent->s.pos.trDuration = ent->gDurationBack;
				f = 1000.0 / ent->gDurationBack;
			}
			else
			{
				ent->s.pos.trDuration = ent->gDuration;
				f = 1000.0 / ent->s.pos.trDuration;
			}
			VectorScale(delta, f, ent->s.pos.trDelta);
			ent->s.pos.trType = TR_LINEAR_STOP;
#ifdef OMNIBOT
			{
				const char *pName = _GetEntityName(ent);
				if (Q_stricmp(pName, ""))
				{
					Bot_Util_SendTrigger(ent, NULL, va("%s_Moving", pName), "closing");
				}
			}
#endif
			break;

		case MOVER_POS1ROTATE:	// at close
			VectorCopy(ent->r.currentAngles, ent->s.apos.trBase);
			ent->s.apos.trType = TR_STATIONARY;
#ifdef OMNIBOT
			{
				const char *pName = _GetEntityName(ent);
				if (Q_stricmp(pName, ""))
				{
					Bot_Util_SendTrigger(ent, NULL, va("%s_Moving", pName), "closed");
				}
			}
#endif
			break;
		case MOVER_POS2ROTATE:	// at open
			VectorCopy(ent->r.currentAngles, ent->s.apos.trBase);
			ent->s.apos.trType = TR_STATIONARY;
#ifdef OMNIBOT
			{
				const char *pName = _GetEntityName(ent);
				if (Q_stricmp(pName, ""))
				{
					Bot_Util_SendTrigger(ent, NULL, va("%s_Moving", pName), "opened");
				}
			}
#endif
			break;
		case MOVER_1TO2ROTATE:	// opening
			VectorClear(ent->s.apos.trBase);	// set base to start position {0,0,0}

			if(kicked)
			{
				f = 2000.0 / ent->gDuration;	// double speed when kicked open
				ent->s.apos.trDuration = ent->gDuration / 2.0;
			}
			else if(soft)
			{
				f = 500.0 / ent->gDuration;	// 1/2 speed when soft opened
				ent->s.apos.trDuration = ent->gDuration * 2;
			}
			else
			{
				f = 1000.0 / ent->gDuration;
//              ent->s.apos.trDuration = ent->gDurationBack;    // (SA) durationback?
				ent->s.apos.trDuration = ent->gDuration;
			}
			VectorScale(ent->rotate, f * ent->angle, ent->s.apos.trDelta);
			ent->s.apos.trType = TR_LINEAR_STOP;
			break;
		case MOVER_2TO1ROTATE:	// closing
			VectorScale(ent->rotate, ent->angle, ent->s.apos.trBase);	// set base to end position
			// (kicked closes same as normally opened)
			// (soft closes at 1/2 speed)
			f = 1000.0 / ent->gDuration;
			ent->s.apos.trDuration = ent->gDuration;
			if(soft)
			{
				ent->s.apos.trDuration *= 2;
				f *= 0.5f;
			}
			VectorScale(ent->s.apos.trBase, -f, ent->s.apos.trDelta);
			ent->s.apos.trType = TR_LINEAR_STOP;
			ent->active = qfalse;
			break;
	}
	BG_EvaluateTrajectory(&ent->s.pos, level.time, ent->r.currentOrigin, qfalse, ent->s.effect2Time);

	if(!(ent->r.svFlags & SVF_NOCLIENT) || (ent->r.contents)) // RF, added this for bats, but this is safe for all movers, since if they aren't solid, and aren't visible to the client, they don't need to be linked
	{
		trap_LinkEntity(ent);
	}
}

/*
================
MatchTeam

All entities in a mover team will move from pos1 to pos2
in the same amount of time
================
*/
void MatchTeam(gentity_t * teamLeader, int moverState, int time)
{
	gentity_t      *slave;

	for(slave = teamLeader; slave; slave = slave->teamchain)
	{

		// pass along flags for how door was activated
		if(teamLeader->flags & FL_KICKACTIVATE)
		{
			slave->flags |= FL_KICKACTIVATE;
		}
		if(teamLeader->flags & FL_SOFTACTIVATE)
		{
			slave->flags |= FL_SOFTACTIVATE;
		}

		SetMoverState(slave, moverState, time);
	}
}

/*
MatchTeamReverseAngleOnSlaves

the activator was blocking the door so reverse its direction
*/
void MatchTeamReverseAngleOnSlaves(gentity_t * teamLeader, int moverState, int time)
{
	gentity_t      *slave;

	for(slave = teamLeader; slave; slave = slave->teamchain)
	{
		// reverse open dir for teamLeader and all slaves
		slave->angle *= -1;

		// pass along flags for how door was activated
		if(teamLeader->flags & FL_KICKACTIVATE)
		{
			slave->flags |= FL_KICKACTIVATE;
		}
		if(teamLeader->flags & FL_SOFTACTIVATE)
		{
			slave->flags |= FL_SOFTACTIVATE;
		}

		SetMoverState(slave, moverState, time);
	}
}

/*
================
ReturnToPos1
================
*/
void ReturnToPos1(gentity_t * ent)
{
	MatchTeam(ent, MOVER_2TO1, level.time);

	// play starting sound
	G_AddEvent(ent, EV_GENERAL_SOUND, ent->sound2to1);

	ent->s.loopSound = 0;
	// set looping sound
	ent->s.loopSound = ent->sound3to2;

}

// JOSEPH 1-26-00
/*
================
ReturnToPos2
================
*/
void ReturnToPos2(gentity_t * ent)
{
	MatchTeam(ent, MOVER_3TO2, level.time);

	ent->s.loopSound = 0;
	// looping sound
	ent->s.loopSound = ent->soundLoop;

	// starting sound
	G_AddEvent(ent, EV_GENERAL_SOUND, ent->sound3to2);
}

/*
================
GotoPos3
================
*/
void GotoPos3(gentity_t * ent)
{
	MatchTeam(ent, MOVER_2TO3, level.time);

	ent->s.loopSound = 0;
	// looping sound
	ent->s.loopSound = ent->soundLoop;

	// starting sound
	G_AddEvent(ent, EV_GENERAL_SOUND, ent->sound2to3);
}

// END JOSEPH

/*
================
ReturnToPos1Rotate
	closing
================
*/
void ReturnToPos1Rotate(gentity_t * ent)
{
	MatchTeam(ent, MOVER_2TO1ROTATE, level.time);

	if(ent->flags & FL_SOFTACTIVATE)
	{
		G_AddEvent(ent, EV_GENERAL_SOUND, ent->soundSoftclose);
	}
	else
	{
		G_AddEvent(ent, EV_GENERAL_SOUND, ent->sound2to1);
	}

	ent->s.loopSound = ent->sound3to2;
}

/*
================
Reached_BinaryMover
================
*/
void Reached_BinaryMover(gentity_t * ent)
{

	// stop the looping sound
	ent->s.loopSound = 0;
//  ent->s.loopSound = ent->soundLoop;

	if(ent->moverState == MOVER_1TO2)
	{
		// reached pos2
		SetMoverState(ent, MOVER_POS2, level.time);

		// play sound
		if(ent->flags & FL_SOFTACTIVATE)
		{
			G_AddEvent(ent, EV_GENERAL_SOUND, ent->soundSoftendo);
		}
		else
		{
			G_AddEvent(ent, EV_GENERAL_SOUND, ent->soundPos2);
		}

		// fire targets
		if(!ent->activator)
		{
			ent->activator = ent;
		}

		G_UseTargets(ent, ent->activator);

		if(ent->flags & FL_TOGGLE)
		{
			ent->think = ReturnToPos1;
			ent->nextthink = 0;
			return;
		}

		// JOSEPH 1-27-00
		// return to pos1 after a delay
		if(ent->wait != -1000)
		{
			ent->think = ReturnToPos1;
			ent->nextthink = level.time + ent->wait;
		}
		// END JOSEPH
	}
	else if(ent->moverState == MOVER_2TO1)
	{
		// reached pos1
		SetMoverState(ent, MOVER_POS1, level.time);

		// play sound
		if(ent->flags & FL_SOFTACTIVATE)
		{
			G_AddEvent(ent, EV_GENERAL_SOUND, ent->soundSoftendc);
		}
		else
		{
			G_AddEvent(ent, EV_GENERAL_SOUND, ent->soundPos1);
		}

		// close areaportals
		if(ent->teammaster == ent || !ent->teammaster)
		{
			trap_AdjustAreaPortalState(ent, qfalse);
		}
	}
	else if(ent->moverState == MOVER_1TO2ROTATE)
	{
		// reached pos2
		SetMoverState(ent, MOVER_POS2ROTATE, level.time);

		// play sound
		if(ent->flags & FL_SOFTACTIVATE)
		{
			G_AddEvent(ent, EV_GENERAL_SOUND, ent->soundSoftendo);
		}
		else
		{
			G_AddEvent(ent, EV_GENERAL_SOUND, ent->soundPos2);
		}

		// fire targets
		if(!ent->activator)
		{
			ent->activator = ent;
		}
		G_UseTargets(ent, ent->activator);

		if(ent->flags & FL_TOGGLE)
		{
			ent->think = ReturnToPos1Rotate;
			ent->nextthink = 0;
			return;
		}

		// return to pos1 after a delay
		ent->think = ReturnToPos1Rotate;
		ent->nextthink = level.time + ent->wait;

	}
	else if(ent->moverState == MOVER_2TO1ROTATE)
	{
		// reached pos1
		SetMoverState(ent, MOVER_POS1ROTATE, level.time);

		// to stop sound from being requested if not in pvs anoying bug
		if(ent->flags & FL_SOFTACTIVATE)
		{
			G_AddEvent(ent, EV_GENERAL_SOUND, ent->soundSoftendc);
		}
		else
		{
			G_AddEvent(ent, EV_GENERAL_SOUND, ent->soundPos1);
		}

		// clear the 'soft' flag
		ent->flags &= ~FL_SOFTACTIVATE;	//----(SA)  added

		// close areaportals
		if(ent->teammaster == ent || !ent->teammaster)
		{
			trap_AdjustAreaPortalState(ent, qfalse);
		}
	}
	else
	{
		G_Error("Reached_BinaryMover: bad moverState");
	}

//  ent->flags &= ~(FL_KICKACTIVATE|FL_SOFTACTIVATE);   // (SA) it was not opened normally.  Clear this so it thinks it's closed normally
	ent->flags &= ~FL_KICKACTIVATE;	// (SA) it was not opened normally.  Clear this so it thinks it's closed normally

}

/*
================
IsBinaryMoverBlocked
================
*/
qboolean IsBinaryMoverBlocked(gentity_t * ent, gentity_t * other, gentity_t * activator)
{

	vec3_t          dir, angles;
	vec3_t          pos;
	vec3_t          vec;
	float           dot;
	vec3_t          forward;
	qboolean        is_relay = qfalse;

	if(Q_stricmp(ent->classname, "func_door_rotating") == 0)
	{
		if(ent->spawnflags & 32)
		{
			return qfalse;
		}

		//----(SA)  only check for blockage by players
		if(!activator)
		{
			if(other && Q_stricmp(other->classname, "target_relay") == 0)
			{
				is_relay = qtrue;
			}
			else				/*if(!activator->client) */
			{
				return qfalse;
			}
		}
		//----(SA)  end

		VectorAdd(ent->r.absmin, ent->r.absmax, pos);
		VectorScale(pos, 0.5, pos);

		VectorSubtract(pos, ent->s.origin, dir);
		vectoangles(dir, angles);

		if(ent->rotate[YAW])
		{
			angles[YAW] += ent->angle;
		}
		else if(ent->rotate[PITCH])
		{
			angles[PITCH] += ent->angle;
		}
		else if(ent->rotate[ROLL])
		{
			angles[ROLL] += ent->angle;
		}

		AngleVectors(angles, forward, NULL, NULL);
		// VectorSubtract (other->r.currentOrigin, pos, vec);

		if(is_relay)
		{
			VectorSubtract(other->r.currentOrigin, pos, vec);
		}
		else
		{
			VectorSubtract(activator->r.currentOrigin, pos, vec);
		}

		VectorNormalize(vec);
		dot = DotProduct(vec, forward);

		if(dot >= 0)
		{
			return qtrue;
		}
		else
		{
			return qfalse;
		}

	}

	return qfalse;

}

// JOSEPH 1-26-00
/*
================
Reached_TrinaryMover
================
*/
void Reached_TrinaryMover(gentity_t * ent)
{

	// stop the looping sound
	ent->s.loopSound = ent->soundLoop;

	if(ent->moverState == MOVER_1TO2)
	{
		// reached pos2
		SetMoverState(ent, MOVER_POS2, level.time);

		// goto pos 3
		ent->think = GotoPos3;
		ent->nextthink = level.time + 1000;	//FRAMETIME;

		// play sound
		G_AddEvent(ent, EV_GENERAL_SOUND, ent->soundPos2);
	}
	else if(ent->moverState == MOVER_2TO1)
	{
		// reached pos1
		SetMoverState(ent, MOVER_POS1, level.time);

		// play sound
		G_AddEvent(ent, EV_GENERAL_SOUND, ent->soundPos1);

		// close areaportals
		if(ent->teammaster == ent || !ent->teammaster)
		{
			trap_AdjustAreaPortalState(ent, qfalse);
		}
	}
	else if(ent->moverState == MOVER_2TO3)
	{
		// reached pos3
		SetMoverState(ent, MOVER_POS3, level.time);

		// play sound
		G_AddEvent(ent, EV_GENERAL_SOUND, ent->soundPos3);

		// return to pos2 after a delay
		if(ent->wait != -1000)
		{
			ent->think = ReturnToPos2;
			ent->nextthink = level.time + ent->wait;
		}

		// fire targets
		if(!ent->activator)
		{
			ent->activator = ent;
		}
		G_UseTargets(ent, ent->activator);
	}
	else if(ent->moverState == MOVER_3TO2)
	{
		// reached pos2
		SetMoverState(ent, MOVER_POS2, level.time);

		// return to pos1
		ent->think = ReturnToPos1;
		ent->nextthink = level.time + 1000;	//FRAMETIME;

		// play sound
		G_AddEvent(ent, EV_GENERAL_SOUND, ent->soundPos3);
	}
	else
	{
		G_Error("Reached_BinaryMover: bad moverState");
	}
}

// END JOSEPH

// JOSEPH 1-26-00
/*
================
Use_TrinaryMover
================
*/
void Use_TrinaryMover(gentity_t * ent, gentity_t * other, gentity_t * activator)
{
	int             total;
	int             partial;
	qboolean        isblocked = qfalse;

	isblocked = IsBinaryMoverBlocked(ent, other, activator);

	if(isblocked)
	{
		MatchTeamReverseAngleOnSlaves(ent, MOVER_1TO2ROTATE, level.time + 50);

		// starting sound
		G_AddEvent(ent, EV_GENERAL_SOUND, ent->sound1to2);

		// looping sound
		ent->s.loopSound = ent->soundLoop;

		// open areaportal
		if(ent->teammaster == ent || !ent->teammaster)
		{
			trap_AdjustAreaPortalState(ent, qtrue);
		}
		return;
	}

	// only the master should be used
	if(ent->flags & FL_TEAMSLAVE)
	{
		Use_TrinaryMover(ent->teammaster, other, activator);
		return;
	}

	ent->activator = activator;

	if(ent->moverState == MOVER_POS1)
	{

		// start moving 50 msec later, becase if this was player
		// triggered, level.time hasn't been advanced yet
		MatchTeam(ent, MOVER_1TO2, level.time + 50);

		// starting sound
		G_AddEvent(ent, EV_GENERAL_SOUND, ent->sound1to2);

		// looping sound
		ent->s.loopSound = ent->soundLoop;

		// open areaportal
		if(ent->teammaster == ent || !ent->teammaster)
		{
			trap_AdjustAreaPortalState(ent, qtrue);
		}
		return;
	}

	if(ent->moverState == MOVER_POS2)
	{

		// start moving 50 msec later, becase if this was player
		// triggered, level.time hasn't been advanced yet
		MatchTeam(ent, MOVER_2TO3, level.time + 50);

		// starting sound
		G_AddEvent(ent, EV_GENERAL_SOUND, ent->sound2to3);

		// looping sound
		ent->s.loopSound = ent->soundLoop;

		return;
	}

	// if all the way up, just delay before coming down
	if(ent->moverState == MOVER_POS3)
	{
		if(ent->wait != -1000)
		{
			ent->nextthink = level.time + ent->wait;
		}
		return;
	}

	// only partway down before reversing
	if(ent->moverState == MOVER_2TO1)
	{
		total = ent->s.pos.trDuration;
		partial = level.time - ent->s.time;
		if(partial > total)
		{
			partial = total;
		}

		MatchTeam(ent, MOVER_1TO2, level.time - (total - partial));

		G_AddEvent(ent, EV_GENERAL_SOUND, ent->sound1to2);
		return;
	}

	if(ent->moverState == MOVER_3TO2)
	{
		total = ent->s.pos.trDuration;
		partial = level.time - ent->s.time;
		if(partial > total)
		{
			partial = total;
		}

		MatchTeam(ent, MOVER_2TO3, level.time - (total - partial));

		G_AddEvent(ent, EV_GENERAL_SOUND, ent->sound2to3);
		return;
	}

	// only partway up before reversing
	if(ent->moverState == MOVER_1TO2)
	{
		total = ent->s.pos.trDuration;
		partial = level.time - ent->s.time;
		if(partial > total)
		{
			partial = total;
		}

		MatchTeam(ent, MOVER_2TO1, level.time - (total - partial));

		if(ent->flags & FL_SOFTACTIVATE)
		{
			G_AddEvent(ent, EV_GENERAL_SOUND, ent->soundSoftclose);
		}
		else
		{
			G_AddEvent(ent, EV_GENERAL_SOUND, ent->sound2to1);
		}
		return;
	}

	if(ent->moverState == MOVER_2TO3)
	{
		total = ent->s.pos.trDuration;
		partial = level.time - ent->s.time;
		if(partial > total)
		{
			partial = total;
		}

		MatchTeam(ent, MOVER_3TO2, level.time - (total - partial));

		G_AddEvent(ent, EV_GENERAL_SOUND, ent->sound3to2);
		return;
	}
}

// END JOSEPH

/*
================
Use_BinaryMover
================
*/
void Use_BinaryMover(gentity_t * ent, gentity_t * other, gentity_t * activator)
{
//  int     total;
//  int     partial;
	qboolean        isblocked = qfalse;
	qboolean        nosound = qfalse;

	if(level.time <= 4000)
	{							// hack.  don't play door sounds if in the first /four/ seconds of game (FIXME: TODO: THIS IS STILL A HACK)
		nosound = qtrue;
	}

	// only the master should be used
	if(ent->flags & FL_TEAMSLAVE)
	{

		// pass along flags for how door was activated
		if(ent->flags & FL_KICKACTIVATE)
		{
			ent->teammaster->flags |= FL_KICKACTIVATE;
		}
		if(ent->flags & FL_SOFTACTIVATE)
		{
			ent->teammaster->flags |= FL_SOFTACTIVATE;
		}

		Use_BinaryMover(ent->teammaster, other, activator);
		return;
	}

#ifdef OMNIBOT
	if(ent->target)
	{
		Bot_Util_SendTrigger(ent, NULL, va("%s activated", ent->target), "pushed");
	}
#endif

	// only check for blocking when opening, otherwise the door has no choice
	if(ent->moverState == MOVER_POS1 || ent->moverState == MOVER_POS1ROTATE)
	{
		isblocked = IsBinaryMoverBlocked(ent, other, activator);
	}


	if(isblocked)
	{
		// start moving 50 msec later, becase if this was player
		// triggered, level.time hasn't been advanced yet
		// ent->angle *= -1;
		// MatchTeam( ent, MOVER_1TO2ROTATE, level.time + 50 );
		MatchTeamReverseAngleOnSlaves(ent, MOVER_1TO2ROTATE, level.time + 50);

		// starting sound
		if(!nosound)
		{
			if(ent->flags & FL_SOFTACTIVATE)
			{
				G_AddEvent(ent, EV_GENERAL_SOUND, ent->soundSoftopen);
			}
			else
			{
//              if(activator)
//                  AICast_AudibleEvent( activator->s.number, ent->s.origin, HEAR_RANGE_DOOR_KICKOPEN );    // "someone kicked open a door near me!"
				G_AddEvent(ent, EV_GENERAL_SOUND, ent->sound1to2);
			}
		}

		ent->s.loopSound = 0;

		// looping sound
		if(!nosound)
		{
			ent->s.loopSound = ent->sound2to3;
		}
		else if(!nosound)
		{
			ent->s.loopSound = ent->soundLoop;
		}

		// open areaportal
		if(ent->teammaster == ent || !ent->teammaster)
		{
			trap_AdjustAreaPortalState(ent, qtrue);
		}
		return;
	}

	ent->activator = activator;

	// Rafael
	if(ent->nextTrain && ent->nextTrain->wait == -1 && ent->nextTrain->count == 1)
	{
		ent->nextTrain->count = 0;
		return;
	}

	if(ent->moverState == MOVER_POS1)
	{

		// start moving 50 msec later, becase if this was player
		// triggered, level.time hasn't been advanced yet
		MatchTeam(ent, MOVER_1TO2, level.time + 50);

		// play starting sound
		if(!nosound)
		{
			G_AddEvent(ent, EV_GENERAL_SOUND, ent->sound1to2);
		}

		ent->s.loopSound = 0;

		// set looping sound
		if(!nosound)
		{
			ent->s.loopSound = ent->sound2to3;
		}

		// open areaportal
		if(ent->teammaster == ent || !ent->teammaster)
		{
			trap_AdjustAreaPortalState(ent, qtrue);
		}
		return;
	}

	if(ent->moverState == MOVER_POS1ROTATE)
	{

		// start moving 50 msec later, becase if this was player
		// triggered, level.time hasn't been advanced yet
		MatchTeam(ent, MOVER_1TO2ROTATE, level.time + 50);

		// play starting sound
		if(!nosound)
		{
			if(ent->flags & FL_SOFTACTIVATE)
			{
				G_AddEvent(ent, EV_GENERAL_SOUND, ent->soundSoftopen);
			}
			else
			{
//              if(activator)
//                  AICast_AudibleEvent( activator->s.number, ent->s.origin, HEAR_RANGE_DOOR_OPEN );    // "someone opened a door near me!"
				G_AddEvent(ent, EV_GENERAL_SOUND, ent->sound1to2);
			}
		}

		ent->s.loopSound = 0;
		// set looping sound
		if(!nosound)
		{
			ent->s.loopSound = ent->sound2to3;
		}

		// open areaportal
		if(ent->teammaster == ent || !ent->teammaster)
		{
			trap_AdjustAreaPortalState(ent, qtrue);
		}
		return;
	}

	// if all the way up, just delay before coming down
	// JOSEPH 1-27-00
	if(ent->moverState == MOVER_POS2)
	{
		if(ent->flags & FL_TOGGLE)
		{
			ent->nextthink = level.time + 50;
			return;
		}

		if(ent->wait != -1000)
		{
			ent->nextthink = level.time + ent->wait;
		}
		return;
	}
	// END JOSEPH

	// if all the way up, just delay before coming down
	if(ent->moverState == MOVER_POS2ROTATE)
	{
		if(ent->flags & FL_TOGGLE)
		{
			ent->nextthink = level.time + 50;	// do it *now* for toggles
		}
		else
		{
			ent->nextthink = level.time + ent->wait;
		}
		return;
	}

	// only partway down before reversing
	if(ent->moverState == MOVER_2TO1)
	{
		Blocked_Door(ent, NULL);

		if(!nosound)
		{
			G_AddEvent(ent, EV_GENERAL_SOUND, ent->sound1to2);
		}
		return;
	}

	// only partway up before reversing
	if(ent->moverState == MOVER_1TO2)
	{
		Blocked_Door(ent, NULL);

		if(!nosound)
		{
			G_AddEvent(ent, EV_GENERAL_SOUND, ent->sound2to1);
		}
		return;
	}

	// only partway closed before reversing
	if(ent->moverState == MOVER_2TO1ROTATE)
	{
		Blocked_DoorRotate(ent, NULL);

		if(!nosound)
		{
			G_AddEvent(ent, EV_GENERAL_SOUND, ent->sound1to2);
		}
		return;
	}

	// only partway open before reversing
	if(ent->moverState == MOVER_1TO2ROTATE)
	{
		Blocked_DoorRotate(ent, NULL);

		if(!nosound)
		{
			if(ent->flags & FL_SOFTACTIVATE)
			{
				G_AddEvent(ent, EV_GENERAL_SOUND, ent->soundSoftclose);
			}
			else
			{
				G_AddEvent(ent, EV_GENERAL_SOUND, ent->sound2to1);
			}
		}
		return;
	}
}



/*
================
InitMover

"pos1", "pos2", and "speed" should be set before calling,
so the movement delta can be calculated
================
*/
void InitMover(gentity_t * ent)
{
	vec3_t          move;
	float           distance;

	// if the "model2" key is set, use a seperate model
	// for drawing, but clip against the brushes
	if(ent->model2)
	{
		ent->s.modelindex2 = G_ModelIndex(ent->model2);
	}

	// JOSEPH 1-26-00
	if(!Q_stricmp(ent->classname, "func_secret"))
	{
		ent->use = Use_TrinaryMover;
		ent->reached = Reached_TrinaryMover;
	}
	else if(!Q_stricmp(ent->classname, "func_rotating"))
	{
		ent->use = Use_Func_Rotate;
		ent->reached = NULL;	// rotating can never reach
	}
	else
	{
		ent->use = Use_BinaryMover;
		ent->reached = Reached_BinaryMover;
	}
	// END JOSEPH

	ent->moverState = MOVER_POS1;
	ent->r.svFlags &= SVF_IGNOREBMODELEXTENTS;
	ent->s.eType = ET_MOVER;

	VectorCopy(ent->pos1, ent->r.currentOrigin);
	trap_LinkEntity(ent);

	ent->s.pos.trType = TR_STATIONARY;
	VectorCopy(ent->pos1, ent->s.pos.trBase);

	// calculate time to reach second position from speed
	VectorSubtract(ent->pos2, ent->pos1, move);
	distance = VectorLength(move);
	if(!ent->speed)
	{
		ent->speed = 100;
	}

	// open time based on speed
	VectorScale(move, ent->speed, ent->gDelta);
	ent->s.pos.trDuration = distance * 1000 / ent->speed;
	if(ent->s.pos.trDuration <= 0)
	{
		ent->s.pos.trDuration = 1;
	}
	ent->gDurationBack = ent->gDuration = ent->s.pos.trDuration;

	// close time based on speed
	if(ent->closespeed)
	{
		VectorScale(move, ent->closespeed, ent->gDelta);
		ent->gDurationBack = distance * 1000 / ent->closespeed;
		if(ent->gDurationBack <= 0)
		{
			ent->gDurationBack = 1;
		}
	}
}

/*
================
InitMoverRotate

"pos1", "pos2", and "speed" should be set before calling,
so the movement delta can be calculated
================
*/
void InitMoverRotate(gentity_t * ent)
{
	vec3_t          move;
	float           distance;
	float           light;
	vec3_t          color;
	qboolean        lightSet, colorSet;

	// if the "model2" key is set, use a seperate model
	// for drawing, but clip against the brushes
	if(ent->model2)
	{
		ent->s.modelindex2 = G_ModelIndex(ent->model2);
	}

	// if the "color" or "light" keys are set, setup constantLight
	lightSet = G_SpawnFloat("light", "100", &light);
	colorSet = G_SpawnVector("color", "1 1 1", color);
	if(lightSet || colorSet)
	{
		int             r, g, b, i;

		r = color[0] * 255;
		if(r > 255)
		{
			r = 255;
		}
		g = color[1] * 255;
		if(g > 255)
		{
			g = 255;
		}
		b = color[2] * 255;
		if(b > 255)
		{
			b = 255;
		}
		i = light / 4;
		if(i > 255)
		{
			i = 255;
		}
		ent->s.constantLight = r | (g << 8) | (b << 16) | (i << 24);
	}


	ent->use = Use_BinaryMover;

	if(!(ent->spawnflags & 64))
	{							// STAYOPEN
		ent->reached = Reached_BinaryMover;
	}

	ent->moverState = MOVER_POS1ROTATE;
	ent->r.svFlags = 0;
	ent->s.eType = ET_MOVER;
	VectorCopy(ent->s.origin, ent->s.pos.trBase);
	VectorCopy(ent->pos1, ent->r.currentOrigin);
	trap_LinkEntity(ent);

	ent->s.pos.trType = TR_STATIONARY;
	VectorCopy(ent->pos1, ent->s.pos.trBase);

	// calculate time to reach second position from speed
	VectorSubtract(ent->pos2, ent->pos1, move);
	distance = VectorLength(move);
	if(!ent->speed)
	{
		ent->speed = 100;
	}

	VectorScale(move, ent->speed, ent->s.pos.trDelta);

	ent->s.apos.trDuration = ent->speed;
	if(ent->s.apos.trDuration <= 0)
	{
		ent->s.apos.trDuration = 1;
	}

	ent->gDuration = ent->gDurationBack = ent->s.apos.trDuration;	// (SA) store 'real' durations so doors can be opened/closed at different speeds
}


/*
===============================================================================

DOOR

A use can be triggered either by a touch function, by being shot, or by being
targeted by another entity.

===============================================================================
*/


/*
================
Blocked_Door
================
*/
void Blocked_Door(gentity_t * ent, gentity_t * other)
{
	gentity_t      *slave;
	int             time;

	// remove anything other than a client
	if(other)
	{
		if(!other->client && other->s.eType != ET_CORPSE)
		{
			// except CTF flags!!!!
			if(other->s.eType == ET_ITEM && other->item->giType == IT_TEAM)
			{
				Team_DroppedFlagThink(other);
				return;
			}
			G_TempEntity(other->s.origin, EV_ITEM_POP);
			G_FreeEntity(other);
			return;
		}

		if(ent->damage)
		{
			G_Damage(other, ent, ent, NULL, NULL, ent->damage, 0, MOD_CRUSH);
		}
	}

	if(ent->spawnflags & 4)
	{
		return;					// crushers don't reverse
	}

	// reverse direction
//  Use_BinaryMover( ent, ent, other );
	for(slave = ent; slave; slave = slave->teamchain)
	{
//      time = level.time - slave->s.pos.trTime;
		time = level.time - (slave->s.pos.trDuration - (level.time - slave->s.pos.trTime));

		if(slave->moverState == MOVER_1TO2)
		{
			SetMoverState(slave, MOVER_2TO1, time);
		}
		else
		{
			SetMoverState(slave, MOVER_1TO2, time);
		}
		trap_LinkEntity(slave);
	}

}

/*
================
Touch_DoorTriggerSpectator
================
*/
static void Touch_DoorTriggerSpectator(gentity_t * ent, gentity_t * other, trace_t * trace)
{
	int             i, axis;
	vec3_t          origin, dir, angles;

	axis = ent->count;
	VectorClear(dir);
	if(fabs(other->s.origin[axis] - ent->r.absmax[axis]) < fabs(other->s.origin[axis] - ent->r.absmin[axis]))
	{
		origin[axis] = ent->r.absmin[axis] - 10;
		dir[axis] = -1;
	}
	else
	{
		origin[axis] = ent->r.absmax[axis] + 10;
		dir[axis] = 1;
	}
	for(i = 0; i < 3; i++)
	{
		if(i == axis)
		{
			continue;
		}
		origin[i] = (ent->r.absmin[i] + ent->r.absmax[i]) * 0.5;
	}
	vectoangles(dir, angles);
	TeleportPlayer(other, origin, angles);
}

/*
================
Blocked_DoorRotate
================
*/

#define DOORPUSHBACK    16

void Blocked_DoorRotate(gentity_t * ent, gentity_t * other)
{

	gentity_t      *slave;
	int             time;

	// remove anything other than a client
	if(other)
	{
		if(!other->client)
		{
			if(other->s.eType == ET_ITEM && other->item->giType == IT_TEAM)
			{
				Team_DroppedFlagThink(other);
				return;
			}
			G_TempEntity(other->s.origin, EV_ITEM_POP);
			G_FreeEntity(other);
			return;
		}

		if(other->health <= 0)
		{
			G_Damage(other, ent, ent, NULL, NULL, 99999, 0, MOD_CRUSH);
		}

		if(ent->damage)
		{
			G_Damage(other, ent, ent, NULL, NULL, ent->damage, 0, MOD_CRUSH);
		}
	}

	for(slave = ent; slave; slave = slave->teamchain)
	{
		// RF, trying to fix "stuck in door" bug
		time = level.time - (slave->s.apos.trDuration - (level.time - slave->s.apos.trTime));
		//time = level.time - slave->s.apos.trTime;

		if(slave->moverState == MOVER_1TO2ROTATE)
		{
			SetMoverState(slave, MOVER_2TO1ROTATE, time);
		}
		else
		{
			SetMoverState(slave, MOVER_1TO2ROTATE, time);
		}
		trap_LinkEntity(slave);
	}


}


/*
================
Touch_DoorTrigger
================
*/
void Touch_DoorTrigger(gentity_t * ent, gentity_t * other, trace_t * trace)
{
	if(other->client && other->client->sess.sessionTeam == TEAM_SPECTATOR)
	{
		// if the door is not open and not opening
		if(ent->parent->moverState != MOVER_1TO2 && ent->parent->moverState != MOVER_POS2)
		{
			Touch_DoorTriggerSpectator(ent, other, trace);
		}
	}
	else if(ent->parent->moverState != MOVER_1TO2)
	{
		Use_BinaryMover(ent->parent, ent, other);
	}
}

/*
======================
Think_SpawnNewDoorTrigger

All of the parts of a door have been spawned, so create
a trigger that encloses all of them
======================
*/
void Think_SpawnNewDoorTrigger(gentity_t * ent)
{
	gentity_t      *other;
	vec3_t          mins, maxs;
	int             i, best;

	// set all of the slaves as shootable
	for(other = ent; other; other = other->teamchain)
	{
		other->takedamage = qtrue;
	}

	// find the bounds of everything on the team
	VectorCopy(ent->r.absmin, mins);
	VectorCopy(ent->r.absmax, maxs);

	for(other = ent->teamchain; other; other = other->teamchain)
	{
		AddPointToBounds(other->r.absmin, mins, maxs);
		AddPointToBounds(other->r.absmax, mins, maxs);
	}

	// find the thinnest axis, which will be the one we expand
	best = 0;
	for(i = 1; i < 3; i++)
	{
		if(maxs[i] - mins[i] < maxs[best] - mins[best])
		{
			best = i;
		}
	}
	maxs[best] += 120;
	mins[best] -= 120;

	// create a trigger with this size
	other = G_Spawn();
	VectorCopy(mins, other->r.mins);
	VectorCopy(maxs, other->r.maxs);
	other->parent = ent;
	other->r.contents = CONTENTS_TRIGGER;
	other->touch = Touch_DoorTrigger;
	trap_LinkEntity(other);

	MatchTeam(ent, ent->moverState, level.time);
}

void Think_MatchTeam(gentity_t * ent)
{
	MatchTeam(ent, ent->moverState, level.time);
}


//----(SA) added

/*
==============
findNonAIBrushTargeter
	determine if there is an entity pointing at ent that is not a "trigger_aidoor"
	(used now for checking which key to set for a door)
==============
*/
qboolean findNonAIBrushTargeter(gentity_t * ent)
{
	gentity_t      *targeter = NULL;

	if(!(ent->targetname))
	{
		return qfalse;
	}

	while((targeter = G_Find(targeter, FOFS(target), ent->targetname)) != NULL)
	{
		if(strcmp(targeter->classname, "trigger_aidoor") && Q_stricmp(targeter->classname, "func_invisible_user"))
		{
			return qtrue;
		}
	}

	return qfalse;
}


/*
==============
finishSpawningKeyedMover
==============
*/
void finishSpawningKeyedMover(gentity_t * ent)
{
	gentity_t      *slave;

	// all ents should be spawned, so it's okay to check for special door triggers now

	// update level.doorAllowTeams
	level.doorAllowTeams |= ent->allowteams;

//----(SA)  modified
	if(ent->key == -2)
	{							// the key was not set in the spawn
		if(ent->targetname && findNonAIBrushTargeter(ent))
		{
			ent->key = -1;		// something is targeting this (other than a trigger_aidoor) so leave locked
		}
		else
		{
			ent->key = 0;
		}
	}
//----(SA)  end

	ent->nextthink = level.time + FRAMETIME;

	if(!(ent->flags & FL_TEAMSLAVE))
	{
		if(ent->targetname || ent->takedamage)
		{						// non touch/shoot doors
			ent->think = Think_MatchTeam;
		}
// (SA) is this safe?  is ent->spawnflags & 8 consistant among all keyed ents?
		else if((ent->spawnflags & 8) && (strcmp(ent->classname, "func_door_rotating")))
		{
			ent->think = Think_SpawnNewDoorTrigger;
		}
		else
		{
			ent->think = Think_MatchTeam;
		}

		// (SA) slaves have been marked as FL_TEAMSLAVE now, so they won't
		// finish their think on their own.  So set keys for teamed doors
		for(slave = ent; slave; slave = slave->teamchain)
		{
			if(slave == ent)
			{
				continue;
			}

			slave->key = ent->key;
		}
	}
}

//----(SA) end



/*
==============
Door_reverse_sounds
	The door has been marked as "START_OPEN" which means the open/closed
	positions have been swapped.
	This swaps the sounds around as well
==============
*/
void Door_reverse_sounds(gentity_t * ent)
{
	int             stemp;

	stemp = ent->sound1to2;
	ent->sound1to2 = ent->sound2to1;
	ent->sound2to1 = stemp;

	stemp = ent->soundPos1;
	ent->soundPos1 = ent->soundPos2;
	ent->soundPos2 = stemp;

	stemp = ent->sound2to3;
	ent->sound2to3 = ent->sound3to2;
	ent->sound3to2 = stemp;


	stemp = ent->soundSoftopen;
	ent->soundSoftopen = ent->soundSoftclose;
	ent->soundSoftclose = stemp;

	stemp = ent->soundSoftendo;
	ent->soundSoftendo = ent->soundSoftendc;
	ent->soundSoftendc = stemp;

}


/*
==============
DoorSetSounds
	get sound indexes for the various door sounds
	(used by SP_func_door() and SP_func_door_rotating() )
==============
*/
void DoorSetSounds(gentity_t * ent, int doortype, qboolean isRotating)
{
	ent->sound1to2 = G_SoundIndex(va("sound/movers/doors/door%i_open.wav", doortype));	// opening
	ent->soundPos2 = G_SoundIndex(va("sound/movers/doors/door%i_endo.wav", doortype));	// open
	ent->sound2to1 = G_SoundIndex(va("sound/movers/doors/door%i_close.wav", doortype));	// closing
	ent->soundPos1 = G_SoundIndex(va("sound/movers/doors/door%i_endc.wav", doortype));	// closed
	ent->sound2to3 = G_SoundIndex(va("sound/movers/doors/door%i_loopo.wav", doortype));	// loopopen
	ent->sound3to2 = G_SoundIndex(va("sound/movers/doors/door%i_loopc.wav", doortype));	// loopclosed
	ent->soundPos3 = G_SoundIndex(va("sound/movers/doors/door%i_locked.wav", doortype));	// locked


	ent->soundSoftopen = G_SoundIndex(va("sound/movers/doors/door%i_openq.wav", doortype));	// opening quietly
	ent->soundSoftendo = G_SoundIndex(va("sound/movers/doors/door%i_endoq.wav", doortype));	// open quietly
	ent->soundSoftclose = G_SoundIndex(va("sound/movers/doors/door%i_closeq.wav", doortype));	// closing quietly
	ent->soundSoftendc = G_SoundIndex(va("sound/movers/doors/door%i_endcq.wav", doortype));	// closed quietly
}

//----(SA)  added
/*
==============
G_TryDoor
	seemed better to have this isolated.  this way i can get func_invisible_user's using the
	regular rules of doors.
==============
*/
void G_TryDoor(gentity_t * ent, gentity_t * other, gentity_t * activator)
{
	qboolean        walking = qfalse;

	walking = (qboolean) (ent->flags & FL_SOFTACTIVATE);


	if((ent->s.apos.trType == TR_STATIONARY && ent->s.pos.trType == TR_STATIONARY))
	{
		if(ent->active == qfalse)
		{
			if(ent->key < 0
			   || !G_AllowTeamsAllowed(ent, activator) || (G_IsSinglePlayerGame() && ent->key == KEY_LOCKED_PICKABLE))
			{					// door force locked
//              if(!walking && activator)   // only send audible event if not trying to open slowly
//                  AICast_AudibleEvent( activator->s.clientNum, ent->s.origin, HEAR_RANGE_DOOR_LOCKED );   // "someone tried locked door near me!"
				G_AddEvent(ent, EV_GENERAL_SOUND, ent->soundPos3);
				return;
			}

/*			// TAT 2/3/2003 - want keys in SP
			if (G_IsSinglePlayerGame()
				&& activator
				// door requires key
				&& ent->key > 0)
			{
				gitem_t *item = BG_FindItemForKey(ent->key, 0);
				if(!(activator->client->ps.stats[STAT_KEYS] & (1<<item->giTag)))
				{
//					if(!walking)	// only send audible event if not trying to open slowly
//						AICast_AudibleEvent( activator->s.clientNum, ent->s.origin, HEAR_RANGE_DOOR_LOCKED );	// "someone tried locked door near me!"
					// player does not have key
					G_AddEvent( ent, EV_GENERAL_SOUND, ent->soundPos3);
					return;
				}
			}
*/

			if(ent->teammaster && ent->team && ent != ent->teammaster)
			{
				ent->teammaster->active = qtrue;
				if(walking)
				{
					ent->teammaster->flags |= FL_SOFTACTIVATE;	// no noise generated
				}
				else
				{
//                  if(activator)
//                      AICast_AudibleEvent( activator->s.clientNum, ent->s.origin, HEAR_RANGE_DOOR_OPEN ); // "someone opened door near me!"
				}

				Use_BinaryMover(ent->teammaster, activator, activator);
				G_UseTargets(ent->teammaster, activator);
			}
			else
			{
				ent->active = qtrue;
				if(walking)
				{
					ent->flags |= FL_SOFTACTIVATE;	// no noise
				}
				else
				{
//                  if(activator)
//                      AICast_AudibleEvent( activator->s.clientNum, ent->s.origin, HEAR_RANGE_DOOR_OPEN ); // "someone opened door near me!"
				}

				Use_BinaryMover(ent, activator, activator);
				G_UseTargets(ent, activator);
			}
		}
	}
}

//----(SA)  end

/*QUAKED func_door (0 .5 .8) ? START_OPEN TOGGLE CRUSHER TOUCH SHOOT-THRU
TOGGLE      wait in both the start and end states for a trigger event.
START_OPEN  the door to moves to its destination when spawned, and operate in reverse.  It is used to temporarily or permanently close off an area when triggered (not useful for touch or takedamage doors).
NOMONSTER   monsters will not trigger this door

"key"       -1 for locked, key number for which key opens, 0 for open.  default '0' unless door is targeted.  (trigger_aidoor entities targeting this door do /not/ affect the key status)
"model2"    .md3 model to also draw
"angle"	    determines the opening direction
"targetname" if set, no touch field will be spawned and a remote button or trigger field activates the door.
"speed"	    movement speed (100 default)
"closespeed" optional different movement speed for door closing
"wait"      wait before returning (3 default, -1 = never return)
"lip"       lip remaining at end of move (8 default)
"dmg"       damage to inflict when blocked (2 default)
"color"     constantLight color
"light"     constantLight radius
"health"    if set, the door must be shot open
"team"		team name.  other doors with same team name will open/close in syncronicity
"lockpickTime" time to unpick the lock on a locked door - use with key set to 99
"type"		use sounds based on construction of door:
	 0 - nosound (default)
	 1 - metal
	 2 - stone
	 3 - lab
	 4 - wood
	 5 - iron/jail
	 6 - portcullis
	 7 - wood (quiet)

SOUND NAMING INFO -
inside "sound/movers/doors/door<number>...
	_open.wav		// opening
	_endo.wav		// open
	_close.wav		// closing
	_endc.wav		// closed
	_loopo.wav		// opening loop
	_loopc.wav		// closing loop
	_locked.wav		// locked

	_openq.wav		// opening quietly
	_endoq.wav		// open quietly
	_closeq.wav		// closing quietly
	_endcq.wav		// closed quietly

and for rotating doors:
	_kicked.wav
	_kickedend.wav

*/
void SP_func_door(gentity_t * ent)
{
	vec3_t          abs_movedir;
	float           distance;
	vec3_t          size;
	float           lip;
	int             key, doortype;

	G_SpawnInt("type", "0", &doortype);

	if(doortype)
	{							// /*why on earthy did this check for <=8?*/ && doortype <= 8)    // no doortype = silent
		DoorSetSounds(ent, doortype, qfalse);
	}

	ent->blocked = Blocked_Door;

	// default speed of 400
	if(!ent->speed)
	{
		ent->speed = 400;
	}

	// default wait of 2 seconds
	if(!ent->wait)
	{
		ent->wait = 2;
	}
	ent->wait *= 1000;

	//---- (SA) door keys

	if(G_SpawnInt("key", "", &key))
	{							// if door has a key entered, set it
		ent->key = key;
	}
	else
	{
		ent->key = -2;			// otherwise, set the key when this ent finishes spawning


	}

	// if the key is invalid, set the key in the finishSpawning routine
	if(ent->key > KEY_NUM_KEYS || ent->key < -2)
	{
		G_Error("invalid key number: %d in func_door_rotating\n", ent->key);
		ent->key = -2;
	}
	//---- (SA) end

	// default lip of 8 units
	G_SpawnFloat("lip", "8", &lip);

	// default damage of 2 points
	G_SpawnInt("dmg", "2", &ent->damage);

	// first position at start
	VectorCopy(ent->s.origin, ent->pos1);

	// calculate second position
	trap_SetBrushModel(ent, ent->model);
#ifdef USE_PHYSICS
	ent->r.svFlags |= SVF_RIGID_BODY;
	trap_AddPhysicsStatic(ent);
#else
	G_SetMovedir(ent->s.angles, ent->movedir);
	abs_movedir[0] = fabs(ent->movedir[0]);
	abs_movedir[1] = fabs(ent->movedir[1]);
	abs_movedir[2] = fabs(ent->movedir[2]);
	VectorSubtract(ent->r.maxs, ent->r.mins, size);
	distance = DotProduct(abs_movedir, size) - lip;
	VectorMA(ent->pos1, distance, ent->movedir, ent->pos2);
#endif
	
	if(ent->spawnflags & 1)
	{							// START_OPEN - reverse position 1 and 2
		vec3_t          temp;
		int             tempi;

		VectorCopy(ent->pos2, temp);
		VectorCopy(ent->s.origin, ent->pos2);
		VectorCopy(temp, ent->pos1);

		// swap speeds if door has 'closespeed'
		if(ent->closespeed)
		{
			tempi = ent->speed;
			ent->speed = ent->closespeed;
			ent->closespeed = tempi;
		}

		// swap sounds
		Door_reverse_sounds(ent);
	}

	// TOGGLE
	if(ent->spawnflags & 2)
	{
		ent->flags |= FL_TOGGLE;
	}

	InitMover(ent);
	ent->s.eFlags = ET_MOVER;

	if(!ent->allowteams)
	{
		ent->s.dmgFlags = HINT_DOOR;	// make it a door for cursorhints
	}

	if(!(ent->flags & FL_TEAMSLAVE))
	{
		int             health;

		G_SpawnInt("health", "0", &health);
		if(health)
		{
			ent->takedamage = qtrue;
		}
	}

	ent->nextthink = level.time + FRAMETIME;
	ent->think = finishSpawningKeyedMover;
}

// JOSEPH 1-26-00
/*QUAKED func_secret (0 .5 .8) ? REVERSE x CRUSHER TOUCH
TOGGLE      wait in both the start and end states for a trigger event.
START_OPEN  the door to moves to its destination when spawned, and operate in reverse.  It is used to temporarily or permanently close off an area when triggered (not useful for touch or takedamage doors).
NOMONSTER   monsters will not trigger this door

"key"       -1 for locked, key number for which key opens, 0 for open.  default '0' unless door is targeted.  (trigger_aidoor entities targeting this door do /not/ affect the key status)
"model2"    .md3 model to also draw
"angle"	    determines the opening direction
"targetname" if set, no touch field will be spawned and a remote button or trigger field activates the door.
"speed"	    movement speed (100 default)
"wait"      wait before returning (2 default, -1 = never return)
"lip"       lip remaining at end of move (8 default)
"dmg"       damage to inflict when blocked (2 default)
"color"     constantLight color
"light"     constantLight radius
"health"    if set, the door must be shot open
"lockpickTime" time to unpick the lock on a locked door - use with key set to 99
*/
void SP_func_secret(gentity_t * ent)
{
	vec3_t          abs_movedir;
	vec3_t          angles2;
	float           distance;
	vec3_t          size;
	float           lip;
	int             key;

	ent->sound1to2 = ent->sound2to1 = ent->sound2to3 = G_SoundIndex("sound/movers/doors/dr1_strt.wav");
	ent->soundPos1 = ent->soundPos3 = G_SoundIndex("sound/movers/doors/dr1_end.wav");

	ent->blocked = Blocked_Door;

	// default speed of 100
	if(!ent->speed)
	{
		ent->speed = 100;
	}

	// default wait of 2 seconds
	if(!ent->wait)
	{
		ent->wait = 2;
	}
	ent->wait *= 1000;

	//---- (SA) door keys

	if(G_SpawnInt("key", "", &key))
	{							// if door has a key entered, set it
		ent->key = key;
	}
	else
	{
		ent->key = -1;			// otherwise, set the key when this ent finishes spawning

	}

	// if the key is invalid, set the key in the finishSpawning routine
	if(ent->key > KEY_NUM_KEYS || ent->key < -1)
	{
		G_Error("invalid key number: %d in func_door_rotating\n", ent->key);
		ent->key = -1;
	}
	//---- (SA) end

	// default lip of 8 units
	G_SpawnFloat("lip", "8", &lip);

	// default damage of 2 points
	G_SpawnInt("dmg", "2", &ent->damage);

	// first position at start
	VectorCopy(ent->s.origin, ent->pos1);

	VectorCopy(ent->s.angles, angles2);

	if(ent->spawnflags & 1)
	{
		angles2[1] -= 90;
	}
	else
	{
		angles2[1] += 90;
	}

	// calculate second position
	trap_SetBrushModel(ent, ent->model);
	G_SetMovedir(ent->s.angles, ent->movedir);
	abs_movedir[0] = fabs(ent->movedir[0]);
	abs_movedir[1] = fabs(ent->movedir[1]);
	abs_movedir[2] = fabs(ent->movedir[2]);
	VectorSubtract(ent->r.maxs, ent->r.mins, size);
	distance = DotProduct(abs_movedir, size) - lip;
	VectorMA(ent->pos1, distance, ent->movedir, ent->pos2);

	// calculate third position
	G_SetMovedir(angles2, ent->movedir);
	abs_movedir[0] = fabs(ent->movedir[0]);
	abs_movedir[1] = fabs(ent->movedir[1]);
	abs_movedir[2] = fabs(ent->movedir[2]);
	VectorSubtract(ent->r.maxs, ent->r.mins, size);
	distance = DotProduct(abs_movedir, size) - lip;
	VectorMA(ent->pos2, distance, ent->movedir, ent->pos3);

	// if "start_open", reverse position 1 and 3
	/*if ( ent->spawnflags & 1 ) {
	   vec3_t   temp;

	   VectorCopy( ent->pos3, temp );
	   VectorCopy( ent->s.origin, ent->pos3 );
	   VectorCopy( temp, ent->pos1 );
	   } */

	InitMover(ent);

	if(!(ent->flags & FL_TEAMSLAVE))
	{
		int             health;

		G_SpawnInt("health", "0", &health);
		if(health)
		{
			ent->takedamage = qtrue;
		}
	}

	ent->nextthink = level.time + FRAMETIME;
	ent->think = finishSpawningKeyedMover;
}

// END JOSEPH

/*
===============================================================================

PLAT

===============================================================================
*/

/*
==============
Touch_Plat

Don't allow decent if a living player is on it
===============
*/
void Touch_Plat(gentity_t * ent, gentity_t * other, trace_t * trace)
{
	if(!other->client || other->client->ps.stats[STAT_HEALTH] <= 0)
	{
		return;
	}

	// delay return-to-pos1 by one second
	if(ent->moverState == MOVER_POS2)
	{
		ent->nextthink = level.time + 1000;
	}
}

/*
==============
Touch_PlatCenterTrigger

If the plat is at the bottom position, start it going up
===============
*/
void Touch_PlatCenterTrigger(gentity_t * ent, gentity_t * other, trace_t * trace)
{
	if(!other->client)
	{
		return;
	}

	if(ent->parent->moverState == MOVER_POS1)
	{
		Use_BinaryMover(ent->parent, ent, other);
	}
}


/*
================
SpawnPlatTrigger

Spawn a trigger in the middle of the plat's low position
Elevator cars require that the trigger extend through the entire low position,
not just sit on top of it.
================
*/
void SpawnPlatTrigger(gentity_t * ent)
{
	gentity_t      *trigger;
	vec3_t          tmin, tmax;

	// the middle trigger will be a thin trigger just
	// above the starting position
	trigger = G_Spawn();
	trigger->touch = Touch_PlatCenterTrigger;
	trigger->r.contents = CONTENTS_TRIGGER;
	trigger->parent = ent;

	tmin[0] = ent->pos1[0] + ent->r.mins[0] + 33;
	tmin[1] = ent->pos1[1] + ent->r.mins[1] + 33;
	tmin[2] = ent->pos1[2] + ent->r.mins[2];

	tmax[0] = ent->pos1[0] + ent->r.maxs[0] - 33;
	tmax[1] = ent->pos1[1] + ent->r.maxs[1] - 33;
	tmax[2] = ent->pos1[2] + ent->r.maxs[2] + 8;

	if(tmax[0] <= tmin[0])
	{
		tmin[0] = ent->pos1[0] + (ent->r.mins[0] + ent->r.maxs[0]) * 0.5;
		tmax[0] = tmin[0] + 1;
	}
	if(tmax[1] <= tmin[1])
	{
		tmin[1] = ent->pos1[1] + (ent->r.mins[1] + ent->r.maxs[1]) * 0.5;
		tmax[1] = tmin[1] + 1;
	}

	VectorCopy(tmin, trigger->r.mins);
	VectorCopy(tmax, trigger->r.maxs);

	trap_LinkEntity(trigger);
}


/*QUAKED func_plat (0 .5 .8) ?
Plats are always drawn in the extended position so they will light correctly.

"lip"		default 8, protrusion above rest position
"height"	total height of movement, defaults to model height
"speed"		overrides default 200.
"dmg"		overrides default 2
"model2"	.md3 model to also draw
"color"		constantLight color
"light"		constantLight radius
*/
void SP_func_plat(gentity_t * ent)
{
	float           lip, height;

	ent->sound1to2 = ent->sound2to1 = G_SoundIndex("sound/movers/plats/pt1_strt.wav");
	ent->soundPos1 = ent->soundPos2 = G_SoundIndex("sound/movers/plats/pt1_end.wav");

	VectorClear(ent->s.angles);

	G_SpawnFloat("speed", "200", &ent->speed);
	G_SpawnInt("dmg", "2", &ent->damage);
	G_SpawnFloat("wait", "1", &ent->wait);
	G_SpawnFloat("lip", "8", &lip);

	ent->wait = 1000;

	// create second position
	trap_SetBrushModel(ent, ent->model);

	if(!G_SpawnFloat("height", "0", &height))
	{
		height = (ent->r.maxs[2] - ent->r.mins[2]) - lip;
	}

	// pos1 is the rest (bottom) position, pos2 is the top
	VectorCopy(ent->s.origin, ent->pos2);
	VectorCopy(ent->pos2, ent->pos1);
	ent->pos1[2] -= height;

	InitMover(ent);

	// touch function keeps the plat from returning while
	// a live player is standing on it
	ent->touch = Touch_Plat;

	ent->blocked = Blocked_Door;

	ent->parent = ent;			// so it can be treated as a door

	// spawn the trigger if one hasn't been custom made
	if(!ent->targetname)
	{
		SpawnPlatTrigger(ent);
	}
}


/*
===============================================================================

BUTTON

===============================================================================
*/

/*
==============
Touch_Button

===============
*/
void Touch_Button(gentity_t * ent, gentity_t * other, trace_t * trace)
{
	if(!other->client)
	{
		return;
	}

	if(ent->moverState == MOVER_POS1)
	{
		Use_BinaryMover(ent, other, other);
	}
}


/*QUAKED func_button (0 .5 .8) ? x x x TOUCH x x STAYOPEN
When a button is touched, it moves some distance in the direction of it's angle, triggers all of it's targets, waits some time, then returns to it's original position where it can be triggered again.

"model2"	.md3 model to also draw
"angle"		determines the opening direction
"target"	all entities with a matching targetname will be used
"speed"		override the default 40 speed
"wait"		override the default 1 second wait (-1 = never return)
"lip"		override the default 4 pixel lip remaining at end of move
"health"	if set, the button must be killed instead of touched
"color"		constantLight color
"light"		constantLight radius
*/
void SP_func_button(gentity_t * ent)
{
	vec3_t          abs_movedir;
	float           distance;
	vec3_t          size;
	float           lip;

	ent->sound1to2 = G_SoundIndex("sound/movers/switches/butn2.wav");

	if(!ent->speed)
	{
		ent->speed = 40;
	}

	if(!ent->wait)
	{
		ent->wait = 1;
	}
	ent->wait *= 1000;

	// first position
	VectorCopy(ent->s.origin, ent->pos1);

	// calculate second position
	trap_SetBrushModel(ent, ent->model);

	G_SpawnFloat("lip", "4", &lip);

	G_SetMovedir(ent->s.angles, ent->movedir);
	abs_movedir[0] = fabs(ent->movedir[0]);
	abs_movedir[1] = fabs(ent->movedir[1]);
	abs_movedir[2] = fabs(ent->movedir[2]);
	VectorSubtract(ent->r.maxs, ent->r.mins, size);
	distance = abs_movedir[0] * size[0] + abs_movedir[1] * size[1] + abs_movedir[2] * size[2] - lip;
	VectorMA(ent->pos1, distance, ent->movedir, ent->pos2);

	if(ent->health)
	{
		// shootable button
		ent->takedamage = qtrue;
	}
	else if(ent->spawnflags & 8)
	{
		// touchable button
		ent->touch = Touch_Button;
	}

	InitMover(ent);
}



/*
===============================================================================

TRAIN

===============================================================================
*/


#define TRAIN_START_ON      1
#define TRAIN_TOGGLE        2
#define TRAIN_BLOCK_STOPS   4

/*
===============
Think_BeginMoving

The wait time at a corner has completed, so start moving again
===============
*/
void Think_BeginMoving(gentity_t * ent)
{
	ent->s.pos.trTime = level.time;
	ent->s.pos.trType = TR_LINEAR_STOP;
}

/*
===============
Reached_Train
===============
*/
void Reached_Train(gentity_t * ent)
{
	gentity_t      *next;
	float           speed;
	vec3_t          move;
	float           length;

	// copy the apropriate values
	next = ent->nextTrain;
	if(!next || !next->nextTrain)
	{
		return;					// just stop
	}

	// Rafael
	if(next->wait == -1 && next->count)
	{
		return;
	}

	// fire all other targets
	G_UseTargets(next, NULL);

	// set the new trajectory
	ent->nextTrain = next->nextTrain;

	if(next->wait == -1)
	{
		next->count = 1;
	}

	VectorCopy(next->s.origin, ent->pos1);
	VectorCopy(next->nextTrain->s.origin, ent->pos2);

	// if the path_corner has a speed, use that
	if(next->speed)
	{
		speed = next->speed;
	}
	else
	{
		// otherwise use the train's speed
		speed = ent->speed;
	}
	if(speed < 1)
	{
		speed = 1;
	}

	// calculate duration
	VectorSubtract(ent->pos2, ent->pos1, move);
	length = VectorLength(move);

	ent->s.pos.trDuration = length * 1000 / speed;
	ent->gDuration = ent->s.pos.trDuration;

	// looping sound
	ent->s.loopSound = next->soundLoop;

	// start it going
	SetMoverState(ent, MOVER_1TO2, level.time);

	// if there is a "wait" value on the target, don't start moving yet
	if(next->wait)
	{
		ent->nextthink = level.time + next->wait * 1000;
		ent->think = Think_BeginMoving;
		ent->s.pos.trType = TR_STATIONARY;
	}
}


/*
===============
Think_SetupTrainTargets

Link all the corners together
===============
*/
void Think_SetupTrainTargets(gentity_t * ent)
{
	gentity_t      *path, *next, *start;

	ent->nextTrain = G_FindByTargetname(NULL, ent->target);
	if(!ent->nextTrain)
	{
		G_Printf("func_train at %s with an unfound target\n", vtos(ent->r.absmin));
		return;
	}

	start = NULL;

	for(path = ent->nextTrain; !path->nextTrain; path = next)
	{

		if(!start)
		{
			start = path;
		}

		if(!path->target)
		{
			G_Printf("Train corner at %s without a target\n", vtos(path->s.origin));
			return;
		}

		// find a path_corner among the targets
		// there may also be other targets that get fired when the corner
		// is reached
		next = NULL;
		do
		{
			next = G_FindByTargetname(next, path->target);
			if(!next)
			{
				G_Printf("Train corner at %s without a target path_corner\n", vtos(path->s.origin));
				return;
			}
		} while(strcmp(next->classname, "path_corner"));

		path->nextTrain = next;
	}

	if(!Q_stricmp(ent->classname, "func_train") && ent->spawnflags & 2)
	{							// TOGGLE
		VectorCopy(ent->nextTrain->s.origin, ent->s.pos.trBase);
		VectorCopy(ent->nextTrain->s.origin, ent->r.currentOrigin);
		trap_LinkEntity(ent);
	}
	else
	{
		Reached_Train(ent);
	}
}



/*QUAKED path_corner (.5 .3 0) (-8 -8 -8) (8 8 8) STOP END REVERSE
Train path corners.
Target: next path corner and other targets to fire
"speed" speed to move to the next corner
"wait" seconds to wait before behining move to next corner

"count2" used only in conjunction with the truck_cam to control playing of gear changes
*/
void SP_path_corner(gentity_t * self)
{
	if(!self->targetname)
	{
		G_Printf("path_corner with no targetname at %s\n", vtos(self->s.origin));
		G_FreeEntity(self);
		return;
	}
	// path corners don't need to be linked in

	if(self->wait == -1)
	{
		self->count = 1;
	}
}

/*QUAKED path_corner_2 (.5 .3 0) (-8 -8 -8) (8 8 8) STOP END REVERSE
Train path corners. THIS VERSION WILL NOT CONTRIBUTE TOWARDS INGAME ENTITY COUNT
"targetname" The script uses this name to identify the corner
*/
void SP_path_corner_2(gentity_t * self)
{
	if(!self->targetname)
	{
		G_Printf("path_corner_2 with no targetname at %s\n", vtos(self->s.origin));
		G_FreeEntity(self);
		return;
	}

	if(numPathCorners >= MAX_PATH_CORNERS)
	{
		G_Printf("Maximum path_corners hit\n");
		G_FreeEntity(self);
		return;
	}

	BG_AddPathCorner(self->targetname, self->s.origin);

	G_FreeEntity(self);
}

/*QUAKED info_train_spline_main (0 1 0) (-8 -8 -8) (8 8 8)
Train spline.
*/
void SP_info_train_spline_main(gentity_t * self)
{
	int             i;
	char           *end;
	splinePath_t   *spline;

	if(!self->targetname)
	{
		G_Printf("info_train_spline_main with no targetname at %s\n", vtos(self->s.origin));
		G_FreeEntity(self);
		return;
	}

//  if( self->target ) {
	spline = BG_AddSplinePath(self->targetname, self->target, self->s.origin);

	if(G_SpawnString("end", "", &end))
	{
		spline->isEnd = qtrue;
	}
	else if(G_SpawnString("start", "", &end))
	{
		spline->isStart = qtrue;
	}

	for(i = 1;; i++)
	{
		char           *control;

		if(!G_SpawnString(i == 1 ? va("control") : va("control%i", i), "", &control))
		{
			break;
		}

		BG_AddSplineControl(spline, control);
	}
/*	} else {
		BG_AddPathCorner( self->targetname, self->s.origin );
	}*/

	G_FreeEntity(self);
}

/*QUAKED info_limbo_camera (.5 .0 .0) ? (-8 -8 -8) (8 8 8)
Camera for limbo menu, target at an appropriate entity (spawn flag, script mover, position marker...)
*/
void info_limbo_camera_setup(gentity_t * self)
{
	limbo_cam_t    *caminfo;
	gentity_t      *target;
	vec3_t          vec;

	if(level.numLimboCams >= MAX_LIMBO_CAMS)
	{
		G_Error("info_limbo_camera: MAX_LIMBO_CAMS (%i) hit", MAX_LIMBO_CAMS);
	}

	caminfo = &level.limboCams[level.numLimboCams];
	level.numLimboCams++;

	if(!self->target || !*self->target)
	{
		G_Error("info_limbo_camera with no target");
	}

	target = G_FindByTargetname(NULL, self->target);
	if(!target)
	{
		G_Error("info_limbo_camera cannot find target");
	}

	VectorCopy(self->s.origin, caminfo->origin);
	caminfo->origin[2] -= 32;
	caminfo->info = self->count;

	switch (target->s.eType)
	{
		case ET_MOVER:
			caminfo->hasEnt = qtrue;
			caminfo->spawn = qfalse;

			caminfo->targetEnt = target - g_entities;
			break;

		case ET_WOLF_OBJECTIVE:
			caminfo->hasEnt = qfalse;
			caminfo->spawn = qtrue;

			caminfo->targetEnt = target - g_entities;
			break;

		default:
			caminfo->hasEnt = qfalse;
			caminfo->spawn = qfalse;
			break;
	}

	if(!caminfo->hasEnt)
	{
		VectorSubtract(target->s.origin, caminfo->origin, vec);
		VectorNormalize(vec);
		vectoangles(vec, caminfo->angles);
	}

	G_FreeEntity(self);
}

void SP_info_limbo_camera( gentity_t* self ) {
	if( !(self->spawnflags & 2) ) {
		if( g_gametype.integer == GT_WOLF_LMS ) {
			if( !(self->spawnflags & 1) ) {
				G_FreeEntity( self );
				return;
			}
		}
		if( g_gametype.integer != GT_WOLF_LMS ) {
			if( (self->spawnflags & 1) ) {
				G_FreeEntity( self );
				return;
			}
		}
	}

	self->think =		info_limbo_camera_setup;
	self->nextthink =	level.time + FRAMETIME;

	G_SpawnInt( "objective", "-1", &self->count );
}


/*QUAKED func_train (0 .5 .8) ? START_ON TOGGLE BLOCK_STOPS
A train is a mover that moves between path_corner target points.
Trains MUST HAVE AN ORIGIN BRUSH.
The train spawns at the first target it is pointing at.
"model2"	.md3 model to also draw
"speed"		default 100
"dmg"		default	2
"noise"		looping sound to play when the train is in motion
"target"	next path corner
"color"		constantLight color
"light"		constantLight radius
*/
void SP_func_train(gentity_t * self)
{
	VectorClear(self->s.angles);

	if(self->spawnflags & TRAIN_BLOCK_STOPS)
	{
		self->damage = 0;
		self->s.eFlags |= EF_MOVER_STOP;
	}
	else
	{
		if(!self->damage)
		{
			self->damage = 2;
		}
	}

	if(!self->speed)
	{
		self->speed = 100;
	}

	if(!self->target)
	{
		G_Printf("func_train without a target at %s\n", vtos(self->r.absmin));
		G_FreeEntity(self);
		return;
	}

	trap_SetBrushModel(self, self->model);
	InitMover(self);

	self->reached = Reached_Train;

	// start trains on the second frame, to make sure their targets have had
	// a chance to spawn
	self->nextthink = level.time + FRAMETIME;
	self->think = Think_SetupTrainTargets;

	self->blocked = Blocked_Door;

}

// JOSEPH 9-27-99
/*
===============
Think_BeginMoving_rotating

The wait time at a corner has completed, so start moving again
===============
*/
void Think_BeginMoving_rotating(gentity_t * ent)
{
	ent->s.pos.trTime = level.time;
	ent->s.pos.trType = TR_LINEAR_STOP;
}

/*
===============
Reached_Train_rotating
===============
*/
void Reached_Train_rotating(gentity_t * ent)
{
	gentity_t      *next;
	float           speed;
	vec3_t          move;
	float           length;
	float           frames;

	// copy the apropriate values
	next = ent->nextTrain;
	if(!next || !next->nextTrain)
	{
		return;					// just stop
	}

	// fire all other targets
	G_UseTargets(next, NULL);

	// set the new trajectory
	ent->nextTrain = next->nextTrain;
	VectorCopy(next->s.origin, ent->pos1);
	VectorCopy(next->nextTrain->s.origin, ent->pos2);

	// if the path_corner has a speed, use that
	if(next->speed)
	{
		speed = next->speed;
	}
	else
	{
		// otherwise use the train's speed
		speed = ent->speed;
	}
	if(speed < 1)
	{
		speed = 1;
	}

	ent->rotate[0] = next->rotate[2];
	ent->rotate[1] = next->rotate[0];
	ent->rotate[2] = next->rotate[1];

	// calculate duration
	VectorSubtract(ent->pos2, ent->pos1, move);
	length = VectorLength(move);

	if(next->duration)
	{
		ent->s.pos.trDuration = (next->duration * 1000);
	}
	else
	{
		ent->s.pos.trDuration = length * 1000 / speed;
	}

	// Rotate the train
	frames = floor(ent->s.pos.trDuration / 100);

	if(!frames)
	{
		frames = 0.001;
	}

	ent->s.apos.trType = TR_LINEAR;

	if(ent->TargetFlag)
	{
		VectorCopy(ent->TargetAngles, ent->r.currentAngles);
		VectorCopy(ent->r.currentAngles, ent->s.angles);
		VectorCopy(ent->s.angles, ent->s.apos.trBase);
		ent->TargetFlag = 0;
	}

	//G_Printf( "Train angles %s\n",
	//          vtos(ent->s.angles) );

	//G_Printf( "Add  X  Y  X %s\n",
	//          vtos(ent->rotate) );

	// X
	if(ent->rotate[2])
	{
		ent->s.apos.trDelta[2] = (ent->rotate[2] / frames) * 10;
	}
	else
	{
		ent->s.apos.trDelta[2] = 0;
	}
	// Y
	if(ent->rotate[0])
	{
		ent->s.apos.trDelta[0] = (ent->rotate[0] / frames) * 10;
	}
	else
	{
		ent->s.apos.trDelta[0] = 0;
	}
	// Z
	if(ent->rotate[1])
	{
		ent->s.apos.trDelta[1] = (ent->rotate[1] / frames) * 10;
	}
	else
	{
		ent->s.apos.trDelta[1] = 0;
	}

	// looping sound
	ent->s.loopSound = next->soundLoop;

	ent->TargetFlag = 1;
	ent->TargetAngles[0] = ent->r.currentAngles[0] + ent->rotate[0];
	//ent->TargetAngles[0] = AngleNormalize360 (ent->TargetAngles[0]);
	ent->TargetAngles[1] = ent->r.currentAngles[1] + ent->rotate[1];
	//ent->TargetAngles[1] = AngleNormalize360 (ent->TargetAngles[1]);
	ent->TargetAngles[2] = ent->r.currentAngles[2] + ent->rotate[2];
	//ent->TargetAngles[2] = AngleNormalize360 (ent->TargetAngles[2]);

	// start it going
	SetMoverState(ent, MOVER_1TO2, level.time);

	// if there is a "wait" value on the target, don't start moving yet
	if(next->wait)
	{
		ent->nextthink = level.time + next->wait * 1000;
		ent->think = Think_BeginMoving_rotating;
		ent->s.pos.trType = TR_STATIONARY;
	}
}

/*
===============
Think_SetupTrainTargets_rotating

Link all the corners together
===============
*/
void Think_SetupTrainTargets_rotating(gentity_t * ent)
{
	gentity_t      *path, *next, *start;


	ent->nextTrain = G_FindByTargetname(NULL, ent->target);
	if(!ent->nextTrain)
	{
		G_Printf("func_train at %s with an unfound target\n", vtos(ent->r.absmin));
		return;
	}

	VectorCopy(ent->s.angles, ent->s.apos.trBase);
	VectorCopy(ent->s.angles, ent->TargetAngles);
	ent->TargetFlag = 1;

	start = NULL;
	for(path = ent->nextTrain; path != start; path = next)
	{
		if(!start)
		{
			start = path;
		}

		if(!path->target)
		{
			G_Printf("Train corner at %s without a target\n", vtos(path->s.origin));
			return;
		}

		// find a path_corner among the targets
		// there may also be other targets that get fired when the corner
		// is reached
		next = NULL;
		do
		{
			next = G_FindByTargetname(next, path->target);
			if(!next)
			{
				G_Printf("Train corner at %s without a target path_corner\n", vtos(path->s.origin));
				return;
			}
		} while(strcmp(next->classname, "path_corner"));

		path->nextTrain = next;
	}

	// start the train moving from the first corner
	Reached_Train_rotating(ent);
}

/*QUAKED func_train_rotating (0 .5 .8) ? START_ON TOGGLE BLOCK_STOPS
A train is a mover that moves between path_corner target points.
This train can also rotate along the X Y Z
Trains MUST HAVE AN ORIGIN BRUSH.
The train spawns at the first target it is pointing at.

"model2"	.md3 model to also draw
"dmg"		default	2
"speed"		default 100
"noise"		looping sound to play when the train is in motion
"target"	next path corner
"color"		constantLight color
"light"		constantLight radius

On the path corner:
speed    departure speed from that corner
rotate   angle change for X Y Z to next corner
duration duration for angle change (overrides speed)
*/

void SP_func_train_rotating(gentity_t * self)
{
	VectorClear(self->s.angles);

	if(self->spawnflags & TRAIN_BLOCK_STOPS)
	{
		self->damage = 0;
	}
	else
	{
		if(!self->damage)
		{
			self->damage = 2;
		}
	}

	if(!self->speed)
	{
		self->speed = 100;
	}

	if(!self->target)
	{
		G_Printf("func_train without a target at %s\n", vtos(self->r.absmin));
		G_FreeEntity(self);
		return;
	}

	trap_SetBrushModel(self, self->model);
	InitMover(self);

	self->reached = Reached_Train_rotating;

	// start trains on the second frame, to make sure their targets have had
	// a chance to spawn
	self->nextthink = level.time + FRAMETIME;
	self->think = Think_SetupTrainTargets_rotating;
}

// END JOSEPH

/*
===============================================================================

STATIC

===============================================================================
*/

/*
==============
Use_Static
	toggle hide or show (including collisions) this entity
==============
*/
void Use_Static(gentity_t * ent, gentity_t * other, gentity_t * activator)
{
	if(ent->r.linked)
	{
		trap_UnlinkEntity(ent);
	}
	else
	{
		trap_LinkEntity(ent);
	}
}

void Static_Pain(gentity_t * ent, gentity_t * attacker, int damage, vec3_t point)
{
	vec3_t          temp;

	if(ent->spawnflags & 4)
	{
		if(level.time > ent->wait + ent->delay + rand() % 1000 + 500)
		{
			ent->wait = level.time;
		}
		else
		{
			return;
		}

		// TBD only venom mg42 rocket and grenade can inflict damage
		if(attacker && attacker->client
		   && (attacker->s.weapon == WP_GRENADE_LAUNCHER
			   || attacker->s.weapon == WP_PANZERFAUST || attacker->client->ps.persistant[PERS_HWEAPON_USE]))
		{

			VectorCopy(ent->r.currentOrigin, temp);
			VectorCopy(ent->pos3, ent->r.currentOrigin);
			Spawn_Shard(ent, attacker, 3, ent->count);
			VectorCopy(temp, ent->r.currentOrigin);
		}
		return;
	}

	if(level.time > ent->wait + ent->delay + rand() % 1000 + 500)
	{
		G_UseTargets(ent, NULL);
		ent->wait = level.time;
	}

}

void G_BlockThink(gentity_t * ent)
{
	ent->nextthink = level.time + FRAMETIME;
}


/*QUAKED func_leaky (0 .5 .8) ?
"type" - leaks particles of this type

1:oil
2:water
3:steam


*/

void SP_func_leaky(gentity_t * ent)
{
	if(ent->model2)
	{
		ent->s.modelindex2 = G_ModelIndex(ent->model2);
	}
	trap_SetBrushModel(ent, ent->model);
	trap_LinkEntity(ent);
	ent->s.pos.trType = TR_STATIONARY;
	VectorCopy(ent->s.origin, ent->s.pos.trBase);
	VectorCopy(ent->s.origin, ent->r.currentOrigin);
}

/*QUAKED func_static (0 .5 .8) ?
A bmodel that just sits there, doing nothing.  Can be used for conditional walls and models.
"model2"	.md3 model to also draw
"color"		constantLight color
"light"		constantLight radius
*/
void SP_func_static(gentity_t * ent)
{
	trap_SetBrushModel(ent, ent->model);

	InitMover(ent);

	VectorCopy(ent->s.origin, ent->s.pos.trBase);
	VectorCopy(ent->s.origin, ent->r.currentOrigin);

	VectorCopy(ent->s.angles, ent->s.apos.trBase);
	VectorCopy(ent->s.angles, ent->r.currentAngles);

	trap_LinkEntity(ent);
}

#ifdef USE_PHYSICS
/*
QUAKED func_dynamic (0 .5 .8) ?
*/
void SP_func_dynamic(gentity_t * ent)
{
	trap_SetBrushModel (ent, ent->model);

	InitMover (ent);
	
	ent->flags |= FL_TEAMSLAVE;
	
	trap_UnlinkEntity (ent); // was linked in InitMover
	trap_AddPhysicsEntity (ent);
	trap_LinkEntity (ent);
}

#endif

/*
===============================================================================

ROTATING

===============================================================================
*/


/*QUAKED func_rotating (0 .5 .8) ? START_ON STARTINVIS X_AXIS Y_AXIS
You need to have an origin brush as part of this entity.  The center of that brush will be
the point around which it is rotated. It will rotate around the Z axis by default.  You can
check either the X_AXIS or Y_AXIS box to change that.

"model2"	.md3 model to also draw
"speed"		determines how fast it moves; default value is 100.
"dmg"		damage to inflict when blocked (2 default)
"color"		constantLight color
"light"		constantLight radius
*/

void Use_Func_Rotate(gentity_t * ent, gentity_t * other, gentity_t * activator)
{
	if(ent->spawnflags & 4)
	{
		ent->s.apos.trDelta[2] = ent->speed;
	}
	else if(ent->spawnflags & 8)
	{
		ent->s.apos.trDelta[0] = ent->speed;
	}
	else
	{
		ent->s.apos.trDelta[1] = ent->speed;
	}

	if(ent->spawnflags & 2)
	{
		ent->flags &= ~FL_TEAMSLAVE;
	}

	trap_LinkEntity(ent);
}

void SP_func_rotating(gentity_t * ent)
{
	if(!ent->speed)
	{
		ent->speed = 100;
	}

	// set the axis of rotation
	ent->s.apos.trType = TR_LINEAR;

	if(ent->spawnflags & 1)
	{
		if(ent->spawnflags & 4)
		{
			ent->s.apos.trDelta[2] = ent->speed;
		}
		else if(ent->spawnflags & 8)
		{
			ent->s.apos.trDelta[0] = ent->speed;
		}
		else
		{
			ent->s.apos.trDelta[1] = ent->speed;
		}
	}

	if(!ent->damage)
	{
		ent->damage = 2;
	}

	trap_SetBrushModel(ent, ent->model);
	InitMover(ent);

	VectorCopy(ent->s.origin, ent->s.pos.trBase);
	VectorCopy(ent->s.pos.trBase, ent->r.currentOrigin);
	VectorCopy(ent->s.apos.trBase, ent->r.currentAngles);

	if(ent->spawnflags & 2)
	{
		ent->flags |= FL_TEAMSLAVE;
		trap_UnlinkEntity(ent);
	}
	else
	{
		trap_LinkEntity(ent);
	}

}


/*
===============================================================================

BOBBING

===============================================================================
*/


/*QUAKED func_bobbing (0 .5 .8) ? X_AXIS Y_AXIS
Normally bobs on the Z axis
"model2"	.md3 model to also draw
"height"	amplitude of bob (32 default)
"speed"		seconds to complete a bob cycle (4 default)
"phase"		the 0.0 to 1.0 offset in the cycle to start at
"dmg"		damage to inflict when blocked (2 default)
"color"		constantLight color
"light"		constantLight radius
*/
void SP_func_bobbing(gentity_t * ent)
{
	float           height;
	float           phase;

	G_SpawnFloat("speed", "4", &ent->speed);
	G_SpawnFloat("height", "32", &height);
	G_SpawnInt("dmg", "2", &ent->damage);
	G_SpawnFloat("phase", "0", &phase);

	trap_SetBrushModel(ent, ent->model);
	InitMover(ent);

	VectorCopy(ent->s.origin, ent->s.pos.trBase);
	VectorCopy(ent->s.origin, ent->r.currentOrigin);

	ent->s.pos.trDuration = ent->speed * 1000;
	ent->s.pos.trTime = ent->s.pos.trDuration * phase;
	ent->s.pos.trType = TR_SINE;

	// set the axis of bobbing
	if(ent->spawnflags & 1)
	{
		ent->s.pos.trDelta[0] = height;
	}
	else if(ent->spawnflags & 2)
	{
		ent->s.pos.trDelta[1] = height;
	}
	else
	{
		ent->s.pos.trDelta[2] = height;
	}
}

/*
===============================================================================

PENDULUM

===============================================================================
*/


/*QUAKED func_pendulum (0 .5 .8) ?
You need to have an origin brush as part of this entity.
Pendulums always swing north / south on unrotated models.  Add an angles field to the model to allow rotation in other directions.
Pendulum frequency is a physical constant based on the length of the beam and gravity.
"model2"	.md3 model to also draw
"speed"		the number of degrees each way the pendulum swings, (30 default)
"phase"		the 0.0 to 1.0 offset in the cycle to start at
"dmg"		damage to inflict when blocked (2 default)
"color"		constantLight color
"light"		constantLight radius
*/
void SP_func_pendulum(gentity_t * ent)
{
	float           freq;
	float           length;
	float           phase;
	float           speed;

	G_SpawnFloat("speed", "30", &speed);
	G_SpawnInt("dmg", "2", &ent->damage);
	G_SpawnFloat("phase", "0", &phase);

	trap_SetBrushModel(ent, ent->model);

	// find pendulum length
	length = fabs(ent->r.mins[2]);
	if(length < 8)
	{
		length = 8;
	}

	freq = 1 / (M_PI * 2) * sqrt(g_gravity.value / (3 * length));

	ent->s.pos.trDuration = (1000 / freq);

	InitMover(ent);

	VectorCopy(ent->s.origin, ent->s.pos.trBase);
	VectorCopy(ent->s.origin, ent->r.currentOrigin);

	VectorCopy(ent->s.angles, ent->s.apos.trBase);

	ent->s.apos.trDuration = 1000 / freq;
	ent->s.apos.trTime = ent->s.apos.trDuration * phase;
	ent->s.apos.trType = TR_SINE;
	ent->s.apos.trDelta[2] = speed;
}

/*QUAKED func_door_rotating (0 .5 .8) ? - TOGGLE X_AXIS Y_AXIS REVERSE FORCE STAYOPEN TAKE_KEY
You need to have an origin brush as part of this entity.  The center of that brush will be
the point around which it is rotated. It will rotate around the Z axis by default.  You can
check either the X_AXIS or Y_AXIS box to change that (only one axis allowed. If both X and Y
are checked, the default of Z will be used).
FORCE		door opens even if blocked
TAKE_KEY	removes the key from the players inventory
AXIS_ONLY	Only axis team can open the doors
ALLIES_ONLY Only allied team can open the doors
ACCEPT_CVOPS Accept a disguised covert ops as a valid team member for team only doors

"key"       -1 for locked, key number for which key opens, 0 for open.  default '0' unless door is targeted.  (trigger_aidoor entities targeting this door do /not/ affect the key status)
"model2"    .md3 model to also draw
"degrees"   determines how many degrees it will turn (90 default)
"speed"	    movement speed (100 default)
"closespeed" optional different movement speed for door closing
"time"      how many milliseconds it will take to open 1 sec = 1000
"dmg"       damage to inflict when blocked (2 default)
"color"     constantLight color
"light"     constantLight radius
"lockpickTime" time to unpick the lock on a locked door - use with key set to 99
"type"		use sounds based on construction of door:
	 0 - nosound (default)
	 1 - metal
	 2 - stone
	 3 - lab
	 4 - wood
	 5 - iron/jail
	 6 - portcullis
	 7 - wood (quiet)
"team"		team name.  other doors with same team name will open/close in syncronicity
*/




//
//
void SP_func_door_rotating(gentity_t * ent)
{
	int             key, doortype;

	G_SpawnInt("type", "0", &doortype);

	if(doortype)
	{							// /*why on earthy did this check for <=8?*/ && doortype <= 8)    // no doortype = silent
		DoorSetSounds(ent, doortype, qtrue);
	}


	// set the duration
	if(!ent->speed)
	{
		ent->speed = 1000;
	}

	// degrees door will open
	if(!ent->angle)
	{
		ent->angle = 90;
	}

	// reverse direction
	if(ent->spawnflags & 16)
	{
		ent->angle *= -1;
	}

	// TOGGLE
	if(ent->spawnflags & 2)
	{
		ent->flags |= FL_TOGGLE;
	}

	//---- (SA) door keys

	if(G_SpawnInt("key", "", &key))
	{							// if door has a key entered, set it
		ent->key = key;
	}
	else
	{
		ent->key = -2;			// otherwise, set the key when this ent finishes spawning


	}
	// special case for single player
	if(G_IsSinglePlayerGame() && (ent->key == 99))
	{
		ent->key = KEY_LOCKED_PICKABLE;

		// TAT 1/29/2003 - also load how long it takes to pick the lock - default to 30 seconds
		G_SpawnInt("lockpickTime", "30", &ent->grenadeFired);
	}

	// if the key is invalid, set the key in the finishSpawning routine
	if(ent->key > KEY_NUM_KEYS || ent->key < -2)
	{
		G_Error("invalid key number: %d in func_door_rotating\n", ent->key);
		ent->key = -2;
	}
	//---- (SA) end


	// set the rotation axis
	VectorClear(ent->rotate);
	if(ent->spawnflags & 4)
	{
		ent->rotate[2] = 1;
	}
	else if(ent->spawnflags & 8)
	{
		ent->rotate[0] = 1;
	}
	else
	{
		ent->rotate[1] = 1;
	}

	if(VectorLengthSquared(ent->rotate) > SQR(1))
	{							// check that rotation is only set for one axis
		G_Error
			("Too many axis marked in func_door_rotating entity.  Only choose one axis of rotation. (defaulting to standard door rotation)");
		VectorClear(ent->rotate);
		ent->rotate[1] = 1;
	}

	if(!ent->wait)
	{
		ent->wait = 2;
	}
	ent->wait *= 1000;

	//if (!ent->damage) {
	//  ent->damage = 2;
	//}

	trap_SetBrushModel(ent, ent->model);

	InitMoverRotate(ent);

	if(!ent->allowteams)
	{
		ent->s.dmgFlags = HINT_DOOR_ROTATING;
	}

	if(!(ent->flags & FL_TEAMSLAVE))
	{
		int             health;

		G_SpawnInt("health", "0", &health);
		if(health)
		{
			ent->takedamage = qtrue;
		}
	}

	ent->nextthink = level.time + FRAMETIME;
	ent->think = finishSpawningKeyedMover;

	VectorCopy(ent->s.origin, ent->s.pos.trBase);
	VectorCopy(ent->s.pos.trBase, ent->r.currentOrigin);
	VectorCopy(ent->s.apos.trBase, ent->r.currentAngles);

	ent->blocked = Blocked_DoorRotate;

	trap_LinkEntity(ent);
}




/*
===============================================================================

EFFECTS

  I'm keeping all this stuff in here just to avoid collisions with Raf right now in g_misc or g_props
  Will move.
===============================================================================
*/

/*
==============
target_effect
==============
*/
void target_effect(gentity_t * self, gentity_t * other, gentity_t * activator)
{
	gentity_t      *tent;

	tent = G_TempEntity(self->r.currentOrigin, EV_EFFECT);
	VectorCopy(self->r.currentOrigin, tent->s.origin);
	if(self->spawnflags & 32)
	{
		tent->s.dl_intensity = 1;	// low grav
	}
	else
	{
		tent->s.dl_intensity = 0;
	}

	trap_SetConfigstring(CS_TARGETEFFECT, self->dl_shader);	//----(SA)  allow shader to be set from entity

	// (SA) this should match the values from func_explosive
	tent->s.frame = self->key;	// pass the type to the client ("glass", "wood", "metal", "gibs", "brick", "stone", "fabric", 0, 1, 2, 3, 4, 5, 6)

	tent->s.eventParm = self->spawnflags;
	tent->s.density = self->health;

	if(self->damage)
	{
		G_RadiusDamage(self->s.pos.trBase, NULL, self, self->damage, self->damage, self, MOD_EXPLOSIVE);
	}

	G_UseTargets(self, other);
}


/*QUAKED target_effect (0 .5 .8) (-6 -6 -6) (6 6 6) fire explode smoke rubble gore lowgrav debris
"mass" defaults to 15.  This determines how much debris is emitted when it explodes.  (number of pieces)
"dmg" defaults to 0.  damage radius blast when triggered
"type" - if 'rubble' is specified, this is the model type ("glass", "wood", "metal", "gibs", "brick", "rock", "fabric") default is "wood"
*/
void SP_target_effect(gentity_t * ent)
{
	int             mass;
	char           *type;

	ent->use = target_effect;

	if(G_SpawnInt("mass", "15", &mass))
	{
		ent->health = mass;
	}
	else
	{
		ent->health = 15;
	}

	// (SA) this should match the values from func_explosive
	if(G_SpawnString("type", "wood", &type))
	{
		if(!Q_stricmp(type, "wood"))
		{
			ent->key = FXTYPE_WOOD;
		}
		else if(!Q_stricmp(type, "glass"))
		{
			ent->key = FXTYPE_GLASS;
		}
		else if(!Q_stricmp(type, "metal"))
		{
			ent->key = FXTYPE_METAL;
		}
		else if(!Q_stricmp(type, "gibs"))
		{
			ent->key = FXTYPE_GIBS;
		}
		else if(!Q_stricmp(type, "brick"))
		{
			ent->key = FXTYPE_BRICK;
		}
		else if(!Q_stricmp(type, "rock"))
		{
			ent->key = FXTYPE_STONE;
		}
		else if(!Q_stricmp(type, "fabric"))
		{
			ent->key = 0;		// fixme: not supported
		}
	}
	else
	{
		ent->key = 5;			// default to 'rock'
	}

	if(ent->dl_shader)
	{
		G_Printf
			("^1Warning: This feature needs to be cleaned up from original wolf since you are using it, go poke Gordon about it\n");
	}
}



/*
===============================================================================

EXPLOSIVE
  I'm keeping all this stuff in here just to avoid collisions with Raf right now in g_misc or g_props
  Will move.
===============================================================================
*/


/*
==============
ThrowDebris
==============
*/
void ThrowDebris(gentity_t * self, char *modelname, float speed, vec3_t origin)
{
	// probably use le->leType = LE_FRAGMENT like brass and gibs
}

/*
==============
BecomeExplosion
	nuke the original entity and create all the debris entities that need to be synced to clients
==============
*/
void BecomeExplosion(gentity_t * self)
{
	self->die = NULL;
	self->pain = NULL;
	self->touch = NULL;
	self->use = NULL;
	self->nextthink = level.time + FRAMETIME;
	self->think = G_FreeEntity;

	G_FreeEntity(self);
}



/*
==============
func_explosive_explode
	NOTE: the 'damage' passed in is ignored completely
==============
*/
void func_explosive_explode(gentity_t * self, gentity_t * inflictor, gentity_t * attacker, int damage, int mod)
{
	vec3_t          origin;
	vec3_t          size;
	vec3_t          dir = { 0, 0, 1 };
	gentity_t      *tent = 0;

	self->takedamage = qfalse;	// don't allow anything try to hurt me now that i'm exploding

	self->think = BecomeExplosion;
	self->nextthink = level.time + FRAMETIME;

	VectorSubtract(self->r.absmax, self->r.absmin, size);
	VectorScale(size, 0.5, size);
	VectorAdd(self->r.absmin, size, origin);

	VectorCopy(origin, self->s.pos.trBase);

	G_UseTargets(self, attacker);

	self->s.density = self->count;	// pass the "mass" to the client
	self->s.weapon = self->duration;	// pass the "force lowgrav" to client
	self->s.frame = self->key;	// pass the type to the client ("glass", "wood", "metal", "gibs", "brick", "stone", "fabric", 0, 1, 2, 3, 4, 5, 6)

	if(self->damage)
	{
//      G_RadiusDamage(self->s.origin, self, self->damage, self->damage+40, self, MOD_EXPLOSIVE);
		G_RadiusDamage(self->s.pos.trBase, NULL, self, self->damage, self->damage + 40, self, MOD_EXPLOSIVE);
	}

	// find target, aim at that
	if(self->target)
	{

		// since the explosive might need to fire the target rather than
		// aim at it, only aim at 'info_notnull' ents
		while(1)
		{
			tent = G_FindByTargetname(tent, self->target);
			if(!tent)
			{
				break;
			}

			if(!Q_stricmp(tent->classname, "info_notnull"))
			{
				break;			// found an info_notnull
			}
		}

		if(tent)
		{
			VectorSubtract(tent->s.pos.trBase, self->s.pos.trBase, dir);
			VectorNormalize(dir);
		}
	}

	// if a valid target entity was not found, check for a specified 'angle' for the explosion direction
	if(!tent)
	{
		if(self->s.angles[1])
		{
			// up
			if(self->s.angles[1] == -1)
			{
				// it's 'up' by default
			}
			// down
			else if(self->s.angles[1] == -2)
			{
				dir[2] = -1;
			}
			// yawed
			else
			{
				RotatePointAroundVector(dir, dir, tv(1, 0, 0), self->s.angles[1]);
			}
		}
	}

	G_AddEvent(self, EV_EXPLODE, DirToByte(dir));

#ifdef OMNIBOT
	if (self->constructibleStats.constructxpbonus == 5)
	{
		G_Script_ScriptEvent( self, "exploded", "" );
	}
#endif

	// Skills stuff

	if(G_GetWeaponClassForMOD(mod) >= self->constructibleStats.weaponclass)
	{
		G_AddKillSkillPointsForDestruction(attacker, mod, &self->constructibleStats);
	}
}

/*
==============
func_explosive_touch
==============
*/
void func_explosive_touch(gentity_t * self, gentity_t * other, trace_t * trace)
{
	func_explosive_explode(self, self, other, self->damage, 0);
}


/*
==============
func_explosive_use
==============
*/
void func_explosive_use(gentity_t * self, gentity_t * other, gentity_t * activator)
{
	G_Script_ScriptEvent(self, "death", "");	// JPW NERVE used to trigger script stuff for MP
	func_explosive_explode(self, self, other, self->damage, 0);
}

/*
==============
func_explosive_alert
==============
*/
void func_explosive_alert(gentity_t * self)
{
	func_explosive_explode(self, self, self, self->damage, 0);
}

/*
==============
func_explosive_spawn
==============
*/
void func_explosive_spawn(gentity_t * self, gentity_t * other, gentity_t * activator)
{
	trap_LinkEntity(self);
	self->use = func_explosive_use;
	// turn the brush to visible
}


/*
==============
InitExplosive
==============
*/
void InitExplosive(gentity_t * ent)
{
	char           *damage;

	// if the "model2" key is set, use a seperate model
	// for drawing, but clip against the brushes
	if(ent->model2)
	{
		ent->s.modelindex2 = G_ModelIndex(ent->model2);
	}

	// pick it up if the level designer uses "damage" instead of "dmg"
	if(G_SpawnString("damage", "0", &damage))
	{
		ent->damage = atoi(damage);
	}

	ent->s.eType = ET_EXPLOSIVE;
	ent->parent = NULL;
	trap_LinkEntity(ent);

	ent->think = G_BlockThink;
	ent->nextthink = level.time + FRAMETIME;
}

/*QUAKED target_explosion (0 .5 .8) (-32 -32 -32) (32 32 32) LOWGRAV
"type" - type of debris ("glass", "wood", "metal", "gibs", "brick", "rock", "fabric") default is "wood"
*/

void target_explosion_use(gentity_t * self, gentity_t * other, gentity_t * attacker)
{
	vec3_t          dir = { 0, 0, 1 };
	gentity_t      *tent = NULL;

	tent = G_TempEntity(self->r.currentOrigin, EV_RUBBLE);

	G_UseTargets(self, attacker);

	tent->s.density = self->count;	// pass the "mass" to the client
	tent->s.weapon = self->duration;	// pass the "force lowgrav" to client
	tent->s.frame = self->key;	// pass the type to the client ("glass", "wood", "metal", "gibs", "brick", "stone", "fabric", 0, 1, 2, 3, 4, 5, 6)
	tent->s.angles2[0] = self->s.angles2[0];
	tent->s.angles2[1] = self->s.angles2[1];

	if(self->damage)
	{
		G_RadiusDamage(self->s.pos.trBase, NULL, self, self->damage, self->damage + 40, self, MOD_EXPLOSIVE);
	}

	tent->s.eventParm = DirToByte(dir);
}

void SP_target_explosion(gentity_t * ent)
{
	char           *type;
	char           *s;
	char            buffer[MAX_QPATH];

	if(ent->spawnflags & 1)
	{							// force lowgravity
		ent->duration = 1;
	}
	else
	{
		ent->duration = 0;
	}

	G_SpawnInt("dmg", "0", &ent->damage);

	ent->takedamage = qfalse;

	if(!G_SpawnInt("mass", "75", &ent->count))
	{
		ent->count = 75;
	}

	G_SpawnFloat("speed", "1", &ent->s.angles2[0]);
	G_SpawnFloat("size", "1", &ent->s.angles2[1]);

	if(G_SpawnString("type", "wood", &type))
	{
		if(!Q_stricmp(type, "wood"))
		{
			ent->key = FXTYPE_WOOD;
		}
		else if(!Q_stricmp(type, "glass"))
		{
			ent->key = FXTYPE_GLASS;
		}
		else if(!Q_stricmp(type, "metal"))
		{
			ent->key = FXTYPE_METAL;
		}
		else if(!Q_stricmp(type, "gibs"))
		{
			ent->key = FXTYPE_GIBS;
		}
		else if(!Q_stricmp(type, "brick"))
		{
			ent->key = FXTYPE_BRICK;
		}
		else if(!Q_stricmp(type, "rock"))
		{
			ent->key = FXTYPE_STONE;
		}
		else if(!Q_stricmp(type, "fabric"))
		{
			ent->key = 0;		// fixme: not supported
		}
	}
	else
	{
		ent->key = 0;
	}

	ent->s.dl_intensity = 0;
	if(G_SpawnString("noise", "NOSOUND", &s))
	{
		if(Q_stricmp(s, "nosound"))
		{
			Q_strncpyz(buffer, s, sizeof(buffer));
			ent->s.dl_intensity = G_SoundIndex(buffer);
		}
		else
		{
			ent->s.dl_intensity = -1;
		}
	}							/* else {
								   switch(ent->key) {
								   case 0: // "wood"
								   ent->s.dl_intensity = G_SoundIndex("sound/world/boardbreak.wav");
								   break;
								   case 1: // "glass"
								   ent->s.dl_intensity = G_SoundIndex("sound/world/glassbreak_01.wav");
								   break;
								   case 2: // "metal"
								   ent->s.dl_intensity = G_SoundIndex("sound/world/metalbreak.wav");
								   break;
								   case 3: // "gibs"
								   ent->s.dl_intensity = G_SoundIndex("sound/player/gibsplit1.wav");
								   break;
								   case 4: // "brick"
								   ent->s.dl_intensity = G_SoundIndex("sound/world/debris1.wav");
								   break;
								   case 5: // "stone"
								   ent->s.dl_intensity = G_SoundIndex("sound/world/stonefall.wav");
								   break;

								   default:
								   break;
								   }
								   } */

	ent->use = target_explosion_use;
}

/*QUAKED func_explosive (0 .5 .8) ? START_INVIS TOUCHABLE USESHADER LOWGRAV NOBLOCKAAS TANK
Any brush that you want to explode or break apart.  If you want an explosion, set dmg and it will do a radius explosion of that amount at the center of the bursh.
TOUCHABLE means automatic use on player contact.
USESHADER will apply the shader used on the brush model to the debris.
LOWGRAV specifies that the debris will /always/ fall slowly
"item" - when it explodes, pop this item out with the debirs (use QUAKED name. ex: "item_health_small")
"dmg" - how much radius damage should be done, defaults to 0
"health" - defaults to 100.  If health is set to '0' the brush will not be shootable.
"targetname" - if set, no touch field will be spawned and a remote button or trigger field triggers the explosion.
"type" - type of debris ("glass", "wood", "metal", "gibs", "brick", "rock", "fabric") default is "wood"
"mass" - defaults to 75.  This determines how much debris is emitted when it explodes.  You get one large chunk per 100 of mass (up to 8) and one small chunk per 25 of mass (up to 16).  So 800 gives the most.
"noise" - sound to play when triggered.  The explosive will default to a sound that matches it's 'type'.  Use the sound name "nosound" (case in-sensitive) if you want it silent.
the default sounds are:
  "wood"	- "sound/world/boardbreak.wav"
  "glass"	- "sound/world/glassbreak_01.wav"
  "metal"	- "sound/world/metalbreak.wav"
  "gibs"	- "sound/player/gibsplit1.wav"
  "brick"	- "sound/world/debris1.wav"
  "stone"	- "sound/world/stonefall.wav"
  "fabric"	- "sound/world/metalbreak.wav"	// (SA) temp
"fxdensity" size of explosion 1 - 100 (default is 10)
*/
void SP_func_explosive(gentity_t * ent)
{
	int             health, mass, dam, i;
	char            buffer[MAX_QPATH];
	char           *s;
	char           *type;
	char           *cursorhint;

	trap_SetBrushModel(ent, ent->model);
	InitExplosive(ent);

	if(ent->spawnflags & EXPLOSIVE_START_INVIS)
	{							// start invis
		ent->use = func_explosive_spawn;
		trap_UnlinkEntity(ent);
	}
	else if(ent->targetname)
	{
		ent->use = func_explosive_use;
		ent->AIScript_AlertEntity = func_explosive_alert;
	}


	if(ent->spawnflags & EXPLOSIVE_TOUCHABLE)
	{							// touchable
		ent->touch = func_explosive_touch;
	}
	else
	{
		ent->touch = NULL;
	}

	if((ent->spawnflags & EXPLOSIVE_USESHADER) && ent->model && strlen(ent->model))
	{							// use shader
		ent->s.eFlags |= EF_INHERITSHADER;
	}

	if(ent->spawnflags & EXPLOSIVE_LOWGRAV)
	{							// force lowgravity
		ent->duration = 1;
	}

	memset(&ent->constructibleStats, 0, sizeof(ent->constructibleStats));
	G_SpawnInt("constructible_class", "0", &i);
	i--;						// non-coder friendlyness. Aren't we nice?
	if(i > 0 && i <= NUM_CONSTRUCTIBLE_CLASSES)
	{
		ent->constructibleStats = g_constructible_classes[i];

		G_SpawnFloat("constructible_destructxpbonus", va("%f", ent->constructibleStats.destructxpbonus),
					 &ent->constructibleStats.destructxpbonus);
		G_SpawnInt("constructible_health", va("%i", ent->constructibleStats.health), &ent->constructibleStats.health);
		G_SpawnInt("constructible_weaponclass", va("%i", ent->constructibleStats.weaponclass),
				   &ent->constructibleStats.weaponclass);
	}
	else
	{
		G_SpawnFloat("constructible_destructxpbonus", "0", &ent->constructibleStats.destructxpbonus);
		G_SpawnInt("constructible_health", "-1", &ent->constructibleStats.health);
		G_SpawnInt("constructible_weaponclass", "0", &ent->constructibleStats.weaponclass);
	}
	ent->constructibleStats.weaponclass--;
	ent->health = ent->constructibleStats.health;

	G_SpawnInt("health", "100", &health);
	ent->health = health;

	G_SpawnInt("dmg", "0", &dam);
	ent->damage = dam;

	if(ent->health)
	{
		ent->takedamage = qtrue;
	}

	if(G_SpawnInt("mass", "75", &mass))
	{
		ent->count = mass;
	}
	else
	{
		ent->count = 75;
	}

	if(G_SpawnString("type", "wood", &type))
	{
		if(!Q_stricmp(type, "wood"))
		{
			ent->key = FXTYPE_WOOD;
		}
		else if(!Q_stricmp(type, "glass"))
		{
			ent->key = FXTYPE_GLASS;
		}
		else if(!Q_stricmp(type, "metal"))
		{
			ent->key = FXTYPE_METAL;
		}
		else if(!Q_stricmp(type, "gibs"))
		{
			ent->key = FXTYPE_GIBS;
		}
		else if(!Q_stricmp(type, "brick"))
		{
			ent->key = FXTYPE_BRICK;
		}
		else if(!Q_stricmp(type, "rock"))
		{
			ent->key = FXTYPE_STONE;
		}
		else if(!Q_stricmp(type, "fabric"))
		{
			ent->key = 0;		// fixme: not supported
		}
	}
	else
	{
		ent->key = 0;
	}

	if(G_SpawnString("noise", "NOSOUND", &s))
	{
		if(Q_stricmp(s, "nosound"))
		{
			Q_strncpyz(buffer, s, sizeof(buffer));
			ent->s.dl_intensity = G_SoundIndex(buffer);
		}
		else
		{
			ent->s.dl_intensity = -1;
		}
	}

//----(SA)  added

	ent->s.dmgFlags = 0;

	if(G_SpawnString("cursorhint", "0", &cursorhint))
	{

		for(i = 0; i < HINT_NUM_HINTS; i++)
		{
			if(!Q_stricmp(cursorhint, hintStrings[i]))
			{
				ent->s.dmgFlags = i;
			}
		}
	}
//----(SA)  end

	// (SA) shouldn't need this
//  ent->s.density = ent->count;    // pass the "mass" to the client


	ent->die = func_explosive_explode;
}

// TODO: stick this somewhere ;) "sonic from australia is the bestest ninja in the world"

/*QUAKED func_invisible_user (.3 .5 .8) ? STARTOFF HAS_USER NO_OFF_NOISE NOT_KICKABLE
when activated will use its target
"delay" - time (in seconds) before it can be used again
"offnoise" - specifies an alternate sound
"cursorhint" - overrides the auto-location of targeted entity (list below)
Normally when a player 'activates' this entity, if the entity has been turned 'off' (by a scripted command) you will hear a sound to indicate that you cannot activate the user.
The sound defaults to "sound/movers/invis_user_off.wav"

NO_OFF_NOISE - no sound will play if the invis_user is used when 'off'
NOT_KICKABLE - kicking doesn't fire, only player activating

"cursorhint" cursor types: (probably more, ask sherman if you think the list is out of date)
they /don't/ need to be all uppercase
	HINT_NONE
	HINT_PLAYER
	HINT_ACTIVATE
	HINT_DOOR
	HINT_DOOR_ROTATING
	HINT_DOOR_LOCKED
	HINT_DOOR_ROTATING_LOCKED
	HINT_MG42
	HINT_BREAKABLE
	HINT_BREAKABLE_BIG
	HINT_CHAIR
	HINT_ALARM
	HINT_HEALTH
	HINT_TREASURE
	HINT_KNIFE
	HINT_LADDER
	HINT_BUTTON
	HINT_WATER
	HINT_CAUTION
	HINT_DANGER
	HINT_SECRET
	HINT_QUESTION
	HINT_EXCLAMATION
	HINT_CLIPBOARD
	HINT_WEAPON
	HINT_AMMO
	HINT_ARMOR
	HINT_POWERUP
	HINT_HOLDABLE
	HINT_INVENTORY
	HINT_SCENARIC
	HINT_EXIT
	HINT_NOEXIT
	HINT_PLYR_FRIEND
	HINT_PLYR_NEUTRAL
	HINT_PLYR_ENEMY
	HINT_PLYR_UNKNOWN
*/

void use_invisible_user(gentity_t * ent, gentity_t * other, gentity_t * activator)
{
	if(ent->wait < level.time)
	{
		ent->wait = level.time + ent->delay;
	}
	else
	{
		return;
	}

	if(!(other->client))
	{
		if(ent->spawnflags & 1)
		{
			ent->spawnflags &= ~1;
		}
		else
		{
			ent->spawnflags |= 1;
		}

		if(ent->spawnflags & 2 && !(ent->spawnflags & 1))
		{
			G_Script_ScriptEvent(ent, "activate", NULL);
			G_UseTargets(ent, other);
			// G_Printf ("ent%s used by %s\n", ent->classname, other->classname);
		}

		return;
	}

	if(other->client && ent->spawnflags & 1)
	{
		//----(SA)  play 'off' sound
		//----(SA)  I think this is where this goes.  Raf, let me know if it's wrong.  I need someone to tell me what a test map is for this (I'll ask Dan tomorrow)
		// not usable by player.  turned off.
		G_Sound(ent, ent->soundPos1);
		return;
	}

	if(other->client)
	{
		G_Script_ScriptEvent(ent, "activate", other->client->sess.sessionTeam == TEAM_AXIS ? "axis" : "allies");
	}
	G_UseTargets(ent, other);	//----(SA)  how about this so the triggered targets have an 'activator' as well as an 'other'?
	//----(SA)  Please let me know if you forsee any problems with this.
}


void SP_func_invisible_user(gentity_t * ent)
{
	int             i;
	char           *sound;
	char           *cursorhint;

	VectorCopy(ent->s.origin, ent->pos1);
	trap_SetBrushModel(ent, ent->model);

	// InitMover (ent);
	VectorCopy(ent->pos1, ent->r.currentOrigin);
	trap_LinkEntity(ent);

	ent->s.pos.trType = TR_STATIONARY;
	VectorCopy(ent->pos1, ent->s.pos.trBase);

	ent->r.contents = CONTENTS_TRIGGER;

	ent->r.svFlags = SVF_NOCLIENT;

	ent->delay *= 1000;			// convert to ms

	ent->use = use_invisible_user;


//----(SA)  added
	if(G_SpawnString("cursorhint", "0", &cursorhint))
	{

		for(i = 0; i < HINT_NUM_HINTS; i++)
		{
			if(!Q_stricmp(cursorhint, hintStrings[i]))
			{
				ent->s.dmgFlags = i;
			}
		}
	}
//----(SA)  end


	if(!(ent->spawnflags & 4))
	{							// !NO_OFF_NOISE
		if(G_SpawnString("offnoise", "0", &sound))
		{
			ent->soundPos1 = G_SoundIndex(sound);
		}
		else
		{
			ent->soundPos1 = G_SoundIndex("sound/movers/doors/default_door_locked.wav");
		}
	}


}

/*
==========
G_Activate

  Generic activation routine for doors
==========
*/
void G_Activate(gentity_t * ent, gentity_t * activator)
{
	if((ent->s.apos.trType == TR_STATIONARY && ent->s.pos.trType == TR_STATIONARY) && ent->active == qfalse)
	{
		// trigger the ent if possible, if not, then we'll just wait at the marker until it opens, which could be never(!?)
		if(ent->key < 0)
		{						// ent force locked
			return;
		}

/*		// TAT 2/3/2003 - want keys for SP
		if (G_IsSinglePlayerGame() && ent->key > 0)	// ent requires key
		{
			gitem_t *item = BG_FindItemForKey(ent->key, 0);
			if (!(activator->client->ps.stats[STAT_KEYS] & (1<<item->giTag)))
				return;
		}
		*/

		if(!Q_stricmp(ent->classname, "script_mover"))
		{						// RF, dont activate script_mover's
			return;
		}

		// hack fix for bigdoor1 on tram1_21

		if(!(ent->teammaster))
		{
			ent->active = qtrue;
			Use_BinaryMover(ent, activator, activator);
			G_UseTargets(ent->teammaster, activator);
			return;
		}

		if(ent->team && ent != ent->teammaster)
		{
			ent->teammaster->active = qtrue;
			Use_BinaryMover(ent->teammaster, activator, activator);
			G_UseTargets(ent->teammaster, activator);
		}
		else
		{
			ent->active = qtrue;
			Use_BinaryMover(ent, activator, activator);
			G_UseTargets(ent->teammaster, activator);
		}
	}
}

/*
==============
InitConstructible
==============
*/
void InitConstructible(gentity_t * ent)
{
	ent->s.angles2[0] = 0;

	ent->s.eType = ET_CONSTRUCTIBLE;
	trap_LinkEntity(ent);
}

void            func_constructible_spawn(gentity_t * self, gentity_t * other, gentity_t * activator);

/*
==============
func_constructible_use
==============
*/
void func_constructible_use(gentity_t * self, gentity_t * other, gentity_t * activator)
{
	self->s.angles2[0] = 0;
	self->grenadeFired = 0;
	self->s.modelindex = 0;

	if(!self->count2)
	{
		self->s.modelindex2 = atoi(self->model + 1);
	}
	else
	{
		self->s.modelindex2 = self->conbmodels[0];
	}

	if(!(self->spawnflags & CONSTRUCTIBLE_BLOCK_PATHS_WHEN_BUILD))
	{
		// backup...
		int             constructibleModelindex = self->s.modelindex;
		int             constructibleClipmask = self->clipmask;
		int             constructibleContents = self->r.contents;
		int             constructibleNonSolidBModel = (self->s.eFlags & EF_NONSOLID_BMODEL);

		// RF, AAS areas are now unusable
		if(!self->count2)
		{
			trap_SetBrushModel(self, self->model);
		}
		else
		{
			trap_SetBrushModel(self, va("*%i", self->conbmodels[self->count2 - 1]));	// set the final stage
		}
		trap_LinkEntity(self);

		// ...and restore
		trap_SetBrushModel(self, va("*%i", constructibleModelindex));
		self->clipmask = constructibleClipmask;
		self->r.contents = constructibleContents;
		if(!constructibleNonSolidBModel)
		{
			self->s.eFlags &= ~EF_NONSOLID_BMODEL;
		}
		trap_UnlinkEntity(self);
	}

	// Arnout: TODO - make this explode or so?
	self->use = func_constructible_spawn;
	trap_UnlinkEntity(self);

	// relink the objective info to get our indicator back
	if(self->parent)
	{
		trap_LinkEntity(self->parent);
		if(self->s.angles2[1])
		{
			self->s.angles2[1] = 0;	// Think_SetupObjectiveInfo needs it
			Think_SetupObjectiveInfo(self->parent);
		}
		else
		{
			// staged contruction got blown to stage 0
			if(self->parent->chain && self->parent->count2)
			{
				// find the constructible indicator and change team
				gentity_t      *indicator = &g_entities[self->parent->count2];

				indicator->s.teamNum = 3;
			}
		}

	}
	else
	{
		self->s.angles2[1] = 0;
	}

	if(!(self->spawnflags & 2))
	{
		self->takedamage = qfalse;
	}
}

/*
==============
func_constructible_spawn
==============
*/
void func_constructible_spawn(gentity_t * self, gentity_t * other, gentity_t * activator)
{
	trap_LinkEntity(self);
	self->use = func_constructible_use;
	// turn the brush to visible
}

/*
==============
func_constructible_explode
==============
*/
void func_constructible_explode(gentity_t * self, gentity_t * inflictor, gentity_t * attacker, int damage, int mod)
{
	if(self->desstages)
	{
		if(self->grenadeFired > 1)
		{
			// swap back one stage
			int             listedEntities, e;
			int             entityList[MAX_GENTITIES];
			gentity_t      *check, *block;

			self->s.angles2[0] = 0;

			if(self->s.angles2[1])
			{

				// relink the objective info to get our indicator back
				if(self->parent)
				{
					trap_LinkEntity(self->parent);
					//if( self->s.angles2[1] ) {
					if(self->s.angles2[1])
					{
						self->s.angles2[1] = 0;	// Think_SetupObjectiveInfo needs it
						Think_SetupObjectiveInfo(self->parent);
					}
					//}
				}
				else
				{
					self->s.angles2[1] = 0;
				}
			}

			// run the script
			if(self->grenadeFired == self->count2)
			{
				G_Script_ScriptEvent(self, "destroyed", "final");
			}
			else
			{
				switch (self->grenadeFired)
				{
						//case 1: G_Script_ScriptEvent( self, "destroyed", "stage1" ); break;
					case 2:
						G_Script_ScriptEvent(self, "destroyed", "stage2");
						break;
					case 3:
						G_Script_ScriptEvent(self, "destroyed", "stage3");
						break;
				}
			}

			self->grenadeFired--;

			// backup...
			{
				int             constructibleClipmask = self->clipmask;
				int             constructibleContents = self->r.contents;
				int             constructibleNonSolidBModel = (self->s.eFlags & EF_NONSOLID_BMODEL);

				trap_SetBrushModel(self, va("*%i", self->desbmodels[self->grenadeFired - 1]));

				// ...and restore
				self->clipmask = constructibleClipmask;
				self->r.contents = constructibleContents;
				if(!constructibleNonSolidBModel)
				{
					self->s.eFlags &= ~EF_NONSOLID_BMODEL;
				}
			}

			// deal with any entities in the solid
			listedEntities = trap_EntitiesInBox(self->r.absmin, self->r.absmax, entityList, MAX_GENTITIES);

			for(e = 0; e < listedEntities; e++)
			{
				check = &g_entities[entityList[e]];

				// ignore everything but items, players and missiles (grenades too)
				if(check->s.eType != ET_MISSILE && check->s.eType != ET_ITEM && check->s.eType != ET_PLAYER &&
				   !check->physicsObject)
				{
					continue;
				}

				if((block = G_TestEntityPosition(check)) == NULL)
				{
					continue;
				}

				if(block != self)
				{
					// the entity is blocked by another entity - that block this should take care of this itself
					continue;
				}

				if(check->client || check->s.eType == ET_CORPSE)
				{
					// gibs anything player like
					G_Damage(check, self, attacker, NULL, NULL, 9999, 0, MOD_CRUSH_CONSTRUCTIONDEATH);
				}
				else if(check->s.eType == ET_ITEM && check->item->giType == IT_TEAM)
				{
					// see if it's a critical entity, one that we can't just simply kill (basically flags)
					Team_DroppedFlagThink(check);
				}
				else
				{
					// remove the landmine from both teamlists
					if(check->s.eType == ET_MISSILE && check->methodOfDeath == MOD_LANDMINE)
					{
						mapEntityData_t *mEnt;

						if((mEnt = G_FindMapEntityData(&mapEntityData[0], check - g_entities)) != NULL)
						{
							G_FreeMapEntityData(&mapEntityData[0], mEnt);
						}

						if((mEnt = G_FindMapEntityData(&mapEntityData[1], check - g_entities)) != NULL)
						{
							G_FreeMapEntityData(&mapEntityData[1], mEnt);
						}
					}

					// just get rid of it
					G_TempEntity(check->s.origin, EV_ITEM_POP);
					G_FreeEntity(check);
				}
			}

			// Skills stuff
			if(G_GetWeaponClassForMOD(mod) >= self->constructibleStats.weaponclass)
			{
				G_AddKillSkillPointsForDestruction(attacker, mod, &self->constructibleStats);
			}
		}
		else
		{

			if(!(self->spawnflags & CONSTRUCTIBLE_NO_AAS_BLOCKING))
			{
				// RF, update blocking status
				if(!(self->spawnflags & CONSTRUCTIBLE_BLOCK_PATHS_WHEN_BUILD))
				{
					// RF, AAS areas are now unusable
					if(!self->count2)
					{
						trap_SetBrushModel(self, self->model);
					}
					else
					{
						trap_SetBrushModel(self, va("*%i", self->conbmodels[self->count2 - 1]));	// set the final stage
					}
					trap_LinkEntity(self);

					if(!self->count2)
					{
						trap_SetBrushModel(self, self->model);
					}
					else
					{
						trap_SetBrushModel(self, va("*%i", self->conbmodels[self->grenadeFired]));	// set the final stage
					}
					trap_UnlinkEntity(self);
				}
			}

			G_Script_ScriptEvent(self, "death", "");

			// Skills stuff
			if(G_GetWeaponClassForMOD(mod) >= self->constructibleStats.weaponclass)
			{
				G_AddKillSkillPointsForDestruction(attacker, mod, &self->constructibleStats);
			}

			// unlink
			G_UseEntity(self, inflictor, attacker);	// this will unlink (call func_constructible_use), if another function is used something is VERY wrong
		}
	}
	else
	{
		if(!(self->spawnflags & CONSTRUCTIBLE_NO_AAS_BLOCKING))
		{
			if(!(self->spawnflags & CONSTRUCTIBLE_BLOCK_PATHS_WHEN_BUILD))
			{
				// RF, AAS areas are now unusable
				if(!self->count2)
				{
					trap_SetBrushModel(self, self->model);
				}
				else
				{
					trap_SetBrushModel(self, va("*%i", self->conbmodels[self->count2 - 1]));	// set the final stage
				}
				trap_LinkEntity(self);

				if(!self->count2)
				{
					trap_SetBrushModel(self, self->model);
				}
				else
				{
					trap_SetBrushModel(self, va("*%i", self->conbmodels[self->grenadeFired]));	// set the final stage
				}
				trap_UnlinkEntity(self);
			}
		}

		// Skills stuff
		if(G_GetWeaponClassForMOD(mod) >= self->constructibleStats.weaponclass)
		{
			G_AddKillSkillPointsForDestruction(attacker, mod, &self->constructibleStats);
		}

		// unlink
		G_UseEntity(self, inflictor, attacker);	// this will unlink (call func_constructible_use), if another function is used something is VERY wrong
	}
}

/*
==============
func_constructible_underconstructionthink
==============
*/
//#define CONSTRUCT_PREDECAY_TIME   3000    // if not under construction for this duration, start decaying
#define CONSTRUCT_PREDECAY_TIME 30000	// if not under construction for this duration, start decaying
void func_constructible_underconstructionthink(gentity_t * ent)
{
	if(level.time - ent->lastHintCheckTime >= CONSTRUCT_PREDECAY_TIME)
	{
		//ent->s.angles2[0] -= 0.5f*(255.f/(ent->wait/(float)FRAMETIME));
		ent->s.angles2[0] = 0;	// insta-decay

		if(ent->s.angles2[0] < 5)
		{
			gentity_t      *te;

			// it decayed into oblivion

			// Play sound
			if(ent->parent->spawnflags & 8)
			{
				te = G_TempEntity(ent->parent->r.currentOrigin, EV_BUILDDECAYED_SOUND);
			}
			else
			{
				te = G_TempEntity(ent->s.origin2, EV_BUILDDECAYED_SOUND);
			}

			if(ent->count2)
			{

				// call script
				if(ent->grenadeFired == ent->count2)
				{
					G_Script_ScriptEvent(ent, "decayed", "final");
				}
				else
				{
					switch (ent->grenadeFired)
					{
						case 1:
							G_Script_ScriptEvent(ent, "decayed", "stage1");
							break;
						case 2:
							G_Script_ScriptEvent(ent, "decayed", "stage2");
							break;
						case 3:
							G_Script_ScriptEvent(ent, "decayed", "stage3");
							break;
					}
				}

				ent->grenadeFired--;
				ent->s.modelindex2 = 0;
				//trap_SetBrushModel( ent, va( "*%i", ent->conbmodels[ent->grenadeFired-1] ) );
			}
			else
			{
				// call script
				G_Script_ScriptEvent(ent, "decayed", "final");
			}

			// Stop sound
			if(ent->parent->spawnflags & 8)
			{
				ent->parent->s.loopSound = 0;
			}
			else
			{
				ent->s.loopSound = 0;
			}

			// call script
			G_Script_ScriptEvent(ent, "failed", "");
			//G_Script_ScriptEvent( ent, "death", "" );

			G_SetEntState(ent, STATE_DEFAULT);

			// get rid of us
			if(!ent->grenadeFired)
			{
				G_UseEntity(ent, ent, ent);

				if(ent->parent->chain && ent->parent->count2)
				{
					// find the constructible indicator and change team
					gentity_t      *indicator = &g_entities[ent->parent->count2];

					indicator->s.teamNum = 3;
				}
			}

			ent->think = NULL;
			ent->nextthink = 0;
			ent->s.angles2[0] = 0;

			ent->lastHintCheckTime = level.time;	// don't allow building again for a lil while
			return;
		}
	}

	ent->nextthink = level.time + FRAMETIME;
}

extern void     explosive_indicator_think(gentity_t * ent);

void func_constructiblespawn(gentity_t * ent)
{
	// count2: the number of construction stages
	// grenadeFired: the current stage (Starting at 1, 0 when destroyed)

	// see if we are staged
	if(ent->constages)
	{
		// parse them
		char           *ptr, *target_ptr;
		char            buf[128];
		gentity_t      *bmodel_ent;

		ent->count2 = 0;
		ent->grenadeFired = 0;

		for(target_ptr = ptr = ent->constages; *ptr; ptr++)
		{
			if(*ptr == ';')
			{
				Q_strncpyz(buf, target_ptr, (ptr - target_ptr) + 1);
				buf[ptr - target_ptr] = '\0';

				if(ent->count2 == MAX_CONSTRUCT_STAGES)
				{
					G_Error("'func_constructible' has more than %i targets in the constages key\n", MAX_CONSTRUCT_STAGES - 1);
					break;
				}

				if((bmodel_ent = G_FindByTargetname(NULL, buf)) != NULL)
				{
					char           *bmodel;

					if(Q_stricmp(bmodel_ent->classname, "func_brushmodel"))
					{
						G_Error("constages entry doesn't target a 'func_brushmodel'\n");
					}

					bmodel = bmodel_ent->model + 1;

					ent->conbmodels[ent->count2++] = atoi(bmodel);
				}

				target_ptr = ptr + 1;
			}
		}

		ent->conbmodels[ent->count2++] = atoi(ent->model + 1);	// the brushmodel of the func_constructible is the final stage

		// parse the destruction stages
		if(ent->count2 && ent->desstages)
		{
			int             numDesStages = 0;

			for(target_ptr = ptr = ent->desstages; *ptr; ptr++)
			{
				if(*ptr == ';')
				{
					Q_strncpyz(buf, target_ptr, (ptr - target_ptr) + 1);
					buf[ptr - target_ptr] = '\0';

					if(numDesStages == MAX_CONSTRUCT_STAGES - 1)
					{
						G_Error("'func_constructible' has more than %i targets in the desstages key\n", MAX_CONSTRUCT_STAGES - 2);
						break;
					}

					if((bmodel_ent = G_FindByTargetname(NULL, buf)) != NULL)
					{
						char           *bmodel;

						if(Q_stricmp(bmodel_ent->classname, "func_brushmodel"))
						{
							G_Error("desstages entry doesn't target a 'func_brushmodel'\n");
						}

						bmodel = bmodel_ent->model + 1;

						ent->desbmodels[numDesStages++] = atoi(bmodel);
					}

					target_ptr = ptr + 1;
				}
			}

			if(numDesStages != ent->count2 - 1)
			{
				G_Error("'func_constructible' has %i entries in the desstages and %i targets in the constages key\n",
						numDesStages, ent->count2 - 1);
			}
		}
	}

	InitConstructible(ent);

	if(!(ent->spawnflags & CONSTRUCTIBLE_START_BUILT))
	{							// if not start build
		ent->use = func_constructible_spawn;

		// RF, first we must temporarily "build" it so we can disable the areas that it touches
		if(!ent->count2)
		{
			trap_SetBrushModel(ent, ent->model);
		}
		else
		{
			trap_SetBrushModel(ent, va("*%i", ent->conbmodels[ent->count2 - 1]));	// set the final stage
		}

		if(!ent->count2)
		{
			// set initial contents
			trap_SetBrushModel(ent, ent->model);
			ent->s.modelindex = 0;
//          ent->s.solid = CONTENTS_SOLID;  // FIXME: allow other contents?
			trap_LinkEntity(ent);

			ent->s.modelindex2 = atoi(ent->model + 1);
		}
		else
		{
			// set initial contents
			trap_SetBrushModel(ent, va("*%i", ent->conbmodels[0]));
			ent->s.modelindex = 0;
//          ent->s.solid = CONTENTS_SOLID;  // FIXME: allow other contents?
			trap_LinkEntity(ent);

			ent->s.modelindex2 = ent->conbmodels[0];
		}

		trap_UnlinkEntity(ent);
	}
	else
	{
		ent->use = func_constructible_use;
		//ent->s.angles2[0] = 255;  // it's fully constructed

		if(!ent->count2)
		{
			trap_SetBrushModel(ent, ent->model);
		}
		else
		{
			trap_SetBrushModel(ent, va("*%i", ent->conbmodels[ent->count2 - 1]));	// set the final stage
			ent->grenadeFired = ent->count2;
		}

		ent->s.angles2[1] = 1;

		if(!(ent->spawnflags & CONSTRUCTIBLE_INVULNERABLE))
		{
			ent->takedamage = qtrue;

			// if dynamite-able, create a 'destructable' marker for the other team
			{
				gentity_t      *e;

				e = G_Spawn();

				e->r.svFlags = SVF_BROADCAST;
				e->classname = "explosive_indicator";
				{
					gentity_t      *tent = NULL;

					e->s.eType = ET_EXPLOSIVE_INDICATOR;

					while((tent = G_Find(tent, FOFS(target), ent->targetname)) != NULL)
					{
						if(tent->s.eType == ET_OID_TRIGGER)
						{
							if(tent->spawnflags & 8)
							{
								e->s.eType = ET_TANK_INDICATOR;
							}
						}
					}

				}
				e->s.pos.trType = TR_STATIONARY;

				if(ent->spawnflags & AXIS_CONSTRUCTIBLE)
				{
					e->s.teamNum = 1;
				}
				else if(ent->spawnflags & ALLIED_CONSTRUCTIBLE)
				{
					e->s.teamNum = 2;
				}

				// Find the trigger_objective_info that targets us (if not set before)
				if(!ent->parent)
				{
					gentity_t      *tent = NULL;

					while((tent = G_Find(tent, FOFS(target), ent->targetname)) != NULL)
					{
						if(tent->s.eType == ET_OID_TRIGGER)
						{
							ent->parent = tent;
							e->parent = tent;
						}
					}

					if(!ent->parent)
					{
						G_Error("'func_constructible' has a missing parent trigger_objective_info '%s'\n", ent->targetname);
					}
				}

				e->s.modelindex2 = ent->parent->s.teamNum;
				e->r.ownerNum = ent->s.number;
				e->think = explosive_indicator_think;
				e->nextthink = level.time + FRAMETIME;

				e->s.effect1Time = ent->constructibleStats.weaponclass;

				if(ent->parent->tagParent)
				{
					e->tagParent = ent->parent->tagParent;
					Q_strncpyz(e->tagName, ent->parent->tagName, MAX_QPATH);
				}
				else
				{
					VectorCopy(ent->r.absmin, e->s.pos.trBase);
					VectorAdd(ent->r.absmax, e->s.pos.trBase, e->s.pos.trBase);
					VectorScale(e->s.pos.trBase, 0.5, e->s.pos.trBase);
				}

				SnapVector(e->s.pos.trBase);

				trap_LinkEntity(e);
			}
		}
	}

	ent->die = func_constructible_explode;
}

/*QUAKED func_constructible (.9 .75 .15) ? START_BUILT INVULNERABLE AXIS_CONSTRUCTIBLE ALLIED_CONSTRUCTIBLE BLOCK_PATHS_WHEN_BUILT NO_AAS_BLOCKING AAS_SCRIPTED
A constructible object that functions as target for engineers.
"track"		functions as a group name. All entities with the
			same 'track' as the func_constructible will be
			constructed at the same time.
"constages"	list of target func_brushmodel entities (up to 3) that
			make up the construction stages (example: "stage1;stage2;")
"desstages"	list of target func_brushmodel entities (up to 3) that
			make up the destruction stages (example: "desstage1;desstage2;")

START_BUILT : building starts built
INVULNERABLE : building can't be destroyed
AXIS_CONSTRUCTIBLE : building is constructible by axis
ALLIED_CONSTRUCTIBLE : building is constructible by allies
BLOCK_PATHS_WHEN_BUILT : when built, areas touching structures are blocked
NO_AAS_BLOCKING : dont interact with AAS at all
*/

g_constructible_stats_t g_constructible_classes[NUM_CONSTRUCTIBLE_CLASSES] = {
	{.5f, 5, 5, 350, 1, 2500}
	,
	{1.f, 7.5f, 7.5f, 100, 2, 5000}
	,
	{1.5f, 10, 10, 100, 3, 7500}
};

void SP_func_constructible(gentity_t * ent)
{
	int /*health, wait, */ i;

//  char    *s;

	/*G_SpawnInt( "wait", "5000", &wait );
	   ent->wait = wait; */

	if(ent->spawnflags & AXIS_CONSTRUCTIBLE)
	{
		ent->s.teamNum = TEAM_AXIS;
	}
	else if(ent->spawnflags & ALLIED_CONSTRUCTIBLE)
	{
		ent->s.teamNum = TEAM_ALLIES;
	}
	else
	{
		G_Error("'func_constructible' does not have a team that can build it\n");
	}

	/*G_SpawnString ("score", "0", &s);
	   ent->accuracy = atof (s);

	   G_SpawnInt( "health", "100", &health ); // default low enough to get killed by dynamite
	   ent->sound1to2 = ent->health = health; */

	memset(&ent->constructibleStats, 0, sizeof(ent->constructibleStats));
	G_SpawnInt("constructible_class", "0", &i);
	i--;						// non-coder friendlyness. Aren't we nice?
	if(i > 0 && i <= NUM_CONSTRUCTIBLE_CLASSES)
	{
		ent->constructibleStats = g_constructible_classes[i];

		G_SpawnFloat("constructible_chargebarreq", va("%f", ent->constructibleStats.chargebarreq),
					 &ent->constructibleStats.chargebarreq);
		G_SpawnFloat("constructible_constructxpbonus", va("%f", ent->constructibleStats.constructxpbonus),
					 &ent->constructibleStats.constructxpbonus);
		G_SpawnFloat("constructible_destructxpbonus", va("%f", ent->constructibleStats.destructxpbonus),
					 &ent->constructibleStats.destructxpbonus);
		G_SpawnInt("constructible_health", va("%i", ent->constructibleStats.health), &ent->constructibleStats.health);
		G_SpawnInt("constructible_weaponclass", va("%i", ent->constructibleStats.weaponclass),
				   &ent->constructibleStats.weaponclass);
		G_SpawnInt("constructible_duration", va("%i", ent->constructibleStats.duration), &ent->constructibleStats.duration);
	}
	else
	{
		G_SpawnFloat("constructible_chargebarreq", "1", &ent->constructibleStats.chargebarreq);
		G_SpawnFloat("constructible_constructxpbonus", "0", &ent->constructibleStats.constructxpbonus);
		G_SpawnFloat("constructible_destructxpbonus", "0", &ent->constructibleStats.destructxpbonus);
		G_SpawnInt("constructible_health", "100", &ent->constructibleStats.health);
		G_SpawnInt("constructible_weaponclass", "0", &ent->constructibleStats.weaponclass);
		G_SpawnInt("constructible_duration", "5000", &ent->constructibleStats.duration);
	}
	ent->constructibleStats.weaponclass--;
	ent->health = ent->constructibleStats.health;

	ent->s.dmgFlags = 0;

	ent->think = func_constructiblespawn;
	ent->nextthink = level.time + (2 * FRAMETIME);
}

/*QUAKED func_brushmodel (.9 .50 .50) ?
A brushmodel that gets deleted on the fourth frame. We use this to hijack it's brushmodel in func_constructible
entities that are based around staged construction.
*/
void func_brushmodel_delete(gentity_t * ent)
{
	G_FreeEntity(ent);
}

void SP_func_brushmodel(gentity_t * ent)
{
	if(!ent->model)
	{
		G_Error("'func_brushmodel' does not have a model\n");
	}

	if(ent->targetname && level.numBrushModels < 128)
	{
		level.brushModelInfo[level.numBrushModels].model = atoi(ent->model + 1);
		Q_strncpyz(level.brushModelInfo[level.numBrushModels].modelname, ent->targetname, 32);
		level.numBrushModels++;
	}

	ent->think = func_brushmodel_delete;
	ent->nextthink = level.time + (3 * FRAMETIME);
}

// Gordon: debris test
/*
===========
SP_func_debris
===========
*/
debrisChunk_t  *G_AllocDebrisChunk(void)
{
	if(level.numDebrisChunks >= MAX_DEBRISCHUNKS)
	{
		G_Error("ERROR: MAX_DEBRISCHUNKS(%i) hit.", MAX_DEBRISCHUNKS);
		return NULL;
	}

	return &level.debrisChunks[level.numDebrisChunks++];
}

void SP_func_debris(gentity_t * ent)
{
	debrisChunk_t  *debris;

	if(!ent->model || !*ent->model)
	{
		G_FreeEntity(ent);
		G_Printf(S_COLOR_YELLOW "WARNING: 'func_debris' without a valid model\n");
		return;
	}

	if(!ent->target || !*ent->target)
	{
		G_Error("ERROR: func_debris with no target");
	}
	if(!ent->targetname || !*ent->targetname)
	{
		G_Error("ERROR: func_debris with no targetname");
	}

	debris = G_AllocDebrisChunk();

	debris->model = atoi(ent->model + 1);

	Q_strncpyz(debris->target, ent->target, sizeof(debris->target));
	Q_strncpyz(debris->targetname, ent->targetname, sizeof(debris->targetname));

	VectorCopy(ent->s.origin, debris->origin);

	G_SpawnFloat("speed", "800", &debris->velocity[0]);

	G_FreeEntity(ent);
}

void G_LinkDamageParents(void)
{
	int             i;

	for(i = 0; i < level.num_entities; i++)
	{
		if(!g_entities[i].damageparent || !*g_entities[i].damageparent)
		{
			continue;
		}

		if(!(g_entities[i].dmgparent = G_FindByTargetname(NULL, g_entities[i].damageparent)))
		{
			G_Error("Error: Failed to find damageparent: %s\n", g_entities[i].damageparent);
		}
	}
}

void G_LinkDebris(void)
{
	float           speed;
	int             i;
	gentity_t      *target;

	for(i = 0; i < level.numDebrisChunks; i++)
	{
		debrisChunk_t  *debris = &level.debrisChunks[i];

		target = G_FindByTargetname(NULL, debris->target);
		if(!target)
		{
			// CHRUKER: b083 - Improving script error messages
			G_Error("ERROR: func_debris with no target (%s)", debris->target);
		}

		speed = debris->velocity[0];

		VectorSubtract(target->s.origin, debris->origin, debris->velocity);
		VectorNormalize(debris->velocity);
		VectorScale(debris->velocity, speed, debris->velocity);
		trap_SnapVector(debris->velocity);
	}
}

// ===================
