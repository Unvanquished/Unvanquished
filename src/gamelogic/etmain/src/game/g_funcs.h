/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

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

In addition, the Daemon Source Code is also subject to certain additional terms.
You should have received a copy of these additional terms immediately following the
terms and conditions of the GNU General Public License which accompanied the Daemon
Source Code.  If not, please request a copy in writing from id Software at the address
below.

If you have questions concerning this license or the applicable additional terms, you
may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville,
Maryland 20850 USA.

===========================================================================
*/

{
	"Info_SetValueForKey_Big", ( byte * ) Info_SetValueForKey_Big
}

,
{
	"Info_SetValueForKey", ( byte * ) Info_SetValueForKey
}

,
{
	"Info_Validate", ( byte * ) Info_Validate
}

,
{
	"Info_RemoveKey_Big", ( byte * ) Info_RemoveKey_Big
}

,
{
	"Info_RemoveKey", ( byte * ) Info_RemoveKey
}

,
{
	"Info_NextPair", ( byte * ) Info_NextPair
}

,
{
	"Info_ValueForKey", ( byte * ) Info_ValueForKey
}

,
{
	"tv", ( byte * ) tv
}

,
{
	"va", ( byte * ) va
}

,
{
	"Com_sprintf", ( byte * ) Com_sprintf
}

,
{
	"Q_CleanDirName", ( byte * ) Q_CleanDirName
}

,
{
	"Q_isBadDirChar", ( byte * ) Q_isBadDirChar
}

,
{
	"Q_CleanStr", ( byte * ) Q_CleanStr
}

,
{
	"Q_PrintStrlen", ( byte * ) Q_PrintStrlen
}

,
{
	"Q_strcat", ( byte * ) Q_strcat
}

,
{
	"Q_strupr", ( byte * ) Q_strupr
}

,
{
	"Q_strlwr", ( byte * ) Q_strlwr
}

,
{
	"Q_stricmp", ( byte * ) Q_stricmp
}

,
{
	"Q_strncmp", ( byte * ) Q_strncmp
}

,
{
	"Q_stricmpn", ( byte * ) Q_stricmpn
}

,
{
	"Q_strncpyz", ( byte * ) Q_strncpyz
}

,
{
	"Q_strrchr", ( byte * ) Q_strrchr
}

,
{
	"Q_isforfilename", ( byte * ) Q_isforfilename
}

,
{
	"Q_isalphanumeric", ( byte * ) Q_isalphanumeric
}

,
{
	"Q_isnumeric", ( byte * ) Q_isnumeric
}

,
{
	"Q_isalpha", ( byte * ) Q_isalpha
}

,
{
	"Q_isupper", ( byte * ) Q_isupper
}

,
{
	"Q_islower", ( byte * ) Q_islower
}

,
{
	"Q_isprint", ( byte * ) Q_isprint
}

,
{
	"Com_ParseInfos", ( byte * ) Com_ParseInfos
}

,
{
	"Parse3DMatrix", ( byte * ) Parse3DMatrix
}

,
{
	"Parse2DMatrix", ( byte * ) Parse2DMatrix
}

,
{
	"Parse1DMatrix", ( byte * ) Parse1DMatrix
}

,
{
	"SkipRestOfLine", ( byte * ) SkipRestOfLine
}

,
{
	"SkipBracedSection", ( byte * ) SkipBracedSection
}

,
{
	"SkipBracedSection_Depth", ( byte * ) SkipBracedSection_Depth
}

,
{
	"COM_MatchToken", ( byte * ) COM_MatchToken
}

,
{
	"COM_ParseExt", ( byte * ) COM_ParseExt
}

,
{
	"COM_Compress", ( byte * ) COM_Compress
}

,
{
	"COM_ParseWarning", ( byte * ) COM_ParseWarning
}

,
{
	"COM_ParseError", ( byte * ) COM_ParseError
}

,
{
	"COM_Parse", ( byte * ) COM_Parse
}

,
{
	"COM_GetCurrentParseLine", ( byte * ) COM_GetCurrentParseLine
}

,
{
	"COM_SetCurrentParseLine", ( byte * ) COM_SetCurrentParseLine
}

,
{
	"COM_RestoreParseSession", ( byte * ) COM_RestoreParseSession
}

,
{
	"COM_BackupParseSession", ( byte * ) COM_BackupParseSession
}

,
{
	"COM_BeginParseSession", ( byte * ) COM_BeginParseSession
}

,
{
	"Swap_Init", ( byte * ) Swap_Init
}

,
{
	"FloatNoSwap", ( byte * ) FloatNoSwap
}

,
{
	"FloatSwap", ( byte * ) FloatSwap
}

,
{
	"Long64NoSwap", ( byte * ) Long64NoSwap
}

,
{
	"Long64Swap", ( byte * ) Long64Swap
}

,
{
	"LongNoSwap", ( byte * ) LongNoSwap
}

,
{
	"LongSwap", ( byte * ) LongSwap
}

,
{
	"ShortNoSwap", ( byte * ) ShortNoSwap
}

,
{
	"ShortSwap", ( byte * ) ShortSwap
}

,
{
	"BigFloat", ( byte * ) BigFloat
}

,
{
	"BigLong64", ( byte * ) BigLong64
}

,
{
	"BigLong", ( byte * ) BigLong
}

,
{
	"BigShort", ( byte * ) BigShort
}

,
{
	"LittleFloat", ( byte * ) LittleFloat
}

,
{
	"LittleLong64", ( byte * ) LittleLong64
}

,
{
	"LittleLong", ( byte * ) LittleLong
}

,
{
	"LittleShort", ( byte * ) LittleShort
}

,
{
	"COM_BitClear", ( byte * ) COM_BitClear
}

,
{
	"COM_BitSet", ( byte * ) COM_BitSet
}

,
{
	"COM_BitCheck", ( byte * ) COM_BitCheck
}

,
{
	"COM_DefaultExtension", ( byte * ) COM_DefaultExtension
}

,
{
	"COM_StripFilename", ( byte * ) COM_StripFilename
}

,
{
	"COM_StripExtension2", ( byte * ) COM_StripExtension2
}

,
{
	"COM_StripExtension", ( byte * ) COM_StripExtension
}

,
{
	"COM_SkipPath", ( byte * ) COM_SkipPath
}

,
{
	"COM_FixPath", ( byte * ) COM_FixPath
}

,
{
	"Com_Clamp", ( byte * ) Com_Clamp
}

,
{
	"Q_fabs", ( byte * ) Q_fabs
}

,
{
	"Q_rsqrt", ( byte * ) Q_rsqrt
}

,
{
	"VectorRotate", ( byte * ) VectorRotate
}

,
{
	"MakeNormalVectors", ( byte * ) MakeNormalVectors
}

,
{
	"ProjectPointOnPlane", ( byte * ) ProjectPointOnPlane
}

,
{
	"AxisCopy", ( byte * ) AxisCopy
}

,
{
	"AxisClear", ( byte * ) AxisClear
}

,
{
	"AnglesToAxis", ( byte * ) AnglesToAxis
}

,
{
	"vectoangles", ( byte * ) vectoangles
}

,
{
	"RotateAroundDirection", ( byte * ) RotateAroundDirection
}

,
{
	"RotatePointAroundVertex", ( byte * ) RotatePointAroundVertex
}

,
{
	"RotatePointAroundVector", ( byte * ) RotatePointAroundVector
}

,
{
	"PlaneFromPoints", ( byte * ) PlaneFromPoints
}

,
{
	"NormalizeColor", ( byte * ) NormalizeColor
}

,
{
	"ColorBytes4", ( byte * ) ColorBytes4
}

,
{
	"ColorBytes3", ( byte * ) ColorBytes3
}

,
{
	"ByteToDir", ( byte * ) ByteToDir
}

,
{
	"DirToByte", ( byte * ) DirToByte
}

,
{
	"ClampShort", ( byte * ) ClampShort
}

,
{
	"ClampChar", ( byte * ) ClampChar
}

,
{
	"Q_crandom", ( byte * ) Q_crandom
}

,
{
	"Q_random", ( byte * ) Q_random
}

,
{
	"Q_rand", ( byte * ) Q_rand
}

,
{
	"Bullet_Fire_Extended", ( byte * ) Bullet_Fire_Extended
}

,
{
	"Bullet_Fire", ( byte * ) Bullet_Fire
}

,
{
	"Bullet_Endpos", ( byte * ) Bullet_Endpos
}

,
{
	"EmitterCheck", ( byte * ) EmitterCheck
}

,
{
	"RubbleFlagCheck", ( byte * ) RubbleFlagCheck
}

,
{
	"G_GetWeaponSpread", ( byte * ) G_GetWeaponSpread
}

,
{
	"G_GetWeaponDamage", ( byte * ) G_GetWeaponDamage
}

,
{
	"SnapVectorTowards", ( byte * ) SnapVectorTowards
}

,
{
	"weapon_smokeBombExplode", ( byte * ) weapon_smokeBombExplode
}

,
{
	"Weapon_Artillery", ( byte * ) Weapon_Artillery
}

,
{
	"G_GlobalClientEvent", ( byte * ) G_GlobalClientEvent
}

,
{
	"artillerySpotterThink", ( byte * ) artillerySpotterThink
}

,
{
	"artilleryGoAway", ( byte * ) artilleryGoAway
}

,
{
	"artilleryThink", ( byte * ) artilleryThink
}

,
{
	"artilleryThink_real", ( byte * ) artilleryThink_real
}

,
{
	"weapon_callAirStrike", ( byte * ) weapon_callAirStrike
}

,
{
	"weapon_checkAirStrike", ( byte * ) weapon_checkAirStrike
}

,
{
	"weapon_callSecondPlane", ( byte * ) weapon_callSecondPlane
}

,
{
	"weapon_checkAirStrikeThink2", ( byte * ) weapon_checkAirStrikeThink2
}

,
{
	"weapon_checkAirStrikeThink1", ( byte * ) weapon_checkAirStrikeThink1
}

,
{
	"G_AddAirstrikeToCounters", ( byte * ) G_AddAirstrikeToCounters
}

,
{
	"G_AvailableAirstrikes", ( byte * ) G_AvailableAirstrikes
}

,
{
	"G_AirStrikeExplode", ( byte * ) G_AirStrikeExplode
}

,
{
	"Weapon_Engineer", ( byte * ) Weapon_Engineer
}

,
{
	"trap_EngineerTrace", ( byte * ) trap_EngineerTrace
}

,
{
	"G_LandmineSpotted", ( byte * ) G_LandmineSpotted
}

,
{
	"G_LandmineTeam", ( byte * ) G_LandmineTeam
}

,
{
	"G_LandmineUnarmed", ( byte * ) G_LandmineUnarmed
}

,
{
	"G_LandmineArmed", ( byte * ) G_LandmineArmed
}

,
{
	"G_LandmineTriggered", ( byte * ) G_LandmineTriggered
}

,
{
	"AutoBuildConstruction", ( byte * ) AutoBuildConstruction
}

,
{
	"EntsThatRadiusCanDamage", ( byte * ) EntsThatRadiusCanDamage
}

,
{
	"Weapon_AdrenalineSyringe", ( byte * ) Weapon_AdrenalineSyringe
}

,
{
	"Weapon_Syringe", ( byte * ) Weapon_Syringe
}

,
{
	"ReviveEntity", ( byte * ) ReviveEntity
}

,
{
	"Weapon_MagicAmmo", ( byte * ) Weapon_MagicAmmo
}

,
{
	"G_PlaceTripmine", ( byte * ) G_PlaceTripmine
}

,
{
	"Weapon_Medic", ( byte * ) Weapon_Medic
}

,
{
	"MagicSink", ( byte * ) MagicSink
}

,
{
	"Weapon_Knife", ( byte * ) Weapon_Knife
}

,
{
	"G_GetWeaponClassForMOD", ( byte * ) G_GetWeaponClassForMOD
}

,
{
	"G_WeaponIsExplosive", ( byte * ) G_WeaponIsExplosive
}

,
{
	"G_Unreferee_v", ( byte * ) G_Unreferee_v
}

,
{
	"G_Warmupfire_v", ( byte * ) G_Warmupfire_v
}

,
{
	"G_WarmupDamageTypeList", ( byte * ) G_WarmupDamageTypeList
}

,
{
	"G_Timelimit_v", ( byte * ) G_Timelimit_v
}

,
{
	"G_BalancedTeams_v", ( byte * ) G_BalancedTeams_v
}

,
{
	"G_AntiLag_v", ( byte * ) G_AntiLag_v
}

,
{
	"G_FriendlyFire_v", ( byte * ) G_FriendlyFire_v
}

,
{
	"G_SwapTeams_v", ( byte * ) G_SwapTeams_v
}

,
{
	"G_StartMatch_v", ( byte * ) G_StartMatch_v
}

,
{
	"G_ShuffleTeams_v", ( byte * ) G_ShuffleTeams_v
}

,
{
	"G_Referee_v", ( byte * ) G_Referee_v
}

,
{
	"G_Pub_v", ( byte * ) G_Pub_v
}

,
{
	"G_Nextmap_v", ( byte * ) G_Nextmap_v
}

,
{
	"G_Mutespecs_v", ( byte * ) G_Mutespecs_v
}

,
{
	"G_MatchReset_v", ( byte * ) G_MatchReset_v
}

,
{
	"G_MapRestart_v", ( byte * ) G_MapRestart_v
}

,
{
	"G_Campaign_v", ( byte * ) G_Campaign_v
}

,
{
	"G_Map_v", ( byte * ) G_Map_v
}

,
{
	"G_UnMute_v", ( byte * ) G_UnMute_v
}

,
{
	"G_Mute_v", ( byte * ) G_Mute_v
}

,
{
	"G_Kick_v", ( byte * ) G_Kick_v
}

,
{
	"G_Gametype_v", ( byte * ) G_Gametype_v
}

,
{
	"G_GametypeList", ( byte * ) G_GametypeList
}

,
{
	"G_Comp_v", ( byte * ) G_Comp_v
}

,
{
	"G_voteSetVoteString", ( byte * ) G_voteSetVoteString
}

,
{
	"G_voteSetValue", ( byte * ) G_voteSetValue
}

,
{
	"G_voteSetOnOff", ( byte * ) G_voteSetOnOff
}

,
{
	"G_voteProcessOnOff", ( byte * ) G_voteProcessOnOff
}

,
{
	"G_voteCurrentSetting", ( byte * ) G_voteCurrentSetting
}

,
{
	"G_playersMessage", ( byte * ) G_playersMessage
}

,
{
	"G_voteDisableMessage", ( byte * ) G_voteDisableMessage
}

,
{
	"G_voteDescription", ( byte * ) G_voteDescription
}

,
{
	"G_voteFlags", ( byte * ) G_voteFlags
}

,
{
	"G_voteHelp", ( byte * ) G_voteHelp
}

,
{
	"G_voteCmdCheck", ( byte * ) G_voteCmdCheck
}

,
{
	"G_GetTeamFromEntity", ( byte * ) G_GetTeamFromEntity
}

,
{
	"G_PrintClientSpammyCenterPrint", ( byte * ) G_PrintClientSpammyCenterPrint
}

,
{
	"G_ParseCampaigns", ( byte * ) G_ParseCampaigns
}

,
{
	"G_MapIsValidCampaignStartMap", ( byte * ) G_MapIsValidCampaignStartMap
}

,
{
	"G_SetEntState", ( byte * ) G_SetEntState
}

,
{
	"DebugLine", ( byte * ) DebugLine
}

,
{
	"G_ProcessTagConnect", ( byte * ) G_ProcessTagConnect
}

,
{
	"infront", ( byte * ) infront
}

,
{
	"G_SetAngle", ( byte * ) G_SetAngle
}

,
{
	"G_SetOrigin", ( byte * ) G_SetOrigin
}

,
{
	"G_AnimScriptSound", ( byte * ) G_AnimScriptSound
}

,
{
	"G_Sound", ( byte * ) G_Sound
}

,
{
	"G_AddEvent", ( byte * ) G_AddEvent
}

,
{
	"G_AddPredictableEvent", ( byte * ) G_AddPredictableEvent
}

,
{
	"G_KillBox", ( byte * ) G_KillBox
}

,
{
	"G_PopupMessage", ( byte * ) G_PopupMessage
}

,
{
	"G_TempEntity", ( byte * ) G_TempEntity
}

,
{
	"G_FreeEntity", ( byte * ) G_FreeEntity
}

,
{
	"G_EntitiesFree", ( byte * ) G_EntitiesFree
}

,
{
	"G_Spawn", ( byte * ) G_Spawn
}

,
{
	"G_InitGentity", ( byte * ) G_InitGentity
}

,
{
	"G_SetMovedir", ( byte * ) G_SetMovedir
}

,
{
	"vtosf", ( byte * ) vtosf
}

,
{
	"vtos", ( byte * ) vtos
}

,
{
	"G_UseTargets", ( byte * ) G_UseTargets
}

,
{
	"G_UseEntity", ( byte * ) G_UseEntity
}

,
{
	"G_AllowTeamsAllowed", ( byte * ) G_AllowTeamsAllowed
}

,
{
	"G_PickTarget", ( byte * ) G_PickTarget
}

,
{
	"G_FindByTargetnameFast", ( byte * ) G_FindByTargetnameFast
}

,
{
	"G_FindByTargetname", ( byte * ) G_FindByTargetname
}

,
{
	"G_Find", ( byte * ) G_Find
}

,
{
	"G_TeamCommand", ( byte * ) G_TeamCommand
}

,
{
	"G_StringIndex", ( byte * ) G_StringIndex
}

,
{
	"G_CharacterIndex", ( byte * ) G_CharacterIndex
}

,
{
	"G_ShaderIndex", ( byte * ) G_ShaderIndex
}

,
{
	"G_SkinIndex", ( byte * ) G_SkinIndex
}

,
{
	"G_SoundIndex", ( byte * ) G_SoundIndex
}

,
{
	"G_ModelIndex", ( byte * ) G_ModelIndex
}

,
{
	"G_FindConfigstringIndex", ( byte * ) G_FindConfigstringIndex
}

,
{
	"BuildShaderStateConfig", ( byte * ) BuildShaderStateConfig
}

,
{
	"AddRemap", ( byte * ) AddRemap
}

,
{
	"SP_trigger_concussive_dust", ( byte * ) SP_trigger_concussive_dust
}

,
{
	"trigger_concussive_touch", ( byte * ) trigger_concussive_touch
}

,
{
	"SP_trigger_objective_info", ( byte * ) SP_trigger_objective_info
}

,
{
	"Think_SetupObjectiveInfo", ( byte * ) Think_SetupObjectiveInfo
}

,
{
	"Touch_ObjectiveInfo", ( byte * ) Touch_ObjectiveInfo
}

,
{
	"G_SetConfigStringValue", ( byte * ) G_SetConfigStringValue
}

,
{
	"constructible_indicator_think", ( byte * ) constructible_indicator_think
}

,
{
	"explosive_indicator_think", ( byte * ) explosive_indicator_think
}

,
{
	"SP_trigger_flagonly_multiple", ( byte * ) SP_trigger_flagonly_multiple
}

,
{
	"SP_trigger_flagonly", ( byte * ) SP_trigger_flagonly
}

,
{
	"Touch_flagonly_multiple", ( byte * ) Touch_flagonly_multiple
}

,
{
	"Touch_flagonly", ( byte * ) Touch_flagonly
}

,
{
	"SP_gas", ( byte * ) SP_gas
}

,
{
	"SP_trigger_aidoor", ( byte * ) SP_trigger_aidoor
}

,
{
	"trigger_aidoor_stayopen", ( byte * ) trigger_aidoor_stayopen
}

,
{
	"SP_trigger_once", ( byte * ) SP_trigger_once
}

,
{
	"SP_func_timer", ( byte * ) SP_func_timer
}

,
{
	"func_timer_use", ( byte * ) func_timer_use
}

,
{
	"func_timer_think", ( byte * ) func_timer_think
}

,
{
	"SP_trigger_ammo", ( byte * ) SP_trigger_ammo
}

,
{
	"SP_misc_cabinet_supply", ( byte * ) SP_misc_cabinet_supply
}

,
{
	"trigger_ammo_setup", ( byte * ) trigger_ammo_setup
}

,
{
	"trigger_ammo_think", ( byte * ) trigger_ammo_think
}

,
{
	"ammo_touch", ( byte * ) ammo_touch
}

,
{
	"G_IsAllowedAmmo", ( byte * ) G_IsAllowedAmmo
}

,
{
	"SP_trigger_heal", ( byte * ) SP_trigger_heal
}

,
{
	"SP_misc_cabinet_health", ( byte * ) SP_misc_cabinet_health
}

,
{
	"trigger_heal_setup", ( byte * ) trigger_heal_setup
}

,
{
	"trigger_heal_think", ( byte * ) trigger_heal_think
}

,
{
	"heal_touch", ( byte * ) heal_touch
}

,
{
	"G_IsAllowedHeal", ( byte * ) G_IsAllowedHeal
}

,
{
	"SP_trigger_hurt", ( byte * ) SP_trigger_hurt
}

,
{
	"hurt_use", ( byte * ) hurt_use
}

,
{
	"hurt_think", ( byte * ) hurt_think
}

,
{
	"hurt_touch", ( byte * ) hurt_touch
}

,
{
	"SP_trigger_teleport", ( byte * ) SP_trigger_teleport
}

,
{
	"trigger_teleporter_touch", ( byte * ) trigger_teleporter_touch
}

,
{
	"SP_target_push", ( byte * ) SP_target_push
}

,
{
	"Use_target_push", ( byte * ) Use_target_push
}

,
{
	"SP_trigger_push", ( byte * ) SP_trigger_push
}

,
{
	"trigger_push_use", ( byte * ) trigger_push_use
}

,
{
	"AimAtTarget", ( byte * ) AimAtTarget
}

,
{
	"trigger_push_touch", ( byte * ) trigger_push_touch
}

,
{
	"SP_trigger_always", ( byte * ) SP_trigger_always
}

,
{
	"trigger_always_think", ( byte * ) trigger_always_think
}

,
{
	"SP_trigger_multiple", ( byte * ) SP_trigger_multiple
}

,
{
	"Touch_Multi", ( byte * ) Touch_Multi
}

,
{
	"Use_Multi", ( byte * ) Use_Multi
}

,
{
	"multi_trigger", ( byte * ) multi_trigger
}

,
{
	"multi_wait", ( byte * ) multi_wait
}

,
{
	"InitTrigger", ( byte * ) InitTrigger
}

,
{
	"G_UpdateTeamMapData", ( byte * ) G_UpdateTeamMapData
}

,
{
	"G_SendMapEntityInfo", ( byte * ) G_SendMapEntityInfo
}

,
{
	"G_SendSpectatorMapEntityInfo", ( byte * ) G_SendSpectatorMapEntityInfo
}

,
{
	"G_UpdateTeamMapData_CommandmapMarker", ( byte * ) G_UpdateTeamMapData_CommandmapMarker
}

,
{
	"G_UpdateTeamMapData_LandMine", ( byte * ) G_UpdateTeamMapData_LandMine
}

,
{
	"G_UpdateTeamMapData_Player", ( byte * ) G_UpdateTeamMapData_Player
}

,
{
	"G_UpdateTeamMapData_Destruct", ( byte * ) G_UpdateTeamMapData_Destruct
}

,
{
	"G_UpdateTeamMapData_Tank", ( byte * ) G_UpdateTeamMapData_Tank
}

,
{
	"G_UpdateTeamMapData_Construct", ( byte * ) G_UpdateTeamMapData_Construct
}

,
{
	"G_ResetTeamMapData", ( byte * ) G_ResetTeamMapData
}

,
{
	"G_VisibleFromBinoculars", ( byte * ) G_VisibleFromBinoculars
}

,
{
	"G_SetupFrustum_ForBinoculars", ( byte * ) G_SetupFrustum_ForBinoculars
}

,
{
	"G_SetupFrustum", ( byte * ) G_SetupFrustum
}

,
{
	"G_FindMapEntityDataSingleClient", ( byte * ) G_FindMapEntityDataSingleClient
}

,
{
	"G_FindMapEntityData", ( byte * ) G_FindMapEntityData
}

,
{
	"G_AllocMapEntityData", ( byte * ) G_AllocMapEntityData
}

,
{
	"G_FreeMapEntityData", ( byte * ) G_FreeMapEntityData
}

,
{
	"G_InitMapEntityData", ( byte * ) G_InitMapEntityData
}

,
{
	"G_PushMapEntityToBuffer", ( byte * ) G_PushMapEntityToBuffer
}

,
{
	"G_desiredFollow", ( byte * ) G_desiredFollow
}

,
{
	"G_allowFollow", ( byte * ) G_allowFollow
}

,
{
	"G_blockoutTeam", ( byte * ) G_blockoutTeam
}

,
{
	"G_removeSpecInvite", ( byte * ) G_removeSpecInvite
}

,
{
	"G_swapTeamLocks", ( byte * ) G_swapTeamLocks
}

,
{
	"G_updateSpecLock", ( byte * ) G_updateSpecLock
}

,
{
	"G_teamJoinCheck", ( byte * ) G_teamJoinCheck
}

,
{
	"G_verifyMatchState", ( byte * ) G_verifyMatchState
}

,
{
	"G_readyMatchState", ( byte * ) G_readyMatchState
}

,
{
	"G_checkReady", ( byte * ) G_checkReady
}

,
{
	"G_teamID", ( byte * ) G_teamID
}

,
{
	"G_shuffleTeams", ( byte * ) G_shuffleTeams
}

,
{
	"G_SortPlayersByXP", ( byte * ) G_SortPlayersByXP
}

,
{
	"G_swapTeams", ( byte * ) G_swapTeams
}

,
{
	"G_teamReset", ( byte * ) G_teamReset
}

,
{
	"Team_ClassForString", ( byte * ) Team_ClassForString
}

,
{
	"SP_team_WOLF_checkpoint", ( byte * ) SP_team_WOLF_checkpoint
}

,
{
	"checkpoint_spawntouch", ( byte * ) checkpoint_spawntouch
}

,
{
	"checkpoint_touch", ( byte * ) checkpoint_touch
}

,
{
	"checkpoint_think", ( byte * ) checkpoint_think
}

,
{
	"checkpoint_hold_think", ( byte * ) checkpoint_hold_think
}

,
{
	"checkpoint_use", ( byte * ) checkpoint_use
}

,
{
	"checkpoint_use_think", ( byte * ) checkpoint_use_think
}

,
{
	"SP_team_WOLF_objective", ( byte * ) SP_team_WOLF_objective
}

,
{
	"objective_Register", ( byte * ) objective_Register
}

,
{
	"team_wolf_objective_use", ( byte * ) team_wolf_objective_use
}

,
{
	"SP_team_CTF_bluespawn", ( byte * ) SP_team_CTF_bluespawn
}

,
{
	"SP_team_CTF_redspawn", ( byte * ) SP_team_CTF_redspawn
}

,
{
	"SP_team_CTF_blueplayer", ( byte * ) SP_team_CTF_blueplayer
}

,
{
	"SP_team_CTF_redplayer", ( byte * ) SP_team_CTF_redplayer
}

,
{
	"Use_Team_Spawnpoint", ( byte * ) Use_Team_Spawnpoint
}

,
{
	"Use_Team_InitialSpawnpoint", ( byte * ) Use_Team_InitialSpawnpoint
}

,
{
	"CheckTeamStatus", ( byte * ) CheckTeamStatus
}

,
{
	"TeamplayInfoMessage", ( byte * ) TeamplayInfoMessage
}

,
{
	"SelectCTFSpawnPoint", ( byte * ) SelectCTFSpawnPoint
}

,
{
	"SelectRandomTeamSpawnPoint", ( byte * ) SelectRandomTeamSpawnPoint
}

,
{
	"FindClosestObjectiveIndex", ( byte * ) FindClosestObjectiveIndex
}

,
{
	"FindFarthestObjectiveIndex", ( byte * ) FindFarthestObjectiveIndex
}

,
{
	"Pickup_Team", ( byte * ) Pickup_Team
}

,
{
	"Team_TouchEnemyFlag", ( byte * ) Team_TouchEnemyFlag
}

,
{
	"Team_TouchOurFlag", ( byte * ) Team_TouchOurFlag
}

,
{
	"Team_DroppedFlagThink", ( byte * ) Team_DroppedFlagThink
}

,
{
	"Team_ReturnFlag", ( byte * ) Team_ReturnFlag
}

,
{
	"Team_ReturnFlagSound", ( byte * ) Team_ReturnFlagSound
}

,
{
	"Team_ResetFlags", ( byte * ) Team_ResetFlags
}

,
{
	"Team_ResetFlag", ( byte * ) Team_ResetFlag
}

,
{
	"Team_CheckHurtCarrier", ( byte * ) Team_CheckHurtCarrier
}

,
{
	"Team_FragBonuses", ( byte * ) Team_FragBonuses
}

,
{
	"OnSameTeam", ( byte * ) OnSameTeam
}

,
{
	"PrintMsg", ( byte * ) PrintMsg
}

,
{
	"TeamColorString", ( byte * ) TeamColorString
}

,
{
	"OtherTeamName", ( byte * ) OtherTeamName
}

,
{
	"TeamName", ( byte * ) TeamName
}

,
{
	"OtherTeam", ( byte * ) OtherTeam
}

,
{
	"Team_InitGame", ( byte * ) Team_InitGame
}

,
{
	"SP_target_rumble", ( byte * ) SP_target_rumble
}

,
{
	"target_rumble_use", ( byte * ) target_rumble_use
}

,
{
	"target_rumble_think", ( byte * ) target_rumble_think
}

,
{
	"SP_target_script_trigger", ( byte * ) SP_target_script_trigger
}

,
{
	"target_script_trigger_use", ( byte * ) target_script_trigger_use
}

,
{
	"SP_target_smoke", ( byte * ) SP_target_smoke
}

,
{
	"smoke_init", ( byte * ) smoke_init
}

,
{
	"smoke_toggle", ( byte * ) smoke_toggle
}

,
{
	"smoke_think", ( byte * ) smoke_think
}

,
{
	"SP_target_alarm", ( byte * ) SP_target_alarm
}

,
{
	"Use_Target_Alarm", ( byte * ) Use_Target_Alarm
}

,
{
	"SP_target_lock", ( byte * ) SP_target_lock
}

,
{
	"SP_target_autosave", ( byte * ) SP_target_autosave
}

,
{
	"SP_target_counter", ( byte * ) SP_target_counter
}

,
{
	"SP_target_fog", ( byte * ) SP_target_fog
}

,
{
	"Use_target_fog", ( byte * ) Use_target_fog
}

,
{
	"Use_Target_Lock", ( byte * ) Use_Target_Lock
}

,
{
	"Use_Target_Counter", ( byte * ) Use_Target_Counter
}

,
{
	"SP_target_location", ( byte * ) SP_target_location
}

,
{
	"SP_target_position", ( byte * ) SP_target_position
}

,
{
	"SP_target_kill", ( byte * ) SP_target_kill
}

,
{
	"target_kill_use", ( byte * ) target_kill_use
}

,
{
	"G_KillEnts", ( byte * ) G_KillEnts
}

,
{
	"SP_target_relay", ( byte * ) SP_target_relay
}

,
{
	"relay_AIScript_AlertEntity", ( byte * ) relay_AIScript_AlertEntity
}

,
{
	"target_relay_use", ( byte * ) target_relay_use
}

,
{
	"SP_target_teleporter", ( byte * ) SP_target_teleporter
}

,
{
	"target_teleporter_use", ( byte * ) target_teleporter_use
}

,
{
	"SP_target_laser", ( byte * ) SP_target_laser
}

,
{
	"target_laser_start", ( byte * ) target_laser_start
}

,
{
	"target_laser_use", ( byte * ) target_laser_use
}

,
{
	"target_laser_off", ( byte * ) target_laser_off
}

,
{
	"target_laser_on", ( byte * ) target_laser_on
}

,
{
	"target_laser_think", ( byte * ) target_laser_think
}

,
{
	"SP_misc_beam", ( byte * ) SP_misc_beam
}

,
{
	"misc_beam_start", ( byte * ) misc_beam_start
}

,
{
	"misc_beam_think", ( byte * ) misc_beam_think
}

,
{
	"SP_target_speaker", ( byte * ) SP_target_speaker
}

,
{
	"target_speaker_multiple", ( byte * ) target_speaker_multiple
}

,
{
	"Use_Target_Speaker", ( byte * ) Use_Target_Speaker
}

,
{
	"SP_target_print", ( byte * ) SP_target_print
}

,
{
	"Use_Target_Print", ( byte * ) Use_Target_Print
}

,
{
	"SP_target_score", ( byte * ) SP_target_score
}

,
{
	"Use_Target_Score", ( byte * ) Use_Target_Score
}

,
{
	"SP_target_delay", ( byte * ) SP_target_delay
}

,
{
	"Use_Target_Delay", ( byte * ) Use_Target_Delay
}

,
{
	"Think_Target_Delay", ( byte * ) Think_Target_Delay
}

,
{
	"SP_target_remove_powerups", ( byte * ) SP_target_remove_powerups
}

,
{
	"Use_target_remove_powerups", ( byte * ) Use_target_remove_powerups
}

,
{
	"SP_target_give", ( byte * ) SP_target_give
}

,
{
	"Use_Target_Give", ( byte * ) Use_Target_Give
}

,
{
	"G_CheckMenDown", ( byte * ) G_CheckMenDown
}

,
{
	"G_CheckForNeededClasses", ( byte * ) G_CheckForNeededClasses
}

,
{
	"G_SendSystemMessage", ( byte * ) G_SendSystemMessage
}

,
{
	"G_GetSysMessageNumber", ( byte * ) G_GetSysMessageNumber
}

,
{
	"G_NeedEngineers", ( byte * ) G_NeedEngineers
}

,
{
	"CreateMapServerEntities", ( byte * ) CreateMapServerEntities
}

,
{
	"CreateServerEntityFromData", ( byte * ) CreateServerEntityFromData
}

,
{
	"InitServerEntitySetupFunc", ( byte * ) InitServerEntitySetupFunc
}

,
{
	"FindServerEntity", ( byte * ) FindServerEntity
}

,
{
	"InitialServerEntitySetup", ( byte * ) InitialServerEntitySetup
}

,
{
	"CreateServerEntity", ( byte * ) CreateServerEntity
}

,
{
	"GetFreeServerEntity", ( byte * ) GetFreeServerEntity
}

,
{
	"GetServerEntity", ( byte * ) GetServerEntity
}

,
{
	"InitServerEntities", ( byte * ) InitServerEntities
}

,
{
	"ConsoleCommand", ( byte * ) ConsoleCommand
}

,
{
	"Svcmd_RevivePlayer", ( byte * ) Svcmd_RevivePlayer
}

,
{
	"Svcmd_ListCampaigns_f", ( byte * ) Svcmd_ListCampaigns_f
}

,
{
	"Svcmd_Campaign_f", ( byte * ) Svcmd_Campaign_f
}

,
{
	"Svcmd_ShuffleTeams_f", ( byte * ) Svcmd_ShuffleTeams_f
}

,
{
	"Svcmd_SwapTeams_f", ( byte * ) Svcmd_SwapTeams_f
}

,
{
	"Svcmd_ResetMatch_f", ( byte * ) Svcmd_ResetMatch_f
}

,
{
	"Svcmd_StartMatch_f", ( byte * ) Svcmd_StartMatch_f
}

,
{
	"Svcmd_ForceTeam_f", ( byte * ) Svcmd_ForceTeam_f
}

,
{
	"G_GetPlayerByName", ( byte * ) G_GetPlayerByName
}

,
{
	"G_GetPlayerByNum", ( byte * ) G_GetPlayerByNum
}

,
{
	"ClientForString", ( byte * ) ClientForString
}

,
{
	"Svcmd_EntityList_f", ( byte * ) Svcmd_EntityList_f
}

,
{
	"ClearMaxLivesBans", ( byte * ) ClearMaxLivesBans
}

,
{
	"Svcmd_RemoveIP_f", ( byte * ) Svcmd_RemoveIP_f
}

,
{
	"Svcmd_AddIP_f", ( byte * ) Svcmd_AddIP_f
}

,
{
	"G_ProcessIPBans", ( byte * ) G_ProcessIPBans
}

,
{
	"AddMaxLivesGUID", ( byte * ) AddMaxLivesGUID
}

,
{
	"AddMaxLivesBan", ( byte * ) AddMaxLivesBan
}

,
{
	"AddIPBan", ( byte * ) AddIPBan
}

,
{
	"AddIP", ( byte * ) AddIP
}

,
{
	"G_FilterMaxLivesPacket", ( byte * ) G_FilterMaxLivesPacket
}

,
{
	"G_FilterMaxLivesIPPacket", ( byte * ) G_FilterMaxLivesIPPacket
}

,
{
	"G_FilterIPBanPacket", ( byte * ) G_FilterIPBanPacket
}

,
{
	"G_FilterPacket", ( byte * ) G_FilterPacket
}

,
{
	"G_FindIpData", ( byte * ) G_FindIpData
}

,
{
	"PrintMaxLivesGUID", ( byte * ) PrintMaxLivesGUID
}

,
{
	"StringToFilter", ( byte * ) StringToFilter
}

,
{
	"G_BuildEndgameStats", ( byte * ) G_BuildEndgameStats
}

,
{
	"G_DebugAddSkillPoints", ( byte * ) G_DebugAddSkillPoints
}

,
{
	"G_DebugAddSkillLevel", ( byte * ) G_DebugAddSkillLevel
}

,
{
	"G_DebugCloseSkillLog", ( byte * ) G_DebugCloseSkillLog
}

,
{
	"G_DebugOpenSkillLog", ( byte * ) G_DebugOpenSkillLog
}

,
{
	"G_AddKillSkillPointsForDestruction", ( byte * ) G_AddKillSkillPointsForDestruction
}

,
{
	"G_AddKillSkillPoints", ( byte * ) G_AddKillSkillPoints
}

,
{
	"G_LoseKillSkillPoints", ( byte * ) G_LoseKillSkillPoints
}

,
{
	"G_AddSkillPoints", ( byte * ) G_AddSkillPoints
}

,
{
	"G_LoseSkillPoints", ( byte * ) G_LoseSkillPoints
}

,
{
	"G_SetPlayerSkill", ( byte * ) G_SetPlayerSkill
}

,
{
	"G_SetPlayerScore", ( byte * ) G_SetPlayerScore
}

,
{
	"G_PrintAccuracyLog", ( byte * ) G_PrintAccuracyLog
}

,
{
	"G_LogRegionHit", ( byte * ) G_LogRegionHit
}

,
{
	"G_LogTeamKill", ( byte * ) G_LogTeamKill
}

,
{
	"G_LogKill", ( byte * ) G_LogKill
}

,
{
	"G_LogDeath", ( byte * ) G_LogDeath
}

,
{
	"G_SpawnEntitiesFromString", ( byte * ) G_SpawnEntitiesFromString
}

,
{
	"SP_worldspawn", ( byte * ) SP_worldspawn
}

,
{
	"G_ParseSpawnVars", ( byte * ) G_ParseSpawnVars
}

,
{
	"G_AddSpawnVarToken", ( byte * ) G_AddSpawnVarToken
}

,
{
	"G_SpawnGEntityFromSpawnVars", ( byte * ) G_SpawnGEntityFromSpawnVars
}

,
{
	"G_ParseField", ( byte * ) G_ParseField
}

,
{
	"G_NewString", ( byte * ) G_NewString
}

,
{
	"G_CallSpawn", ( byte * ) G_CallSpawn
}

,
{
	"G_SpawnVector2DExt", ( byte * ) G_SpawnVector2DExt
}

,
{
	"G_SpawnVectorExt", ( byte * ) G_SpawnVectorExt
}

,
{
	"G_SpawnIntExt", ( byte * ) G_SpawnIntExt
}

,
{
	"G_SpawnFloatExt", ( byte * ) G_SpawnFloatExt
}

,
{
	"G_SpawnStringExt", ( byte * ) G_SpawnStringExt
}

,
{
	"G_WriteSessionData", ( byte * ) G_WriteSessionData
}

,
{
	"G_InitWorldSession", ( byte * ) G_InitWorldSession
}

,
{
	"G_InitSessionData", ( byte * ) G_InitSessionData
}

,
{
	"G_ReadSessionData", ( byte * ) G_ReadSessionData
}

,
{
	"G_CalcRank", ( byte * ) G_CalcRank
}

,
{
	"G_ClientSwap", ( byte * ) G_ClientSwap
}

,
{
	"G_WriteClientSessionData", ( byte * ) G_WriteClientSessionData
}

,
{
	"etpro_ScriptAction_SetValues", ( byte * ) etpro_ScriptAction_SetValues
}

,
{
	"G_ScriptAction_AbortIfNotSinglePlayer", ( byte * ) G_ScriptAction_AbortIfNotSinglePlayer
}

,
{
	"G_ScriptAction_AbortIfWarmup", ( byte * ) G_ScriptAction_AbortIfWarmup
}

,
{
	"G_ScriptAction_Cvar", ( byte * ) G_ScriptAction_Cvar
}

,
{
	"G_ScriptAction_ConstructibleDuration", ( byte * ) G_ScriptAction_ConstructibleDuration
}

,
{
	"G_ScriptAction_ConstructibleWeaponclass", ( byte * ) G_ScriptAction_ConstructibleWeaponclass
}

,
{
	"G_ScriptAction_ConstructibleHealth", ( byte * ) G_ScriptAction_ConstructibleHealth
}

,
{
	"G_ScriptAction_ConstructibleDestructXPBonus", ( byte * ) G_ScriptAction_ConstructibleDestructXPBonus
}

,
{
	"G_ScriptAction_ConstructibleConstructXPBonus", ( byte * ) G_ScriptAction_ConstructibleConstructXPBonus
}

,
{
	"G_ScriptAction_ConstructibleChargeBarReq", ( byte * ) G_ScriptAction_ConstructibleChargeBarReq
}

,
{
	"G_ScriptAction_ConstructibleClass", ( byte * ) G_ScriptAction_ConstructibleClass
}

,
{
	"G_ScriptAction_Construct", ( byte * ) G_ScriptAction_Construct
}

,
{
	"G_ScriptAction_SetAASState", ( byte * ) G_ScriptAction_SetAASState
}

,
{
	"G_ScriptAction_PrintGlobalAccum", ( byte * ) G_ScriptAction_PrintGlobalAccum
}

,
{
	"G_ScriptAction_PrintAccum", ( byte * ) G_ScriptAction_PrintAccum
}

,
{
	"G_ScriptAction_SetHQStatus", ( byte * ) G_ScriptAction_SetHQStatus
}

,
{
	"G_ScriptAction_RepairMG42", ( byte * ) G_ScriptAction_RepairMG42
}

,
{
	"G_ScriptAction_StopCam", ( byte * ) G_ScriptAction_StopCam
}

,
{
	"G_ScriptAction_SetInitialCamera", ( byte * ) G_ScriptAction_SetInitialCamera
}

,
{
	"G_ScriptAction_StartCam", ( byte * ) G_ScriptAction_StartCam
}

,
{
	"G_ScriptAction_SetState", ( byte * ) G_ScriptAction_SetState
}

,
{
	"G_ScriptAction_SetDamagable", ( byte * ) G_ScriptAction_SetDamagable
}

,
{
	"G_ScriptAction_RemoveEntity", ( byte * ) G_ScriptAction_RemoveEntity
}

,
{
	"G_ScriptAction_SetRoundTimelimit", ( byte * ) G_ScriptAction_SetRoundTimelimit
}

,
{
	"G_ScriptAction_EndRound", ( byte * ) G_ScriptAction_EndRound
}

,
{
	"G_ScriptAction_Announce", ( byte * ) G_ScriptAction_Announce
}

,
{
	"G_ScriptAction_Announce_Icon", ( byte * ) G_ScriptAction_Announce_Icon
}

,
{
	"G_ScriptAction_TeamVoiceAnnounce", ( byte * ) G_ScriptAction_TeamVoiceAnnounce
}

,
{
	"G_ScriptAction_RemoveTeamVoiceAnnounce", ( byte * ) G_ScriptAction_RemoveTeamVoiceAnnounce
}

,
{
	"G_ScriptAction_AddTeamVoiceAnnounce", ( byte * ) G_ScriptAction_AddTeamVoiceAnnounce
}

,
{
	"G_ScriptAction_SetDefendingTeam", ( byte * ) G_ScriptAction_SetDefendingTeam
}

,
{
	"G_ScriptAction_SetWinner", ( byte * ) G_ScriptAction_SetWinner
}

,
{
	"G_ScriptAction_VoiceAnnounce", ( byte * ) G_ScriptAction_VoiceAnnounce
}

,
{
	"G_ScriptAction_SetDebugLevel", ( byte * ) G_ScriptAction_SetDebugLevel
}

,
{
	"G_ScriptAction_ObjectiveStatus", ( byte * ) G_ScriptAction_ObjectiveStatus
}

,
{
	"G_ScriptAction_SetMainObjective", ( byte * ) G_ScriptAction_SetMainObjective
}

,
{
	"G_ScriptAction_NumberofObjectives", ( byte * ) G_ScriptAction_NumberofObjectives
}

,
{
	"G_ScriptAction_AlliedRespawntime", ( byte * ) G_ScriptAction_AlliedRespawntime
}

,
{
	"G_ScriptAction_AxisRespawntime", ( byte * ) G_ScriptAction_AxisRespawntime
}

,
{
	"G_ScriptAction_AIScriptName", ( byte * ) G_ScriptAction_AIScriptName
}

,
{
	"G_ScriptAction_EntityScriptName", ( byte * ) G_ScriptAction_EntityScriptName
}

,
{
	"G_ScriptAction_StopSound", ( byte * ) G_ScriptAction_StopSound
}

,
{
	"G_ScriptAction_Halt", ( byte * ) G_ScriptAction_Halt
}

,
{
	"G_ScriptAction_TagConnect", ( byte * ) G_ScriptAction_TagConnect
}

,
{
	"G_ScriptAction_ResetScript", ( byte * ) G_ScriptAction_ResetScript
}

,
{
	"G_ScriptAction_FaceAngles", ( byte * ) G_ScriptAction_FaceAngles
}

,
{
	"G_ScriptAction_Print", ( byte * ) G_ScriptAction_Print
}

,
{
	"G_ScriptAction_GlobalAccum", ( byte * ) G_ScriptAction_GlobalAccum
}

,
{
	"G_ScriptAction_Accum", ( byte * ) G_ScriptAction_Accum
}

,
{
	"G_ScriptAction_EnableSpeaker", ( byte * ) G_ScriptAction_EnableSpeaker
}

,
{
	"G_ScriptAction_DisableSpeaker", ( byte * ) G_ScriptAction_DisableSpeaker
}

,
{
	"G_ScriptAction_ToggleSpeaker", ( byte * ) G_ScriptAction_ToggleSpeaker
}

,
{
	"G_ScriptAction_AlertEntity", ( byte * ) G_ScriptAction_AlertEntity
}

,
{
	"G_ScriptAction_PlayAnim", ( byte * ) G_ScriptAction_PlayAnim
}

,
{
	"G_ScriptAction_MusicFade", ( byte * ) G_ScriptAction_MusicFade
}

,
{
	"G_ScriptAction_MusicQueue", ( byte * ) G_ScriptAction_MusicQueue
}

,
{
	"G_ScriptAction_MusicStop", ( byte * ) G_ScriptAction_MusicStop
}

,
{
	"G_ScriptAction_MusicPlay", ( byte * ) G_ScriptAction_MusicPlay
}

,
{
	"G_ScriptAction_MusicStart", ( byte * ) G_ScriptAction_MusicStart
}

,
{
	"G_ScriptAction_FadeAllSounds", ( byte * ) G_ScriptAction_FadeAllSounds
}

,
{
	"G_ScriptAction_PlaySound", ( byte * ) G_ScriptAction_PlaySound
}

,
{
	"G_ScriptAction_Trigger", ( byte * ) G_ScriptAction_Trigger
}

,
{
	"G_ScriptAction_Wait", ( byte * ) G_ScriptAction_Wait
}

,
{
	"G_ScriptAction_GotoMarker", ( byte * ) G_ScriptAction_GotoMarker
}

,
{
	"G_ScriptAction_SetGlobalFog", ( byte * ) G_ScriptAction_SetGlobalFog
}

,
{
	"G_ScriptAction_Kill", ( byte * ) G_ScriptAction_Kill
}

,
{
	"G_ScriptAction_DisableMessage", ( byte * ) G_ScriptAction_DisableMessage
}

,
{
	"G_ScriptAction_AddTankAmmo", ( byte * ) G_ScriptAction_AddTankAmmo
}

,
{
	"G_ScriptAction_SetTankAmmo", ( byte * ) G_ScriptAction_SetTankAmmo
}

,
{
	"G_ScriptAction_AllowTankEnter", ( byte * ) G_ScriptAction_AllowTankEnter
}

,
{
	"G_ScriptAction_AllowTankExit", ( byte * ) G_ScriptAction_AllowTankExit
}

,
{
	"G_ScriptAction_SpawnRubble", ( byte * ) G_ScriptAction_SpawnRubble
}

,
{
	"G_ScriptAction_SetChargeTimeFactor", ( byte * ) G_ScriptAction_SetChargeTimeFactor
}

,
{
	"G_ScriptAction_AbortMove", ( byte * ) G_ScriptAction_AbortMove
}

,
{
	"G_ScriptAction_FollowSpline", ( byte * ) G_ScriptAction_FollowSpline
}

,
{
	"G_ScriptAction_StopRotation", ( byte * ) G_ScriptAction_StopRotation
}

,
{
	"G_ScriptAction_SetRotation", ( byte * ) G_ScriptAction_SetRotation
}

,
{
	"G_ScriptAction_SetSpeed", ( byte * ) G_ScriptAction_SetSpeed
}

,
{
	"G_ScriptAction_StartAnimation", ( byte * ) G_ScriptAction_StartAnimation
}

,
{
	"G_ScriptAction_UnFreezeAnimation", ( byte * ) G_ScriptAction_UnFreezeAnimation
}

,
{
	"G_ScriptAction_FreezeAnimation", ( byte * ) G_ScriptAction_FreezeAnimation
}

,
{
	"G_ScriptAction_AttatchToTrain", ( byte * ) G_ScriptAction_AttatchToTrain
}

,
{
	"G_ScriptAction_FollowPath", ( byte * ) G_ScriptAction_FollowPath
}

,
{
	"G_ScriptAction_ShaderRemapFlush", ( byte * ) G_ScriptAction_ShaderRemapFlush
}

,
{
	"G_ScriptAction_ShaderRemap", ( byte * ) G_ScriptAction_ShaderRemap
}

,
{
	"G_ScriptAction_ChangeModel", ( byte * ) G_ScriptAction_ChangeModel
}

,
{
	"G_ScriptAction_SetAutoSpawn", ( byte * ) G_ScriptAction_SetAutoSpawn
}

,
{
	"G_ScriptAction_SetPosition", ( byte * ) G_ScriptAction_SetPosition
}

,
{
	"G_ScriptAction_SetModelFromBrushmodel", ( byte * ) G_ScriptAction_SetModelFromBrushmodel
}

,
{
	"SP_script_multiplayer", ( byte * ) SP_script_multiplayer
}

,
{
	"SP_script_camera", ( byte * ) SP_script_camera
}

,
{
	"SP_script_model_med", ( byte * ) SP_script_model_med
}

,
{
	"script_model_med_use", ( byte * ) script_model_med_use
}

,
{
	"script_model_med_spawn", ( byte * ) script_model_med_spawn
}

,
{
	"SP_script_mover", ( byte * ) SP_script_mover
}

,
{
	"script_mover_blocked", ( byte * ) script_mover_blocked
}

,
{
	"script_mover_use", ( byte * ) script_mover_use
}

,
{
	"script_mover_spawn", ( byte * ) script_mover_spawn
}

,
{
	"script_mover_aas_blocking", ( byte * ) script_mover_aas_blocking
}

,
{
	"script_mover_set_blocking", ( byte * ) script_mover_set_blocking
}

,
{
	"script_mover_die", ( byte * ) script_mover_die
}

,
{
	"script_linkentity", ( byte * ) script_linkentity
}

,
{
	"mountedmg42_fire", ( byte * ) mountedmg42_fire
}

,
{
	"G_Script_ScriptRun", ( byte * ) G_Script_ScriptRun
}

,
{
	"G_Script_ScriptEvent", ( byte * ) G_Script_ScriptEvent
}

,
{
	"G_Script_GetEventIndex", ( byte * ) G_Script_GetEventIndex
}

,
{
	"G_Script_EventStringInit", ( byte * ) G_Script_EventStringInit
}

,
{
	"G_Script_ScriptChange", ( byte * ) G_Script_ScriptChange
}

,
{
	"G_Script_ScriptParse", ( byte * ) G_Script_ScriptParse
}

,
{
	"G_Script_ParseSpawnbot", ( byte * ) G_Script_ParseSpawnbot
}

,
{
	"G_Script_ScriptLoad", ( byte * ) G_Script_ScriptLoad
}

,
{
	"G_Script_ActionForString", ( byte * ) G_Script_ActionForString
}

,
{
	"G_Script_EventForString", ( byte * ) G_Script_EventForString
}

,
{
	"G_Script_EventMatch_IntInRange", ( byte * ) G_Script_EventMatch_IntInRange
}

,
{
	"G_Script_EventMatch_StringEqual", ( byte * ) G_Script_EventMatch_StringEqual
}

,
{
	"G_refPrintf", ( byte * ) G_refPrintf
}

,
{
	"G_refClientnumForName", ( byte * ) G_refClientnumForName
}

,
{
	"G_UnMuteClient", ( byte * ) G_UnMuteClient
}

,
{
	"G_MuteClient", ( byte * ) G_MuteClient
}

,
{
	"G_RemoveReferee", ( byte * ) G_RemoveReferee
}

,
{
	"G_MakeReferee", ( byte * ) G_MakeReferee
}

,
{
	"G_PlayerBan", ( byte * ) G_PlayerBan
}

,
{
	"Cmd_AuthRcon_f", ( byte * ) Cmd_AuthRcon_f
}

,
{
	"G_refMute_cmd", ( byte * ) G_refMute_cmd
}

,
{
	"G_refWarning_cmd", ( byte * ) G_refWarning_cmd
}

,
{
	"G_refWarmup_cmd", ( byte * ) G_refWarmup_cmd
}

,
{
	"G_refSpeclockTeams_cmd", ( byte * ) G_refSpeclockTeams_cmd
}

,
{
	"G_refRemove_cmd", ( byte * ) G_refRemove_cmd
}

,
{
	"G_refPlayerPut_cmd", ( byte * ) G_refPlayerPut_cmd
}

,
{
	"G_refPause_cmd", ( byte * ) G_refPause_cmd
}

,
{
	"G_refLockTeams_cmd", ( byte * ) G_refLockTeams_cmd
}

,
{
	"G_refAllReady_cmd", ( byte * ) G_refAllReady_cmd
}

,
{
	"G_ref_cmd", ( byte * ) G_ref_cmd
}

,
{
	"G_refHelp_cmd", ( byte * ) G_refHelp_cmd
}

,
{
	"G_refCommandCheck", ( byte * ) G_refCommandCheck
}

,
{
	"SP_props_flamethrower", ( byte * ) SP_props_flamethrower
}

,
{
	"props_flamethrower_init", ( byte * ) props_flamethrower_init
}

,
{
	"props_flamethrower_use", ( byte * ) props_flamethrower_use
}

,
{
	"props_flamethrower_think", ( byte * ) props_flamethrower_think
}

,
{
	"SP_props_footlocker", ( byte * ) SP_props_footlocker
}

,
{
	"props_locker_death", ( byte * ) props_locker_death
}

,
{
	"props_locker_mass", ( byte * ) props_locker_mass
}

,
{
	"props_locker_spawn_item", ( byte * ) props_locker_spawn_item
}

,
{
	"init_locker", ( byte * ) init_locker
}

,
{
	"props_locker_pain", ( byte * ) props_locker_pain
}

,
{
	"props_locker_use", ( byte * ) props_locker_use
}

,
{
	"props_locker_endrattle", ( byte * ) props_locker_endrattle
}

,
{
	"Spawn_Junk", ( byte * ) Spawn_Junk
}

,
{
	"SP_props_statueBRUSH", ( byte * ) SP_props_statueBRUSH
}

,
{
	"SP_props_statue", ( byte * ) SP_props_statue
}

,
{
	"props_statue_touch", ( byte * ) props_statue_touch
}

,
{
	"props_statue_death", ( byte * ) props_statue_death
}

,
{
	"props_statue_animate", ( byte * ) props_statue_animate
}

,
{
	"props_statue_blocked", ( byte * ) props_statue_blocked
}

,
{
	"SP_skyportal", ( byte * ) SP_skyportal
}

,
{
	"SP_props_decor_Scale", ( byte * ) SP_props_decor_Scale
}

,
{
	"SP_props_decorBRUSH", ( byte * ) SP_props_decorBRUSH
}

,
{
	"SP_props_decoration", ( byte * ) SP_props_decoration
}

,
{
	"props_touch", ( byte * ) props_touch
}

,
{
	"Use_props_decoration", ( byte * ) Use_props_decoration
}

,
{
	"props_decoration_death", ( byte * ) props_decoration_death
}

,
{
	"props_decoration_animate", ( byte * ) props_decoration_animate
}

,
{
	"SP_props_snowGenerator", ( byte * ) SP_props_snowGenerator
}

,
{
	"props_snowGenerator_use", ( byte * ) props_snowGenerator_use
}

,
{
	"props_snowGenerator_think", ( byte * ) props_snowGenerator_think
}

,
{
	"SP_props_castlebed", ( byte * ) SP_props_castlebed
}

,
{
	"props_castlebed_die", ( byte * ) props_castlebed_die
}

,
{
	"props_castlebed_animate", ( byte * ) props_castlebed_animate
}

,
{
	"props_castlebed_touch", ( byte * ) props_castlebed_touch
}

,
{
	"SP_Props_58x112tablew", ( byte * ) SP_Props_58x112tablew
}

,
{
	"props_58x112tablew_die", ( byte * ) props_58x112tablew_die
}

,
{
	"props_58x112tablew_think", ( byte * ) props_58x112tablew_think
}

,
{
	"SP_Props_Flipping_Table", ( byte * ) SP_Props_Flipping_Table
}

,
{
	"props_flippy_blocked", ( byte * ) props_flippy_blocked
}

,
{
	"props_flippy_table_die", ( byte * ) props_flippy_table_die
}

,
{
	"flippy_table_animate", ( byte * ) flippy_table_animate
}

,
{
	"flippy_table_use", ( byte * ) flippy_table_use
}

,
{
	"SP_Props_Crate32x64", ( byte * ) SP_Props_Crate32x64
}

,
{
	"props_crate32x64_die", ( byte * ) props_crate32x64_die
}

,
{
	"props_crate32x64_think", ( byte * ) props_crate32x64_think
}

,
{
	"SP_crate_32", ( byte * ) SP_crate_32
}

,
{
	"SP_crate_64", ( byte * ) SP_crate_64
}

,
{
	"crate_die", ( byte * ) crate_die
}

,
{
	"crate_animate", ( byte * ) crate_animate
}

,
{
	"touch_crate_64", ( byte * ) touch_crate_64
}

,
{
	"SP_Props_Flamebarrel", ( byte * ) SP_Props_Flamebarrel
}

,
{
	"Props_Barrel_Think", ( byte * ) Props_Barrel_Think
}

,
{
	"Props_Barrel_Die", ( byte * ) Props_Barrel_Die
}

,
{
	"OilSlick_remove", ( byte * ) OilSlick_remove
}

,
{
	"OilSlick_remove_think", ( byte * ) OilSlick_remove_think
}

,
{
	"Props_Barrel_Pain", ( byte * ) Props_Barrel_Pain
}

,
{
	"SP_OilParticles", ( byte * ) SP_OilParticles
}

,
{
	"validOilSlickSpawnPoint", ( byte * ) validOilSlickSpawnPoint
}

,
{
	"Delayed_Leak_Think", ( byte * ) Delayed_Leak_Think
}

,
{
	"OilParticles_think", ( byte * ) OilParticles_think
}

,
{
	"SP_OilSlick", ( byte * ) SP_OilSlick
}

,
{
	"smoker_think", ( byte * ) smoker_think
}

,
{
	"barrel_smoke", ( byte * ) barrel_smoke
}

,
{
	"Props_Barrel_Animate", ( byte * ) Props_Barrel_Animate
}

,
{
	"Props_Barrel_Touch", ( byte * ) Props_Barrel_Touch
}

,
{
	"SP_Props_Desklamp", ( byte * ) SP_Props_Desklamp
}

,
{
	"SP_props_shard_generator", ( byte * ) SP_props_shard_generator
}

,
{
	"Use_Props_Shard_Generator", ( byte * ) Use_Props_Shard_Generator
}

,
{
	"SP_Props_DamageInflictor", ( byte * ) SP_Props_DamageInflictor
}

,
{
	"Use_DamageInflictor", ( byte * ) Use_DamageInflictor
}

,
{
	"SP_Props_ChairChatArm", ( byte * ) SP_Props_ChairChatArm
}

,
{
	"SP_Props_ChairChat", ( byte * ) SP_Props_ChairChat
}

,
{
	"SP_Props_ChateauChair", ( byte * ) SP_Props_ChateauChair
}

,
{
	"SP_Props_ChairSide", ( byte * ) SP_Props_ChairSide
}

,
{
	"SP_Props_ChairHiback", ( byte * ) SP_Props_ChairHiback
}

,
{
	"SP_Props_Chair", ( byte * ) SP_Props_Chair
}

,
{
	"Props_Chair_Skyboxtouch", ( byte * ) Props_Chair_Skyboxtouch
}

,
{
	"Props_Chair_Die", ( byte * ) Props_Chair_Die
}

,
{
	"Prop_Break_Sound", ( byte * ) Prop_Break_Sound
}

,
{
	"Spawn_Shard", ( byte * ) Spawn_Shard
}

,
{
	"Props_Chair_Animate", ( byte * ) Props_Chair_Animate
}

,
{
	"Props_Chair_Touch", ( byte * ) Props_Chair_Touch
}

,
{
	"Prop_Check_Ground", ( byte * ) Prop_Check_Ground
}

,
{
	"Prop_Touch", ( byte * ) Prop_Touch
}

,
{
	"Props_Chair_Think", ( byte * ) Props_Chair_Think
}

,
{
	"Props_Activated", ( byte * ) Props_Activated
}

,
{
	"Props_TurnLightsOff", ( byte * ) Props_TurnLightsOff
}

,
{
	"Just_Got_Thrown", ( byte * ) Just_Got_Thrown
}

,
{
	"SP_Props_Locker_Tall", ( byte * ) SP_Props_Locker_Tall
}

,
{
	"props_locker_tall_die", ( byte * ) props_locker_tall_die
}

,
{
	"locker_tall_think", ( byte * ) locker_tall_think
}

,
{
	"SP_Props_RadioSEVEN", ( byte * ) SP_Props_RadioSEVEN
}

,
{
	"props_radio_dieSEVEN", ( byte * ) props_radio_dieSEVEN
}

,
{
	"SP_Props_Radio", ( byte * ) SP_Props_Radio
}

,
{
	"props_radio_die", ( byte * ) props_radio_die
}

,
{
	"SP_Props_Bench", ( byte * ) SP_Props_Bench
}

,
{
	"props_bench_die", ( byte * ) props_bench_die
}

,
{
	"props_bench_think", ( byte * ) props_bench_think
}

,
{
	"InitProp", ( byte * ) InitProp
}

,
{
	"propExplosion", ( byte * ) propExplosion
}

,
{
	"propExplosionLarge", ( byte * ) propExplosionLarge
}

,
{
	"SP_Dust", ( byte * ) SP_Dust
}

,
{
	"dust_angles_think", ( byte * ) dust_angles_think
}

,
{
	"dust_use", ( byte * ) dust_use
}

,
{
	"SP_SmokeDust", ( byte * ) SP_SmokeDust
}

,
{
	"smokedust_use", ( byte * ) smokedust_use
}

,
{
	"SP_props_gunsparks", ( byte * ) SP_props_gunsparks
}

,
{
	"SP_props_sparks", ( byte * ) SP_props_sparks
}

,
{
	"sparks_angles_think", ( byte * ) sparks_angles_think
}

,
{
	"Psparks_think", ( byte * ) Psparks_think
}

,
{
	"PGUNsparks_use", ( byte * ) PGUNsparks_use
}

,
{
	"prop_smoke", ( byte * ) prop_smoke
}

,
{
	"Psmoke_think", ( byte * ) Psmoke_think
}

,
{
	"SP_props_box_64", ( byte * ) SP_props_box_64
}

,
{
	"touch_props_box_64", ( byte * ) touch_props_box_64
}

,
{
	"SP_props_box_48", ( byte * ) SP_props_box_48
}

,
{
	"touch_props_box_48", ( byte * ) touch_props_box_48
}

,
{
	"SP_props_box_32", ( byte * ) SP_props_box_32
}

,
{
	"touch_props_box_32", ( byte * ) touch_props_box_32
}

,
{
	"moveit", ( byte * ) moveit
}

,
{
	"DropToFloor", ( byte * ) DropToFloor
}

,
{
	"DropToFloorG", ( byte * ) DropToFloorG
}

,
{
	"G_smvRunCamera", ( byte * ) G_smvRunCamera
}

,
{
	"G_smvAllRemoveSingleClient", ( byte * ) G_smvAllRemoveSingleClient
}

,
{
	"G_smvRemoveInvalidClients", ( byte * ) G_smvRemoveInvalidClients
}

,
{
	"G_smvUpdateClientCSList", ( byte * ) G_smvUpdateClientCSList
}

,
{
	"G_smvRegenerateClients", ( byte * ) G_smvRegenerateClients
}

,
{
	"G_smvGenerateClientList", ( byte * ) G_smvGenerateClientList
}

,
{
	"G_smvRemoveEntityInMVList", ( byte * ) G_smvRemoveEntityInMVList
}

,
{
	"G_smvLocateEntityInMVList", ( byte * ) G_smvLocateEntityInMVList
}

,
{
	"G_smvAddView", ( byte * ) G_smvAddView
}

,
{
	"G_smvDel_cmd", ( byte * ) G_smvDel_cmd
}

,
{
	"G_smvAddTeam_cmd", ( byte * ) G_smvAddTeam_cmd
}

,
{
	"G_smvAdd_cmd", ( byte * ) G_smvAdd_cmd
}

,
{
	"G_smvCommands", ( byte * ) G_smvCommands
}

,
{
	"G_LinkDebris", ( byte * ) G_LinkDebris
}

,
{
	"G_LinkDamageParents", ( byte * ) G_LinkDamageParents
}

,
{
	"SP_func_debris", ( byte * ) SP_func_debris
}

,
{
	"G_AllocDebrisChunk", ( byte * ) G_AllocDebrisChunk
}

,
{
	"SP_func_brushmodel", ( byte * ) SP_func_brushmodel
}

,
{
	"func_brushmodel_delete", ( byte * ) func_brushmodel_delete
}

,
{
	"SP_func_constructible", ( byte * ) SP_func_constructible
}

,
{
	"func_constructiblespawn", ( byte * ) func_constructiblespawn
}

,
{
	"func_constructible_underconstructionthink", ( byte * ) func_constructible_underconstructionthink
}

,
{
	"func_constructible_explode", ( byte * ) func_constructible_explode
}

,
{
	"func_constructible_spawn", ( byte * ) func_constructible_spawn
}

,
{
	"func_constructible_use", ( byte * ) func_constructible_use
}

,
{
	"InitConstructible", ( byte * ) InitConstructible
}

,
{
	"G_Activate", ( byte * ) G_Activate
}

,
{
	"SP_func_invisible_user", ( byte * ) SP_func_invisible_user
}

,
{
	"use_invisible_user", ( byte * ) use_invisible_user
}

,
{
	"SP_func_explosive", ( byte * ) SP_func_explosive
}

,
{
	"SP_target_explosion", ( byte * ) SP_target_explosion
}

,
{
	"target_explosion_use", ( byte * ) target_explosion_use
}

,
{
	"InitExplosive", ( byte * ) InitExplosive
}

,
{
	"func_explosive_spawn", ( byte * ) func_explosive_spawn
}

,
{
	"func_explosive_alert", ( byte * ) func_explosive_alert
}

,
{
	"func_explosive_use", ( byte * ) func_explosive_use
}

,
{
	"func_explosive_touch", ( byte * ) func_explosive_touch
}

,
{
	"func_explosive_explode", ( byte * ) func_explosive_explode
}

,
{
	"BecomeExplosion", ( byte * ) BecomeExplosion
}

,
{
	"ThrowDebris", ( byte * ) ThrowDebris
}

,
{
	"SP_target_effect", ( byte * ) SP_target_effect
}

,
{
	"target_effect", ( byte * ) target_effect
}

,
{
	"SP_func_door_rotating", ( byte * ) SP_func_door_rotating
}

,
{
	"SP_func_pendulum", ( byte * ) SP_func_pendulum
}

,
{
	"SP_func_bobbing", ( byte * ) SP_func_bobbing
}

,
{
	"SP_func_rotating", ( byte * ) SP_func_rotating
}

,
{
	"Use_Func_Rotate", ( byte * ) Use_Func_Rotate
}

,
{
	"SP_func_static", ( byte * ) SP_func_static
}

,
{
	"SP_func_leaky", ( byte * ) SP_func_leaky
}

,
{
	"G_BlockThink", ( byte * ) G_BlockThink
}

,
{
	"Static_Pain", ( byte * ) Static_Pain
}

,
{
	"Use_Static", ( byte * ) Use_Static
}

,
{
	"SP_func_train_rotating", ( byte * ) SP_func_train_rotating
}

,
{
	"Think_SetupTrainTargets_rotating", ( byte * ) Think_SetupTrainTargets_rotating
}

,
{
	"Reached_Train_rotating", ( byte * ) Reached_Train_rotating
}

,
{
	"Think_BeginMoving_rotating", ( byte * ) Think_BeginMoving_rotating
}

,
{
	"SP_func_train", ( byte * ) SP_func_train
}

,
{
	"SP_info_limbo_camera", ( byte * ) SP_info_limbo_camera
}

,
{
	"info_limbo_camera_setup", ( byte * ) info_limbo_camera_setup
}

,
{
	"SP_info_train_spline_main", ( byte * ) SP_info_train_spline_main
}

,
{
	"SP_path_corner_2", ( byte * ) SP_path_corner_2
}

,
{
	"SP_path_corner", ( byte * ) SP_path_corner
}

,
{
	"Think_SetupTrainTargets", ( byte * ) Think_SetupTrainTargets
}

,
{
	"Reached_Train", ( byte * ) Reached_Train
}

,
{
	"Think_BeginMoving", ( byte * ) Think_BeginMoving
}

,
{
	"SP_func_button", ( byte * ) SP_func_button
}

,
{
	"Touch_Button", ( byte * ) Touch_Button
}

,
{
	"SP_func_plat", ( byte * ) SP_func_plat
}

,
{
	"SpawnPlatTrigger", ( byte * ) SpawnPlatTrigger
}

,
{
	"Touch_PlatCenterTrigger", ( byte * ) Touch_PlatCenterTrigger
}

,
{
	"Touch_Plat", ( byte * ) Touch_Plat
}

,
{
	"SP_func_secret", ( byte * ) SP_func_secret
}

,
{
	"SP_func_door", ( byte * ) SP_func_door
}

,
{
	"G_TryDoor", ( byte * ) G_TryDoor
}

,
{
	"DoorSetSounds", ( byte * ) DoorSetSounds
}

,
{
	"Door_reverse_sounds", ( byte * ) Door_reverse_sounds
}

,
{
	"finishSpawningKeyedMover", ( byte * ) finishSpawningKeyedMover
}

,
{
	"findNonAIBrushTargeter", ( byte * ) findNonAIBrushTargeter
}

,
{
	"Think_MatchTeam", ( byte * ) Think_MatchTeam
}

,
{
	"Think_SpawnNewDoorTrigger", ( byte * ) Think_SpawnNewDoorTrigger
}

,
{
	"Touch_DoorTrigger", ( byte * ) Touch_DoorTrigger
}

,
{
	"Blocked_DoorRotate", ( byte * ) Blocked_DoorRotate
}

,
{
	"Blocked_Door", ( byte * ) Blocked_Door
}

,
{
	"InitMoverRotate", ( byte * ) InitMoverRotate
}

,
{
	"InitMover", ( byte * ) InitMover
}

,
{
	"Use_BinaryMover", ( byte * ) Use_BinaryMover
}

,
{
	"Use_TrinaryMover", ( byte * ) Use_TrinaryMover
}

,
{
	"Reached_TrinaryMover", ( byte * ) Reached_TrinaryMover
}

,
{
	"IsBinaryMoverBlocked", ( byte * ) IsBinaryMoverBlocked
}

,
{
	"Reached_BinaryMover", ( byte * ) Reached_BinaryMover
}

,
{
	"ReturnToPos1Rotate", ( byte * ) ReturnToPos1Rotate
}

,
{
	"GotoPos3", ( byte * ) GotoPos3
}

,
{
	"ReturnToPos2", ( byte * ) ReturnToPos2
}

,
{
	"ReturnToPos1", ( byte * ) ReturnToPos1
}

,
{
	"MatchTeamReverseAngleOnSlaves", ( byte * ) MatchTeamReverseAngleOnSlaves
}

,
{
	"MatchTeam", ( byte * ) MatchTeam
}

,
{
	"SetMoverState", ( byte * ) SetMoverState
}

,
{
	"G_RunMover", ( byte * ) G_RunMover
}

,
{
	"G_MoverTeam", ( byte * ) G_MoverTeam
}

,
{
	"G_MoverPush", ( byte * ) G_MoverPush
}

,
{
	"G_TryPushingEntity", ( byte * ) G_TryPushingEntity
}

,
{
	"G_TestEntityMoveTowardsPos", ( byte * ) G_TestEntityMoveTowardsPos
}

,
{
	"G_TestEntityDropToFloor", ( byte * ) G_TestEntityDropToFloor
}

,
{
	"G_TestEntityPosition", ( byte * ) G_TestEntityPosition
}

,
{
	"fire_mortar", ( byte * ) fire_mortar
}

,
{
	"visible", ( byte * ) visible
}

,
{
	"fire_lead", ( byte * ) fire_lead
}

,
{
	"fire_flamebarrel", ( byte * ) fire_flamebarrel
}

,
{
	"fire_rocket", ( byte * ) fire_rocket
}

,
{
	"fire_grenade", ( byte * ) fire_grenade
}

,
{
	"G_LandmineSnapshotCallback", ( byte * ) G_LandmineSnapshotCallback
}

,
{
	"G_LandminePrime", ( byte * ) G_LandminePrime
}

,
{
	"LandminePostThink", ( byte * ) LandminePostThink
}

,
{
	"G_LandmineThink", ( byte * ) G_LandmineThink
}

,
{
	"sEntWillTriggerMine", ( byte * ) sEntWillTriggerMine
}

,
{
	"G_TripMinePrime", ( byte * ) G_TripMinePrime
}

,
{
	"G_TripMineThink", ( byte * ) G_TripMineThink
}

,
{
	"LandMinePostTrigger", ( byte * ) LandMinePostTrigger
}

,
{
	"LandMineTrigger", ( byte * ) LandMineTrigger
}

,
{
	"G_FreeSatchel", ( byte * ) G_FreeSatchel
}

,
{
	"G_ExplodeSatchels", ( byte * ) G_ExplodeSatchels
}

,
{
	"G_ExplodeMines", ( byte * ) G_ExplodeMines
}

,
{
	"G_HasDroppedItem", ( byte * ) G_HasDroppedItem
}

,
{
	"G_FindSatchel", ( byte * ) G_FindSatchel
}

,
{
	"G_SweepForLandmines", ( byte * ) G_SweepForLandmines
}

,
{
	"G_CountTeamLandmines", ( byte * ) G_CountTeamLandmines
}

,
{
	"G_FadeItems", ( byte * ) G_FadeItems
}

,
{
	"DynaFree", ( byte * ) DynaFree
}

,
{
	"DynaSink", ( byte * ) DynaSink
}

,
{
	"fire_flamechunk", ( byte * ) fire_flamechunk
}

,
{
	"G_RunFlamechunk", ( byte * ) G_RunFlamechunk
}

,
{
	"G_FlameDamage", ( byte * ) G_FlameDamage
}

,
{
	"G_BurnTarget", ( byte * ) G_BurnTarget
}

,
{
	"G_PredictMissile", ( byte * ) G_PredictMissile
}

,
{
	"G_PredictBounceMissile", ( byte * ) G_PredictBounceMissile
}

,
{
	"G_RunMissile", ( byte * ) G_RunMissile
}

,
{
	"Landmine_Check_Ground", ( byte * ) Landmine_Check_Ground
}

,
{
	"G_RunBomb", ( byte * ) G_RunBomb
}

,
{
	"G_MissileDie", ( byte * ) G_MissileDie
}

,
{
	"G_ExplodeMissile", ( byte * ) G_ExplodeMissile
}

,
{
	"M_think", ( byte * ) M_think
}

,
{
	"G_MissileImpact", ( byte * ) G_MissileImpact
}

,
{
	"G_BounceMissile", ( byte * ) G_BounceMissile
}

,
{
	"G_TempTraceIgnorePlayersAndBodies", ( byte * ) G_TempTraceIgnorePlayersAndBodies
}

,
{
	"G_TempTraceIgnoreEntity", ( byte * ) G_TempTraceIgnoreEntity
}

,
{
	"G_ResetTempTraceIgnoreEnts", ( byte * ) G_ResetTempTraceIgnoreEnts
}

,
{
	"G_InitTempTraceIgnoreEnts", ( byte * ) G_InitTempTraceIgnoreEnts
}

,
{
	"SP_misc_commandmap_marker", ( byte * ) SP_misc_commandmap_marker
}

,
{
	"SP_misc_landmine", ( byte * ) SP_misc_landmine
}

,
{
	"landmine_setup", ( byte * ) landmine_setup
}

,
{
	"SP_misc_constructiblemarker", ( byte * ) SP_misc_constructiblemarker
}

,
{
	"constructiblemarker_setup", ( byte * ) constructiblemarker_setup
}

,
{
	"SP_misc_firetrails", ( byte * ) SP_misc_firetrails
}

,
{
	"misc_firetrails_think", ( byte * ) misc_firetrails_think
}

,
{
	"firetrail_use", ( byte * ) firetrail_use
}

,
{
	"firetrail_die", ( byte * ) firetrail_die
}

,
{
	"SP_misc_spawner", ( byte * ) SP_misc_spawner
}

,
{
	"misc_spawner_use", ( byte * ) misc_spawner_use
}

,
{
	"misc_spawner_think", ( byte * ) misc_spawner_think
}

,
{
	"SP_misc_flak", ( byte * ) SP_misc_flak
}

,
{
	"flak_spawn", ( byte * ) flak_spawn
}

,
{
	"Flak_Animate", ( byte * ) Flak_Animate
}

,
{
	"SP_mg42", ( byte * ) SP_mg42
}

,
{
	"mg42_spawn", ( byte * ) mg42_spawn
}

,
{
	"mg42_use", ( byte * ) mg42_use
}

,
{
	"mg42_die", ( byte * ) mg42_die
}

,
{
	"mg42_stopusing", ( byte * ) mg42_stopusing
}

,
{
	"mg42_think", ( byte * ) mg42_think
}

,
{
	"mg42_track", ( byte * ) mg42_track
}

,
{
	"mg42_fire", ( byte * ) mg42_fire
}

,
{
	"mg42_touch", ( byte * ) mg42_touch
}

,
{
	"SP_aagun", ( byte * ) SP_aagun
}

,
{
	"aagun_spawn", ( byte * ) aagun_spawn
}

,
{
	"aagun_touch", ( byte * ) aagun_touch
}

,
{
	"aagun_fire", ( byte * ) aagun_fire
}

,
{
	"aagun_stopusing", ( byte * ) aagun_stopusing
}

,
{
	"aagun_think", ( byte * ) aagun_think
}

,
{
	"aagun_track", ( byte * ) aagun_track
}

,
{
	"aagun_die", ( byte * ) aagun_die
}

,
{
	"aagun_use", ( byte * ) aagun_use
}

,
{
	"clamp_hweapontofirearc", ( byte * ) clamp_hweapontofirearc
}

,
{
	"clamp_playerbehindgun", ( byte * ) clamp_playerbehindgun
}

,
{
	"Fire_Lead_Ext", ( byte * ) Fire_Lead_Ext
}

,
{
	"flakPuff", ( byte * ) flakPuff
}

,
{
	"SP_dlight", ( byte * ) SP_dlight
}

,
{
	"use_dlight", ( byte * ) use_dlight
}

,
{
	"shutoff_dlight", ( byte * ) shutoff_dlight
}

,
{
	"dlight_finish_spawning", ( byte * ) dlight_finish_spawning
}

,
{
	"SP_corona", ( byte * ) SP_corona
}

,
{
	"use_corona", ( byte * ) use_corona
}

,
{
	"SP_shooter_grenade", ( byte * ) SP_shooter_grenade
}

,
{
	"SP_shooter_rocket", ( byte * ) SP_shooter_rocket
}

,
{
	"SP_shooter_mortar", ( byte * ) SP_shooter_mortar
}

,
{
	"InitShooter", ( byte * ) InitShooter
}

,
{
	"Use_Shooter", ( byte * ) Use_Shooter
}

,
{
	"SP_misc_portal_camera", ( byte * ) SP_misc_portal_camera
}

,
{
	"SP_misc_portal_surface", ( byte * ) SP_misc_portal_surface
}

,
{
	"locateCamera", ( byte * ) locateCamera
}

,
{
	"SP_misc_light_surface", ( byte * ) SP_misc_light_surface
}

,
{
	"SP_misc_vis_dummy_multiple", ( byte * ) SP_misc_vis_dummy_multiple
}

,
{
	"SP_misc_vis_dummy", ( byte * ) SP_misc_vis_dummy
}

,
{
	"locateMaster", ( byte * ) locateMaster
}

,
{
	"SP_misc_gamemodel", ( byte * ) SP_misc_gamemodel
}

,
{
	"SP_misc_model", ( byte * ) SP_misc_model
}

,
{
	"SP_misc_spotlight", ( byte * ) SP_misc_spotlight
}

,
{
	"spotlight_finish_spawning", ( byte * ) spotlight_finish_spawning
}

,
{
	"use_spotlight", ( byte * ) use_spotlight
}

,
{
	"SP_misc_grabber_trap", ( byte * ) SP_misc_grabber_trap
}

,
{
	"grabber_wake_touch", ( byte * ) grabber_wake_touch
}

,
{
	"grabber_use", ( byte * ) grabber_use
}

,
{
	"grabber_wake", ( byte * ) grabber_wake
}

,
{
	"grabber_pain", ( byte * ) grabber_pain
}

,
{
	"grabber_close", ( byte * ) grabber_close
}

,
{
	"grabber_attack", ( byte * ) grabber_attack
}

,
{
	"grabber_die", ( byte * ) grabber_die
}

,
{
	"grabber_think_hit", ( byte * ) grabber_think_hit
}

,
{
	"grabber_think_idle", ( byte * ) grabber_think_idle
}

,
{
	"SP_misc_teleporter_dest", ( byte * ) SP_misc_teleporter_dest
}

,
{
	"TeleportPlayer", ( byte * ) TeleportPlayer
}

,
{
	"SP_lightJunior", ( byte * ) SP_lightJunior
}

,
{
	"SP_light", ( byte * ) SP_light
}

,
{
	"SP_info_notnull", ( byte * ) SP_info_notnull
}

,
{
	"SP_info_null", ( byte * ) SP_info_null
}

,
{
	"SP_info_camp", ( byte * ) SP_info_camp
}

,
{
	"Svcmd_GameMem_f", ( byte * ) Svcmd_GameMem_f
}

,
{
	"G_InitMemory", ( byte * ) G_InitMemory
}

,
{
	"G_Alloc", ( byte * ) G_Alloc
}

,
{
	"G_resetModeState", ( byte * ) G_resetModeState
}

,
{
	"G_resetRoundState", ( byte * ) G_resetRoundState
}

,
{
	"G_statsPrint", ( byte * ) G_statsPrint
}

,
{
	"G_checkServerToggle", ( byte * ) G_checkServerToggle
}

,
{
	"G_matchInfoDump", ( byte * ) G_matchInfoDump
}

,
{
	"G_printMatchInfo", ( byte * ) G_printMatchInfo
}

,
{
	"G_parseStats", ( byte * ) G_parseStats
}

,
{
	"G_deleteStats", ( byte * ) G_deleteStats
}

,
{
	"G_createStats", ( byte * ) G_createStats
}

,
{
	"G_weapStatIndex_MOD", ( byte * ) G_weapStatIndex_MOD
}

,
{
	"G_addStatsHeadShot", ( byte * ) G_addStatsHeadShot
}

,
{
	"G_addStats", ( byte * ) G_addStats
}

,
{
	"G_spawnPrintf", ( byte * ) G_spawnPrintf
}

,
{
	"G_delayPrint", ( byte * ) G_delayPrint
}

,
{
	"G_globalSound", ( byte * ) G_globalSound
}

,
{
	"G_printFull", ( byte * ) G_printFull
}

,
{
	"G_loadMatchGame", ( byte * ) G_loadMatchGame
}

,
{
	"G_initMatch", ( byte * ) G_initMatch
}

,
{
	"G_RunItem", ( byte * ) G_RunItem
}

,
{
	"G_RunItemProp", ( byte * ) G_RunItemProp
}

,
{
	"G_BounceItem", ( byte * ) G_BounceItem
}

,
{
	"G_SpawnItem", ( byte * ) G_SpawnItem
}

,
{
	"FinishSpawningItem", ( byte * ) FinishSpawningItem
}

,
{
	"Use_Item", ( byte * ) Use_Item
}

,
{
	"Drop_Item", ( byte * ) Drop_Item
}

,
{
	"LaunchItem", ( byte * ) LaunchItem
}

,
{
	"Touch_Item", ( byte * ) Touch_Item
}

,
{
	"Touch_Item_Auto", ( byte * ) Touch_Item_Auto
}

,
{
	"RespawnItem", ( byte * ) RespawnItem
}

,
{
	"Pickup_Health", ( byte * ) Pickup_Health
}

,
{
	"Pickup_Weapon", ( byte * ) Pickup_Weapon
}

,
{
	"G_CanPickupWeapon", ( byte * ) G_CanPickupWeapon
}

,
{
	"G_DropWeapon", ( byte * ) G_DropWeapon
}

,
{
	"G_GetPrimaryWeaponForClient", ( byte * ) G_GetPrimaryWeaponForClient
}

,
{
	"AddMagicAmmo", ( byte * ) AddMagicAmmo
}

,
{
	"Pickup_Ammo", ( byte * ) Pickup_Ammo
}

,
{
	"Add_Ammo", ( byte * ) Add_Ammo
}

,
{
	"Fill_Clip", ( byte * ) Fill_Clip
}

,
{
	"AddToClip", ( byte * ) AddToClip
}

,
{
	"Pickup_Holdable", ( byte * ) Pickup_Holdable
}

,
{
	"UseHoldableItem", ( byte * ) UseHoldableItem
}

,
{
	"Pickup_Treasure", ( byte * ) Pickup_Treasure
}

,
{
	"Pickup_Clipboard", ( byte * ) Pickup_Clipboard
}

,
{
	"Pickup_Key", ( byte * ) Pickup_Key
}

,
{
	"Pickup_Powerup", ( byte * ) Pickup_Powerup
}

,
{
	"Cmd_FireTeam_MP_f", ( byte * ) Cmd_FireTeam_MP_f
}

,
{
	"G_FindFreePublicFireteam", ( byte * ) G_FindFreePublicFireteam
}

,
{
	"G_FireteamNumberForString", ( byte * ) G_FireteamNumberForString
}

,
{
	"G_ProposeFireTeamPlayer", ( byte * ) G_ProposeFireTeamPlayer
}

,
{
	"G_ApplyToFireTeam", ( byte * ) G_ApplyToFireTeam
}

,
{
	"G_KickFireTeamPlayer", ( byte * ) G_KickFireTeamPlayer
}

,
{
	"G_WarnFireTeamPlayer", ( byte * ) G_WarnFireTeamPlayer
}

,
{
	"G_DestroyFireteam", ( byte * ) G_DestroyFireteam
}

,
{
	"G_InviteToFireTeam", ( byte * ) G_InviteToFireTeam
}

,
{
	"G_RemoveClientFromFireteams", ( byte * ) G_RemoveClientFromFireteams
}

,
{
	"G_AddClientToFireteam", ( byte * ) G_AddClientToFireteam
}

,
{
	"G_RegisterFireteam", ( byte * ) G_RegisterFireteam
}

,
{
	"G_FindFreeFireteamIdent", ( byte * ) G_FindFreeFireteamIdent
}

,
{
	"G_IsFireteamLeader", ( byte * ) G_IsFireteamLeader
}

,
{
	"G_IsOnFireteam", ( byte * ) G_IsOnFireteam
}

,
{
	"G_UpdateFireteamConfigString", ( byte * ) G_UpdateFireteamConfigString
}

,
{
	"G_CountTeamFireteams", ( byte * ) G_CountTeamFireteams
}

,
{
	"G_GetFireteamTeam", ( byte * ) G_GetFireteamTeam
}

,
{
	"G_FindFreeFireteam", ( byte * ) G_FindFreeFireteam
}

,
{
	"G_configSet", ( byte * ) G_configSet
}

,
{
	"G_Damage", ( byte * ) G_Damage
}

,
{
	"IsArmShot", ( byte * ) IsArmShot
}

,
{
	"IsLegShot", ( byte * ) IsLegShot
}

,
{
	"IsHeadShot", ( byte * ) IsHeadShot
}

,
{
	"G_BuildLeg", ( byte * ) G_BuildLeg
}

,
{
	"G_BuildHead", ( byte * ) G_BuildHead
}

,
{
	"IsHeadShotWeapon", ( byte * ) IsHeadShotWeapon
}

,
{
	"player_die", ( byte * ) player_die
}

,
{
	"body_die", ( byte * ) body_die
}

,
{
	"GibEntity", ( byte * ) GibEntity
}

,
{
	"LookAtKiller", ( byte * ) LookAtKiller
}

,
{
	"TossClientItems", ( byte * ) TossClientItems
}

,
{
	"AddKillScore", ( byte * ) AddKillScore
}

,
{
	"AddScore", ( byte * ) AddScore
}

,
{
	"G_weaponRankings_cmd", ( byte * ) G_weaponRankings_cmd
}

,
{
	"G_weaponStatsLeaders_cmd", ( byte * ) G_weaponStatsLeaders_cmd
}

,
{
	"SortStats", ( byte * ) SortStats
}

,
{
	"G_teamready_cmd", ( byte * ) G_teamready_cmd
}

,
{
	"G_statsall_cmd", ( byte * ) G_statsall_cmd
}

,
{
	"G_weaponStats_cmd", ( byte * ) G_weaponStats_cmd
}

,
{
	"G_speclock_cmd", ( byte * ) G_speclock_cmd
}

,
{
	"G_specinvite_cmd", ( byte * ) G_specinvite_cmd
}

,
{
	"G_scores_cmd", ( byte * ) G_scores_cmd
}

,
{
	"G_say_teamnl_cmd", ( byte * ) G_say_teamnl_cmd
}

,
{
	"G_ready_cmd", ( byte * ) G_ready_cmd
}

,
{
	"G_players_cmd", ( byte * ) G_players_cmd
}

,
{
	"G_pause_cmd", ( byte * ) G_pause_cmd
}

,
{
	"G_lock_cmd", ( byte * ) G_lock_cmd
}

,
{
	"G_commands_cmd", ( byte * ) G_commands_cmd
}

,
{
	"G_noTeamControls", ( byte * ) G_noTeamControls
}

,
{
	"G_cmdDebounce", ( byte * ) G_cmdDebounce
}

,
{
	"G_commandHelp", ( byte * ) G_commandHelp
}

,
{
	"G_commandCheck", ( byte * ) G_commandCheck
}

,
{
	"ClientCommand", ( byte * ) ClientCommand
}

,
{
	"Cmd_SwapPlacesWithBot_f", ( byte * ) Cmd_SwapPlacesWithBot_f
}

,
{
	"Cmd_UnIgnore_f", ( byte * ) Cmd_UnIgnore_f
}

,
{
	"Cmd_TicketTape_f", ( byte * ) Cmd_TicketTape_f
}

,
{
	"Cmd_Ignore_f", ( byte * ) Cmd_Ignore_f
}

,
{
	"Cmd_SelectedObjective_f", ( byte * ) Cmd_SelectedObjective_f
}

,
{
	"Cmd_IntermissionWeaponAccuracies_f", ( byte * ) Cmd_IntermissionWeaponAccuracies_f
}

,
{
	"G_CalcClientAccuracies", ( byte * ) G_CalcClientAccuracies
}

,
{
	"Cmd_IntermissionPlayerKillsDeaths_f", ( byte * ) Cmd_IntermissionPlayerKillsDeaths_f
}

,
{
	"Cmd_IntermissionReady_f", ( byte * ) Cmd_IntermissionReady_f
}

,
{
	"G_MakeUnready", ( byte * ) G_MakeUnready
}

,
{
	"G_MakeReady", ( byte * ) G_MakeReady
}

,
{
	"Cmd_IntermissionWeaponStats_f", ( byte * ) Cmd_IntermissionWeaponStats_f
}

,
{
	"Cmd_WeaponStat_f", ( byte * ) Cmd_WeaponStat_f
}

,
{
	"Cmd_SetSniperSpot_f", ( byte * ) Cmd_SetSniperSpot_f
}

,
{
	"Cmd_SetSpawnPoint_f", ( byte * ) Cmd_SetSpawnPoint_f
}

,
{
	"SetPlayerSpawn", ( byte * ) SetPlayerSpawn
}

,
{
	"G_UpdateSpawnCounts", ( byte * ) G_UpdateSpawnCounts
}

,
{
	"Cmd_Activate2_f", ( byte * ) Cmd_Activate2_f
}

,
{
	"Cmd_Activate_f", ( byte * ) Cmd_Activate_f
}

,
{
	"G_LeaveTank", ( byte * ) G_LeaveTank
}

,
{
	"Do_Activate_f", ( byte * ) Do_Activate_f
}

,
{
	"Do_Activate2_f", ( byte * ) Do_Activate2_f
}

,
{
	"G_TankIsMountable", ( byte * ) G_TankIsMountable
}

,
{
	"G_TankIsOccupied", ( byte * ) G_TankIsOccupied
}

,
{
	"Cmd_InterruptCamera_f", ( byte * ) Cmd_InterruptCamera_f
}

,
{
	"Cmd_SetCameraOrigin_f", ( byte * ) Cmd_SetCameraOrigin_f
}

,
{
	"Cmd_StopCamera_f", ( byte * ) Cmd_StopCamera_f
}

,
{
	"Cmd_StartCamera_f", ( byte * ) Cmd_StartCamera_f
}

,
{
	"Cmd_SetViewpos_f", ( byte * ) Cmd_SetViewpos_f
}

,
{
	"G_canPickupMelee", ( byte * ) G_canPickupMelee
}

,
{
	"Cmd_Vote_f", ( byte * ) Cmd_Vote_f
}

,
{
	"G_FindFreeComplainIP", ( byte * ) G_FindFreeComplainIP
}

,
{
	"Cmd_CallVote_f", ( byte * ) Cmd_CallVote_f
}

,
{
	"Cmd_Where_f", ( byte * ) Cmd_Where_f
}

,
{
	"G_Voice", ( byte * ) G_Voice
}

,
{
	"G_VoiceTo", ( byte * ) G_VoiceTo
}

,
{
	"Cmd_Say_f", ( byte * ) Cmd_Say_f
}

,
{
	"G_Say", ( byte * ) G_Say
}

,
{
	"G_SayTo", ( byte * ) G_SayTo
}

,
{
	"G_EntitySoundNoCut", ( byte * ) G_EntitySoundNoCut
}

,
{
	"G_EntitySound", ( byte * ) G_EntitySound
}

,
{
	"Cmd_FollowCycle_f", ( byte * ) Cmd_FollowCycle_f
}

,
{
	"Cmd_Follow_f", ( byte * ) Cmd_Follow_f
}

,
{
	"Cmd_TeamBot_f", ( byte * ) Cmd_TeamBot_f
}

,
{
	"Cmd_SetWeapons_f", ( byte * ) Cmd_SetWeapons_f
}

,
{
	"Cmd_SetClass_f", ( byte * ) Cmd_SetClass_f
}

,
{
	"Cmd_ResetSetup_f", ( byte * ) Cmd_ResetSetup_f
}

,
{
	"Cmd_Team_f", ( byte * ) Cmd_Team_f
}

,
{
	"G_SetClientWeapons", ( byte * ) G_SetClientWeapons
}

,
{
	"G_IsWeaponDisabled", ( byte * ) G_IsWeaponDisabled
}

,
{
	"G_TeamCount", ( byte * ) G_TeamCount
}

,
{
	"G_IsHeavyWeapon", ( byte * ) G_IsHeavyWeapon
}

,
{
	"G_NumPlayersOnTeam", ( byte * ) G_NumPlayersOnTeam
}

,
{
	"G_NumPlayersWithWeapon", ( byte * ) G_NumPlayersWithWeapon
}

,
{
	"StopFollowing", ( byte * ) StopFollowing
}

,
{
	"SetTeam", ( byte * ) SetTeam
}

,
{
	"G_TeamDataForString", ( byte * ) G_TeamDataForString
}

,
{
	"Cmd_Kill_f", ( byte * ) Cmd_Kill_f
}

,
{
	"Cmd_Noclip_f", ( byte * ) Cmd_Noclip_f
}

,
{
	"Cmd_Notarget_f", ( byte * ) Cmd_Notarget_f
}

,
{
	"Cmd_Nofatigue_f", ( byte * ) Cmd_Nofatigue_f
}

,
{
	"Cmd_God_f", ( byte * ) Cmd_God_f
}

,
{
	"Cmd_Give_f", ( byte * ) Cmd_Give_f
}

,
{
	"ClientNumberFromString", ( byte * ) ClientNumberFromString
}

,
{
	"SanitizeString", ( byte * ) SanitizeString
}

,
{
	"ConcatArgs", ( byte * ) ConcatArgs
}

,
{
	"CheatsOk", ( byte * ) CheatsOk
}

,
{
	"Cmd_Score_f", ( byte * ) Cmd_Score_f
}

,
{
	"G_SendScore", ( byte * ) G_SendScore
}

,
{
	"ClientStoreSurfaceFlags", ( byte * ) ClientStoreSurfaceFlags
}

,
{
	"ClientDisconnect", ( byte * ) ClientDisconnect
}

,
{
	"ClientSpawn", ( byte * ) ClientSpawn
}

,
{
	"SelectSpawnPointFromList", ( byte * ) SelectSpawnPointFromList
}

,
{
	"ClientBegin", ( byte * ) ClientBegin
}

,
{
	"G_ComputeMaxLives", ( byte * ) G_ComputeMaxLives
}

,
{
	"ClientConnect", ( byte * ) ClientConnect
}

,
{
	"ClientUserinfoChanged", ( byte * ) ClientUserinfoChanged
}

,
{
	"G_StartPlayerAppropriateSound", ( byte * ) G_StartPlayerAppropriateSound
}

,
{
	"AddMedicTeamBonus", ( byte * ) AddMedicTeamBonus
}

,
{
	"G_CountTeamMedics", ( byte * ) G_CountTeamMedics
}

,
{
	"SetWolfSpawnWeapons", ( byte * ) SetWolfSpawnWeapons
}

,
{
	"AddWeaponToPlayer", ( byte * ) AddWeaponToPlayer
}

,
{
	"PickTeam", ( byte * ) PickTeam
}

,
{
	"TeamCount", ( byte * ) TeamCount
}

,
{
	"respawn", ( byte * ) respawn
}

,
{
	"reinforce", ( byte * ) reinforce
}

,
{
	"limbo", ( byte * ) limbo
}

,
{
	"SetClientViewAnglePitch", ( byte * ) SetClientViewAnglePitch
}

,
{
	"SetClientViewAngle", ( byte * ) SetClientViewAngle
}

,
{
	"CopyToBodyQue", ( byte * ) CopyToBodyQue
}

,
{
	"BodySink", ( byte * ) BodySink
}

,
{
	"BodySink2", ( byte * ) BodySink2
}

,
{
	"BodyUnlink", ( byte * ) BodyUnlink
}

,
{
	"InitBodyQue", ( byte * ) InitBodyQue
}

,
{
	"SelectSpectatorSpawnPoint", ( byte * ) SelectSpectatorSpawnPoint
}

,
{
	"SelectSpawnPoint", ( byte * ) SelectSpawnPoint
}

,
{
	"SelectRandomDeathmatchSpawnPoint", ( byte * ) SelectRandomDeathmatchSpawnPoint
}

,
{
	"SelectNearestDeathmatchSpawnPoint", ( byte * ) SelectNearestDeathmatchSpawnPoint
}

,
{
	"SpotWouldTelefrag", ( byte * ) SpotWouldTelefrag
}

,
{
	"SP_info_player_intermission", ( byte * ) SP_info_player_intermission
}

,
{
	"SP_info_player_start", ( byte * ) SP_info_player_start
}

,
{
	"SP_info_player_checkpoint", ( byte * ) SP_info_player_checkpoint
}

,
{
	"SP_info_player_deathmatch", ( byte * ) SP_info_player_deathmatch
}

,
{
	"G_UpdateCharacter", ( byte * ) G_UpdateCharacter
}

,
{
	"G_RegisterPlayerClasses", ( byte * ) G_RegisterPlayerClasses
}

,
{
	"G_RegisterCharacter", ( byte * ) G_RegisterCharacter
}

,
{
	"G_RemoveFromAllIgnoreLists", ( byte * ) G_RemoveFromAllIgnoreLists
}

,
{
	"G_ClassForString", ( byte * ) G_ClassForString
}

,
{
	"G_CheckMinimumPlayers", ( byte * ) G_CheckMinimumPlayers
}

,
{
	"G_CountBotPlayers", ( byte * ) G_CountBotPlayers
}

,
{
	"G_CountHumanPlayers", ( byte * ) G_CountHumanPlayers
}

,
{
	"G_RemoveRandomBot", ( byte * ) G_RemoveRandomBot
}

,
{
	"G_AddRandomBot", ( byte * ) G_AddRandomBot
}

,
{
	"G_GetArenaInfoByMap", ( byte * ) G_GetArenaInfoByMap
}

,
{
	"G_Trace", ( byte * ) G_Trace
}

,
{
	"G_HistoricalTraceEnd", ( byte * ) G_HistoricalTraceEnd
}

,
{
	"G_HistoricalTraceBegin", ( byte * ) G_HistoricalTraceBegin
}

,
{
	"G_HistoricalTrace", ( byte * ) G_HistoricalTrace
}

,
{
	"G_SwitchBodyPartEntity", ( byte * ) G_SwitchBodyPartEntity
}

,
{
	"G_DettachBodyParts", ( byte * ) G_DettachBodyParts
}

,
{
	"G_AttachBodyParts", ( byte * ) G_AttachBodyParts
}

,
{
	"G_ResetMarkers", ( byte * ) G_ResetMarkers
}

,
{
	"G_AdjustClientPositions", ( byte * ) G_AdjustClientPositions
}

,
{
	"G_StoreClientPosition", ( byte * ) G_StoreClientPosition
}

,
{
	"SP_alarm_box", ( byte * ) SP_alarm_box
}

,
{
	"alarmbox_finishspawning", ( byte * ) alarmbox_finishspawning
}

,
{
	"alarmbox_die", ( byte * ) alarmbox_die
}

,
{
	"alarmbox_use", ( byte * ) alarmbox_use
}

,
{
	"alarmbox_updateparts", ( byte * ) alarmbox_updateparts
}

,
{
	"ClientEndFrame", ( byte * ) ClientEndFrame
}

,
{
	"WolfReviveBbox", ( byte * ) WolfReviveBbox
}

,
{
	"WolfRevivePushEnt", ( byte * ) WolfRevivePushEnt
}

,
{
	"StuckInClient", ( byte * ) StuckInClient
}

,
{
	"SpectatorClientEndFrame", ( byte * ) SpectatorClientEndFrame
}

,
{
	"G_RunClient", ( byte * ) G_RunClient
}

,
{
	"ClientThink", ( byte * ) ClientThink
}

,
{
	"ClientThink_real", ( byte * ) ClientThink_real
}

,
{
	"WolfFindMedic", ( byte * ) WolfFindMedic
}

,
{
	"SendPendingPredictableEvents", ( byte * ) SendPendingPredictableEvents
}

,
{
	"ClientEvents", ( byte * ) ClientEvents
}

,
{
	"ClientIntermissionThink", ( byte * ) ClientIntermissionThink
}

,
{
	"ClientTimerActions", ( byte * ) ClientTimerActions
}

,
{
	"ClientInactivityTimer", ( byte * ) ClientInactivityTimer
}

,
{
	"SpectatorThink", ( byte * ) SpectatorThink
}

,
{
	"G_TouchTriggers", ( byte * ) G_TouchTriggers
}

,
{
	"ClientImpacts", ( byte * ) ClientImpacts
}

,
{
	"ReadyToConstruct", ( byte * ) ReadyToConstruct
}

,
{
	"ReadyToCallArtillery", ( byte * ) ReadyToCallArtillery
}

,
{
	"ClientNeedsAmmo", ( byte * ) ClientNeedsAmmo
}

,
{
	"PushBot", ( byte * ) PushBot
}

,
{
	"G_SetClientSound", ( byte * ) G_SetClientSound
}

,
{
	"P_WorldEffects", ( byte * ) P_WorldEffects
}

,
{
	"P_DamageFeedback", ( byte * ) P_DamageFeedback
}

,
{
	"etpro_FinalizeTracemapClamp", ( byte * ) etpro_FinalizeTracemapClamp
}

,
{
	"BG_GetTracemapGroundCeil", ( byte * ) BG_GetTracemapGroundCeil
}

,
{
	"BG_GetTracemapGroundFloor", ( byte * ) BG_GetTracemapGroundFloor
}

,
{
	"BG_GetGroundHeightAtPoint", ( byte * ) BG_GetGroundHeightAtPoint
}

,
{
	"BG_GetSkyGroundHeightAtPoint", ( byte * ) BG_GetSkyGroundHeightAtPoint
}

,
{
	"BG_GetSkyHeightAtPoint", ( byte * ) BG_GetSkyHeightAtPoint
}

,
{
	"BG_LoadTraceMap", ( byte * ) BG_LoadTraceMap
}

,
{
	"BG_WeapStatForWeapon", ( byte * ) BG_WeapStatForWeapon
}

,
{
	"BG_LoadSpeakerScript", ( byte * ) BG_LoadSpeakerScript
}

,
{
	"BG_SS_StoreSpeaker", ( byte * ) BG_SS_StoreSpeaker
}

,
{
	"BG_SS_DeleteSpeaker", ( byte * ) BG_SS_DeleteSpeaker
}

,
{
	"BG_GetScriptSpeaker", ( byte * ) BG_GetScriptSpeaker
}

,
{
	"BG_GetIndexForSpeaker", ( byte * ) BG_GetIndexForSpeaker
}

,
{
	"BG_NumScriptSpeakers", ( byte * ) BG_NumScriptSpeakers
}

,
{
	"BG_ClearScriptSpeakerPool", ( byte * ) BG_ClearScriptSpeakerPool
}

,
{
	"PM_StepSlideMove", ( byte * ) PM_StepSlideMove
}

,
{
	"PM_SlideMove", ( byte * ) PM_SlideMove
}

,
{
	"BG_ClassSkillForClass", ( byte * ) BG_ClassSkillForClass
}

,
{
	"BG_ClassTextToClass", ( byte * ) BG_ClassTextToClass
}

,
{
	"BG_ClassLetterForNumber", ( byte * ) BG_ClassLetterForNumber
}

,
{
	"BG_ClassnameForNumber", ( byte * ) BG_ClassnameForNumber
}

,
{
	"BG_ShortClassnameForNumber", ( byte * ) BG_ShortClassnameForNumber
}

,
{
	"BG_WeaponIsPrimaryForClassAndTeam", ( byte * ) BG_WeaponIsPrimaryForClassAndTeam
}

,
{
	"BG_ClassHasWeapon", ( byte * ) BG_ClassHasWeapon
}

,
{
	"BG_PlayerClassForPlayerState", ( byte * ) BG_PlayerClassForPlayerState
}

,
{
	"BG_GetPlayerClassInfo", ( byte * ) BG_GetPlayerClassInfo
}

,
{
	"BG_FindCharacter", ( byte * ) BG_FindCharacter
}

,
{
	"BG_FindFreeCharacter", ( byte * ) BG_FindFreeCharacter
}

,
{
	"BG_ClearCharacterPool", ( byte * ) BG_ClearCharacterPool
}

,
{
	"BG_GetCharacterForPlayerstate", ( byte * ) BG_GetCharacterForPlayerstate
}

,
{
	"BG_GetCharacter", ( byte * ) BG_GetCharacter
}

,
{
	"BG_ParseCharacterFile", ( byte * ) BG_ParseCharacterFile
}

,
{
	"BG_StoreCampaignSave", ( byte * ) BG_StoreCampaignSave
}

,
{
	"BG_LoadCampaignSave", ( byte * ) BG_LoadCampaignSave
}

,
{
	"BG_R_RegisterAnimationGroup", ( byte * ) BG_R_RegisterAnimationGroup
}

,
{
	"BG_ClearAnimationPool", ( byte * ) BG_ClearAnimationPool
}

,
{
	"BG_AnimUpdatePlayerStateConditions", ( byte * ) BG_AnimUpdatePlayerStateConditions
}

,
{
	"BG_GetAnimationForIndex", ( byte * ) BG_GetAnimationForIndex
}

,
{
	"BG_GetAnimScriptEvent", ( byte * ) BG_GetAnimScriptEvent
}

,
{
	"BG_GetAnimScriptAnimation", ( byte * ) BG_GetAnimScriptAnimation
}

,
{
	"BG_ClearConditionBitFlag", ( byte * ) BG_ClearConditionBitFlag
}

,
{
	"BG_SetConditionBitFlag", ( byte * ) BG_SetConditionBitFlag
}

,
{
	"BG_GetConditionBitFlag", ( byte * ) BG_GetConditionBitFlag
}

,
{
	"BG_GetConditionValue", ( byte * ) BG_GetConditionValue
}

,
{
	"BG_UpdateConditionValue", ( byte * ) BG_UpdateConditionValue
}

,
{
	"BG_GetAnimString", ( byte * ) BG_GetAnimString
}

,
{
	"BG_AnimScriptEvent", ( byte * ) BG_AnimScriptEvent
}

,
{
	"BG_AnimScriptCannedAnimation", ( byte * ) BG_AnimScriptCannedAnimation
}

,
{
	"BG_AnimScriptAnimation", ( byte * ) BG_AnimScriptAnimation
}

,
{
	"BG_ExecuteCommand", ( byte * ) BG_ExecuteCommand
}

,
{
	"BG_PlayAnimName", ( byte * ) BG_PlayAnimName
}

,
{
	"BG_PlayAnim", ( byte * ) BG_PlayAnim
}

,
{
	"BG_ClearAnimTimer", ( byte * ) BG_ClearAnimTimer
}

,
{
	"BG_FirstValidItem", ( byte * ) BG_FirstValidItem
}

,
{
	"BG_EvaluateConditions", ( byte * ) BG_EvaluateConditions
}

,
{
	"BG_AnimParseAnimScript", ( byte * ) BG_AnimParseAnimScript
}

,
{
	"BG_ParseConditions", ( byte * ) BG_ParseConditions
}

,
{
	"BG_ParseConditionBits", ( byte * ) BG_ParseConditionBits
}

,
{
	"BG_InitWeaponStrings", ( byte * ) BG_InitWeaponStrings
}

,
{
	"BG_CopyStringIntoBuffer", ( byte * ) BG_CopyStringIntoBuffer
}

,
{
	"BG_IndexForString", ( byte * ) BG_IndexForString
}

,
{
	"BG_AnimationForString", ( byte * ) BG_AnimationForString
}

,
{
	"BG_AnimParseError", ( byte * ) BG_AnimParseError
}

,
{
	"BG_StringHashValue_Lwr", ( byte * ) BG_StringHashValue_Lwr
}

,
{
	"BG_StringHashValue", ( byte * ) BG_StringHashValue
}

,
{
	0, 0
}
