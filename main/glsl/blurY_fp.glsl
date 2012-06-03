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

/* blurY_fp.glsl */

uniform sampler2D	u_ColorMap;
uniform float		u_DeformMagnitude;

void	main()
{
	vec2 st = gl_FragCoord.st;

	// calculate the screen texcoord in the 0.0 to 1.0 range
	st *= r_FBufScale;

	// multiply with 4 because the FBO is only 1/4th of the screen resolution
	st *= vec2(4.0, 4.0);
	
	// scale by the screen non-power-of-two-adjust
	st *= r_NPOTScale;
	
#if 1
	// set so a magnitude of 1 is approximately 1 pixel with 640x480
	//vec2 deform = vec2(u_DeformMagnitude * 0.0016, u_DeformMagnitude * 0.00213333);
	vec2 deform = u_DeformMagnitude * r_FBufScale;
	
	// these are the multipliers for the gaussian blur
	float mu01 = 0.00261097;
	float mu02 = 0.00522193;
	float mu03 = 0.01044386;
	float mu04 = 0.02088773;
	float mu05 = 0.04177546;
	float mu06 = 0.08355091;
	float mu07 = 0.16710183;
	float mu08 = 0.33420366;
	float mu09 = 0.66840732;
	
	// fragment offsets for blur samples
	vec2 offset01 = vec2( 0.0, -8.0);
	vec2 offset02 = vec2( 0.0, -7.0);
	vec2 offset03 = vec2( 0.0, -6.0);
	vec2 offset04 = vec2( 0.0, -5.0);
	vec2 offset05 = vec2( 0.0, -4.0);
	vec2 offset06 = vec2( 0.0, -3.0);
	vec2 offset07 = vec2( 0.0, -2.0);
	vec2 offset08 = vec2( 0.0, -1.0);
	vec2 offset09 = vec2( 0.0,  1.0);
	vec2 offset10 = vec2( 0.0,  2.0);
	vec2 offset11 = vec2( 0.0,  3.0);
	vec2 offset12 = vec2( 0.0,  4.0);
	vec2 offset13 = vec2( 0.0,  5.0);
	vec2 offset14 = vec2( 0.0,  6.0);
	vec2 offset15 = vec2( 0.0,  7.0);
	vec2 offset16 = vec2( 0.0,  8.0);
	
	// calculate our offset texture coordinates
	vec2 st01 = st + offset01 * deform;
	vec2 st02 = st + offset02 * deform;
	vec2 st03 = st + offset03 * deform;
	vec2 st04 = st + offset04 * deform;
	vec2 st05 = st + offset05 * deform;
	vec2 st06 = st + offset06 * deform;
	vec2 st07 = st + offset07 * deform;
	vec2 st08 = st + offset08 * deform;
	vec2 st09 = st + offset09 * deform;
	vec2 st10 = st + offset10 * deform;
	vec2 st11 = st + offset11 * deform;
	vec2 st12 = st + offset12 * deform;
	vec2 st13 = st + offset13 * deform;
	vec2 st14 = st + offset14 * deform;
	vec2 st15 = st + offset15 * deform;
	vec2 st16 = st + offset16 * deform;
	
	// base color
	vec4 c00 = texture2D(u_ColorMap, st);

	// sample the current render for each coordinate
	vec4 c01 = texture2D(u_ColorMap, st01);
	vec4 c02 = texture2D(u_ColorMap, st02);
	vec4 c03 = texture2D(u_ColorMap, st03);
	vec4 c04 = texture2D(u_ColorMap, st04);
	vec4 c05 = texture2D(u_ColorMap, st05);
	vec4 c06 = texture2D(u_ColorMap, st06);
	vec4 c07 = texture2D(u_ColorMap, st07);
	vec4 c08 = texture2D(u_ColorMap, st08);
	vec4 c09 = texture2D(u_ColorMap, st09);
	vec4 c10 = texture2D(u_ColorMap, st10);
	vec4 c11 = texture2D(u_ColorMap, st11);
	vec4 c12 = texture2D(u_ColorMap, st12);
	vec4 c13 = texture2D(u_ColorMap, st13);
	vec4 c14 = texture2D(u_ColorMap, st14);
	vec4 c15 = texture2D(u_ColorMap, st15);
	vec4 c16 = texture2D(u_ColorMap, st16);
	
	vec4 color = c01 * mu01;
	color += c02 * mu02;
	color += c03 * mu03;
	color += c04 * mu04;
	color += c05 * mu05;
	color += c06 * mu06;
	color += c07 * mu07;
	color += c08 * mu08;
	color += c00 * mu09;
	color += c09 * mu08;
	color += c10 * mu07;
	color += c11 * mu06;
	color += c12 * mu05;
	color += c13 * mu04;
	color += c14 * mu03;
	color += c15 * mu02;
	color += c16 * mu01;
	
	gl_FragColor = color;
#else

	#if 0
	float gaussFact[3] = float[3](1.0, 2.0, 1.0);
	#elif 0
	float gaussFact[5] = float[5](1.0, 4.0, 6.0, 4.0, 1.0);
	float gaussSum = 16.0;
	#elif 1
	float gaussFact[7] = float[7](1.0, 6.0, 15.0, 20.0, 15.0, 6.0, 1.0);
	float gaussSum = 64.0;
	#endif

	// do a full gaussian blur
	vec4 sumColors = vec4(0.0);

	int tap = 3;
	for(int i = -tap; i < tap; i++)
    {
	    float weight = gaussFact[i + 2];
		vec4 color = texture2D(u_ColorMap, st + vec2(0, i) * u_DeformMagnitude * r_FBufScale) * weight;
			
		sumColors += color;
	}
	
	gl_FragColor = sumColors / gaussSum;
#endif
}
