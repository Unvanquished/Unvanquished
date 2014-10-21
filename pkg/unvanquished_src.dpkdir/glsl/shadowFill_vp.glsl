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

/* shadowFill_vp.glsl */

attribute vec3 		attr_Position;
attribute vec4		attr_QTangent;
attribute vec2 		attr_TexCoord0;
attribute vec4		attr_Color;

attribute vec3 		attr_Position2;
attribute vec4		attr_QTangent2;

uniform float		u_VertexInterpolation;

uniform vec4		u_Color;

uniform mat4		u_ColorTextureMatrix;
uniform mat4		u_ModelMatrix;
uniform mat4		u_ModelViewProjectionMatrix;

uniform float		u_Time;

varying vec3		var_Position;
varying vec2		var_Tex;
varying vec4		var_Color;

vec3 QuatTransVec(in vec4 quat, in vec3 vec) {
	vec3 tmp = 2.0 * cross( quat.xyz, vec );
	return vec + quat.w * tmp + cross( quat.xyz, tmp );
}


void	main()
{
	vec4 position;
	vec3 normal;
	vec2 texCoord;

#if defined(USE_VERTEX_SKINNING)

	VertexSkinning_P_N(	attr_Position, attr_QTangent,
				position, normal);

#elif defined(USE_VERTEX_ANIMATION)

	VertexAnimation_P_N(attr_Position, attr_Position2,
			    attr_QTangent, attr_QTangent2,
			    u_VertexInterpolation,
			    position, normal);

#else
	position = vec4(attr_Position, 1.0);
	normal = QuatTransVec( attr_QTangent, vec3( 0.0, 0.0, 1.0 ) );
#endif

	texCoord = attr_TexCoord0;

	DeformVertex( position,
		      normal,
		      texCoord,
		      u_Time);

	// transform vertex position into homogenous clip-space
	gl_Position = u_ModelViewProjectionMatrix * position;
	// transform position into world space
#if defined(LIGHT_DIRECTIONAL)
	var_Position = gl_Position.xyz / gl_Position.w;
#else
		// transform position into world space
		var_Position = (u_ModelMatrix * position).xyz;
#endif

	// transform texcoords
	var_Tex = (u_ColorTextureMatrix * vec4(texCoord, 0.0, 1.0)).st;

	// assign color
	var_Color = u_Color;
}
