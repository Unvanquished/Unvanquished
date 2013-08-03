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
uniform mat3x4		u_BoneMatrix[MAX_GLSL_BONES];

void VertexSkinning_P_N(const vec3 inPosition,
						const vec3 inNormal,

						inout vec4 position,
						inout vec3 normal)
{
	mat3x4 boneMatrix = u_BoneMatrix[ int( attr_BoneIndexes.x ) ] * attr_BoneWeights.x;
	boneMatrix += u_BoneMatrix[ int( attr_BoneIndexes.y ) ] * attr_BoneWeights.y;
	boneMatrix += u_BoneMatrix[ int( attr_BoneIndexes.z ) ] * attr_BoneWeights.z;
	boneMatrix += u_BoneMatrix[ int( attr_BoneIndexes.w ) ] * attr_BoneWeights.w;

	position.xyz = ( vec4( inPosition, 1.0 ) * boneMatrix ).xyz;
	position.w = 1.0;
	
	normal = ( vec4( inNormal, 0.0 ) * boneMatrix ).xyz;
}

void VertexSkinning_P_TBN(	const vec3 inPosition,
							const vec3 inTangent,
							const vec3 inBinormal,
							const vec3 inNormal,

							inout vec4 position,
							inout vec3 tangent,
							inout vec3 binormal,
							inout vec3 normal)
{

	mat3x4 boneMatrix = u_BoneMatrix[ int( attr_BoneIndexes.x ) ] * attr_BoneWeights.x;
	boneMatrix += u_BoneMatrix[ int( attr_BoneIndexes.y ) ] * attr_BoneWeights.y;
	boneMatrix += u_BoneMatrix[ int( attr_BoneIndexes.z ) ] * attr_BoneWeights.z;
	boneMatrix += u_BoneMatrix[ int( attr_BoneIndexes.w ) ] * attr_BoneWeights.w;

	position.xyz = ( vec4( inPosition, 1.0 ) * boneMatrix ).xyz;
	position.w = 1.0;
	
	tangent = ( vec4( inTangent, 0.0 ) * boneMatrix ).xyz;
	binormal = ( vec4( inBinormal, 0.0 ) * boneMatrix ).xyz;
	normal = ( vec4( inNormal, 0.0 ) * boneMatrix ).xyz;
}
