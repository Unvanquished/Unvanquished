/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 2012-2013 Unvanquished Developers

This file is part of the Daemon GPL Source Code (Daemon Source Code).

Daemon Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Daemon Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon Source Code.  If not, see <http://www.gnu.org/licenses/>.

===========================================================================
*/

#ifndef G_PUBLIC_H_
#define G_PUBLIC_H_

// g_active.c
void              G_UnlaggedStore( void );
void              G_UnlaggedClear( gentity_t *ent );
void              G_UnlaggedCalc( int time, gentity_t *skipEnt );
void              G_UnlaggedOn( gentity_t *attacker, vec3_t muzzle, float range );
void              G_UnlaggedOff( void );
void              ClientThink( int clientNum );
void              ClientEndFrame( gentity_t *ent );
void              G_RunClient( gentity_t *ent );
void              G_TouchTriggers( gentity_t *ent );

// g_admin.c
const char        *G_admin_name( gentity_t *ent );
const char        *G_quoted_admin_name( gentity_t *ent );

// g_buildable.c
gentity_t         *G_CheckSpawnPoint( int spawnNum, const vec3_t origin, const vec3_t normal, buildable_t spawn, vec3_t spawnOrigin );
gentity_t         *G_Reactor( void );
gentity_t         *G_AliveReactor( void );
gentity_t         *G_ActiveReactor( void );
gentity_t         *G_Overmind( void );
gentity_t         *G_AliveOvermind( void );
gentity_t         *G_ActiveOvermind( void );
float             G_DistanceToBase( gentity_t *self, qboolean ownBase );
qboolean          G_InsideBase( gentity_t *self, qboolean ownBase );
qboolean          G_FindCreep( gentity_t *self );
gentity_t         *G_Build( gentity_t *builder, buildable_t buildable, const vec3_t origin, const vec3_t normal, const vec3_t angles, int groundEntityNum );
float             G_RGSPredictEfficiency( vec3_t origin );
float             G_RGSPredictEfficiencyDelta( vec3_t origin, team_t team );
void              G_BuildableThink( gentity_t *ent, int msec );
qboolean          G_BuildableInRange( vec3_t origin, float radius, buildable_t buildable );
void              G_IgniteBuildable( gentity_t *self, gentity_t *fireStarter );
void              G_Deconstruct( gentity_t *self, gentity_t *deconner, meansOfDeath_t deconType );
itemBuildError_t  G_CanBuild( gentity_t *ent, buildable_t buildable, int distance, vec3_t origin, vec3_t normal, int *groundEntNum );
qboolean          G_BuildIfValid( gentity_t *ent, buildable_t buildable );
void              G_SetBuildableAnim( gentity_t *ent, buildableAnimNumber_t anim, qboolean force );
void              G_SetIdleBuildableAnim( gentity_t *ent, buildableAnimNumber_t anim );
void              G_SpawnBuildable(gentity_t *ent, buildable_t buildable);
void              G_LayoutSave( const char *name );
int               G_LayoutList( const char *map, char *list, int len );
void              G_LayoutSelect( void );
void              G_LayoutLoad( void );
void              G_BaseSelfDestruct( team_t team );
int               G_GetBuildPointsInt( team_t team );
int               G_GetMarkedBuildPointsInt( team_t team );
buildLog_t        *G_BuildLogNew( gentity_t *actor, buildFate_t fate );
void              G_BuildLogSet( buildLog_t *log, gentity_t *ent );
void              G_BuildLogAuto( gentity_t *actor, gentity_t *buildable, buildFate_t fate );
void              G_BuildLogRevert( int id );
qboolean          G_CanAffordBuildPoints( team_t team, float amount );
void              G_ModifyBuildPoints( team_t team, float amount );
void              G_GetBuildableResourceValue( int *teamValue );
void              G_SetHumanBuildablePowerState();

// g_client.c
void              G_AddCreditToClient( gclient_t *client, short credit, qboolean cap );
void              G_SetClientViewAngle( gentity_t *ent, const vec3_t angle );
gentity_t         *G_SelectUnvanquishedSpawnPoint( team_t team, vec3_t preference, vec3_t origin, vec3_t angles );
gentity_t         *G_SelectRandomFurthestSpawnPoint( vec3_t avoidPoint, vec3_t origin, vec3_t angles );
gentity_t         *G_SelectLockSpawnPoint( vec3_t origin, vec3_t angles , char const* intermission );
gentity_t         *G_SelectAlienLockSpawnPoint( vec3_t origin, vec3_t angles );
gentity_t         *G_SelectHumanLockSpawnPoint( vec3_t origin, vec3_t angles );
gentity_t         *G_SelectSpectatorSpawnPoint( vec3_t origin, vec3_t angles );
void              respawn( gentity_t *ent );
void              ClientSpawn( gentity_t *ent, gentity_t *spawn, const vec3_t origin, const vec3_t angles );
qboolean          SpotWouldTelefrag( gentity_t *spot );
qboolean          G_IsUnnamed( const char *name );
char              *ClientConnect( int clientNum, qboolean firstTime );
char              *ClientBotConnect( int clientNum, qboolean firstTime, team_t team );
char              *ClientUserinfoChanged( int clientNum, qboolean forceName );
void              ClientDisconnect( int clientNum );
void              ClientBegin( int clientNum );
void              ClientAdminChallenge( int clientNum );

// g_cmds.c
void              G_StopFollowing( gentity_t *ent );
void              G_StopFromFollowing( gentity_t *ent );
void              G_FollowLockView( gentity_t *ent );
qboolean          G_FollowNewClient( gentity_t *ent, int dir );
void              G_ToggleFollow( gentity_t *ent );
int               G_MatchOnePlayer( const int *plist, int found, char *err, int len );
int               G_ClientNumberFromString( const char *s, char *err, int len );
int               G_ClientNumbersFromString( const char *s, int *plist, int max );
char              *ConcatArgs( int start );
char              *ConcatArgsPrintable( int start );
void              G_Say( gentity_t *ent, saymode_t mode, const char *chatText );
void              G_DecolorString( const char *in, char *out, int len );
void              G_UnEscapeString( const char *in, char *out, int len );
void              G_SanitiseString( const char *in, char *out, int len );
void              Cmd_PrivateMessage_f( gentity_t *ent );
void              Cmd_ListMaps_f( gentity_t *ent );
void              Cmd_Test_f( gentity_t *ent );
void              Cmd_AdminMessage_f( gentity_t *ent );
int               G_FloodLimited( gentity_t *ent );
void              G_ListCommands( gentity_t *ent );
void              G_LoadCensors( void );
void              G_CensorString( char *out, const char *in, int len, gentity_t *ent );
qboolean          G_CheckStopVote( team_t );
qboolean          G_RoomForClassChange( gentity_t *ent, class_t pcl, vec3_t newOrigin );
void              ScoreboardMessage( gentity_t *client );
void              ClientCommand( int clientNum );
void              G_ClearRotationStack( void );
void              G_MapLog_NewMap( void );
void              G_MapLog_Result( char result );
void              Cmd_MapLog_f( gentity_t *ent );

// g_combat.c
qboolean          G_CanDamage( gentity_t *targ, vec3_t origin );
void              G_KnockbackByDir( gentity_t *target, const vec3_t direction, float strength, qboolean ignoreMass );
void              G_KnockbackBySource( gentity_t *target, gentity_t *source, float strength, qboolean ignoreMass );
void              G_Damage( gentity_t *targ, gentity_t *inflictor, gentity_t *attacker, vec3_t dir, vec3_t point, int damage, int dflags, int mod );
void              G_SelectiveDamage( gentity_t *targ, gentity_t *inflictor, gentity_t *attacker, vec3_t dir, vec3_t point, int damage, int dflags, int mod, int team );
qboolean          G_RadiusDamage( vec3_t origin, gentity_t *attacker, float damage, float radius, gentity_t *ignore, int mod );
qboolean          G_SelectiveRadiusDamage( vec3_t origin, gentity_t *attacker, float damage, float radius, gentity_t *ignore, int mod, int ignoreTeam );
void              G_RewardAttackers( gentity_t *self );
void              G_AddCreditsToScore( gentity_t *self, int credits );
void              G_AddMomentumToScore( gentity_t *self, float momentum );
void              G_LogDestruction( gentity_t *self, gentity_t *actor, int mod );
float             G_GetNonLocDamageMod( class_t pcl );
float             G_GetPointDamageMod( gentity_t *target, class_t pcl, float angle, float height );
void              G_InitDamageLocations( void );
void              G_PlayerDie( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int mod );

// g_momentum.c
void              G_DecreaseMomentum( void );
float             G_AddMomentumGeneric( team_t team, float amount );
float             G_AddMomentumGenericStep( team_t team, float amount );
float             G_PredictMomentumForBuilding( gentity_t *buildable );
float             G_AddMomentumForBuilding( gentity_t *buildable );
float             G_RemoveMomentumForDecon( gentity_t *buildable, gentity_t *deconner );
float             G_AddMomentumForKillingStep( gentity_t *victim, gentity_t *attacker, float share );
float             G_AddMomentumForDestroyingStep( gentity_t *buildable, gentity_t *attacker, float share );
void              G_AddMomentumEnd( void );

// g_main.c
void              G_InitSpawnQueue( spawnQueue_t *sq );
int               G_GetSpawnQueueLength( spawnQueue_t *sq );
int               G_PopSpawnQueue( spawnQueue_t *sq );
int               G_PeekSpawnQueue( spawnQueue_t *sq );
qboolean          G_SearchSpawnQueue( spawnQueue_t *sq, int clientNum );
qboolean          G_PushSpawnQueue( spawnQueue_t *sq, int clientNum );
qboolean          G_RemoveFromSpawnQueue( spawnQueue_t *sq, int clientNum );
int               G_GetPosInSpawnQueue( spawnQueue_t *sq, int clientNum );
void              G_PrintSpawnQueue( spawnQueue_t *sq );
void              BeginIntermission( void );
void              MoveClientToIntermission( gentity_t *client );
void              G_MapConfigs( const char *mapname );
void              CalculateRanks( void );
void              FindIntermissionPoint( void );
void              G_RunThink( gentity_t *ent );
void              G_AdminMessage( gentity_t *ent, const char *string );
void QDECL        G_LogPrintf( const char *fmt, ... ) PRINTF_LIKE(1);
void              SendScoreboardMessageToAllClients( void );
void QDECL        G_Printf( const char *fmt, ... ) PRINTF_LIKE(1);
void QDECL NORETURN G_Error( const char *fmt, ... ) PRINTF_LIKE(1);
void              G_Vote( gentity_t *ent, team_t team, qboolean voting );
void              G_ResetVote( team_t team );
void              G_ExecuteVote( team_t team );
void              G_CheckVote( team_t team );
void              LogExit( const char *string );
void              G_InitGame( int levelTime, int randomSeed, int restart );
void              G_RunFrame( int levelTime );
void              G_ShutdownGame( int restart );
vmCvar_t          *G_FindCvar( const char *name );

// g_maprotation.c
void              G_PrintRotations( void );
void              G_PrintCurrentRotation( gentity_t *ent );
void              G_AdvanceMapRotation( int depth );
qboolean          G_StartMapRotation( const char *name, qboolean advance, qboolean putOnStack, qboolean reset_index, int depth );
void              G_StopMapRotation( void );
qboolean          G_MapRotationActive( void );
void              G_InitMapRotations( void );
void              G_ShutdownMapRotations( void );
qboolean          G_MapExists( const char *name );

// g_missile.c
void              G_ExplodeMissile( gentity_t *ent );
void              G_RunMissile( gentity_t *ent );
gentity_t         *G_SpawnMissile( missile_t missile, gentity_t *parent, vec3_t start, vec3_t dir, gentity_t *target, void ( *think )( gentity_t *self ), int nextthink );

// g_namelog.c
void              G_namelog_connect( gclient_t *client );
void              G_namelog_disconnect( gclient_t *client );
void              G_namelog_restore( gclient_t *client );
void              G_namelog_update_score( gclient_t *client );
void              G_namelog_update_name( gclient_t *client );
void              G_namelog_cleanup( void );

// g_physcis.c
void              G_Physics( gentity_t *ent, int msec );

// g_session.c
void              G_ReadSessionData( gclient_t *client );
void              G_InitSessionData( gclient_t *client, const char *userinfo );
void              G_WriteSessionData( void );

// g_svcmds.c
qboolean          ConsoleCommand( void );
void              G_RegisterCommands( void );
void              G_UnregisterCommands( void );

// g_team.c
team_t            G_TeamFromString( const char *str );
void              G_TeamCommand( team_t team, const char *cmd );
void              G_AreaTeamCommand( gentity_t *ent, const char *cmd );
qboolean          G_OnSameTeam( gentity_t *ent1, gentity_t *ent2 );
void              G_LeaveTeam( gentity_t *self );
void              G_ChangeTeam( gentity_t *ent, team_t newTeam );
gentity_t         *Team_GetLocation( gentity_t *ent );
void              TeamplayInfoMessage( gentity_t *ent );
void              CheckTeamStatus( void );
void              G_UpdateTeamConfigStrings( void );

// g_utils.c
qboolean          G_AddressParse( const char *str, addr_t *addr );
qboolean          G_AddressCompare( const addr_t *a, const addr_t *b );
int               G_ParticleSystemIndex( const char *name );
int               G_ShaderIndex( const char *name );
int               G_ModelIndex( const char *name );
int               G_SoundIndex( const char *name );
int               G_GradingTextureIndex( const char *name );
int               G_ReverbEffectIndex( const char *name );
int               G_LocationIndex( const char *name );
void              G_KillBox( gentity_t *ent );
void              G_KillBrushModel( gentity_t *ent, gentity_t *activator );
void              G_TeleportPlayer( gentity_t *player, vec3_t origin, vec3_t angles, float speed );
void              G_Sound( gentity_t *ent, int channel, int soundIndex );
char              *G_CopyString( const char *str );
char              *vtos( const vec3_t v );
void              G_AddPredictableEvent( gentity_t *ent, int event, int eventParm );
void              G_AddEvent( gentity_t *ent, int event, int eventParm );
void              G_BroadcastEvent( int event, int eventParm, team_t team );
void              G_SetShaderRemap( const char *oldShader, const char *newShader, float timeOffset );
const char        *BuildShaderStateConfig( void );
qboolean          G_ClientIsLagging( gclient_t *client );
void              G_TriggerMenu( int clientNum, dynMenu_t menu );
void              G_TriggerMenuArgs( int clientNum, dynMenu_t menu, int arg );
void              G_CloseMenus( int clientNum );
void              G_TeamToClientmask( team_t team, int *loMask, int *hiMask );
void              G_FireThink( gentity_t *self );
gentity_t         *G_SpawnFire(vec3_t origin, vec3_t normal, gentity_t *fireStarter );
qboolean          G_LineOfSight( gentity_t *ent1, gentity_t *ent2 );
int               G_Heal( gentity_t *self, int amount );

// g_weapon.c
void              G_ForceWeaponChange( gentity_t *ent, weapon_t weapon );
void              G_GiveMaxAmmo( gentity_t *self );
qboolean          G_RefillAmmo( gentity_t *self, qboolean triggerEvent );
qboolean          G_RefillFuel( gentity_t *self, qboolean triggerEvent );
qboolean          G_FindAmmo( gentity_t *self );
qboolean          G_FindFuel( gentity_t *self );
void              G_CalcMuzzlePoint( gentity_t *self, vec3_t forward, vec3_t right, vec3_t up, vec3_t muzzlePoint );
void              G_SnapVectorTowards( vec3_t v, vec3_t to );
qboolean          G_CheckVenomAttack( gentity_t *self );
qboolean          G_CheckPounceAttack( gentity_t *self );
void              G_CheckCkitRepair( gentity_t *self );
void              G_ChargeAttack( gentity_t *self, gentity_t *victim );
void              G_ImpactAttack( gentity_t *self, gentity_t *victim );
void              G_WeightAttack( gentity_t *self, gentity_t *victim );
void              G_UpdateZaps( int msec );
void              G_ClearPlayerZapEffects( gentity_t *player );
void              G_FireWeapon( gentity_t *self, weapon_t weapon, weaponMode_t weaponMode );
void              G_FireUpgrade( gentity_t *self, upgrade_t upgrade );

#endif // G_PUBLIC_H_
