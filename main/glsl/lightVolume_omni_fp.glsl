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

/* lightVolume_omni_fp.glsl */

uniform sampler2D	u_DepthMap;
uniform sampler2D	u_AttenuationMapXY;
uniform sampler2D	u_AttenuationMapZ;
uniform samplerCube	u_ShadowMap;
uniform vec3		u_ViewOrigin;
uniform vec3		u_LightOrigin;
uniform vec3		u_LightColor;
uniform float		u_LightRadius;
uniform float       u_LightScale;
uniform mat4		u_LightAttenuationMatrix;
uniform int			u_ShadowCompare;
uniform mat4		u_UnprojectMatrix;

varying vec2		var_TexDiffuse;
varying vec3		var_TexAttenXYZ;

void	main()
{
	// calculate the screen texcoord in the 0.0 to 1.0 range
	vec2 st = gl_FragCoord.st * r_FBufScale;
	
	// scale by the screen non-power-of-two-adjust
	st *= r_NPOTScale;
		
	// reconstruct vertex position in world space
	float depth = texture2D(u_DepthMap, st).r;
	vec4 P = u_UnprojectMatrix * vec4(gl_FragCoord.xy, depth, 1.0);
	P.xyz /= P.w;
	
#if 0
	if(bool(u_PortalClipping))
	{
		float dist = dot(P.xyz, u_PortalPlane.xyz) - u_PortalPlane.w;
		if(dist < 0.0)
		{
			discard;
			return;
		}
	}
#endif
	
	// compute incident ray in world space
	vec3 R = normalize(P.xyz - u_ViewOrigin);
	//vec3 R = normalize(u_ViewOrigin - P.xyz);
	
	//float traceDistance = dot(P.xyz - (u_ViewOrigin.xyz + R * u_ZNear ), forward);
	//traceDistance = clamp(traceDistance, 0.0, 2500.0 ); // Far trace distance
	
	float traceDistance = distance(P.xyz, u_ViewOrigin);
	traceDistance = clamp(traceDistance, 0.0, 2500.0);
	
	// TODO move to front clipping plane
	
	vec4 color = vec4(0.0, 0.0, 0.0, 1.0);
	
	//int steps = 40;
	//float stepSize = traceDistance / float(steps);
	
	int steps = int(min(traceDistance, 2000.0));	// TODO r_MaxSteps
	float stepSize = 64.0;
	
	for(float tracedDistance = 0.0; tracedDistance < traceDistance; tracedDistance += stepSize)
	{
		//vec3 T = P.xyz + (R * stepSize * float(i));
		//vec3 T = u_ViewOrigin + (R * stepSize * float(i));
		vec3 T = u_ViewOrigin + (R * tracedDistance);
	
		// compute attenuation
		vec3 texAttenXYZ		= (u_LightAttenuationMatrix * vec4(T, 1.0)).xyz;
		vec3 attenuationXY		= texture2D(u_AttenuationMapXY, texAttenXYZ.xy).rgb;
		vec3 attenuationZ		= texture2D(u_AttenuationMapZ, vec2(texAttenXYZ.z, 0)).rgb;
		
		float shadow = 1.0;

		#if defined(VSM)
		if(bool(u_ShadowCompare))
		{
			// compute incident ray
			vec3 I2 = T - u_LightOrigin;
			
			vec4 shadowMoments = textureCube(u_ShadowMap, I2);
			
			#if defined(VSM_CLAMP)
			// convert to [-1, 1] vector space
			shadowMoments = 0.5 * (shadowMoments + 1.0);
			#endif
	
			float shadowDistance = shadowMoments.r;
			float shadowDistanceSquared = shadowMoments.g;
		
			const float	SHADOW_BIAS = 0.001;
			float vertexDistance = length(I2) / u_LightRadius - SHADOW_BIAS;
	
			// standard shadow map comparison
			shadow = vertexDistance <= shadowDistance ? 1.0 : 0.0;
	
			// variance shadow mapping
			float E_x2 = shadowDistanceSquared;
			float Ex_2 = shadowDistance * shadowDistance;
	
			// AndyTX: VSM_EPSILON is there to avoid some ugly numeric instability with fp16
			float variance = min(max(E_x2 - Ex_2, 0.0) + VSM_EPSILON, 1.0);
	
			float mD = shadowDistance - vertexDistance;
			float mD_2 = mD * mD;
			float p = variance / (variance + mD_2);
	
			color.rgb += attenuationXY * attenuationZ * max(shadow, p);
		}
		
		if(shadow <= 0.0)
		{
			continue;
		}
		else
		#endif
		{
			color.rgb += attenuationXY * attenuationZ;
		}
	}
	
	color.rgb /= float(steps);
	color.rgb *= u_LightColor;
	//color.rgb *= u_LightScale;

	gl_FragColor = color;
}
