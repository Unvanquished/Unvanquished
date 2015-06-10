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

#include "qcommon/q_shared.h"
#include "qcommon/qcommon.h"

cvar_t *cl_shownet;

void CL_Shutdown()
{
}

void CL_Init()
{
	cl_shownet = Cvar_Get( "cl_shownet", "0", CVAR_TEMP );
	// TTimo: localisation, prolly not any use in dedicated / null client
}

void CL_MouseEvent( int dx, int dy, int time )
{
}

void Key_WriteBindings( fileHandle_t f )
{
}

void CL_Frame( int msec )
{
}

void CL_PacketEvent( netadr_t from, msg_t *msg )
{
}

void CL_CharEvent( int c )
{
}

void CL_Disconnect( bool showMainMenu )
{
}

void CL_MapLoading()
{
}

bool CL_GameCommand()
{
	return false; // bk001204 - non-void
}

void CL_KeyEvent( int key, bool down, unsigned time )
{
}

bool UI_GameCommand()
{
	return false;
}

void CL_ForwardCommandToServer( const char *cmd )
{
	Com_Printf( "Unknown command \"%s\"\n", cmd );
}

void CL_ConsolePrint( std::string text )
{
}

void CL_JoystickEvent( int axis, int value, int time )
{
}

void CL_InitKeyCommands()
{
}

void CL_FlushMemory()
{
}

void CL_StartHunkUsers()
{
}

// bk001119 - added new dummy for sv_init.c
void CL_ShutdownAll()
{
}

// TTimo added for win32 dedicated
void Key_ClearStates()
{
}
