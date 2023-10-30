/*
===========================================================================

Unvanquished GPL Source Code
Copyright (C) 1999-2005 Id Software, Inc.

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

#include "shared/parse.h"
#include "sg_bot_parse.h"
#include "sg_bot_util.h"
#include "CBSE.h"
#include "Entities.h"

static bool expectToken( const char *s, pc_token_list **list, bool next )
{
	const pc_token_list *current = *list;

	if ( !current )
	{
		Log::Warn( "Expected token %s but found end of file", s );
		return false;
	}

	if ( Q_stricmp( current->token.string, s ) != 0 )
	{
		Log::Warn( "Expected token %s but found %s on line %d", s, current->token.string, current->token.line );
		return false;
	}

	if ( next )
	{
		*list = current->next;
	}
	return true;
}

static AIValue_t AIBoxToken( const pc_token_stripped_t *token, bool negative = false )
{
	if ( token->type == tokenType_t::TT_STRING )
	{
		return AIBoxString( token->string );
	}

	if ( static_cast<float>( token->intvalue ) != token->floatvalue )
	{
		return AIBoxFloat( negative ? -token->floatvalue : token->floatvalue );
	}
	return AIBoxInt( negative ? -token->intvalue : token->intvalue );
}

// functions that are used to provide values to the behavior tree in condition nodes
static AIValue_t buildingIsDamaged( gentity_t *self, const AIValue_t* )
{
	return AIBoxInt( self->botMind->closestDamagedBuilding.ent != nullptr );
}

static AIValue_t haveWeapon( gentity_t *self, const AIValue_t *params )
{
	return AIBoxInt( BG_InventoryContainsWeapon( AIUnBoxInt( params[ 0 ] ), self->client->ps.stats ) );
}

static AIValue_t alertedToEnemy( gentity_t *self, const AIValue_t* )
{
	return AIBoxInt( self->botMind->bestEnemy.ent != nullptr );
}

static AIValue_t botTeam( gentity_t *self, const AIValue_t* )
{
	return AIBoxInt( self->client->pers.team );
}

static AIValue_t goalTeam( gentity_t *self, const AIValue_t* )
{
	botTarget_t const& goal = self->botMind->goal;
	if ( !goal.targetsValidEntity() )
	{
		return AIBoxInt( TEAM_NONE );
	}
	return AIBoxInt( G_Team( goal.getTargetedEntity() ) );
}

static AIValue_t goalType( gentity_t *self, const AIValue_t* )
{
	return AIBoxInt( Util::ordinal( self->botMind->goal.getTargetType() ) );
}

static AIValue_t goalDead( gentity_t *self, const AIValue_t* )
{
	const gentity_t *target =
		self->botMind->goal.getTargetedEntity();

	if ( !self->botMind->goal.targetsValidEntity() )
	{
		return AIBoxInt( true );
	}
	// is this needed?
	return AIBoxInt( target->s.eType == entityType_t::ET_BUILDABLE
		&& target->buildableTeam == self->client->pers.team
		&& !target->powered );
}

static AIValue_t goalBuildingType( gentity_t *self, const AIValue_t* )
{
	botTarget_t const& goal = self->botMind->goal;
	if ( goal.getTargetType() != entityType_t::ET_BUILDABLE )
	{
		return AIBoxInt( BA_NONE );
	}

	// entity has to be valid now or we would have returned BA_NONE
	const gentity_t *ent = goal.getTargetedEntity();
	return AIBoxInt( ent->s.modelindex );
}

static AIValue_t currentWeapon( gentity_t *self, const AIValue_t* )
{
	return AIBoxInt( BG_GetPlayerWeapon( &self->client->ps ) );
}

static AIValue_t haveUpgrade( gentity_t *self, const AIValue_t *params )
{
	int upgrade = AIUnBoxInt( params[ 0 ] );

	if ( upgrade == UP_MEDKIT && !self->botMind->skillSet[BOT_H_MEDKIT] )
	{
		// we don't know how to use it, pretend we don't have it
		return AIBoxInt( (int) false );
	}

	return AIBoxInt( !BG_UpgradeIsActive( upgrade, self->client->ps.stats )
		&& BG_InventoryContainsUpgrade( upgrade, self->client->ps.stats ) );
}

// This function returns the ratio of 2 ammo values (ammo/clips)
// for a specific weapon.
// Only used to reduce duplicated code.
static AIValue_t Ratio( int current, int max, bool infiniteAmmo )
{
	if ( infiniteAmmo )
	{
		return AIBoxFloat( 1.f );
	}
	return AIBoxFloat( current / static_cast<float>( max ) );
}

// Return emptyness ammo ratio of currently loaded clip
static AIValue_t percentAmmoClip( gentity_t *self, const AIValue_t* )
{
	ASSERT( self && self->client );
	playerState_t const& ps = self->client->ps;
	weaponAttributes_t const* wpa = BG_Weapon( BG_PrimaryWeapon( ps.stats ) );

	if ( wpa->maxAmmo == 0 )
	{
		return AIBoxFloat( 1.f );
	}
	return Ratio( ps.ammo, wpa->maxAmmo, wpa->infiniteAmmo );
}

// Return remaining ratio of remaining clips
static AIValue_t percentClips( gentity_t *self, const AIValue_t* )
{
	ASSERT( self && self->client );
	playerState_t const& ps = self->client->ps;
	weaponAttributes_t const* wpa = BG_Weapon( BG_PrimaryWeapon( ps.stats ) );

	if ( wpa->maxClips == 0 )
	{
		return AIBoxFloat( 0.f );
	}
	return Ratio( ps.clips, wpa->maxClips, wpa->infiniteAmmo );
}

static AIValue_t teamateHasWeapon( gentity_t *self, const AIValue_t *params )
{
	return AIBoxInt( BotTeamateHasWeapon( self, AIUnBoxInt( params[ 0 ] ) ) );
}

static AIValue_t distanceTo( gentity_t *self, const AIValue_t *params )
{
	AIEntity_t e = ( AIEntity_t ) AIUnBoxInt( params[ 0 ] );
	botEntityAndDistance_t ent = AIEntityToGentity( self, e );

	return AIBoxFloat( ent.distance );
}

static AIValue_t baseRushScore( gentity_t *self, const AIValue_t* )
{
	return AIBoxFloat( BotGetBaseRushScore( self ) );
}

static AIValue_t healScore( gentity_t *self, const AIValue_t* )
{
	return AIBoxFloat( BotGetHealScore( self ) );
}

static AIValue_t botClass( gentity_t *self, const AIValue_t* )
{
	return AIBoxInt( self->client->ps.stats[ STAT_CLASS ] );
}

static AIValue_t botSkill( gentity_t *self, const AIValue_t* )
{
	return AIBoxInt( self->botMind->skillLevel );
}

static AIValue_t inAttackRange( gentity_t *self, const AIValue_t *params )
{
	botTarget_t target;
	AIEntity_t et = ( AIEntity_t ) AIUnBoxInt( params[ 0 ] );
	botEntityAndDistance_t e = AIEntityToGentity( self ,et );

	if ( !e.ent )
	{
		return AIBoxInt( false );
	}

	target = e.ent;

	return AIBoxInt( BotTargetInAttackRange( self, target ) );
}

static AIValue_t isVisible( gentity_t *self, const AIValue_t *params )
{
	botTarget_t target;
	AIEntity_t et = ( AIEntity_t ) AIUnBoxInt( params[ 0 ] );
	botEntityAndDistance_t e = AIEntityToGentity( self, et );

	if ( !e.ent )
	{
		return AIBoxInt( false );
	}

	target = e.ent;

	if ( BotTargetIsVisible( self, target, MASK_OPAQUE ) )
	{
		if ( BotEntityIsValidTarget( e.ent ) )
		{
			self->botMind->enemyLastSeen = level.time;
		}
		return AIBoxInt( true );
	}

	return AIBoxInt( false );
}

static AIValue_t matchTime( gentity_t*, const AIValue_t* )
{
	return AIBoxInt( level.matchTime );
}

static AIValue_t directPathTo( gentity_t *self, const AIValue_t *params )
{
	AIEntity_t e = ( AIEntity_t ) AIUnBoxInt( params[ 0 ] );
	botEntityAndDistance_t ed = AIEntityToGentity( self, e );

	if ( e == E_GOAL )
	{
		return AIBoxInt( self->botMind->nav().directPathToGoal );
	}

	if ( ed.ent )
	{
		botTarget_t target;
		target = ed.ent;
		return AIBoxInt( BotPathIsWalkable( self, target ) );
	}

	return AIBoxInt( false );
}

static AIValue_t botCanEvolveTo( gentity_t *self, const AIValue_t *params )
{
	class_t c = ( class_t ) AIUnBoxInt( params[ 0 ] );

	return AIBoxInt( BotCanEvolveToClass( self, c ) &&
		G_AlienEvolve( self, c, false, /* dryRun = */ true ) );
}

// Returns a team's momentum for use in behavior trees.
static AIValue_t momentum( gentity_t* self, const AIValue_t *params )
{
	int requestedTeam = AIUnBoxInt( params[ 0 ] ); //is really a team_t
	if( !G_IsPlayableTeam( requestedTeam ) )
	{
		Log::Warn( "invalid argument %d to 'momentum' in behavior tree", requestedTeam );
		return AIBoxInt( 0 );
	}
	return AIBoxInt( level.team[ requestedTeam ].momentum );
}

// Number of alive (not necessarily active) friendly buildables of a given type
static AIValue_t numOurBuildings( gentity_t* self, const AIValue_t *params )
{
	int type = AIUnBoxInt( params[ 0 ] );
	if ( type <= E_NONE || type >= E_NUM_BUILDABLES )
	{
		Log::Warn( "invalid argument %d to 'numOurBuildings' in behavior tree", type );
		return AIBoxInt( 0 );
	}

	// fast track when counting spawns
	if ( type == BA_H_SPAWN )
	{
		return AIBoxInt( level.team[ TEAM_HUMANS ].numSpawns );
	}
	else if ( type == BA_A_SPAWN )
	{
		return AIBoxInt( level.team[ TEAM_ALIENS ].numSpawns );
	}

	team_t team = G_Team( self );
	int count = 0;
	ForEntities<BuildableComponent>( [&]( Entity &ent, BuildableComponent & ) {
		if ( ent.oldEnt->s.modelindex == type
		     && G_Team( ent.oldEnt ) == team
		     && Entities::IsAlive( ent ) )
		{
			++count;
		}
	});

	return AIBoxInt( count );
}

static AIValue_t aliveTime( gentity_t*self, const AIValue_t* )
{
	return AIBoxInt( level.time - self->botMind->spawnTime );
}

static AIValue_t randomChance( gentity_t*, const AIValue_t* )
{
	return AIBoxFloat( random() );
}

static AIValue_t resupplyScore( gentity_t* self, const AIValue_t* )
{
	return AIBoxFloat( BotGetResupplyScore( self ) );
}

// Read a cvar, no matter its type
static AIValue_t cvar( gentity_t*, const AIValue_t *params )
{
	// TODO: improve performance when the need arises
	// This guess the type of the cvar from what successfully parses.
	// You may want to change that someday.
	std::string cvar = AIUnBoxString( params[ 0 ] );
	bool boolean;
	int integer;
	float floating;

	if ( Cvar::ParseCvarValue( Cvar::GetValue( cvar ), boolean )  )
	{
		return AIBoxInt( boolean ? 1 : 0 );
	}
	if ( Cvar::ParseCvarValue( Cvar::GetValue( cvar ), integer )  )
	{
		return AIBoxInt( integer );
	}
	if ( Cvar::ParseCvarValue( Cvar::GetValue( cvar ), floating )  )
	{
		return AIBoxFloat( floating );
	}

	Log::Warn("Bot: could not read cvar '%s' as"
			" a number or a boolean", cvar);
	return AIBoxInt( 0 );
}

static AIValue_t percentHealth( gentity_t *self, const AIValue_t *params )
{
	AIEntity_t e = ( AIEntity_t ) AIUnBoxInt( params[ 0 ] );
	botEntityAndDistance_t et = AIEntityToGentity( self, e );

	if ( Entities::HasHealthComponent( et.ent ) )
	{
		return AIBoxFloat( Entities::HealthFraction( et.ent ) );
	}

	return AIBoxFloat( 0.f );
}

static AIValue_t stuckTime( gentity_t *self, const AIValue_t* )
{
	return AIBoxInt( level.time - self->botMind->stuckTime );
}

static AIValue_t usableBuildPoints( gentity_t *self, const AIValue_t* )
{
	team_t team = G_Team( self );
	int buildPoints = G_GetFreeBudget( team );
	if ( team == TEAM_ALIENS && !BG_BuildableUnlocked( BA_A_BOOSTER ) )
	{
		// save build points to build a booster as soon as unlocked
		buildPoints -= BG_Buildable( BA_A_BOOSTER )->buildPoints;
	}
	return AIBoxInt( buildPoints < 0 ? 0 : buildPoints );
}

static AIValue_t chosenBuildableCost( gentity_t *self, const AIValue_t* )
{
	buildable_t toBuild = BotChooseBuildableToBuild( self );
	if ( toBuild == BA_NONE )
	{
		return AIBoxInt( 32767 );  // return an "infinite" cost
	}
	return AIBoxInt( BG_Buildable( toBuild )->buildPoints );
}

static AIValue_t numUsersInTeam( gentity_t *self, const AIValue_t* )
{
	return AIBoxInt( level.team[ G_Team( self ) ].numPlayers );
}

static AIValue_t myTimer( gentity_t *self, const AIValue_t* )
{
	return AIBoxInt( level.time - self->botMind->myTimer );
}

static AIValue_t levelTime( gentity_t *self, const AIValue_t* )
{
	return AIBoxInt( level.matchTime );
}

static AIValue_t blackboardNumTransient( gentity_t *self, const AIValue_t *params )
{
	int val = AIUnBoxInt( params[ 0 ] );
	int counter = 0;
	for ( int i = 0; i < MAX_CLIENTS; i++ )
	{
		gentity_t *ent = &g_entities[ i ];
		if ( !( ent->r.svFlags & SVF_BOT ) || !G_OnSameTeam( ent, self ) )
		{
			continue;
		}
		if ( ent->botMind->blackboardTransient == val )
		{
			counter++;
		}
	}
	return AIBoxInt( counter );
}

// functions accessible to the behavior tree for use in condition nodes
static const struct AIConditionMap_s
{
	const char    *name;
	AIFunc        func;
	int           nparams;
} conditionFuncs[] =
{
	// It looks like behavior tree function names must be ordered alphabetically.
	{ "alertedToEnemy",    alertedToEnemy,    0 },
	{ "aliveTime",         aliveTime,         0 },
	{ "baseRushScore",     baseRushScore,     0 },
	{ "blackboardNumTransient", blackboardNumTransient, 1 },
	{ "buildingIsDamaged", buildingIsDamaged, 0 },
	{ "canEvolveTo",       botCanEvolveTo,    1 },
	{ "chosenBuildableCost", chosenBuildableCost, 0 },
	{ "class",             botClass,          0 },
	{ "cvar",              cvar,              1 },
	{ "directPathTo",      directPathTo,      1 },
	{ "distanceTo",        distanceTo,        1 },
	{ "goalBuildingType",  goalBuildingType,  0 },
	{ "goalIsDead",        goalDead,          0 },
	{ "goalTeam",          goalTeam,          0 },
	{ "goalType",          goalType,          0 },
	{ "haveUpgrade",       haveUpgrade,       1 },
	{ "haveWeapon",        haveWeapon,        1 },
	{ "healScore",         healScore,         0 },
	{ "inAttackRange",     inAttackRange,     1 },
	{ "isVisible",         isVisible,         1 },
	{ "levelTime",         levelTime,         0 },
	{ "matchTime",         matchTime,         0 },
	{ "momentum",          momentum,          1 },
	{ "myTimer",           myTimer,           0 },
	{ "numOurBuildings",   numOurBuildings,   1 },
	{ "numUsersInTeam",    numUsersInTeam,    0 },
	{ "percentAmmoClip",   percentAmmoClip,   0 },
	{ "percentClips",      percentClips,      0 },
	{ "percentHealth",     percentHealth,     1 },
	{ "random",            randomChance,      0 },
	{ "resupplyScore",     resupplyScore,     0 },
	{ "skill",             botSkill,          0 },
	{ "stuckTime",         stuckTime,         0 },
	{ "team",              botTeam,           0 },
	{ "teamateHasWeapon",  teamateHasWeapon,  1 },
	{ "usableBuildPoints", usableBuildPoints, 0 },
	{ "weapon",            currentWeapon,     0 }
};

static const struct AIOpMap_s
{
	const char            *str;
	int                   tokenSubtype;
	AIOpType_t            opType;
} conditionOps[] =
{
	{ ">=", P_LOGIC_GEQ,     OP_GREATERTHANEQUAL },
	{ ">",  P_LOGIC_GREATER, OP_GREATERTHAN      },
	{ "<=", P_LOGIC_LEQ,     OP_LESSTHANEQUAL    },
	{ "<",  P_LOGIC_LESS,    OP_LESSTHAN         },
	{ "==", P_LOGIC_EQ,      OP_EQUAL            },
	{ "!=", P_LOGIC_UNEQ,    OP_NEQUAL           },
	{ "!",  P_LOGIC_NOT,     OP_NOT              },
	{ "&&", P_LOGIC_AND,     OP_AND              },
	{ "||", P_LOGIC_OR,      OP_OR               }
};

static AIOpType_t opTypeFromToken( pc_token_stripped_t *token )
{
	if ( token->type != tokenType_t::TT_PUNCTUATION )
	{
		return OP_NONE;
	}

	for ( unsigned i = 0; i < ARRAY_LEN( conditionOps ); i++ )
	{
		if ( token->subtype == conditionOps[ i ].tokenSubtype )
		{
			return conditionOps[ i ].opType;
		}
	}
	return OP_NONE;
}

static const char *opTypeToString( AIOpType_t op )
{
	for ( unsigned i = 0; i < ARRAY_LEN( conditionOps ); i++ )
	{
		if ( conditionOps[ i ].opType == op )
		{
			return conditionOps[ i ].str;
		}
	}
	return nullptr;
}

// compare operator precedence
static int opCompare( AIOpType_t op1, AIOpType_t op2 )
{
	if ( op1 < op2 )
	{
		return 1;
	}
	if ( op1 > op2 )
	{
		return -1;
	}
	return 0;
}

static pc_token_list *findCloseParen( pc_token_list *start, pc_token_list *end )
{
	pc_token_list *list = start;
	int depth = 0;

	while ( list != end )
	{
		if ( list->token.string[ 0 ] == '(' )
		{
			depth++;
		}

		if ( list->token.string[ 0 ] == ')' )
		{
			depth--;
		}

		if ( depth == 0 )
		{
			return list;
		}

		list = list->next;
	}

	return nullptr;
}

static AIOp_t *newOp( pc_token_list *list )
{
	pc_token_list *current = list;
	AIOp_t *ret = nullptr;

	AIOpType_t op = opTypeFromToken( &current->token );

	if ( isBinaryOp( op ) )
	{
		AIBinaryOp_t *b = ( AIBinaryOp_t * ) BG_Alloc( sizeof( *b ) );
		b->opType = op;
		ret = ( AIOp_t * ) b;
	}
	else if ( isUnaryOp( op ) )
	{
		AIUnaryOp_t *u = ( AIUnaryOp_t * ) BG_Alloc( sizeof( *u ) );
		u->opType = op;
		ret = ( AIOp_t * ) u;
	}

	return ret;
}

static AIValue_t *newValueLiteral( pc_token_list **list )
{
	AIValue_t *ret;
	pc_token_list *current = *list;
	pc_token_stripped_t *token = &current->token;

	ret = ( AIValue_t * ) BG_Alloc( sizeof( *ret ) );

	*ret = AIBoxToken( token );

	*list = current->next;
	return ret;
}

static AIValue_t *parseFunctionParameters( pc_token_list **list, int *nparams, int minparams, int maxparams )
{
	pc_token_list *current = *list;
	pc_token_list *parenBegin = current->next;
	pc_token_list *parenEnd;
	pc_token_list *parse;
	AIValue_t     *params = nullptr;
	int           numParams = 0;

	// functions should always be proceeded by a '(' if they have parameters
	if ( !expectToken( "(", &parenBegin, false ) )
	{
		*list = current;
		return nullptr;
	}

	// find the end parenthesis around the function's args
	parenEnd = findCloseParen( parenBegin, nullptr );

	if ( !parenEnd )
	{
		Log::Warn( "could not find matching ')' for '(' on line %d", parenBegin->token.line );
		*list = parenBegin->next;
		return nullptr;
	}

	// count the number of parameters
	parse = parenBegin->next;

	while ( parse != parenEnd )
	{
		if ( parse->token.type == tokenType_t::TT_NUMBER || parse->token.type == tokenType_t::TT_STRING )
		{
			numParams++;
		}
		else if ( parse->token.string[ 0 ] != ',' && parse->token.string[ 0 ] != '-' )
		{
			Log::Warn( "Invalid token %s in parameter list on line %d", parse->token.string, parse->token.line );
			*list = parenEnd->next; // skip invalid function expression
			return nullptr;
		}
		parse = parse->next;
	}

	// warn if too many or too few parameters
	if ( numParams < minparams )
	{
		Log::Warn( "too few parameters for %s on line %d", current->token.string, current->token.line );
		*list = parenEnd->next;
		return nullptr;
	}

	if ( numParams > maxparams )
	{
		Log::Warn( "too many parameters for %s on line %d", current->token.string, current->token.line );
		*list = parenEnd->next;
		return nullptr;
	}

	*nparams = numParams;

	if ( numParams )
	{
		// add the parameters
		params = ( AIValue_t * ) BG_Alloc( sizeof( *params ) * numParams );

		numParams = 0;
		parse = parenBegin->next;
		bool negative = false;
		while ( parse != parenEnd )
		{
			if ( parse->token.type == tokenType_t::TT_PUNCTUATION && parse->token.string[ 0 ] == '-' )
			{
				negative = true;
			}
			else if ( parse->token.type == tokenType_t::TT_NUMBER || parse->token.type == tokenType_t::TT_STRING )
			{
				params[ numParams ] = AIBoxToken( &parse->token, negative );
				numParams++;
				negative = false;
			}
			parse = parse->next;
		}
	}
	*list = parenEnd->next;
	return params;
}

static AIValueFunc_t *newValueFunc( pc_token_list **list )
{
	AIValueFunc_t *ret = nullptr;
	AIValueFunc_t v;
	pc_token_list *current = *list;
	pc_token_list *parenBegin = nullptr;
	struct AIConditionMap_s *f;

	memset( &v, 0, sizeof( v ) );

	f = (struct AIConditionMap_s*) bsearch( current->token.string, conditionFuncs, ARRAY_LEN( conditionFuncs ), sizeof( *conditionFuncs ), cmdcmp );

	if ( !f )
	{
		Log::Warn( "Unknown function: %s on line %d", current->token.string, current->token.line );
		*list = current->next;
		return nullptr;
	}

	v.expType = EX_FUNC;
	v.func =    f->func;
	v.nparams = f->nparams;

	parenBegin = current->next;

	// if the function has no parameters, allow it to be used without parenthesis
	if ( v.nparams == 0 && parenBegin->token.string[ 0 ] != '(' )
	{
		ret = ( AIValueFunc_t * ) BG_Alloc( sizeof( *ret ) );
		memcpy( ret, &v, sizeof( *ret ) );

		*list = current->next;
		return ret;
	}

	v.params = parseFunctionParameters( list, &v.nparams, f->nparams, f->nparams );

	if ( !v.params && f->nparams > 0 )
	{
		return nullptr;
	}

	// create the value op
	ret = ( AIValueFunc_t * ) BG_Alloc( sizeof( *ret ) );

	// copy the members
	memcpy( ret, &v, sizeof( *ret ) );

	return ret;
}

static AIExpType_t *makeExpression( AIOp_t *op, AIExpType_t *exp1, AIExpType_t *exp2 )
{
	if ( isUnaryOp( op->opType ) )
	{
		AIUnaryOp_t *u = ( AIUnaryOp_t * ) op;
		u->exp = exp1;
	}
	else if ( isBinaryOp( op->opType ) )
	{
		AIBinaryOp_t *b = ( AIBinaryOp_t * ) op;
		b->exp1 = exp1;
		b->exp2 = exp2;
	}

	return ( AIExpType_t * ) op;
}

static AIExpType_t *Primary( pc_token_list **list );
static AIExpType_t *ReadConditionExpression( pc_token_list **list, AIOpType_t op2 )
{
	AIExpType_t *t;
	AIOpType_t  op;

	if ( !*list )
	{
		Log::Warn( "Unexpected end of file" );
		return nullptr;
	}

	t = Primary( list );

	if ( !t )
	{
		return nullptr;
	}

	op = opTypeFromToken( &(*list)->token );

	while ( isBinaryOp( op ) && opCompare( op, op2 ) >= 0 )
	{
		AIExpType_t *t1;
		pc_token_list *prev = *list;
		AIOp_t *exp = newOp( *list );
		*list = (*list)->next;
		t1 = ReadConditionExpression( list, op );

		if ( !t1 )
		{
			Log::Warn( "Missing right operand for %s on line %d", opTypeToString( op ), prev->token.line );
			FreeExpression( t );
			FreeOp( exp );
			return nullptr;
		}

		t = makeExpression( exp, t, t1 );

		op = opTypeFromToken( &(*list)->token );
	}

	return t;
}

static AIExpType_t *Primary( pc_token_list **list )
{
	pc_token_list *current = *list;
	AIExpType_t *tree = nullptr;

	if ( isUnaryOp( opTypeFromToken( &current->token ) ) )
	{
		AIExpType_t *t;
		AIOp_t *op = newOp( current );
		*list = current->next;
		t = ReadConditionExpression( list, op->opType );

		if ( !t )
		{
			Log::Warn( "Missing right operand for %s on line %d", opTypeToString( op->opType ), current->token.line );
			FreeOp( op );
			return nullptr;
		}

		tree = makeExpression( op, t, nullptr );
	}
	else if ( current->token.string[0] == '(' )
	{
		*list = current->next;
		tree = ReadConditionExpression( list, OP_NONE );
		if ( !expectToken( ")", list, true ) )
		{
			return nullptr;
		}
	}
	else if ( current->token.type == tokenType_t::TT_NUMBER )
	{
		tree = ( AIExpType_t * ) newValueLiteral( list );
	}
	else if ( current->token.type == tokenType_t::TT_NAME )
	{
		tree = ( AIExpType_t * ) newValueFunc( list );
	}
	else
	{
		Log::Warn( "token %s on line %d is not valid", current->token.string, current->token.line );
	}
	return tree;
}

static void BotInitNode( AINode_t type, AINodeRunner func, void *node )
{
	AIGenericNode_t *n = ( AIGenericNode_t * ) node;
	n->type = type;
	n->run = func;
}

static AIGenericNode_t *ReadNode( pc_token_list **tokenlist );
static AIGenericNode_t *ReadNodeList( pc_token_list **tokenlist );

/*
======================
ReadConditionNode

Parses and creates an AIConditionNode_t from a token list
The token list pointer is modified to point to the beginning of the next node text block

A condition node has the form:
condition [expression] {
	child node
}

or the form:
condition [expression]

[expression] can be any valid set of boolean operations and values
======================
*/

static AIGenericNode_t *ReadConditionNode( pc_token_list **tokenlist )
{
	pc_token_list *current = *tokenlist;

	AIConditionNode_t *condition;

	if ( !expectToken( "condition", &current, true ) )
	{
		return nullptr;
	}

	condition = allocNode( AIConditionNode_t );
	BotInitNode( CONDITION_NODE, BotConditionNode, condition );

	condition->exp = ReadConditionExpression( &current, OP_NONE );

	if ( !current )
	{
		*tokenlist = current;
		Log::Warn( "Unexpected end of file" );
		FreeConditionNode( condition );
		return nullptr;
	}

	if ( !condition->exp )
	{
		*tokenlist = current;
		FreeConditionNode( condition );
		return nullptr;
	}

	if ( Q_stricmp( current->token.string, "{" ) )
	{
		// this condition node has no child nodes
		*tokenlist = current;
		return ( AIGenericNode_t * ) condition;
	}

	current = current->next;

	condition->child = ReadNode( &current );

	if ( !condition->child )
	{
		Log::Warn( "Failed to parse child node of condition on line %d", (*tokenlist)->token.line );
		*tokenlist = current;
		FreeConditionNode( condition );
		return nullptr;
	}

	if ( !expectToken( "}", &current, true ) )
	{
		*tokenlist = current;
		FreeConditionNode( condition );
		return nullptr;
	}

	*tokenlist = current;

	return ( AIGenericNode_t * ) condition;
}

static const struct AIDecoratorMap_s
{
	const char   *name;
	AINodeRunner run;
	int          minparams;
	int          maxparams;
} AIDecorators[] =
{
	{ "invert", BotDecoratorInvert, 0, 0 },
	{ "return", BotDecoratorReturn, 1, 1 },
	{ "timer", BotDecoratorTimer, 1, 1 }
};

static AIGenericNode_t *ReadDecoratorNode( pc_token_list **list )
{
	pc_token_list *current = *list;
	struct AIDecoratorMap_s *dec;
	AIDecoratorNode_t       node;
	AIDecoratorNode_t       *ret;
	pc_token_list           *parenBegin;

	if ( !expectToken( "decorator", &current, true ) )
	{
		return nullptr;
	}

	if ( !current )
	{
		Log::Warn( "Unexpected end of file after line %d", (*list)->token.line );
		*list = current;
		return nullptr;
	}

	dec = (struct AIDecoratorMap_s*) bsearch( current->token.string, AIDecorators, ARRAY_LEN( AIDecorators ), sizeof( *AIDecorators ), cmdcmp );

	if ( !dec )
	{
		Log::Warn( "%s on line %d is not a valid decorator", current->token.string, current->token.line );
		*list = current;
		return nullptr;
	}

	parenBegin = current->next;

	memset( &node, 0, sizeof( node ) );

	BotInitNode( DECORATOR_NODE, dec->run, &node );

	// allow dropping of parenthesis if we don't require any parameters
	if ( dec->minparams == 0 && parenBegin->token.string[0] != '(' )
	{
		ret = allocNode( AIDecoratorNode_t );
		memcpy( ret, &node, sizeof( node ) );
		*list = parenBegin;
		return ( AIGenericNode_t * ) ret;
	}

	node.params = parseFunctionParameters( &current, &node.nparams, dec->minparams, dec->maxparams );

	if ( !node.params && dec->minparams > 0 )
	{
		*list = current;
		return nullptr;
	}

	if ( !expectToken( "{", &current, true ) )
	{
		*list = current;
		return nullptr;
	}

	node.child = ReadNode( &current );

	if ( !node.child )
	{
		Log::Warn( "Failed to parse child node of decorator on line %d",  (*list)->token.line );
		*list = current;
		return nullptr;
	}

	if ( !expectToken( "}", &current, true ) )
	{
		*list = current;
		return nullptr;
	}

	// create the decorator node
	ret = allocNode( AIDecoratorNode_t );
	memcpy( ret, &node, sizeof( *ret ) );

	*list = current;
	return ( AIGenericNode_t * ) ret;
}

static const struct AIActionMap_s
{
	const char   *name;
	AINodeRunner run;
	int          minparams;
	int          maxparams;
} AIActions[] =
{
	{ "activateUpgrade",   BotActionActivateUpgrade,   1, 1 },
	{ "aimAtGoal",         BotActionAimAtGoal,         0, 0 },
	{ "alternateStrafe",   BotActionAlternateStrafe,   0, 0 },
	{ "blackboardNoteTransient", BotActionBlackboardNoteTransient, 1, 1 },
	{ "buildNowChosenBuildable", BotActionBuildNowChosenBuildable, 0, 0 },
	{ "buy",               BotActionBuy,               1, 4 },
	{ "changeGoal",        BotActionChangeGoal,        1, 3 },
	{ "classDodge",        BotActionClassDodge,        0, 0 },
	{ "deactivateUpgrade", BotActionDeactivateUpgrade, 1, 1 },
	{ "equip",             BotActionBuy,               0, 0 },
	{ "evolve",            BotActionEvolve,            0, 0 },
	{ "evolveTo",          BotActionEvolveTo,          1, 1 },
	{ "fight",             BotActionFight,             0, 0 },
	{ "fireWeapon",        BotActionFireWeapon,        0, 0 },
	{ "flee",              BotActionFlee,              0, 0 },
	{ "follow",            BotActionFollow,            1, 1 },
	{ "gesture",           BotActionGesture,           0, 0 },
	{ "heal",              BotActionHeal,              0, 0 },
	{ "jump",              BotActionJump,              0, 0 },
	{ "moveInDir",         BotActionMoveInDir,         1, 2 },
	{ "moveTo",            BotActionMoveTo,            1, 2 },
	{ "moveToGoal",        BotActionMoveToGoal,        0, 0 },
	{ "repair",            BotActionRepair,            0, 0 },
	{ "resetMyTimer",      BotActionResetMyTimer,      0, 0 },
	{ "resetStuckTime",    BotActionResetStuckTime,    0, 0 },
	{ "roam",              BotActionRoam,              0, 0 },
	{ "roamInRadius",      BotActionRoamInRadius,      2, 2 },
	{ "rush",              BotActionRush,              0, 0 },
	{ "say",               BotActionSay,               1, 2 },
	{ "stayHere",          BotActionStayHere,          1, 1 },
	{ "strafeDodge",       BotActionStrafeDodge,       0, 0 },
	{ "suicide",           BotActionSuicide,           0, 0 },
	{ "teleport",          BotActionTeleport,          3, 3 },
};

/*
======================
ReadActionNode

Parses and creates an AIGenericNode_t with the type ACTION_NODE from a token list
The token list pointer is modified to point to the beginning of the next node text block after reading

An action node has the form:
action name( p1, p2, ... )

Where name defines the action to execute, and the parameters are surrounded by parenthesis
======================
*/

static AIGenericNode_t *ReadActionNode( pc_token_list **tokenlist )
{
	pc_token_list *current = *tokenlist;
	pc_token_list *parenBegin;
	AIActionNode_t        *ret = nullptr;
	AIActionNode_t        node = {};
	struct AIActionMap_s  *action = nullptr;

	node.lineNum = current->token.line;

	if ( !expectToken( "action", &current, true ) )
	{
		return nullptr;
	}

	if ( !current )
	{
		Log::Warn( "Unexpected end of file after line %d", (*tokenlist)->token.line );
		return nullptr;
	}

	action = (struct AIActionMap_s*) bsearch( current->token.string, AIActions, ARRAY_LEN( AIActions ), sizeof( *AIActions ), cmdcmp );

	if ( !action )
	{
		Log::Warn( "%s on line %d is not a valid action", current->token.string, current->token.line );
		*tokenlist = current;
		return nullptr;
	}

	parenBegin = current->next;


	BotInitNode( ACTION_NODE, action->run, &node );

	// allow dropping of parenthesis if we don't require any parameters
	if ( action->minparams == 0 && parenBegin->token.string[0] != '(' )
	{
		ret = allocNode( AIActionNode_t );
		memcpy( ret, &node, sizeof( node ) );
		*tokenlist = parenBegin;
		return ( AIGenericNode_t * ) ret;
	}

	node.params = parseFunctionParameters( &current, &node.nparams, action->minparams, action->maxparams );

	if ( !node.params && action->minparams > 0 )
	{
		return nullptr;
	}

	// create the action node
	ret = allocNode( AIActionNode_t );
	memcpy( ret, &node, sizeof( *ret ) );

	*tokenlist = current;
	return ( AIGenericNode_t * ) ret;
}

static AIGenericNode_t *ReadSpawnNode( pc_token_list **tokenlist )
{
	pc_token_list *current = *tokenlist;

	ASSERT( Str::IsIEqual( current->token.string, "spawnAs" ) );
	current = current->next;

	if ( !current || current->token.type != tokenType_t::TT_NUMBER )
	{
		Log::Warn( "Invalid token following spawnAs on line %d", (*tokenlist)->token.line );
		return nullptr;
	}

	AISpawnNode_t *ret = allocNode( AISpawnNode_t );
	BotInitNode( SPAWN_NODE, BotSpawnNode, ret );
	ret->selection = current->token.intvalue;
	current = current->next;

	*tokenlist = current;
	return ( AIGenericNode_t * ) ret;

}

static AIGenericNode_t *ReadFallback( pc_token_list **tokenlist )
{
	pc_token_list *current = *tokenlist;
	AIGenericNode_t *node = nullptr;
	current = current->next;

	node = ReadNodeList( &current );

	*tokenlist = current;
	if ( !node )
	{
		return nullptr;
	}
	BotInitNode( SELECTOR_NODE, BotFallbackNode, node );
	return node;
}

static AIGenericNode_t *ReadSequence( pc_token_list **tokenlist )
{
	pc_token_list *current = *tokenlist;
	AIGenericNode_t *node = nullptr;
	current = current->next;

	node = ReadNodeList( &current );

	*tokenlist = current;
	if ( !node )
	{
		return nullptr;
	}
	BotInitNode( SELECTOR_NODE, BotSequenceNode, node );
	return node;
}

static AIGenericNode_t *ReadSelector( pc_token_list **tokenlist )
{
	pc_token_list *current = *tokenlist;
	AIGenericNode_t *node = nullptr;
	current = current->next;

	node = ReadNodeList( &current );

	*tokenlist = current;
	if ( !node )
	{
		return nullptr;
	}
	BotInitNode( SELECTOR_NODE, BotSelectorNode, node );
	return node;
}

static AIGenericNode_t *ReadConcurrent( pc_token_list **tokenlist )
{
	pc_token_list *current = *tokenlist;
	AIGenericNode_t *node = nullptr;
	current = current->next;

	node = ReadNodeList( &current );

	*tokenlist = current;
	if ( !node )
	{
		return nullptr;
	}
	BotInitNode( SELECTOR_NODE, BotConcurrentNode, node );
	return node;
}

// A selectClass block at the beginning of a behavior tree file can be used to instruct the bot
// which class to spawn as. For example:
/*
// Spawn as builder if there is no main structure
selectClass {
	selector {
		condition team == TEAM_ALIENS && numOurBuildings(E_A_OVERMIND) == 0 {
			spawnAs PCL_ALIEN_BUILDER0
		}
		condition team == TEAM_HUMANS && numOurBuildings(E_H_REACTOR) == 0 {
			spawnAs WP_HBUILD
		}
	}
}
*/
// It is evaluated immediately before spawning. You might argue that this is a form of cheating
// because normal players must decide their class when joining the spawn queue, but since bots
// automatically go to the back of the queue when a real person joins it, their behavior here
// is something the bots could have legitimately accomplished by leaving and rejoining the queue.
static AIGenericNode_t *ReadSelectClass( pc_token_list **tokenlist )
{
	pc_token_list *current = *tokenlist;
	ASSERT( Str::IsIEqual( current->token.string, "selectClass" ) );
	current = current->next;

	if ( !expectToken( "{", &current, true ) )
	{
		return nullptr;
	}

	AIGenericNode_t *node = ReadNode( &current );
	*tokenlist = current;

	if ( !node )
	{
		return nullptr;
	}

	if ( !expectToken( "}", &current, true ) )
	{
		FreeNode( node );
		return nullptr;
	}

	*tokenlist = current;
	return node;
}

/*
======================
ReadNodeList

Parses and creates an AINodeList_t from a token list
The token list pointer is modified to point to the beginning of the next node text block after reading
======================
*/

static AIGenericNode_t *ReadNodeList( pc_token_list **tokenlist )
{
	AINodeList_t *list;
	pc_token_list *current = *tokenlist;

	if ( !expectToken( "{", &current, true ) )
	{
		return nullptr;
	}

	list = allocNode( AINodeList_t );

	while ( Q_stricmp( current->token.string, "}" ) )
	{
		AIGenericNode_t *node = ReadNode( &current );

		if ( node && list->numNodes >= MAX_NODE_LIST )
		{
			Log::Warn( "Max selector children limit exceeded at line %d", (*tokenlist)->token.line );
			FreeNode( node );
			FreeNodeList( list );
			*tokenlist = current;
			return nullptr;
		}
		else if ( node )
		{
			list->list[ list->numNodes ] = node;
			list->numNodes++;
		}

		if ( !node )
		{
			*tokenlist = current;
			FreeNodeList( list );
			return nullptr;
		}

		if ( !current )
		{
			*tokenlist = current;
			return ( AIGenericNode_t * ) list;
		}
	}

	*tokenlist = current->next;
	return ( AIGenericNode_t * ) list;
}

static AITreeList_t *currentList = nullptr;

static AIGenericNode_t *ReadBehaviorTreeInclude( pc_token_list **tokenlist )
{
	pc_token_list *first = *tokenlist;
	pc_token_list *current = first;

	AIBehaviorTree_t *behavior;

	if ( !expectToken( "behavior", &current, true ) )
	{
		return nullptr;
	}

	if ( !current )
	{
		Log::Warn( "Unexpected end of file after line %d", first->token.line );
		*tokenlist = current;
		return nullptr;
	}

	behavior = ReadBehaviorTree( current->token.string, currentList );

	if ( !behavior )
	{
		Log::Warn( "Could not load behavior %s on line %d", current->token.string, current->token.line );
		*tokenlist = current->next;
		return nullptr;
	}

	if ( !behavior->root )
	{
		Log::Warn( "Recursive behavior %s on line %d", current->token.string, current->token.line );
		*tokenlist = current->next;
		return nullptr;
	}

	*tokenlist = current->next;
	return ( AIGenericNode_t * ) behavior;
}

/*
======================
ReadNode

Parses and creates an AIGenericNode_t from a token list
The token list pointer is modified to point to the next node text block after reading

This function delegates the reading to the sub functions
ReadNodeList, ReadActionNode, and ReadConditionNode depending on the first token in the list
======================
*/

static AIGenericNode_t *ReadNode( pc_token_list **tokenlist )
{
	pc_token_list *current = *tokenlist;
	AIGenericNode_t *node;

	if ( !Q_stricmp( current->token.string, "selector" ) )
	{
		node = ReadSelector( &current );
	}
	else if ( !Q_stricmp( current->token.string, "fallback" ) )
	{
		node = ReadFallback( &current );
	}
	else if ( !Q_stricmp( current->token.string, "sequence" ) )
	{
		node = ReadSequence( &current );
	}
	else if ( !Q_stricmp( current->token.string, "concurrent" ) )
	{
		node = ReadConcurrent( &current );
	}
	else if ( !Q_stricmp( current->token.string, "action" ) )
	{
		node = ReadActionNode( &current );
	}
	else if ( !Q_stricmp( current->token.string, "condition" ) )
	{
		node = ReadConditionNode( &current );
	}
	else if ( !Q_stricmp( current->token.string, "decorator" ) )
	{
		node = ReadDecoratorNode( &current );
	}
	else if ( !Q_stricmp( current->token.string, "behavior" ) )
	{
		node = ReadBehaviorTreeInclude( &current );
	}
	else if ( !Q_stricmp( current->token.string, "spawnAs" ) )
	{
		node = ReadSpawnNode( &current );
	}
	else
	{
		Log::Warn( "Invalid token on line %d found: %s", current->token.line, current->token.string );
		node = nullptr;
	}

	*tokenlist = current;
	return node;
}

// Add preprocessor defines for use in the behavior tree
static void SetBehaviorTreeDefines()
{
	// This function only needs to be used once
	static bool loaded = false;
	if ( loaded )
	{
		return;
	}
	loaded = true;

	// add upgrades
	D( UP_LIGHTARMOUR );
	D( UP_MEDIUMARMOUR );
	D( UP_BATTLESUIT );
	D( UP_RADAR );
	D( UP_JETPACK );
	D( UP_GRENADE );
	D( UP_FIREBOMB );
	D( UP_MEDKIT );

	// add weapons
	D( WP_MACHINEGUN );
	D( WP_PAIN_SAW );
	D( WP_SHOTGUN );
	D( WP_LAS_GUN );
	D( WP_MASS_DRIVER );
	D( WP_CHAINGUN );
	D( WP_FLAMER );
	D( WP_PULSE_RIFLE );
	D( WP_LUCIFER_CANNON );
	D( WP_HBUILD );
	D( WP_ABUILD );
	D( WP_ABUILD2 );

	// add teams
	D( TEAM_ALIENS );
	D( TEAM_HUMANS );
	D( TEAM_NONE );

	// add AIEntitys
	D( E_NONE );
	D( E_A_SPAWN );
	D( E_A_OVERMIND );
	D( E_A_BARRICADE );
	D( E_A_ACIDTUBE );
	D( E_A_TRAPPER );
	D( E_A_BOOSTER );
	D( E_A_HIVE );
	D( E_A_LEECH );
	D( E_A_SPIKER );
	D( E_H_SPAWN );
	D( E_H_MGTURRET );
	D( E_H_ROCKETPOD );
	D( E_H_ARMOURY );
	D( E_H_MEDISTAT );
	D( E_H_DRILL );
	D( E_H_REACTOR );
	D( E_GOAL );
	D( E_ENEMY );
	D( E_DAMAGEDBUILDING );
	D( E_FRIENDLYBUILDING );
	D( E_ENEMYBUILDING );
	D( E_SELF );

	// add player classes
	D( PCL_NONE );
	D( PCL_ALIEN_BUILDER0 );
	D( PCL_ALIEN_BUILDER0_UPG );
	D( PCL_ALIEN_LEVEL0 );
	D( PCL_ALIEN_LEVEL1 );
	D( PCL_ALIEN_LEVEL2 );
	D( PCL_ALIEN_LEVEL2_UPG );
	D( PCL_ALIEN_LEVEL3 );
	D( PCL_ALIEN_LEVEL3_UPG );
	D( PCL_ALIEN_LEVEL4 );
	D( PCL_HUMAN_NAKED );
	D( PCL_HUMAN_LIGHT );
	D( PCL_HUMAN_MEDIUM );
	D( PCL_HUMAN_BSUIT );

	D( MOVE_FORWARD );
	D( MOVE_BACKWARD );
	D( MOVE_RIGHT );
	D( MOVE_LEFT );

	D2( ET_BUILDABLE, Util::ordinal(entityType_t::ET_BUILDABLE) );
	D2( ET_PLAYER, Util::ordinal(entityType_t::ET_PLAYER) );

	// node return status
	D( STATUS_RUNNING );
	D( STATUS_SUCCESS );
	D( STATUS_FAILURE );

	D( SAY_ALL );
	D( SAY_TEAM );
	D( SAY_AREA );
	D( SAY_AREA_TEAM );
}

/*
======================
ReadBehaviorTree

Load a behavior tree of the given name from a file
TODO: don't crash if there is an unexpected EOF anywhere in the tree
======================
*/

AIBehaviorTree_t *ReadBehaviorTree( const char *name, AITreeList_t *list )
{
	char treefilename[ MAX_QPATH ];
	int handle;
	pc_token_list *tokenlist;
	pc_token_list *current;
	AIGenericNode_t *node = nullptr;

	currentList = list;

	// check if this behavior tree has already been loaded
	for ( AIBehaviorTree_t *tree : *list )
	{
		if ( !Q_stricmp( tree->name, name ) )
		{
			return tree;
		}
	}

	SetBehaviorTreeDefines();

	Q_strncpyz( treefilename, va( "bots/%s.bt", name ), sizeof( treefilename ) );

	handle = Parse_LoadSourceHandle( treefilename, BG_FOpenGameOrPakPath );
	if ( !handle )
	{
		Log::Warn( "Cannot load behavior tree %s: File not found", treefilename );
		return nullptr;
	}

	tokenlist = CreateTokenList( handle );
	Parse_FreeSourceHandle( handle );

	auto *tree = ( AIBehaviorTree_t * ) BG_Alloc( sizeof( AIBehaviorTree_t ) );

	Q_strncpyz( tree->name, name, sizeof( tree->name ) );

	tree->run = BotBehaviorNode;
	tree->type = BEHAVIOR_NODE;

	list->push_back( tree );

	current = tokenlist;

	if ( Str::IsIEqual( current->token.string, "selectClass" ) )
	{
		if ( AIGenericNode_t *selectClass = ReadSelectClass( &current ) )
		{
			tree->classSelectionTree = selectClass;
			node = ReadNode( &current );
		}
	}
	else
	{
		node = ReadNode( &current );
	}

	if ( node )
	{
		tree->root = node;
	}
	else
	{
		auto it = std::find( list->begin(), list->end(), tree );
		ASSERT_NQ( it, list->end() );
		list->erase( it );
		FreeBehaviorTree( tree );
		tree = nullptr;
	}

	FreeTokenList( tokenlist );
	return tree;
}

pc_token_list *CreateTokenList( int handle )
{
	pc_token_t token;
	char filename[ MAX_QPATH ];
	pc_token_list *current = nullptr;
	pc_token_list *root = nullptr;

	while ( Parse_ReadTokenHandle( handle, &token ) )
	{
		pc_token_list *list = ( pc_token_list * ) BG_Alloc( sizeof( pc_token_list ) );

		if ( current )
		{
			list->prev = current;
			current->next = list;
		}
		else
		{
			list->prev = list;
			root = list;
		}

		current = list;
		current->next = nullptr;

		current->token.floatvalue = token.floatvalue;
		current->token.intvalue = token.intvalue;
		current->token.subtype = token.subtype;
		current->token.type = token.type;
		current->token.string = BG_strdup( token.string );
		Parse_SourceFileAndLine( handle, filename, &current->token.line );
	}

	return root;
}

void FreeTokenList( pc_token_list *list )
{
	pc_token_list *current = list;
	while( current )
	{
		pc_token_list *node = current;
		current = current->next;

		BG_Free( node->token.string );
		BG_Free( node );
	}
}

// functions for keeping a list of behavior trees loaded
void FreeTreeList( AITreeList_t *list )
{
	for ( AIBehaviorTree_t *tree : *list )
	{
		FreeBehaviorTree( tree );
	}

	list->clear();
}

// functions for freeing the memory of condition expressions
void FreeValue( AIValue_t *v )
{
	if ( !v )
	{
		return;
	}
	AIDestroyValue( *v );
	BG_Free( v );
}

void FreeValueFunc( AIValueFunc_t *v )
{
	int i;
	if ( !v )
	{
		return;
	}

	for ( i = 0; i < v->nparams; i++ )
	{
		AIDestroyValue( v->params[ i ] );
	}

	BG_Free( v->params );
	BG_Free( v );
}

void FreeExpression( AIExpType_t *exp )
{
	if ( !exp )
	{
		return;
	}

	if ( *exp == EX_FUNC )
	{
		AIValueFunc_t *v = ( AIValueFunc_t * ) exp;
		FreeValueFunc( v );
	}
	else if ( *exp == EX_VALUE )
	{
		AIValue_t *v = ( AIValue_t * ) exp;

		FreeValue( v );
	}
	else if ( *exp == EX_OP )
	{
		AIOp_t *op = ( AIOp_t * ) exp;

		FreeOp( op );
	}
}
void FreeOp( AIOp_t *op )
{
	if ( !op )
	{
		return;
	}

	if ( isBinaryOp( op->opType ) )
	{
		AIBinaryOp_t *b = ( AIBinaryOp_t * ) op;
		FreeExpression( b->exp1 );
		FreeExpression( b->exp2 );
	}
	else if ( isUnaryOp( op->opType ) )
	{
		AIUnaryOp_t *u = ( AIUnaryOp_t * ) op;
		FreeExpression( u->exp );
	}

	BG_Free( op );
}

// freeing behavior tree nodes
void FreeConditionNode( AIConditionNode_t *node )
{
	FreeNode( node->child );
	FreeExpression( node->exp );
	BG_Free( node );
}

void FreeNodeList( AINodeList_t *node )
{
	int i;
	for ( i = 0; i < node->numNodes; i++ )
	{
		FreeNode( node->list[ i ] );
	}
	BG_Free( node );
}

void FreeActionNode( AIActionNode_t *action )
{
	int i;
	for ( i = 0; i < action->nparams; i++ )
	{
		AIDestroyValue( action->params[ i ] );
	}

	BG_Free( action->params );
	BG_Free( action );
}

static void FreeDecoratorNode( AIDecoratorNode_t *decorator )
{
	BG_Free( decorator->params );
	FreeNode( decorator->child );
	BG_Free( decorator );
}

void FreeNode( AIGenericNode_t *node )
{
	if ( !node )
	{
		return;
	}

	switch( node->type )
	{
		case SELECTOR_NODE:
			FreeNodeList( ( AINodeList_t * ) node );
			break;
		case CONDITION_NODE:
			FreeConditionNode( ( AIConditionNode_t * ) node );
			break;
		case ACTION_NODE:
			FreeActionNode( ( AIActionNode_t * ) node );
			break;
		case DECORATOR_NODE:
			FreeDecoratorNode( ( AIDecoratorNode_t * ) node );
			break;
		case SPAWN_NODE:
			BG_Free( node );
			break;
		case BEHAVIOR_NODE:
			// this is a pointer into the tree list
			break;
	}
}

void FreeBehaviorTree( AIBehaviorTree_t *tree )
{
	if ( tree )
	{
		FreeNode(tree->root);
		FreeNode( tree->classSelectionTree );
		BG_Free( tree );
	}
	else
	{
		Log::Warn( "Attempted to free NULL behavior tree" );
	}
}
