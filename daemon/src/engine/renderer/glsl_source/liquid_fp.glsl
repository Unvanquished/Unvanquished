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
uniform vec2		u_SpecularExponent;

uniform sampler3D       u_LightGrid1;
uniform sampler3D       u_LightGrid2;
uniform vec3            u_LightGridOrigin;
uniform vec3            u_LightGridScale;

IN(smooth) vec3		var_Position;
IN(smooth) vec2		var_TexNormal;
IN(smooth) vec3		var_Tangent;
IN(smooth) vec3		var_Binormal;
IN(smooth) vec3		var_Normal;

DECLARE_OUTPUT(vec4)

#if defined(USE_PARALLAX_MAPPING)
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

void ReadLightGrid(in vec3 pos, out vec3 lgtDir,
		   out vec3 ambCol, out vec3 lgtCol ) {
	vec4 texel1 = texture3D(u_LightGrid1, pos);
	vec4 texel2 = texture3D(u_LightGrid2, pos);
	float ambLum, lgtLum;

	texel1.xyz = (texel1.xyz * 255.0 - 128.0) / 127.0;
	texel2.xyzw = texel2.xyzw - 0.5;

	lgtDir = normalize(texel1.xyz);

	lgtLum = 2.0 * length(texel1.xyz) * texel1.w;
	ambLum = 2.0 * texel1.w - lgtLum;

	// YCoCg decode chrominance
	ambCol.g = ambLum + texel2.x;
	ambLum   = ambLum - texel2.x;
	ambCol.r = ambLum + texel2.y;
	ambCol.b = ambLum - texel2.y;

	lgtCol.g = lgtLum + texel2.z;
	lgtLum   = lgtLum - texel2.z;
	lgtCol.r = lgtLum + texel2.w;
	lgtCol.b = lgtLum - texel2.w;
}


void	main()
{
	// compute incident ray
	vec3 I = normalize(u_ViewOrigin - var_Position);

	mat3 tangentToWorldMatrix = mat3(var_Tangent.xyz, var_Binormal.xyz, var_Normal.xyz);
	if(gl_FrontFacing)
	{
		tangentToWorldMatrix = -tangentToWorldMatrix;
	}

	// calculate the screen texcoord in the 0.0 to 1.0 range
	vec2 texScreen = gl_FragCoord.st * r_FBufScale;
	vec2 texNormal = var_TexNormal.st;

#if defined(USE_PARALLAX_MAPPING)
	// compute view direction in tangent space
	vec3 V = I * tangentToWorldMatrix;
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

	vec3 N2 = texture2D(u_NormalMap, texNormal.st).xyw);
	N2.x *= N2.z;
	N2.xy = 2.0 * N2.xy - 1.0;
	N2.z = sqrt(1.0 - dot(N2.xy, N2.xy));
	N2 = normalize(tangentToWorldMatrix * N2);

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

	vec3 L;
	vec3 ambCol;
	vec3 lgtCol;

	ReadLightGrid( (var_Position - u_LightGridOrigin) * u_LightGridScale,
		       L, ambCol, lgtCol );

	// compute half angle in world space
	vec3 H = normalize(L + I);

	// compute the light term
	vec3 light = lgtCol * clamp(dot(N2, L), 0.0, 1.0);

	// compute the specular term
	vec3 specular = reflectColor * lgtCol * pow(clamp(dot(N2, H), 0.0, 1.0), u_SpecularExponent.x + u_SpecularExponent.y) * r_SpecularScale;
	color.rgb += specular;

	outputColor = color;
}
