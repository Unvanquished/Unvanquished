/*
===========================================================================
Copyright (C) 2010 Robert Beckebans <trebor_7@users.sourceforge.net>

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
// vertexSimple_vp.glsl - simple vertex fetch

struct localBasis {
	vec3 normal;
	vec3 tangent, binormal;
};

vec3 QuatTransVec(in vec4 quat, in vec3 vec) {
	vec3 tmp = 2.0 * cross( quat.xyz, vec );
	return vec + quat.w * tmp + cross( quat.xyz, tmp );
}

void QTangentToLocalBasis( in vec4 qtangent, out localBasis LB ) {
	LB.normal = QuatTransVec( qtangent, vec3( 0.0, 0.0, 1.0 ) );
	LB.tangent = QuatTransVec( qtangent, vec3( 1.0, 0.0, 0.0 ) );
	LB.tangent *= sign( qtangent.w );
	LB.binormal = QuatTransVec( qtangent, vec3( 0.0, 1.0, 0.0 ) );
}

#if !defined(USE_VERTEX_ANIMATION) && !defined(USE_VERTEX_SKINNING) && !defined(USE_VERTEX_SPRITE)

attribute vec3 attr_Position;
attribute vec4 attr_Color;
attribute vec4 attr_QTangent;
attribute vec4 attr_TexCoord0;

void VertexFetch(out vec4 position,
		 out localBasis normalBasis,
		 out vec4 color,
		 out vec2 texCoord,
		 out vec2 lmCoord)
{
	position = vec4( attr_Position, 1.0 );
	QTangentToLocalBasis( attr_QTangent, normalBasis );
	color    = attr_Color;
	texCoord = attr_TexCoord0.xy;
	lmCoord  = attr_TexCoord0.zw;
}
#endif
