/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
Copyright (C) 2004-2006 Tony J. White
Copyright (C) 2011 Dusan Jocic

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

#ifndef _G_ADMIN_H
#define _G_ADMIN_H

#define AP(x)         trap_SendServerCommand(-1, x)
#define CP(x)         trap_SendServerCommand(ent - g_entities, x)
#define CPx(x, y)     trap_SendServerCommand(x, y)
#define ADMP(x)       G_admin_print(ent, x)
#define ADMBP(x)      G_admin_buffer_print(ent, x)
#define ADMBP_begin() G_admin_buffer_begin()
#define ADMBP_end()   G_admin_buffer_end(ent)

#define MAX_ADMIN_LEVELS        999
#define MAX_ADMIN_ADMINS        32768
#define MAX_ADMIN_BANS          1024
#define MAX_ADMIN_NAMELOGS      128
#define MAX_ADMIN_NAMELOG_NAMES 5
#define MAX_ADMIN_FLAGS         64
#define MAX_ADMIN_COMMANDS      64
#define MAX_ADMIN_CMD_LEN       20
#define MAX_ADMIN_BAN_REASON    50

/*
 * 1 - cannot be vote kicked, vote muted
 * 2 - cannot be censored or flood protected TODO
 * 3 - UNUSED
 * 4 - can see team chat as a spectator
 * 5 - can switch teams any time, regardless of balance
 * 6 - does not need to specify a reason for a kick/ban
 * 7 - can call a vote at any time (regardless of a vote being disabled or
 * voting limitations)
 * 8 - does not need to specify a duration for a ban
 * 9 - can run commands from team chat
 * 0 - inactivity rules do not apply to them
 * ! - admin commands cannot be used on them
 * @ - does not show up as an admin in !listplayers
 */
#define ADMF_IMMUNITY           '1'
#define ADMF_NOCENSORFLOOD      '2'         /* TODO */
#define ADMF_STEALTH            '3'
#define ADMF_SPEC_ALLCHAT       '4'
#define ADMF_FORCETEAMCHANGE    '5'
#define ADMF_UNACCOUNTABLE      '6'
#define ADMF_NO_VOTE_LIMIT      '7'
#define ADMF_CAN_PERM_BAN       '8'
#define ADMF_TEAMFTCMD          '9'
#define ADMF_ACTIVITY           '0'

#define ADMF_IMMUTABLE          '!'
#define ADMF_INCOGNITO          '@'

#define MAX_ADMIN_LISTITEMS     20
#define MAX_ADMIN_SHOWBANS      10

typedef struct
{
	char *keyword;
	qboolean ( * handler )( gentity_t *ent, int skiparg );
	char *flag;
	char *function; // used for !help
	char *syntax;   // used for !help
} g_admin_cmd_t;

typedef struct g_admin_level
{
	int  level;
	char name[ MAX_NAME_LENGTH ];
	char flags[ MAX_ADMIN_FLAGS ];
} g_admin_level_t;

typedef struct g_admin_admin
{
	char guid[ 33 ];
	char name[ MAX_NAME_LENGTH ];
	int  level;
	char flags[ MAX_ADMIN_FLAGS ];
} g_admin_admin_t;

typedef struct g_admin_ban
{
	char name[ MAX_NAME_LENGTH ];
	char guid[ 33 ];
	char ip[ 40 ];
	char reason[ MAX_ADMIN_BAN_REASON ];
	char made[ 20 ]; // "YYYY-MM-DD hh:mm:ss"
	int  expires;
	char banner[ MAX_NAME_LENGTH ];
} g_admin_ban_t;

typedef struct g_admin_command
{
	char command[ MAX_ADMIN_CMD_LEN ];
	char exec[ MAX_QPATH ];
	char desc[ 50 ];
	int  levels[ MAX_ADMIN_LEVELS + 1 ];
} g_admin_command_t;

typedef struct g_admin_namelog
{
	char     name[ MAX_ADMIN_NAMELOG_NAMES ][ MAX_NAME_LENGTH ];
	char     ip[ 40 ];
	char     guid[ 33 ];
	int      slot;
	qboolean banned;
} g_admin_namelog_t;

qboolean G_admin_autoregister( gentity_t *ent );
qboolean G_admin_ban_check( char *userinfo, char *reason, int rlen );
qboolean G_admin_cmd_check( gentity_t *ent, qboolean say );
qboolean G_admin_readconfig( gentity_t *ent, int skiparg );
qboolean G_admin_permission( gentity_t *ent, char flag );
qboolean G_admin_name_check( gentity_t *ent, char *name, char *err, int len );
void     G_admin_namelog_update( gclient_t *ent, qboolean disconnect );
int      G_admin_level( gentity_t *ent );
int      G_admin_parse_time( const char *time );

// ! command functions
qboolean G_admin_time( gentity_t *ent, int skiparg );
qboolean G_admin_setlevel( gentity_t *ent, int skiparg );
qboolean G_admin_kick( gentity_t *ent, int skiparg );
qboolean G_admin_adjustban( gentity_t *ent, int skiparg );
qboolean G_admin_ban( gentity_t *ent, int skiparg );
qboolean G_admin_unban( gentity_t *ent, int skiparg );
qboolean G_admin_putteam( gentity_t *ent, int skiparg );
qboolean G_admin_listadmins( gentity_t *ent, int skiparg );
qboolean G_admin_listplayers( gentity_t *ent, int skiparg );
qboolean G_admin_mute( gentity_t *ent, int skiparg );
qboolean G_admin_showbans( gentity_t *ent, int skiparg );
qboolean G_admin_help( gentity_t *ent, int skiparg );
qboolean G_admin_admintest( gentity_t *ent, int skiparg );
qboolean G_admin_cancelvote( gentity_t *ent, int skiparg );
qboolean G_admin_passvote( gentity_t *ent, int skiparg );
qboolean G_admin_spec999( gentity_t *ent, int skiparg );
qboolean G_admin_register( gentity_t *ent, int skiparg );
qboolean G_admin_rename( gentity_t *ent, int skiparg );
qboolean G_admin_restart( gentity_t *ent, int skiparg );
qboolean G_admin_nextmap( gentity_t *ent, int skiparg );
qboolean G_admin_namelog( gentity_t *ent, int skiparg );

void     G_admin_print( gentity_t *ent, char *m );
void     G_admin_buffer_print( gentity_t *ent, char *m );
void     G_admin_buffer_begin( void );
void     G_admin_buffer_end( gentity_t *ent );

void     G_admin_duration( int secs, char *duration, int dursize );
void     G_admin_cleanup( void );
void     G_admin_namelog_cleanup( void );

#endif /* ifndef _G_ADMIN_H */
