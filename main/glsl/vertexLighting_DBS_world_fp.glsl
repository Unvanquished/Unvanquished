/*
===========================================================================
Copyright (C) 2008-2011 Robert Beckebans <trebor_7@users.sourceforge.net>

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

/* vertexLighting_DBS_world_fp.glsl */

uniform sampler2D	u_DiffuseMap;
uniform sampler2D	u_NormalMap;
uniform sampler2D	u_SpecularMap;
uniform sampler2D	u_GlowMap;

uniform float		u_AlphaThreshold;
uniform float		u_DepthScale;
uniform	float		u_LightWrapAround;

uniform sampler3D       u_LightGrid1;
uniform sampler3D       u_LightGrid2;
uniform vec3            u_LightGridOrigin;
uniform vec3            u_LightGridScale;

varying vec4		var_TexDiffuseGlow;
varying vec4		var_Color;

#if defined(USE_NORMAL_MAPPING)
varying vec4		var_TexNormalSpecular;
varying vec3		var_ViewDir; // direction from surface to viewer
varying vec3            var_Position;
#else
varying vec3		var_Normal;
#endif

#if defined(USE_NORMAL_MAPPING)
void ReadLightGrid(in vec3 pos, out vec3 lgtDir,
		   out vec3 ambCol, out vec3 lgtCol ) {
	vec4 texel1 = texture3D(u_LightGrid1, pos);
	vec4 texel2 = texture3D(u_LightGrid2, pos);

	ambCol = texel1.xyz;
	lgtCol = texel2.xyz;

	lgtDir.x = (255.0 * texel1.w - 128.0) / 127.0;
	lgtDir.y = (255.0 * texel2.w - 128.0) / 127.0;
	lgtDir.z = 1.0 - abs( lgtDir.x ) - abs( lgtDir.y );

	vec2 signs = 2.0 * step( 0.0, lgtDir.xy ) - vec2( 1.0 );
	if( lgtDir.z < 0.0 ) {
		lgtDir.xy = signs * ( vec2( 1.0 ) - abs( lgtDir.yx ) );
	}

	lgtDir = normalize( lgtDir );
}
#endif

void	main()
{
	vec2 texDiffuse = var_TexDiffuseGlow.st;
	vec2 texGlow = var_TexDiffuseGlow.pq;

#if defined(USE_NORMAL_MAPPING)
	vec3 V = normalize(var_ViewDir);

	vec3 L, ambCol, dirCol;
	ReadLightGrid( (var_Position - u_LightGridOrigin) * u_LightGridScale,
		       L, ambCol, dirCol);

	vec2 texNormal = var_TexNormalSpecular.st;
	vec2 texSpecular = var_TexNormalSpecular.pq;

#if defined(USE_PARALLAX_MAPPING)

	// ray intersect in view direction
	vec3 Vts = V;
	
	// size and start position of search in texture space
	vec2 S = Vts.xy * -u_DepthScale / Vts.z;

#if 0
	vec2 texOffset = vec2(0.0);
	for(int i = 0; i < 4; i++) {
		vec4 Normal = texture2D(u_NormalMap, texNormal.st + texOffset);
		float height = Normal.a * 0.2 - 0.0125;
		texOffset += height * Normal.z * S;
	}
#else
	float depth = RayIntersectDisplaceMap(texNormal, S, u_NormalMap);

	// compute texcoords offset
	vec2 texOffset = S * depth;
#endif

	texDiffuse.st += texOffset;
	texNormal.st += texOffset;
	texSpecular.st += texOffset;
#endif // USE_PARALLAX_MAPPING

	// compute the diffuse term
	vec4 diffuse = texture2D(u_DiffuseMap, texDiffuse) * var_Color;

	if( abs(diffuse.a + u_AlphaThreshold) <= 1.0 )
	{
		discard;
		return;
	}

	vec4 specular = texture2D(u_SpecularMap, texSpecular);

	// compute normal in tangent space from normalmap
	vec3 N = texture2D(u_NormalMap, texNormal.st).xyw;
	N.x *= N.z;
	N.xy = 2.0 * N.xy - 1.0;
	N.z = sqrt(1.0 - dot(N.xy, N.xy));
	
	#if defined(r_NormalScale)
	N.z *= r_NormalScale;
	#endif
	
	N = normalize(N);

	// compute final color
	vec4 color = vec4( ambCol, diffuse.a );
	computeLight( L, N, V, dirCol, diffuse, specular, color );

	#if defined(USE_GLOW_MAPPING)
	color.rgb += texture2D(u_GlowMap, texGlow).rgb;
	#endif

	gl_FragColor = color;
#else // USE_NORMAL_MAPPING

	vec3 N = normalize(var_Normal);

	// compute the diffuse term
	vec4 diffuse = texture2D(u_DiffuseMap, texDiffuse) * var_Color;

	if( abs(diffuse.a + u_AlphaThreshold) <= 1.0 )
	{
		discard;
		return;
	}

	vec4 specular = vec4(0.0);

	vec4 color = vec4( 0.0, 0.0, 0.0, diffuse.a );
	computeLight( N, N, N, vec3(1.0), diffuse, specular, color );

#if defined(USE_GLOW_MAPPING)
	color.rgb += texture2D(u_GlowMap, texGlow).rgb;
#endif

	gl_FragColor = color;
#endif
}
