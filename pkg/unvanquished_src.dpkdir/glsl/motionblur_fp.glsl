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

/* motionblur_fp.glsl */

uniform sampler2D	u_ColorMap;
uniform sampler2D	u_DepthMap;

uniform vec3            u_blurVec;

void	main()
{
	vec2 st = gl_FragCoord.st;
	vec4 color = vec4( 0.0 );

	// calculate the screen texcoord in the 0.0 to 1.0 range
	st *= r_FBufScale;

	// scale by the screen non-power-of-two-adjust
	st *= r_NPOTScale;

	float depth = texture2D( u_DepthMap, st ).r;

	if( depth >= 1.0 ) {
		discard;
		return;
        }
	depth /= 1.0 - depth;

	vec3 start = vec3(st * 2.0 - 1.0, 1.0) * depth;
	vec3 end   = start + u_blurVec.yzx;

	float weight = 1.0;
	float total = 0.0;

	for( int i = 0; i < 6; i ++ ) {
		vec3 pos = mix( start, end, float(i) * 0.1 );
		pos /= pos.z;

		color += weight * texture2D( u_ColorMap, 0.5 * pos.xy + 0.5 );
		total += weight;
		weight *= 0.5;
        }

	gl_FragColor = color / total;
}
