/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 2013 the Unvanquished developpers

This file is part of the Daemon GPL Source Code (Daemon Source Code).

Daemon Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Daemon Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon Source Code.  If not, see <http://www.gnu.org/licenses/>.

===========================================================================
*/

/* fxaa_fp.glsl */

// The FXAA parameters are put directly in fxaa3_11_fp.glsl
// because we cannot #include in the middle of a shader

uniform sampler2D	u_ColorMap;

void	main()
{
	gl_FragColor = FxaaPixelShader(
		gl_FragCoord.xy * r_FBufScale, //pos
		vec4(0.0), //not used
		u_ColorMap, //tex
		u_ColorMap, //not used
		u_ColorMap, //not used
		r_FBufScale, //fxaaQualityRcpFrame
		vec4(0.0), //not used
		vec4(0.0), //not used
		vec4(0.0), //not used
		0.75, //fxaaQualitySubpix
		0.166, //fxaaQualityEdgeThreshold
		0.0625, //fxaaQualityEdgeThresholdMin
		0.0, //not used
		0.0, //not used
		0.0, //not used
		vec4(0.0) //not used
	);
}
