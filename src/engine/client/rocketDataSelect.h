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

// Override for the default dataselect element.

#include <Rocket/Core/Core.h>
#include <Rocket/Controls/ElementFormControlDataSelect.h>


extern "C"
{
#include "client.h"
}

class RocketDataSelect : public Rocket::Controls::ElementFormControlDataSelect
{
public:
	RocketDataSelect( const Rocket::Core::String &tag ) : Rocket::Controls::ElementFormControlDataSelect( tag ), selection( -2 ) { }
	~RocketDataSelect() { }
	void OnUpdate( void )
	{
		extern std::queue< RocketEvent_t * > eventQueue;

		ElementFormControlDataSelect::OnUpdate();

		if ( GetSelection() != selection )
		{
			Rocket::Core::String dataSource = GetAttribute<Rocket::Core::String>( "source", "" );
			Rocket::Core::String dsName = dataSource.Substring( 0, dataSource.Find( "." ) );
			Rocket::Core::String tableName =  dataSource.Substring( dataSource.Find( "." ) + 1, dataSource.Length() );

			if ( dataSource.Empty() )
			{
				return;
			}

			selection = GetSelection();

			// dispatch event so cgame knows about it
			eventQueue.push( new RocketEvent_t( Rocket::Core::String( va( "setDS %s %s %d", dsName.CString(), tableName.CString(), selection ) ) ) );

			// dispatch event so rocket knows about it
			Rocket::Core::Dictionary parameters;
			parameters.Set( "index", va( "%d", selection ) );
			parameters.Set( "datasource", dsName );
			parameters.Set( "table", tableName );
			DispatchEvent( "rowselect", parameters );
		}
	}
private:
	int selection;
};

