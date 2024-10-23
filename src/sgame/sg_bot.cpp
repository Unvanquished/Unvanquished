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

#include "common/Common.h"
#include "sg_bot_parse.h"
#include "sg_bot_util.h"
#include "Entities.h"

Cvar::Modified<Cvar::Cvar<int>> g_bot_defaultFill("g_bot_defaultFill", "fills both teams with that number of bots at start of game", Cvar::NONE, 0);
static Cvar::Range<Cvar::Cvar<int>> generateNeededMesh(
	"g_bot_navgen_onDemand",
	"automatically generate navmeshes when a bot is added (1 = in background, -1 = blocking)",
	Cvar::NONE, 1, -1, 1);
static Cvar::Cvar<int> traceClient(
	"g_bot_traceClient", "show running BT node for this client num", Cvar::NONE, -1);

static botMemory_t g_botMind[MAX_CLIENTS];
static AITreeList_t treeList;

/*
=======================
Bot management functions
=======================
*/
struct nameInfo_t {
	std::string name;
	bool inUse;
};
static BoundedVector<nameInfo_t, MAX_CLIENTS> botNames[NUM_TEAMS];

static void G_BotListTeamNames( gentity_t *ent, const char *heading, team_t team, const char *marker )
{
	if ( !botNames[team].empty() )
	{
		ADMP( heading );
		ADMBP_begin();

		for ( nameInfo_t &nameInfo : botNames[team] )
		{
			ADMBP( va( "  %s^* %s", nameInfo.inUse ? marker : " ", nameInfo.name.c_str() ) );
		}

		ADMBP_end();
	}
}

void G_BotListNames( gentity_t *ent )
{
	G_BotListTeamNames( ent, QQ( N_( "^3Alien bot names:" ) ), TEAM_ALIENS, "^1*" );
	G_BotListTeamNames( ent, QQ( N_( "^3Human bot names:" ) ), TEAM_HUMANS, "^5*" );
}

bool G_BotClearNames()
{
	for ( auto &teamsBotNames : botNames )
	{
		for ( nameInfo_t &nameInfo : teamsBotNames )
		{
			if ( nameInfo.inUse )
			{
				return false;
			}
		}
	}

	for ( auto &teamsBotNames : botNames )
	{
		teamsBotNames.clear();
	}

	return true;
}

int G_BotAddNames( team_t team, int arg, int last )
{
	int  added = 0;
	char name[MAX_NAME_LENGTH];

	while ( arg < last )
	{
		trap_Argv( arg++, name, sizeof( name ) );

		// name already in the list? (quick check, including colours & invalid)
		for ( int t = 1; t < NUM_TEAMS; ++t )
		{
			for ( nameInfo_t &nameInfo : botNames[t] )
			{
				if ( !Q_stricmp( nameInfo.name.c_str(), name ) )
				{
					goto next;
				}
			}
		}

		botNames[team].append({ name, /*.inUse = */ false });
		++added;

		next:
		;
	}

	return added;
}

static const char *G_BotSelectName( team_t team )
{
	unsigned int count = 0;
	for ( nameInfo_t &nameInfo : botNames[team] )
	{
		if ( !nameInfo.inUse )
		{
			count++;
		}
	}

	if (count == 0)
	{
		return nullptr;
	}

	unsigned int choice = rand() % count;

	unsigned int index = 0;
	for ( nameInfo_t &nameInfo : botNames[team] )
	{
		if ( nameInfo.inUse )
		{
			continue;
		}

		if (index++ == choice)
		{
			return nameInfo.name.c_str();
		}
	}

	ASSERT_UNREACHABLE();
}

static void G_BotNameUsed( team_t team, const char *name, bool inUse )
{
	for ( nameInfo_t &nameInfo : botNames[team] )
	{
		if ( !Q_stricmp( name, nameInfo.name.c_str() ) )
		{
			nameInfo.inUse = inUse;
			return;
		}
	}
}

void G_BotSetSkill( int clientNum, int skill )
{
	gentity_t *bot = &g_entities[clientNum];

	if ( !bot->botMind )
	{
		return;
	}

	BotSetSkillLevel( bot, skill );
}

void G_BotChangeBehavior( int clientNum, Str::StringRef behavior )
{
	gentity_t *bot = &g_entities[clientNum];
	ASSERT( bot->client->pers.isBot && bot->botMind );

	G_BotSetBehavior( bot->botMind, behavior );
}

bool G_BotSetBehavior( botMemory_t *botMind, Str::StringRef behavior )
{
	G_Bot_ResetBehaviorState( *botMind );
	botMind->blackboardTransient = 0;
	botMind->myTimer = level.time;

	botMind->behaviorTree = ReadBehaviorTree( behavior.c_str(), &treeList );

	if ( !botMind->behaviorTree )
	{
		Log::Warn( "Problem when loading behavior tree %s, trying default", behavior );
		const std::string behaviorString = g_bot_defaultBehavior.Get();
		botMind->behaviorTree = ReadBehaviorTree( behaviorString.c_str(), &treeList );

		if ( !botMind->behaviorTree )
		{
			Log::Warn( "Problem when loading default behavior tree" );
			return false;
		}
	}
	return true;
}

bool G_BotSetDefaults( int clientNum, team_t team, Str::StringRef behavior )
{
	botMemory_t *botMind;
	gentity_t *self = &g_entities[ clientNum ];
	botMind = self->botMind = &g_botMind[clientNum];
	ResetStruct( *botMind );

	if ( !G_BotSetBehavior( botMind, behavior ) )
	{
		return false;
	}

	// TODO(0.56): Remove. Use client->pers.isBot instead
	self->r.svFlags |= SVF_BOT;

	if ( team != TEAM_NONE )
	{
		self->client->sess.restartTeam = team;
	}

	self->pain = BotPain;

	return true;
}

bool G_BotAdd( const char *name, team_t team, int skill, const char *behavior, bool filler )
{
	char userinfo[MAX_INFO_STRING];
	const char* s = 0;
	bool autoname = false;

	ASSERT( navMeshLoaded == navMeshStatus_t::LOADED || navMeshLoaded == navMeshStatus_t::GENERATING );

	// find what clientNum to use for bot
	int clientNum = trap_BotAllocateClient();

	if ( clientNum < 0 )
	{
		Log::Warn( "no more slots for bot" );
		return false;
	}
	gentity_t *bot = &g_entities[ clientNum ];

	// TODO(0.56): Remove. Use client->pers.isBot instead
	bot->r.svFlags |= SVF_BOT;

	if ( !Q_stricmp( name, BOT_NAME_FROM_LIST ) )
	{
		name = G_BotSelectName( team );
		autoname = name != nullptr;
	}

	//default bot data
	bool okay = G_BotSetDefaults( clientNum, team, behavior );

	// register user information
	userinfo[0] = '\0';
	Info_SetValueForKey( userinfo, "cg_unlagged", "0", false ); // bots do not lag
	Info_SetValueForKey( userinfo, "name", name ? name : "", false ); // allow defaulting
	Info_SetValueForKey( userinfo, "rate", "25000", false );
	Info_SetValueForKey( userinfo, "snaps", "20", false );
	if ( autoname )
	{
		Info_SetValueForKey( userinfo, "autoname", name, false );
	}

	//so we can connect if server is password protected
	if ( g_needpass.Get() )
	{
		Info_SetValueForKey( userinfo, "password", g_password.Get().c_str(), false );
	}

	trap_SetUserinfo( clientNum, userinfo );

	// have it connect to the game as a normal client
	if ( ( s = ClientBotConnect( clientNum, true ) ) )
	{
		// won't let us join
		Log::Warn( s );
		okay = false;
	}

	if ( !okay )
	{
		G_BotDel( clientNum );
		return false;
	}

	if ( autoname )
	{
		G_BotNameUsed( team, name, true );
	}

	ClientBegin( clientNum );
	bot->pain = BotPain; // ClientBegin resets the pain function
	level.clients[clientNum].pers.isFillerBot = filler;
	G_ChangeTeam( bot, team );
	BotSetSkillLevel( bot, skill );
	return true;
}

void G_BotDel( int clientNum )
{
	gentity_t *bot = &g_entities[clientNum];
	char userinfo[MAX_INFO_STRING];
	const char *autoname;

	if ( !g_clients[ clientNum ].pers.isBot || !bot->botMind )
	{
		Log::Warn( "'^7%s^*' is not a bot", bot->client->pers.netname );
		return;
	}

	trap_GetUserinfo( clientNum, userinfo, sizeof( userinfo ) );

	autoname = Info_ValueForKey( userinfo, "autoname" );
	if ( autoname && *autoname )
	{
		G_BotNameUsed( G_Team( bot ), autoname, false );
	}

	trap_SendServerCommand( -1, va( "print_tr %s %s", QQ( N_( "$1$^* disconnected" ) ),
					Quote( bot->client->pers.netname ) ) );
	trap_DropClient( clientNum, "disconnected" );
}

void G_BotDelAllBots()
{
	for ( int i = 0; i < MAX_CLIENTS; i++ )
	{
		if ( level.clients[ i ].pers.connected != CON_DISCONNECTED && level.clients[ i ].pers.isBot )
		{
			G_BotDel( i );
		}
	}

	for ( auto &teamsBotNames : botNames )
	{
		for ( nameInfo_t &nameInfo : teamsBotNames )
		{
			nameInfo.inUse = false;
		}
	}

	g_bot_defaultFill.GetModifiedValue(); // clear modified flag
	for ( auto &team : level.team )
	{
		team.botFillTeamSize = 0;
	}
}

static void ShowRunningNode( gentity_t *self, AINodeStatus_t status )
{
	const char *name = self->client->pers.netname;
	switch ( status )
	{
	case STATUS_FAILURE:
		Log::defaultLogger.WithoutSuppression().Notice( "%s^* root tree exited with STATUS_FAILURE", name );
		break;
	case STATUS_SUCCESS:
		Log::defaultLogger.WithoutSuppression().Notice( "%s^* root tree exited with STATUS_SUCCESS", name );
		break;
	case STATUS_RUNNING:
		ASSERT( !self->botMind->runningNodes.empty() );
		ASSERT_EQ( self->botMind->runningNodes[ 0 ]->type, AINode_t::ACTION_NODE );
		int line = reinterpret_cast<AIActionNode_t *>( self->botMind->runningNodes[ 0 ] )->lineNum;
		const char *tree = "<unknown>";
		for ( const AIGenericNode_t *node : self->botMind->runningNodes )
		{
			if ( node->type == AINode_t::BEHAVIOR_NODE )
			{
				tree = reinterpret_cast<const AIBehaviorTree_t *>( node )->name;
				break;
			}
		}
		Log::defaultLogger.WithoutSuppression().Notice( "%s^* running at %s.bt:%d", name, tree, line );
		break;
	}
}

/*
=======================
Bot Thinks
=======================
*/

void G_BotThink( gentity_t *self )
{
	char buf[MAX_STRING_CHARS];
	usercmd_t *botCmdBuffer;

	self->botMind->cmdBuffer = self->client->pers.cmd;
	botCmdBuffer = &self->botMind->cmdBuffer;

	//reset command buffer
	usercmdClearButtons( botCmdBuffer->buttons );

	// for nudges, e.g. spawn blocking
	glm::vec3 nudge = { 0, 0, 0 };
	if ( botCmdBuffer->doubleTap != dtType_t::DT_NONE )
	{
		nudge = { botCmdBuffer->forwardmove, botCmdBuffer->rightmove, botCmdBuffer->upmove };
	}

	botCmdBuffer->forwardmove = 0;
	botCmdBuffer->rightmove = 0;
	botCmdBuffer->upmove = 0;
	botCmdBuffer->doubleTap = dtType_t::DT_NONE;
	botCmdBuffer->flags = 0;

	//acknowledge recieved server commands
	//MUST be done
	while ( trap_BotGetServerCommand( self->num(), buf, sizeof( buf ) ) );

	BotSearchForEnemy( self );

	// Populate transient caches
	BotFindClosestBuildings( self );
	BotFindDamagedFriendlyStructure( self );
	BotCalculateStuckTime( self );

	//infinite funds cvar
	if ( g_bot_infiniteFunds.Get() )
	{
		G_AddCreditToClient( self->client, HUMAN_MAX_CREDITS, true );
	}

	//hacky ping fix
	self->client->ps.ping = rand() % 50 + 50;

	//reset the user specified client number if the client disconnected
	if ( self->botMind->userSpecifiedClientNum )
	{
		int userSpecifiedClientNum = *self->botMind->userSpecifiedClientNum;
		gentity_t *ent = &g_entities[ userSpecifiedClientNum ];
		if ( !ent->client || ent->client->pers.connected == CON_DISCONNECTED )
		{
			self->botMind->userSpecifiedClientNum = Util::nullopt;
		}
	}

	self->botMind->blackboardTransient = 0;

	if ( !self->botMind->behaviorTree )
	{
		Log::Warn( "NULL behavior tree" );
		return;
	}

	// always update the path corridor
	if ( self->botMind->goal.isValid() )
	{
		botRouteTarget_t routeTarget;
		BotTargetToRouteTarget( self, self->botMind->goal, &routeTarget );
		G_BotUpdatePath( self->s.number, &routeTarget, &self->botMind->m_nav );
	}

	self->botMind->willSprint( false ); //let the BT decide that
	AINodeStatus_t status =
		self->botMind->behaviorTree->run( self, ( AIGenericNode_t * ) self->botMind->behaviorTree );
	if ( traceClient.Get() == self->num() )
	{
		ShowRunningNode( self, status );
	}

	// if we were nudged...
	VectorAdd( self->client->ps.velocity, nudge, self->client->ps.velocity );

	// ensure we really want to sprint or not
	self->client->pers.cmd = self->botMind->cmdBuffer;
	self->botMind->doSprint(
			BG_Class( self->client->ps.stats[ STAT_CLASS ] )->staminaJumpCost,
			self->client->ps.stats[ STAT_STAMINA ],
			self->client->pers.cmd );
}

void G_BotSpectatorThink( gentity_t *self )
{
	char buf[MAX_STRING_CHARS];
	//hacky ping fix
	self->client->ps.ping = rand() % 50 + 50;

	//acknowledge recieved console messages
	//MUST be done
	while ( trap_BotGetServerCommand( self->num(), buf, sizeof( buf ) ) );

	if ( navMeshLoaded == navMeshStatus_t::GENERATING )
	{
		return;
	}

	self->botMind->spawnTime = level.time;
	self->botMind->myTimer = level.time;

	if ( self->client->ps.pm_flags & PMF_QUEUED )
	{
		//we're queued to spawn, all good
		//check for humans in the spawn que
		{
			spawnQueue_t *sq;
			if ( self->client->pers.team != TEAM_NONE )
			{
				sq = &level.team[ self->client->pers.team ].spawnQueue;
			}
			else
			{
				sq = nullptr;
			}

			if ( sq && PlayersBehindBotInSpawnQueue( self ) )
			{
				G_RemoveFromSpawnQueue( sq, self->s.number );
				G_PushSpawnQueue( sq, self->s.number );
			}
		}
		return;
	}

	G_Bot_ResetBehaviorState( *self->botMind );

	// Reset non-time-dependent alive state
	self->botMind->lastThink = -999999;
	self->botMind->stuckTime = 0;
	self->botMind->stuckPosition = {1.0e12f, 1.0e12f, 1.0e12f};
	self->botMind->futureAimTime = 0;
	self->botMind->futureAimTimeInterval = 0;
	BotResetEnemyQueue( &self->botMind->enemyQueue );
	self->botMind->enemyLastSeen = -999999;
	self->botMind->exhausted = false;

	//FIXME: duplicate of sg_cmds.cpp:883 function "void Cmd_Team_f( gentity_t * )"
	if ( g_doWarmup.Get() && ( ( level.warmupTime - level.time ) / 1000 ) > 0 )
	{
		return;
	}

	if ( self->client->sess.restartTeam == TEAM_NONE )
	{
		int teamnum = self->client->pers.team;

		// I think all that decision taking about birth... should go in BT.
		if ( teamnum == TEAM_HUMANS )
		{
			// TODO: use wp->canBuyNow() from sg_bot_util
			weapon_t weapon = WP_NONE;
			if ( g_bot_rifle.Get() )
			{
				weapon = WP_MACHINEGUN;
			}
			else if ( g_bot_ckit.Get() )
			{
				weapon = WP_HBUILD;
			}

			G_ScheduleSpawn( self->client, PCL_HUMAN_NAKED, weapon );
		}
		else if ( teamnum == TEAM_ALIENS )
		{
			G_ScheduleSpawn( self->client, PCL_ALIEN_LEVEL0 );
		}
	}
}

void G_BotIntermissionThink( gclient_t *client )
{
	client->readyToExit = true;
}

void G_BotSelectSpawnClass( gentity_t *self )
{
	if ( self->botMind->behaviorTree && self->botMind->behaviorTree->classSelectionTree )
	{
		BotEvaluateNode( self, self->botMind->behaviorTree->classSelectionTree );
	}

	G_BotSetNavMesh( self->num(), self->client->pers.classSelection );
}

// Initialization happens whenever someone first tries to add a bot.
// Assuming the meshes already exist, this incurs some delay (a few tenths of a second), but on
// servers bots are normally added at the beginning of the round so it shouldn't be noticeable.
//
// If the mesh does not already exist and g_bot_navgen_onDemand is 1, the function returns true
// and bots can join, but they don't spawn until the navmesh is finished.
//
// If the mesh does not exist and g_bot_navgen_onDemand is -1, the function will block and the
// server will freeze until the navmesh is done (usually tens of seconds).
bool G_BotInit()
{
	switch ( navMeshLoaded )
	{
	case navMeshStatus_t::GENERATING:
	case navMeshStatus_t::LOADED:
		return true;
	case navMeshStatus_t::LOAD_FAILED:
		Log::Warn( "Navmesh initialization previously failed, doing nothing" );
		return false;
	case navMeshStatus_t::UNINITIALIZED:
		break;
	}

	G_BotNavInit( generateNeededMesh.Get() );

	if ( navMeshLoaded != navMeshStatus_t::LOADED && navMeshLoaded != navMeshStatus_t::GENERATING )
	{
		Log::Notice( "Failed to load navmeshes" );
		return false;
	}
	return true;
}

void G_BotCleanup()
{
	for ( int i = 0; i < MAX_CLIENTS; ++i )
	{
		if ( level.clients[i].pers.connected != CON_DISCONNECTED && level.clients[i].pers.isBot )
		{
			G_BotDel( i );
		}
	}

	G_BotClearNames();

	FreeTreeList( &treeList );
	G_BotNavCleanup();
}

// add or remove bots to match team size targets set by 'bot fill' command
static void G_BotCheckDefaultFill()
{
	Util::optional<int> fillCount = g_bot_defaultFill.GetModifiedValue();
	if ( fillCount ) // if modified
	{
		int adjustedCount = Math::Clamp( *fillCount, 0, MAX_CLIENTS );
		// init bots if they aren't already and if we need to
		if ( adjustedCount != 0 && !G_BotInit() )
		{
			Log::Warn( "Navigation mesh files unavailable for this map" );
			return;
		}

		for ( int team = TEAM_NONE + 1; team < NUM_TEAMS; ++team )
		{
			level.team[team].botFillTeamSize = adjustedCount;
			level.team[team].botFillSkillLevel = 0; // default
		}
	}
}

void G_BotFill(bool immediately)
{
	static int nextCheck = 0;
	if (!immediately && level.time < nextCheck) {
		return;  // don't check every frame to prevent warning spam
	}

	if ( !immediately && level.inClient && g_clients[ 0 ].pers.connected < CON_CONNECTED )
	{
		// In case cg_navgenOnLoad is enabled, give that a chance to finish so
		// that we don't have two dueling navgens.
		return;
	}

	G_BotCheckDefaultFill();

	nextCheck = level.time + 2000;
	//FIXME this function can actually be called before bots had time to connect
	//  resulting in filling too many bots.
	if ( level.matchTime < 2000 )
	{
		return;
	}

	struct fill_t
	{
		std::vector<int> current; // list of filler bots
		int target; // if <0, too many bots, if >0, not enough
	} fillers[ NUM_TEAMS ] = {};
	int missingFillers = 0;

	for (int client = 0; client < MAX_CLIENTS; client++)
	{
		auto& pers = level.clients[client].pers;
		if ( pers.connected == CON_CONNECTED && pers.isFillerBot )
		{
			fillers[ pers.team ].current.push_back( client );
		}
	}

	for ( team_t team : {TEAM_ALIENS, TEAM_HUMANS} )
	{
		auto& fill = fillers[team];
		fill.target = level.team[team].botFillTeamSize - level.team[team].numClients;
		// remove excedent
		while ( fill.target < 0 && fill.current.size() > 0 )
		{
			G_BotDel( fill.current.back() );
			fill.current.pop_back();
			fill.target++;
		}
		// remember how much bots are missing
		if ( fill.target > 0 )
		{
			missingFillers += fill.target;
		}
	}

	const std::string behaviorString = g_bot_defaultBehavior.Get();
	while ( missingFillers )
	{
		for ( team_t team : {TEAM_ALIENS, TEAM_HUMANS} )
		{
			if ( fillers[ team ].target > 0 )
			{
				fillers[ team ].target--;
				if ( !G_BotAdd( BOT_NAME_FROM_LIST, team, level.team[team].botFillSkillLevel, behaviorString.c_str(), true ) )
				{
					//TODO modify the "/bot fill" number so that further warnings will be ignored.
					//     Note that this means players disconnecting would possibly not be replaced by bot
					//     so this might be more a problem than an improvement.
					//TODO if number of clients in both teams is unevent, this is unfair, so kick one of other team.
					//     to do this, it would be needed to add the new client to fillers[team].current
					return;
				}
				--missingFillers;
			}
		}
	}
}

// declares an intent to sprint or not, should be used by BT functions
void botMemory_t::willSprint( bool enable )
{
	wantSprinting = enable;
}

// applies the sprint intent, should only be called after all directional
// or speed intents have been decided, that is, in G_BotThink(), *after*
// the BT was examined.
// This currently implements an hysteresis to prevent smart bot to be so
// exhausted that they can't jump over an obstacle.
// The hysteresis is meant to allow to recharge enough stamina so that a
// sprint can be useful, instead of constantly enabling sprint, which in
// practice result is the "moonwalk bug": AIs moving slower than if just
// walking, and stamina not recharging at all.
void botMemory_t::doSprint( int jumpCost, int stamina, usercmd_t& cmd )
{
	exhausted = exhausted || ( skillLevel >= 5 && stamina <= jumpCost + jumpCost / 10 );
	if ( !exhausted && wantSprinting )
	{
		usercmdPressButton( cmd.buttons, BTN_SPRINT );
	}
	else
	{
		usercmdReleaseButton( cmd.buttons, BTN_SPRINT );
	}

	exhausted = exhausted && stamina <= jumpCost * 2;
}

// TODO: also reset state stored in BT nodes
void G_Bot_ResetBehaviorState( botMemory_t &memory )
{
	memory.currentNode = nullptr;
	memory.runningNodes.clear();
	memory.goal.clear();
	memory.clearNav();
	memory.lastNavconTime = 0;
	memory.lastNavconDistance = 0;
	memory.hasOffmeshGoal = false;
}

// assumes bot is a bot, otherwise will crash.
static std::string BotGoalToString( gentity_t *bot )
{
	const botTarget_t& target = bot->botMind->goal;
	if ( !target.isValid() )
	{
		return "<invalid>";
	}

	if ( target.targetsValidEntity() )
	{
		return etos( target.getTargetedEntity() );
	}
	else if ( target.targetsCoordinates() )
	{
		return vtos( GLM4READ( target.getPos() ) );
	}

	return "<unknown goal>";
}

std::string G_BotToString( gentity_t *bot )
{
	ASSERT( bot->inuse && bot->client->pers.isBot );

	return Str::Format( "^*%s^*: %s [b=%s g=%s s=%d ss=\"%s\"]",
			bot->client->pers.netname,
			BG_TeamName( G_Team( bot ) ),
			bot->botMind->behaviorTree->name,
			BotGoalToString( bot ),
			bot->botMind->skillLevel,
			bot->botMind->skillSetExplaination );
}
