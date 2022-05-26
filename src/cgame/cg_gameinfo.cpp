/*
===========================================================================

Unvanquished GPL Source Code
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2000-2009 Darklegion Development

This file is part of the Unvanquished GPL Source Code (Unvanquished Source Code).

Unvanquished is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Unvanquished is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Unvanquished; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

===========================================================================
*/

//
// gameinfo.c
//

#include "common/FileSystem.h"
#include "cg_local.h"

/*
===============
CG_LoadArenas
===============
*/
void CG_LoadArenas()
{
	rocketInfo.data.mapList.clear();
	for (const std::string& mapName : FS::GetAvailableMaps(false))
	{
		rocketInfo.data.mapList.push_back( {mapName} );
	}

	std::sort(
		rocketInfo.data.mapList.begin(), rocketInfo.data.mapList.end(),
		[]( const mapInfo_t& a, const mapInfo_t& b ) { return Q_stricmp( a.mapLoadName.c_str(), b.mapLoadName.c_str() ) < 0; }
	);
}
