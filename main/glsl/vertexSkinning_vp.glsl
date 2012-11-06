/*
===========================================================================
Copyright (C) 2009-2011 Robert Beckebans <trebor_7@users.sourceforge.net>

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
// vertexSkinning_vp.glsl - GPU vertex skinning for skeletal meshes

attribute vec4		attr_BoneIndexes;
attribute vec4		attr_BoneWeights;
uniform int			u_VertexSkinning;
uniform mat4		u_BoneMatrix[MAX_GLSL_BONES];

void VertexSkinning_P_N(const vec4 inPosition,
						const vec3 inNormal,

						inout vec4 position,
						inout vec3 normal)
{
	mat4 boneMatrix = u_BoneMatrix[ int( attr_BoneIndexes[ 0 ] ) ] * attr_BoneWeights[ 0 ];
	boneMatrix += u_BoneMatrix[ int( attr_BoneIndexes[ 1 ] ) ] * attr_BoneWeights[ 1 ];
	boneMatrix += u_BoneMatrix[ int( attr_BoneIndexes[ 2 ] ) ] * attr_BoneWeights[ 2 ];
	boneMatrix += u_BoneMatrix[ int( attr_BoneIndexes[ 3 ] ) ] * attr_BoneWeights[ 3 ];

	position.xyz = ( boneMatrix * inPosition ).xyz;
	position.w = 1.0;
	
	normal = mat3( boneMatrix ) * inNormal;
}

void VertexSkinning_P_TBN(	const vec4 inPosition,
							const vec3 inTangent,
							const vec3 inBinormal,
							const vec3 inNormal,

							inout vec4 position,
							inout vec3 tangent,
							inout vec3 binormal,
							inout vec3 normal)
{

	mat4 boneMatrix = u_BoneMatrix[ int( attr_BoneIndexes[ 0 ] ) ] * attr_BoneWeights[ 0 ];
	boneMatrix += u_BoneMatrix[ int( attr_BoneIndexes[ 1 ] ) ] * attr_BoneWeights[ 1 ];
	boneMatrix += u_BoneMatrix[ int( attr_BoneIndexes[ 2 ] ) ] * attr_BoneWeights[ 2 ];
	boneMatrix += u_BoneMatrix[ int( attr_BoneIndexes[ 3 ] ) ] * attr_BoneWeights[ 3 ];

	position.xyz = ( boneMatrix * inPosition ).xyz;
	position.w = 1.0;
	
	tangent = mat3( boneMatrix ) * inTangent;
	binormal = mat3( boneMatrix ) * inBinormal;
	normal = mat3( boneMatrix ) * inNormal;
}
