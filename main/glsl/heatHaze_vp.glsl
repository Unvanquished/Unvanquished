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

/* heatHaze_vp.glsl */

uniform float		u_Time;

uniform mat4		u_NormalTextureMatrix;
uniform mat4		u_ProjectionMatrixTranspose;
uniform mat4		u_ModelViewMatrixTranspose;
uniform mat4		u_ModelViewProjectionMatrix;

uniform float		u_DeformMagnitude;

varying vec2		var_TexNormal;
varying float		var_Deform;

void	main()
{
	vec4            deformVec;
	float           d1, d2;

	vec4 position;
	localBasis LB;
	vec2 texCoord, lmCoord;
	vec4 color;

	VertexFetch( position, LB, color, texCoord, lmCoord );

	DeformVertex( position,
		      LB.normal,
		      texCoord,
		      color,
		      u_Time);

	// transform vertex position into homogenous clip-space
	gl_Position = u_ModelViewProjectionMatrix * position;

	// take the deform magnitude and scale it by the projection distance
	deformVec = vec4(1, 0, 0, 1);
	deformVec.z = dot(u_ModelViewMatrixTranspose[2], position);

	// transform normalmap texcoords
	var_TexNormal = (u_NormalTextureMatrix * vec4(texCoord, 0.0, 1.0)).st;

	d1 = dot(u_ProjectionMatrixTranspose[0],  deformVec);
	d2 = dot(u_ProjectionMatrixTranspose[3],  deformVec);

	// clamp the distance, so the deformations don't get too wacky near the view
	var_Deform = min(d1 * (1.0 / max(d2, 1.0)), 0.02) * u_DeformMagnitude;
}
