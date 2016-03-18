/*
===========================================================================
Copyright (C) 2007-2011 Robert Beckebans <trebor_7@users.sourceforge.net>

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

/* forwardLighting_fp.glsl */

/* swizzle one- and two-component textures to RG */
#ifdef TEXTURE_RG
#  define SWIZ1 r
#  define SWIZ2 rg
#else
#  define SWIZ1 a
#  define SWIZ2 ar
#endif

uniform sampler2D	u_DiffuseMap;
uniform sampler2D	u_NormalMap;
uniform sampler2D	u_SpecularMap;
uniform sampler2D	u_AttenuationMapXY;
uniform sampler2D	u_AttenuationMapZ;

#if defined(LIGHT_DIRECTIONAL)
uniform sampler2D	u_ShadowMap0;
uniform sampler2D	u_ShadowMap1;
uniform sampler2D	u_ShadowMap2;
uniform sampler2D	u_ShadowMap3;
uniform sampler2D	u_ShadowMap4;
uniform sampler2D	u_ShadowClipMap0;
uniform sampler2D	u_ShadowClipMap1;
uniform sampler2D	u_ShadowClipMap2;
uniform sampler2D	u_ShadowClipMap3;
uniform sampler2D	u_ShadowClipMap4;
#elif defined(LIGHT_PROJ)
uniform sampler2D	u_ShadowMap0;
uniform sampler2D	u_ShadowClipMap0;
#else
uniform samplerCube	u_ShadowMap;
uniform samplerCube	u_ShadowClipMap;
#endif

uniform sampler2D	u_RandomMap;	// random normals


uniform vec3		u_ViewOrigin;

#if defined(LIGHT_DIRECTIONAL)
uniform vec3		u_LightDir;
#else
uniform vec3		u_LightOrigin;
#endif
uniform vec3		u_LightColor;
uniform float		u_LightRadius;
uniform float       u_LightScale;
uniform	float		u_LightWrapAround;
uniform float		u_AlphaThreshold;

uniform mat4		u_ShadowMatrix[MAX_SHADOWMAPS];
uniform vec4		u_ShadowParallelSplitDistances;
uniform float       u_ShadowTexelSize;
uniform float       u_ShadowBlur;

uniform mat4		u_ViewMatrix;

uniform float		u_DepthScale;

varying vec3		var_Position;
varying vec4		var_TexDiffuse;
varying vec4		var_TexNormal;
varying vec2		var_TexSpecular;
varying vec4		var_TexAttenuation;
varying vec4		var_Tangent;
varying vec4		var_Binormal;
varying vec4		var_Normal;
//varying vec4		var_Color;

#if __VERSION__ > 120
out vec4 outputColor;
#else
#define outputColor gl_FragColor
#endif


/*
================
MakeNormalVectors

Given a normalized forward vector, create two
other perpendicular vectors
================
*/
void MakeNormalVectors(const vec3 forward, inout vec3 right, inout vec3 up)
{
	// this rotate and negate guarantees a vector
	// not colinear with the original
	right.y = -forward.x;
	right.z = forward.y;
	right.x = forward.z;

	float d = dot(right, forward);
	right += forward * -d;
	normalize(right);
	up = cross(right, forward);	// GLSL cross product is the same as in Q3A
}

float Rand(vec2 co)
{
	return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

float Noise(vec2 co)
{
	return Rand(floor(co * 128.0));
}

vec3 RandomVec3(vec2 uv)
{
	vec3 dir;

#if 1
	float r = Rand(uv);
	float angle = 2.0 * M_PI * r;// / 360.0;

	dir = normalize(vec3(cos(angle), sin(angle), r));
#else
	// dir = texture2D(u_NoiseMap, gl_FragCoord.st * r_FBufScale).rgb;
	dir = normalize(2.0 * (texture2D(u_RandomMap, uv).xyz - 0.5));
#endif

	return dir;
}


/*
source: http://en.wikipedia.org/wiki/Chebyshev%27s_inequality

X = distribution
mu = mean
sigma = standard deviation

=> then for any real number k > 0:

Pr(X -mu >= k * sigma) <= 1 / ( 1 + k^2)
*/

#if defined(VSM) || defined(EVSM)

float linstep(float low, float high, float v)
{
	return clamp((v - low)/(high - low), 0.0, 1.0);
}

float ChebyshevUpperBound(vec2 shadowMoments, float vertexDistance, float minVariance)
{
	float shadowDistance = shadowMoments.x;
	float shadowDistanceSquared = shadowMoments.y;

	// compute variance
	float E_x2 = shadowDistanceSquared;
	float Ex_2 = shadowDistance * shadowDistance;

	float variance = max(E_x2 - Ex_2, max(minVariance, VSM_EPSILON));
	// float variance = smoothstep(minVariance, 1.0, max(E_x2 - Ex_2, 0.0));

	// compute probabilistic upper bound
	float d = vertexDistance - shadowDistance;
	float pMax = variance / (variance + (d * d));

#if defined(r_LightBleedReduction)
	pMax = linstep(r_LightBleedReduction, 1.0, pMax);
#endif

	// one-tailed Chebyshev with k > 0
	return (vertexDistance <= shadowDistance ? 1.0 : pMax);
}
#endif


#if defined(EVSM)
vec2 WarpDepth(float depth)
{
    // rescale depth into [-1, 1]
    depth = 2.0 * depth - 1.0;
    float pos =  exp( r_EVSMExponents.x * depth);
    float neg = -exp(-r_EVSMExponents.y * depth);

    return vec2(pos, neg);
}

vec4 ShadowDepthToEVSM(float depth)
{
	vec2 warpedDepth = WarpDepth(depth);
	return vec4(warpedDepth.x, warpedDepth.x * warpedDepth.x, warpedDepth.y, warpedDepth.y * warpedDepth.y);
}
#endif // #if defined(EVSM)

vec4 FixShadowMoments( vec4 moments )
{
#if !defined(EVSM) || defined(r_EVSMPostProcess)
	return vec4( moments.SWIZ2, moments.SWIZ2 );
#else
	return moments;
#endif
}

#if defined(LIGHT_DIRECTIONAL)

void FetchShadowMoments(vec3 Pworld, out vec4 shadowVert,
     			out vec4 shadowMoments, out vec4 shadowClipMoments)
{
	// transform to camera space
	vec4 Pcam = u_ViewMatrix * vec4(Pworld.xyz, 1.0);
	float vertexDistanceToCamera = -Pcam.z;

#if defined(r_ParallelShadowSplits_1)
	if(vertexDistanceToCamera < u_ShadowParallelSplitDistances.x)
	{
		shadowVert = u_ShadowMatrix[0] * vec4(Pworld.xyz, 1.0);
		shadowMoments = texture2DProj(u_ShadowMap0, shadowVert.xyw);
		shadowClipMoments = texture2DProj(u_ShadowClipMap0, shadowVert.xyw);
	}
	else
	{
		shadowVert = u_ShadowMatrix[1] * vec4(Pworld.xyz, 1.0);
		shadowMoments = texture2DProj(u_ShadowMap1, shadowVert.xyw);
		shadowClipMoments = texture2DProj(u_ShadowClipMap1, shadowVert.xyw);
	}
#elif defined(r_ParallelShadowSplits_2)
	if(vertexDistanceToCamera < u_ShadowParallelSplitDistances.x)
	{
		shadowVert = u_ShadowMatrix[0] * vec4(Pworld.xyz, 1.0);
		shadowMoments = texture2DProj(u_ShadowMap0, shadowVert.xyw);
		shadowClipMoments = texture2DProj(u_ShadowClipMap0, shadowVert.xyw);
	}
	else if(vertexDistanceToCamera < u_ShadowParallelSplitDistances.y)
	{
		shadowVert = u_ShadowMatrix[1] * vec4(Pworld.xyz, 1.0);
		shadowMoments = texture2DProj(u_ShadowMap1, shadowVert.xyw);
		shadowClipMoments = texture2DProj(u_ShadowClipMap1, shadowVert.xyw);
	}
	else
	{
		shadowVert = u_ShadowMatrix[2] * vec4(Pworld.xyz, 1.0);
		shadowMoments = texture2DProj(u_ShadowMap2, shadowVert.xyw);
		shadowClipMoments = texture2DProj(u_ShadowClipMap2, shadowVert.xyw);
	}
#elif defined(r_ParallelShadowSplits_3)
	if(vertexDistanceToCamera < u_ShadowParallelSplitDistances.x)
	{
		shadowVert = u_ShadowMatrix[0] * vec4(Pworld.xyz, 1.0);
		shadowMoments = texture2DProj(u_ShadowMap0, shadowVert.xyw);
		shadowClipMoments = texture2DProj(u_ShadowClipMap0, shadowVert.xyw);
	}
	else if(vertexDistanceToCamera < u_ShadowParallelSplitDistances.y)
	{
		shadowVert = u_ShadowMatrix[1] * vec4(Pworld.xyz, 1.0);
		shadowMoments = texture2DProj(u_ShadowMap1, shadowVert.xyw);
		shadowClipMoments = texture2DProj(u_ShadowClipMap1, shadowVert.xyw);
	}
	else if(vertexDistanceToCamera < u_ShadowParallelSplitDistances.z)
	{
		shadowVert = u_ShadowMatrix[2] * vec4(Pworld.xyz, 1.0);
		shadowMoments = texture2DProj(u_ShadowMap2, shadowVert.xyw);
		shadowClipMoments = texture2DProj(u_ShadowClipMap2, shadowVert.xyw);
	}
	else
	{
		shadowVert = u_ShadowMatrix[3] * vec4(Pworld.xyz, 1.0);
		shadowMoments = texture2DProj(u_ShadowMap3, shadowVert.xyw);
		shadowClipMoments = texture2DProj(u_ShadowClipMap3, shadowVert.xyw);
	}
#elif defined(r_ParallelShadowSplits_4)
	if(vertexDistanceToCamera < u_ShadowParallelSplitDistances.x)
	{
		shadowVert = u_ShadowMatrix[0] * vec4(Pworld.xyz, 1.0);
		shadowMoments = texture2DProj(u_ShadowMap0, shadowVert.xyw);
		shadowClipMoments = texture2DProj(u_ShadowClipMap0, shadowVert.xyw);
	}
	else if(vertexDistanceToCamera < u_ShadowParallelSplitDistances.y)
	{
		shadowVert = u_ShadowMatrix[1] * vec4(Pworld.xyz, 1.0);
		shadowMoments = texture2DProj(u_ShadowMap1, shadowVert.xyw);
		shadowClipMoments = texture2DProj(u_ShadowClipMap1, shadowVert.xyw);
	}
	else if(vertexDistanceToCamera < u_ShadowParallelSplitDistances.z)
	{
		shadowVert = u_ShadowMatrix[2] * vec4(Pworld.xyz, 1.0);
		shadowMoments = texture2DProj(u_ShadowMap2, shadowVert.xyw);
		shadowClipMoments = texture2DProj(u_ShadowClipMap2, shadowVert.xyw);
	}
	else if(vertexDistanceToCamera < u_ShadowParallelSplitDistances.w)
	{
		shadowVert = u_ShadowMatrix[3] * vec4(Pworld.xyz, 1.0);
		shadowMoments = texture2DProj(u_ShadowMap3, shadowVert.xyw);
		shadowClipMoments = texture2DProj(u_ShadowClipMap3, shadowVert.xyw);
	}
	else
	{
		shadowVert = u_ShadowMatrix[4] * vec4(Pworld.xyz, 1.0);
		shadowMoments = texture2DProj(u_ShadowMap4, shadowVert.xyw);
		shadowClipMoments = texture2DProj(u_ShadowClipMap4, shadowVert.xyw);
	}
#else
	{
		shadowVert = u_ShadowMatrix[0] * vec4(Pworld.xyz, 1.0);
		shadowMoments = texture2DProj(u_ShadowMap0, shadowVert.xyw);
		shadowClipMoments = texture2DProj(u_ShadowClipMap0, shadowVert.xyw);
	}
#endif

	shadowMoments = FixShadowMoments(shadowMoments);
	shadowClipMoments = FixShadowMoments(shadowClipMoments);
	
#if defined(EVSM) && defined(r_EVSMPostProcess)
	shadowMoments = ShadowDepthToEVSM(shadowMoments.x);
	shadowClipMoments = ShadowDepthToEVSM(shadowClipMoments.x);
#endif
}

#if defined(r_PCFSamples)
vec4 PCF(vec3 Pworld, float filterWidth, float samples, out vec4 clipMoments)
{
	vec3 forward, right, up;

	// filterWidth *= u_LightRadius;

	forward = normalize(-u_LightDir);
	MakeNormalVectors(forward, right, up);

	vec4 moments = vec4(0.0, 0.0, 0.0, 0.0);
	clipMoments = vec4(0.0, 0.0, 0.0, 0.0);

#if 0
	// compute step size for iterating through the kernel
	float stepSize = 2.0 * filterWidth / samples;

	for(float i = -filterWidth; i < filterWidth; i += stepSize)
	{
		for(float j = -filterWidth; j < filterWidth; j += stepSize)
		{
			vec4 shadowVert;
			vec4 shadowMoments;
			vec4 shadowClipMoments;

			FetchShadowMoments(Pworld + right * i + up * j, shadowVert, shadowMoments, shadowClipMoments);
			moments += shadowMoments;
			clipMoments += shadowClipMoments;
		}
	}
#else
	for(int i = 0; i < samples; i++)
	{
		for(int j = 0; j < samples; j++)
		{
			vec3 rand = RandomVec3(gl_FragCoord.st * r_FBufScale + vec2(i, j)) * filterWidth;
			// rand.z = 0;
			// rand = normalize(rand) * filterWidth;

			vec4 shadowVert;
			vec4 shadowMoments;
			vec4 shadowClipMoments;

			FetchShadowMoments(Pworld + right * rand.x + up * rand.y, shadowVert, shadowMoments, shadowClipMoments);
			moments += shadowMoments;
			clipMoments += shadowClipMoments;
		}
	}
#endif

	// return average of the samples
	float factor = (1.0 / (samples * samples));
	moments *= factor;
	clipMoments *= factor;
	return moments;
}
#endif // #if defined(r_PCFSamples)



#elif defined(LIGHT_PROJ)

void FetchShadowMoments(vec2 st, out vec4 shadowMoments, out vec4 shadowClipMoments)
{
#if defined(EVSM) && defined(r_EVSMPostProcess)
	shadowMoments = ShadowDepthToEVSM(texture2D(u_ShadowMap0, st).SWIZ1);
	shadowClipMoments = ShadowDepthToEVSM(texture2D(u_ShadowClipMap0, st).SWIZ1);
#else
	shadowMoments = FixShadowMoments(texture2D(u_ShadowMap0, st));
	shadowClipMoments = FixShadowMoments(texture2D(u_ShadowClipMap0, st));
#endif
}

#if defined(r_PCFSamples)
vec4 PCF(vec4 shadowVert, float filterWidth, float samples, out vec4 clipMoments)
{
	vec4 moments = vec4(0.0, 0.0, 0.0, 0.0);
	clipMoments = moments;

#if 0
	// compute step size for iterating through the kernel
	float stepSize = 2.0 * filterWidth / samples;

	for(float i = -filterWidth; i < filterWidth; i += stepSize)
	{
		for(float j = -filterWidth; j < filterWidth; j += stepSize)
		{
			vec4 sm, scm;
			FetchShadowMoments(shadowVert.xy / shadowVert.w + vec2(i, j), sm, scm);
			moments += sm;
			clipMoments += scm;
		}
	}
#else
	for(int i = 0; i < samples; i++)
	{
		for(int j = 0; j < samples; j++)
		{
			vec3 rand = RandomVec3(gl_FragCoord.st * r_FBufScale + vec2(i, j)) * filterWidth;
			// rand = vec3(0.0, 0.0, 1.0);
			// rand.z = 0;
			// rand = normalize(rand);// * filterWidth;

			vec4 sm, scm;
			FetchShadowMoments(shadowVert.xy / shadowVert.w + rand.xy, sm, scm);
			moments += sm;
			clipMoments += scm;
		}
	}
#endif

	// return average of the samples
	float factor = (1.0 / (samples * samples));
	moments *= factor;
	clipMoments *= factor;
	return moments;
}
#endif // #if defined(r_PCFSamples)

#else

void FetchShadowMoments(vec3 I, out vec4 shadowMoments, out vec4 shadowClipMoments)
{
#if defined(EVSM) && defined(r_EVSMPostProcess)
	shadowMoments = ShadowDepthToEVSM(textureCube(u_ShadowMap, I).SWIZ1);
	shadowClipMoments = ShadowDepthToEVSM(textureCube(u_ShadowClipMap, I).SWIZ1);
#else
	shadowMoments = FixShadowMoments(textureCube(u_ShadowMap, I));
	shadowClipMoments = FixShadowMoments(textureCube(u_ShadowClipMap, I));
#endif
}

#if defined(r_PCFSamples)
vec4 PCF(vec4 I, float filterWidth, float samples, out vec4 clipMoments)
{
	vec3 forward, right, up;

	forward = normalize(I.xyz);
	MakeNormalVectors(forward, right, up);

	vec4 moments = vec4(0.0, 0.0, 0.0, 0.0);
	clipMoments = moments;

#if 0
	// compute step size for iterating through the kernel
	float stepSize = 2.0 * filterWidth / samples;

	for(float i = -filterWidth; i < filterWidth; i += stepSize)
	{
		for(float j = -filterWidth; j < filterWidth; j += stepSize)
		{
			vec4 sm, scm;
			FetchShadowMoments(I.xyz + right * i + up * j, sm, scm);
			moments += sm;
			clipMoments += scm;
		}
	}
#else
	for(int i = 0; i < samples; i++)
	{
		for(int j = 0; j < samples; j++)
		{
			vec3 rand = RandomVec3(gl_FragCoord.st * r_FBufScale + vec2(i, j)) * filterWidth;
			// rand.z = 0;
			// rand = normalize(rand) * filterWidth;

			vec4 sm, scm;
			FetchShadowMoments(I.xyz + right * rand.x + up * rand.y, sm, scm);
			moments += sm;
			clipMoments += scm;
		}
	}
#endif

	// return average of the samples
	float factor = (1.0 / (samples * samples));
	moments *= factor;
	clipMoments *= factor;
	return moments;
}
#endif // #if defined(r_PCFSamples)

#endif


#if 0//defined(PCSS)


#if defined(LIGHT_DIRECTIONAL)
// TODO SumBlocker for sun shadowing

#elif defined(LIGHT_PROJ)
float SumBlocker(vec4 shadowVert, float vertexDistance, float filterWidth, float samples)
{
	float stepSize = 2.0 * filterWidth / samples;

	float blockerCount = 0.0;
    float blockerSum = 0.0;

	for(float i = -filterWidth; i < filterWidth; i += stepSize)
	{
		for(float j = -filterWidth; j < filterWidth; j += stepSize)
		{
			float shadowDistance = texture2DProj(u_ShadowMap0, vec3(shadowVert.xy + vec2(i, j), shadowVert.w)).SWIZ1;
			// float shadowDistance = texture2D(u_ShadowMap, shadowVert.xy / shadowVert.w + vec2(i, j)).x;

			// FIXME VSM_CLAMP

			if(vertexDistance > shadowDistance)
			{
				blockerCount += 1.0;
				blockerSum += shadowDistance;
			}
		}
	}

	float result;
	if(blockerCount > 0.0)
		result = blockerSum / blockerCount;
	else
		result = 0.0;

	return result;
}
#else
// case LIGHT_OMNI
float SumBlocker(vec4 I, float vertexDistance, float filterWidth, float samples)
{
	vec3 forward, right, up;

	forward = normalize(I.xyz);
	MakeNormalVectors(forward, right, up);

	float stepSize = 2.0 * filterWidth / samples;

	float blockerCount = 0.0;
    float blockerSum = 0.0;

	for(float i = -filterWidth; i < filterWidth; i += stepSize)
	{
		for(float j = -filterWidth; j < filterWidth; j += stepSize)
		{
			float shadowDistance = textureCube(u_ShadowMap, I.xyz + right * i + up * j).SWIZ1;

			if(vertexDistance > shadowDistance)
			{
				blockerCount += 1.0;
				blockerSum += shadowDistance;
			}
		}
	}

	float result;
	if(blockerCount > 0.0)
		result = blockerSum / blockerCount;
	else
		result = -1.0;

	return result;
}
#endif

float EstimatePenumbra(float vertexDistance, float blocker)
{
	float penumbra;

	if(blocker == 0.0)
		penumbra = 0.0;
	else
		penumbra = ((vertexDistance - blocker) * u_LightRadius) / blocker;

	return penumbra;
}

vec4 PCSS( vec4 I, float vertexDistance, float PCFSamples )
{
	// step 1: find blocker estimate
	const float blockerSamples = 6.0;
	float blockerSearchWidth = u_ShadowTexelSize * u_LightRadius / vertexDistance;
	float blocker = SumBlocker(I, vertexDistance, blockerSearchWidth, blockerSamples);

	// step 2: estimate penumbra using parallel planes approximation
	float penumbra = EstimatePenumbra(vertexDistance, blocker);

	// step 3: compute percentage-closer filter
	vec4 shadowMoments;

	if(penumbra > 0.0 && blocker > -1.0)
	{
		// float maxpen = PCFsamples * (1.0 / u_ShadowTexelSize);
		// if(penumbra > maxpen)
		//	penumbra = maxpen;
		//

		// shadowMoments = PCF(I, penumbra, PCFsamples);
		vec4 clipMoments;
		shadowMoments = PCF(I, u_ShadowTexelSize * u_ShadowBlur * penumbra, PCFsamples, clipMoments);
	}
	else
	{
		shadowMoments = FetchShadowMoments(I);
	}
}
#endif

float ShadowTest( float vertexDistance, vec4 shadowMoments, vec4 shadowClipMoments )
{
	float shadow = 1.0;
#if defined( ESM )
	float shadowDistance = shadowMoments.x;

	// standard shadow mapping
	if( vertexDistance <= 1.0 )
		shadow = step( vertexDistance, shadowDistance );
	if( u_LightScale < 0.0 ) {
		shadow = 1.0 - shadow;
		shadow *= step( vertexDistance, shadowClipMoments.x );
	}

	//shadow = vertexDistance <= shadowDistance ? 1.0 : 0.0;
	// exponential shadow mapping
	// shadow = clamp(exp(r_OverDarkeningFactor * (shadowDistance - log(vertexDistance))), 0.0, 1.0);
	// shadow = clamp(exp(r_OverDarkeningFactor * shadowDistance) * exp(-r_OverDarkeningFactor * vertexDistance), 0.0, 1.0);
	// shadow = smoothstep(0.0, 1.0, shadow);

#if defined(r_DebugShadowMaps) && defined( HAVE_EXT_gpu_shader4 )
	outputColor.r = (r_DebugShadowMaps & 1) != 0 ? shadowDistance : 0.0;
	outputColor.g = (r_DebugShadowMaps & 2) != 0 ? -(shadowDistance - vertexDistance) : 0.0;
	outputColor.b = (r_DebugShadowMaps & 4) != 0 ? shadow : 0.0;
	outputColor.a = 1.0;
#endif
		
#elif defined( VSM )
#if defined(VSM_CLAMP)
	// convert to [-1, 1] vector space
	shadowMoments = 2.0 * (shadowMoments - 0.5);
#endif

	shadow = ChebyshevUpperBound(shadowMoments.xy, vertexDistance, VSM_EPSILON);
	if( u_LightScale < 0.0 ) {
		shadow = 1.0 - shadow;
		shadow *= ChebyshevUpperBound(shadowClipMoments.xy, vertexDistance, VSM_EPSILON);
	}
#elif defined( EVSM )
	vec2 warpedVertexDistances = WarpDepth(vertexDistance);

	// derivative of warping at depth
	vec2 depthScale = VSM_EPSILON * r_EVSMExponents * warpedVertexDistances;
	vec2 minVariance = depthScale * depthScale;

	float posContrib = ChebyshevUpperBound(shadowMoments.xy, warpedVertexDistances.x, minVariance.x);
	float negContrib = ChebyshevUpperBound(shadowMoments.zw, warpedVertexDistances.y, minVariance.y);

	shadow = min(posContrib, negContrib);
	
#if defined(r_DebugShadowMaps) && defined( HAVE_EXT_gpu_shader4 )
	outputColor.r = (r_DebugShadowMaps & 1) != 0 ? posContrib : 0.0;
	outputColor.g = (r_DebugShadowMaps & 2) != 0 ? negContrib : 0.0;
	outputColor.b = (r_DebugShadowMaps & 4) != 0 ? shadow : 0.0;
	outputColor.a = 1.0;
#endif

	if( u_LightScale < 0.0 ) {
		shadow = 1.0 - shadow;
		posContrib = ChebyshevUpperBound(shadowClipMoments.xy, warpedVertexDistances.x, minVariance.x);
		negContrib = ChebyshevUpperBound(shadowClipMoments.zw, warpedVertexDistances.y, minVariance.y);

		shadow *= 1.0 - min(posContrib, negContrib);
	}
#endif
	return shadow;
}

/*
Some explanations by Marco Salvi about exponential shadow mapping:

Now you are filtering exponential values which rapidly go out of range,
to avoid this issue you can filter the logarithm of these values (and the go back to exp space)

For example if you averaging two exponential value such as exp(A) and exp(B) you have:

a*exp(A) + b*exp(B) (a and b are some filter weights)

but you can rewrite the same expression as:

exp(A) * (a + b*exp(B-A)) ,

exp(A) * exp( log (a + b*exp(B-A)))),

and:

exp(A + log(a + b*exp(B-A))

Now your sum of exponential is written as a single exponential, if you take the logarithm of it you can then just work on its argument:

A + log(a + b*exp(B-A))

Basically you end up filtering the argument of your exponential functions, which are just linear depth values,
so you enjoy the same range you have with less exotic techniques.
Just don't forget to go back to exp space when you use the final filtered value.


Though hardware texture filtering is not mathematically correct in log space it just causes a some overdarkening, nothing major.

If you have your shadow map filtered in log space occlusion is just computed like this (let assume we use bilinear filtering):

float occluder = tex2D( esm_sampler, esm_uv );
float occlusion = exp( occluder - receiver );

while with filtering in exp space you have:

float exp_occluder = tex2D( esm_sampler, esm_uv );
float occlusion = exp_occluder / exp( receiver );

EDIT: if more complex filters are used (trilinear, aniso, with mip maps) you need to generate mip maps using log filteirng as well.
*/

/*
float log_conv(float x0, float X, float y0, float Y)
{
    return (X + log(x0 + (y0 * exp(Y - X))));
}
*/

void	main()
{
#if 0
	// create random noise vector
	vec3 rand = RandomVec3(gl_FragCoord.st * r_FBufScale);

	outputColor = vec4(rand * 0.5 + 0.5, 1.0);
	return;
#endif


	float shadow = 1.0;
#if defined(USE_SHADOWING)
	const float SHADOW_BIAS = 0.010;
#if defined(LIGHT_DIRECTIONAL)


	vec4 shadowVert;
	vec4 shadowMoments, shadowClipMoments;
	FetchShadowMoments(var_Position.xyz, shadowVert, shadowMoments, shadowClipMoments);

	float vertexDistance = shadowVert.z - SHADOW_BIAS;
	// FIXME
#if 0 // defined(r_PCFSamples)
	shadowMoments = PCF(var_Position.xyz, u_ShadowTexelSize * u_ShadowBlur, r_PCFSamples);
#endif

#if 0
	outputColor = vec4(u_ShadowTexelSize * u_ShadowBlur * u_LightRadius, 0.0, 0.0, 1.0);
	return;
#endif

#if defined(r_ShowParallelShadowSplits)
	// transform to camera space
	vec4 Pcam = u_ViewMatrix * vec4(var_Position.xyz, 1.0);
	float vertexDistanceToCamera = -Pcam.z;

#if defined(r_ParallelShadowSplits_1)
	if(vertexDistanceToCamera < u_ShadowParallelSplitDistances.x)
	{
		outputColor = vec4(1.0, 0.0, 0.0, 1.0);
		return;
	}
	else
	{
		outputColor = vec4(1.0, 0.0, 1.0, 1.0);
		return;
	}
#elif defined(r_ParallelShadowSplits_2)
	if(vertexDistanceToCamera < u_ShadowParallelSplitDistances.x)
	{
		outputColor = vec4(1.0, 0.0, 0.0, 1.0);
		return;
	}
	else if(vertexDistanceToCamera < u_ShadowParallelSplitDistances.y)
	{
		outputColor = vec4(0.0, 1.0, 0.0, 1.0);
		return;
	}
	else
	{
		outputColor = vec4(1.0, 0.0, 1.0, 1.0);
		return;
	}
#elif defined(r_ParallelShadowSplits_3)
	if(vertexDistanceToCamera < u_ShadowParallelSplitDistances.x)
	{
		outputColor = vec4(1.0, 0.0, 0.0, 1.0);
		return;
	}
	else if(vertexDistanceToCamera < u_ShadowParallelSplitDistances.y)
	{
		outputColor = vec4(0.0, 1.0, 0.0, 1.0);
		return;
	}
	else if(vertexDistanceToCamera < u_ShadowParallelSplitDistances.z)
	{
		outputColor = vec4(0.0, 0.0, 1.0, 1.0);
		return;
	}
	else
	{
		outputColor = vec4(1.0, 0.0, 1.0, 1.0);
		return;
	}
#elif defined(r_ParallelShadowSplits_4)
	if(vertexDistanceToCamera < u_ShadowParallelSplitDistances.x)
	{
		outputColor = vec4(1.0, 0.0, 0.0, 1.0);
		return;
	}
	else if(vertexDistanceToCamera < u_ShadowParallelSplitDistances.y)
	{
		outputColor = vec4(0.0, 1.0, 0.0, 1.0);
		return;
	}
	else if(vertexDistanceToCamera < u_ShadowParallelSplitDistances.z)
	{
		outputColor = vec4(0.0, 0.0, 1.0, 1.0);
		return;
	}
	else if(vertexDistanceToCamera < u_ShadowParallelSplitDistances.w)
	{
		outputColor = vec4(1.0, 1.0, 0.0, 1.0);
		return;
	}
	else
	{
		outputColor = vec4(1.0, 0.0, 1.0, 1.0);
		return;
	}
#else
	{
		outputColor = vec4(1.0, 0.0, 1.0, 1.0);
		return;
	}
#endif
#endif // #if defined(r_ShowParallelShadowSplits)

#elif defined(LIGHT_PROJ)

	vec4 shadowVert = u_ShadowMatrix[0] * vec4(var_Position.xyz, 1.0);

	// compute incident ray
	vec3 I = var_Position.xyz - u_LightOrigin;
	
	float vertexDistance = length(I) / u_LightRadius - SHADOW_BIAS;
	if( vertexDistance >= 1.0f ) {
		discard;
		return;
	}

#if defined(r_PCFSamples)
#if 0//defined( PCSS )
		vec4 shadowMoments = PCSS(vertexDistance, r_PCFSamples);
#else
		vec4 shadowClipMoments;
		vec4 shadowMoments = PCF(shadowVert, u_ShadowTexelSize * u_ShadowBlur, r_PCFSamples, shadowClipMoments);
#endif
#else

	// no filter
	vec4 shadowMoments, shadowClipMoments;

	FetchShadowMoments(shadowVert.xy / shadowVert.w, shadowMoments, shadowClipMoments);

#endif

#else
	// compute incident ray
	vec3 I = var_Position.xyz - u_LightOrigin;
	float ILen = length(I);
	float vertexDistance = ILen / u_LightRadius - SHADOW_BIAS;

#if 0
	outputColor = vec4(u_ShadowTexelSize * u_ShadowBlur * ILen, 0.0, 0.0, 1.0);
	return;
#endif

#if defined(r_PCFSamples)
#if 0//defined(PCSS)
	vec4 shadowMoments = PCSS(vec4(I, 0.0), r_PCFSamples);
#else
	vec4 shadowClipMoments;
	vec4 shadowMoments = PCF(vec4(I, 0.0), u_ShadowTexelSize * u_ShadowBlur * ILen, r_PCFSamples, shadowClipMoments);
#endif
#else
	// no extra filtering, single tap
	vec4 shadowMoments, shadowClipMoments;
	FetchShadowMoments(I, shadowMoments, shadowClipMoments);
#endif
#endif
	shadow = ShadowTest(vertexDistance, shadowMoments, shadowClipMoments);
	
#if defined(r_DebugShadowMaps)
	return;
#endif
	
	if(shadow <= 0.0)
	{
		discard;
		return;
	}

#endif // USE_SHADOWING

	// compute light direction in world space
#if defined(LIGHT_DIRECTIONAL)
	vec3 L = u_LightDir;
#else
	vec3 L = normalize(u_LightOrigin - var_Position);
#endif

	vec2 texDiffuse = var_TexDiffuse.st;

	mat3 tangentToWorldMatrix = mat3(var_Tangent.xyz, var_Binormal.xyz, var_Normal.xyz);

	vec2 texNormal = var_TexNormal.st;
	vec2 texSpecular = var_TexSpecular.st;

	// compute view direction in world space
	vec3 V = normalize(u_ViewOrigin - var_Position.xyz);

#if defined(USE_PARALLAX_MAPPING)

	// ray intersect in view direction

	// compute view direction in tangent space
	vec3 Vts = V * tangentToWorldMatrix;
	Vts = normalize(Vts);

	// size and start position of search in texture space
	vec2 S = Vts.xy * -u_DepthScale / Vts.z;

	float depth = RayIntersectDisplaceMap(texNormal, S, u_NormalMap);

	// compute texcoords offset
	vec2 texOffset = S * depth;

	texDiffuse.st += texOffset;
	texNormal.st += texOffset;
	texSpecular.st += texOffset;
#endif // USE_PARALLAX_MAPPING

	// compute half angle in world space
	vec3 H = normalize(L + V);

	// compute normal in tangent space from normalmap
	vec3 N = texture2D(u_NormalMap, texNormal.st).xyw;
	N.x *= N.z;
	N.xy = 2.0 * N.xy - 1.0;
	N.z = sqrt(1.0 - dot(N.xy, N.xy));
	
#if defined(r_NormalScale)
	N.z *= r_NormalScale;
	normalize(N);
#endif

	// transform normal into world space
	N = normalize(tangentToWorldMatrix * N);

	// compute the light term
#if defined(r_WrapAroundLighting)
	float NL = clamp(dot(N, L) + u_LightWrapAround, 0.0, 1.0) / clamp(1.0 + u_LightWrapAround, 0.0, 1.0);
#else
	float NL = clamp(dot(N, L), 0.0, 1.0);
#endif

	// compute the diffuse term
	vec4 diffuse = texture2D(u_DiffuseMap, texDiffuse.st);
	if( abs(diffuse.a + u_AlphaThreshold) <= 1.0 )
	{
		discard;
		return;
	}
	diffuse.rgb *= u_LightColor * NL;

	// compute the specular term
	vec4 spec = texture2D(u_SpecularMap, texSpecular).rgba;
	vec3 specular = spec.rgb * u_LightColor * pow(clamp(dot(N, H), 0.0, 1.0), u_SpecularExponent.x * spec.a + u_SpecularExponent.y) * r_SpecularScale;

	// compute light attenuation
#if defined(LIGHT_PROJ)
	vec3 attenuationXY = texture2DProj(u_AttenuationMapXY, var_TexAttenuation.xyw).rgb;
	vec3 attenuationZ  = texture2D(u_AttenuationMapZ, vec2(var_TexAttenuation.z + 0.5, 0.0)).rgb; // FIXME

#elif defined(LIGHT_DIRECTIONAL)
	vec3 attenuationXY = vec3(1.0);
	vec3 attenuationZ  = vec3(1.0);

#else
	vec3 attenuationXY = texture2D(u_AttenuationMapXY, var_TexAttenuation.xy).rgb;
	vec3 attenuationZ  = texture2D(u_AttenuationMapZ, vec2(var_TexAttenuation.z, 0)).rgb;
#endif

	// compute final color
	vec4 color = diffuse;

	color.rgb += specular;

#if !defined(LIGHT_DIRECTIONAL)
	color.rgb *= attenuationXY;
	color.rgb *= attenuationZ;
#endif
	color.rgb *= abs(u_LightScale);
	color.rgb *= shadow;

	color.r *= var_TexDiffuse.p;
	color.gb *= var_TexNormal.pq;

	if( u_LightScale < 0.0 ) {
		color.rgb = vec3( clamp(dot(color.rgb, vec3( 0.3333 ) ), 0.3, 0.7 ) );
	}

	outputColor = color;

#if 0
#if defined(USE_PARALLAX_MAPPING)
	outputColor = vec4(vec3(1.0, 0.0, 0.0), diffuse.a);
#else
	outputColor = vec4(vec3(0.0, 0.0, 1.0), diffuse.a);
#endif
#endif

#if 0
#if defined(USE_VERTEX_SKINNING)
	outputColor = vec4(vec3(1.0, 0.0, 0.0), diffuse.a);
#elif defined(USE_VERTEX_ANIMATION)
	outputColor = vec4(vec3(0.0, 0.0, 1.0), diffuse.a);
#else
	outputColor = vec4(vec3(0.0, 1.0, 0.0), diffuse.a);
#endif
#endif
}
