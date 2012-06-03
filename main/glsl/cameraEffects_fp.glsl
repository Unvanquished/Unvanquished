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

uniform sampler2D	u_CurrentMap;
uniform sampler2D	u_GrainMap;
uniform sampler2D	u_VignetteMap;

varying vec2		var_Tex;

void	main()
{
	// calculate the screen texcoord in the 0.0 to 1.0 range
	vec2 stClamped = gl_FragCoord.st * r_FBufScale;
	
	// scale by the screen non-power-of-two-adjust
	vec2 st = stClamped * r_NPOTScale;
	
	vec4 original = clamp(texture2D(u_CurrentMap, st), 0.0, 1.0);
	
	vec4 color = original;

	// calculate chromatic aberration
#if 0
	vec2 redOffset = vec2(0.5, 0.25);
	vec2 greenOffset = vec2(0.0, 0.0);
	vec2 blueOffset = vec2(-0.5, -0.25);
	
	color.r = texture2D(u_CurrentMap, st + redOffset * r_FBufScale).r;
	color.g = texture2D(u_CurrentMap, st + greenOffset * r_FBufScale).g;
	color.b = texture2D(u_CurrentMap, st + blueOffset * r_FBufScale).b;
#endif

	// blend the vignette
	vec4 vignette = texture2D(u_VignetteMap, stClamped);
	color.rgb *= vignette.rgb; 
	
	// add grain
	vec4 grain = texture2D(u_GrainMap, var_Tex);
	color.rgb = (color.rgb + (grain.rgb * vec3(0.035, 0.065, 0.09))) + (color.rgb * (grain.rgb * vec3(0.035, 0.065, 0.09)));
	//color.rgb = grain.rgb;

	gl_FragColor = color;
}
