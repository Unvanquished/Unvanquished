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

#ifndef ROCKETDATAFORMATTER_H
#define ROCKETDATAFORMATTER_H
#include "client.h"
#include "rocket.h"

#include <Rocket/Controls/DataFormatter.h>

class RocketDataFormatter : public Rocket::Controls::DataFormatter
{
public:
	Rocket::Core::String name;
	int handle;
	char data[ BIG_INFO_STRING ];
	Rocket::Core::String out;

	RocketDataFormatter( const char *name, int handle ) : Rocket::Controls::DataFormatter( name ), name( name ), handle( handle ) { }
	~RocketDataFormatter() { }

	void FormatData( Rocket::Core::String &formatted_data, const Rocket::Core::StringList &raw_data )
	{
		Com_Memset( &data, 0, sizeof( data ) );

		for ( size_t i = 0; i < raw_data.size(); ++i )
		{
			Info_SetValueForKeyRocket( data, va( "%u", ( uint32_t ) i+1 ), raw_data[ i ].CString(), true );
		}
		cgvm.CGameRocketFormatData(handle);
		formatted_data = out;
	}
};
#endif
