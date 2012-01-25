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
#if defined(r_heatHazeFix)
uniform sampler2D	u_ContrastMap;
#endif
uniform int			u_AlphaTest;

varying vec2		var_TexNormal;
varying float		var_Deform;

void	main()
{
	vec4 color0, color1;

	// compute normal in tangent space from normalmap
	color0 = texture2D(u_NormalMap, var_TexNormal).rgba;
	vec3 N = 2.0 * (color0.rgb - 0.5);

	// calculate the screen texcoord in the 0.0 to 1.0 range
	vec2 st = gl_FragCoord.st * r_FBufScale;

#if defined(USE_ALPHA_TESTING)
	if(u_AlphaTest == ATEST_GT_0 && color0.a <= 0.0)
	{
		discard;
		return;
	}
	else if(u_AlphaTest == ATEST_LT_128 && color0.a >= 0.5)
	{
		discard;
		return;
	}
	else if(u_AlphaTest == ATEST_GE_128 && color0.a < 0.5)
	{
		discard;
		return;
	}
#endif
	
	// offset by the scaled normal and clamp it to 0.0 - 1.0
	st += N.xy * var_Deform;
	st = clamp(st, 0.0, 1.0);
	
	// scale by the screen non-power-of-two-adjust
	st *= r_NPOTScale;
	
#if defined(r_heatHazeFix)
	// check if the distortion got too far
	float vis = texture2D(u_ContrastMap, st).r;
	if(vis > 0.0)
	{
		color0 = texture2D(u_CurrentMap, st);
		color1 = vec4(0.0, 1.0, 0.0, color0.a);
	}
	else
	{
		// reset st and don't offset
		st = gl_FragCoord.st * r_FBufScale * r_NPOTScale;
		
		color0 = texture2D(u_CurrentMap, st);
		color1 = vec4(1.0, 0.0, 0.0, color0.a);
	}
	
#if 0
	gl_FragColor = texture2D(u_ContrastMap, gl_FragCoord.st * r_FBufScale * r_NPOTScale);
	return;
#endif
	
#else
	color0 = texture2D(u_CurrentMap, st);
#endif
	
	gl_FragColor = color0;
}
