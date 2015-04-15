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

#ifndef SG_DEFINITIONS_H_
#define SG_DEFINITIONS_H_

// ------------------
// shared definitions
// ------------------

#define S_BUILTIN_LAYOUT           "*BUILTIN*"

#define N_( text ) text
#define P_( one, many, count ) ( ( count ) == 1 ? ( one ) : ( many ) )

// factor applied to burning durations for randomization
#define BURN_PERIODS_RAND_MOD ( 1.0f + ( random() - 0.5f ) * 2.0f * BURN_PERIODS_RAND )

// resolves a variatingTime_t to a variated next level.time
#define VariatedLevelTime( variableTime ) level.time + ( variableTime.time + variableTime.variance * crandom() ) * 1000

#define QUEUE_PLUS1(x)  ((( x ) + 1 ) % MAX_CLIENTS )
#define QUEUE_MINUS1(x) ((( x ) + MAX_CLIENTS - 1 ) % MAX_CLIENTS )

#define DECOLOR_OFF '\16'
#define DECOLOR_ON  '\17'

// TODO: Move to HealthComponent.
#define DAMAGE_PURE          0x00000001 /**< Amount won't be modified. */
#define DAMAGE_KNOCKBACK     0x00000002 /**< Push the target in damage direction. */
#define DAMAGE_NO_PROTECTION 0x00000004 /**< Game settings don't prevent damage. */
#define DAMAGE_NO_LOCDAMAGE  0x00000008 /**< Don't apply locational modifier. */

#define MAX_DAMAGE_REGIONS     16
#define MAX_DAMAGE_REGION_TEXT 8192

#define FOFS(x) ((size_t)&(((gentity_t *)0 )->x ))

#endif // SG_DEFINITIONS_H_
