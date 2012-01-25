/*
===========================================================================
Copyright (C) 2006-2008 Robert Beckebans <trebor_7@users.sourceforge.net>

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

/* refraction_C_vp.glsl */

attribute vec4		attr_Position;
attribute vec3		attr_Normal;
#if defined(r_VertexSkinning)
attribute vec4		attr_BoneIndexes;
attribute vec4		attr_BoneWeights;
uniform int			u_VertexSkinning;
uniform mat4		u_BoneMatrix[MAX_GLSL_BONES];
#endif

uniform mat4		u_ModelMatrix;
uniform mat4		u_ModelViewProjectionMatrix;

varying vec3		var_Position;
varying vec3		var_Normal;

void	main()
{
#if defined(r_VertexSkinning)
	if(bool(u_VertexSkinning))
	{
		vec4 vertex = vec4(0.0);
		vec3 normal = vec3(0.0);

		for(int i = 0; i < 4; i++)
		{
			int boneIndex = int(attr_BoneIndexes[i]);
			float boneWeight = attr_BoneWeights[i];
			mat4  boneMatrix = u_BoneMatrix[boneIndex];
			
			vertex += (boneMatrix * attr_Position) * boneWeight;
			normal += (boneMatrix * vec4(attr_Normal, 0.0)).xyz * boneWeight;
		}

		// transform vertex position into homogenous clip-space
		gl_Position = u_ModelViewProjectionMatrix * vertex;
		
		// transform position into world space
		var_Position = (u_ModelMatrix * vertex).xyz;
	
		// transform normal into world space
		var_Normal = (u_ModelMatrix * vec4(normal, 0.0)).xyz;
	}
	else
#endif
	{
		// transform vertex position into homogenous clip-space
		gl_Position = u_ModelViewProjectionMatrix * attr_Position;
	
		// transform position into world space
		var_Position = (u_ModelMatrix * attr_Position).xyz;
	
		// transform normal into world space
		var_Normal = (u_ModelMatrix * vec4(attr_Normal, 0.0)).xyz;
	}
}

