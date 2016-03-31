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
// reliefMapping_vp.glsl - Relief mapping helper functions

#if defined( USE_SHADER_LIGHTS )
#extension GL_ARB_uniform_buffer_object : require
#if __VERSION__ < 130
#extension GL_EXT_texture_integer : enable
#extension GL_EXT_gpu_shader4 : enable
#if defined( GL_EXT_texture_integer ) && defined( GL_EXT_gpu_shader4 )
#define HAVE_INTTEX 1
#endif
#else
#define HAVE_INTTEX 1
#endif

struct light {
  vec3  center;
  float radius;
  vec3  color;
  float type;
  vec3  direction;
  float angle;
};

layout(std140) uniform u_Lights {
  light lights[ MAX_REF_LIGHTS ];
};

uniform int u_numLights;
#endif

uniform vec2 u_SpecularExponent;

// lighting helper functions
void computeLight( vec3 lightDir, vec3 normal, vec3 eyeDir, vec3 lightColor,
		   vec4 diffuseColor, vec4 specularColor,
		   inout vec4 accumulator ) {
  vec3 H = normalize( lightDir + eyeDir );
  float NdotL = dot( normal, lightDir );
#if defined(r_HalfLambertLighting)
  // http://developer.valvesoftware.com/wiki/Half_Lambert
  NdotL = NdotL * 0.5 + 0.5;
  NdotL *= NdotL;
#elif defined(r_WrapAroundLighting)
  NdotL = clamp( NdotL + r_WrapAroundLighting, 0.0, 1.0) / clamp(1.0 + r_WrapAroundLighting, 0.0, 1.0);
#else
  NdotL = clamp( NdotL, 0.0, 1.0 );
#endif
  float NdotH = clamp( dot( normal, H ), 0.0, 1.0 );

  accumulator.xyz += diffuseColor.xyz * lightColor.xyz * NdotL;
  accumulator.xyz += specularColor.xyz * lightColor.xyz * pow( NdotH, u_SpecularExponent.x * specularColor.w + u_SpecularExponent.y) * r_SpecularScale;
}

#if defined( USE_SHADER_LIGHTS )

#if defined( HAVE_INTTEX )
const int lightsPerLayer = 16;
uniform usampler3D u_LightTiles;
#define idxs_t uvec4
idxs_t fetchIdxs( in vec3 coords ) {
  return texture( u_LightTiles, coords );
}
int nextIdx( inout idxs_t idxs ) {
  uvec4 tmp = ( idxs & uvec4( 3 ) ) * uvec4( 0x40, 0x10, 0x04, 0x01 );
  idxs = idxs >> 2;
  return int( tmp.x + tmp.y + tmp.z + tmp.w );
}
#else
const int lightsPerLayer = 4;
uniform sampler3D u_LightTiles;
#define idxs_t vec4
idxs_t fetchIdxs( in vec3 coords ) {
  return texture3D( u_LightTiles, coords ) * 255.0;
}
int nextIdx( inout idxs_t idxs ) {
  vec4 tmp = idxs;
  idxs = floor(idxs * 0.25);
  tmp -= 4.0 * idxs;
  return int( dot( tmp, vec4( 64.0, 16.0, 4.0, 1.0 ) ) );
}
#endif

const int numLayers = MAX_REF_LIGHTS / 256;

void computeDLight( int idx, vec3 P, vec3 N, vec3 I, vec4 diffuse,
		    vec4 specular, inout vec4 color ) {
  vec3 L = lights[idx].center - P;
  float attenuation = 1.0 / (1.0 + 8.0 * length(L) / lights[idx].radius);
  computeLight( normalize( L ), N, I,
		attenuation * attenuation * lights[idx].color,
		diffuse, specular, color );
}

void computeDLights( vec3 P, vec3 N, vec3 I, vec4 diffuse, vec4 specular,
		     inout vec4 color ) {
  vec2 tile = floor( gl_FragCoord.xy * (1.0 / float( TILE_SIZE ) ) ) + 0.5;
  vec3 tileScale = vec3( r_tileStep, 1.0/numLayers );

  for( int layer = 0; layer < numLayers; layer++ ) {
    idxs_t idxs = fetchIdxs( tileScale * vec3( tile, float( layer ) + 0.5 ) );
    for( int i = 0; i < lightsPerLayer; i++ ) {
      int idx = numLayers * nextIdx( idxs ) + layer;

      if( idx > u_numLights )
	return;
      computeDLight( idx, P, N, I, diffuse, specular, color );
    }
  }
}
#endif

#if defined(USE_PARALLAX_MAPPING)
float RayIntersectDisplaceMap(vec2 dp, vec2 ds, sampler2D normalMap)
{
	const int linearSearchSteps = 16;
	const int binarySearchSteps = 6;

	float depthStep = 1.0 / float(linearSearchSteps);

	// current size of search window
	float size = depthStep;

	// current depth position
	float depth = 0.0;

	// best match found (starts with last position 1.0)
	float bestDepth = 1.0;

	// search front to back for first point inside object
	for(int i = 0; i < linearSearchSteps - 1; ++i)
	{
		depth += size;

		vec4 t = texture2D(normalMap, dp + ds * depth);

		if(bestDepth > 0.996)		// if no depth found yet
			if(depth >= t.w)
				bestDepth = depth;	// store best depth
	}

	depth = bestDepth;

	// recurse around first point (depth) for closest match
	for(int i = 0; i < binarySearchSteps; ++i)
	{
		size *= 0.5;

		vec4 t = texture2D(normalMap, dp + ds * depth);

		if(depth >= t.w)
		#ifdef RM_DOUBLEDEPTH
			if(depth <= t.z)
		#endif
			{
				bestDepth = depth;
				depth -= 2.0 * size;
			}

		depth += size;
	}

	return bestDepth;
}
#endif



