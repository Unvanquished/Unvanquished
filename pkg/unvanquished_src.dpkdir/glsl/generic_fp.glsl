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

/* generic_fp.glsl */

uniform sampler2D	u_ColorMap;
uniform float		u_AlphaThreshold;

varying vec3		var_Position;
varying vec2		var_Tex;
varying vec4		var_Color;

void	main()
{
	vec4 color = texture2D(u_ColorMap, var_Tex);

	if( abs(color.a + u_AlphaThreshold) <= 1.0 )
	{
		discard;
		return;
	}

	color *= var_Color;
	gl_FragColor = color;

#if 0 //defined(USE_TCGEN_ENVIRONMENT)
	gl_FragColor = vec4(vec3(1.0, 0.0, 0.0), color.a);
#endif
}
