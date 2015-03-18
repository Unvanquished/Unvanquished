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


uniform float		u_DeformParms[MAX_SHADER_DEFORM_PARMS];

float triangle(float x)
{
	return 1.0 - abs( 4.0 * fract( x + 0.25 ) - 2.0 );
}

float sawtooth(float x)
{
	return fract( x );
}

/*
	define	WAVEVALUE( table, base, amplitude, phase, freq ) \
		((base) + table[ Q_ftol( ( ( (phase) + backEnd.refdef.floatTime * (freq) ) * FUNCTABLE_SIZE ) ) & FUNCTABLE_MASK ] * (amplitude))
*/

float WaveValue(float func, float base, float amplitude, float phase, float freq, float time)
{
	float value = phase + ( time * freq );
	
	float d;
	
	if(func == GF_SIN)
		d = sin(value * 2.0 * M_PI) ;
	else if(func == GF_SQUARE)
		d = sign(sin(value * 2.0 * M_PI));
	else if(func == GF_TRIANGLE)
		d = triangle(value);
	else if(func == GF_SAWTOOTH)
		d = sawtooth(value);
	else
		d = 1.0 - sawtooth(value);

	return base + d * amplitude;
}

// --------------------------------------------------------------------
//
// Description : Array and textureless GLSL 2D/3D/4D simplex 
//               noise functions.
//      Author : Ian McEwan, Ashima Arts.
//  Maintainer : ijm
//     Lastmod : 20110822 (ijm)
//     License : Copyright (C) 2011 Ashima Arts. All rights reserved.
//               Distributed under the MIT License. See LICENSE file.
//               https://github.com/ashima/webgl-noise
// 

vec4 mod289(vec4 x) {
	return x - floor(x * (1.0 / 289.0)) * 289.0;
}

float mod289(float x) {
	return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec4 permute(vec4 x) {
	return mod289(((x*34.0)+1.0)*x);
}

float permute(float x) {
	return mod289(((x*34.0)+1.0)*x);
}

vec4 taylorInvSqrt(vec4 r) {
	return 1.79284291400159 - 0.85373472095314 * r;
}

float taylorInvSqrt(float r) {
	return 1.79284291400159 - 0.85373472095314 * r;
}

vec4 grad4(float j, vec4 ip) {
	const vec4 ones = vec4(1.0, 1.0, 1.0, -1.0);
	vec4 p,s;

	p.xyz = floor( fract (vec3(j) * ip.xyz) * 7.0) * ip.z - 1.0;
	p.w = 1.5 - dot(abs(p.xyz), ones.xyz);
	s = vec4(lessThan(p, vec4(0.0)));
	p.xyz = p.xyz + (s.xyz*2.0 - 1.0) * s.www; 

	return p;
}

// (sqrt(5) - 1)/4 = F4, used once below
const float F4 = 0.309016994374947451;
const float G4 = 0.138196601125011;

float snoise(vec4 v) {
	const vec4  C = vec4( G4, 2.0 * G4, 3.0 * G4, 4.0 * G4 - 1.0);

	// First corner
	vec4 i  = floor(v + dot(v, vec4(F4)) );
	vec4 x0 = v -   i + dot(i, C.xxxx);

	// Other corners

	// Rank sorting originally contributed by Bill Licea-Kane, AMD (formerly ATI)
	vec4 i0;
	vec3 isX = step( x0.yzw, x0.xxx );
	vec3 isYZ = step( x0.zww, x0.yyz );
	//  i0.x = dot( isX, vec3( 1.0 ) );
	i0.x = isX.x + isX.y + isX.z;
	i0.yzw = 1.0 - isX;
	//  i0.y += dot( isYZ.xy, vec2( 1.0 ) );
	i0.y += isYZ.x + isYZ.y;
	i0.zw += 1.0 - isYZ.xy;
	i0.z += isYZ.z;
	i0.w += 1.0 - isYZ.z;

	// i0 now contains the unique values 0,1,2,3 in each channel
	vec4 i3 = clamp( i0, 0.0, 1.0 );
	vec4 i2 = clamp( i0-1.0, 0.0, 1.0 );
	vec4 i1 = clamp( i0-2.0, 0.0, 1.0 );

	//  x0 = x0 - 0.0 + 0.0 * C.xxxx
	//  x1 = x0 - i1  + 1.0 * C.xxxx
	//  x2 = x0 - i2  + 2.0 * C.xxxx
	//  x3 = x0 - i3  + 3.0 * C.xxxx
	//  x4 = x0 - 1.0 + 4.0 * C.xxxx
	vec4 x1 = x0 - i1 + C.xxxx;
	vec4 x2 = x0 - i2 + C.yyyy;
	vec4 x3 = x0 - i3 + C.zzzz;
	vec4 x4 = x0 + C.wwww;

	// Permutations
	i = mod289(i);
	float j0 = permute( permute( permute( permute(i.w) + i.z) + i.y) + i.x);
	vec4 j1 = permute( permute( permute( permute (
						     i.w + vec4(i1.w, i2.w, i3.w, 1.0 ))
					     + i.z + vec4(i1.z, i2.z, i3.z, 1.0 ))
				    + i.y + vec4(i1.y, i2.y, i3.y, 1.0 ))
			   + i.x + vec4(i1.x, i2.x, i3.x, 1.0 ));

	// Gradients: 7x7x6 points over a cube, mapped onto a 4-cross polytope
	// 7*7*6 = 294, which is close to the ring size 17*17 = 289.
	vec4 ip = vec4(1.0/294.0, 1.0/49.0, 1.0/7.0, 0.0) ;

	vec4 p0 = grad4(j0,   ip);
	vec4 p1 = grad4(j1.x, ip);
	vec4 p2 = grad4(j1.y, ip);
	vec4 p3 = grad4(j1.z, ip);
	vec4 p4 = grad4(j1.w, ip);

	// Normalise gradients
	vec4 norm = taylorInvSqrt(vec4(dot(p0,p0), dot(p1,p1), dot(p2, p2), dot(p3,p3)));
	p0 *= norm.x;
	p1 *= norm.y;
	p2 *= norm.z;
	p3 *= norm.w;
	p4 *= taylorInvSqrt(dot(p4,p4));

	// Mix contributions from the five corners
	vec3 m0 = max(0.6 - vec3(dot(x0,x0), dot(x1,x1), dot(x2,x2)), 0.0);
	vec2 m1 = max(0.6 - vec2(dot(x3,x3), dot(x4,x4)            ), 0.0);
	m0 = m0 * m0;
	m1 = m1 * m1;
	return 49.0 * ( dot(m0*m0, vec3( dot( p0, x0 ), dot( p1, x1 ), dot( p2, x2 )))
			+ dot(m1*m1, vec2( dot( p3, x3 ), dot( p4, x4 ) ) ) ) ;
}
// --------------------------------------------------------------------

void DeformVertex( inout vec4 pos,
		   inout vec3 normal,
		   inout vec2 st,
		   in    float time)
{

	int i, deformOfs = 0;
	int numDeforms = int(u_DeformParms[deformOfs++]);

	for(i = 0; i < numDeforms; i++)
	{
		int deformGen = int(u_DeformParms[deformOfs++]);


		if(deformGen == DEFORM_WAVE)
		{
			float func = u_DeformParms[deformOfs++];
			float base = u_DeformParms[deformOfs++];
			float amplitude = u_DeformParms[deformOfs++];
			float phase = u_DeformParms[deformOfs++];
			float freq = u_DeformParms[deformOfs++];

			float spread = u_DeformParms[deformOfs++];

			float off = (pos.x + pos.y + pos.z) * spread;
			float scale = WaveValue(func, base, amplitude, phase + off, freq, time);
			vec3 offset = normal * scale;

			pos.xyz += offset;
		}
		else if(deformGen == DEFORM_BULGE)
		{
			float bulgeWidth = u_DeformParms[deformOfs++];
			float bulgeHeight = u_DeformParms[deformOfs++];
			float bulgeSpeed = u_DeformParms[deformOfs++];

			float now = time * bulgeSpeed;

			float off = st.x * bulgeWidth + now;
			float scale = sin(off) * bulgeHeight;
			vec3 offset = normal * scale;

			pos.xyz += offset;
		}
		else if(deformGen == DEFORM_MOVE)
		{
			float func = u_DeformParms[deformOfs++];
			float base = u_DeformParms[deformOfs++];
			float amplitude = u_DeformParms[deformOfs++];
			float phase = u_DeformParms[deformOfs++];
			float freq = u_DeformParms[deformOfs++];

			vec3 move;
			move.x = u_DeformParms[deformOfs++];
			move.y = u_DeformParms[deformOfs++];
			move.z = u_DeformParms[deformOfs++];
			
			float scale = WaveValue(func, base, amplitude, phase, freq, time);
			vec3 offset = move * scale;

			pos.xyz += offset;
		}
		else if(deformGen == DEFORM_NORMALS)
		{
			float amplitude = u_DeformParms[deformOfs++];
			float freq = u_DeformParms[deformOfs++];

			vec3 noise;
			noise.x = snoise(vec4(pos.xyz, time * freq));
			noise.y = snoise(vec4(pos.xyz, time * freq));
			noise.z = snoise(vec4(pos.xyz, time * freq));

			normal += (0.98 * amplitude) * noise;
			normal = normalize( normal );
		}
	}
}
