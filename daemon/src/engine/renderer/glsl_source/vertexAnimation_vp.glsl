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
// vertexAnimation_vp.glsl - interpolates .md3/.mdc vertex animations

#if defined(USE_VERTEX_ANIMATION)

attribute vec3 attr_Position;
attribute vec4 attr_Color;
attribute vec4 attr_QTangent;
attribute vec2 attr_TexCoord0;
attribute vec3 attr_Position2;
attribute vec4 attr_QTangent2;

uniform float u_VertexInterpolation;

void VertexAnimation_P_N(	vec3 fromPosition, vec3 toPosition,
				vec4 fromQTangent, vec4 toQTangent,
				float frac,
				inout vec4 position, inout vec3 normal)
{
	vec3 fromNormal = QuatTransVec( fromQTangent, vec3( 0.0, 0.0, 1.0 ) );
	vec3 toNormal = QuatTransVec( toQTangent, vec3( 0.0, 0.0, 1.0 ) );

	position.xyz = 512.0 * mix(fromPosition, toPosition, frac);
	position.w = 1;

	normal = normalize(mix(fromNormal, toNormal, frac));
}

void VertexFetch(out vec4 position,
		 out localBasis LB,
		 out vec4 color,
		 out vec2 texCoord,
		 out vec2 lmCoord)
{
	localBasis fromLB, toLB;

	QTangentToLocalBasis( attr_QTangent, fromLB );
	QTangentToLocalBasis( attr_QTangent2, toLB );

	position.xyz = 512.0 * mix(attr_Position, attr_Position2, u_VertexInterpolation);
	position.w = 1;
	
	LB.normal = normalize(mix(fromLB.normal, toLB.normal, u_VertexInterpolation));
	LB.tangent = normalize(mix(fromLB.tangent, toLB.tangent, u_VertexInterpolation));
	LB.binormal = normalize(mix(fromLB.binormal, toLB.binormal, u_VertexInterpolation));

	color    = attr_Color;
	texCoord = attr_TexCoord0;
	lmCoord  = attr_TexCoord0;
}
#endif
