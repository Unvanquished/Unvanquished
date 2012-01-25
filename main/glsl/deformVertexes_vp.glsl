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

#if !defined(GLDRV_MESA)

float triangle(float x)
{
	return max(1.0 - abs(x), 0);
}

float sawtooth(float x)
{
	return x - floor(x);
}

/*
vec4 DeformPosition(const int deformGen,
					const vec4 wave,	// [base amplitude phase freq]
					const vec3 bulge,	// [width height speed]
					const float spread,
					const float time,
					const vec4 pos,
					const vec3 normal,
					const vec2 st)
{
	vec4 deformed = pos;

	if(deformGen == DGEN_WAVE_SIN)
	{
		float off = (pos.x + pos.y + pos.z) * spread;
		float scale = wave.x  + sin(off + wave.z + (time * wave.w)) * wave.y;
		vec3 offset = normal * scale;

		deformed.xyz += offset;
	}
	
	if(deformGen == DGEN_WAVE_SQUARE)
	{
		float off = (pos.x + pos.y + pos.z) * spread;
		float scale = wave.x  + sign(sin(off + wave.z + (time * wave.w))) * wave.y;
		vec3 offset = normal * scale;

		deformed.xyz += offset;
	}
	
	if(deformGen == DGEN_WAVE_TRIANGLE)
	{
		float off = (pos.x + pos.y + pos.z) * spread;
		float scale = wave.x  + triangle(off + wave.z + (time * wave.w)) * wave.y;
		vec3 offset = normal * scale;

		deformed.xyz += offset;
	}
	
	if(deformGen == DGEN_WAVE_SAWTOOTH)
	{
		float off = (pos.x + pos.y + pos.z) * spread;
		float scale = wave.x  + sawtooth(off + wave.z + (time * wave.w)) * wave.y;
		vec3 offset = normal * scale;

		deformed.xyz += offset;
	}
	
	if(deformGen == DGEN_WAVE_INVERSE_SAWTOOTH)
	{
		float off = (pos.x + pos.y + pos.z) * spread;
		float scale = wave.x + (1.0 - sawtooth(off + wave.z + (time * wave.w))) * wave.y;
		vec3 offset = normal * scale;

		deformed.xyz += offset;
	}
	
	if(deformGen == DGEN_BULGE)
	{
		float bulgeWidth = bulge.x;
		float bulgeHeight = bulge.y;
		float bulgeSpeed = bulge.z;
	
		float now = time * bulgeSpeed;

		float off = (M_PI * 0.25) * st.x * bulgeWidth + now; 
		float scale = sin(off) * bulgeHeight;
		vec3 offset = normal * scale;

		deformed.xyz += offset;
	}

	return deformed;
}
*/

/*
	define	WAVEVALUE( table, base, amplitude, phase, freq ) \
		((base) + table[ Q_ftol( ( ( (phase) + backEnd.refdef.floatTime * (freq) ) * FUNCTABLE_SIZE ) ) & FUNCTABLE_MASK ] * (amplitude))
*/

float WaveValue(float func, float base, float amplitude, float phase, float freq, float time)
{
	if(func == GF_SIN)
		return base  + sin(phase + (time * freq)) * amplitude;
	
	if(func == GF_SQUARE)
		return base  + sign(sin(phase + (time * freq))) * amplitude;
	
	if(func == GF_TRIANGLE)
		return base  + triangle(phase + (time * freq)) * amplitude;
	
	if(func == GF_SAWTOOTH)
		return base  + sawtooth(phase + (time * freq)) * amplitude;
	
	if(func == GF_INVERSE_SAWTOOTH)
		return base + (1.0 - sawtooth(phase + (time * freq))) * amplitude;
	
	// if(func == GF_NOISE)
		// TODO
		
	return 0.0; // GF_NONE
}

vec4 DeformPosition2(	const vec4 pos,
						const vec3 normal,
						const vec2 st,
						float time)
{
	
	int i, deformOfs = 0;
	int numDeforms = int(u_DeformParms[deformOfs++]);
	
	vec4 deformed = pos;
	
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

			deformed.xyz += offset;
		}
		
		if(deformGen == DEFORM_BULGE)
		{
			float bulgeWidth = u_DeformParms[deformOfs++];
			float bulgeHeight = u_DeformParms[deformOfs++];
			float bulgeSpeed = u_DeformParms[deformOfs++];
		
			float now = time * bulgeSpeed;

			float off = (M_PI * 0.25) * st.x * bulgeWidth + now; 
			float scale = sin(off) * bulgeHeight;
			vec3 offset = normal * scale;

			deformed.xyz += offset;
		}
		
		if(deformGen == DEFORM_MOVE)
		{
			float func = u_DeformParms[deformOfs++];
			float base = u_DeformParms[deformOfs++];
			float amplitude = u_DeformParms[deformOfs++];
			float phase = u_DeformParms[deformOfs++];
			float freq = u_DeformParms[deformOfs++];
			
			vec3 move = vec3(u_DeformParms[deformOfs++], u_DeformParms[deformOfs++], u_DeformParms[deformOfs++]);
			float scale = WaveValue(func, base, amplitude, phase, freq, time);
			vec3 offset = move * scale;

			deformed.xyz += offset;
		}
	}
	
	return deformed;
}


#endif // !defined(GLDRV_MESA)

