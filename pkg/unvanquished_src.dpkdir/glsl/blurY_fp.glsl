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
uniform vec2		u_TexScale;

void	main()
{
	vec2 st = gl_FragCoord.st * u_TexScale;
	
	#if 0
	float gaussFact[3] = float[3](1.0, 2.0, 1.0);
	float gaussSum = 4;
	const int tap = 1;
	#elif 0
	float gaussFact[5] = float[5](1.0, 4.0, 6.0, 4.0, 1.0);
	float gaussSum = 16.0;
	const int tap = 2;
	#elif 1
	float gaussFact[7] = float[7](1.0, 6.0, 15.0, 20.0, 15.0, 6.0, 1.0);
	float gaussSum = 64.0;
	const int tap = 3;
	#endif

	// do a full gaussian blur
	vec4 sumColors = vec4(0.0);
	
	for(int t = -tap; t <= tap; t++)
	{
		float weight = gaussFact[t + tap];
		sumColors += texture2D(u_ColorMap, st + vec2(0, t) * u_TexScale * u_DeformMagnitude) * weight;
	}
	
	gl_FragColor = sumColors * (1.0 / gaussSum);
}
