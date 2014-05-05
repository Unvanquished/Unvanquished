/*
===========================================================================
Copyright (C) 2009-2010 Robert Beckebans <trebor_7@users.sourceforge.net>

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

/* cameraEffects_fp.glsl */

uniform sampler2D u_CurrentMap;
uniform sampler3D u_ColorMap;
uniform vec4      u_ColorModulate;
uniform float     u_InverseGamma;

varying vec2		var_Tex;

void	main()
{
	// calculate the screen texcoord in the 0.0 to 1.0 range
	vec2 stClamped = gl_FragCoord.st * r_FBufScale;

	// scale by the screen non-power-of-two-adjust
	vec2 st = stClamped * r_NPOTScale;

	vec4 original = clamp(texture2D(u_CurrentMap, st), 0.0, 1.0);

	vec4 color = original;

	// apply color grading
	vec3 colCoord = color.rgb * 15.0 / 16.0 + 0.5 / 16.0;
	colCoord.z *= 0.25;
	color.rgb = u_ColorModulate.x * texture3D(u_ColorMap, colCoord).rgb;
	colCoord.z += 0.25;
	color.rgb += u_ColorModulate.y * texture3D(u_ColorMap, colCoord).rgb;
	colCoord.z += 0.25;
	color.rgb += u_ColorModulate.z * texture3D(u_ColorMap, colCoord).rgb;
	colCoord.z += 0.25;
	color.rgb += u_ColorModulate.w * texture3D(u_ColorMap, colCoord).rgb;

	color.xyz = pow(color.xyz, vec3(u_InverseGamma));

	gl_FragColor = color;
}
