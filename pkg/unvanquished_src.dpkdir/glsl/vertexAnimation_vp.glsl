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

void VertexAnimation_P_N(	vec3 fromPosition, vec3 toPosition,
							vec3 fromNormal, vec3 toNormal,
							float frac,
							inout vec4 position, inout vec3 normal)
{
	position.xyz = mix(fromPosition, toPosition, frac);
	position.w = 1;
	
	normal = normalize(mix(fromNormal, toNormal, frac));
}

void VertexAnimation_P_TBN(	vec3 fromPosition, vec3 toPosition,
							vec3 fromTangent, vec3 toTangent,
							vec3 fromBinormal, vec3 toBinormal,
							vec3 fromNormal, vec3 toNormal,
							float frac,
							inout vec4 position, inout vec3 tangent, inout vec3 binormal, inout vec3 normal)
{
	position.xyz = mix(fromPosition, toPosition, frac);
	position.w = 1;
	
	tangent = normalize(mix(fromTangent, toTangent, frac));
	binormal = normalize(mix(fromBinormal, toBinormal, frac));
	normal = normalize(mix(fromNormal, toNormal, frac));
}
