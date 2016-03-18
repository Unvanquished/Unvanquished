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

/* forwardLighting_vp.glsl */

uniform mat4		u_DiffuseTextureMatrix;
uniform mat4		u_NormalTextureMatrix;
uniform mat4		u_SpecularTextureMatrix;

uniform vec4		u_ColorModulate;
uniform vec4		u_Color;

uniform mat4		u_LightAttenuationMatrix;
uniform mat4		u_ModelMatrix;
uniform mat4		u_ModelViewProjectionMatrix;

uniform float		u_Time;

varying vec3		var_Position;
varying vec4		var_TexDiffuse;
varying vec4		var_TexNormal;
varying vec2		var_TexSpecular;

varying vec4		var_TexAttenuation;

varying vec4		var_Tangent;
varying vec4		var_Binormal;
varying vec4		var_Normal;
//varying vec4		var_Color;	// Tr3B - maximum vars reached

void DeformVertex( inout vec4 pos,
		   inout vec3 normal,
		   inout vec2 st,
		   inout vec4 color,
		   in    float time);

void	main()
{
	vec4 position;
	localBasis LB;
	vec2 texCoord, lmCoord;
	vec4 color;

	VertexFetch( position, LB, color, texCoord, lmCoord);

	// assign color
        color = color * u_ColorModulate + u_Color;

	DeformVertex( position,
		      LB.normal,
		      texCoord.st,
		      color,
		      u_Time);

	// transform vertex position into homogenous clip-space
	gl_Position = u_ModelViewProjectionMatrix * position;

	// transform position into world space
	var_Position = (u_ModelMatrix * position).xyz;

	var_Tangent.xyz = mat3(u_ModelMatrix) * LB.tangent;
	var_Binormal.xyz = mat3(u_ModelMatrix) * LB.binormal;
	var_Normal.xyz = mat3(u_ModelMatrix) * LB.normal;

	// calc light xy,z attenuation in light space
	var_TexAttenuation = u_LightAttenuationMatrix * position;

	// transform diffusemap texcoords
	var_TexDiffuse.xy = (u_DiffuseTextureMatrix * vec4(texCoord, 0.0, 1.0)).st;

	// transform normalmap texcoords
	var_TexNormal.xy = (u_NormalTextureMatrix * vec4(texCoord, 0.0, 1.0)).st;

	// transform specularmap texture coords
	var_TexSpecular = (u_SpecularTextureMatrix * vec4(texCoord, 0.0, 1.0)).st;

	var_TexDiffuse.p = color.r;
	var_TexNormal.pq = color.gb;
}
