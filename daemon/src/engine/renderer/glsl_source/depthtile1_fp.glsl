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

/* depthtile1_fp.glsl */

uniform sampler2D u_DepthMap;
IN(flat) vec3 unprojectionParams;

uniform vec3 u_zFar;

const vec2 pixelScale = r_FBufScale;

DECLARE_OUTPUT(vec4)

vec4 depthToZ(in vec4 depth) {
  return unprojectionParams.x / ( unprojectionParams.y * depth - unprojectionParams.z );
}

float max16(in vec4 data0, in vec4 data1, in vec4 data2, in vec4 data3) {
  vec4 max01 = max(data0, data1);
  vec4 max23 = max(data2, data3);
  vec4 max4 = max(max01, max23);
  vec2 max2 = max(max4.xy, max4.zw);
  return max(max2.x, max2.y);
}

float min16(in vec4 data0, in vec4 data1, in vec4 data2, in vec4 data3)
{
  vec4 min01 = min(data0, data1);
  vec4 min23 = min(data2, data3);
  vec4 min4 = min(min01, min23);
  vec2 min2 = min(min4.xy, min4.zw);
  return min(min2.x, min2.y);
}
void	main()
{
  vec2 st = gl_FragCoord.st * 4.0 * pixelScale;
  vec4 depth[4], mask[4];

#ifdef HAVE_ARB_texture_gather
  depth[0] = textureGather( u_DepthMap, st + vec2(-1.0, -1.0) * pixelScale );
  depth[1] = textureGather( u_DepthMap, st + vec2(-1.0,  1.0) * pixelScale );
  depth[2] = textureGather( u_DepthMap, st + vec2( 1.0,  1.0) * pixelScale );
  depth[3] = textureGather( u_DepthMap, st + vec2( 1.0, -1.0) * pixelScale );
#else
  depth[0] = vec4(texture2D( u_DepthMap, st + vec2(-1.5, -1.5) * pixelScale ).x,
		  texture2D( u_DepthMap, st + vec2(-1.5, -0.5) * pixelScale ).x,
		  texture2D( u_DepthMap, st + vec2(-0.5, -0.5) * pixelScale ).x,
		  texture2D( u_DepthMap, st + vec2(-0.5, -1.5) * pixelScale ).x);
  depth[1] = vec4(texture2D( u_DepthMap, st + vec2( 0.5, -1.5) * pixelScale ).x,
		  texture2D( u_DepthMap, st + vec2( 0.5, -0.5) * pixelScale ).x,
		  texture2D( u_DepthMap, st + vec2( 1.5, -0.5) * pixelScale ).x,
		  texture2D( u_DepthMap, st + vec2( 1.5, -1.5) * pixelScale ).x);
  depth[2] = vec4(texture2D( u_DepthMap, st + vec2( 0.5,  0.5) * pixelScale ).x,
		  texture2D( u_DepthMap, st + vec2( 0.5,  1.5) * pixelScale ).x,
		  texture2D( u_DepthMap, st + vec2( 1.5,  1.5) * pixelScale ).x,
		  texture2D( u_DepthMap, st + vec2( 1.5,  0.5) * pixelScale ).x);
  depth[3] = vec4(texture2D( u_DepthMap, st + vec2(-1.5,  0.5) * pixelScale ).x,
		  texture2D( u_DepthMap, st + vec2(-1.5,  1.5) * pixelScale ).x,
		  texture2D( u_DepthMap, st + vec2(-0.5,  1.5) * pixelScale ).x,
		  texture2D( u_DepthMap, st + vec2(-0.5,  0.5) * pixelScale ).x);
#endif

 // mask out sky pixels (z buffer depth == 1.0) to help with depth discontinuities
  mask[0] = 1.0 - step(1.0, depth[0]);
  mask[1] = 1.0 - step(1.0, depth[1]);
  mask[2] = 1.0 - step(1.0, depth[2]);
  mask[3] = 1.0 - step(1.0, depth[3]);

  float samples = dot( mask[0] + mask[1] + mask[2] + mask[3], vec4(1.0) );
  if ( samples > 0.0 ) {
    samples = 1.0 / samples;

    depth[0] = depthToZ(depth[0]);
    depth[1] = depthToZ(depth[1]);
    depth[2] = depthToZ(depth[2]);
    depth[3] = depthToZ(depth[3]);

    // don't need to mask out sky depth for min operation (because sky has a larger depth than everything else)
    float minDepth = min16( depth[0], depth[1], depth[2], depth[3] );

    depth[0] *= mask[0];
    depth[1] *= mask[1];
    depth[2] *= mask[2];
    depth[3] *= mask[3];

    float maxDepth = max16( depth[0], depth[1], depth[2], depth[3] );

    float avgDepth = dot( depth[0] + depth[1] + depth[2] + depth[3],
			  vec4( samples ) );

    depth[0] -= avgDepth * mask[0];
    depth[1] -= avgDepth * mask[1];
    depth[2] -= avgDepth * mask[2];
    depth[3] -= avgDepth * mask[3];

    float variance = dot( depth[0], depth[0] ) + dot( depth[1], depth[1] ) +
      dot( depth[2], depth[2] ) + dot( depth[3], depth[3] );
    variance *= samples;
    outputColor = vec4( maxDepth, minDepth, avgDepth, sqrt( variance ) );
  } else {
    // found just sky pixels
    outputColor = vec4( 99999.0, 99999.0, 99999.0, 0.0 );
  }
}
