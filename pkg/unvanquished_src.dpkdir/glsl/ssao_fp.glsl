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

/* ssao_fp.glsl */

#extension GL_ARB_texture_gather : require

uniform sampler2D u_DepthMap;

const vec2 pixelScale = r_FBufScale * r_NPOTScale;

float depthToZ(float depth) {
	return r_zNear / (1.0 - depth);
}
vec4 depthToZ(vec4 depth) {
	return r_zNear / (1.0 - depth);
}

vec4 rangeTest(vec4 diff) {
	return step( vec4( 0.00312 ), diff ) * step( diff, vec4( 0.43 ) );
}

// Based on AMD HDAO, adapted for lack of normals
void computeOcclusionForQuad( in vec2 centerTC, in float centerDepth,
			      in vec2 quadOffset, in vec4 weights,
			      inout vec4 occlusion, inout vec4 total ) {
	vec2 tc1 = centerTC + quadOffset * pixelScale;
	vec2 tc2 = centerTC - quadOffset * pixelScale;

	// depths1 and depth2 pixels are swizzled to be in opposite position
	vec4 depths1 = depthToZ( textureGather( u_DepthMap, tc1 ) ).xyzw;
	vec4 depths2 = depthToZ( textureGather( u_DepthMap, tc2 ) ).zwxy;

	total += weights;
	occlusion += weights * rangeTest( centerDepth - depths1 ) * rangeTest( centerDepth - depths2 );
}

void	main()
{
	vec2 st = gl_FragCoord.st * pixelScale;
	vec4 color = vec4( 0.0 );
	vec4 occlusion = vec4( 0.0 );
	vec4 total = vec4( 0.0 );

	float center = texture2D( u_DepthMap, st ).r;
	if( center >= 1.0 ) {
		// no SSAO on the skybox !
		discard;
		return;
        }
	center = depthToZ( center );

	computeOcclusionForQuad( st, center, vec2( 1.5, -1.5 ),
				 vec4( 1.00000, 0.50000, 0.44721, 0.70711 ),
				 occlusion, total );
	computeOcclusionForQuad( st, center, vec2( 0.5,  1.5 ),
				 vec4( 0.50000, 0.44721, 0.70711, 1.00000 ),
				 occlusion, total );
	computeOcclusionForQuad( st, center, vec2( 0.5,  3.5 ),
				 vec4( 0.30000, 0.29104, 0.37947, 0.40000 ),
				 occlusion, total );
	computeOcclusionForQuad( st, center, vec2( 2.5,  1.5 ),
				 vec4( 0.42426, 0.33282, 0.37947, 0.53666 ),
				 occlusion, total );
	computeOcclusionForQuad( st, center, vec2( 3.5, -1.5 ),
				 vec4( 0.40000, 0.30000, 0.29104, 0.37947 ),
				 occlusion, total );
	computeOcclusionForQuad( st, center, vec2( 1.5, -2.5 ),
				 vec4( 0.53666, 0.42426, 0.33282, 0.37947 ),
				 occlusion, total );
#if 1
	computeOcclusionForQuad( st, center, vec2( 1.5, -4.5 ),
				 vec4( 0.31530, 0.29069, 0.24140, 0.25495 ),
				 occlusion, total );
	computeOcclusionForQuad( st, center, vec2( 3.5, -2.5 ),
				 vec4( 0.36056, 0.29069, 0.26000, 0.30641 ),
				 occlusion, total );
	computeOcclusionForQuad( st, center, vec2( 5.5, -0.5 ),
				 vec4( 0.26000, 0.21667, 0.21372, 0.25495 ),
				 occlusion, total );
	computeOcclusionForQuad( st, center, vec2( 4.5,  1.5 ),
				 vec4( 0.29069, 0.24140, 0.25495, 0.31530 ),
				 occlusion, total );
	computeOcclusionForQuad( st, center, vec2( 2.5,  3.5 ),
				 vec4( 0.29069, 0.26000, 0.30641, 0.36056 ),
				 occlusion, total );
	computeOcclusionForQuad( st, center, vec2( 0.5,  5.5 ),
				 vec4( 0.21667, 0.21372, 0.25495, 0.26000 ),
				 occlusion, total );
#endif

	float summedOcclusion = dot( occlusion, vec4( 1.0 ) );
	float summedTotal = dot( total, vec4( 1.0 ) );

	if ( summedTotal > 0.0 ) {
		summedOcclusion /= summedTotal;
	}
	gl_FragColor = vec4( clamp(1.0 - summedOcclusion, 0.0, 1.0) );
}
