/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2000-2009 Darklegion Development

This file is part of Daemon.

Daemon is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Daemon is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "g_local.h"
#include "g_spawn.h"

/**
 * Warning: The following comment contains information, that might be parsed and used by radiant based mapeditors.
 */
/*QUAKED func_group (0 0 0) ?
Used to group brushes together just for editor convenience.  Groups are turned into whole brushes by the utilities.
*/
/*QUAKED light (.65 .65 1) (-8 -8 -8) (8 8 8) LINEAR NOANGLE UNUSED1 UNUSED2 NOGRIDLIGHT
Non-displayed point light source. The -pointscale and -scale arguments to Q3Map2 affect the brightness of these lights. The -skyscale argument affects brightness of entity sun lights.
Especially a target_position is well suited for targeting.
*/
/*QUAKED info_null (0 0.5 0) (-4 -4 -4) (4 4 4)
Used as a positional target for calculations in the utilities (spotlights, etc), but removed during gameplay.
*/
void SP_NULL( gentity_t *self )
{
	G_FreeEntity( self );
}
