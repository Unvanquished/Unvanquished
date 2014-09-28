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

/* reflection_CB_vp.glsl */

attribute vec3 		attr_Position;
attribute vec2 		attr_TexCoord0;
attribute vec4		attr_QTangent;

attribute vec3 		attr_Position2;
attribute vec4		attr_QTangent2;

uniform float		u_VertexInterpolation;
uniform mat4		u_NormalTextureMatrix;
uniform mat4		u_ModelMatrix;
uniform mat4		u_ModelViewProjectionMatrix;

uniform float		u_Time;

varying vec3		var_Position;
varying vec2		var_TexNormal;
varying vec4		var_Tangent;
varying vec4		var_Binormal;
varying vec4		var_Normal;

vec3 QuatTransVec(in vec4 quat, in vec3 vec) {
	vec3 tmp = 2.0 * cross( quat.xyz, vec );
	return vec + quat.w * tmp + cross( quat.xyz, tmp );
}

void	main()
{
	vec4 position;
	vec3 tangent;
	vec3 binormal;
	vec3 normal;
	vec2 texCoord;

#if defined(USE_VERTEX_SKINNING)

	#if defined(USE_NORMAL_MAPPING)
	VertexSkinning_P_TBN(	attr_Position, attr_QTangent,
				position, tangent, binormal, normal);
	#else
	VertexSkinning_P_N(	attr_Position, attr_QTangent,
				position, normal);
	#endif

#elif defined(USE_VERTEX_ANIMATION)

	#if defined(USE_NORMAL_MAPPING)
	VertexAnimation_P_TBN(	attr_Position, attr_Position2,
				attr_QTangent, attr_QTangent2,
				u_VertexInterpolation,
				position, tangent, binormal, normal);
	#else
	VertexAnimation_P_N(attr_Position, attr_Position2,
			    attr_QTangent, attr_QTangent2,
			    u_VertexInterpolation,
			    position, normal);
	#endif

#else
	position = vec4(attr_Position, 1.0);

	#if defined(USE_NORMAL_MAPPING)
	tangent = QuatTransVec( attr_QTangent, vec3( 1.0, 0.0, 0.0 ) );
	binormal = QuatTransVec( attr_QTangent, vec3( 0.0, 1.0, 0.0 ) );
	tangent *= sign( attr_QTangent.w );
	#endif

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
	var_Position = (u_ModelMatrix * position).xyz;

	#if defined(USE_NORMAL_MAPPING)
	var_Tangent.xyz = (u_ModelMatrix * vec4(tangent, 0.0)).xyz;
	var_Binormal.xyz = (u_ModelMatrix * vec4(binormal, 0.0)).xyz;
	#endif

	var_Normal.xyz = (u_ModelMatrix * vec4(normal, 0.0)).xyz;

#if defined(USE_NORMAL_MAPPING)
	// transform normalmap texcoords
	var_TexNormal = (u_NormalTextureMatrix * vec4(texCoord, 0.0, 1.0)).st;
#endif
}

