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

attribute vec3 		attr_Position;
attribute vec2 		attr_TexCoord0;
attribute vec2 		attr_TexCoord1;
attribute vec3		attr_Normal;
attribute vec4		attr_Color;

attribute vec3 		attr_Position2;
attribute vec3		attr_Normal2;

uniform float		u_VertexInterpolation;

uniform mat4		u_ColorTextureMatrix;
uniform vec3		u_ViewOrigin;

uniform float		u_Time;

uniform vec4		u_ColorModulate;
uniform vec4		u_Color;
uniform mat4		u_ModelMatrix;
uniform mat4		u_ModelViewProjectionMatrix;

varying vec2		var_Tex;
varying vec4		var_Color;

void	main()
{
	vec4 position;
	vec3 normal;

#if defined(USE_VERTEX_SKINNING)

	VertexSkinning_P_N(	attr_Position, attr_Normal,
						position, normal);

#elif defined(USE_VERTEX_ANIMATION)

	VertexAnimation_P_N(attr_Position, attr_Position2,
						attr_Normal, attr_Normal2,
						u_VertexInterpolation,
						position, normal);

#else
	position = vec4(attr_Position, 1.0);
	normal = attr_Normal;
#endif

#if defined(USE_DEFORM_VERTEXES)
	position = DeformPosition2(	position,
								normal,
								attr_TexCoord0.st,
								u_Time);
#endif

	// transform vertex position into homogenous clip-space
	gl_Position = u_ModelViewProjectionMatrix * position;

	// transform texcoords
	vec4 texCoord;
#if defined(USE_TCGEN_ENVIRONMENT)
	{
		position = (mat3(u_ModelMatrix) * position.xyz).xyzx;
		vec3 viewer = normalize(u_ViewOrigin - position.xyz);

		float d = dot(normal, viewer);

		vec3 reflected = normal * 2.0 * d - viewer;

		texCoord.s = 0.5 + reflected.y * 0.5;
		texCoord.t = 0.5 - reflected.z * 0.5;
		texCoord.q = 0;
		texCoord.w = 1;
	}
#elif defined(USE_TCGEN_LIGHTMAP)
	texCoord = vec4(attr_TexCoord1, 0.0, 1.0);
#else
	texCoord = vec4(attr_TexCoord0, 0.0, 1.0);
#endif

	var_Tex = (u_ColorTextureMatrix * texCoord).st;

	var_Color = attr_Color * u_ColorModulate + u_Color;
}
