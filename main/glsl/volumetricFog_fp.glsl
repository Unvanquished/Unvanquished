/*
===========================================================================
Copyright (C) 2009 Robert Beckebans <trebor_7@users.sourceforge.net>

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

/* volumetricFog_fp.glsl */

uniform sampler2D	u_DepthMap;			// raw depth in red channel
uniform sampler2D	u_DepthMapBack;		// color encoded depth
uniform sampler2D	u_DepthMapFront;	// color encoded depth
uniform vec3		u_ViewOrigin;
uniform float		u_FogDensity;
uniform vec3		u_FogColor;
uniform mat4		u_UnprojectMatrix;


float DecodeDepth(vec4 color)
{
	float depth;
	
	const vec4 bitShifts = vec4(1.0 / (256.0 * 256.0 * 256.0), 1.0 / (256.0 * 256.0), 1.0 / 256.0, 1.0); 	// gl_FragCoord.z with 32 bit precision
//	const vec4 bitShifts = vec4(1.0 / (256.0 * 256.0), 1.0 / 256.0, 1.0, 0.0); 								// gl_FragCoord.z with 24 bit precision
	
	depth = dot(color, bitShifts);
	
	return depth;
}

void	main()
{
	// calculate the screen texcoord in the 0.0 to 1.0 range
	vec2 st = gl_FragCoord.st * r_FBufScale;
	
	// scale by the screen non-power-of-two-adjust
	st *= r_NPOTScale;
	
	// calculate fog volume depth
	float fogDepth;
	
	float depthSolid = texture2D(u_DepthMap, st).r;
	
//#if 0 //defined(GLHW_ATI_DX10) || defined(GLHW_NV_DX10)
//
//	float depthBack = texture2D(u_DepthMapBack, st).a;
//	float depthFront = texture2D(u_DepthMapFront, st).a;
//#else
	float depthBack = DecodeDepth(texture2D(u_DepthMapBack, st));
	float depthFront = DecodeDepth(texture2D(u_DepthMapFront, st));
//#endif

	if(depthSolid < depthFront)
	{
		discard;
		return;
	}
	
	if(depthSolid < depthBack)
	{
		depthBack = depthSolid;
	}
	
	fogDepth = depthSolid;
	
//#if 1
	// reconstruct vertex position in world space
	vec4 posBack = u_UnprojectMatrix * vec4(gl_FragCoord.xy, depthBack, 1.0);
	posBack.xyz /= posBack.w;
	
	vec4 posFront = u_UnprojectMatrix * vec4(gl_FragCoord.xy, depthFront, 1.0);
	posFront.xyz /= posFront.w;
	
	if(posFront.w <= 0.0)
	{
		// we might be in the volume
		posFront = vec4(u_ViewOrigin, 1.0);
	}

	// calculate fog distance
	//if(depthFront < 
	float fogDistance = distance(posBack, posFront);
	//float fogDistance = abs(depthBack - depthFront);
	
/*
#elif 0
	vec4 P = u_UnprojectMatrix * vec4(gl_FragCoord.xy, depthBack - depthFront, 1.0);
	P.xyz /= P.w;

	// calculate fog distance
	float fogDistance = distance(P.xyz, u_ViewOrigin);
	
#else
	float fogDistance = depthBack - depthFront;
#endif
*/
	
	// calculate fog exponent
	float fogExponent = fogDistance * u_FogDensity;
	
	// calculate fog factor
	float fogFactor = exp2(-abs(fogExponent));

	// lerp between FBO color and fog color with GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA. GLS_DSTBLEND_SRC_ALPHA
	gl_FragColor = vec4(u_FogColor, fogFactor);
}
