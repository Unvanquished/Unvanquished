/*
===========================================================================
Copyright (C) 2008-2011 Robert Beckebans <trebor_7@users.sourceforge.net>

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

/* vertexLighting_DBS_world_vp.glsl */

attribute vec4		attr_Position;
attribute vec4		attr_TexCoord0;
attribute vec3		attr_Tangent;
attribute vec3		attr_Binormal;
attribute vec3		attr_Normal;
attribute vec4		attr_Color;
attribute vec3		attr_AmbientLight;
attribute vec3		attr_DirectedLight;
attribute vec3		attr_LightDirection;

uniform mat4		u_DiffuseTextureMatrix;
uniform mat4		u_NormalTextureMatrix;
uniform mat4		u_SpecularTextureMatrix;
uniform mat4		u_ModelViewProjectionMatrix;

uniform float		u_Time;

uniform vec4		u_ColorModulate;
uniform vec4		u_Color;

varying vec3		var_Position;
varying vec4		var_TexDiffuseNormal;
varying vec2		var_TexSpecular;
#if defined(USE_NORMAL_MAPPING)
//varying vec3		var_AmbientLight;
varying vec3		var_DirectedLight;
varying vec3		var_LightDirection;
#else
varying vec4		var_LightColor;
#endif
varying vec3		var_Tangent;
varying vec3		var_Binormal;
varying vec3		var_Normal;




void	main()
{
	vec4 position = attr_Position;
#if defined(USE_DEFORM_VERTEXES)
	position = DeformPosition2(	position,
								attr_Normal,
								attr_TexCoord0.st,
								u_Time);
#endif

	// transform vertex position into homogenous clip-space
	gl_Position = u_ModelViewProjectionMatrix * position;

	// assign position in object space
	var_Position = position.xyz;

	// transform diffusemap texcoords
	var_TexDiffuseNormal.st = (u_DiffuseTextureMatrix * attr_TexCoord0).st;

#if defined(USE_NORMAL_MAPPING)
	// transform normalmap texcoords
	var_TexDiffuseNormal.pq = (u_NormalTextureMatrix * attr_TexCoord0).st;

	// transform specularmap texture coords
	var_TexSpecular = (u_SpecularTextureMatrix * attr_TexCoord0).st;

	// assign vertex to light vector in object space
//	var_AmbientLight = attr_AmbientLight;
	// assign vertex to light vector in object space
	var_DirectedLight = attr_DirectedLight;
	// assign vertex to light vector in object space
	var_LightDirection = attr_LightDirection;
#else
	// assign color
	var_LightColor = attr_Color * u_ColorModulate + u_Color;
#endif

#if defined(USE_NORMAL_MAPPING)
	var_Tangent = attr_Tangent;
	var_Binormal = attr_Binormal;
#endif

	var_Normal = attr_Normal;
}
