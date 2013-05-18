/*
 = =*=========================================================================

 Daemon GPL Source Code
 Copyright (C) 2013 Unvanquished Developers

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

#ifndef ROCKETPROGRESSBAR_H
#define ROCKETPROGRESSBAR_H

#include <Rocket/ProgressBar/ElementProgressBar.h>

extern "C"
{
#include "client.h"
}

class RocketProgressBar : public Rocket::ProgressBar::ElementProgressBar
{
public:
	RocketProgressBar( const Rocket::Core::String &tag ) : Rocket::ProgressBar::ElementProgressBar( tag ) {}
	void OnUpdate( void )
	{
		float value;
		const char *str = GetAttribute<Rocket::Core::String>( "src", "" ).CString();

		if ( *str )
		{
			Cmd_TokenizeString( str );
			value = _vmf( VM_Call( cgvm, CG_ROCKET_PROGRESSBARVALUE ) );
			Com_Printf( "%f\n", value );

			if ( value != GetValue() )
			{
				SetValue( value );
			}
		}
	}

	void OnRender( void )
	{
		OnUpdate();
		Rocket::ProgressBar::ElementProgressBar::OnRender();
	}

	bool GetIntrinsicDimensions( Rocket::Core::Vector2f &dimensions )
	{
		// Calculate the x dimension.
		if ( HasAttribute( "width" ) )
		{
			dimensions.x = GetAttribute< float >( "width", -1 );
		}
		else
		{
			dimensions.x = ( float ) GetParentNode()->GetBox( Rocket::Core::Box::CONTENT ).GetSize().x;
		}

		// Calculate the y dimension.
		if ( HasAttribute( "height" ) )
		{
			dimensions.y = GetAttribute< float >( "height", -1 );
		}
		else
		{
			dimensions.y = ( float ) GetParentNode()->GetBox( Rocket::Core::Box::CONTENT ).GetSize().y;
		}

		// Return the calculated dimensions. If this changes the size of the element, it will result in
		// a 'resize' event which is caught below and will regenerate the geometry.
		return true;
	}
};

#endif
// kate: indent-mode cstyle; indent-width 4; replace-tabs off; tab-width 4;
