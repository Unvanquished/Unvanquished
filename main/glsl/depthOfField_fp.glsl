/*
===========================================================================
Copyright (C) 2008 Robert Beckebans <trebor_7@users.sourceforge.net>

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

/* depthOfField_fp.glsl */

uniform sampler2D	u_CurrentMap;
uniform sampler2D	u_DepthMap;

void	main()
{
	// calculate the screen texcoord in the 0.0 to 1.0 range
	vec2 st = gl_FragCoord.st * r_FBufScale;
	
	// scale by the screen non-power-of-two-adjust
	st *= r_NPOTScale;

	float focus = 0.98;			// focal distance, normalized 0.8-0.999
	float radius = 0.5;	  	// 0 - 20.0
	
	vec4 sum = vec4(0.0, 0.0, 0.0, 0.0);

	// autofocus
	focus = texture2D(u_DepthMap, vec2(0.5, 0.5) * r_NPOTScale).r;
	
	const float tap = 5.0;
	const float taps = tap * 2.0 + 1.0;
	
	float depth = texture2D(u_DepthMap, st).r;
	float delta = (abs(depth - focus) * abs(depth - focus)) / float(tap);
	delta *= radius;
	//delta = clamp(radius * delta, -max, max);
	
	for(float i = -tap; i < tap; i++)
    {
	    for(float j = -tap; j < tap; j++)
	    {
			sum += texture2D(u_CurrentMap, st + vec2(i, j) * delta);
		}
	}
	
	sum *=  1.0 / (taps * taps);
	
	gl_FragColor = sum;
}
