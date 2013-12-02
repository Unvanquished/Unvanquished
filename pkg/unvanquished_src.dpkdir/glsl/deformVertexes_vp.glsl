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
		else if(deformGen == DEFORM_BULGE)
		{
			float bulgeWidth = u_DeformParms[deformOfs++];
			float bulgeHeight = u_DeformParms[deformOfs++];
			float bulgeSpeed = u_DeformParms[deformOfs++];

			float now = time * bulgeSpeed;

			float off = st.x * bulgeWidth + now;
			float scale = sin(off) * bulgeHeight;
			vec3 offset = normal * scale;

			deformed.xyz += offset;
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

			deformed.xyz += offset;
		}
	}

	return deformed;
}


#endif // !defined(GLDRV_MESA)

