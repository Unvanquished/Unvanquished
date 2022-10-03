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

#ifndef SG_DEFINITIONS_H_
#define SG_DEFINITIONS_H_

// ------------------
// shared definitions
// ------------------

#define S_BUILTIN_LAYOUT           "*BUILTIN*"

// These translation markers are used for messages transmitted to clients as arguments of print_tr* commands.
// P_ immediately chooses the appropriate pluralization for English, in case the message is not translated;
// the count is also transmitted with the command so that the correct pluralization can be chosen later
// for another language.
#define N_( text ) text
#define P_( one, many, count ) ( ( count ) == 1 ? ( one ) : ( many ) )

// resolves a variatingTime_t to a variated next level.time
#define VariatedLevelTime( variableTime ) (level.time + ( variableTime.time + variableTime.variance * crandom() ) * 1000)

#define QUEUE_PLUS1(x)  ((( x ) + 1 ) % MAX_CLIENTS )
#define QUEUE_MINUS1(x) ((( x ) + MAX_CLIENTS - 1 ) % MAX_CLIENTS )


// TODO: Move to HealthComponent.
// Flags for the CBSE Damage message
#define DAMAGE_PURE          0x00000001 /**< Amount won't be modified. */
#define DAMAGE_KNOCKBACK     0x00000002 /**< Push the target in damage direction. */
#define DAMAGE_NO_PROTECTION 0x00000004 /**< Game settings don't prevent damage. */
#define DAMAGE_NO_LOCDAMAGE  0x00000008 /**< Don't apply locational modifier. */

#define MAX_DAMAGE_REGIONS     16
#define MAX_DAMAGE_REGION_TEXT 8192

#define MAX_NAME_CHARACTERS 32

#define FOFS(x) ((size_t)&(((gentity_t *)0 )->x ))


// ----------
// enum types
// ----------

enum clientConnected_t
{
	CON_DISCONNECTED,
	CON_CONNECTING,
	CON_CONNECTED
};

//status of the warning of certain events
enum timeWarning_t
{
	TW_NOT = 0,
	TW_IMMINENT,
	TW_PASSED
};

// fate of a buildable
enum buildFate_t
{
	BF_CONSTRUCT,
	BF_DECONSTRUCT,
	BF_REPLACE,
	BF_DESTROY,
	BF_TEAMKILL,
	BF_UNPOWER,
	BF_AUTO
};

#endif // SG_DEFINITIONS_H_
