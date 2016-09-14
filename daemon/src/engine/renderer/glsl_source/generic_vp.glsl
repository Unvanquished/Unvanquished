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

/* generic_vp.glsl */

uniform mat4		u_ColorTextureMatrix;
#if !defined(USE_VERTEX_SPRITE)
uniform vec3		u_ViewOrigin;
#endif

uniform float		u_Time;

uniform vec4		u_ColorModulate;
uniform vec4		u_Color;
#if defined(USE_TCGEN_ENVIRONMENT)
uniform mat4		u_ModelMatrix;
#endif
uniform mat4		u_ModelViewProjectionMatrix;

#if defined(USE_VERTEX_SPRITE)
varying vec2            var_FadeDepth;
uniform mat4		u_ProjectionMatrixTranspose;
#elif defined(USE_DEPTH_FADE)
uniform float           u_DepthScale;
varying vec2            var_FadeDepth;
#endif

varying vec2		var_Tex;
varying vec4		var_Color;

void DeformVertex( inout vec4 pos,
		   inout vec3 normal,
		   inout vec2 st,
		   inout vec4 color,
		   in    float time);

void	main()
{
	vec4 position;
	localBasis LB;
	vec4 color;
	vec2 texCoord, lmCoord;

	VertexFetch( position, LB, color, texCoord, lmCoord );
	color = color * u_ColorModulate + u_Color;

	DeformVertex( position,
		      LB.normal,
		      texCoord,
		      color,
		      u_Time);

	// transform vertex position into homogenous clip-space
	gl_Position = u_ModelViewProjectionMatrix * position;

	// transform texcoords
#if defined(USE_TCGEN_ENVIRONMENT)
	{
		// TODO: Explain why only the rotational part of u_ModelMatrix is relevant
		position.xyz = mat3(u_ModelMatrix) * position.xyz;

		vec3 viewer = normalize(u_ViewOrigin - position.xyz);

		float d = dot(LB.normal, viewer);

		vec3 reflected = LB.normal * 2.0 * d - viewer;

		var_Tex = 0.5 + vec2(0.5, -0.5) * reflected.yz;
	}
#elif defined(USE_TCGEN_LIGHTMAP)
	var_Tex = (u_ColorTextureMatrix * vec4(lmCoord, 0.0, 1.0)).xy;
#else
	var_Tex = (u_ColorTextureMatrix * vec4(texCoord, 0.0, 1.0)).xy;
#endif

#if defined(USE_DEPTH_FADE) || defined(USE_VERTEX_SPRITE)
	// compute z of end of fading effect
	vec4 fadeDepth = u_ModelViewProjectionMatrix * (position - u_DepthScale * vec4(LB.normal, 0.0));
	var_FadeDepth = fadeDepth.zw;
#endif

	var_Color = color;
}
