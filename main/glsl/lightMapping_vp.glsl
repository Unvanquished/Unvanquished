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

/* lightMapping_vp.glsl */

attribute vec3 		attr_Position;
attribute vec4 		attr_TexCoord0;
attribute vec4		attr_QTangent;
attribute vec4		attr_Color;

uniform mat4		u_DiffuseTextureMatrix;
uniform mat4		u_NormalTextureMatrix;
uniform mat4		u_SpecularTextureMatrix;
uniform mat4		u_GlowTextureMatrix;
uniform mat4		u_ModelMatrix;
uniform mat4		u_ModelViewProjectionMatrix;

uniform float		u_Time;

uniform vec4		u_ColorModulate;
uniform vec4		u_Color;

varying vec3		var_Position;
varying vec4		var_TexDiffuseGlow;
varying vec4		var_TexNormalSpecular;
varying vec2		var_TexLight;
varying vec3		var_Tangent;
varying vec3		var_Binormal;
varying vec3		var_Normal;
varying vec4		var_Color;


vec3 QuatTransVec(in vec4 quat, in vec3 vec) {
	vec3 tmp = 2.0 * cross( quat.xyz, vec );
	return vec + quat.w * tmp + cross( quat.xyz, tmp );
}

void	main()
{
	vec4 position = vec4(attr_Position, 1.0);

	var_Normal = QuatTransVec( attr_QTangent, vec3( 0.0, 0.0, 1.0 ) );

#if defined(USE_DEFORM_VERTEXES)
	position = DeformPosition2(	position,
					var_Normal,
					attr_TexCoord0.st / 4096.0,
					u_Time);
#endif

	// transform vertex position into homogenous clip-space
	gl_Position = u_ModelViewProjectionMatrix * position;

	// transform diffusemap texcoords
	var_TexDiffuseGlow.st = (u_DiffuseTextureMatrix * vec4(attr_TexCoord0.xy / 4096.0, 0.0, 1.0)).st;
	var_TexLight = attr_TexCoord0.zw / 4096.0;

#if defined(USE_NORMAL_MAPPING)
	// transform normalmap texcoords
	var_TexNormalSpecular.st = (u_NormalTextureMatrix * vec4(attr_TexCoord0.xy / 4096.0, 0.0, 1.0)).st;

	// transform specularmap texcoords
	var_TexNormalSpecular.pq = (u_SpecularTextureMatrix * vec4(attr_TexCoord0.xy / 4096.0, 0.0, 1.0)).st;
#endif

#if defined(USE_GLOW_MAPPING)
	var_TexDiffuseGlow.pq = (u_GlowTextureMatrix * vec4(attr_TexCoord0.xy / 4096.0, 0.0, 1.0)).st;
#endif

#if 0

	// transform position into world space
	var_Position = (u_ModelMatrix * position).xyz;

	var_Normal.xyz = (u_ModelMatrix * vec4(attr_Normal, 0.0)).xyz;

#if defined(USE_NORMAL_MAPPING)
	var_Tangent.xyz = (u_ModelMatrix * vec4(attr_Tangent, 0.0)).xyz;
	var_Binormal.xyz = (u_ModelMatrix * vec4(attr_Binormal, 0.0)).xyz;
#endif

#else

	var_Position = position.xyz;

#if defined(USE_NORMAL_MAPPING)
	var_Tangent = QuatTransVec( attr_QTangent, vec3( 1.0, 0.0, 0.0 ) );
	var_Binormal = QuatTransVec( attr_QTangent, vec3( 0.0, 1.0, 0.0 ) );
	var_Tangent *= sign( attr_QTangent.w );
#endif

#endif

	var_Color = attr_Color * u_ColorModulate + u_Color;
}
