/*
===========================================================================
Copyright (C) 2009 Robert Beckebans <trebor_7@users.sourceforge.net>

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

/* depthToColor_fp.glsl */


void	main()
{
	// compute depth instead of world vertex position in a [0..1] range
	float depth = gl_FragCoord.z;

//#if 0 //defined(GLHW_ATI_DX10) || defined(GLHW_NV_DX10)
//	gl_FragColor = vec4(0.0, 0.0, 0.0, depth);
//#else
	// 32 bit precision
	const vec4 bitSh = vec4(256 * 256 * 256,	256 * 256,				256,         1);
	const vec4 bitMsk = vec4(			0,		1.0 / 256.0,    1.0 / 256.0,    1.0 / 256.0);
	
	vec4 comp;
	comp = depth * bitSh;
	comp = fract(comp);
	comp -= comp.xxyz * bitMsk;
	gl_FragColor = comp;
/*
	// 24 bit precision
	const vec3 bitSh = vec3(256 * 256,			256,		1);
	const vec3 bitMsk = vec3(		0,	1.0 / 256.0,		1.0 / 256.0);
	
	vec3 comp;
	comp = depth * bitSh;
	comp = fract(comp);
	comp -= comp.xxy * bitMsk;
	gl_FragColor = vec4(comp, 0.0);
*/
//#endif
}

	