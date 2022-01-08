/*
===========================================================================

Unvanquished GPL Source Code
Copyright (C) 2000-2009 Darklegion Development

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

#ifndef SG_ADMIN_H
#define SG_ADMIN_H

struct gentity_t;

#define AP(x)         trap_SendServerCommand(-1, x)
#define CP(x)         trap_SendServerCommand(ent - g_entities, x)
#define CPx(x, y)     trap_SendServerCommand(x, y)
#define ADMP(x)       G_admin_print(ent, x)
#define ADMP_P(x,c)   G_admin_print_plural(ent, x, c)
#define ADMBP_raw(x)  G_admin_buffer_print_raw(ent, x)
#define ADMBP(x)      G_admin_buffer_print(ent, x)
#define ADMBP_begin() G_admin_buffer_begin()
#define ADMBP_end()   G_admin_buffer_end(ent)
#define QQ(s)         "\"" s "\""

#define MAX_ADMIN_FLAG_LEN   20
#define MAX_ADMIN_FLAGS      1024
#define MAX_ADMIN_FLAG_KEYS  128
#define MAX_ADMIN_CMD_LEN    20
#define MAX_ADMIN_BAN_REASON 100

#define MAX_ADMIN_EXPIRED_BANS   64
#define G_ADMIN_BAN_EXPIRED(b,t) ( (b)->expires != 0 && (b)->expires <= (t) )
#define G_ADMIN_BAN_STALE(b,t)   ( (b)->expires != 0 && (b)->expires + ( g_adminRetainExpiredBans.Get() ? 86400 : 0 ) <= (t) )
#define G_ADMIN_BAN_IS_WARNING(b) ( (b)->warnCount < 0 )

/*
 * IMMUNITY - cannot be vote kicked, vote muted
 * NOCENSORFLOOD - cannot be censored or flood protected
 * SPECALLCHAT - can see team chat as a spectator
 * FORCETEAMCHANGE - can switch teams any time, regardless of balance
 * UNACCOUNTABLE - does not need to specify a reason for a kick/ban
 * NOVOTELIMIT - can call a vote at any time (regardless of a vote being
 * disabled or voting limitations)
 * CANPERMBAN - does not need to specify a duration for a ban
 * ACTIVITY - inactivity rules do not apply to them
 * IMMUTABLE - admin commands cannot be used on them
 * INCOGNITO - does not show up as an admin in /listplayers
 * ALLFLAGS - all flags (including command flags) apply to this player
 * ADMINCHAT - receives and can send /a admin messages
 */
#define ADMF_IMMUNITY        "IMMUNITY"
#define ADMF_NOCENSORFLOOD   "NOCENSORFLOOD"
#define ADMF_SPEC_ALLCHAT    "SPECALLCHAT"
#define ADMF_FORCETEAMCHANGE "FORCETEAMCHANGE"
#define ADMF_UNACCOUNTABLE   "UNACCOUNTABLE"
#define ADMF_NO_VOTE_LIMIT   "NOVOTELIMIT"
#define ADMF_CAN_PERM_BAN    "CANPERMBAN"
#define ADMF_ACTIVITY        "ACTIVITY"

#define ADMF_IMMUTABLE       "IMMUTABLE"
#define ADMF_INCOGNITO       "INCOGNITO"
#define ADMF_ALLFLAGS        "ALLFLAGS"
#define ADMF_ADMINCHAT       "ADMINCHAT"

#define MAX_ADMIN_LISTITEMS  20
#define MAX_ADMIN_SHOWBANS   10

struct g_admin_cmd_t;
struct g_admin_level_t;
struct g_admin_ban_t;
struct g_admin_command_t;
struct g_admin_admin_t;

#define ADDRLEN 16

/*
addr_ts are passed as "arg" to admin_search for IP address matching
admin_search prints (char *)arg, so the stringified address needs to be first
*/
enum
{
  IPv4,
  IPv6
};
struct addr_t
{
	char str[ 44 ];
	int type;

	byte addr[ ADDRLEN ];
	int  mask;
};

struct g_admin_spec_t;

void            G_admin_register_cmds();
void            G_admin_unregister_cmds();
void            G_admin_cmdlist( gentity_t *ent );
void            G_admin_writeconfig();
void            G_admin_pubkey();

bool        G_admin_ban_check( gentity_t *ent, char *reason, int rlen );
bool        G_admin_cmd_check( gentity_t *ent );
bool        G_admin_readconfig( gentity_t *ent );
bool        G_admin_permission( gentity_t *ent, const char *flag );
bool        G_admin_name_check( gentity_t *ent, const char *name, char *err, int len );
g_admin_admin_t *G_admin_admin( const char *guid );
void            G_admin_authlog( gentity_t *ent );

g_admin_spec_t  *G_admin_match_spec( gentity_t *ent );
int G_admin_joindelay( g_admin_spec_t const* ptr );

// admin command functions
bool        G_admin_time( gentity_t *ent );
bool        G_admin_setlevel( gentity_t *ent );
bool        G_admin_kick( gentity_t *ent );
bool        G_admin_adjustban( gentity_t *ent );
bool        G_admin_ban( gentity_t *ent );
bool        G_admin_unban( gentity_t *ent );
bool        G_admin_putteam( gentity_t *ent );
bool        G_admin_speclock( gentity_t *ent );
bool        G_admin_specunlock( gentity_t *ent );
bool        G_admin_listadmins( gentity_t *ent );
bool        G_admin_listinactive( gentity_t *ent );
bool        G_admin_listlayouts( gentity_t *ent );
bool        G_admin_listgeoip( gentity_t *ent );
bool        G_admin_listplayers( gentity_t *ent );
bool        G_admin_changemap( gentity_t *ent );
bool        G_admin_warn( gentity_t *ent );
bool        G_admin_mute( gentity_t *ent );
bool        G_admin_denybuild( gentity_t *ent );
bool        G_admin_showbans( gentity_t *ent );
bool        G_admin_adminhelp( gentity_t *ent );
bool        G_admin_admintest( gentity_t *ent );
bool        G_admin_allready( gentity_t *ent );
bool        G_admin_endvote( gentity_t *ent );
bool        G_admin_spec999( gentity_t *ent );
bool        G_admin_rename( gentity_t *ent );
bool        G_admin_restart( gentity_t *ent );
bool        G_admin_nextmap( gentity_t *ent );
bool        G_admin_namelog( gentity_t *ent );
bool        G_admin_lock( gentity_t *ent );
bool        G_admin_flaglist( gentity_t *ent );
bool        G_admin_flag( gentity_t *ent );
bool        G_admin_pause( gentity_t *ent );
bool        G_admin_builder( gentity_t *ent );
bool        G_admin_buildlog( gentity_t *ent );
bool        G_admin_revert( gentity_t *ent );
bool        G_admin_l0( gentity_t *ent );  // AA-QVM 1.2
bool        G_admin_l1( gentity_t *ent );  // AA-QVM 1.2
bool        G_admin_register( gentity_t *ent );  // AA-QVM 1.2
bool        G_admin_unregister( gentity_t *ent );  // AA-QVM 1.2
bool        G_admin_bot( gentity_t *ent );
bool        G_admin_navgen( gentity_t *ent );

void            G_admin_print( gentity_t *ent, Str::StringRef m );
void            G_admin_print_plural( gentity_t *ent, Str::StringRef m, int number );
void            G_admin_buffer_print_raw( gentity_t *ent, Str::StringRef m );
void            G_admin_buffer_print( gentity_t *ent, Str::StringRef m );
void            G_admin_buffer_begin();
void            G_admin_buffer_end( gentity_t *ent );

void            G_admin_duration( int secs, char *time, int timesize, char *duration, int dursize );
void            G_admin_cleanup();

void Cmd_Pubkey_Identify_f( gentity_t *ent );
qtime_t* G_admin_lastSeen( g_admin_admin_t* admin );

#endif /* ifndef SG_ADMIN_H */
