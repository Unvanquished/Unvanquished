/*
===========================================================================
Copyright (C) 2007-2009 Robert Beckebans <trebor_7@users.sourceforge.net>

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

/* liquid_fp.glsl */

uniform sampler2D	u_CurrentMap;
uniform sampler2D	u_PortalMap;
uniform sampler2D	u_DepthMap;
uniform sampler2D	u_NormalMap;
uniform vec3		u_ViewOrigin;
uniform float		u_FogDensity;
uniform vec3		u_FogColor;
uniform float		u_RefractionIndex;
uniform float		u_FresnelPower;
uniform float		u_FresnelScale;
uniform float		u_FresnelBias;
uniform float		u_NormalScale;
uniform mat4		u_ModelMatrix;
uniform mat4		u_UnprojectMatrix;

varying vec3		var_Position;
varying vec2		var_TexNormal;
varying vec3		var_Tangent;
varying vec3		var_Binormal;
varying vec3		var_Normal;
varying vec4		var_LightColor;
varying vec3		var_LightDirection;

#if defined(r_ParallaxMapping)
float RayIntersectDisplaceMap(vec2 dp, vec2 ds)
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
		
		vec4 t = texture2D(u_NormalMap, dp + ds * depth);

		if(bestDepth > 0.996)		// if no depth found yet
			if(depth >= t.w)
				bestDepth = depth;	// store best depth
	}

	depth = bestDepth;
	
	// recurse around first point (depth) for closest match
	for(int i = 0; i < binarySearchSteps; ++i)
	{
		size *= 0.5;

		vec4 t = texture2D(u_NormalMap, dp + ds * depth);
		
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


void	main()
{
	// compute incident ray
	vec3 I = normalize(u_ViewOrigin - var_Position);
	
	mat3 tangentToWorldMatrix;
	if(gl_FrontFacing)
		tangentToWorldMatrix = mat3(-var_Tangent.xyz, -var_Binormal.xyz, -var_Normal.xyz);
	else
		tangentToWorldMatrix = mat3(var_Tangent.xyz, var_Binormal.xyz, var_Normal.xyz);
	
	mat3 worldToTangentMatrix;
#if defined(GLHW_ATI) || defined(GLHW_ATI_DX10) || defined(GLDRV_MESA)
	worldToTangentMatrix = mat3(tangentToWorldMatrix[0][0], tangentToWorldMatrix[1][0], tangentToWorldMatrix[2][0],
								tangentToWorldMatrix[0][1], tangentToWorldMatrix[1][1], tangentToWorldMatrix[2][1], 
								tangentToWorldMatrix[0][2], tangentToWorldMatrix[1][2], tangentToWorldMatrix[2][2]);
#else
	worldToTangentMatrix = transpose(tangentToWorldMatrix);
#endif

	// calculate the screen texcoord in the 0.0 to 1.0 range
	vec2 texScreen = gl_FragCoord.st * r_FBufScale * r_NPOTScale;
	vec2 texNormal = var_TexNormal.st;

#if defined(r_ParallaxMapping)
	// compute view direction in tangent space
	vec3 V = worldToTangentMatrix * (I);
	V = normalize(V);
	
	// ray intersect in view direction
	
	// size and start position of search in texture space
	//vec2 S = V.xy * -u_DepthScale / V.z;
	vec2 S = V.xy * -0.03 / V.z;
		
	float depth = RayIntersectDisplaceMap(texNormal, S);
	
	// compute texcoords offset
	vec2 texOffset = S * depth;
	
	texScreen.st += texOffset;
	texNormal.st += texOffset;
#endif

	// compute normals
	
	vec3 N = normalize(var_Normal);

	vec3 N2 = 2.0 * (texture2D(u_NormalMap, texNormal).xyz - 0.5);
	N2 = tangentToWorldMatrix * N2;
			
	// compute fresnel term
	float fresnel = clamp(u_FresnelBias + pow(1.0 - dot(I, N), u_FresnelPower) * 
			u_FresnelScale, 0.0, 1.0);

	texScreen += u_NormalScale * N2.xy;

	vec3 refractColor = texture2D(u_CurrentMap, texScreen).rgb;
	vec3 reflectColor = texture2D(u_PortalMap, texScreen).rgb;
	
	vec4 color;

	color.rgb = mix(refractColor, reflectColor, fresnel);
	color.a = 1.0;

	if(u_FogDensity > 0.0)
	{
		// reconstruct vertex position in world space
		float depth = texture2D(u_DepthMap, texScreen).r;
		vec4 P = u_UnprojectMatrix * vec4(gl_FragCoord.xy, depth, 1.0);
		P.xyz /= P.w;

		// calculate fog distance
		float fogDistance = distance(P.xyz, var_Position);
	
		// calculate fog exponent
		float fogExponent = fogDistance * u_FogDensity;
	
		// calculate fog factor
		float fogFactor = exp2(-abs(fogExponent));
		
		color.rgb = mix(u_FogColor, color.rgb, fogFactor);
	}
	
	vec3 L = normalize(var_LightDirection);
	
	// compute half angle in world space
	vec3 H = normalize(L + I);
	
	// compute the light term
	vec3 light = var_LightColor.rgb * clamp(dot(N2, L), 0.0, 1.0);
	
	// compute the specular term
	vec3 specular = reflectColor * var_LightColor.rgb * pow(clamp(dot(N2, H), 0.0, 1.0), r_SpecularExponent) * r_SpecularScale;
	color.rgb += specular;
	
	gl_FragColor = color;
}
