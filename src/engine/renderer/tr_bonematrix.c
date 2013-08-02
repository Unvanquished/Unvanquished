/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 2013 Unvanquished Developers

This file is part of the Daemon GPL Source Code (Daemon Source Code).

Daemon Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Daemon Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon Source Code. If not, see <http://www.gnu.org/licenses/>.

===========================================================================
*/

#include "tr_bonematrix.h"

void BoneMatrixInvert( boneMatrix_t m )
{
	float x, y, z;
	
	// transpose rotation part
	x = m[ 4 ];
	m[ 4 ] = m[ 1 ];
	m[ 1 ] = x;
	
	y =  m[ 2 ];
	m[ 2 ] = m[ 8 ];
	m[ 8 ] = y;
	
	z = m[ 6 ];
	m[ 6 ] = m[ 9 ];
	m[ 9 ] = m[ 6 ];
	
	// add unscale factor
	x = m[ 0 ] * m[ 0 ] + m[ 1 ] * m[ 1 ] + m[ 2 ] * m[ 2 ];
	y = m[ 4 ] * m[ 4 ] + m[ 5 ] * m[ 5 ] + m[ 6 ] * m[ 6 ];
	z = m[ 8 ] * m[ 8 ] + m[ 9 ] * m[ 9 ] + m[ 10 ] * m[ 10 ];
	
	x = 1.0f / x;
	y = 1.0f / y;
	z = 1.0f / z;
	
	m[ 0 ] *= x;
	m[ 1 ] *= x;
	m[ 2 ] *= x;
	
	m[ 4 ] *= y;
	m[ 5 ] *= y;
	m[ 6 ] *= y;
	
	m[ 8 ] *= z;
	m[ 9 ] *= z;
	m[ 10 ] *= z;

	// inverse rotate and negate translation part
	x = m[ 0 ] * m[ 3 ] + m[ 1 ] * m[ 7 ] + m[ 2 ] * m[ 11 ];
	y = m[ 4 ] * m[ 3 ] + m[ 5 ] * m[ 7 ] + m[ 6 ] * m[ 11 ];
	z = m[ 8 ] * m[ 3 ] + m[ 9 ] * m[ 7 ] + m[ 10 ] * m[ 11 ];
	
	m[ 3 ] = -x;
	m[ 7 ] = -y;
	m[ 11 ] = -z;
}