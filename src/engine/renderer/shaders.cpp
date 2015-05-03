#include <string>
#include <unordered_map>

std::unordered_map<std::string, std::string> shadermap(
  {{ "glsl/deformVertexes_vp.glsl", R"(
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


float waveSin(float x) {
	return sin( radians( 360.0 * x ) );
}

float waveSquare(float x) {
	return sign( waveSin( x ) );
}

float waveTriangle(float x)
{
	return 1.0 - abs( 4.0 * fract( x + 0.25 ) - 2.0 );
}

float waveSawtooth(float x)
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

// spread provides some variation on location
#define spread (dot(work.xyz, vec3(1.0)))

#define DSTEP_LOAD_POS(X,Y,Z)     work.xyz = pos.xyz * vec3(X,Y,Z);
#define DSTEP_LOAD_NORM(X,Y,Z)    work.xyz = normal.xyz * vec3(X,Y,Z);
#define DSTEP_LOAD_COLOR(X,Y,Z)   work.xyz = color.xyz * vec3(X,Y,Z);
#define DSTEP_LOAD_TC(X,Y,Z)      work.xyz = vec3(st, 1.0) * vec3(X,Y,Z);
#define DSTEP_LOAD_VEC(X,Y,Z)     work.xyz = vec3(X,Y,Z);
#define DSTEP_MODIFY_POS(X,Y,Z)   pos.xyz += (X + Y * work.a) * work.xyz;
#define DSTEP_MODIFY_NORM(X,Y,Z)  normal.xyz += (X + Y * work.a) * work.xyz; \
				  normal = normalize(normal);
#define DSTEP_MODIFY_COLOR(X,Y,Z) color.xyz += (X + Y * work.a) * work.xyz;
#define DSTEP_SIN(X,Y,Z)          work.a = waveSin( X + Y * spread + Z * time );
#define DSTEP_SQUARE(X,Y,Z)       work.a = waveSquare( X + Y * spread + Z * time);
#define DSTEP_TRIANGLE(X,Y,Z)     work.a = waveTriangle( X + Y * spread + Z * time);
#define	DSTEP_SAWTOOTH(X,Y,Z)     work.a = waveSawtooth( X + Y * spread + Z * time);
#define DSTEP_INV_SAWTOOTH(X,Y,Z) work.a = 1.0 - waveSawtooth( X + Y * spread + Z * time);
#define DSTEP_NOISE(X,Y,Z)        work = noise4(vec4( Y * work.xyz, Z * time));
#define DSTEP_ROTGROW(X,Y,Z)      if(work.z > X * time) { work.a = 0.0; } else { work.a = Y * atan(pos.y, pos.x) + Z * time; work.a = 0.5 * sin(work.a) + 0.5; }

	// this macro has to be #defined by the shader compiler
	DEFORM_STEPS
})" }}
);
