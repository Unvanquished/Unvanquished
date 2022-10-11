/*
===========================================================================

Unvanquished GPL Source Code
Copyright (C) 2012 Unvanquished Developers

This file is part of the Unvanquished GPL Source Code (Unvanquished Source Code).

Unvanquished Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Unvanquished Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Unvanquished Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Unvanquished Source Code is also subject to certain additional terms.
You should have received a copy of these additional terms immediately following the
terms and conditions of the GNU General Public License which accompanied the Unvanquished
Source Code.  If not, please request a copy in writing from id Software at the address
below.

If you have questions concerning this license or the applicable additional terms, you
may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville,
Maryland 20850 USA.

===========================================================================
*/

#ifndef ROCKETINCLUDEELEMENT_H
#define ROCKETINCLUDEELEMENT_H

#include "../cg_local.h"
#include "rocket.h"
#include <common/FileSystem.h>

class RocketIncludeElement : public Rml::Element
{
public:
	RocketIncludeElement( const Rml::String &tag ) : Rml::Element( tag ) { }
	void OnAttributeChange( const Rml::ElementAttributes &changed_attributes )
	{
		Element::OnAttributeChange( changed_attributes );
		if ( changed_attributes.find( "src" ) != changed_attributes.end() )
		{
			Rml::String filename = GetAttribute<Rml::String>("src", "");

			if ( !filename.empty() )
			{
				std::string buffer;
				std::error_code err;
				buffer = FS::PakPath::ReadFile( filename, err );
				if ( err )
				{
					Log::Warn( "Error reading %s: %s", filename, err.message() );
					return;
				}

				SetInnerRML( buffer );
			}
		}
	}
};


#endif
