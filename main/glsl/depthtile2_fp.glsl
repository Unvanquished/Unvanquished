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

/* depthtile2_fp.glsl */

uniform sampler2D u_DepthMap;

const vec2 pixelScale = r_FBufScale * r_NPOTScale;

void	main()
{
  vec2 st = gl_FragCoord.st * 16.0 * pixelScale;
  float x, y;
  vec4 accum = vec4( 0.0, 99999.0, 0.0, 0.0 );
  float count = 0.0;

  for( x = -1.5; x < 2.0; x += 1.0 ) {
    for( y = -1.5; y < 2.0; y += 1.0 ) {
      vec4 data = texture2D( u_DepthMap, st + vec2(x, y) * pixelScale );
      if( data.y < 99999.0 ) {
	accum.x = max( accum.x, data.x );
	accum.y = min( accum.y, data.y );
	accum.zw += data.zw;
	count += 1.0;
      }
    }
  }

  if( count >= 1.0 ) {
    accum.zw *= 1.0 / count;
  }

  gl_FragColor = accum;
}
