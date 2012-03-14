/*
===========================================================================

Wolfenstein: Enemy Territory GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Wolfenstein: Enemy Territory GPL Source Code (Wolf ET Source Code).

Wolf ET Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Wolf ET Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Wolf ET Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Wolf: ET Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Wolf ET Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

// snddma_null.c
// all other sound mixing is portable

#include "../client/client.h"

qboolean SNDDMA_Init( void )
{
	return qfalse;
}

int SNDDMA_GetDMAPos( void )
{
	return 0;
}

void SNDDMA_Shutdown( void )
{
}

void SNDDMA_BeginPainting( void )
{
}

void SNDDMA_Submit( void )
{
}

// bk001119 - added boolean flag, match client/snd_public.h
sfxHandle_t S_RegisterSound( const char *name, qboolean compressed )
{
	return 0;
}

void S_StartLocalSound( sfxHandle_t sfx, int channelNum )
{
}

void S_ClearSoundBuffer( void )
{
}

// TTimo: added for win32 dedicated
void SNDDMA_Activate( void )
{
}

// show_bug.cgi?id=574
int S_GetSoundLength( sfxHandle_t sfxHandle )
{
	Com_Error( ERR_DROP, "null_snddma.c: S_GetSoundLength\n" );
	return 0;
}

void S_UpdateThread( void )
{
}

void S_AddLoopSounds( void )
{
}
