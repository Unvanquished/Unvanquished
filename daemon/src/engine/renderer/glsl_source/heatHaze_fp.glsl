/*
===========================================================================
Copyright (C) 2006-2011 Robert Beckebans <trebor_7@users.sourceforge.net>

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

/* heatHaze_fp.glsl */

uniform sampler2D	u_NormalMap;
uniform sampler2D	u_CurrentMap;
uniform float		u_AlphaThreshold;

IN(smooth) vec2		var_TexNormal;
IN(smooth) float	var_Deform;

DECLARE_OUTPUT(vec4)

void	main()
{
	vec4 color0, color1;

	// compute normal in tangent space from normalmap
	color0 = texture2D(u_NormalMap, var_TexNormal).rgba;
	vec3 N = 2.0 * (color0.rgb - 0.5);

	// calculate the screen texcoord in the 0.0 to 1.0 range
	vec2 st = gl_FragCoord.st * r_FBufScale;

	// offset by the scaled normal and clamp it to 0.0 - 1.0
	st += N.xy * var_Deform;
	st = clamp(st, 0.0, 1.0);

	color0 = texture2D(u_CurrentMap, st);

	outputColor = color0;
}
