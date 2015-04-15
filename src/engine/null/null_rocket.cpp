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

void Rocket_Init( void ) {}
void Rocket_Shutdown( void ) {}
void Rocket_Render( void ) {}
void Rocket_Update( void ) {}
void Rocket_InjectMouseMotion( int x, int y ) {}
void Rocket_LoadDocument( const char *path ) {}
void Rocket_LoadCursor( const char *path ) {}
void Rocket_DocumentAction( const char *name, const char *action ) {}
bool Rocket_GetEvent(std::string& ) { return false; }
void Rocket_DeleteEvent( void ) {}
void Rocket_RegisterDataSource( const char *name ) {}
void Rocket_DSAddRow( const char *name, const char *table, const char *data ) {}
void Rocket_DSChangeRow( const char *name, const char *table, const int row, const char *data ) {}
void Rocket_DSRemoveRow( const char *name, const char *table, const int row ) {}
void Rocket_DSClearTable( const char *name, const char *table ) {}
void Rocket_SetInnerRML( const char *name, const char *id, const char *RML, int parseFlags ) {}
void Rocket_QuakeToRMLBuffer( const char *in, char *out, int length ) {}
void Rocket_GetEventParameters( char *params, int length ) {}
void Rocket_RegisterDataFormatter( const char *name ) {}
void Rocket_DataFormatterRawData( int handle, char *name, int nameLength, char *data, int dataLength ) {}
void Rocket_DataFormatterFormattedData( int handle, const char *data, bool parseQuake ) {}
void Rocket_GetElementTag( char *tag, int length ) {}
void Rocket_SetElementDimensions( float x, float y ) {}
void Rocket_RegisterElement( const char *tag ) {}
void Rocket_SetAttribute( const char *name, const char *id, const char *attribute, const char *value ) {}
void Rocket_GetAttribute( const char *name, const char *id, const char *attribute, char *out, int length ) {}
void Rocket_GetProperty( const char *name, void *out, int len, rocketVarType_t type ) {}
void Rocket_GetElementAbsoluteOffset( float *x, float *y ) {}
void Rocket_SetClass( const char *in, bool activate ) {}
void Rocket_SetPropertyById( const char *id, const char *property, const char *value ) {}
void Rocket_SetActiveContext( int catcher ) {}
void Rocket_AddConsoleText( Str::StringRef ) {}
void Rocket_InitializeHuds( int size ) {}
void Rocket_LoadUnit( const char *path ) {}
void Rocket_AddUnitToHud( int weapon, const char *id ) {}
void Rocket_ShowHud( int weapon ) {}
void Rocket_ClearHud( int weapon ) {}
void Rocket_InitKeys( void ) {}
keyNum_t Rocket_ToQuake( int key ) { return K_NONE; }
void Rocket_ProcessKeyInput( int key, bool down ) {}
void Rocket_ProcessTextInput( int key ) {}
void Rocket_MouseMove( int x, int y ) {}
void Rocket_AddTextElement( const char *text, const char *_class, float x, float y ) {}
void Rocket_ClearText( void ) {}
void Rocket_RegisterProperty( const char *name, const char *defaultValue, bool inherited, bool force_layout, const char *parseAs ) {}
void Rocket_ShowScoreboard( const char *name, bool show ) {}
void Rocket_SetDataSelectIndex( int index ) {}
void Rocket_LoadFont( const char *font ) {}
