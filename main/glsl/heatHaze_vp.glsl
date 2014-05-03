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

/* heatHaze_vp.glsl */

attribute vec3 		attr_Position;
attribute vec4		attr_QTangent;
attribute vec2 		attr_TexCoord0;

attribute vec3 		attr_Position2;
attribute vec4		attr_QTangent2;

uniform float		u_VertexInterpolation;

uniform float		u_Time;

uniform mat4		u_NormalTextureMatrix;
uniform mat4		u_ProjectionMatrixTranspose;
uniform mat4		u_ModelViewMatrixTranspose;
uniform mat4		u_ModelViewProjectionMatrix;

uniform float		u_DeformMagnitude;

varying vec2		var_TexNormal;
varying float		var_Deform;

vec3 QuatTransVec(in vec4 quat, in vec3 vec) {
	vec3 tmp = 2.0 * cross( quat.xyz, vec );
	return vec + quat.w * tmp + cross( quat.xyz, tmp );
}

void	main()
{
	vec4            deformVec;
	float           d1, d2;

	vec4 position;
	vec3 normal;

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

#if defined(USE_DEFORM_VERTEXES)
	position = DeformPosition2(	position,
					normal,
					attr_TexCoord0.st / 4096.0,
					u_Time);
#endif

	// transform vertex position into homogenous clip-space
	gl_Position = u_ModelViewProjectionMatrix * position;

	// take the deform magnitude and scale it by the projection distance
	deformVec = vec4(1, 0, 0, 1);
	deformVec.z = dot(u_ModelViewMatrixTranspose[2], position);

	// transform normalmap texcoords
	var_TexNormal = (u_NormalTextureMatrix * vec4(attr_TexCoord0 / 4096.0, 0.0, 1.0)).st;

	d1 = dot(u_ProjectionMatrixTranspose[0],  deformVec);
    d2 = dot(u_ProjectionMatrixTranspose[3],  deformVec);

	// clamp the distance, so the deformations don't get too wacky near the view
	var_Deform = min(d1 * (1.0 / max(d2, 1.0)), 0.02) * u_DeformMagnitude;
}
