/*
===========================================================================
Copyright (C) 2008 Robert Beckebans <trebor_7@users.sourceforge.net>

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

/* deferredShadowing_proj_fp.glsl */

uniform sampler2D	u_DepthMap;
uniform sampler2D	u_AttenuationMapXY;
uniform sampler2D	u_AttenuationMapZ;
uniform sampler2D	u_ShadowMap;
uniform vec3		u_ViewOrigin;
uniform vec3		u_LightOrigin;
uniform vec3		u_LightColor;
uniform float		u_LightRadius;
uniform mat4		u_LightAttenuationMatrix;
uniform mat4		u_ShadowMatrix[MAX_SHADOWMAPS];
uniform int			u_ShadowCompare;
uniform int         u_PortalClipping;
uniform vec4		u_PortalPlane;
uniform mat4		u_UnprojectMatrix;

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
	
	if(bool(u_PortalClipping))
	{
		float dist = dot(P.xyz, u_PortalPlane.xyz) - u_PortalPlane.w;
		if(dist < 0.0)
		{
			discard;
			return;
		}
	}
	
	// transform vertex position into light space
	vec4 texAtten			= u_LightAttenuationMatrix * vec4(P.xyz, 1.0);
	if(texAtten.q <= 0.0)
	{
		// point is behind the near clip plane
		discard;
		return;
	}

	float shadow = 1.0;
	
#if defined(VSM)
	if(bool(u_ShadowCompare))
	{
		// compute incident ray
		vec3 I = P.xyz - u_LightOrigin;
		
		const float	SHADOW_BIAS = 0.001;
		float vertexDistance = length(I) / u_LightRadius - SHADOW_BIAS;
		
		// no filter
		vec4 texShadow = u_ShadowMatrix[0] * vec4(P.xyz, 1.0);
		vec4 shadowMoments = texture2DProj(u_ShadowMap, texShadow.xyw);
		//vec4 shadowMoments = texture2DProj(u_ShadowMap, SP.xyw);
	
		#if defined(VSM_CLAMP)
		// convert to [-1, 1] vector space
		shadowMoments = 2.0 * (shadowMoments - 0.5);
		#endif
		
		float shadowDistance = shadowMoments.r;
		float shadowDistanceSquared = shadowMoments.a;
	
		// standard shadow map comparison
		shadow = vertexDistance <= shadowDistance ? 1.0 : 0.0;
	
		// variance shadow mapping
		float E_x2 = shadowDistanceSquared;
		float Ex_2 = shadowDistance * shadowDistance;
	
		// AndyTX: VSM_EPSILON is there to avoid some ugly numeric instability with fp16
		float variance = min(max(E_x2 - Ex_2, 0.0) + VSM_EPSILON, 1.0);
		//float variance = smoothstep(VSM_EPSILON, 1.0, max(E_x2 - Ex_2, 0.0));
	
		float mD = shadowDistance - vertexDistance;
		float mD_2 = mD * mD;
		float p = variance / (variance + mD_2);
		p = smoothstep(0.0, 1.0, p);
	
		#if defined(DEBUG_VSM)
		#extension GL_EXT_gpu_shader4 : enable
		gl_FragColor.r = (DEBUG_VSM & 1) != 0 ? variance : 0.0;
		gl_FragColor.g = (DEBUG_VSM & 2) != 0 ? mD_2 : 0.0;
		gl_FragColor.b = (DEBUG_VSM & 4) != 0 ? p : 0.0;
		gl_FragColor.a = 1.0;
		return;
		#else
		shadow = max(shadow, p);
		#endif
		
		shadow = clamp(shadow, 0.0, 1.0);
		shadow = 1.0 - shadow;
	}
	
	if(shadow <= 0.0)
	{
		discard;
	}
	else
#elif defined(ESM)
	if(bool(u_ShadowCompare))
	{
		// compute incident ray
		vec3 I = P.xyz - u_LightOrigin;
		
		// no filter
		vec4 texShadow = u_ShadowMatrix[0] * vec4(P.xyz, 1.0);
		vec4 shadowMoments = texture2DProj(u_ShadowMap, texShadow.xyw);
		
		const float	SHADOW_BIAS = 0.001;
		float vertexDistance = (length(I) / u_LightRadius) * r_ShadowMapDepthScale; // - SHADOW_BIAS;
		
		float shadowDistance = shadowMoments.a;
		
		// exponential shadow mapping
		//shadow = vertexDistance <= shadowDistance ? 1.0 : 0.0;
		shadow = clamp(exp(r_OverDarkeningFactor * (shadowDistance - vertexDistance)), 0.0, 1.0);
		//shadow = smoothstep(0.0, 1.0, shadow);
		
		shadow = clamp(shadow, 0.0, 1.0);
		shadow = 1.0 - shadow;
		
		#if defined(DEBUG_ESM)
		#extension GL_EXT_gpu_shader4 : enable
		gl_FragColor.r = (DEBUG_ESM & 1) != 0 ? shadowDistance : 0.0;
		gl_FragColor.g = (DEBUG_ESM & 2) != 0 ? -(shadowDistance - vertexDistance) : 0.0;
		gl_FragColor.b = (DEBUG_ESM & 4) != 0 ? shadow : 0.0;
		gl_FragColor.a = 1.0;
		return;
		#endif
	}
	
	if(shadow <= 0.0)
	{
		discard;
	}
	else
#endif
	{
		// compute attenuation
		vec3 attenuationXY = texture2DProj(u_AttenuationMapXY, texAtten.xyw).rgb;
		vec3 attenuationZ  = texture2D(u_AttenuationMapZ, vec2(clamp(texAtten.z, 0.0, 1.0), 0.0)).rgb;
	
		// compute final color
		vec4 color = vec4(1.0, 1.0, 1.0, 1.0);
		color.rgb *= u_LightColor;
		//color.rgb *= attenuationXY;
		//color.rgb *= attenuationZ;
		color.rgb *= shadow;
		color.rgb *= 1.0 - clamp(distance(P.xyz, u_LightOrigin) / (u_LightRadius * 0.7), 0.0, 1.0);
		
		gl_FragColor = color;
	}
}
