/*
===========================================================================
Copyright (C) 2006 Robert Beckebans <trebor_7@users.sourceforge.net>

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

/* rotoscope_fp.glsl */

uniform sampler2D	u_ColorMap;
uniform float		u_BlurMagnitude;

void	main()
{
	vec2 st00 = gl_FragCoord.st;

	// calculate the screen texcoord in the 0.0 to 1.0 range
	st00 *= r_FBufScale;
	
	// scale by the screen non-power-of-two-adjust
	st00 *= r_NPOTScale;
	
	// set so a magnitude of 1 is approximately 1 pixel with 640x480
	//vec2 deform = vec2(u_BlurMagnitude * 0.0016, u_BlurMagnitude * 0.00213333);
	vec2 deform = u_BlurMagnitude * r_FBufScale;
	
	// fragment offsets
	vec2 offset01 = vec2(-0.5, -0.5);
	vec2 offset02 = vec2(-0.5,  0.0);
	vec2 offset03 = vec2(-0.5,  0.5);
	vec2 offset04 = vec2( 0.5, -0.5);
	vec2 offset05 = vec2( 0.5,  0.0);
	vec2 offset06 = vec2( 0.5,  0.5);
	vec2 offset07 = vec2( 0.0, -0.5);
	vec2 offset08 = vec2( 0.0,  0.5);
	
	// calculate our offset texture coordinates
	vec2 st01 = st00 + offset01 * deform;
	vec2 st02 = st00 + offset02 * deform;
	vec2 st03 = st00 + offset03 * deform;
	vec2 st04 = st00 + offset04 * deform;
	vec2 st05 = st00 + offset05 * deform;
	vec2 st06 = st00 + offset06 * deform;
	vec2 st07 = st00 + offset07 * deform;
	vec2 st08 = st00 + offset08 * deform;
	
	// base color
	vec4 c00 = texture2D(u_ColorMap, st00);

	// sample the current render for each coordinate
	vec4 c01 = texture2D(u_ColorMap, st01);
	vec4 c02 = texture2D(u_ColorMap, st02);
	vec4 c03 = texture2D(u_ColorMap, st03);
	vec4 c04 = texture2D(u_ColorMap, st04);
	vec4 c05 = texture2D(u_ColorMap, st05);
	vec4 c06 = texture2D(u_ColorMap, st06);
	vec4 c07 = texture2D(u_ColorMap, st07);
	vec4 c08 = texture2D(u_ColorMap, st08);
	
	// each of them have a scale value of two in
	vec4 xedge = c02 * 2.0 - c05 * 2.0;
	vec4 yedge = c07 * 2.0 - c08 * 2.0;
	
	xedge = xedge + c01 + c03 - c04 - c06;
	yedge = yedge + c01 - c03 + c04 - c06;

	// square each and add and take the sqrt
	xedge *= xedge;
	yedge *= yedge;
	
	vec4 tmp = xedge + yedge;
	tmp.x = max(tmp.x, tmp.y);
	tmp.x = max(tmp.x, tmp.z);
	
	// write result from edge detection into sobel variable
	vec4 sobel = vec4(pow(tmp.x, 0.5), 0, 0, 0);
	
	//
	// normalize the color hue
	//
	vec4 hue;
	
	// calculate [sum RGB]
	hue.x = c00.x + c00.y + c00.z;

	// calculate 1 / [sum RGB]
	hue.y = 1.0 / hue.x;
	
	// multiply pixel color with 1 / [sum RGB]
	c00 *= hue.y;
	
	//
	// calculate the pixel intensity
	// make everything with [sum RGB] < .2 black
	//
	
	// define darktones ([sum RGB] > .2) and store in channel z
	if(hue.x >= 0.2)
		hue.z = 1.0;
	else
		hue.z = 0.0;
		
	// darktones will be normalized HUE * 0.5
	hue.z *= 0.5;
	
	// define midtones ([sum RGB] >.8) and store in channel y
	if(hue.x >= 0.8)
		hue.y = 1.0;
	else
		hue.y = 0.0;
		
	// midtones will be normalized HUE * (0.5 + 0.5)
	hue.y *= (0.5 + 0.5);
	
	// define brighttones ([sum RGB] > 1.5) and store in channel x
	if(hue.x >= 1.5)
		hue.x = 1.0;
	else
		hue.x = 0.0;
		
	// brighttones will be normalized HUE * (0.5 + 0.5 + 1.5)
	hue.x *= (0.5 + 0.5 + 1.5);

	// sum darktones + midtones + brighttones to calculate final pixel intensity
	hue.x += hue.y + hue.z;

	// multiply pixel color with rotoscope intensity
	c00 *= hue.x;

	// generate edge mask -> make 1 for all regions not belonging to an edge
	// use 0.8 as threshold for edge detection
	if(sobel.x < 0.8)
		sobel.x = 1.0;
	else
		sobel.x = 0.0;

	// blend white into edge artifacts (occuring in bright areas)
	// find pixels with intensity > 2.5
	if(hue.x >= 2.5)
		sobel.y = 1.0;
	else
		sobel.y = 0.0;

	// set the edge mask to 1 in such areas
	sobel.x = max(sobel.x, sobel.y);
	
	// multiply cleaned up edge mask with final color image
	c00 *= sobel.x;
	
	// transfer image to output
	gl_FragColor = c00;
}
