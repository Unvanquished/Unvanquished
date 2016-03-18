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

/* vertexLighting_DBS_entity_fp.glsl */

uniform sampler2D	u_DiffuseMap;
uniform sampler2D	u_NormalMap;
uniform sampler2D	u_SpecularMap;
uniform sampler2D	u_GlowMap;

uniform samplerCube	u_EnvironmentMap0;
uniform samplerCube	u_EnvironmentMap1;
uniform float		u_EnvironmentInterpolation;

uniform float		u_AlphaThreshold;
uniform vec3		u_ViewOrigin;
uniform float		u_DepthScale;

uniform sampler3D       u_LightGrid1;
uniform sampler3D       u_LightGrid2;
uniform vec3            u_LightGridOrigin;
uniform vec3            u_LightGridScale;

varying vec3		var_Position;
varying vec2		var_TexDiffuse;
varying vec4		var_Color;
varying vec4		var_TexNormalSpecular;
varying vec3		var_Tangent;
varying vec3		var_Binormal;
varying vec2		var_TexGlow;
varying vec3		var_Normal;

#if __VERSION__ > 120
out vec4 outputColor;
#else
#define outputColor gl_FragColor
#endif

void ReadLightGrid(in vec3 pos, out vec3 lgtDir,
		   out vec3 ambCol, out vec3 lgtCol ) {
	vec4 texel1 = texture3D(u_LightGrid1, pos);
	vec4 texel2 = texture3D(u_LightGrid2, pos);

	ambCol = texel1.xyz;
	lgtCol = texel2.xyz;

	lgtDir.x = (texel1.w * 255.0 - 128.0) / 127.0;
	lgtDir.y = (texel2.w * 255.0 - 128.0) / 127.0;
	lgtDir.z = 1.0 - abs( lgtDir.x ) - abs( lgtDir.y );

	vec2 signs = 2.0 * step( 0.0, lgtDir.xy ) - vec2( 1.0 );
	if( lgtDir.z < 0.0 ) {
		lgtDir.xy = signs * ( vec2( 1.0 ) - abs( lgtDir.yx ) );
	}

	lgtDir = normalize( lgtDir );
}

void	main()
{
	// compute light direction in world space
	vec3 L;
	vec3 ambCol;
	vec3 lgtCol;

	ReadLightGrid( (var_Position - u_LightGridOrigin) * u_LightGridScale,
		       L, ambCol, lgtCol );

	// compute view direction in world space
	vec3 V = normalize(u_ViewOrigin - var_Position);

	vec2 texDiffuse = var_TexDiffuse.st;

	mat3 tangentToWorldMatrix = mat3(var_Tangent.xyz, var_Binormal.xyz, var_Normal.xyz);

	vec2 texNormal = var_TexNormalSpecular.xy;
	vec2 texSpecular = var_TexNormalSpecular.zw;

#if defined(USE_PARALLAX_MAPPING)

	// ray intersect in view direction

	// compute view direction in tangent space
	vec3 Vts = (u_ViewOrigin - var_Position.xyz) * tangentToWorldMatrix;
	Vts = normalize(Vts);

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

	// compute normal in world space from normalmap
	vec3 N = texture2D(u_NormalMap, texNormal.st).xyw;
	N.x *= N.z;
	N.xy = 2.0 * N.xy - 1.0;
	N.z = sqrt(1.0 - dot(N.xy, N.xy));

#if defined(r_NormalScale)
	N.z *= r_NormalScale;
#endif

	N = normalize(tangentToWorldMatrix * N);

	// compute the specular term
#if defined(USE_REFLECTIVE_SPECULAR)

	vec4 specBase = texture2D(u_SpecularMap, texSpecular).rgba;

	vec4 envColor0 = textureCube(u_EnvironmentMap0, reflect(-V, N)).rgba;
	vec4 envColor1 = textureCube(u_EnvironmentMap1, reflect(-V, N)).rgba;

	specBase.rgb *= mix(envColor0, envColor1, u_EnvironmentInterpolation).rgb;

#else
	// simple Blinn-Phong
	vec4 specBase = texture2D(u_SpecularMap, texSpecular).rgba;

#endif // USE_REFLECTIVE_SPECULAR

	// compute the diffuse term
	vec4 diffuse = texture2D(u_DiffuseMap, texDiffuse) * var_Color;

	if( abs(diffuse.a + u_AlphaThreshold) <= 1.0 )
	{
		discard;
		return;
	}

// add Rim Lighting to highlight the edges
#if defined(r_RimLighting)
	float rim = pow(1.0 - clamp(dot(N, V), 0.0, 1.0), r_RimExponent);
	vec3 emission = ambCol * rim * rim * 0.2;
#endif

	// compute final color
	vec4 color = vec4(ambCol * r_AmbientScale * diffuse.xyz, diffuse.a);
	computeLight( L, N, V, lgtCol, diffuse, specBase, color );

	computeDLights( var_Position, N, V, diffuse, specBase, color );

#if defined(r_RimLighting)
	color.rgb += 0.7 * emission;
#endif

	color.rgb += texture2D(u_GlowMap, var_TexGlow).rgb;

	// convert normal to [0,1] color space
	N = N * 0.5 + 0.5;

	outputColor = color;

// Debugging
#if defined(r_showEntityNormals)
	outputColor = vec4(N, 1.0);
#endif
}
