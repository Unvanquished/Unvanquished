/*
===========================================================================
Copyright (C) 2009-2011 Robert Beckebans <trebor_7@users.sourceforge.net>

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
// deformVertexes_vp.glsl - Quake 3 deformVertexes semantic


uniform vec4 u_DeformParms[MAX_SHADER_DEFORM_PARMS];

float triangle(float x)
{
	return 1.0 - abs( 4.0 * fract( x + 0.25 ) - 2.0 );
}

float sawtooth(float x)
{
	return fract( x );
}

#if 0
// --------------------------------------------------------------------
ivec4 permute(ivec4 x) {
	return ((62 * x + 1905) * x) % 961;
}

float grad(int idx, vec4 p) {
	int i = idx & 31;
	float u = i < 24 ? p.x : p.y;
	float v = i < 16 ? p.y : p.z;
	float w = i < 8  ? p.z : p.w;

	return ((i & 4) != 0 ? -u : u)
	     + ((i & 2) != 0 ? -v : v)
	     + ((i & 1) != 0 ? -w : w);
}

vec4 fade(in vec4 t) {
	return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
}

vec4 pnoise(vec4 v) {
	ivec4 iv = ivec4(floor(v));
	vec4 rv = fract(v);
	vec4 f[2];

	f[0] = fade(rv);
	f[1] = 1.0 - f[0];

	vec4 value = vec4(0.0);
	for(int idx = 0; idx < 16; idx++) {
		ivec4 offs = ivec4( idx & 1, (idx & 2) >> 1, (idx & 4) >> 2, (idx & 8) >> 3 );
		float w = f[offs.x].x * f[offs.y].y * f[offs.z].z * f[offs.w].w;
		ivec4 p = permute(iv + offs);

		value.x += w * grad(p.x, rv);
		value.y += w * grad(p.y, rv);
		value.z += w * grad(p.z, rv);
		value.w += w * grad(p.w, rv);
	}
	return value;
}
// --------------------------------------------------------------------
#endif

void DeformVertex( inout vec4 pos,
		   inout vec3 normal,
		   inout vec2 st,
		   inout vec4 color,
		   in    float time)
{
	vec4 work = vec4(0.0);

	for(int deformOfs = 0; deformOfs < MAX_SHADER_DEFORM_PARMS; deformOfs++)
	{
		vec4 parms = u_DeformParms[ deformOfs ];
		int  cmd   = int(parms.w);

		if( cmd == DSTEP_LOAD_POS ) {
			work.xyz = pos.xyz * parms.xyz;
		} else if( cmd == DSTEP_LOAD_NORM ) {
			work.xyz = normal.xyz * parms.xyz;
		} else if( cmd == DSTEP_LOAD_COLOR ) {
			work.xyz = color.xyz * parms.xyz;
		} else if( cmd == DSTEP_LOAD_TC ) {
			work.xyz = vec3(st, 1.0) * parms.xyz;
		} else if( cmd == DSTEP_LOAD_VEC ) {
			work.xyz = parms.xyz;
		} else if( cmd == DSTEP_MODIFY_POS ) {
			pos.xyz += (parms.x + parms.y * work.a) * work.xyz;
		} else if( cmd == DSTEP_MODIFY_NORM ) {
			normal.xyz += (parms.x + parms.y * work.a) * work.xyz;
			normal = normalize(normal);
		} else if( cmd == DSTEP_MODIFY_COLOR ) {
			color.xyz += (parms.x + parms.y * work.a) * work.xyz;
		} else if( cmd == DSTEP_SIN ) {
			work.a = sin( 2.0 * M_PI * (parms.x + parms.y * (work.x + work.y + work.z) + parms.z * time) );
		} else if( cmd == DSTEP_SQUARE ) {
			work.a = sign(sin( 2.0 * M_PI * (parms.x + parms.y * (work.x + work.y + work.z) + parms.z * time) ) );
		} else if( cmd == DSTEP_TRIANGLE ) {
			work.a = triangle(parms.x + parms.y * (work.x + work.y + work.z) + parms.z * time);
		} else if( cmd == DSTEP_SAWTOOTH ) {
			work.a = sawtooth(parms.x + parms.y * (work.x + work.y + work.z) + parms.z * time);
		} else if( cmd == DSTEP_INVERSE_SAWTOOTH ) {
			work.a = 1.0 - sawtooth(parms.x + parms.y * (work.x + work.y + work.z) + parms.z * time);
		} else if( cmd == DSTEP_NOISE ) {
			//work = pnoise(vec4(parms.y * work.xyz, parms.z * time));
			work = noise4(vec4(parms.y * work.xyz, parms.z * time));
		} else if( cmd == DSTEP_ROTGROW ) {
			if(work.z > parms.x * time)
				work.a = 0.0;
			else {
				work.a = parms.y * atan(pos.y, pos.x) + parms.z * time;
				work.a = 0.5 * sin(work.a) + 0.5;
			}
		} else
			break;
	}
}
