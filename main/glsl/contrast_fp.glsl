/*
===========================================================================
Copyright (C) 2006-2009 Robert Beckebans <trebor_7@users.sourceforge.net>

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

/* contrast_fp.glsl */

uniform sampler2D	u_ColorMap;

const vec4			LUMINANCE_VECTOR = vec4(0.2125, 0.7154, 0.0721, 0.0);

void	main()
{
	vec2 st = gl_FragCoord.st;

	// calculate the screen texcoord in the 0.0 to 1.0 range
	st *= r_FBufScale;

	// multiply with 4 because the FBO is only 1/4th of the screen resolution
	st *= vec2(4.0, 4.0);
	
	// scale by the screen non-power-of-two-adjust
	st *= r_NPOTScale;
	
	// calculate contrast color
#if 0
	// perform a box filter for the downsample
	vec3 color = texture2D(u_ColorMap, st + vec2(-1.0, 1.0) * r_FBufScale).rgb;
	color += texture2D(u_ColorMap, st + vec2(1.0, 1.0) * r_FBufScale).rgb;
	color += texture2D(u_ColorMap, st + vec2(1.0, -1.0) * r_FBufScale).rgb;
	color += texture2D(u_ColorMap, st + vec2(-1.0, -1.0) * r_FBufScale).rgb;
	color *= 0.25;
#else
	vec4 color = texture2D(u_ColorMap, st);
#endif

	float L = dot(LUMINANCE_VECTOR, color);
	
	// adjust contrast
	L = pow(L, 1.32);

	float T = clamp(L - 0.71, 0.0, 1.0);
	
	color.rgb *= T;
	
	gl_FragColor = color;
}
