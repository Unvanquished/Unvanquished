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
#ifndef ROCKET_H
#define ROCKET_H
#if defined( __cplusplus )

extern Rocket::Core::Context *context;

class RocketEvent_t
{
public:
	RocketEvent_t( Rocket::Core::Event &event, Rocket::Core::String cmds ) : cmd( cmds )
	{
		targetElement = event.GetTargetElement();
		Parameters = *(event.GetParameters());
	}
	~RocketEvent_t() { }
	Rocket::Core::Element *targetElement;
	Rocket::Core::Dictionary Parameters;
	Rocket::Core::String cmd;
};


extern "C"
{
#endif

void Rocket_Init( void );
void Rocket_Shutdown( void );
void Rocket_Render( void );
void Rocket_Update( void );
void Rocket_InjectMouseMotion( int x, int y );
void Rocket_LoadDocument( const char *path );
void Rocket_LoadCursor( const char *path );
void Rocket_DocumentAction( const char *name, const char *action );
void Rocket_GetEvent( char *event, int length );
void Rocket_DeleteEvent( void );
void Rocket_RegisterDataSource( const char *name );
void Rocket_DSAddRow( const char *name, const char *table, const char *data );
void Rocket_DSChangeRow( const char *name, const char *table, const int row, const char *data );
void Rocket_DSRemoveRow( const char *name, const char *table, const int row );
void Rocket_DSClearTable( const char *name, const char *table );
void Rocket_SetInnerRML( const char *name, const char *id, const char *RML );
void Rocket_GetEventParameters( char *params, int length );
void Rocket_RegisterDataFormatter( const char *name );
void Rocket_DataFormatterRawData( int handle, char *name, int nameLength, char *data, int dataLength );
void Rocket_DataFormatterFormattedData( int handle, const char *data );
int Rocket_ToQuakeKey( const int rocketKey );
void Rocket_GetElementTag( char *tag, int length );
void Rocket_SetElementDimensions( float x, float y );
void Rocket_RegisterElement( const char *tag );
void Rocket_SetAttribute( const char *name, const char *id, const char *attribute, const char *value );
void Rocket_GetAttribute( const char *name, const char *id, const char *attribute, char *out, int length );
void Rocket_GetElementAbsoluteOffset( float *x, float *y );
void Rocket_DrawElementPic( float x, float y, float w, float h, float t1, float s1, float t2, float s2, const char *src );
void Rocket_ClearElementGeometry( void );
#if defined( __cplusplus )
}
#endif
#endif
