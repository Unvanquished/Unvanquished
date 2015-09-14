/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 2012 Unvanquished Developers

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

// Null interface for libRocket

#include "client/client.h"

void Rocket_Init() {}
void Rocket_Shutdown() {}
void Rocket_Render() {}
void Rocket_Update() {}
void Rocket_InjectMouseMotion( int, int ) {}
void Rocket_LoadDocument( const char* ) {}
void Rocket_LoadCursor( const char* ) {}
void Rocket_DocumentAction( const char*, const char* ) {}
bool Rocket_GetEvent(std::string& ) { return false; }
void Rocket_DeleteEvent() {}
void Rocket_RegisterDataSource( const char* ) {}
void Rocket_DSAddRow( const char*, const char*, const char* ) {}
void Rocket_DSChangeRow( const char*, const char*, const int, const char* ) {}
void Rocket_DSRemoveRow( const char*, const char*, const int ) {}
void Rocket_DSClearTable( const char*, const char* ) {}
void Rocket_SetInnerRML( const char*, const char*, const char*, int ) {}
void Rocket_QuakeToRMLBuffer( const char*, char*, int ) {}
void Rocket_GetEventParameters( char*, int ) {}
void Rocket_RegisterDataFormatter( const char* ) {}
void Rocket_DataFormatterRawData( int, char*, int, char*, int ) {}
void Rocket_DataFormatterFormattedData( int, const char*, bool ) {}
void Rocket_GetElementTag( char*, int ) {}
void Rocket_SetElementDimensions( float, float ) {}
void Rocket_RegisterElement( const char* ) {}
void Rocket_SetAttribute( const char*, const char*, const char*, const char* ) {}
void Rocket_GetAttribute( const char*, const char*, const char*, char*, int ) {}
void Rocket_GetProperty( const char*, void*, int, rocketVarType_t ) {}
void Rocket_GetElementAbsoluteOffset( float*, float* ) {}
void Rocket_SetClass( const char*, bool ) {}
void Rocket_SetPropertyById( const char*, const char*, const char* ) {}
void Rocket_SetActiveContext( int ) {}
void Rocket_AddConsoleText( Str::StringRef ) {}
void Rocket_InitializeHuds( int ) {}
void Rocket_LoadUnit( const char* ) {}
void Rocket_AddUnitToHud( int, const char* ) {}
void Rocket_ShowHud( int ) {}
void Rocket_ClearHud( int ) {}
void Rocket_InitKeys() {}
keyNum_t Rocket_ToQuake( int ) { return K_NONE; }
void Rocket_ProcessKeyInput( int, bool ) {}
void Rocket_ProcessTextInput( int ) {}
void Rocket_MouseMove( int, int ) {}
void Rocket_AddTextElement( const char*, const char*, float, float ) {}
void Rocket_ClearText() {}
void Rocket_RegisterProperty( const char*, const char*, bool, bool, const char* ) {}
void Rocket_ShowScoreboard( const char*, bool ) {}
void Rocket_SetDataSelectIndex( int ) {}
void Rocket_LoadFont( const char* ) {}
