/*
===========================================================================
Copyright (C) 2007-2011 Robert Beckebans <trebor_7@users.sourceforge.net>

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

/* geometricFill_fp.glsl */

#extension GL_ARB_draw_buffers : enable

uniform sampler2D	u_DiffuseMap;
uniform sampler2D	u_NormalMap;
uniform sampler2D	u_SpecularMap;

uniform samplerCube	u_EnvironmentMap0;
uniform samplerCube	u_EnvironmentMap1;
uniform float		u_EnvironmentInterpolation;

uniform int			u_AlphaTest;
uniform vec3		u_ViewOrigin;
uniform vec3        u_AmbientColor;
uniform float		u_DepthScale;
uniform mat4		u_ModelMatrix;
uniform vec4		u_PortalPlane;

varying vec4		var_Position;
varying vec2		var_TexDiffuse;
#if defined(USE_NORMAL_MAPPING)
varying vec2		var_TexNormal;
#if !defined(r_DeferredLighting)
varying vec2		var_TexSpecular;
#endif
varying vec3		var_Tangent;
varying vec3		var_Binormal;
#endif
varying vec3		var_Normal;
varying vec4		var_Color;




void	main()
{

#if defined(USE_PORTAL_CLIPPING)
	{
		float dist = dot(var_Position.xyz, u_PortalPlane.xyz) - u_PortalPlane.w;
		if(dist < 0.0)
		{
			discard;
			return;
		}
	}
#endif

	// compute view direction in world space
	vec3 V = normalize(u_ViewOrigin - var_Position.xyz);

	vec2 texDiffuse = var_TexDiffuse.st;

#if defined(USE_NORMAL_MAPPING)
	// invert tangent space for two sided surfaces
	mat3 tangentToWorldMatrix;
	
#if defined(TWOSIDED)
	if(gl_FrontFacing)
	{
		tangentToWorldMatrix = mat3(-var_Tangent.xyz, -var_Binormal.xyz, -var_Normal.xyz);
	}
	else
#endif
	{
		tangentToWorldMatrix = mat3(var_Tangent.xyz, var_Binormal.xyz, var_Normal.xyz);
	}
	
	vec2 texNormal = var_TexNormal.st;
#if !defined(r_DeferredLighting)
	vec2 texSpecular = var_TexSpecular.st;
#endif

#if defined(USE_PARALLAX_MAPPING)
	
	// ray intersect in view direction
	
	mat3 worldToTangentMatrix;
	#if defined(GLHW_ATI) || defined(GLHW_ATI_DX10) || defined(GLDRV_MESA)
	worldToTangentMatrix = mat3(tangentToWorldMatrix[0][0], tangentToWorldMatrix[1][0], tangentToWorldMatrix[2][0],
								tangentToWorldMatrix[0][1], tangentToWorldMatrix[1][1], tangentToWorldMatrix[2][1], 
								tangentToWorldMatrix[0][2], tangentToWorldMatrix[1][2], tangentToWorldMatrix[2][2]);
	#else
	worldToTangentMatrix = transpose(tangentToWorldMatrix);
	#endif

	// compute view direction in tangent space
	vec3 Vts = worldToTangentMatrix * (u_ViewOrigin - var_Position.xyz);
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
	
	texNormal.st += texOffset;
	
#if !defined(r_DeferredLighting)
	texDiffuse.st += texOffset;
	texSpecular.st += texOffset;
#endif
	
#endif // USE_PARALLAX_MAPPING

	// compute normal in world space from normalmap
	vec3 N = normalize(tangentToWorldMatrix * (2.0 * (texture2D(u_NormalMap, texNormal).xyz - 0.5)));
	
#if !defined(r_DeferredLighting)
	
	// compute the specular term
#if defined(USE_REFLECTIVE_SPECULAR)

	vec3 specular = texture2D(u_SpecularMap, texSpecular).rgb;

	vec4 envColor0 = textureCube(u_EnvironmentMap0, reflect(-V, N)).rgba;
	vec4 envColor1 = textureCube(u_EnvironmentMap1, reflect(-V, N)).rgba;
	
	specular *= mix(envColor0, envColor1, u_EnvironmentInterpolation).rgb;
	
#if 0
	// specular = vec4(u_EnvironmentInterpolation, u_EnvironmentInterpolation, u_EnvironmentInterpolation, 1.0);
	specular = envColor0;
#endif

#else

	vec3 specular = texture2D(u_SpecularMap, texSpecular).rgb;
#endif // USE_REFLECTIVE_SPECULAR

#endif // #if !defined(r_DeferredLighting)


#else // USE_NORMAL_MAPPING
	
	vec3 N;

#if defined(TWOSIDED)
	if(gl_FrontFacing)
	{
		N = -normalize(var_Normal);
		// gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
		// return;
	}
	else
#endif
	{
		N = normalize(var_Normal);
	}
		
	vec3 specular = vec3(0.0);
	
#endif // USE_NORMAL_MAPPING


	// compute the diffuse term
	vec4 diffuse = texture2D(u_DiffuseMap, texDiffuse);
	
#if defined(USE_ALPHA_TESTING)
	if(u_AlphaTest == ATEST_GT_0 && diffuse.a <= 0.0)
	{
		discard;
		return;
	}
	else if(u_AlphaTest == ATEST_LT_128 && diffuse.a >= 0.5)
	{
		discard;
		return;
	}
	else if(u_AlphaTest == ATEST_GE_128 && diffuse.a < 0.5)
	{
		discard;
		return;
	}
#endif
	
//	vec4 depthColor = diffuse;
//	depthColor.rgb *= u_AmbientColor;
	
	// convert normal back to [0,1] color space
	N = N * 0.5 + 0.5;

#if defined(r_DeferredLighting)
	gl_FragColor = vec4(N, 0.0);
#else
	gl_FragData[0] = vec4(0.0, 0.0, 0.0, 1.0);
	gl_FragData[1] = vec4(diffuse.rgb, var_Color.a);
	gl_FragData[2] = vec4(N, var_Color.a);
	gl_FragData[3] = vec4(specular, var_Color.a);

#endif // r_DeferredLighting
}


