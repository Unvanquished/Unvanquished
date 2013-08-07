/*
===========================================================================
Copyright (C) 2008-2011 Robert Beckebans <trebor_7@users.sourceforge.net>

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

/* vertexLighting_DBS_world_fp.glsl */

uniform sampler2D	u_DiffuseMap;
uniform sampler2D	u_NormalMap;
uniform sampler2D	u_SpecularMap;
uniform sampler2D	u_GlowMap;

uniform float		u_AlphaThreshold;
uniform float		u_DepthScale;
uniform	float		u_LightWrapAround;
uniform vec2		u_SpecularExponent;

varying vec4		var_TexDiffuseGlow;

#if defined(USE_NORMAL_MAPPING)
varying vec4		var_TexNormalSpecular;
varying vec3		var_ViewDir; // direction from surface to viewer
varying vec3		var_AmbientLight;
varying vec3		var_DirectedLight;
varying vec3		var_LightDirection; // direction from surface to light
#else
varying vec4		var_LightColor;
varying vec3		var_Normal;
#endif



void	main()
{

vec2 texDiffuse = var_TexDiffuseGlow.st;
vec2 texGlow = var_TexDiffuseGlow.pq;

#if defined(USE_NORMAL_MAPPING)
	vec3 V = normalize(var_ViewDir);
	vec3 L = normalize(var_LightDirection);

#if defined(TWOSIDED)
	if(gl_FrontFacing)
	{
		V = -V;
		L = -L;
	}
#endif

	vec2 texNormal = var_TexNormalSpecular.st;
	vec2 texSpecular = var_TexNormalSpecular.pq;

#if defined(USE_PARALLAX_MAPPING)

	// ray intersect in view direction
	vec3 Vts = V;
	
	// size and start position of search in texture space
	vec2 S = Vts.xy * -u_DepthScale / Vts.z;

#if 0
	vec2 texOffset = vec2(0.0);
	for(int i = 0; i < 4; i++) {
		vec4 Normal = texture2D(u_NormalMap, texNormal.st + texOffset);
		float height = Normal.a * 0.2 - 0.0125;
		texOffset += height * Normal.z * S;
	}
#else
	float depth = RayIntersectDisplaceMap(texNormal, S, u_NormalMap);

	// compute texcoords offset
	vec2 texOffset = S * depth;
#endif

	texDiffuse.st += texOffset;
	texNormal.st += texOffset;
	texSpecular.st += texOffset;
#endif // USE_PARALLAX_MAPPING

	// compute the diffuse term
	vec4 diffuse = texture2D(u_DiffuseMap, texDiffuse);

	if( abs(diffuse.a + u_AlphaThreshold) <= 1.0 )
	{
		discard;
		return;
	}

	// compute normal in tangent space from normalmap
	vec3 N = 2.0 * (texture2D(u_NormalMap, texNormal).xyz - 0.5);
	
	#if defined(r_NormalScale)
	N.z *= r_NormalScale;
	#endif
	
	N = normalize(N);

 	// compute half angle in tangent space
	vec3 H = normalize(L + V);

	// compute the light term
#if defined(r_WrapAroundLighting)
	float NL = clamp(dot(N, L) + u_LightWrapAround, 0.0, 1.0) / clamp(1.0 + u_LightWrapAround, 0.0, 1.0);
#else
	float NL = clamp(dot(N, L), 0.0, 1.0);
#endif
	vec3 light = var_AmbientLight + var_DirectedLight.rgb * NL;

	// compute the specular term
	vec4 spec = texture2D(u_SpecularMap, texSpecular).rgba;
	vec3 specular = spec.rgb * var_DirectedLight.rgb * pow(clamp(dot(N, H), 0.0, 1.0), u_SpecularExponent.x * spec.a + u_SpecularExponent.y) * r_SpecularScale;

	// compute final color
	vec4 color = vec4(diffuse.rgb, 1.0);
	color.rgb *= light;
	color.rgb += specular;

	#if defined(USE_GLOW_MAPPING)
	color.rgb += texture2D(u_GlowMap, texGlow).rgb;
	#endif
#if defined(r_DeferredShading)
	gl_FragData[0] = color; 								// var_Color;
	gl_FragData[1] = vec4(diffuse.rgb, var_LightColor.a);	// vec4(var_Color.rgb, 1.0 - var_Color.a);
	gl_FragData[2] = vec4(N, var_LightColor.a);
	gl_FragData[3] = vec4(specular, var_LightColor.a);
#else
	gl_FragColor = color;
#endif
#else // USE_NORMAL_MAPPING

	vec3 N = normalize(var_Normal);

#if defined(TWOSIDED)
	if(gl_FrontFacing)
	{
		N = -N;
	}
#endif

	// compute the diffuse term
	vec4 diffuse = texture2D(u_DiffuseMap, texDiffuse);

	if( abs(diffuse.a + u_AlphaThreshold) <= 1.0 )
	{
		discard;
		return;
	}

	vec4 color = vec4(diffuse.rgb * var_LightColor.rgb, var_LightColor.a);
#if defined(USE_GLOW_MAPPING)
	color.rgb += texture2D(u_GlowMap, texGlow).rgb;
#endif
	// gl_FragColor = vec4(diffuse.rgb * var_LightColor.rgb, diffuse.a);
	// color = vec4(vec3(1.0, 0.0, 0.0), diffuse.a);
	// gl_FragColor = vec4(vec3(diffuse.a, diffuse.a, diffuse.a), 1.0);
	// gl_FragColor = vec4(vec3(var_LightColor.a, var_LightColor.a, var_LightColor.a), 1.0);
	// gl_FragColor = var_LightColor;

#if 0 //defined(r_ShowTerrainBlends)
	color = vec4(vec3(var_LightColor.a), 1.0);
#endif

#if defined(r_DeferredShading)
	gl_FragData[0] = color;
	gl_FragData[1] = vec4(diffuse.rgb, var_LightColor.a);
	gl_FragData[2] = vec4(N, var_LightColor.a);
	gl_FragData[3] = vec4(0.0, 0.0, 0.0, var_LightColor.a);
#else
	gl_FragColor = color;
#endif

#endif

#if 0
#if defined(USE_PARALLAX_MAPPING)
	gl_FragColor = vec4(vec3(1.0, 0.0, 0.0), 1.0);
#elif defined(USE_NORMAL_MAPPING)
	gl_FragColor = vec4(vec3(0.0, 0.0, 1.0), 1.0);
#else
	gl_FragColor = vec4(vec3(0.0, 1.0, 0.0), 1.0);
#endif
#endif
}
