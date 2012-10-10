/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2000-2006 Tim Angus

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
#include "g_bot.h"
#include "../../engine/botlib/nav.h"

#ifndef RAND_MAX
#define RAND_MAX 32768
#endif

botMemory_t g_botMind[MAX_CLIENTS];

void BotDPrintf(const char* fmt, ...) {
	if(g_bot_debug.integer) {
		va_list argptr;
		char    text[ 1024 ];

		va_start( argptr, fmt );
		Q_vsnprintf( text, sizeof( text ), fmt, argptr );
		va_end( argptr );

		trap_Print( text );
	}
}
/*
=======================
Scoring functions for logic
=======================
*/
float BotGetBaseRushScore(gentity_t *ent) {
	switch(ent->s.weapon) {
		case WP_LUCIFER_CANNON: return 1.0f; break;
		case WP_MACHINEGUN: return 0.5f; break;
		case WP_PULSE_RIFLE: return 0.7f; break;
		case WP_LAS_GUN: return 0.7f; break;
		case WP_SHOTGUN: return 0.2f; break;
		case WP_CHAINGUN: if(BG_InventoryContainsUpgrade(UP_BATTLESUIT,ent->client->ps.stats))
							  return 0.5f;
						  else
							  return 0.2f;
			break;
		case WP_ALEVEL0: return 0.0f; break;
		case WP_ALEVEL1: return 0.2f; break;
		case WP_ALEVEL2: return 0.7f; break;
		case WP_ALEVEL3: return 0.8f; break;
		case WP_ALEVEL4: return 1.0f; break;
		default: return 0.5f; break;
	}
}

float BotGetEnemyPriority(gentity_t *self, gentity_t *ent) {
	float enemyScore;
	float distanceScore;
	distanceScore = Distance(self->s.origin,ent->s.origin);

	if(ent->client) {
		switch(ent->s.weapon) {
		case WP_ALEVEL0:
			enemyScore = 0.1;
			break;
		case WP_ALEVEL1:
		case WP_ALEVEL1_UPG:
			enemyScore = 0.3;
			break;
		case WP_ALEVEL2:
			enemyScore = 0.4;
			break;
		case WP_ALEVEL2_UPG:
			enemyScore = 0.7;
			break;
		case WP_ALEVEL3:
			enemyScore = 0.7;
			break;
		case WP_ALEVEL3_UPG:
			enemyScore = 0.8;
			break;
		case WP_ALEVEL4:
			enemyScore = 1.0;
			break;
		case WP_BLASTER:
			enemyScore = 0.2;
			break;
		case WP_MACHINEGUN:
			enemyScore = 0.4;
			break;
		case WP_PAIN_SAW:
			enemyScore = 0.4;
			break;
		case WP_LAS_GUN:
			enemyScore = 0.4;
			break;
		case WP_MASS_DRIVER:
			enemyScore = 0.4;
			break;
		case WP_CHAINGUN:
			enemyScore = 0.6;
			break;
		case WP_FLAMER:
			enemyScore = 0.6;
			break;
		case WP_PULSE_RIFLE:
			enemyScore = 0.5;
			break;
		case WP_LUCIFER_CANNON:
			enemyScore = 1.0;
			break;
		default: enemyScore = 0.5; break;
		}
	} else {
		switch(ent->s.modelindex) {
		case BA_H_MGTURRET:
			enemyScore = 0.6;
			break;
		case BA_H_REACTOR:
			enemyScore = 0.5;
			break;
		case BA_H_TESLAGEN:
			enemyScore = 0.7;
			break;
		case BA_H_SPAWN:
			enemyScore = 0.9;
			break;
		case BA_H_ARMOURY:
			enemyScore = 0.8;
			break;
		case BA_H_MEDISTAT:
			enemyScore = 0.6;
			break;
		case BA_H_DCC:
			enemyScore = 0.5;
			break;
		case BA_A_ACIDTUBE:
			enemyScore = 0.7;
			break;
		case BA_A_SPAWN:
			enemyScore = 0.9;
			break;
		case BA_A_OVERMIND:
			enemyScore = 0.5;
			break;
		case BA_A_HIVE:
			enemyScore = 0.7;
			break;
		case BA_A_BOOSTER:
			enemyScore = 0.4;
			break;
		case BA_A_TRAPPER:
			enemyScore = 0.8;
			break;
		default: enemyScore = 0.5; break;

		}
	}
	return enemyScore * 1000 / distanceScore;
}
/*
=======================
Entity Querys
=======================
*/
gentity_t* BotFindBuilding(gentity_t *self, int buildingType, int range) {
	float minDistance = -1;
	gentity_t* closestBuilding = NULL;
	float newDistance;
	float rangeSquared = Square(range);
	gentity_t *target = &g_entities[MAX_CLIENTS];

	for(int  i = MAX_CLIENTS; i < ENTITYNUM_MAX_NORMAL; i++, target++ ) {
		if(!target->inuse)
			continue;
		if( target->s.eType == ET_BUILDABLE && target->s.modelindex == buildingType && (target->buildableTeam == TEAM_ALIENS || (target->powered && target->spawned)) && target->health > 0) {
			newDistance = DistanceSquared( self->s.origin, target->s.origin);
			if(newDistance > rangeSquared)
				continue;
			if( newDistance < minDistance || minDistance == -1) {
				minDistance = newDistance;
				closestBuilding = target;
			}
		}
	}
	return closestBuilding;
}

//overloading FTW
gentity_t* BotFindBuilding(gentity_t *self, int buildingType) {
	float minDistance = -1;
	gentity_t* closestBuilding = NULL;
	float newDistance;
	gentity_t *target = &g_entities[MAX_CLIENTS];

	for( int i = MAX_CLIENTS; i < ENTITYNUM_MAX_NORMAL; i++, target++ ) {
		if(!target->inuse)
			continue;
		if( target->s.eType == ET_BUILDABLE && target->s.modelindex == buildingType && (target->buildableTeam == TEAM_ALIENS || (target->powered && target->spawned)) && target->health > 0) {
			newDistance = DistanceSquared(self->s.origin,target->s.origin);
			if( newDistance < minDistance|| minDistance == -1) {
				minDistance = newDistance;
				closestBuilding = target;
			}
		}
	}
	return closestBuilding;
}

void BotFindClosestBuildings(gentity_t *self, botClosestBuildings_t *closest) {

	memset(closest,0,sizeof(botClosestBuildings_t));

	for(gentity_t *testEnt = &g_entities[MAX_CLIENTS];testEnt<&g_entities[level.num_entities-1];testEnt++) {
		float newDist;
		//ignore entities that arnt in use
		if(!testEnt->inuse)
			continue;

		//ignore dead targets
		if(testEnt->health <= 0)
			continue;

		//skip non buildings
		if(testEnt->s.eType != ET_BUILDABLE)
			continue;

		//skip human buildings that are currently building or arn't powered
		if(testEnt->buildableTeam == TEAM_HUMANS && (!testEnt->powered || !testEnt->spawned))
			continue;

		newDist = Distance(self->s.origin,testEnt->s.origin);
		switch(testEnt->s.modelindex) {
			case BA_A_SPAWN:
				if(newDist < closest->egg.distance || closest->egg.distance == 0) {
					closest->egg.ent = testEnt;
					closest->egg.distance = newDist;
				}
				break;
			case BA_A_OVERMIND:
				if(newDist < closest->overmind.distance || closest->overmind.distance == 0) {
					closest->overmind.ent = testEnt;
					closest->overmind.distance = newDist;
				}
				break;
			case BA_A_BARRICADE:
				if(newDist < closest->barricade.distance || closest->barricade.distance == 0) {
					closest->barricade.ent = testEnt;
					closest->barricade.distance = newDist;
				}
				break;
			case BA_A_ACIDTUBE:
				if(newDist < closest->acidtube.distance || closest->acidtube.distance == 0) {
					closest->acidtube.ent = testEnt;
					closest->acidtube.distance = newDist;
				}
				break;
			case BA_A_TRAPPER:
				if(newDist < closest->trapper.distance || closest->trapper.distance == 0) {
					closest->trapper.ent = testEnt;
					closest->trapper.distance = newDist;
				}
				break;
			case BA_A_BOOSTER:
				if(newDist < closest->booster.distance || closest->booster.distance == 0) {
					closest->booster.ent = testEnt;
					closest->booster.distance = newDist;
				}
				break;
			case BA_A_HIVE:
				if(newDist < closest->hive.distance || closest->hive.distance == 0) {
					closest->hive.ent = testEnt;
					closest->hive.distance = newDist;
				}
				break;
			case BA_H_SPAWN:
				if(newDist < closest->telenode.distance || closest->telenode.distance == 0) {
					closest->telenode.ent = testEnt;
					closest->telenode.distance = newDist;
				}
				break;
			case BA_H_MGTURRET:
				if(newDist < closest->turret.distance || closest->turret.distance == 0) {
					closest->turret.ent = testEnt;
					closest->turret.distance = newDist;
				}
				break;
			case BA_H_TESLAGEN:
				if(newDist < closest->tesla.distance || closest->tesla.distance == 0) {
					closest->tesla.ent = testEnt;
					closest->tesla.distance = newDist;
				}
				break;
			case BA_H_ARMOURY:
				if(newDist < closest->armoury.distance || closest->armoury.distance == 0) {
					closest->armoury.ent = testEnt;
					closest->armoury.distance = newDist;
				}
				break;
			case BA_H_DCC:
				if(newDist < closest->dcc.distance || closest->dcc.distance == 0) {
					closest->dcc.ent = testEnt;
					closest->dcc.distance = newDist;
				}
				break;
			case BA_H_MEDISTAT:
				if(newDist < closest->medistation.distance || closest->medistation.distance == 0) {
					closest->medistation.ent = testEnt;
					closest->medistation.distance = newDist;
				}
				break;
			case BA_H_REACTOR:
				if(newDist < closest->reactor.distance || closest->reactor.distance == 0) {
					closest->reactor.ent = testEnt;
					closest->reactor.distance = sqrt(newDist);
				}
				break;
			case BA_H_REPEATER:
				if(newDist < closest->repeater.distance || closest->repeater.distance == 0) {
					closest->repeater.ent = testEnt;
					closest->repeater.distance = newDist;
				}
				break;
		}
	}
}

gentity_t* BotFindBestEnemy( gentity_t *self ) {
	float bestVisibleEnemyScore = 0;
	float bestInvisibleEnemyScore = 0;
	gentity_t *bestVisibleEnemy = NULL;
	gentity_t *bestInvisibleEnemy = NULL;

	for(gentity_t *target=g_entities;target<&g_entities[level.num_entities-1];target++) {
		float newScore;
		//ignore entities that arnt in use
		if(!target->inuse)
			continue;

		//ignore dead targets
		if(target->health <= 0)
			continue;

		//ignore buildings if we cant attack them
		if(target->s.eType == ET_BUILDABLE && (!g_bot_attackStruct.integer || self->client->ps.stats[STAT_CLASS] == PCL_ALIEN_LEVEL0))
			continue;

		//ignore neutrals
		if(BotGetTeam(target) == TEAM_NONE)
			continue;

		//ignore teamates
		if(BotGetTeam(target) == BotGetTeam(self))
			continue;

		//ignore spectators
		if(target->client) {
			if(target->client->sess.spectatorState != SPECTATOR_NOT)
				continue;
		}

		if(DistanceSquared(self->s.origin,target->s.origin) > Square(ALIENSENSE_RANGE))
			continue;

		newScore = BotGetEnemyPriority(self, target);

		if(newScore > bestVisibleEnemyScore && G_Visible(self,target,MASK_SHOT)) {
			//store the new score and the index of the entity
			bestVisibleEnemyScore = newScore;
			bestVisibleEnemy = target;
		} else if(newScore > bestInvisibleEnemyScore && BotGetTeam(self) == TEAM_ALIENS) {
			bestInvisibleEnemyScore = newScore;
			bestInvisibleEnemy = target;
		}
	}
	if(bestVisibleEnemy || BotGetTeam(self) == TEAM_HUMANS)
		return bestVisibleEnemy;
	else
		return bestInvisibleEnemy;
}

gentity_t* BotFindClosestEnemy( gentity_t *self ) {
	gentity_t* closestEnemy = NULL;
	float minDistance = Square(ALIENSENSE_RANGE);
	for(gentity_t *target = g_entities;target<&g_entities[level.num_entities-1];target++) {
		float newDistance;
		//ignore entities that arnt in use
		if(!target->inuse)
			continue;

		//ignore dead targets
		if(target->health <= 0)
			continue;

		//ignore buildings if we cant attack them
		if(target->s.eType == ET_BUILDABLE && (!g_bot_attackStruct.integer || self->client->ps.stats[STAT_CLASS] == PCL_ALIEN_LEVEL0))
			continue;

		//ignore neutrals
		if(BotGetTeam(target) == TEAM_NONE)
			continue;

		//ignore teamates
		if(BotGetTeam(target) == BotGetTeam(self))
			continue;

		//ignore spectators
		if(target->client) {
			if(target->client->sess.spectatorState != SPECTATOR_NOT)
				continue;
		}
		newDistance = DistanceSquared(self->s.origin,target->s.origin);
		if(newDistance <= minDistance) {
			minDistance = newDistance;
			closestEnemy = target;
		}
	}
	return closestEnemy;
}

botTarget_t BotGetRushTarget(gentity_t *self) {
	botTarget_t target;
	gentity_t* rushTarget = NULL;
	if(BotGetTeam(self) == TEAM_HUMANS) {
		if(self->botMind->closestBuildings.egg.ent) {
			rushTarget = self->botMind->closestBuildings.egg.ent;
		} else if(self->botMind->closestBuildings.overmind.ent) {
			rushTarget = self->botMind->closestBuildings.overmind.ent;
		}
	} else {//team aliens
		if(self->botMind->closestBuildings.telenode.ent) {
			rushTarget = self->botMind->closestBuildings.telenode.ent;
		} else if(self->botMind->closestBuildings.reactor.ent) {
			rushTarget = self->botMind->closestBuildings.reactor.ent;
		}
	}
	BotSetTarget(&target,rushTarget,NULL);
	return target;
}

botTarget_t BotGetRetreatTarget(gentity_t *self) {
	botTarget_t target;
	gentity_t* retreatTarget = NULL;
	//FIXME, this seems like it could be done better...
	if(self->client->ps.stats[STAT_TEAM] == TEAM_HUMANS) {
		if(self->botMind->closestBuildings.reactor.ent) {
			retreatTarget = self->botMind->closestBuildings.reactor.ent;
		}
	} else {
		if(self->botMind->closestBuildings.overmind.ent) {
			retreatTarget = self->botMind->closestBuildings.overmind.ent;
		}
	}
	BotSetTarget(&target,retreatTarget,NULL);
	return target;
}

/*
========================
BotTarget Helpers
========================
*/

void BotSetTarget(botTarget_t *target, gentity_t *ent, vec3_t *pos) {
	if(ent) {
		target->ent = ent;
		VectorClear(target->coord);
		target->inuse = qtrue;
	} else if(pos) {
		target->ent = NULL;
		VectorCopy(*pos,target->coord);
		target->inuse = qtrue;
	} else {
		target->ent = NULL;
		VectorClear(target->coord);
		target->inuse = qfalse;
	}
}

void BotSetGoal(gentity_t *self, gentity_t *ent, vec3_t *pos) {
	BotSetTarget(&self->botMind->goal, ent, pos);
}

qboolean BotTargetInAttackRange(gentity_t *self, botTarget_t target) {
	float range, secondaryRange;
	vec3_t forward,right,up;
	vec3_t muzzle;
	vec3_t maxs, mins;
	vec3_t targetPos;
	vec3_t end;
	trace_t trace;
	float width = 0, height = 0;
	AngleVectors( self->client->ps.viewangles, forward, right, up);
	CalcMuzzlePoint( self, forward, right, up , muzzle);
	BotGetTargetPos(target,targetPos);
	switch(self->client->ps.weapon) {
		case WP_ABUILD:
			range = ABUILDER_CLAW_RANGE;
			secondaryRange = 0;
			width = height = ABUILDER_CLAW_WIDTH;
			break;
		case WP_ABUILD2:
			range = ABUILDER_CLAW_RANGE;
			secondaryRange = 300; //An arbitrary value for the blob launcher, has nothing to do with actual range
			width = height = ABUILDER_CLAW_WIDTH;
			break;
		case WP_ALEVEL0:
			range = LEVEL0_BITE_RANGE;
			secondaryRange = 0;
			break;
		case WP_ALEVEL1:
			range = LEVEL1_CLAW_RANGE;
			secondaryRange = 0;
			width = height = LEVEL1_CLAW_WIDTH;
			break;
		case WP_ALEVEL1_UPG:
			range = LEVEL1_CLAW_RANGE;
			secondaryRange = LEVEL1_PCLOUD_RANGE;
			width = height = LEVEL1_CLAW_WIDTH;
			break;
		case WP_ALEVEL2:
			range = LEVEL2_CLAW_RANGE;
			secondaryRange = 0;
			width = height = LEVEL2_CLAW_WIDTH;
			break;
		case WP_ALEVEL2_UPG:
			range = LEVEL2_CLAW_RANGE;
			secondaryRange = LEVEL2_AREAZAP_RANGE;
			width = height = LEVEL2_CLAW_WIDTH;
			break;
		case WP_ALEVEL3:
			range = LEVEL3_CLAW_RANGE;
			//need to check if we can pounce to the target
			secondaryRange = LEVEL3_POUNCE_JUMP_MAG; //An arbitrary value for pounce, has nothing to do with actual range
			break;
		case WP_ALEVEL3_UPG:
			range = LEVEL3_CLAW_RANGE;
			//we can pounce, or we have barbs
			secondaryRange = LEVEL3_POUNCE_JUMP_MAG_UPG; //An arbitrary value for pounce and barbs, has nothing to do with actual range
			if(self->client->ps.Ammo > 0)
				secondaryRange = 900;
			width = height = LEVEL3_CLAW_WIDTH;
			break;
		case WP_ALEVEL4:
			range = LEVEL4_CLAW_RANGE;
			secondaryRange = 0; //Using 0 since tyrant rush is basically just movement, not a ranged attack
			break;
		case WP_HBUILD:
			range = 100;
			secondaryRange = 0;
			break;
		case WP_PAIN_SAW:
			range = PAINSAW_RANGE;
			secondaryRange = 0;
			break;
		case WP_FLAMER:
			range = FLAMER_SPEED;
			secondaryRange = 0;
			width = height = FLAMER_SIZE;
			// Correct muzzle so that the missile does not start in the ceiling
			VectorMA( muzzle, -7.0f, up, muzzle );

			// Correct muzzle so that the missile fires from the player's hand
			VectorMA( muzzle, 4.5f, right, muzzle);
			break;
		case WP_SHOTGUN:
			range = (50 * 8192)/SHOTGUN_SPREAD; //50 is the maximum radius we want the spread to be
			secondaryRange = 0;
			break;
		case WP_MACHINEGUN:
			range = (100 * 8192)/RIFLE_SPREAD; //100 is the maximum radius we want the spread to be
			secondaryRange = 0;
			break;
		case WP_CHAINGUN:
			range = (60 * 8192)/CHAINGUN_SPREAD; //60 is the maximum radius we want the spread to be
			secondaryRange = 0;
			break;
		default:
			range = 4098 * 4; //large range for guns because guns have large ranges :)
			secondaryRange = 0; //no secondary attack
		}
	VectorSet(maxs, width, width, width);
	VectorSet(mins, -width, -width, -height);
	VectorSubtract(targetPos,muzzle,forward);
	VectorNormalize(forward);
	VectorScale(forward,MAX(range,secondaryRange),end);
	trap_Trace(&trace,muzzle,mins,maxs,targetPos,self->s.number,MASK_SHOT);

	if(trace.entityNum < ENTITYNUM_MAX_NORMAL && Distance(muzzle,trace.endpos) <= MAX(range,secondaryRange)) {
		trap_Trace(&trace,muzzle,mins,maxs,end,self->s.number,MASK_SHOT);

		if(BotGetTeam(self) != BotGetTeam(&g_entities[trace.entityNum])
			&& BotAimNegligence(self,target) <= BOT_AIM_NEGLIGENCE)
			return qtrue;
		else
			return qfalse;
	} else
		return qfalse;
}

qboolean BotTargetIsVisible( gentity_t *self, botTarget_t target, int mask ) {
	trace_t trace;
	vec3_t  muzzle, targetPos;
	vec3_t  forward, right, up;

	AngleVectors( self->client->ps.viewangles, forward, right, up );
	CalcMuzzlePoint( self, forward, right, up, muzzle );
	BotGetTargetPos(target, targetPos);
	trap_Trace( &trace, muzzle, NULL, NULL,targetPos, self->s.number, mask);

	if( trace.surfaceFlags & SURF_NOIMPACT )
		return qfalse;

	//target is in range
	if( (trace.entityNum == BotGetTargetEntityNumber(target) || trace.fraction == 1.0f) && !trace.startsolid )
		return qtrue;
	return qfalse;
}
/*
========================
Bot Aiming
========================
*/
void BotGetIdealAimLocation(gentity_t *self, botTarget_t target, vec3_t aimLocation) {
	vec3_t mins;
	//get the position of the target
	BotGetTargetPos(target, aimLocation);

	if(BotGetTargetType(target) != ET_BUILDABLE && BotTargetIsEntity(target) && BotGetTeam(target) == TEAM_HUMANS) {

		//aim at head
		aimLocation[2] += target.ent->r.maxs[2] * 0.85;

	} else if(BotGetTargetType(target) == ET_BUILDABLE || BotGetTeam(target) == TEAM_ALIENS) {
		//make lucifer cannons aim ahead based on the target's velocity
		if(self->client->ps.weapon == WP_LUCIFER_CANNON && self->botMind->botSkill.level >= 5) {
			VectorMA(aimLocation, Distance(self->s.origin, aimLocation) / LCANNON_SPEED, target.ent->s.pos.trDelta, aimLocation);
		}
	} else {
		//get rid of 'bobing' motion when aiming at waypoints by making the aimlocation the same height above ground as our viewheight
		VectorCopy(self->botMind->route[0],aimLocation);
		VectorCopy(BG_ClassConfig((class_t) self->client->ps.stats[STAT_CLASS])->mins,mins);
		aimLocation[2] +=  self->client->ps.viewheight - mins[2] - 8;
	}
}

void BotAimAtLocation( gentity_t *self, vec3_t target, usercmd_t *rAngles)
{
	vec3_t aimVec, aimAngles, viewBase;
	//vec3_t oldAimVec;
	//vec3_t refNormal, grapplePoint, xNormal, viewBase;
	//vec3_t highPoint;
	//float turnAngle;

	if( ! (self && self->client) )
		return;
	//VectorCopy( self->s.pos.trBase, viewBase );
	//viewBase[2] += self->client->ps.viewheight;
	BG_GetClientViewOrigin( &self->client->ps, viewBase);
	VectorSubtract( target, viewBase, aimVec );
	//VectorCopy(aimVec, oldAimVec);

	//Much of this isnt needed anymore due to BG_GetClientViewOrigin and/or BG_GetClientNormal...
	//VectorSet(refNormal, 0.0f, 0.0f, 1.0f);
	// VectorCopy( self->client->ps.grapplePoint, grapplePoint);
	//cross the reference normal and the surface normal to get the rotation axis
	//CrossProduct( refNormal, grapplePoint, xNormal );
	//VectorNormalize( xNormal );
	// turnAngle = RAD2DEG( acos( DotProduct( refNormal, grapplePoint ) ) );
	//G_Printf("turn angle: %f\n", turnAngle);

	//if(self->client->ps.stats[ STAT_STATE ] & SS_WALLCLIMBING && self->client->ps.eFlags & EF_WALLCLIMBCEILING) {
	//NOTE: the grapplePoint is here not an inverted refNormal :(
	// RotatePointAroundVector( aimVec, grapplePoint, oldAimVec, -180.0);
	//}
	//if( turnAngle != 0.0f)
	//  RotatePointAroundVector( aimVec, xNormal, oldAimVec, -turnAngle);

	vectoangles( aimVec, aimAngles );

	VectorSet(self->client->ps.delta_angles, 0.0f, 0.0f, 0.0f);

	for(int i = 0; i < 3; i++ ) {
		aimAngles[i] = ANGLE2SHORT(aimAngles[i]);
	}

	//save bandwidth
	SnapVector(aimAngles);
	rAngles->angles[0] = aimAngles[0];
	rAngles->angles[1] = aimAngles[1];
	rAngles->angles[2] = aimAngles[2];
}

void BotSlowAim( gentity_t *self, vec3_t target, float slowAmount) {
	vec3_t viewBase;
	vec3_t aimVec, forward;
	vec3_t skilledVec;
	float length;
	float slow;

	if( !(self && self->client) )
		return;
	//clamp to 0-1
	slow = Com_Clamp(0.1f,1.0,slowAmount);

	//get the point that the bot is aiming from
	BG_GetClientViewOrigin(&self->client->ps,viewBase);

	//get the Vector from the bot to the enemy (ideal aim Vector)
	VectorSubtract( target, viewBase, aimVec );
	length = VectorNormalize(aimVec);

	//take the current aim Vector
	AngleVectors( self->client->ps.viewangles, forward, NULL, NULL);

	float cosAngle = DotProduct(forward,aimVec);
	cosAngle = (cosAngle + 1)/2;
	cosAngle = 1 - cosAngle;
	cosAngle = Com_Clamp(0.1,0.5,cosAngle);
	VectorLerp( forward, aimVec, slow * (cosAngle), skilledVec);

	//now find a point to return, this point will be aimed at
	VectorMA(viewBase, length, skilledVec,target);
}
//blatently ripped from ShotgunPattern() in g_weapon.c :)
void BotShakeAim( gentity_t *self, vec3_t rVec ){
	vec3_t forward, right, up, diffVec;
	int seed;
	float r,u, length, speedAngle;
	AngleVectors(self->client->ps.viewangles, forward, right, up);
	//seed crandom
	seed = (int) rand() & 255;

	VectorSubtract(rVec,self->s.origin, diffVec);

	length = VectorNormalize(diffVec)/1000;

	speedAngle=RAD2DEG(acos(DotProduct(forward,diffVec)))/100;
	r = Q_crandom(&seed) * self->botMind->botSkill.aimShake * length * speedAngle;
	u = Q_crandom(&seed) * self->botMind->botSkill.aimShake * length * speedAngle;

	VectorMA(rVec, r, right, rVec);
	VectorMA(rVec,u,up,rVec);
}

float BotAimNegligence(gentity_t *self, botTarget_t target) {
	vec3_t forward;
	vec3_t ideal;
	vec3_t targetPos;
	vec3_t viewPos;
	float angle;
	AngleVectors(self->client->ps.viewangles,forward, NULL, NULL);
	BG_GetClientViewOrigin(&self->client->ps,viewPos);
	BotGetIdealAimLocation(self, target, targetPos);
	VectorSubtract(targetPos,viewPos,ideal);
	VectorNormalize(ideal);
	angle = DotProduct(ideal,forward);
	angle = RAD2DEG(acos(angle));
	return angle;
}
/*
========================
Bot Team Querys
========================
*/
qboolean BotTeamateIsInModus(gentity_t *self, botModus_t modus) {
	gentity_t *testEnt;
	int botNumbers[MAX_CLIENTS];
	int numBots = FindBots(botNumbers,MAX_CLIENTS,BotGetTeam(self));
	//slots 0-MAX_CLIENTS are reserved for clients
	for(int i=0; i < numBots; i++) {
		//dont check ourselves
		if(self->s.number == botNumbers[i])
			continue;
		testEnt = &g_entities[botNumbers[i]];

		//bot is in specified modus
		if(testEnt->botMind->modus == modus) {
			return qtrue;
		}
	}
	//we havent returned true so there are no bots in that modus
	return qfalse;
}

int FindBots(int *botEntityNumbers, int maxBots, team_t team) {
	gentity_t *testEntity;
	int numBots = 0;
	memset(botEntityNumbers,0,sizeof(int)*maxBots);
	for(int i=0;i<MAX_CLIENTS;i++) {
		testEntity = &g_entities[i];
		if(testEntity->r.svFlags & SVF_BOT) {
			if(testEntity->client->pers.teamSelection == team && numBots < maxBots) {
				botEntityNumbers[numBots++] = i;
			}
		}
	}
	return numBots;
}

qboolean PlayersBehindBotInSpawnQueue( gentity_t *self )
{
	//this function only checks if there are Humans in the SpawnQueue
	//which are behind the bot
	int i;
	int botPos = 0, lastPlayerPos = 0;
	spawnQueue_t *sq;

	if(self->client->pers.teamSelection == TEAM_HUMANS)
		sq = &level.humanSpawnQueue;

	else if(self->client->pers.teamSelection == TEAM_ALIENS)
		sq = &level.alienSpawnQueue;
	else
		return qfalse;

	i = sq->front;

	if(G_GetSpawnQueueLength(sq)) {
		do {
			if( !(g_entities[sq->clients[ i ]].r.svFlags & SVF_BOT)) {
				if(i < sq->front)
					lastPlayerPos = i + MAX_CLIENTS - sq->front;
				else
					lastPlayerPos = i - sq->front;
			}

			if(sq->clients[ i ] == self->s.number) {
				if(i < sq->front)
					botPos = i + MAX_CLIENTS - sq->front;
				else
					botPos = i - sq->front;
			}

			i = QUEUE_PLUS1(i);
		} while(i != QUEUE_PLUS1(sq->back));
	}

	if(botPos < lastPlayerPos)
		return qtrue;
	else
		return qfalse;
}

/*
========================
Boolean Functions for determining actions
========================
*/
qboolean BotWillBuildSomething(gentity_t *self) {
	vec3_t origin;
	vec3_t normal;
	buildable_t buildable;
	return BotGetBuildingToBuild(self, origin, normal, &buildable);
}
qboolean BotShouldBuild(gentity_t *self) {
	if(BotGetTeam(self) != TEAM_HUMANS)
		return qfalse;
	if((!g_bot_repair.integer || !self->botMind->closestDamagedBuilding.ent)
		&& (!g_bot_build.integer || !BotWillBuildSomething(self)))
		return qfalse;
	if(self->botMind->modus == BOT_MODUS_BUILD)
		return qtrue;

	if(BotTeamateIsInModus(self, BOT_MODUS_BUILD)) {
		return qfalse;
	} else {
		return qtrue;
	}
}

qboolean BotShouldRushEnemyBase(gentity_t *self) {
	float totalBaseRushScore = 0;
	//FIXME: Uncomment when groups are (re)implemented
	/*int botNumbers[MAX_CLIENTS];
	int numBotsOnTeam = FindBots(botNumbers,MAX_CLIENTS,(team_t)BotGetTeam(self));
	for(int i=0;i<numBotsOnTeam;i++) {
	gentity_t *bot = &g_entities[botNumbers[i]];

	//only care about our followers and ourselves
	if((bot->botMind->trait == TRAIT_FOLLOWER && bot->botMind->leader == self) || bot == self) {
	totalBaseRushScore += BotGetBaseRushScore(bot);
	}
	}*/
	totalBaseRushScore += BotGetBaseRushScore(self);
	//if(((float)totalBaseRushScore / self->botMind->numGroup) > 0.5)
	if(totalBaseRushScore > 0.5)
		return qtrue;
	else
		return qfalse;
}
//This function is unused now with the redesign...
qboolean BotShouldRetreat(gentity_t *self) {
	int totalHealth = 0, maxHealth = 0;
	//int totalAmmo = 1, maxAmmo = 1;
	//FIXME: Uncomment when groups are (re)implemented
	/*int botNumbers[MAX_CLIENTS];
	int numBotsOnTeam = FindBots(botNumbers,MAX_CLIENTS,(team_t)BotGetTeam(self));
	for(int i=0;i<numBotsOnTeam;i++) {
	gentity_t *bot = &g_entities[botNumbers[i]];

	//only care about our followers and ourselves
	if((bot->botMind->trait == TRAIT_FOLLOWER && bot->botMind->leader == self) || bot == self) {
	totalHealth += bot->health;
	maxHealth += BG_Class((class_t)bot->client->ps.stats[STAT_CLASS])->health;
	}
	}*/
	totalHealth += self->health;
	maxHealth += BG_Class((class_t)self->client->ps.stats[STAT_CLASS])->health;
	//retreat if below health threshold
	if(((float)totalHealth / maxHealth) < 0.40f)
		return qtrue;

	//also check ammo counts for humans
	/*if(BotGetTeam(self) == TEAM_HUMANS && g_bot_buy.integer && BotFindBuilding(self, BA_H_ARMOURY) != ENTITYNUM_NONE) {
	for(int i=0;i<numBotsOnTeam;i++) {
	gentity_t *bot = &g_entities[botNumbers[i]];

	//only care about our followers and ourselves
	if((bot->botMind->trait == TRAIT_FOLLOWER && bot->botMind->leader == self) || bot == self) {
	const weaponAttributes_t *weapon = BG_Weapon((weapon_t)bot->s.weapon);
	//skip over infinite ammo weapons
	if(weapon->infiniteAmmo)
	continue;
	totalAmmo += bot->client->ps.Ammo + weapon->maxAmmo * bot->client->ps.clips;
	maxAmmo += weapon->maxAmmo + weapon->maxAmmo * weapon->maxClips;
	}
	}
	if(((float)totalAmmo / maxAmmo) < 0.40f)
	return qtrue;
	}*/
	return qfalse;
}
//FIXME: Uncomment when groups are (re)implemeted
/*
qboolean GroupIsReady(gentity_t *self) {
int botNumbers[MAX_CLIENTS];
int numBotsOnTeam = FindBots(botNumbers,MAX_CLIENTS,(team_t)BotGetTeam(self));
if(self->botMind->numGroup == 1)
return qtrue;
for(int i=0;i<numBotsOnTeam;i++) {

gentity_t *bot = &g_entities[botNumbers[i]];
//wait for the group if a bot in the bot's group is close enough, not attacking, not on spectators, and not in followleadermode
if(bot->botMind->trait == TRAIT_FOLLOWER && bot->botMind->leader == self && bot->botMind->currentModus != ATTACK
&& BotGetTeam(bot) != TEAM_NONE && DistanceSquared(self->s.origin, bot->s.origin) < Square(BOT_LEADER_WAIT_RANGE * self->botMind->numGroup) &&
DistanceSquared(self->s.origin, bot->s.origin) > Square((BOT_FOLLOW_RANGE + BOT_FOLLOW_RANGE_NEGLIGENCE) * self->botMind->numGroup))
return qfalse;
}
return qtrue;
}*/

/*botTarget_t BotGetRoamTarget(gentity_t *self) {
vec3_t point;
botTarget_t target;
if(!BotFindRandomPoint(self, point)) {
target.inuse = qfalse;
return target;
}
BotSetTarget(&target,NULL,&point);
return target;*/
//This function is expensive since we check the route first!
//Pls use with caution
//returns true if successful, false if unsuccessful
botTarget_t BotGetRoamTarget(gentity_t *self) {
	botTarget_t target;
	int numTiles = 0;
	const dtNavMesh *navMesh = self->botMind->navQuery->getAttachedNavMesh();
	numTiles = navMesh->getMaxTiles();
	const dtMeshTile *tile;
	vec3_t targetPos;

	//pick a random tile
	do {
		tile = navMesh->getTile(rand() % numTiles);
	} while(!tile->header->vertCount);

	//pick a random vertex in the tile
	int vertStart = 3 * (rand() % tile->header->vertCount);

	//convert from recast to quake3
	float *v = &tile->verts[vertStart];
	VectorCopy(v, targetPos);
	recast2quake(targetPos);

	BotSetTarget(&target, NULL, &targetPos);
	return target;
}

/*
========================
Misc Bot Stuff
========================
*/
void BotFireWeapon(weaponMode_t mode, usercmd_t *botCmdBuffer) {
	if(mode == WPM_PRIMARY) {
		usercmdPressButton(botCmdBuffer->buttons, BUTTON_ATTACK);
	} else if(mode == WPM_SECONDARY) {
		usercmdPressButton(botCmdBuffer->buttons, BUTTON_ATTACK2);
	} else if(mode == WPM_TERTIARY) {
		usercmdPressButton(botCmdBuffer->buttons, BUTTON_USE_HOLDABLE);
	}
}
void BotClassMovement(gentity_t *self, qboolean inAttackRange, usercmd_t *botCmdBuffer) {
	switch(self->client->ps.stats[STAT_CLASS]) {
	case PCL_ALIEN_LEVEL0:
		BotDodge(self, botCmdBuffer);
		break;
	case PCL_ALIEN_LEVEL1:
	case PCL_ALIEN_LEVEL1_UPG:
		if(BotTargetIsPlayer(self->botMind->goal) && (self->botMind->goal.ent->client->ps.stats[STAT_STATE] & SS_GRABBED) && inAttackRange) {
			if(self->botMind->botSkill.level == 10) {
				botCmdBuffer->forwardmove = 0;
				botCmdBuffer->upmove = 0;
				BotDodge(self, botCmdBuffer); //only move if skill == 10 because otherwise we wont aim fast enough to not lose grab
			} else {
				botCmdBuffer->forwardmove = 0;
				botCmdBuffer->rightmove = 0;
				botCmdBuffer->upmove = 0;
			}
		} else {
			BotDodge(self, botCmdBuffer);
		}
		break;
	case PCL_ALIEN_LEVEL2:
	case PCL_ALIEN_LEVEL2_UPG:
		if(self->botMind->numCorners == 1) {
			if(self->client->time1000 % 300 == 0)
				botCmdBuffer->upmove = 127;
			BotDodge(self, botCmdBuffer);
		}
		break;
	case PCL_ALIEN_LEVEL3:
		break;
	case PCL_ALIEN_LEVEL3_UPG:
		if(BotGetTargetType(self->botMind->goal) == ET_BUILDABLE && self->client->ps.Ammo > 0
			&& inAttackRange) {
				//dont move when sniping buildings
				botCmdBuffer->forwardmove = 0;
				botCmdBuffer->rightmove = 0;
				botCmdBuffer->upmove = 0;
		}
		break;
	case PCL_ALIEN_LEVEL4:
		//use rush to approach faster
		if(!inAttackRange)
			BotFireWeapon(WPM_SECONDARY, botCmdBuffer);
		break;
	default: break;
	}
}
void BotFireWeaponAI(gentity_t *self, usercmd_t *botCmdBuffer) {
	float distance;
	vec3_t targetPos;
	vec3_t forward,right,up;
	vec3_t muzzle;
	trace_t trace;
	AngleVectors(self->client->ps.viewangles,forward,right,up);
	CalcMuzzlePoint(self,forward,right,up,muzzle);
	BotGetIdealAimLocation(self,self->botMind->goal,targetPos);

	trap_Trace(&trace,muzzle,NULL,NULL,targetPos,-1,MASK_SHOT);
	distance = Distance(muzzle, trace.endpos);
	switch(self->s.weapon) {
	case WP_ABUILD:
		if(distance <= ABUILDER_CLAW_RANGE)
			BotFireWeapon(WPM_SECONDARY, botCmdBuffer);
		else
			usercmdPressButton(botCmdBuffer->buttons, BUTTON_GESTURE); //make cute granger sounds to ward off the would be attackers
		break;
	case WP_ABUILD2:
		if(distance <= ABUILDER_CLAW_RANGE)
			BotFireWeapon(WPM_SECONDARY, botCmdBuffer); //swipe
		else
			BotFireWeapon(WPM_TERTIARY, botCmdBuffer); //blob launcher
		break;
	case WP_ALEVEL0:
		break; //auto hit
	case WP_ALEVEL1:
		BotFireWeapon(WPM_PRIMARY, botCmdBuffer); //basi swipe
		break;
	case WP_ALEVEL1_UPG:
		if(distance <= LEVEL1_CLAW_U_RANGE)
			BotFireWeapon(WPM_PRIMARY, botCmdBuffer); //basi swipe
		/*
		else
		BotFireWeapn(WPM_SECONDARY,botCmdBuffer); //basi poisen
		*/
		break;
	case WP_ALEVEL2:
		BotFireWeapon(WPM_PRIMARY, botCmdBuffer); //mara swipe
		break;
	case WP_ALEVEL2_UPG:
		if(distance <= LEVEL2_CLAW_U_RANGE)
			BotFireWeapon(WPM_PRIMARY,botCmdBuffer); //mara swipe
		else
			BotFireWeapon(WPM_SECONDARY, botCmdBuffer); //mara lightning
		break;
	case WP_ALEVEL3:
		if(distance > LEVEL3_CLAW_RANGE && self->client->ps.stats[ STAT_MISC ] < LEVEL3_POUNCE_TIME) {
			botCmdBuffer->angles[PITCH] = ANGLE2SHORT(-CalcPounceAimPitch(self,self->botMind->goal)); //compute and apply correct aim pitch to hit target
			BotFireWeapon(WPM_SECONDARY, botCmdBuffer); //goon pounce
		} else
			BotFireWeapon(WPM_PRIMARY, botCmdBuffer); //goon chomp
		break;
	case WP_ALEVEL3_UPG:
		if(self->client->ps.Ammo > 0 && distance > LEVEL3_CLAW_UPG_RANGE) {
			botCmdBuffer->angles[PITCH] = ANGLE2SHORT(-CalcBarbAimPitch(self, self->botMind->goal)); //compute and apply correct aim pitch to hit target
			BotFireWeapon(WPM_TERTIARY, botCmdBuffer); //goon barb
		} else if(distance > LEVEL3_CLAW_UPG_RANGE && self->client->ps.stats[ STAT_MISC ] < LEVEL3_POUNCE_TIME_UPG) {
			botCmdBuffer->angles[PITCH] = ANGLE2SHORT(-CalcPounceAimPitch(self, self->botMind->goal)); //compute and apply correct aim pitch to hit target
			BotFireWeapon(WPM_SECONDARY,botCmdBuffer); //goon pounce
		} else
			BotFireWeapon(WPM_PRIMARY, botCmdBuffer); //goon chomp
		break;
	case WP_ALEVEL4:
		if (distance > LEVEL4_CLAW_RANGE && self->client->ps.stats[STAT_MISC] < LEVEL4_TRAMPLE_CHARGE_MAX)
			BotFireWeapon(WPM_SECONDARY,botCmdBuffer); //rant charge
		else
			BotFireWeapon(WPM_PRIMARY,botCmdBuffer); //rant swipe
		break;
	case WP_LUCIFER_CANNON:
		if( self->client->ps.stats[STAT_MISC] < LCANNON_CHARGE_TIME_MAX * Com_Clamp(0.5,1,random()))
			BotFireWeapon(WPM_PRIMARY, botCmdBuffer);
		break;
	default: BotFireWeapon(WPM_PRIMARY,botCmdBuffer);
	}
}

extern "C" void G_BotLoadBuildLayout() {
	fileHandle_t f;
	int len;
	char *layout, *layoutHead;
	char map[ MAX_QPATH ];
	char buildName[ MAX_TOKEN_CHARS ];
	buildable_t buildable;
	vec3_t origin = { 0.0f, 0.0f, 0.0f };
	vec3_t angles = { 0.0f, 0.0f, 0.0f };
	vec3_t origin2 = { 0.0f, 0.0f, 0.0f };
	vec3_t angles2 = { 0.0f, 0.0f, 0.0f };
	char line[ MAX_STRING_CHARS ];
	int i = 0;
	level.botBuildLayout.numBuildings = 0;
	if( !g_bot_buildLayout.string[0] || !Q_stricmp(g_bot_buildLayout.string, "*BUILTIN*" ) )
		return;

	trap_Cvar_VariableStringBuffer( "mapname", map, sizeof( map ) );
	trap_Print("Attempting to Load Bot Build Layout File...\n");
	len = trap_FS_FOpenFile( va( "layouts/%s/%s.dat", map, g_bot_buildLayout.string ),
		&f, FS_READ );
	if( len < 0 )
	{
		G_Printf( "ERROR: layout %s could not be opened\n", g_bot_buildLayout.string );
		return;
	}
	layoutHead = layout = (char *)BG_Alloc( len + 1 );
	trap_FS_Read( layout, len, f );
	layout[ len ] = '\0';
	trap_FS_FCloseFile( f );
	while( *layout )
	{
		if( i >= sizeof( line ) - 1 )
		{
			G_Printf( S_COLOR_RED "ERROR: line overflow in %s before \"%s\"\n",
				va( "layouts/%s/%s.dat", map, g_bot_buildLayout.string ), line );
			break;
		}
		line[ i++ ] = *layout;
		line[ i ] = '\0';
		if( *layout == '\n' )
		{
			i = 0;
			sscanf( line, "%s %f %f %f %f %f %f %f %f %f %f %f %f\n",
				buildName,
				&origin[ 0 ], &origin[ 1 ], &origin[ 2 ],
				&angles[ 0 ], &angles[ 1 ], &angles[ 2 ],
				&origin2[ 0 ], &origin2[ 1 ], &origin2[ 2 ],
				&angles2[ 0 ], &angles2[ 1 ], &angles2[ 2 ] );

			buildable = BG_BuildableByName( buildName )->number;
			if( buildable <= BA_NONE || buildable >= BA_NUM_BUILDABLES )
				G_Printf( S_COLOR_YELLOW "WARNING: bad buildable name (%s) in layout."
				" skipping\n", buildName );
			else if(level.botBuildLayout.numBuildings == MAX_BOT_BUILDINGS) {
				G_Printf( S_COLOR_YELLOW "WARNING: reached max buildings for bot layout (%d)."
					" skipping\n",MAX_BOT_BUILDINGS );
			} else if(BG_Buildable(buildable)->team == TEAM_HUMANS && level.botBuildLayout.numBuildings < MAX_BOT_BUILDINGS) {
				level.botBuildLayout.buildings[level.botBuildLayout.numBuildings].type = buildable;
				VectorCopy(origin, level.botBuildLayout.buildings[level.botBuildLayout.numBuildings].origin);
				VectorCopy(origin2, level.botBuildLayout.buildings[level.botBuildLayout.numBuildings].normal);
				level.botBuildLayout.numBuildings++;
			}

		}
		layout++;
	}
	BG_Free( layoutHead );
}

void BotSetSkillLevel(gentity_t *self, int skill) {
	self->botMind->botSkill.level = skill;
	//different aim for different teams
	if(self->botMind->botTeam == TEAM_HUMANS) {
		self->botMind->botSkill.aimSlowness = (float) skill / 10;
		self->botMind->botSkill.aimShake = (int) (10 - skill);
	} else {
		self->botMind->botSkill.aimSlowness = (float) skill / 10;
		self->botMind->botSkill.aimShake = (int) (10 - skill);
	}
}

extern "C" void G_BotAssignGroups(void) {
	int humanBots[MAX_CLIENTS];
	int alienBots[MAX_CLIENTS];
	int numHumanBots = FindBots(humanBots,MAX_CLIENTS,TEAM_HUMANS);
	int numAlienBots = FindBots(alienBots,MAX_CLIENTS,TEAM_ALIENS);
	int maxHumanLeaders = 0;
	int maxAlienLeaders = 0;
	int numAlienLeaders = 0;
	int numHumanLeaders = 0;
	int maxInGroup = 1;

	if(g_bot_numInGroup.integer > 0)
		maxInGroup = g_bot_numInGroup.integer;

	maxHumanLeaders = (int)ceilf((float)numHumanBots / maxInGroup);
	maxAlienLeaders = (int)ceilf((float)numAlienBots / maxInGroup);

	//compute which bots are leaders
	for(int i=0;i<level.numConnectedClients;i++) {
		gentity_t *testEntity = &g_entities[level.sortedClients[i]];
		if(!(testEntity->r.svFlags & SVF_BOT))
			continue;
		if(BotGetTeam(testEntity) == TEAM_HUMANS) {
			if(numHumanLeaders < maxHumanLeaders) {
				testEntity->botMind->trait = TRAIT_LEADER;
				testEntity->botMind->numGroup = 1;
				numHumanLeaders++;
			} else {
				testEntity->botMind->trait = TRAIT_FOLLOWER;
			}
		} else if(BotGetTeam(testEntity) == TEAM_ALIENS) {
			if(numAlienLeaders < maxAlienLeaders) {
				testEntity->botMind->trait = TRAIT_LEADER;
				testEntity->botMind->numGroup = 1;
				numAlienLeaders++;
			} else {
				testEntity->botMind->trait = TRAIT_FOLLOWER;
			}
		}
	}
	//compute which human bots are followers
	for(int i=0;i<numHumanBots;i++) {
		gentity_t *testEntity = &g_entities[humanBots[i]];
		//ignore leaders
		if(testEntity->botMind->trait == TRAIT_LEADER)
			continue;
		testEntity->botMind->trait = TRAIT_LEADER; //if there is no leader in range, become a leader
		for(int n=0;n<numHumanBots;n++) {
			gentity_t *testLeader = &g_entities[humanBots[n]];
			//leader should not be spectating
			if( testLeader->client->sess.spectatorState != SPECTATOR_NOT)
				continue;
			if(testLeader->botMind->trait == TRAIT_LEADER) {
				if(testLeader->botMind->numGroup < g_bot_numInGroup.integer){// && DistanceSquared(testLeader->s.origin,testEntity->s.origin) < Square((BOT_FOLLOW_RANGE + BOT_FOLLOW_RANGE_NEGLIGENCE) * (testLeader->botMind->numGroup + 1))) {
					testLeader->botMind->numGroup++;
					testEntity->botMind->trait = TRAIT_FOLLOWER;
					testEntity->botMind->leader = testLeader;
					break;
				}
			}
		}
	}
	//compute which alien bots are followers
	for(int i=0;i<numAlienBots;i++) {
		gentity_t *testEntity = &g_entities[alienBots[i]];
		//ignore leaders
		if(testEntity->botMind->trait == TRAIT_LEADER)
			continue;
		testEntity->botMind->trait = TRAIT_LEADER; //if there is no leader in range, become a leader
		for(int n=0;n<numAlienBots;n++) {
			gentity_t *testLeader = &g_entities[alienBots[n]];
			//leader should not be spectating
			if( testLeader->client->sess.spectatorState != SPECTATOR_NOT)
				continue;
			if(testLeader->botMind->trait == TRAIT_LEADER) {
				if(testLeader->botMind->numGroup < g_bot_numInGroup.integer){ //&& DistanceSquared(testLeader->s.origin,testEntity->s.origin) < Square((BOT_FOLLOW_RANGE + BOT_FOLLOW_RANGE_NEGLIGENCE) * (testLeader->botMind->numGroup + 1))) {
					testLeader->botMind->numGroup++;
					testEntity->botMind->trait = TRAIT_FOLLOWER;
					testEntity->botMind->leader = testLeader;
					break;
				}
			}
		}
	}
}


/*
=======================
Bot Modus
=======================
*/
void EvaluateTasks(gentity_t *self, usercmd_t *botCmdBuffer) {
	if(self->botMind->task == BOT_TASK_NONE) {
		//our task stack is empty, lets start it
		if(!BotBuildModus(self,botCmdBuffer))
			BotAttackModus(self,botCmdBuffer);
	} else {
		//execute the task we are in
		botTask_t task = self->botMind->task;
		botTaskStatus_t taskStatus;
		switch(task) {
			case BOT_TASK_FIGHT:
				taskStatus = BotTaskFight(self, botCmdBuffer);
				break;
			case BOT_TASK_BUILD:
				taskStatus = BotTaskBuild(self, botCmdBuffer);
				break;
			case BOT_TASK_ROAM:
				taskStatus = BotTaskRoam(self, botCmdBuffer);
				break;
			case BOT_TASK_BUY:
				taskStatus = BotTaskBuy(self, botCmdBuffer);
				break;
			case BOT_TASK_HEAL:
				taskStatus = BotTaskHeal(self, botCmdBuffer);
				break;
			case BOT_TASK_REPAIR:
				taskStatus = BotTaskRepair(self, botCmdBuffer);
				break;
			case BOT_TASK_RUSH:
				taskStatus = BotTaskRush(self, botCmdBuffer);
				break;
			case BOT_TASK_RETREAT:
				taskStatus = BotTaskRetreat(self, botCmdBuffer);
				break;
			default:
				taskStatus = TASK_STOPPED;
		}
		if(taskStatus == TASK_STOPPED) {
			self->botMind->task = BOT_TASK_NONE; //the task has ended
			self->botMind->needNewGoal = qtrue;
		}
	}
}
botModusStatus_t BotAttackModus(gentity_t *self, usercmd_t *botCmdBuffer) {
	self->botMind->modus = BOT_MODUS_ATTACK;
	if(BotTaskFight(self, botCmdBuffer)) {
		BotChangeTask(self, BOT_TASK_FIGHT);
		return MODUS_RUNNING;
	} else if(BotTaskHeal(self, botCmdBuffer)) {
		BotChangeTask(self, BOT_TASK_HEAL);
		return MODUS_RUNNING;
	} else if(BotTaskBuy(self, botCmdBuffer)) {
		BotChangeTask(self, BOT_TASK_BUY);
		return MODUS_RUNNING;
	} else if(BotTaskEvolve(self, botCmdBuffer)) {
		BotChangeTask(self, BOT_TASK_EVOLVE);
		return MODUS_RUNNING;
	} else if(BotShouldRushEnemyBase(self) && BotTaskRush(self, botCmdBuffer)) {
		BotChangeTask(self, BOT_TASK_RUSH);
		return MODUS_RUNNING;
	} else if(BotTaskRoam(self, botCmdBuffer)) {
		BotChangeTask(self, BOT_TASK_ROAM);
		return MODUS_RUNNING;
	} else {
		return MODUS_STOPPED;
	}
}

botModusStatus_t BotDefendModus(gentity_t *self, usercmd_t *botCmdBuffer) {
	return MODUS_STOPPED;
}

botModusStatus_t BotBuildModus(gentity_t *self, usercmd_t *botCmdBuffer) {
	if(!BotShouldBuild(self))
		return MODUS_STOPPED;
	self->botMind->modus = BOT_MODUS_BUILD;

	if(self->botMind->bestEnemy.ent || level.time - self->botMind->enemyLastSeen <= BOT_RETREAT_TIME) {
		if(!BG_InventoryContainsWeapon(WP_HBUILD,self->client->ps.stats)) {
			if(BotTaskFight(self, botCmdBuffer))
				BotChangeTask(self, BOT_TASK_FIGHT);
			else
				goto rest;
		} else {
			rest:
			if(BotTaskRetreat(self, botCmdBuffer)) {
				BotChangeTask(self, BOT_TASK_RETREAT);
			} else {
				BotChangeTask(self, BOT_TASK_FIGHT);
			}
		}
		return MODUS_RUNNING;
	} else if(BotTaskBuy(self, botCmdBuffer)) {
		BotChangeTask(self, BOT_TASK_BUY);
		return MODUS_RUNNING;
	} else if(BotTaskHeal(self, botCmdBuffer)) {
		BotChangeTask(self, BOT_TASK_HEAL);
		return MODUS_RUNNING;
	} else if(BotTaskBuild(self, botCmdBuffer)) {
		BotChangeTask(self, BOT_TASK_BUILD);
		return MODUS_RUNNING;
	} else if(BotTaskRepair(self, botCmdBuffer)){
		BotChangeTask(self, BOT_TASK_REPAIR);
		return MODUS_RUNNING;
	} else {
		return MODUS_STOPPED;
	}
}
/*
=======================
General Bot Tasks

For specific team tasks, see their respective files
=======================
*/
botTaskStatus_t BotTaskBuild(gentity_t *self, usercmd_t *botCmdBuffer) {
	if(!g_bot_build.integer)
		return TASK_STOPPED;
	switch(BotGetTeam(self)) {
	case TEAM_HUMANS:
		return BotTaskBuildH(self, botCmdBuffer);
		break;
	case TEAM_ALIENS:
		return BotTaskBuildA(self, botCmdBuffer);
		break;
	default:
		return TASK_STOPPED;
		break;
	}
}
botTaskStatus_t BotTaskFight(gentity_t *self, usercmd_t *botCmdBuffer) {
	//don't enter this task until we are given enough time to react to seeing the enemy
	if(level.time - self->botMind->timeFoundEnemy <= BOT_REACTION_TIME) {
		if(self->botMind->goal.inuse) {
			BotMoveToGoal(self, botCmdBuffer); //continue moving to our previous goal
		}
		return TASK_RUNNING;
	}

	if(BotRoutePermission(self,BOT_TASK_FIGHT)) {
		if(!self->botMind->bestEnemy.ent)
			return TASK_STOPPED;
		BotChangeTarget(self, self->botMind->bestEnemy.ent, NULL);
	}

	//safety check
	if(!BotTargetIsEntity(self->botMind->goal)) {
		return TASK_STOPPED; //we dont want to end this task yet incase there is another enemy
	}

	//enemy has switched teams so signal that the goal is unusable
	if(BotGetTeam(self->botMind->goal) == BotGetTeam(self) || BotGetTeam(self->botMind->goal) == TEAM_NONE) {
		return TASK_STOPPED;
	}

	//enemy has died so signal that the goal is unusable
	if(self->botMind->goal.ent->health <= 0 ||
		(BotTargetIsPlayer(self->botMind->goal) && self->botMind->goal.ent->client->ps.pm_type == PM_DEAD))
	{
		return TASK_STOPPED;
	}

	//switch to blaster if we need to
	if((WeaponIsEmpty((weapon_t)self->client->ps.weapon, self->client->ps)
		|| self->client->ps.weapon == WP_HBUILD) && BotGetTeam(self) == TEAM_HUMANS)
		G_ForceWeaponChange( self, WP_BLASTER );

	//aliens have radar so they will always 'see' the enemy if they are in radar range
	if(BotGetTeam(self) == TEAM_ALIENS && DistanceToGoalSquared(self) <= Square(ALIENSENSE_RANGE)) {
		self->botMind->enemyLastSeen = level.time;
	}

	//we can't see our target
	if(!BotTargetIsVisible(self, self->botMind->goal, MASK_SHOT)) {
		botTarget_t proposedTarget;
		BotSetTarget(&proposedTarget, self->botMind->bestEnemy.ent, NULL);
		//we can see another enemy (not our target)
		if(self->botMind->bestEnemy.ent && BotPathIsWalkable(self,proposedTarget)) {
			//change targets
			return TASK_STOPPED;
		} else if(level.time - self->botMind->enemyLastSeen >= BOT_ENEMY_CHASETIME) {
			//we have chased our current (invisible) target for a while, but have not caught sight of him again, so end this task
			return TASK_STOPPED;
		} else {
			//move to the enemy
			BotMoveToGoal(self, botCmdBuffer);
		}
	} else { //we see our target!!
		qboolean inAttackRange;
		//record the time we saw him
		self->botMind->enemyLastSeen = level.time;

		//only enter attacking AI if we can walk in a straight line to the enemy or we can hit him from our current position
		if((inAttackRange = BotTargetInAttackRange(self, self->botMind->goal)) || self->botMind->numCorners == 1) {
			vec3_t tmpVec;
			//update the path corridor
			UpdatePathCorridor(self);
			//aim at the enemy
			BotGetIdealAimLocation(self, self->botMind->goal, tmpVec);
			BotSlowAim(self, tmpVec, self->botMind->botSkill.aimSlowness);
			if(BotGetTargetType(self->botMind->goal) != ET_BUILDABLE) {
				BotShakeAim(self, tmpVec);
			}
			BotAimAtLocation(self, tmpVec, botCmdBuffer);

			botCmdBuffer->forwardmove = 127; //move forward

			//fire our weapon if we are in range or have pain saw
			//TODO: Make this better with BotAimNegligence?
			if(inAttackRange || self->client->ps.weapon == WP_PAIN_SAW)
				BotFireWeaponAI(self, botCmdBuffer);

			//human specific attacking AI
			if(BotGetTeam(self) == TEAM_HUMANS) {
				//dont back away unless skill > 3 + distance logic
				if(self->botMind->botSkill.level >= 3 && DistanceToGoalSquared(self) < Square(MAX_HUMAN_DANCE_DIST)
					&& (DistanceToGoalSquared(self) > Square(MIN_HUMAN_DANCE_DIST) || self->botMind->botSkill.level < 5)
					&& self->client->ps.weapon != WP_PAIN_SAW)
				{
					botCmdBuffer->forwardmove = -127; //max backward speed
				} else if(DistanceToGoalSquared(self) <= Square(MIN_HUMAN_DANCE_DIST)) { //we wont hit this if skill < 5
					//we will be moving toward enemy, strafe too
					//the result: we go around the enemy
					botCmdBuffer->rightmove = BotGetStrafeDirection();

					//also try to dodge if high enough level
					//all the conditions are there so we dont stop moving forward when we cant dodge anyway
					if(self->client->ps.weapon != WP_PAIN_SAW && self->botMind->botSkill.level >= 7
						&& self->client->ps.stats[STAT_STAMINA] > STAMINA_SLOW_LEVEL + STAMINA_DODGE_TAKE
						&& !(self->client->ps.pm_flags & ( PMF_TIME_LAND | PMF_CHARGE ))
						&& self->client->ps.groundEntityNum != ENTITYNUM_NONE)
					{
						usercmdPressButton(botCmdBuffer->buttons, BUTTON_DODGE);
						botCmdBuffer->forwardmove = 0;
					}
				} else {
					//we should be >= MAX_HUMAN_DANCE_DIST from the enemy
					//dont go forward
					botCmdBuffer->forwardmove = 0;

					//strafe randomly
					botCmdBuffer->rightmove = BotGetStrafeDirection();
				}

				//humans should not move if they are targeting, and can hit, a building
				//FIXME: Maybe add some "move to best attack point" logic here? Bots will be damaged a lot by buildings otherwise...
				if(BotGetTargetType(self->botMind->goal) == ET_BUILDABLE) {
					botCmdBuffer->forwardmove = 0;
					botCmdBuffer->rightmove= 0;
					botCmdBuffer->upmove = 0;
				}


				if(self->client->ps.stats[STAT_STAMINA] > STAMINA_SLOW_LEVEL && self->botMind->botSkill.level >= 5) {
					usercmdPressButton(botCmdBuffer->buttons, BUTTON_SPRINT);
				} else {
					//dont sprint or dodge, we are about to slow
					usercmdReleaseButton(botCmdBuffer->buttons, BUTTON_SPRINT);
					usercmdReleaseButton(botCmdBuffer->buttons, BUTTON_DODGE);
				}

			} else if(BotGetTeam(self) == TEAM_ALIENS) {
				//alien specific attacking AI

				BotClassMovement(self,inAttackRange, botCmdBuffer); //FIXME: aliens are so much dumber :'(

				// enable wallwalk for classes that can wallwalk
				if( BG_ClassHasAbility( (class_t) self->client->ps.stats[STAT_CLASS], SCA_WALLCLIMBER ) )
					botCmdBuffer->upmove = -1;
			}
		} else {
			//move to the enemy since we cant get to him directly, nor can we attack him from here
			BotMoveToGoal(self, botCmdBuffer);
		}
	}
	return TASK_RUNNING;
}

botTaskStatus_t BotTaskGroup(gentity_t *self, usercmd_t *botCmdBuffer) {
	return TASK_STOPPED;
}

botTaskStatus_t BotTaskHeal(gentity_t *self, usercmd_t *botCmdBuffer) {
	if(BotGetTeam(self) == TEAM_HUMANS)
		return BotTaskHealH(self, botCmdBuffer);
	else
		return BotTaskHealA(self, botCmdBuffer);
}
botTaskStatus_t BotTaskRetreat(gentity_t *self, usercmd_t *botCmdBuffer) {
	if(!g_bot_retreat.integer)
		return TASK_STOPPED;

	if(BotRoutePermission(self, BOT_TASK_RETREAT)) {
		if(!BotChangeTarget(self, BotGetRetreatTarget(self)))
			return TASK_STOPPED;
	}
	if(!BotTargetIsEntity(self->botMind->goal)) {
		return TASK_STOPPED;
	}
	if(DistanceToGoalSquared(self) > Square(70))
		BotMoveToGoal(self, botCmdBuffer);
	else
		return TASK_STOPPED;
	return TASK_RUNNING;
}

botTaskStatus_t BotTaskRush(gentity_t *self, usercmd_t *botCmdBuffer) {
	if(!g_bot_rush.integer)
		return TASK_STOPPED;

	//dont start rushing immediately afer spawning to help avoid congestion when bots all take the same path
	if(self->client->pers.aliveSeconds < 5)
		return TASK_STOPPED;

	//engage any enemies we find
	if(self->botMind->bestEnemy.ent) {
		return TASK_STOPPED;
	}

	if(BotRoutePermission(self,BOT_TASK_RUSH)) {
		if(!BotChangeTarget(self, BotGetRushTarget(self)))
			return TASK_STOPPED;
	}
	if(!BotTargetIsEntity(self->botMind->goal)) {
		return TASK_STOPPED;
	}
	if(self->botMind->goal.ent->health <= 0) {
		return TASK_STOPPED;
	}
	if(DistanceToGoalSquared(self) > Square(70)) {
		BotMoveToGoal(self, botCmdBuffer);
	} else {
		return TASK_STOPPED;
	}
	return TASK_RUNNING;
}

botTaskStatus_t BotTaskRoam(gentity_t *self, usercmd_t *botCmdBuffer) {
	//engage any enemies we find
	if(self->botMind->bestEnemy.ent) {
		return TASK_STOPPED;
	}

	if(BotRoutePermission(self,BOT_TASK_ROAM)) {
		if( !BotChangeTarget(self, BotGetRoamTarget(self) ) )
			return TASK_STOPPED;
	}

	if(DistanceToGoalSquared(self) > Square(70)) {
		BotMoveToGoal(self, botCmdBuffer);
	} else {
		return TASK_STOPPED;
	}
	return TASK_RUNNING;
}
/*
=======================
Bot management functions
=======================
*/
qboolean G_BotAdd( char *name, team_t team, int skill ) {
	int clientNum;
	char userinfo[MAX_INFO_STRING];
	const char* s = 0;
	gentity_t *bot;

	if(!navMeshLoaded) {
		trap_Print("No Navigation Mesh file is available for this map\n");
		return qfalse;
	}

	// find what clientNum to use for bot
	clientNum = trap_BotAllocateClient(0);

	if(clientNum < 0) {
		trap_Print("no more slots for bot\n");
		return qfalse;
	}
	bot = &g_entities[ clientNum ];
	bot->r.svFlags |= SVF_BOT;
	bot->inuse = qtrue;


	//default bot data
	bot->botMind = &g_botMind[clientNum];
	bot->botMind->enemyLastSeen = 0;
	bot->botMind->botTeam = team;
	bot->botMind->command = BOT_AUTO;
	bot->botMind->spawnItem = WP_HBUILD;
	bot->botMind->needNewGoal = qtrue;
	bot->botMind->modus = BOT_MODUS_IDLE;
	bot->botMind->task = BOT_TASK_NONE;
	bot->botMind->pathCorridor = &pathCorridor[clientNum];
	BotSetSkillLevel(bot, skill);

	// register user information
	userinfo[0] = '\0';
	Info_SetValueForKey( userinfo, "name", name );
	Info_SetValueForKey( userinfo, "rate", "25000" );
	Info_SetValueForKey( userinfo, "snaps", "20" );

	//so we can connect if server is password protected
	if(g_needpass.integer == 1)
		Info_SetValueForKey( userinfo, "password", g_password.string);

	trap_SetUserinfo( clientNum, userinfo );

	// have it connect to the game as a normal client
	if((s = ClientConnect(clientNum, qtrue)))
	{
		// won't let us join
		trap_Print(s);
		return qfalse;
	}

	ClientBegin( clientNum );
	G_ChangeTeam( bot, team );
	return qtrue;
}

void G_BotDel( int clientNum ) {
	gentity_t *bot = &g_entities[clientNum];
	if( !( bot->r.svFlags & SVF_BOT ) || !bot->botMind) {
		trap_Print( va("'^7%s^7' is not a bot\n", bot->client->pers.netname) );
		return;
	}
	trap_DropClient(clientNum, "disconnected");
}

void G_BotCmd( gentity_t *master, int clientNum, char *command) {
	gentity_t *bot = &g_entities[clientNum];
	if( !( bot->r.svFlags & SVF_BOT ) || !bot->botMind)
		return;

	if( !Q_stricmp( command, "auto" ) )
		bot->botMind->command = BOT_AUTO;
	else if( !Q_stricmp( command, "attack" ) )
		bot->botMind->command = BOT_ATTACK;
	else if( !Q_stricmp( command, "idle" ) )
		bot->botMind->command = BOT_IDLE;
	else if( !Q_stricmp( command, "repair" ) )
		bot->botMind->command = BOT_REPAIR;
	else if( !Q_stricmp( command, "spawnrifle" ) )
		bot->botMind->spawnItem = WP_MACHINEGUN;
	else if( !Q_stricmp( command, "spawnckit" ) )
		bot->botMind->spawnItem = WP_HBUILD;
	else {
		bot->botMind->command = BOT_AUTO;
	}
	return;
}

/*
=======================
Bot Thinks
=======================
*/
extern "C" void G_BotThink( gentity_t *self) {
	char buf[MAX_STRING_CHARS];
	usercmd_t  botCmdBuffer = self->client->pers.cmd;
	vec3_t     nudge;

	//reset command buffer
	usercmdClearButtons(botCmdBuffer.buttons);

	// for nudges, e.g. spawn blocking
	nudge[0] = botCmdBuffer.doubleTap ? botCmdBuffer.forwardmove : 0;
	nudge[1] = botCmdBuffer.doubleTap ? botCmdBuffer.rightmove : 0;
	nudge[2] = botCmdBuffer.doubleTap ? botCmdBuffer.upmove : 0;

	botCmdBuffer.forwardmove = 0;
	botCmdBuffer.rightmove = 0;
	botCmdBuffer.upmove = 0;
	botCmdBuffer.doubleTap = 0;

	//acknowledge recieved server commands
	//MUST be done
	while(trap_BotGetServerCommand(self->client->ps.clientNum, buf, sizeof(buf)));

	//self->client->pers.cmd.forwardmove = 127;
	//return;
	//update closest structs
	gentity_t* bestEnemy = BotFindBestEnemy(self);
	botTarget_t target;
	BotSetTarget(&target, bestEnemy, NULL);
	//if we do not already have an enemy, and we found an enemy, update the time that we found an enemy
	if(!self->botMind->bestEnemy.ent && bestEnemy && level.time - self->botMind->enemyLastSeen > BOT_ENEMY_CHASETIME && BotAimNegligence(self,target) > 45) {
		self->botMind->timeFoundEnemy = level.time;
	}

	self->botMind->bestEnemy.ent = bestEnemy;
	self->botMind->bestEnemy.distance = (!bestEnemy) ? 0 : Distance(self->s.origin,bestEnemy->s.origin);

	BotFindClosestBuildings(self, &self->botMind->closestBuildings);

	gentity_t* bestDamaged = BotFindDamagedFriendlyStructure(self);
	self->botMind->closestDamagedBuilding.ent = bestDamaged;
	self->botMind->closestDamagedBuilding.distance = (!bestDamaged) ? 0 :  Distance(self->s.origin,bestDamaged->s.origin);

	//use medkit when hp is low
	if(self->health < BOT_USEMEDKIT_HP && BG_InventoryContainsUpgrade(UP_MEDKIT,self->client->ps.stats))
		BG_ActivateUpgrade(UP_MEDKIT,self->client->ps.stats);

	//infinite funds cvar
	if(g_bot_infinite_funds.integer)
		G_AddCreditToClient(self->client, HUMAN_MAX_CREDITS, qtrue);

	//hacky ping fix
	self->client->ps.ping = rand() % 50 + 50;

	//Execute the bot's tasks (The actual AI of the bot)
	EvaluateTasks(self, &botCmdBuffer);

	// if we were nudged...
	VectorAdd( self->client->ps.velocity, nudge, self->client->ps.velocity );

	self->client->pers.cmd = botCmdBuffer;
}

extern "C" void G_BotSpectatorThink( gentity_t *self ) {
	char buf[MAX_STRING_CHARS];
	//hacky ping fix
	self->client->ps.ping = rand() % 50 + 50;

	//acknowledge recieved console messages
	//MUST be done
	while(trap_BotGetServerCommand(self->client->ps.clientNum, buf, sizeof(buf)));

	if( self->client->ps.pm_flags & PMF_QUEUED) {
		//we're queued to spawn, all good
		//check for humans in the spawn que
		{
			spawnQueue_t *sq;
			if(self->client->pers.teamSelection == TEAM_HUMANS)
				sq = &level.humanSpawnQueue;
			else if(self->client->pers.teamSelection == TEAM_ALIENS)
				sq = &level.alienSpawnQueue;
			else
				sq = NULL;

			if( sq && PlayersBehindBotInSpawnQueue( self )) {
				G_RemoveFromSpawnQueue( sq, self->s.number );
				G_PushSpawnQueue( sq, self->s.number );
			}
		}
		if(self->client->pers.teamSelection == TEAM_HUMANS) {
			//we want to spawn with a ckit if we need to build
			if(BotShouldBuild(self)) {
				if(self->client->pers.humanItemSelection != WP_HBUILD) {
					G_RemoveFromSpawnQueue(&level.humanSpawnQueue, self->s.number);
					self->client->pers.humanItemSelection = WP_HBUILD;
					G_PushSpawnQueue( &level.humanSpawnQueue, self->s.number);
				}
			} else if(self->client->pers.humanItemSelection != WP_MACHINEGUN && g_bot_rifle.integer) {
				//we should not build, so spawn with a rifle
				G_RemoveFromSpawnQueue(&level.humanSpawnQueue, self->s.number);
				self->client->pers.humanItemSelection = WP_MACHINEGUN;
				G_PushSpawnQueue( &level.humanSpawnQueue, self->s.number);
			}
		}

		return;
	}

	//reset stuff
	self->botMind->followingRoute = qfalse;
	BotSetGoal(self, NULL, NULL);
	self->botMind->modus = BOT_MODUS_IDLE;
	self->botMind->needNewGoal = qtrue;
	self->botMind->bestEnemy.ent = NULL;
	self->botMind->timeFoundEnemy = 0;
	self->botMind->enemyLastSeen = 0;
	self->botMind->task = BOT_TASK_NONE;
	if( self->client->sess.restartTeam == TEAM_NONE ) {
		int teamnum = self->client->pers.teamSelection;
		int clientNum = self->client->ps.clientNum;

		if( teamnum == TEAM_HUMANS ) {
			self->client->pers.classSelection = PCL_HUMAN;
			self->client->ps.stats[STAT_CLASS] = PCL_HUMAN;
			self->botMind->navQuery = navQuerys[PCL_HUMAN];
			self->botMind->navFilter = &navFilters[PCL_HUMAN];
			//we want to spawn with rifle unless it is disabled or we need to build
			if(g_bot_rifle.integer && !BotShouldBuild(self))
				self->client->pers.humanItemSelection = WP_MACHINEGUN;
			else
				self->client->pers.humanItemSelection = WP_HBUILD;

			G_PushSpawnQueue( &level.humanSpawnQueue, clientNum );
		} else if( teamnum == TEAM_ALIENS) {
			self->client->pers.classSelection = PCL_ALIEN_LEVEL0;
			self->client->ps.stats[STAT_CLASS] = PCL_ALIEN_LEVEL0;
			self->botMind->navQuery = navQuerys[PCL_ALIEN_LEVEL0];
			self->botMind->navFilter = &navFilters[PCL_ALIEN_LEVEL0];
			G_PushSpawnQueue( &level.alienSpawnQueue, clientNum );
		}
	}
}

extern "C" void G_BotIntermissionThink( gclient_t *client )
{
	client->readyToExit = qtrue;
}

