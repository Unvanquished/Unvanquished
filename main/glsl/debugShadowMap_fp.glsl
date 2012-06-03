/*
===========================================================================
Copyright (C) 2009 Robert Beckebans <trebor_7@users.sourceforge.net>

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

/* debugShadowMap_fp.glsl */

uniform sampler2D	u_ShadowMap;

varying vec2		var_TexCoord;

void	main()
{
#if defined(ESM)

	vec4 shadowMoments = texture2D(u_ShadowMap, var_TexCoord);
	
	float shadowDistance = shadowMoments.a;
		
	gl_FragColor = vec4(shadowDistance, 0.0, 0.0, 1.0);

#elif defined(VSM)
	vec4 shadowMoments = texture2D(u_ShadowMap, var_TexCoord);
	
	float shadowDistance = shadowMoments.r;
	float shadowDistanceSquared = shadowMoments.a;
		
	gl_FragColor = vec4(shadowDistance, 0.0, 0.0, 1.0);
	
#elif defined(EVSM)
	
	vec4 shadowMoments = texture2D(u_ShadowMap, var_TexCoord);
	
#if defined(r_EVSMPostProcess)
	float shadowDistance = shadowMoments.r;
#else
	float shadowDistance = log(shadowMoments.b);
#endif
		
	gl_FragColor = vec4(shadowDistance, 0.0, 0.0, 1.0);
#else
	discard;
#endif
}
