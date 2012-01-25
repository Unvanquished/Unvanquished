/*
===========================================================================
Copyright (C) 2011 Robert Beckebans <trebor_7@users.sourceforge.net>

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

/* fogQuake3_vp.glsl */

attribute vec4		attr_Position;
attribute vec4		attr_TexCoord0;
attribute vec3		attr_Normal;
attribute vec4		attr_Color;

attribute vec4		attr_Position2;
attribute vec3		attr_Normal2;

uniform float		u_VertexInterpolation;

uniform vec3		u_ViewOrigin;

uniform float		u_Time;

uniform vec4		u_ColorModulate;
uniform vec4		u_Color;
uniform mat4		u_ModelMatrix;
uniform mat4		u_ModelViewProjectionMatrix;

uniform vec4		u_FogDistanceVector;
uniform vec4		u_FogDepthVector;
uniform float		u_FogEyeT;

varying vec3		var_Position;
varying vec2		var_Tex;
varying vec4		var_Color;





void	main()
{
	vec4 position;
	vec3 normal;

#if defined(USE_VERTEX_SKINNING)

	VertexSkinning_P_N(	attr_Position, attr_Normal,
						position, normal);
						
#elif defined(USE_VERTEX_ANIMATION)
	
	VertexAnimation_P_N(attr_Position, attr_Position2,
						attr_Normal, attr_Normal2,
						u_VertexInterpolation,
						position, normal);
	
#else
	position = attr_Position;
	normal = attr_Normal;
#endif
	
#if defined(USE_DEFORM_VERTEXES)
	position = DeformPosition2(	position,
								normal,
								attr_TexCoord0.st,
								u_Time);
#endif

	// transform vertex position into homogenous clip-space
	gl_Position = u_ModelViewProjectionMatrix * position;
	
	// transform position into world space
	var_Position = (u_ModelMatrix * position).xyz;
	
	// calculate the length in fog
	float s = dot(position.xyz, u_FogDistanceVector.xyz) + u_FogDistanceVector.w;
	float t = dot(position.xyz, u_FogDepthVector.xyz) + u_FogDepthVector.w;
		
	// partially clipped fogs use the T axis
#if defined(EYE_OUTSIDE)
	if(t < 1.0)
	{
		t = 1.0 / 32;	// point is outside, so no fogging
	}
	else
	{
		t = 1.0 / 32 + 30.0 / 32 * t / (t - u_FogEyeT);	// cut the distance at the fog plane
	}
#else
	if(t < 0)
	{
		t = 1.0 / 32;	// point is outside, so no fogging
	}
	else
	{
		t = 31.0 / 32;
	}
#endif

	var_Tex = vec2(s, t);
	
	var_Color = /* attr_Color * u_ColorModulate + */ u_Color;
}
