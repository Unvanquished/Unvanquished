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

#ifndef G_TYPEDEF_H_
#define G_TYPEDEF_H_

// ---------------
// primitive types
// ---------------

typedef signed int unnamed_t;

// ------------
// struct types
// ------------

typedef struct variatingTime_s     variatingTime_t;
typedef struct gentityConditions_s gentityConditions_t;
typedef struct gentityConfig_s     gentityConfig_t;
typedef struct entityClass_s       entityClass_t;
typedef struct gentity_s           gentity_t;
typedef struct clientSession_s     clientSession_t;
typedef struct namelog_s           namelog_t;
typedef struct clientPersistant_s  clientPersistant_t;
typedef struct unlagged_s          unlagged_t;
typedef struct gclient_s           gclient_t;
typedef struct damageRegion_s      damageRegion_t;
typedef struct spawnQueue_s        spawnQueue_t;
typedef struct buildLog_s          buildLog_t;
typedef struct level_locals_s      level_locals_t;
typedef struct commands_s          commands_t;
typedef struct zap_s               zap_t;

// ----------
// enum types
// ----------

typedef enum
{
	CON_DISCONNECTED,
	CON_CONNECTING,
	CON_CONNECTED
} clientConnected_t;

//status of the warning of certain events
typedef enum
{
	TW_NOT = 0,
	TW_IMMINENT,
	TW_PASSED
} timeWarning_t;

// fate of a buildable
typedef enum
{
	BF_CONSTRUCT,
	BF_DECONSTRUCT,
	BF_REPLACE,
	BF_DESTROY,
	BF_TEAMKILL,
	BF_UNPOWER,
	BF_AUTO
} buildFate_t;

typedef enum {
	VOTE_KICK,
	VOTE_SPECTATE,
	VOTE_MUTE,
	VOTE_UNMUTE,
	VOTE_DENYBUILD,
	VOTE_ALLOWBUILD,
	VOTE_EXTEND,
	VOTE_ADMIT_DEFEAT,
	VOTE_DRAW,
	VOTE_MAP_RESTART,
	VOTE_MAP,
	VOTE_LAYOUT,
	VOTE_NEXT_MAP,
	VOTE_POLL,
	VOTE_BOT_KICK,
	VOTE_BOT_SPECTATE
} voteType_t;

#endif // G_TYPEDEF_H_
