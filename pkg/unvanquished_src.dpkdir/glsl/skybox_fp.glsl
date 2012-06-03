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

/* skybox_fp.glsl */

uniform samplerCube	u_ColorMap;
uniform vec3		u_ViewOrigin;
uniform vec4		u_PortalPlane;

varying vec3		var_Position;

void	main()
{
	#if defined(USE_PORTAL_CLIPPING)
	{
		float dist = dot(var_Position.xyz, u_PortalPlane.xyz) - u_PortalPlane.w;
		if(dist < 0.0)
		{
			discard;
			return;
		}
	}
#endif
	
	// compute incident ray
	vec3 I = normalize(var_Position - u_ViewOrigin);
	
	vec4 color = textureCube(u_ColorMap, I).rgba;
	
#if defined(r_DeferredShading)
	gl_FragData[0] = color;		
	gl_FragData[1] = vec4(0.0);
	gl_FragData[2] = vec4(0.0);
	gl_FragData[3] = vec4(0.0);
#else
	gl_FragColor = color;
#endif
}
