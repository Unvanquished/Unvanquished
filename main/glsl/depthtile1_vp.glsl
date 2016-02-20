/*
===========================================================================
Copyright (C) 2006-2008 Robert Beckebans <trebor_7@users.sourceforge.net>

This file is part of XreaL source code.

XreaL source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

XreaL source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with XreaL source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

/* depthtile1_vp.glsl */

attribute vec3 attr_Position;

uniform vec3 u_zFar;
varying vec3 unprojectionParams;

void	main()
{
	unprojectionParams.x = - r_zNear * u_zFar.z;
	unprojectionParams.y = 2.0 * (u_zFar.z - r_zNear);
	unprojectionParams.z = 2.0 * u_zFar.z - r_zNear;

	// no vertex transformation needed
	gl_Position = vec4(attr_Position, 1.0);
}
