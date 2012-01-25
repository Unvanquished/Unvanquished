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

/* screenSpaceAmbientOcclusion_fp.glsl */

uniform sampler2D	u_CurrentMap;
uniform sampler2D	u_DepthMap;
//uniform vec3		u_ViewOrigin;
//uniform vec3		u_SSAOJitter[32];
//uniform float		u_SSAORadius;
//uniform mat4		u_UnprojectMatrix;
//uniform mat4		u_ProjectMatrix;

float  ReadDepth(vec2 st)
{
	vec2 camerarange = vec2(4.0, 4096.0);
	
	return (2.0 * camerarange.x) / (camerarange.y + camerarange.x - texture2D(u_DepthMap, st).x * (camerarange.y - camerarange.x));	
}


void	main()
{
	// calculate the screen texcoord in the 0.0 to 1.0 range
	vec2 st = gl_FragCoord.st * r_FBufScale;
	
	// scale by the screen non-power-of-two-adjust
	st *= r_NPOTScale;

	float depth =  ReadDepth(st);
	float d;

	float aoCap = 1.0;
	float ao = 0.0;
	
	float aoMultiplier = 1000.0;
	float depthTolerance = 0.0001;
	
	float tap = 3.0;
	float taps = tap * 2.0 + 1.0;
	for(float i = -tap; i < tap; i++)
    {
	    for(float j = -tap; j < tap; j++)
	    {
			d = ReadDepth(st + vec2(j, i) * r_FBufScale);
			ao += min(aoCap, max(0.0, depth -d -depthTolerance) * aoMultiplier);
		}
	}
	
	ao /= (taps * taps);
	
	gl_FragColor = vec4(1.0 - ao) * texture2D(u_CurrentMap, st);
}
