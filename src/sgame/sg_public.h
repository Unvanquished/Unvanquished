/*
===========================================================================

Unvanquished GPL Source Code
Copyright (C) 2012-2013 Unvanquished Developers

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

===========================================================================
*/

#ifndef SG_PUBLIC_H_
#define SG_PUBLIC_H_

// sg_active.c
void              G_UnlaggedStore();
void              G_UnlaggedClear( gentity_t *ent );
void              G_UnlaggedCalc( int time, gentity_t *skipEnt );
void              G_UnlaggedOn( gentity_t *attacker, glm::vec3 const& muzzle, float range );
void              G_UnlaggedOff();
void              ClientThink( int clientNum );
void              ClientEndFrame( gentity_t *ent );
void              G_RunClient( gentity_t *ent );
void              G_TouchTriggers( gentity_t *ent );

// sg_admin.c
const char        *G_admin_name( gentity_t *ent );
const char        *G_quoted_admin_name( gentity_t *ent );

// Beacon.cpp
namespace Beacon
{
	void Frame();
	void Move( gentity_t *ent, glm::vec3 const& origin );
	gentity_t *New( glm::vec3 const& origin, beaconType_t type, int data, team_t team,
	                int owner = ENTITYNUM_NONE,
	                beaconConflictHandler_t conflictHandler = BCH_NONE );
	gentity_t *NewArea( beaconType_t type, glm::vec3 const& point, team_t team );
	void Delete( gentity_t *ent, bool verbose = false );
	glm::vec3 MoveTowardsRoom( glm::vec3 const& origin );
	gentity_t *MoveSimilar( glm::vec3 const& from, glm::vec3 const& to, beaconType_t type, int data,
	                        int team, int owner, float radius = 128.0f, int eFlags = 0,
	                        int eFlagsRelevant = 0 );
	void Propagate( gentity_t *ent );
	void PropagateAll();
	void RemoveOrphaned( int clientNum );
	bool EntityTaggable( int num, team_t team, bool trace );
	gentity_t *TagTrace( glm::vec3 const& begin, glm::vec3 const& end, int skip, int mask, team_t team, bool refreshTagged );
	void Tag( gentity_t *ent, team_t team, bool permanent );
	void UpdateTags( gentity_t *ent );
	void DetachTags( gentity_t *ent );
	void DeleteTags( gentity_t *ent );
}

// sg_buildable.c
bool              G_IsWarnableMOD(meansOfDeath_t mod);
gentity_t         *G_Overmind();
gentity_t         *G_ActiveOvermind();
gentity_t         *G_Reactor();
gentity_t         *G_ActiveReactor();
gentity_t         *G_MainBuildable(team_t team);
gentity_t         *G_ActiveMainBuildable(team_t team);
int               G_GetBaseStatusCode(team_t team);
float             G_DistanceToBase(gentity_t *self);
bool              G_InsideBase(gentity_t *self);
bool              G_DretchCanDamageEntity( const gentity_t *self, const gentity_t *other );
bool              G_BuildableInRange( glm::vec3 const& origin, float radius, buildable_t buildable );
gentity_t         *G_GetDeconstructibleBuildable( gentity_t *ent );
bool              G_DeconstructDead( gentity_t *buildable );
void              G_DeconstructUnprotected( gentity_t *buildable, gentity_t *ent );
bool              G_CheckDeconProtectionAndWarn( gentity_t *buildable, gentity_t *player );
itemBuildError_t  G_CanBuild( gentity_t *ent, buildable_t buildable, int distance, glm::vec3& origin, glm::vec3& normal, int *groundEntNum );
bool              G_BuildIfValid( gentity_t *ent, buildable_t buildable );
void              G_SetBuildableAnim(gentity_t *ent, buildableAnimNumber_t animation, bool force);
void              G_SetIdleBuildableAnim(gentity_t *ent, buildableAnimNumber_t animation);
void              G_SpawnBuildable(gentity_t *ent, buildable_t buildable);
void              G_LayoutSave( const char *name );
int               G_LayoutList( const char *map, char *list, int len );
bool G_LayoutExists( Str::StringRef map, Str::StringRef layout );
void              G_LayoutSelect();
void              G_LayoutLoad();
void              G_BaseSelfDestruct( team_t team );
buildLog_t        *G_BuildLogNew( gentity_t *actor, buildFate_t fate );
void              G_BuildLogSet( buildLog_t *log, gentity_t *ent );
void              G_BuildLogAuto( gentity_t *actor, gentity_t *buildable, buildFate_t fate );
void              G_BuildLogRevert( int id );
int               G_GetBuildDuration( const gentity_t* ent );
void              G_UpdateBuildablePowerStates();
void              G_BuildableTouchTriggers( gentity_t *ent );

// TODO: Convert these functions to component methods.
void ABarricade_Shrink( gentity_t *self, bool shrink );

// sg_buildpoints
float             G_RGSPredictEfficiencyDelta( glm::vec3 const& origin, team_t team );
void              G_UpdateBuildPointBudgets();
void              G_RecoverBuildPoints();
int               G_GetFreeBudget(team_t team);
int               G_GetMarkedBudget(team_t team);
int               G_GetSpendableBudget(team_t team);
void              G_FreeBudget(team_t team, int immediateAmount , int queuedAmount);
void              G_SpendBudget(team_t team, int amount);
int               G_BuildableDeconValue(gentity_t *ent);
void              G_GetTotalBuildableValues(int *buildableValuesByTeam);

// sg_client.c
void              G_AddCreditToClient( gclient_t *client, short credit, bool cap );
void              G_SetClientViewAngle( gentity_t *ent, glm::vec3 const& angle );
gentity_t         *G_SelectUnvanquishedSpawnPoint( team_t team, glm::vec3 const& preference, glm::vec3& origin, glm::vec3& angles );
gentity_t         *G_SelectRandomFurthestSpawnPoint( glm::vec3 const& avoidPoint, glm::vec3& origin, glm::vec3& angles );
gentity_t         *G_SelectLockSpawnPoint( glm::vec3& origin, glm::vec3& angles , team_t team );
void G_SelectSpectatorSpawnPoint( glm::vec3& origin, glm::vec3& angles );
void              respawn( gentity_t *ent );
void              ClientSpawn( gentity_t *ent, gentity_t *spawn, glm::vec3 const* origin, glm::vec3 const* angles );
bool          G_IsUnnamed( const char *name );
const char        *ClientConnect( int clientNum, bool firstTime );
const char        *ClientBotConnect( int clientNum, bool firstTime, team_t team );
const char        *ClientUserinfoChanged( int clientNum, bool forceName );
void              ClientDisconnect( int clientNum );
void              ClientBegin( int clientNum );
void              ClientAdminChallenge( int clientNum );

// sg_clustering.c
namespace BaseClustering {
	void Init();
	void Update(gentity_t *beacon);
	void Remove(gentity_t *beacon);
	void Debug();
}

// sg_cmds.c
void              G_StopFollowing( gentity_t *ent );
void              G_StopFromFollowing( gentity_t *ent );
void              G_FollowLockView( gentity_t *ent );
bool          G_FollowNewClient( gentity_t *ent, int dir );
void              G_ToggleFollow( gentity_t *ent );
int               G_MatchOnePlayer( const int *plist, int found, char *err, int len );
int               G_ClientNumberFromString( const char *s, char *err, int len );
int               G_ClientNumbersFromString( const char *s, int *plist, int max );
char              *ConcatArgs( int start );
char              *ConcatArgsPrintable( int start );
void              G_Say( gentity_t *ent, saymode_t mode, const char *chatText );
void              G_UnEscapeString( const char *in, char *out, int len );
void              G_SanitiseString( const char *in, char *out, int len );
void              Cmd_PrivateMessage_f( gentity_t *ent );
void              Cmd_AdminMessage_f( gentity_t *ent );
int               G_FloodLimited( gentity_t *ent );
bool          G_CheckStopVote( team_t );
bool          G_RoomForClassChange( gentity_t *ent, class_t pcl, glm::vec3& newOrigin );
bool          G_ScheduleSpawn( gclient_t *client, class_t class_, weapon_t humanItem = WP_NONE );
bool          G_AlienEvolve( gentity_t *ent, class_t newClass, bool report, bool dryRun );
void              ScoreboardMessage( gentity_t *client );
void              ClientCommand( int clientNum );
void              G_ClearRotationStack();
void              G_MapLog_NewMap();
void              G_MapLog_Result( char result );
void              Cmd_MapLog_f( gentity_t *ent );

// sg_combat.c
bool          G_CanDamage( gentity_t *targ, glm::vec3 const& origin );
bool          G_RadiusDamage( glm::vec3 const& origin, gentity_t *attacker, float damage, float radius, gentity_t *ignore, int dflags, meansOfDeath_t mod, team_t testHit = TEAM_NONE );
bool          G_SelectiveRadiusDamage( glm::vec3 const& origin, gentity_t *attacker, float damage, float radius, gentity_t *ignore, meansOfDeath_t mod, int ignoreTeam );
void              G_RewardAttackers( gentity_t *self );
void              G_AddCreditsToScore( gentity_t *self, int credits );
void              G_AddMomentumToScore( gentity_t *self, float momentum );
void              G_LogDestruction( gentity_t *self, gentity_t *actor, meansOfDeath_t mod );
void              G_InitDamageLocations();
void              G_PlayerDie( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, meansOfDeath_t mod );

// sg_momentum.c
void              G_DecreaseMomentum();
float             G_AddMomentumGeneric( team_t team, float amount );
float             G_AddMomentumGenericStep( team_t team, float amount );
float             G_PredictMomentumForBuilding( gentity_t *buildable );
float             G_AddMomentumForBuilding( gentity_t *buildable );
float             G_RemoveMomentumForDecon( gentity_t *buildable, gentity_t *deconner );
float             G_AddMomentumForKillingStep( gentity_t *victim, gentity_t *attacker, float share );
float             G_AddMomentumForDestroyingStep( gentity_t *buildable, gentity_t *attacker, float amount );
void              G_AddMomentumEnd();

// sg_main.c
void              G_InitSpawnQueue( spawnQueue_t *sq );
int               G_GetSpawnQueueLength( spawnQueue_t *sq );
int               G_PopSpawnQueue( spawnQueue_t *sq );
int               G_PeekSpawnQueue( spawnQueue_t *sq );
bool          G_SearchSpawnQueue( spawnQueue_t *sq, int clientNum );
bool          G_PushSpawnQueue( spawnQueue_t *sq, int clientNum );
bool          G_RemoveFromSpawnQueue( spawnQueue_t *sq, int clientNum );
int               G_GetPosInSpawnQueue( spawnQueue_t *sq, int clientNum );
void              G_PrintSpawnQueue( spawnQueue_t *sq );
void              BeginIntermission();
void              MoveClientToIntermission( gentity_t *client );
void              G_MapConfigs( Str::StringRef mapname, Str::StringRef layout );
void              CalculateRanks();
void              FindIntermissionPoint();
void              G_RunThink( gentity_t *ent );
void              G_AdminMessage( gentity_t *ent, const char *string );
void              G_LogPrintf( const char *fmt, ... ) PRINTF_LIKE(1);
void              SendScoreboardMessageToAllClients();
void              G_Vote( gentity_t *ent, team_t team, bool voting );
void              G_ResetVote( team_t team );
void              G_ExecuteVote( team_t team );
void              G_CheckVote( team_t team );
void              LogExit( const char *string );
void              G_InitGame( int levelTime, int randomSeed, bool inClient );
void              G_RunFrame( int levelTime );
void              G_ShutdownGame( int restart );
void              G_CheckPmoveParamChanges();
void              G_SendClientPmoveParams(int client);
void              G_PrepareEntityNetCode();
Str::StringRef G_NextMapCommand();

// sg_maprotation.c
void              G_PrintRotations();
void              G_PrintCurrentRotation( gentity_t *ent );
void              G_AdvanceMapRotation( int depth );
bool          G_StartMapRotation( const char *name, bool advance, bool putOnStack, bool reset_index, int depth );
void              G_StopMapRotation();
bool          G_MapRotationActive();
void              G_InitMapRotations();
void              G_ShutdownMapRotations();
bool          G_MapExists( const char *name );

// sg_missile.c
void              G_ExplodeMissile( gentity_t *ent );
void              G_RunMissile( gentity_t *ent );
gentity_t         *G_SpawnMissile( missile_t missile, gentity_t *parent, glm::vec3 const& start, glm::vec3 const& dir, gentity_t *target, void ( *think )( gentity_t *self ), int nextthink );
gentity_t         *G_SpawnFire( glm::vec3 const& origin, glm::vec3 const& normal, gentity_t const* fireStarter );

// sg_namelog.c
void              G_namelog_connect( gclient_t *client );
void              G_namelog_disconnect( gclient_t *client );
void              G_namelog_restore( gclient_t *client );
void              G_namelog_update_score( gclient_t *client );
void              G_namelog_update_name( gclient_t *client );
void              G_namelog_cleanup();

// sg_physcis.c
void              G_Physics( gentity_t *ent );

// sg_session.c
void              G_ReadSessionData( gclient_t *client );
void              G_InitSessionData( gclient_t *client, const char *userinfo );
void              G_WriteSessionData();

// sg_svcmds.c
bool          ConsoleCommand();
void              G_RegisterCommands();
void              G_UnregisterCommands();

// sg_team.c
team_t            G_TeamFromString( const char *str );
void              G_TeamCommand( team_t team, const char *cmd );
void              G_AreaTeamCommand( const gentity_t *ent, const char *cmd );
team_t            G_Team( const gentity_t *ent );
bool              G_OnSameTeam( const gentity_t *ent1, const gentity_t *ent2 );
void              G_LeaveTeam( gentity_t *self );
void              G_ChangeTeam( gentity_t *ent, team_t newTeam );
gentity_t         *GetCloseLocationEntity( gentity_t *ent );
void              TeamplayInfoMessage( gentity_t *ent );
int               G_PlayerCountForBalance( team_t team );
void              CheckTeamStatus();
void              G_UpdateTeamConfigStrings();

// sg_utils.c
bool          G_AddressParse( const char *str, addr_t *addr );
bool          G_AddressCompare( const addr_t *a, const addr_t *b );
int               G_ParticleSystemIndex( const char *name );
int               G_ModelIndex( std::string const& name );
int               G_SoundIndex( const char *name );
int               G_GradingTextureIndex( const char *name );
int               G_ReverbEffectIndex( const char *name );
int               G_LocationIndex( const char *name );
void              G_KillBox( gentity_t *ent );
void              G_TeleportPlayer( gentity_t *player, vec3_t origin, vec3_t angles, float speed );
void              G_Sound( gentity_t *ent, soundChannel_t channel, int soundIndex );
char              *vtos( const vec3_t v );
void              G_AddPredictableEvent( gentity_t *ent, int event, int eventParm );
void              G_AddEvent( gentity_t *ent, int event, int eventParm );
void              G_BroadcastEvent( int event, int eventParm, team_t team );
bool          G_ClientIsLagging( gclient_t *client );
void              G_TriggerMenu( int clientNum, dynMenu_t menu );
void              G_TriggerMenuArgs( int clientNum, dynMenu_t menu, int arg );
void              G_CloseMenus( int clientNum );
void              G_ClientnumToMask( int clientNum, int *loMask, int *hiMask );
void              G_TeamToClientmask( team_t team, int *loMask, int *hiMask );
bool          G_LineOfSight( const gentity_t *from, const gentity_t *to, int mask, bool useTrajBase );
bool          G_LineOfSight( const gentity_t *from, const gentity_t *to );
bool          G_LineOfFire( const gentity_t *from, const gentity_t *to );
bool          G_LineOfSight( const vec3_t point1, const vec3_t point2 );
bool              G_IsPlayableTeam( team_t team );
bool              G_IsPlayableTeam( int team );
team_t            G_IterateTeams( team_t team );
std::string G_EscapeServerCommandArg( Str::StringRef str );
char *Quote( Str::StringRef str );
float             G_Distance( gentity_t *ent1, gentity_t *ent2 );
float G_DistanceToBBox( const vec3_t origin, gentity_t* ent );
int BG_FOpenGameOrPakPath( Str::StringRef filename, fileHandle_t &handle );
bool G_IsOnFire( const gentity_t *ent );

// sg_weapon.c
void              G_ForceWeaponChange( gentity_t *ent, weapon_t weapon );
void              G_GiveMaxAmmo( gentity_t *self );
bool          G_RefillAmmo( gentity_t *self, bool triggerEvent );
bool          G_RefillFuel( gentity_t *self, bool triggerEvent );
bool          G_FindAmmo( gentity_t *self );
bool          G_FindFuel( gentity_t *self );
void              G_CalcMuzzlePoint( const gentity_t *self, const vec3_t forward, const vec3_t right, const vec3_t up, vec3_t muzzlePoint );
glm::vec3         G_CalcMuzzlePoint( const gentity_t *self, const glm::vec3 &forward );
void              G_SnapVectorTowards( vec3_t v, vec3_t to );
bool              G_CheckDretchAttack( gentity_t *self );
bool              G_CheckPounceAttack( gentity_t *self );
void              G_CheckCkitRepair( gentity_t *self );
void              G_ChargeAttack( gentity_t *self, gentity_t *victim );
void              G_ImpactAttack( gentity_t *self, gentity_t *victim );
void              G_WeightAttack( gentity_t *self, gentity_t *victim );
void              G_UpdateZaps( int msec );
void              G_ClearPlayerZapEffects( gentity_t *player );
void              G_FireWeapon( gentity_t *self, weapon_t weapon, weaponMode_t weaponMode );
void              G_FireUpgrade( gentity_t *self, upgrade_t upgrade );

// CombatFeedback.cpp
namespace CombatFeedback
{
  void HitNotify(gentity_t *attacker, gentity_t *victim, Util::optional<glm::vec3> point, float damage, meansOfDeath_t mod, bool lethal);
}

void Cmd_Reload_f( gentity_t *ent );

#endif // SG_PUBLIC_H_
