/*
===========================================================================
Copyright (C) 2007-2009 Robert Beckebans <trebor_7@users.sourceforge.net>

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

/* liquid_vp.glsl */

IN vec3 		attr_Position;
IN vec2 		attr_TexCoord0;
IN vec3			attr_Tangent;
IN vec3			attr_Binormal;
IN vec3			attr_Normal;

uniform mat4		u_NormalTextureMatrix;
uniform mat4		u_ModelMatrix;
uniform mat4		u_ModelViewProjectionMatrix;

OUT(smooth) vec3	var_Position;
OUT(smooth) vec2	var_TexNormal;
OUT(smooth) vec3	var_Tangent;
OUT(smooth) vec3	var_Binormal;
OUT(smooth) vec3	var_Normal;

void	main()
{
	// transform vertex position into homogenous clip-space
	gl_Position = u_ModelViewProjectionMatrix * vec4(attr_Position, 1.0);

	// transform position into world space
	var_Position = (u_ModelMatrix * vec4(attr_Position, 1.0)).xyz;

	// transform normalmap texcoords
	var_TexNormal = (u_NormalTextureMatrix * vec4(attr_TexCoord0, 0.0, 1.0)).st;

	var_Tangent.xyz = (u_ModelMatrix * vec4(attr_Tangent, 0.0)).xyz;
	var_Binormal.xyz = (u_ModelMatrix * vec4(attr_Binormal, 0.0)).xyz;

	// transform normal into world space
	var_Normal = (u_ModelMatrix * vec4(attr_Normal, 0.0)).xyz;
}

