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
uniform int		u_VertexSkinning;
// even elements are rotation quat, odd elements are translation + scale (in .w)
uniform vec4		u_Bones[ 2 * MAX_GLSL_BONES ];

vec3 QuatTransVec(in vec4 quat, in vec3 vec) {
	vec3 tmp = 2.0 * cross( quat.xyz, vec );
	return vec + quat.w * tmp + cross( quat.xyz, tmp );
}

void VertexSkinning_P_N(const vec3 inPosition,
			const vec3 inNormal,

			out vec4 position,
			out vec3 normal)
{
	ivec4 idx = 2 * ivec4( attr_BoneIndexes );

	vec4 quat = u_Bones[ idx.x ];
	vec4 trans = u_Bones[ idx.x + 1 ];

	position.xyz = attr_BoneWeights.x * (QuatTransVec( quat, inPosition ) * trans.w + trans.xyz);
	normal = attr_BoneWeights.x * (QuatTransVec( quat, inNormal ));

	quat = u_Bones[ idx.y ];
	trans = u_Bones[ idx.y + 1 ];

	position.xyz += attr_BoneWeights.y * (QuatTransVec( quat, inPosition ) * trans.w + trans.xyz);
	normal += attr_BoneWeights.y * (QuatTransVec( quat, inNormal ));

	quat = u_Bones[ idx.z ];
	trans = u_Bones[ idx.z + 1 ];

	position.xyz += attr_BoneWeights.z * (QuatTransVec( quat, inPosition ) * trans.w + trans.xyz);
	normal += attr_BoneWeights.z * (QuatTransVec( quat, inNormal ));

	quat = u_Bones[ idx.w ];
	trans = u_Bones[ idx.w + 1 ];

	position.xyz += attr_BoneWeights.w * (QuatTransVec( quat, inPosition ) * trans.w + trans.xyz);
	normal += attr_BoneWeights.w * (QuatTransVec( quat, inNormal ));

	position.w = 1.0;
	normal = normalize(normal);
}

void VertexSkinning_P_TBN(	const vec3 inPosition,
				const vec3 inTangent,
				const vec3 inBinormal,
				const vec3 inNormal,

				out vec4 position,
				out vec3 tangent,
				out vec3 binormal,
				out vec3 normal)
{
	ivec4 idx = 2 * ivec4( attr_BoneIndexes );

	vec4 quat = u_Bones[ idx.x ];
	vec4 trans = u_Bones[ idx.x + 1 ];

	position.xyz = attr_BoneWeights.x * (QuatTransVec( quat, inPosition ) * trans.w + trans.xyz);
	normal = attr_BoneWeights.x * (QuatTransVec( quat, inNormal ));
	tangent = attr_BoneWeights.x * (QuatTransVec( quat, inTangent ));
	binormal = attr_BoneWeights.x * (QuatTransVec( quat, inBinormal ));
	
	quat = u_Bones[ idx.y ];
	trans = u_Bones[ idx.y + 1 ];

	position.xyz += attr_BoneWeights.y * (QuatTransVec( quat, inPosition ) * trans.w + trans.xyz);
	normal += attr_BoneWeights.y * (QuatTransVec( quat, inNormal ));
	tangent += attr_BoneWeights.y * (QuatTransVec( quat, inTangent ));
	binormal += attr_BoneWeights.y * (QuatTransVec( quat, inBinormal ));

	quat = u_Bones[ idx.z ];
	trans = u_Bones[ idx.z + 1 ];

	position.xyz += attr_BoneWeights.z * (QuatTransVec( quat, inPosition ) * trans.w + trans.xyz);
	normal += attr_BoneWeights.z * (QuatTransVec( quat, inNormal ));
	tangent += attr_BoneWeights.z * (QuatTransVec( quat, inTangent ));
	binormal += attr_BoneWeights.z * (QuatTransVec( quat, inBinormal ));

	quat = u_Bones[ idx.w ];
	trans = u_Bones[ idx.w + 1 ];

	position.xyz += attr_BoneWeights.w * (QuatTransVec( quat, inPosition ) * trans.w + trans.xyz);
	normal += attr_BoneWeights.w * (QuatTransVec( quat, inNormal ));
	tangent += attr_BoneWeights.w * (QuatTransVec( quat, inTangent ));
	binormal += attr_BoneWeights.w * (QuatTransVec( quat, inBinormal ));

	position.w = 1.0;
	normal = normalize(normal);
	tangent = normalize(tangent);
	binormal = normalize(binormal);
}
