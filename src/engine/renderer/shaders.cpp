#include <string>
#include <unordered_map>

std::unordered_map<std::string, std::string> shadermap( {
{ "glsl/blurX_fp.glsl", R"(




uniform sampler2D	u_ColorMap;
uniform float		u_DeformMagnitude;
uniform vec2		u_TexScale;

void	main()
{
	vec2 st = gl_FragCoord.st * u_TexScale;

	#if 0
	float gaussFact[3] = float[3](1.0, 2.0, 1.0);
	float gaussSum = 4;
	const int tap = 1;
	#elif 0
	float gaussFact[5] = float[5](1.0, 4.0, 6.0, 4.0, 1.0);
	float gaussSum = 16.0;
	const int tap = 2;
	#elif 1
	float gaussFact[7] = float[7](1.0, 6.0, 15.0, 20.0, 15.0, 6.0, 1.0);
	float gaussSum = 64.0;
	const int tap = 3;
	#endif

	
	vec4 sumColors = vec4(0.0);
	
	for(int t = -tap; t <= tap; t++)
	{
		float weight = gaussFact[t + tap];
		sumColors += texture2D(u_ColorMap, st + vec2(t, 0) * u_TexScale * u_DeformMagnitude) * weight;
	}

	gl_FragColor = sumColors * (1.0 / gaussSum);
}
)" },
{ "glsl/blurX_vp.glsl", R"(




attribute vec3 		attr_Position;

uniform mat4		u_ModelViewProjectionMatrix;

void	main()
{
	
	gl_Position = u_ModelViewProjectionMatrix * vec4(attr_Position, 1.0);
}
)" },
{ "glsl/blurY_fp.glsl", R"(




uniform sampler2D	u_ColorMap;
uniform float		u_DeformMagnitude;
uniform vec2		u_TexScale;

void	main()
{
	vec2 st = gl_FragCoord.st * u_TexScale;
	
	#if 0
	float gaussFact[3] = float[3](1.0, 2.0, 1.0);
	float gaussSum = 4;
	const int tap = 1;
	#elif 0
	float gaussFact[5] = float[5](1.0, 4.0, 6.0, 4.0, 1.0);
	float gaussSum = 16.0;
	const int tap = 2;
	#elif 1
	float gaussFact[7] = float[7](1.0, 6.0, 15.0, 20.0, 15.0, 6.0, 1.0);
	float gaussSum = 64.0;
	const int tap = 3;
	#endif

	
	vec4 sumColors = vec4(0.0);
	
	for(int t = -tap; t <= tap; t++)
	{
		float weight = gaussFact[t + tap];
		sumColors += texture2D(u_ColorMap, st + vec2(0, t) * u_TexScale * u_DeformMagnitude) * weight;
	}
	
	gl_FragColor = sumColors * (1.0 / gaussSum);
}
)" },
{ "glsl/blurY_vp.glsl", R"(




attribute vec3 		attr_Position;

uniform mat4		u_ModelViewProjectionMatrix;

void	main()
{
	
	gl_Position = u_ModelViewProjectionMatrix * vec4(attr_Position, 1.0);
}
)" },
{ "glsl/cameraEffects_fp.glsl", R"(




uniform sampler2D u_CurrentMap;
uniform sampler3D u_ColorMap;
uniform vec4      u_ColorModulate;
uniform float     u_InverseGamma;

varying vec2		var_Tex;

void	main()
{
	
	vec2 stClamped = gl_FragCoord.st * r_FBufScale;

	
	vec2 st = stClamped * r_NPOTScale;

	vec4 original = clamp(texture2D(u_CurrentMap, st), 0.0, 1.0);

	vec4 color = original;

	
	vec3 colCoord = color.rgb * 15.0 / 16.0 + 0.5 / 16.0;
	colCoord.z *= 0.25;
	color.rgb = u_ColorModulate.x * texture3D(u_ColorMap, colCoord).rgb;
	color.rgb += u_ColorModulate.y * texture3D(u_ColorMap, colCoord + vec3(0.0, 0.0, 0.25)).rgb;
	color.rgb += u_ColorModulate.z * texture3D(u_ColorMap, colCoord + vec3(0.0, 0.0, 0.50)).rgb;
	color.rgb += u_ColorModulate.w * texture3D(u_ColorMap, colCoord + vec3(0.0, 0.0, 0.75)).rgb;

	color.xyz = pow(color.xyz, vec3(u_InverseGamma));

	gl_FragColor = color;
}
)" },
{ "glsl/cameraEffects_vp.glsl", R"(




attribute vec3 		attr_Position;
attribute vec2 		attr_TexCoord0;

uniform mat4		u_ModelViewProjectionMatrix;
uniform mat4		u_ColorTextureMatrix;

varying vec2		var_Tex;

void	main()
{
	
	gl_Position = u_ModelViewProjectionMatrix * vec4(attr_Position, 1.0);

	var_Tex = (u_ColorTextureMatrix * vec4(attr_TexCoord0, 0.0, 1.0)).st;
}
)" },
{ "glsl/contrast_fp.glsl", R"(




uniform sampler2D	u_ColorMap;

const vec4			LUMINANCE_VECTOR = vec4(0.2125, 0.7154, 0.0721, 0.0);


vec4 f(vec4 color) {
	float L = dot(LUMINANCE_VECTOR, color);
	L = max(L - 0.71, 0.0) * (1.0 / (1.0 - 0.71));
	
	return color * L;
}

void	main()
{
	vec2 scale = r_FBufScale * r_NPOTScale;
	vec2 st = gl_FragCoord.st;

	
	st *= r_FBufScale;

	
	st *= vec2(4.0, 4.0);

	
	st *= r_NPOTScale;

	
	vec4 color = f(texture2D(u_ColorMap, st + vec2(-1.0, -1.0) * scale));
	color += f(texture2D(u_ColorMap, st + vec2(-1.0, 1.0) * scale));
	color += f(texture2D(u_ColorMap, st + vec2(1.0, -1.0) * scale));
	color += f(texture2D(u_ColorMap, st + vec2(1.0, 1.0) * scale));
	color *= 0.25;

	gl_FragColor = color;
}
)" },
{ "glsl/contrast_vp.glsl", R"(




attribute vec3 		attr_Position;

uniform mat4		u_ModelViewProjectionMatrix;

void	main()
{
	
	gl_Position = u_ModelViewProjectionMatrix * vec4(attr_Position, 1.0);
}
)" },
{ "glsl/debugShadowMap_fp.glsl", R"(





#ifdef TEXTURE_RG
#  define SWIZ1 r
#  define SWIZ2 rg
#else
#  define SWIZ1 a
#  define SWIZ2 ar
#endif

uniform sampler2D	u_ShadowMap;

varying vec2		var_TexCoord;

void	main()
{
#if defined(ESM)

	float shadowMoments = texture2D(u_ShadowMap, var_TexCoord).SWIZ1;

	float shadowDistance = shadowMoments;

	gl_FragColor = vec4(shadowDistance, 0.0, 0.0, 1.0);

#elif defined(VSM)
	vec2 shadowMoments = texture2D(u_ShadowMap, var_TexCoord).SWIZ2;

	float shadowDistance = shadowMoments.r;
	float shadowDistanceSquared = shadowMoments.g;

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
)" },
{ "glsl/debugShadowMap_vp.glsl", R"(




attribute vec3 		attr_Position;
attribute vec2 		attr_TexCoord0;

uniform mat4		u_ModelViewProjectionMatrix;

varying vec2		var_TexCoord;

void	main()
{
	
	gl_Position = u_ModelViewProjectionMatrix * vec4(attr_Position, 1.0);

	var_TexCoord = attr_TexCoord0.st;
}
)" },
{ "glsl/deformVertexes_vp.glsl", R"(




float waveSin(float x) {
	return sin( radians( 360.0 * x ) );
}

float waveSquare(float x) {
	return sign( waveSin( x ) );
}

float waveTriangle(float x)
{
	return 1.0 - abs( 4.0 * fract( x + 0.25 ) - 2.0 );
}

float waveSawtooth(float x)
{
	return fract( x );
}

#if 0

ivec4 permute(ivec4 x) {
	return ((62 * x + 1905) * x) % 961;
}

float grad(int idx, vec4 p) {
	int i = idx & 31;
	float u = i < 24 ? p.x : p.y;
	float v = i < 16 ? p.y : p.z;
	float w = i < 8  ? p.z : p.w;

	return ((i & 4) != 0 ? -u : u)
	     + ((i & 2) != 0 ? -v : v)
	     + ((i & 1) != 0 ? -w : w);
}

vec4 fade(in vec4 t) {
	return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
}

vec4 pnoise(vec4 v) {
	ivec4 iv = ivec4(floor(v));
	vec4 rv = fract(v);
	vec4 f[2];

	f[0] = fade(rv);
	f[1] = 1.0 - f[0];

	vec4 value = vec4(0.0);
	for(int idx = 0; idx < 16; idx++) {
		ivec4 offs = ivec4( idx & 1, (idx & 2) >> 1, (idx & 4) >> 2, (idx & 8) >> 3 );
		float w = f[offs.x].x * f[offs.y].y * f[offs.z].z * f[offs.w].w;
		ivec4 p = permute(iv + offs);

		value.x += w * grad(p.x, rv);
		value.y += w * grad(p.y, rv);
		value.z += w * grad(p.z, rv);
		value.w += w * grad(p.w, rv);
	}
	return value;
}

#endif

void DeformVertex( inout vec4 pos,
		   inout vec3 normal,
		   inout vec2 st,
		   inout vec4 color,
		   in    float time)
{
	vec4 work = vec4(0.0);


#define spread (dot(work.xyz, vec3(1.0)))

#define DSTEP_LOAD_POS(X,Y,Z)     work.xyz = pos.xyz * vec3(X,Y,Z);
#define DSTEP_LOAD_NORM(X,Y,Z)    work.xyz = normal.xyz * vec3(X,Y,Z);
#define DSTEP_LOAD_COLOR(X,Y,Z)   work.xyz = color.xyz * vec3(X,Y,Z);
#define DSTEP_LOAD_TC(X,Y,Z)      work.xyz = vec3(st, 1.0) * vec3(X,Y,Z);
#define DSTEP_LOAD_VEC(X,Y,Z)     work.xyz = vec3(X,Y,Z);
#define DSTEP_MODIFY_POS(X,Y,Z)   pos.xyz += (X + Y * work.a) * work.xyz;
#define DSTEP_MODIFY_NORM(X,Y,Z)  normal.xyz += (X + Y * work.a) * work.xyz; \
				  normal = normalize(normal);
#define DSTEP_MODIFY_COLOR(X,Y,Z) color.xyz += (X + Y * work.a) * work.xyz;
#define DSTEP_SIN(X,Y,Z)          work.a = waveSin( X + Y * spread + Z * time );
#define DSTEP_SQUARE(X,Y,Z)       work.a = waveSquare( X + Y * spread + Z * time);
#define DSTEP_TRIANGLE(X,Y,Z)     work.a = waveTriangle( X + Y * spread + Z * time);
#define	DSTEP_SAWTOOTH(X,Y,Z)     work.a = waveSawtooth( X + Y * spread + Z * time);
#define DSTEP_INV_SAWTOOTH(X,Y,Z) work.a = 1.0 - waveSawtooth( X + Y * spread + Z * time);
#define DSTEP_NOISE(X,Y,Z)        work = noise4(vec4( Y * work.xyz, Z * time));
#define DSTEP_ROTGROW(X,Y,Z)      if(work.z > X * time) { work.a = 0.0; } else { work.a = Y * atan(pos.y, pos.x) + Z * time; work.a = 0.5 * sin(work.a) + 0.5; }

	
	DEFORM_STEPS
}
)" },
{ "glsl/depthToColor_fp.glsl", R"(





void	main()
{
	
	float depth = gl_FragCoord.z;




	
	const vec4 bitSh = vec4(256 * 256 * 256,	256 * 256,				256,         1);
	const vec4 bitMsk = vec4(			0,		1.0 / 256.0,    1.0 / 256.0,    1.0 / 256.0);

	vec4 comp;
	comp = depth * bitSh;
	comp = fract(comp);
	comp -= comp.xxyz * bitMsk;
	gl_FragColor = comp;


}

)" },
{ "glsl/depthToColor_vp.glsl", R"(




attribute vec3 		attr_Position;
#if defined(USE_VERTEX_SKINNING)
attribute vec3      attr_Normal;
#endif

uniform mat4		u_ModelViewProjectionMatrix;

void	main()
{
	#if defined(USE_VERTEX_SKINNING)
	{
		vec4 position;
		vec3 normal;
		
		VertexSkinning_P_N( attr_Position, attr_Normal, position, normal );

		
		gl_Position = u_ModelViewProjectionMatrix * position;
	}
	#else
	{
		
		gl_Position = u_ModelViewProjectionMatrix * vec4(attr_Position, 1.0);
	}
	#endif
}
)" },
{ "glsl/dispersion_C_fp.glsl", R"(




uniform samplerCube	u_ColorMap;
uniform vec3		u_ViewOrigin;
uniform vec3		u_EtaRatio;
uniform float		u_FresnelPower;
uniform float		u_FresnelScale;
uniform float		u_FresnelBias;

varying vec3		var_Position;
varying vec3		var_Normal;

void	main()
{
	
	vec3 I = normalize(var_Position - u_ViewOrigin);

	
	vec3 N = normalize(var_Normal);	

	
	vec3 R = reflect(I, N);

	
	float fresnel = u_FresnelBias + pow(1.0 - dot(I, N), u_FresnelPower) * u_FresnelScale;

	
	vec3 reflectColor = textureCube(u_ColorMap, R).rgb;

	
	vec3 refractColor;

	refractColor.r = textureCube(u_ColorMap, refract(I, N, u_EtaRatio.x)).r;
	refractColor.g = textureCube(u_ColorMap, refract(I, N, u_EtaRatio.y)).g;
	refractColor.b = textureCube(u_ColorMap, refract(I, N, u_EtaRatio.z)).b;

	
	vec4 color;
	color.r = (1.0 - fresnel) * refractColor.r + reflectColor.r * fresnel;
	color.g = (1.0 - fresnel) * refractColor.g + reflectColor.g * fresnel;
	color.b = (1.0 - fresnel) * refractColor.b + reflectColor.b * fresnel;
	color.a = 1.0;

	gl_FragColor = color;
}
)" },
{ "glsl/dispersion_C_vp.glsl", R"(




attribute vec3 		attr_Position;
attribute vec3		attr_Normal;
#if defined(r_VertexSkinning)
attribute vec4		attr_BoneIndexes;
attribute vec4		attr_BoneWeights;
uniform int			u_VertexSkinning;
uniform mat4		u_BoneMatrix[MAX_GLSL_BONES];
#endif

uniform mat4		u_ModelMatrix;
uniform mat4		u_ModelViewProjectionMatrix;

varying vec3		var_Position;
varying vec3		var_Normal;

void	main()
{
#if defined(r_VertexSkinning)
	if(bool(u_VertexSkinning))
	{
		vec4 vertex = vec4(0.0);
		vec3 normal = vec3(0.0);

		for(int i = 0; i < 4; i++)
		{
			int boneIndex = int(attr_BoneIndexes[i]);
			float boneWeight = attr_BoneWeights[i];
			mat4  boneMatrix = u_BoneMatrix[boneIndex];

			vertex += (boneMatrix * vec4(attr_Position, 1.0)) * boneWeight;
			normal += (boneMatrix * vec4(attr_Normal, 0.0)).xyz * boneWeight;
		}

		
		gl_Position = u_ModelViewProjectionMatrix * vertex;

		
		var_Position = (u_ModelMatrix * vertex).xyz;

		
		var_Normal = (u_ModelMatrix * vec4(normal, 0.0)).xyz;
	}
	else
#endif
	{
		
		gl_Position = u_ModelViewProjectionMatrix * vec4(attr_Position, 1.0);

		
		var_Position = (u_ModelMatrix * vec4(attr_Position, 1.0)).xyz;

		
		var_Normal = (u_ModelMatrix * vec4(attr_Normal, 0.0)).xyz;
	}
}

)" },
{ "glsl/fogGlobal_fp.glsl", R"(




uniform sampler2D	u_ColorMap; 
uniform sampler2D	u_DepthMap;
uniform vec3		u_ViewOrigin;
uniform vec4		u_FogDistanceVector;
uniform vec4		u_FogDepthVector;
uniform vec4		u_Color;
uniform mat4		u_ViewMatrix;
uniform mat4		u_UnprojectMatrix;

void	main()
{
	
	vec2 st = gl_FragCoord.st * r_FBufScale;

	
	st *= r_NPOTScale;

	
	float depth = texture2D(u_DepthMap, st).r;
	vec4 P = u_UnprojectMatrix * vec4(gl_FragCoord.xy, depth, 1.0);
	P.xyz /= P.w;

	
	st.s = dot(P.xyz, u_FogDistanceVector.xyz) + u_FogDistanceVector.w;
	
	st.t = 1.0;

	gl_FragColor = u_Color * texture2D(u_ColorMap, st);
}
)" },
{ "glsl/fogGlobal_vp.glsl", R"(




attribute vec3 		attr_Position;

uniform mat4		u_ModelViewProjectionMatrix;

void	main()
{
	
	gl_Position = u_ModelViewProjectionMatrix * vec4(attr_Position, 1.0);
}
)" },
{ "glsl/fogQuake3_fp.glsl", R"(




uniform sampler2D	u_ColorMap;

varying vec3		var_Position;
varying vec2		var_Tex;
varying vec4		var_Color;

void	main()
{
	vec4 color = texture2D(u_ColorMap, var_Tex);

	color *= var_Color;
	gl_FragColor = color;

#if 0
	gl_FragColor = vec4(vec3(1.0, 0.0, 0.0), color.a);
#endif
}
)" },
{ "glsl/fogQuake3_vp.glsl", R"(




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

void DeformVertex( inout vec4 pos,
		   inout vec3 normal,
		   inout vec2 st,
		   inout vec4 color,
		   in    float time);

void	main()
{
	vec4 position;
	localBasis LB;
	vec2 texCoord, lmCoord;
	vec4 color;

	VertexFetch( position, LB, color, texCoord, lmCoord );

	color =  u_Color;

	DeformVertex( position,
		      LB.normal,
		      texCoord,
		      color,
		      u_Time);

	
	gl_Position = u_ModelViewProjectionMatrix * position;

	
	var_Position = (u_ModelMatrix * position).xyz;

	
	float s = dot(position.xyz, u_FogDistanceVector.xyz) + u_FogDistanceVector.w;
	float t = dot(position.xyz, u_FogDepthVector.xyz) + u_FogDepthVector.w;

	
	if(u_FogEyeT < 0.0)
	{
		if(t < 1.0)
		{
			t = 1.0 / 32.0;	
		}
		else
		{
			t = 1.0 / 32.0 + 30.0 / 32.0 * t / (t - u_FogEyeT);	
		}
	}
	else
	{
		if(t < 0.0)
		{
			t = 1.0 / 32.0;	
		}
		else
		{
			t = 31.0 / 32.0;
		}
	}

	var_Tex = vec2(s, t);

	var_Color = color;
}
)" },
{ "glsl/forwardLighting_fp.glsl", R"(





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

uniform sampler2D	u_RandomMap;	


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
uniform vec2		u_SpecularExponent;

varying vec3		var_Position;
varying vec4		var_TexDiffuse;
varying vec4		var_TexNormal;
#if defined(USE_NORMAL_MAPPING)
varying vec2		var_TexSpecular;
#endif
varying vec4		var_TexAttenuation;
#if defined(USE_NORMAL_MAPPING)
varying vec4		var_Tangent;
varying vec4		var_Binormal;
#endif
varying vec4		var_Normal;





void MakeNormalVectors(const vec3 forward, inout vec3 right, inout vec3 up)
{
	
	
	right.y = -forward.x;
	right.z = forward.y;
	right.x = forward.z;

	float d = dot(right, forward);
	right += forward * -d;
	normalize(right);
	up = cross(right, forward);	
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
	float angle = 2.0 * M_PI * r;

	dir = normalize(vec3(cos(angle), sin(angle), r));
#else
	
	dir = normalize(2.0 * (texture2D(u_RandomMap, uv).xyz - 0.5));
#endif

	return dir;
}




#if defined(VSM) || defined(EVSM)

float linstep(float low, float high, float v)
{
	return clamp((v - low)/(high - low), 0.0, 1.0);
}

float ChebyshevUpperBound(vec2 shadowMoments, float vertexDistance, float minVariance)
{
	float shadowDistance = shadowMoments.x;
	float shadowDistanceSquared = shadowMoments.y;

	
	float E_x2 = shadowDistanceSquared;
	float Ex_2 = shadowDistance * shadowDistance;

	float variance = max(E_x2 - Ex_2, max(minVariance, VSM_EPSILON));
	

	
	float d = vertexDistance - shadowDistance;
	float pMax = variance / (variance + (d * d));

	#if defined(r_LightBleedReduction)
		pMax = linstep(r_LightBleedReduction, 1.0, pMax);
	#endif

	
	return (vertexDistance <= shadowDistance ? 1.0 : pMax);
}
#endif


#if defined(EVSM)
vec2 WarpDepth(float depth)
{
    
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
#endif 

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

	

	forward = normalize(-u_LightDir);
	MakeNormalVectors(forward, right, up);

	vec4 moments = vec4(0.0, 0.0, 0.0, 0.0);
	clipMoments = vec4(0.0, 0.0, 0.0, 0.0);

#if 0
	
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
			
			

			vec4 shadowVert;
			vec4 shadowMoments;
			vec4 shadowClipMoments;

			FetchShadowMoments(Pworld + right * rand.x + up * rand.y, shadowVert, shadowMoments, shadowClipMoments);
			moments += shadowMoments;
			clipMoments += shadowClipMoments;
		}
	}
#endif

	
	float factor = (1.0 / (samples * samples));
	moments *= factor;
	clipMoments *= factor;
	return moments;
}
#endif 



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
			
			
			

			vec4 sm, scm;
			FetchShadowMoments(shadowVert.xy / shadowVert.w + rand.xy, sm, scm);
			moments += sm;
			clipMoments += scm;
		}
	}
#endif

	
	float factor = (1.0 / (samples * samples));
	moments *= factor;
	clipMoments *= factor;
	return moments;
}
#endif 

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
			
			

			vec4 sm, scm;
			FetchShadowMoments(I.xyz + right * rand.x + up * rand.y, sm, scm);
			moments += sm;
			clipMoments += scm;
		}
	}
#endif

	
	float factor = (1.0 / (samples * samples));
	moments *= factor;
	clipMoments *= factor;
	return moments;
}
#endif 

#endif


#if 0


#if defined(LIGHT_DIRECTIONAL)


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
	
	const float blockerSamples = 6.0;
	float blockerSearchWidth = u_ShadowTexelSize * u_LightRadius / vertexDistance;
	float blocker = SumBlocker(I, vertexDistance, blockerSearchWidth, blockerSamples);

	
	float penumbra = EstimatePenumbra(vertexDistance, blocker);

	
	vec4 shadowMoments;

	if(penumbra > 0.0 && blocker > -1.0)
	{
		
		
		
		

		
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

	
	if( vertexDistance <= 1.0 )
		shadow = step( vertexDistance, shadowDistance );
	if( u_LightScale < 0.0 ) {
		shadow = 1.0 - shadow;
		shadow *= step( vertexDistance, shadowClipMoments.x );
	}

	
	
	
	
	

	#if defined(r_DebugShadowMaps)
	#extension GL_EXT_gpu_shader4 : enable
		gl_FragColor.r = (r_DebugShadowMaps & 1) != 0 ? shadowDistance : 0.0;
		gl_FragColor.g = (r_DebugShadowMaps & 2) != 0 ? -(shadowDistance - vertexDistance) : 0.0;
		gl_FragColor.b = (r_DebugShadowMaps & 4) != 0 ? shadow : 0.0;
		gl_FragColor.a = 1.0;
	#endif
		
#elif defined( VSM )
	#if defined(VSM_CLAMP)
		
		shadowMoments = 2.0 * (shadowMoments - 0.5);
	#endif

	shadow = ChebyshevUpperBound(shadowMoments.xy, vertexDistance, VSM_EPSILON);
	if( u_LightScale < 0.0 ) {
		shadow = 1.0 - shadow;
		shadow *= ChebyshevUpperBound(shadowClipMoments.xy, vertexDistance, VSM_EPSILON);
	}
#elif defined( EVSM )
	vec2 warpedVertexDistances = WarpDepth(vertexDistance);

	
	vec2 depthScale = VSM_EPSILON * r_EVSMExponents * warpedVertexDistances;
	vec2 minVariance = depthScale * depthScale;

	float posContrib = ChebyshevUpperBound(shadowMoments.xy, warpedVertexDistances.x, minVariance.x);
	float negContrib = ChebyshevUpperBound(shadowMoments.zw, warpedVertexDistances.y, minVariance.y);

	shadow = min(posContrib, negContrib);
	
	#if defined(r_DebugShadowMaps)
	#extension GL_EXT_gpu_shader4 : enable
		gl_FragColor.r = (r_DebugShadowMaps & 1) != 0 ? posContrib : 0.0;
		gl_FragColor.g = (r_DebugShadowMaps & 2) != 0 ? negContrib : 0.0;
		gl_FragColor.b = (r_DebugShadowMaps & 4) != 0 ? shadow : 0.0;
		gl_FragColor.a = 1.0;
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





void	main()
{
#if 0
	
	vec3 rand = RandomVec3(gl_FragCoord.st * r_FBufScale);

	gl_FragColor = vec4(rand * 0.5 + 0.5, 1.0);
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
	
	#if 0 
	shadowMoments = PCF(var_Position.xyz, u_ShadowTexelSize * u_ShadowBlur, r_PCFSamples);
	#endif

#if 0
	gl_FragColor = vec4(u_ShadowTexelSize * u_ShadowBlur * u_LightRadius, 0.0, 0.0, 1.0);
	return;
#endif

#if defined(r_ShowParallelShadowSplits)
	
	vec4 Pcam = u_ViewMatrix * vec4(var_Position.xyz, 1.0);
	float vertexDistanceToCamera = -Pcam.z;

#if defined(r_ParallelShadowSplits_1)
	if(vertexDistanceToCamera < u_ShadowParallelSplitDistances.x)
	{
		gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
		return;
	}
	else
	{
		gl_FragColor = vec4(1.0, 0.0, 1.0, 1.0);
		return;
	}
#elif defined(r_ParallelShadowSplits_2)
	if(vertexDistanceToCamera < u_ShadowParallelSplitDistances.x)
	{
		gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
		return;
	}
	else if(vertexDistanceToCamera < u_ShadowParallelSplitDistances.y)
	{
		gl_FragColor = vec4(0.0, 1.0, 0.0, 1.0);
		return;
	}
	else
	{
		gl_FragColor = vec4(1.0, 0.0, 1.0, 1.0);
		return;
	}
#elif defined(r_ParallelShadowSplits_3)
	if(vertexDistanceToCamera < u_ShadowParallelSplitDistances.x)
	{
		gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
		return;
	}
	else if(vertexDistanceToCamera < u_ShadowParallelSplitDistances.y)
	{
		gl_FragColor = vec4(0.0, 1.0, 0.0, 1.0);
		return;
	}
	else if(vertexDistanceToCamera < u_ShadowParallelSplitDistances.z)
	{
		gl_FragColor = vec4(0.0, 0.0, 1.0, 1.0);
		return;
	}
	else
	{
		gl_FragColor = vec4(1.0, 0.0, 1.0, 1.0);
		return;
	}
#elif defined(r_ParallelShadowSplits_4)
	if(vertexDistanceToCamera < u_ShadowParallelSplitDistances.x)
	{
		gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
		return;
	}
	else if(vertexDistanceToCamera < u_ShadowParallelSplitDistances.y)
	{
		gl_FragColor = vec4(0.0, 1.0, 0.0, 1.0);
		return;
	}
	else if(vertexDistanceToCamera < u_ShadowParallelSplitDistances.z)
	{
		gl_FragColor = vec4(0.0, 0.0, 1.0, 1.0);
		return;
	}
	else if(vertexDistanceToCamera < u_ShadowParallelSplitDistances.w)
	{
		gl_FragColor = vec4(1.0, 1.0, 0.0, 1.0);
		return;
	}
	else
	{
		gl_FragColor = vec4(1.0, 0.0, 1.0, 1.0);
		return;
	}
#else
	{
		gl_FragColor = vec4(1.0, 0.0, 1.0, 1.0);
		return;
	}
#endif
#endif 

#elif defined(LIGHT_PROJ)

	vec4 shadowVert = u_ShadowMatrix[0] * vec4(var_Position.xyz, 1.0);

	
	vec3 I = var_Position.xyz - u_LightOrigin;
	
	float vertexDistance = length(I) / u_LightRadius - SHADOW_BIAS;
	if( vertexDistance >= 1.0f ) {
		discard;
		return;
	}

	#if defined(r_PCFSamples)
		#if 0
		vec4 shadowMoments = PCSS(vertexDistance, r_PCFSamples);
		#else
		vec4 shadowClipMoments;
		vec4 shadowMoments = PCF(shadowVert, u_ShadowTexelSize * u_ShadowBlur, r_PCFSamples, shadowClipMoments);
		#endif
	#else

	
	vec4 shadowMoments, shadowClipMoments;

	FetchShadowMoments(shadowVert.xy / shadowVert.w, shadowMoments, shadowClipMoments);

	#endif

#else
	
	vec3 I = var_Position.xyz - u_LightOrigin;
	float ILen = length(I);
	float vertexDistance = ILen / u_LightRadius - SHADOW_BIAS;

#if 0
	gl_FragColor = vec4(u_ShadowTexelSize * u_ShadowBlur * ILen, 0.0, 0.0, 1.0);
	return;
#endif

	#if defined(r_PCFSamples)
		#if 0
		vec4 shadowMoments = PCSS(vec4(I, 0.0), r_PCFSamples);
		#else
		vec4 shadowClipMoments;
		vec4 shadowMoments = PCF(vec4(I, 0.0), u_ShadowTexelSize * u_ShadowBlur * ILen, r_PCFSamples, shadowClipMoments);
		#endif
	#else
	
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

#endif 

	
#if defined(LIGHT_DIRECTIONAL)
	vec3 L = u_LightDir;
#else
	vec3 L = normalize(u_LightOrigin - var_Position);
#endif

	vec2 texDiffuse = var_TexDiffuse.st;

#if defined(USE_NORMAL_MAPPING)

	mat3 tangentToWorldMatrix = mat3(var_Tangent.xyz, var_Binormal.xyz, var_Normal.xyz);

	vec2 texNormal = var_TexNormal.st;
	vec2 texSpecular = var_TexSpecular.st;

	
	vec3 V = normalize(u_ViewOrigin - var_Position.xyz);

#if defined(USE_PARALLAX_MAPPING)

	

	
	vec3 Vts = V * tangentToWorldMatrix;
	Vts = normalize(Vts);

	
	vec2 S = Vts.xy * -u_DepthScale / Vts.z;

	float depth = RayIntersectDisplaceMap(texNormal, S, u_NormalMap);

	
	vec2 texOffset = S * depth;

	texDiffuse.st += texOffset;
	texNormal.st += texOffset;
	texSpecular.st += texOffset;
#endif 

	
	vec3 H = normalize(L + V);

	
	vec3 N = texture2D(u_NormalMap, texNormal.st).xyw;
	N.x *= N.z;
	N.xy = 2.0 * N.xy - 1.0;
	N.z = sqrt(1.0 - dot(N.xy, N.xy));
	
	#if defined(r_NormalScale)
	N.z *= r_NormalScale;
	normalize(N);
	#endif

	
	N = normalize(tangentToWorldMatrix * N);


#else 

	vec3 N = normalize(var_Normal.xyz);

#endif 

	
#if defined(r_WrapAroundLighting)
	float NL = clamp(dot(N, L) + u_LightWrapAround, 0.0, 1.0) / clamp(1.0 + u_LightWrapAround, 0.0, 1.0);
#else
	float NL = clamp(dot(N, L), 0.0, 1.0);
#endif

	
	vec4 diffuse = texture2D(u_DiffuseMap, texDiffuse.st);
	if( abs(diffuse.a + u_AlphaThreshold) <= 1.0 )
	{
		discard;
		return;
	}
	diffuse.rgb *= u_LightColor * NL;

#if defined(USE_NORMAL_MAPPING)
	
	vec4 spec = texture2D(u_SpecularMap, texSpecular).rgba;
	vec3 specular = spec.rgb * u_LightColor * pow(clamp(dot(N, H), 0.0, 1.0), u_SpecularExponent.x * spec.a + u_SpecularExponent.y) * r_SpecularScale;
#endif



	
#if defined(LIGHT_PROJ)
	vec3 attenuationXY = texture2DProj(u_AttenuationMapXY, var_TexAttenuation.xyw).rgb;
	vec3 attenuationZ  = texture2D(u_AttenuationMapZ, vec2(var_TexAttenuation.z + 0.5, 0.0)).rgb; 

#elif defined(LIGHT_DIRECTIONAL)
	vec3 attenuationXY = vec3(1.0);
	vec3 attenuationZ  = vec3(1.0);

#else
	vec3 attenuationXY = texture2D(u_AttenuationMapXY, var_TexAttenuation.xy).rgb;
	vec3 attenuationZ  = texture2D(u_AttenuationMapZ, vec2(var_TexAttenuation.z, 0)).rgb;
#endif

	
	vec4 color = diffuse;

#if defined(USE_NORMAL_MAPPING)
	color.rgb += specular;
#endif

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

	gl_FragColor = color;

#if 0
#if defined(USE_PARALLAX_MAPPING)
	gl_FragColor = vec4(vec3(1.0, 0.0, 0.0), diffuse.a);
#elif defined(USE_NORMAL_MAPPING)
	gl_FragColor = vec4(vec3(0.0, 0.0, 1.0), diffuse.a);
#else
	gl_FragColor = vec4(vec3(0.0, 1.0, 0.0), diffuse.a);
#endif
#endif

#if 0
#if defined(USE_VERTEX_SKINNING)
	gl_FragColor = vec4(vec3(1.0, 0.0, 0.0), diffuse.a);
#elif defined(USE_VERTEX_ANIMATION)
	gl_FragColor = vec4(vec3(0.0, 0.0, 1.0), diffuse.a);
#else
	gl_FragColor = vec4(vec3(0.0, 1.0, 0.0), diffuse.a);
#endif
#endif
}
)" },
{ "glsl/forwardLighting_vp.glsl", R"(




uniform mat4		u_DiffuseTextureMatrix;
uniform mat4		u_NormalTextureMatrix;
uniform mat4		u_SpecularTextureMatrix;

uniform vec4		u_ColorModulate;
uniform vec4		u_Color;

uniform mat4		u_LightAttenuationMatrix;
uniform mat4		u_ModelMatrix;
uniform mat4		u_ModelViewProjectionMatrix;

uniform float		u_Time;

varying vec3		var_Position;
varying vec4		var_TexDiffuse;
varying vec4		var_TexNormal;
#if defined(USE_NORMAL_MAPPING)
varying vec2		var_TexSpecular;
#endif

varying vec4		var_TexAttenuation;

#if defined(USE_NORMAL_MAPPING)
varying vec4		var_Tangent;
varying vec4		var_Binormal;
#endif
varying vec4		var_Normal;


void DeformVertex( inout vec4 pos,
		   inout vec3 normal,
		   inout vec2 st,
		   inout vec4 color,
		   in    float time);

void	main()
{
	vec4 position;
	localBasis LB;
	vec2 texCoord, lmCoord;
	vec4 color;

	VertexFetch( position, LB, color, texCoord, lmCoord);

	
        color = color * u_ColorModulate + u_Color;

	DeformVertex( position,
		      LB.normal,
		      texCoord.st,
		      color,
		      u_Time);

	
	gl_Position = u_ModelViewProjectionMatrix * position;

	
	var_Position = (u_ModelMatrix * position).xyz;

#if defined(USE_NORMAL_MAPPING)
	var_Tangent.xyz = (u_ModelMatrix * vec4(LB.tangent, 0.0)).xyz;
	var_Binormal.xyz = (u_ModelMatrix * vec4(LB.binormal, 0.0)).xyz;
#endif

	var_Normal.xyz = mat3(u_ModelMatrix) * LB.normal;

	
	var_TexAttenuation = u_LightAttenuationMatrix * position;

	
	var_TexDiffuse.xy = (u_DiffuseTextureMatrix * vec4(texCoord, 0.0, 1.0)).st;

#if defined(USE_NORMAL_MAPPING)
	
	var_TexNormal.xy = (u_NormalTextureMatrix * vec4(texCoord, 0.0, 1.0)).st;

	
	var_TexSpecular = (u_SpecularTextureMatrix * vec4(texCoord, 0.0, 1.0)).st;
#endif

	var_TexDiffuse.p = color.r;
	var_TexNormal.pq = color.gb;
}
)" },
{ "glsl/fxaa3_11_fp.glsl", R"(



#define FXAA_PC 1
#if __VERSION__ == 120
    #define FXAA_GLSL_120 1
    #ifdef GL_EXT_gpu_shader4
        #extension GL_EXT_gpu_shader4 : enable
    #endif
#else
    #define FXAA_GLSL_130 1
#endif

#define FXAA_QUALITY_PRESET 12
#define FXAA_GREEN_AS_LUMA 1












#ifndef FXAA_PS3
    #define FXAA_PS3 0
#endif

#ifndef FXAA_360
    #define FXAA_360 0
#endif

#ifndef FXAA_360_OPT
    #define FXAA_360_OPT 0
#endif

#ifndef FXAA_PC
    
    
    
    
    #define FXAA_PC 0
#endif

#ifndef FXAA_PC_CONSOLE
    
    
    
    
    
    #define FXAA_PC_CONSOLE 0
#endif

#ifndef FXAA_GLSL_120
    #define FXAA_GLSL_120 0
#endif

#ifndef FXAA_GLSL_130
    #define FXAA_GLSL_130 0
#endif

#ifndef FXAA_HLSL_3
    #define FXAA_HLSL_3 0
#endif

#ifndef FXAA_HLSL_4
    #define FXAA_HLSL_4 0
#endif

#ifndef FXAA_HLSL_5
    #define FXAA_HLSL_5 0
#endif

#ifndef FXAA_GREEN_AS_LUMA
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    #define FXAA_GREEN_AS_LUMA 0
#endif

#ifndef FXAA_EARLY_EXIT
    
    
    
    
    
    
    
    
    
    
    #define FXAA_EARLY_EXIT 1
#endif

#ifndef FXAA_DISCARD
    
    
    
    
    
    
    
    
    #define FXAA_DISCARD 0
#endif

#ifndef FXAA_FAST_PIXEL_OFFSET
    
    
    
    
    
    
    #ifdef GL_EXT_gpu_shader4
        #define FXAA_FAST_PIXEL_OFFSET 1
        #extension GL_EXT_gpu_shader4 : enable
    #endif
    #ifdef GL_NV_gpu_shader5
        #define FXAA_FAST_PIXEL_OFFSET 1
        #extension GL_NV_gpu_shader5 : enable
    #endif
    #ifdef GL_ARB_gpu_shader5
        #define FXAA_FAST_PIXEL_OFFSET 1
        #extension GL_ARB_gpu_shader5 : enable
    #endif
    #ifndef FXAA_FAST_PIXEL_OFFSET
        #define FXAA_FAST_PIXEL_OFFSET 0
    #endif
#endif

#ifndef FXAA_GATHER4_ALPHA
    
    
    
    
    #if (FXAA_HLSL_5 == 1)
        #define FXAA_GATHER4_ALPHA 1
    #endif
    #ifdef GL_ARB_gpu_shader5
        #define FXAA_GATHER4_ALPHA 1
        #extension GL_ARB_gpu_shader5 : enable
    #endif
    #ifdef GL_NV_gpu_shader5
        #define FXAA_GATHER4_ALPHA 1
        #extension GL_NV_gpu_shader5 : enable
    #endif
    #ifndef FXAA_GATHER4_ALPHA
        #define FXAA_GATHER4_ALPHA 0
    #endif
#endif


#ifndef FXAA_CONSOLE_PS3_EDGE_SHARPNESS
    
    
    
    
    
    
    
    
    
    
    
    
    #if 1
        #define FXAA_CONSOLE_PS3_EDGE_SHARPNESS 8.0
    #endif
    #if 0
        #define FXAA_CONSOLE_PS3_EDGE_SHARPNESS 4.0
    #endif
    #if 0
        #define FXAA_CONSOLE_PS3_EDGE_SHARPNESS 2.0
    #endif
#endif

#ifndef FXAA_CONSOLE_PS3_EDGE_THRESHOLD
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    #if 1
        #define FXAA_CONSOLE_PS3_EDGE_THRESHOLD 0.125
    #else
        #define FXAA_CONSOLE_PS3_EDGE_THRESHOLD 0.25
    #endif
#endif


#ifndef FXAA_QUALITY_PRESET
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    #define FXAA_QUALITY_PRESET 12
#endif





#if (FXAA_QUALITY_PRESET == 10)
    #define FXAA_QUALITY_PS 3
    #define FXAA_QUALITY_P0 1.5
    #define FXAA_QUALITY_P1 3.0
    #define FXAA_QUALITY_P2 12.0
#endif

#if (FXAA_QUALITY_PRESET == 11)
    #define FXAA_QUALITY_PS 4
    #define FXAA_QUALITY_P0 1.0
    #define FXAA_QUALITY_P1 1.5
    #define FXAA_QUALITY_P2 3.0
    #define FXAA_QUALITY_P3 12.0
#endif

#if (FXAA_QUALITY_PRESET == 12)
    #define FXAA_QUALITY_PS 5
    #define FXAA_QUALITY_P0 1.0
    #define FXAA_QUALITY_P1 1.5
    #define FXAA_QUALITY_P2 2.0
    #define FXAA_QUALITY_P3 4.0
    #define FXAA_QUALITY_P4 12.0
#endif

#if (FXAA_QUALITY_PRESET == 13)
    #define FXAA_QUALITY_PS 6
    #define FXAA_QUALITY_P0 1.0
    #define FXAA_QUALITY_P1 1.5
    #define FXAA_QUALITY_P2 2.0
    #define FXAA_QUALITY_P3 2.0
    #define FXAA_QUALITY_P4 4.0
    #define FXAA_QUALITY_P5 12.0
#endif

#if (FXAA_QUALITY_PRESET == 14)
    #define FXAA_QUALITY_PS 7
    #define FXAA_QUALITY_P0 1.0
    #define FXAA_QUALITY_P1 1.5
    #define FXAA_QUALITY_P2 2.0
    #define FXAA_QUALITY_P3 2.0
    #define FXAA_QUALITY_P4 2.0
    #define FXAA_QUALITY_P5 4.0
    #define FXAA_QUALITY_P6 12.0
#endif

#if (FXAA_QUALITY_PRESET == 15)
    #define FXAA_QUALITY_PS 8
    #define FXAA_QUALITY_P0 1.0
    #define FXAA_QUALITY_P1 1.5
    #define FXAA_QUALITY_P2 2.0
    #define FXAA_QUALITY_P3 2.0
    #define FXAA_QUALITY_P4 2.0
    #define FXAA_QUALITY_P5 2.0
    #define FXAA_QUALITY_P6 4.0
    #define FXAA_QUALITY_P7 12.0
#endif


#if (FXAA_QUALITY_PRESET == 20)
    #define FXAA_QUALITY_PS 3
    #define FXAA_QUALITY_P0 1.5
    #define FXAA_QUALITY_P1 2.0
    #define FXAA_QUALITY_P2 8.0
#endif

#if (FXAA_QUALITY_PRESET == 21)
    #define FXAA_QUALITY_PS 4
    #define FXAA_QUALITY_P0 1.0
    #define FXAA_QUALITY_P1 1.5
    #define FXAA_QUALITY_P2 2.0
    #define FXAA_QUALITY_P3 8.0
#endif

#if (FXAA_QUALITY_PRESET == 22)
    #define FXAA_QUALITY_PS 5
    #define FXAA_QUALITY_P0 1.0
    #define FXAA_QUALITY_P1 1.5
    #define FXAA_QUALITY_P2 2.0
    #define FXAA_QUALITY_P3 2.0
    #define FXAA_QUALITY_P4 8.0
#endif

#if (FXAA_QUALITY_PRESET == 23)
    #define FXAA_QUALITY_PS 6
    #define FXAA_QUALITY_P0 1.0
    #define FXAA_QUALITY_P1 1.5
    #define FXAA_QUALITY_P2 2.0
    #define FXAA_QUALITY_P3 2.0
    #define FXAA_QUALITY_P4 2.0
    #define FXAA_QUALITY_P5 8.0
#endif

#if (FXAA_QUALITY_PRESET == 24)
    #define FXAA_QUALITY_PS 7
    #define FXAA_QUALITY_P0 1.0
    #define FXAA_QUALITY_P1 1.5
    #define FXAA_QUALITY_P2 2.0
    #define FXAA_QUALITY_P3 2.0
    #define FXAA_QUALITY_P4 2.0
    #define FXAA_QUALITY_P5 3.0
    #define FXAA_QUALITY_P6 8.0
#endif

#if (FXAA_QUALITY_PRESET == 25)
    #define FXAA_QUALITY_PS 8
    #define FXAA_QUALITY_P0 1.0
    #define FXAA_QUALITY_P1 1.5
    #define FXAA_QUALITY_P2 2.0
    #define FXAA_QUALITY_P3 2.0
    #define FXAA_QUALITY_P4 2.0
    #define FXAA_QUALITY_P5 2.0
    #define FXAA_QUALITY_P6 4.0
    #define FXAA_QUALITY_P7 8.0
#endif

#if (FXAA_QUALITY_PRESET == 26)
    #define FXAA_QUALITY_PS 9
    #define FXAA_QUALITY_P0 1.0
    #define FXAA_QUALITY_P1 1.5
    #define FXAA_QUALITY_P2 2.0
    #define FXAA_QUALITY_P3 2.0
    #define FXAA_QUALITY_P4 2.0
    #define FXAA_QUALITY_P5 2.0
    #define FXAA_QUALITY_P6 2.0
    #define FXAA_QUALITY_P7 4.0
    #define FXAA_QUALITY_P8 8.0
#endif

#if (FXAA_QUALITY_PRESET == 27)
    #define FXAA_QUALITY_PS 10
    #define FXAA_QUALITY_P0 1.0
    #define FXAA_QUALITY_P1 1.5
    #define FXAA_QUALITY_P2 2.0
    #define FXAA_QUALITY_P3 2.0
    #define FXAA_QUALITY_P4 2.0
    #define FXAA_QUALITY_P5 2.0
    #define FXAA_QUALITY_P6 2.0
    #define FXAA_QUALITY_P7 2.0
    #define FXAA_QUALITY_P8 4.0
    #define FXAA_QUALITY_P9 8.0
#endif

#if (FXAA_QUALITY_PRESET == 28)
    #define FXAA_QUALITY_PS 11
    #define FXAA_QUALITY_P0 1.0
    #define FXAA_QUALITY_P1 1.5
    #define FXAA_QUALITY_P2 2.0
    #define FXAA_QUALITY_P3 2.0
    #define FXAA_QUALITY_P4 2.0
    #define FXAA_QUALITY_P5 2.0
    #define FXAA_QUALITY_P6 2.0
    #define FXAA_QUALITY_P7 2.0
    #define FXAA_QUALITY_P8 2.0
    #define FXAA_QUALITY_P9 4.0
    #define FXAA_QUALITY_P10 8.0
#endif

#if (FXAA_QUALITY_PRESET == 29)
    #define FXAA_QUALITY_PS 12
    #define FXAA_QUALITY_P0 1.0
    #define FXAA_QUALITY_P1 1.5
    #define FXAA_QUALITY_P2 2.0
    #define FXAA_QUALITY_P3 2.0
    #define FXAA_QUALITY_P4 2.0
    #define FXAA_QUALITY_P5 2.0
    #define FXAA_QUALITY_P6 2.0
    #define FXAA_QUALITY_P7 2.0
    #define FXAA_QUALITY_P8 2.0
    #define FXAA_QUALITY_P9 2.0
    #define FXAA_QUALITY_P10 4.0
    #define FXAA_QUALITY_P11 8.0
#endif


#if (FXAA_QUALITY_PRESET == 39)
    #define FXAA_QUALITY_PS 12
    #define FXAA_QUALITY_P0 1.0
    #define FXAA_QUALITY_P1 1.0
    #define FXAA_QUALITY_P2 1.0
    #define FXAA_QUALITY_P3 1.0
    #define FXAA_QUALITY_P4 1.0
    #define FXAA_QUALITY_P5 1.5
    #define FXAA_QUALITY_P6 2.0
    #define FXAA_QUALITY_P7 2.0
    #define FXAA_QUALITY_P8 2.0
    #define FXAA_QUALITY_P9 2.0
    #define FXAA_QUALITY_P10 4.0
    #define FXAA_QUALITY_P11 8.0
#endif




#if (FXAA_GLSL_120 == 1) || (FXAA_GLSL_130 == 1)
    #define FxaaBool bool
    #define FxaaDiscard discard
    #define FxaaFloat float
    #define FxaaFloat2 vec2
    #define FxaaFloat3 vec3
    #define FxaaFloat4 vec4
    #define FxaaHalf float
    #define FxaaHalf2 vec2
    #define FxaaHalf3 vec3
    #define FxaaHalf4 vec4
    #define FxaaInt2 ivec2
    #define FxaaSat(x) clamp(x, 0.0, 1.0)
    #define FxaaTex sampler2D
#else
    #define FxaaBool bool
    #define FxaaDiscard clip(-1)
    #define FxaaFloat float
    #define FxaaFloat2 float2
    #define FxaaFloat3 float3
    #define FxaaFloat4 float4
    #define FxaaHalf half
    #define FxaaHalf2 half2
    #define FxaaHalf3 half3
    #define FxaaHalf4 half4
    #define FxaaSat(x) saturate(x)
#endif

#if (FXAA_GLSL_120 == 1)
    
    
    
    
    
    #ifdef GL_EXT_gpu_shader4
        #define FxaaTexTop(t, p) texture2DLod(t, p, 0.0)
        #if (FXAA_FAST_PIXEL_OFFSET == 1)
            #define FxaaTexOff(t, p, o, r) texture2DLodOffset(t, p, 0.0, o)
        #else
            #define FxaaTexOff(t, p, o, r) texture2DLod(t, p + (o * r), 0.0)
        #endif
    #else
        #define FxaaTexTop(t, p) texture2D(t, p)
        #define FxaaTexOff(t, p, o, r) texture2D(t, p + (o * r))
    #endif
    #if (FXAA_GATHER4_ALPHA == 1)
        
        #define FxaaTexAlpha4(t, p) textureGather(t, p, 3)
        #define FxaaTexOffAlpha4(t, p, o) textureGatherOffset(t, p, o, 3)
        #define FxaaTexGreen4(t, p) textureGather(t, p, 1)
        #define FxaaTexOffGreen4(t, p, o) textureGatherOffset(t, p, o, 1)
    #endif
#endif

#if (FXAA_GLSL_130 == 1)
    
    #define FxaaTexTop(t, p) textureLod(t, p, 0.0)
    #define FxaaTexOff(t, p, o, r) textureLodOffset(t, p, 0.0, o)
    #if (FXAA_GATHER4_ALPHA == 1)
        
        #define FxaaTexAlpha4(t, p) textureGather(t, p, 3)
        #define FxaaTexOffAlpha4(t, p, o) textureGatherOffset(t, p, o, 3)
        #define FxaaTexGreen4(t, p) textureGather(t, p, 1)
        #define FxaaTexOffGreen4(t, p, o) textureGatherOffset(t, p, o, 1)
    #endif
#endif

#if (FXAA_HLSL_3 == 1) || (FXAA_360 == 1) || (FXAA_PS3 == 1)
    #define FxaaInt2 float2
    #define FxaaTex sampler2D
    #define FxaaTexTop(t, p) tex2Dlod(t, float4(p, 0.0, 0.0))
    #define FxaaTexOff(t, p, o, r) tex2Dlod(t, float4(p + (o * r), 0, 0))
#endif

#if (FXAA_HLSL_4 == 1)
    #define FxaaInt2 int2
    struct FxaaTex { SamplerState smpl; Texture2D tex; };
    #define FxaaTexTop(t, p) t.tex.SampleLevel(t.smpl, p, 0.0)
    #define FxaaTexOff(t, p, o, r) t.tex.SampleLevel(t.smpl, p, 0.0, o)
#endif

#if (FXAA_HLSL_5 == 1)
    #define FxaaInt2 int2
    struct FxaaTex { SamplerState smpl; Texture2D tex; };
    #define FxaaTexTop(t, p) t.tex.SampleLevel(t.smpl, p, 0.0)
    #define FxaaTexOff(t, p, o, r) t.tex.SampleLevel(t.smpl, p, 0.0, o)
    #define FxaaTexAlpha4(t, p) t.tex.GatherAlpha(t.smpl, p)
    #define FxaaTexOffAlpha4(t, p, o) t.tex.GatherAlpha(t.smpl, p, o)
    #define FxaaTexGreen4(t, p) t.tex.GatherGreen(t.smpl, p)
    #define FxaaTexOffGreen4(t, p, o) t.tex.GatherGreen(t.smpl, p, o)
#endif



#if (FXAA_GREEN_AS_LUMA == 0)
    FxaaFloat FxaaLuma(FxaaFloat4 rgba) { return rgba.w; }
#else
    FxaaFloat FxaaLuma(FxaaFloat4 rgba) { return rgba.y; }
#endif





#if (FXAA_PC == 1)

FxaaFloat4 FxaaPixelShader(
    
    
    
    FxaaFloat2 pos,
    
    
    
    
    
    FxaaFloat4 fxaaConsolePosPos,
    
    
    
    
    
    FxaaTex tex,
    
    
    
    
    
    FxaaTex fxaaConsole360TexExpBiasNegOne,
    
    
    
    
    
    FxaaTex fxaaConsole360TexExpBiasNegTwo,
    
    
    
    
    
    FxaaFloat2 fxaaQualityRcpFrame,
    
    
    
    
    
    
    
    
    
    
    
    FxaaFloat4 fxaaConsoleRcpFrameOpt,
    
    
    
    
    
    
    
    
    FxaaFloat4 fxaaConsoleRcpFrameOpt2,
    
    
    
    
    
    
    
    
    FxaaFloat4 fxaaConsole360RcpFrameOpt2,
    
    
    
    
    
    
    
    
    
    
    
    FxaaFloat fxaaQualitySubpix,
    
    
    
    
    
    
    
    
    
    
    FxaaFloat fxaaQualityEdgeThreshold,
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    FxaaFloat fxaaQualityEdgeThresholdMin,
    
    
    
    
    
    
    
    
    
    
    
    
    
    FxaaFloat fxaaConsoleEdgeSharpness,
    
    
    
    
    
    
    
    
    
    
    
    
    
    FxaaFloat fxaaConsoleEdgeThreshold,
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    FxaaFloat fxaaConsoleEdgeThresholdMin,
    
    
    
    
    
    
    FxaaFloat4 fxaaConsole360ConstDir
) {

    FxaaFloat2 posM;
    posM.x = pos.x;
    posM.y = pos.y;
    #if (FXAA_GATHER4_ALPHA == 1)
        #if (FXAA_DISCARD == 0)
            FxaaFloat4 rgbyM = FxaaTexTop(tex, posM);
            #if (FXAA_GREEN_AS_LUMA == 0)
                #define lumaM rgbyM.w
            #else
                #define lumaM rgbyM.y
            #endif
        #endif
        #if (FXAA_GREEN_AS_LUMA == 0)
            FxaaFloat4 luma4A = FxaaTexAlpha4(tex, posM);
            FxaaFloat4 luma4B = FxaaTexOffAlpha4(tex, posM, FxaaInt2(-1, -1));
        #else
            FxaaFloat4 luma4A = FxaaTexGreen4(tex, posM);
            FxaaFloat4 luma4B = FxaaTexOffGreen4(tex, posM, FxaaInt2(-1, -1));
        #endif
        #if (FXAA_DISCARD == 1)
            #define lumaM luma4A.w
        #endif
        #define lumaE luma4A.z
        #define lumaS luma4A.x
        #define lumaSE luma4A.y
        #define lumaNW luma4B.w
        #define lumaN luma4B.z
        #define lumaW luma4B.x
    #else
        FxaaFloat4 rgbyM = FxaaTexTop(tex, posM);
        #if (FXAA_GREEN_AS_LUMA == 0)
            #define lumaM rgbyM.w
        #else
            #define lumaM rgbyM.y
        #endif
        FxaaFloat lumaS = FxaaLuma(FxaaTexOff(tex, posM, FxaaInt2( 0, 1), fxaaQualityRcpFrame.xy));
        FxaaFloat lumaE = FxaaLuma(FxaaTexOff(tex, posM, FxaaInt2( 1, 0), fxaaQualityRcpFrame.xy));
        FxaaFloat lumaN = FxaaLuma(FxaaTexOff(tex, posM, FxaaInt2( 0,-1), fxaaQualityRcpFrame.xy));
        FxaaFloat lumaW = FxaaLuma(FxaaTexOff(tex, posM, FxaaInt2(-1, 0), fxaaQualityRcpFrame.xy));
    #endif

    FxaaFloat maxSM = max(lumaS, lumaM);
    FxaaFloat minSM = min(lumaS, lumaM);
    FxaaFloat maxESM = max(lumaE, maxSM);
    FxaaFloat minESM = min(lumaE, minSM);
    FxaaFloat maxWN = max(lumaN, lumaW);
    FxaaFloat minWN = min(lumaN, lumaW);
    FxaaFloat rangeMax = max(maxWN, maxESM);
    FxaaFloat rangeMin = min(minWN, minESM);
    FxaaFloat rangeMaxScaled = rangeMax * fxaaQualityEdgeThreshold;
    FxaaFloat range = rangeMax - rangeMin;
    FxaaFloat rangeMaxClamped = max(fxaaQualityEdgeThresholdMin, rangeMaxScaled);
    FxaaBool earlyExit = range < rangeMaxClamped;

    if(earlyExit)
        #if (FXAA_DISCARD == 1)
            FxaaDiscard;
        #else
            return rgbyM;
        #endif

    #if (FXAA_GATHER4_ALPHA == 0)
        FxaaFloat lumaNW = FxaaLuma(FxaaTexOff(tex, posM, FxaaInt2(-1,-1), fxaaQualityRcpFrame.xy));
        FxaaFloat lumaSE = FxaaLuma(FxaaTexOff(tex, posM, FxaaInt2( 1, 1), fxaaQualityRcpFrame.xy));
        FxaaFloat lumaNE = FxaaLuma(FxaaTexOff(tex, posM, FxaaInt2( 1,-1), fxaaQualityRcpFrame.xy));
        FxaaFloat lumaSW = FxaaLuma(FxaaTexOff(tex, posM, FxaaInt2(-1, 1), fxaaQualityRcpFrame.xy));
    #else
        FxaaFloat lumaNE = FxaaLuma(FxaaTexOff(tex, posM, FxaaInt2(1, -1), fxaaQualityRcpFrame.xy));
        FxaaFloat lumaSW = FxaaLuma(FxaaTexOff(tex, posM, FxaaInt2(-1, 1), fxaaQualityRcpFrame.xy));
    #endif

    FxaaFloat lumaNS = lumaN + lumaS;
    FxaaFloat lumaWE = lumaW + lumaE;
    FxaaFloat subpixRcpRange = 1.0/range;
    FxaaFloat subpixNSWE = lumaNS + lumaWE;
    FxaaFloat edgeHorz1 = (-2.0 * lumaM) + lumaNS;
    FxaaFloat edgeVert1 = (-2.0 * lumaM) + lumaWE;

    FxaaFloat lumaNESE = lumaNE + lumaSE;
    FxaaFloat lumaNWNE = lumaNW + lumaNE;
    FxaaFloat edgeHorz2 = (-2.0 * lumaE) + lumaNESE;
    FxaaFloat edgeVert2 = (-2.0 * lumaN) + lumaNWNE;

    FxaaFloat lumaNWSW = lumaNW + lumaSW;
    FxaaFloat lumaSWSE = lumaSW + lumaSE;
    FxaaFloat edgeHorz4 = (abs(edgeHorz1) * 2.0) + abs(edgeHorz2);
    FxaaFloat edgeVert4 = (abs(edgeVert1) * 2.0) + abs(edgeVert2);
    FxaaFloat edgeHorz3 = (-2.0 * lumaW) + lumaNWSW;
    FxaaFloat edgeVert3 = (-2.0 * lumaS) + lumaSWSE;
    FxaaFloat edgeHorz = abs(edgeHorz3) + edgeHorz4;
    FxaaFloat edgeVert = abs(edgeVert3) + edgeVert4;

    FxaaFloat subpixNWSWNESE = lumaNWSW + lumaNESE;
    FxaaFloat lengthSign = fxaaQualityRcpFrame.x;
    FxaaBool horzSpan = edgeHorz >= edgeVert;
    FxaaFloat subpixA = subpixNSWE * 2.0 + subpixNWSWNESE;

    if(!horzSpan) lumaN = lumaW;
    if(!horzSpan) lumaS = lumaE;
    if(horzSpan) lengthSign = fxaaQualityRcpFrame.y;
    FxaaFloat subpixB = (subpixA * (1.0/12.0)) - lumaM;

    FxaaFloat gradientN = lumaN - lumaM;
    FxaaFloat gradientS = lumaS - lumaM;
    FxaaFloat lumaNN = lumaN + lumaM;
    FxaaFloat lumaSS = lumaS + lumaM;
    FxaaBool pairN = abs(gradientN) >= abs(gradientS);
    FxaaFloat gradient = max(abs(gradientN), abs(gradientS));
    if(pairN) lengthSign = -lengthSign;
    FxaaFloat subpixC = FxaaSat(abs(subpixB) * subpixRcpRange);

    FxaaFloat2 posB;
    posB.x = posM.x;
    posB.y = posM.y;
    FxaaFloat2 offNP;
    offNP.x = (!horzSpan) ? 0.0 : fxaaQualityRcpFrame.x;
    offNP.y = ( horzSpan) ? 0.0 : fxaaQualityRcpFrame.y;
    if(!horzSpan) posB.x += lengthSign * 0.5;
    if( horzSpan) posB.y += lengthSign * 0.5;

    FxaaFloat2 posN;
    posN.x = posB.x - offNP.x * FXAA_QUALITY_P0;
    posN.y = posB.y - offNP.y * FXAA_QUALITY_P0;
    FxaaFloat2 posP;
    posP.x = posB.x + offNP.x * FXAA_QUALITY_P0;
    posP.y = posB.y + offNP.y * FXAA_QUALITY_P0;
    FxaaFloat subpixD = ((-2.0)*subpixC) + 3.0;
    FxaaFloat lumaEndN = FxaaLuma(FxaaTexTop(tex, posN));
    FxaaFloat subpixE = subpixC * subpixC;
    FxaaFloat lumaEndP = FxaaLuma(FxaaTexTop(tex, posP));

    if(!pairN) lumaNN = lumaSS;
    FxaaFloat gradientScaled = gradient * 1.0/4.0;
    FxaaFloat lumaMM = lumaM - lumaNN * 0.5;
    FxaaFloat subpixF = subpixD * subpixE;
    FxaaBool lumaMLTZero = lumaMM < 0.0;

    lumaEndN -= lumaNN * 0.5;
    lumaEndP -= lumaNN * 0.5;
    FxaaBool doneN = abs(lumaEndN) >= gradientScaled;
    FxaaBool doneP = abs(lumaEndP) >= gradientScaled;
    if(!doneN) posN.x -= offNP.x * FXAA_QUALITY_P1;
    if(!doneN) posN.y -= offNP.y * FXAA_QUALITY_P1;
    FxaaBool doneNP = (!doneN) || (!doneP);
    if(!doneP) posP.x += offNP.x * FXAA_QUALITY_P1;
    if(!doneP) posP.y += offNP.y * FXAA_QUALITY_P1;

    if(doneNP) {
        if(!doneN) lumaEndN = FxaaLuma(FxaaTexTop(tex, posN.xy));
        if(!doneP) lumaEndP = FxaaLuma(FxaaTexTop(tex, posP.xy));
        if(!doneN) lumaEndN = lumaEndN - lumaNN * 0.5;
        if(!doneP) lumaEndP = lumaEndP - lumaNN * 0.5;
        doneN = abs(lumaEndN) >= gradientScaled;
        doneP = abs(lumaEndP) >= gradientScaled;
        if(!doneN) posN.x -= offNP.x * FXAA_QUALITY_P2;
        if(!doneN) posN.y -= offNP.y * FXAA_QUALITY_P2;
        doneNP = (!doneN) || (!doneP);
        if(!doneP) posP.x += offNP.x * FXAA_QUALITY_P2;
        if(!doneP) posP.y += offNP.y * FXAA_QUALITY_P2;

        #if (FXAA_QUALITY_PS > 3)
        if(doneNP) {
            if(!doneN) lumaEndN = FxaaLuma(FxaaTexTop(tex, posN.xy));
            if(!doneP) lumaEndP = FxaaLuma(FxaaTexTop(tex, posP.xy));
            if(!doneN) lumaEndN = lumaEndN - lumaNN * 0.5;
            if(!doneP) lumaEndP = lumaEndP - lumaNN * 0.5;
            doneN = abs(lumaEndN) >= gradientScaled;
            doneP = abs(lumaEndP) >= gradientScaled;
            if(!doneN) posN.x -= offNP.x * FXAA_QUALITY_P3;
            if(!doneN) posN.y -= offNP.y * FXAA_QUALITY_P3;
            doneNP = (!doneN) || (!doneP);
            if(!doneP) posP.x += offNP.x * FXAA_QUALITY_P3;
            if(!doneP) posP.y += offNP.y * FXAA_QUALITY_P3;

            #if (FXAA_QUALITY_PS > 4)
            if(doneNP) {
                if(!doneN) lumaEndN = FxaaLuma(FxaaTexTop(tex, posN.xy));
                if(!doneP) lumaEndP = FxaaLuma(FxaaTexTop(tex, posP.xy));
                if(!doneN) lumaEndN = lumaEndN - lumaNN * 0.5;
                if(!doneP) lumaEndP = lumaEndP - lumaNN * 0.5;
                doneN = abs(lumaEndN) >= gradientScaled;
                doneP = abs(lumaEndP) >= gradientScaled;
                if(!doneN) posN.x -= offNP.x * FXAA_QUALITY_P4;
                if(!doneN) posN.y -= offNP.y * FXAA_QUALITY_P4;
                doneNP = (!doneN) || (!doneP);
                if(!doneP) posP.x += offNP.x * FXAA_QUALITY_P4;
                if(!doneP) posP.y += offNP.y * FXAA_QUALITY_P4;

                #if (FXAA_QUALITY_PS > 5)
                if(doneNP) {
                    if(!doneN) lumaEndN = FxaaLuma(FxaaTexTop(tex, posN.xy));
                    if(!doneP) lumaEndP = FxaaLuma(FxaaTexTop(tex, posP.xy));
                    if(!doneN) lumaEndN = lumaEndN - lumaNN * 0.5;
                    if(!doneP) lumaEndP = lumaEndP - lumaNN * 0.5;
                    doneN = abs(lumaEndN) >= gradientScaled;
                    doneP = abs(lumaEndP) >= gradientScaled;
                    if(!doneN) posN.x -= offNP.x * FXAA_QUALITY_P5;
                    if(!doneN) posN.y -= offNP.y * FXAA_QUALITY_P5;
                    doneNP = (!doneN) || (!doneP);
                    if(!doneP) posP.x += offNP.x * FXAA_QUALITY_P5;
                    if(!doneP) posP.y += offNP.y * FXAA_QUALITY_P5;

                    #if (FXAA_QUALITY_PS > 6)
                    if(doneNP) {
                        if(!doneN) lumaEndN = FxaaLuma(FxaaTexTop(tex, posN.xy));
                        if(!doneP) lumaEndP = FxaaLuma(FxaaTexTop(tex, posP.xy));
                        if(!doneN) lumaEndN = lumaEndN - lumaNN * 0.5;
                        if(!doneP) lumaEndP = lumaEndP - lumaNN * 0.5;
                        doneN = abs(lumaEndN) >= gradientScaled;
                        doneP = abs(lumaEndP) >= gradientScaled;
                        if(!doneN) posN.x -= offNP.x * FXAA_QUALITY_P6;
                        if(!doneN) posN.y -= offNP.y * FXAA_QUALITY_P6;
                        doneNP = (!doneN) || (!doneP);
                        if(!doneP) posP.x += offNP.x * FXAA_QUALITY_P6;
                        if(!doneP) posP.y += offNP.y * FXAA_QUALITY_P6;

                        #if (FXAA_QUALITY_PS > 7)
                        if(doneNP) {
                            if(!doneN) lumaEndN = FxaaLuma(FxaaTexTop(tex, posN.xy));
                            if(!doneP) lumaEndP = FxaaLuma(FxaaTexTop(tex, posP.xy));
                            if(!doneN) lumaEndN = lumaEndN - lumaNN * 0.5;
                            if(!doneP) lumaEndP = lumaEndP - lumaNN * 0.5;
                            doneN = abs(lumaEndN) >= gradientScaled;
                            doneP = abs(lumaEndP) >= gradientScaled;
                            if(!doneN) posN.x -= offNP.x * FXAA_QUALITY_P7;
                            if(!doneN) posN.y -= offNP.y * FXAA_QUALITY_P7;
                            doneNP = (!doneN) || (!doneP);
                            if(!doneP) posP.x += offNP.x * FXAA_QUALITY_P7;
                            if(!doneP) posP.y += offNP.y * FXAA_QUALITY_P7;

    #if (FXAA_QUALITY_PS > 8)
    if(doneNP) {
        if(!doneN) lumaEndN = FxaaLuma(FxaaTexTop(tex, posN.xy));
        if(!doneP) lumaEndP = FxaaLuma(FxaaTexTop(tex, posP.xy));
        if(!doneN) lumaEndN = lumaEndN - lumaNN * 0.5;
        if(!doneP) lumaEndP = lumaEndP - lumaNN * 0.5;
        doneN = abs(lumaEndN) >= gradientScaled;
        doneP = abs(lumaEndP) >= gradientScaled;
        if(!doneN) posN.x -= offNP.x * FXAA_QUALITY_P8;
        if(!doneN) posN.y -= offNP.y * FXAA_QUALITY_P8;
        doneNP = (!doneN) || (!doneP);
        if(!doneP) posP.x += offNP.x * FXAA_QUALITY_P8;
        if(!doneP) posP.y += offNP.y * FXAA_QUALITY_P8;

        #if (FXAA_QUALITY_PS > 9)
        if(doneNP) {
            if(!doneN) lumaEndN = FxaaLuma(FxaaTexTop(tex, posN.xy));
            if(!doneP) lumaEndP = FxaaLuma(FxaaTexTop(tex, posP.xy));
            if(!doneN) lumaEndN = lumaEndN - lumaNN * 0.5;
            if(!doneP) lumaEndP = lumaEndP - lumaNN * 0.5;
            doneN = abs(lumaEndN) >= gradientScaled;
            doneP = abs(lumaEndP) >= gradientScaled;
            if(!doneN) posN.x -= offNP.x * FXAA_QUALITY_P9;
            if(!doneN) posN.y -= offNP.y * FXAA_QUALITY_P9;
            doneNP = (!doneN) || (!doneP);
            if(!doneP) posP.x += offNP.x * FXAA_QUALITY_P9;
            if(!doneP) posP.y += offNP.y * FXAA_QUALITY_P9;

            #if (FXAA_QUALITY_PS > 10)
            if(doneNP) {
                if(!doneN) lumaEndN = FxaaLuma(FxaaTexTop(tex, posN.xy));
                if(!doneP) lumaEndP = FxaaLuma(FxaaTexTop(tex, posP.xy));
                if(!doneN) lumaEndN = lumaEndN - lumaNN * 0.5;
                if(!doneP) lumaEndP = lumaEndP - lumaNN * 0.5;
                doneN = abs(lumaEndN) >= gradientScaled;
                doneP = abs(lumaEndP) >= gradientScaled;
                if(!doneN) posN.x -= offNP.x * FXAA_QUALITY_P10;
                if(!doneN) posN.y -= offNP.y * FXAA_QUALITY_P10;
                doneNP = (!doneN) || (!doneP);
                if(!doneP) posP.x += offNP.x * FXAA_QUALITY_P10;
                if(!doneP) posP.y += offNP.y * FXAA_QUALITY_P10;

                #if (FXAA_QUALITY_PS > 11)
                if(doneNP) {
                    if(!doneN) lumaEndN = FxaaLuma(FxaaTexTop(tex, posN.xy));
                    if(!doneP) lumaEndP = FxaaLuma(FxaaTexTop(tex, posP.xy));
                    if(!doneN) lumaEndN = lumaEndN - lumaNN * 0.5;
                    if(!doneP) lumaEndP = lumaEndP - lumaNN * 0.5;
                    doneN = abs(lumaEndN) >= gradientScaled;
                    doneP = abs(lumaEndP) >= gradientScaled;
                    if(!doneN) posN.x -= offNP.x * FXAA_QUALITY_P11;
                    if(!doneN) posN.y -= offNP.y * FXAA_QUALITY_P11;
                    doneNP = (!doneN) || (!doneP);
                    if(!doneP) posP.x += offNP.x * FXAA_QUALITY_P11;
                    if(!doneP) posP.y += offNP.y * FXAA_QUALITY_P11;

                    #if (FXAA_QUALITY_PS > 12)
                    if(doneNP) {
                        if(!doneN) lumaEndN = FxaaLuma(FxaaTexTop(tex, posN.xy));
                        if(!doneP) lumaEndP = FxaaLuma(FxaaTexTop(tex, posP.xy));
                        if(!doneN) lumaEndN = lumaEndN - lumaNN * 0.5;
                        if(!doneP) lumaEndP = lumaEndP - lumaNN * 0.5;
                        doneN = abs(lumaEndN) >= gradientScaled;
                        doneP = abs(lumaEndP) >= gradientScaled;
                        if(!doneN) posN.x -= offNP.x * FXAA_QUALITY_P12;
                        if(!doneN) posN.y -= offNP.y * FXAA_QUALITY_P12;
                        doneNP = (!doneN) || (!doneP);
                        if(!doneP) posP.x += offNP.x * FXAA_QUALITY_P12;
                        if(!doneP) posP.y += offNP.y * FXAA_QUALITY_P12;

                    }
                    #endif

                }
                #endif

            }
            #endif

        }
        #endif

    }
    #endif

                        }
                        #endif

                    }
                    #endif

                }
                #endif

            }
            #endif

        }
        #endif

    }

    FxaaFloat dstN = posM.x - posN.x;
    FxaaFloat dstP = posP.x - posM.x;
    if(!horzSpan) dstN = posM.y - posN.y;
    if(!horzSpan) dstP = posP.y - posM.y;

    FxaaBool goodSpanN = (lumaEndN < 0.0) != lumaMLTZero;
    FxaaFloat spanLength = (dstP + dstN);
    FxaaBool goodSpanP = (lumaEndP < 0.0) != lumaMLTZero;
    FxaaFloat spanLengthRcp = 1.0/spanLength;

    FxaaBool directionN = dstN < dstP;
    FxaaFloat dst = min(dstN, dstP);
    FxaaBool goodSpan = directionN ? goodSpanN : goodSpanP;
    FxaaFloat subpixG = subpixF * subpixF;
    FxaaFloat pixelOffset = (dst * (-spanLengthRcp)) + 0.5;
    FxaaFloat subpixH = subpixG * fxaaQualitySubpix;

    FxaaFloat pixelOffsetGood = goodSpan ? pixelOffset : 0.0;
    FxaaFloat pixelOffsetSubpix = max(pixelOffsetGood, subpixH);
    if(!horzSpan) posM.x += pixelOffsetSubpix * lengthSign;
    if( horzSpan) posM.y += pixelOffsetSubpix * lengthSign;
    #if (FXAA_DISCARD == 1)
        return FxaaTexTop(tex, posM);
    #else
        return FxaaFloat4(FxaaTexTop(tex, posM).xyz, lumaM);
    #endif
}

#endif





#if (FXAA_PC_CONSOLE == 1)

FxaaFloat4 FxaaPixelShader(
    
    FxaaFloat2 pos,
    FxaaFloat4 fxaaConsolePosPos,
    FxaaTex tex,
    FxaaTex fxaaConsole360TexExpBiasNegOne,
    FxaaTex fxaaConsole360TexExpBiasNegTwo,
    FxaaFloat2 fxaaQualityRcpFrame,
    FxaaFloat4 fxaaConsoleRcpFrameOpt,
    FxaaFloat4 fxaaConsoleRcpFrameOpt2,
    FxaaFloat4 fxaaConsole360RcpFrameOpt2,
    FxaaFloat fxaaQualitySubpix,
    FxaaFloat fxaaQualityEdgeThreshold,
    FxaaFloat fxaaQualityEdgeThresholdMin,
    FxaaFloat fxaaConsoleEdgeSharpness,
    FxaaFloat fxaaConsoleEdgeThreshold,
    FxaaFloat fxaaConsoleEdgeThresholdMin,
    FxaaFloat4 fxaaConsole360ConstDir
) {

    FxaaFloat lumaNw = FxaaLuma(FxaaTexTop(tex, fxaaConsolePosPos.xy));
    FxaaFloat lumaSw = FxaaLuma(FxaaTexTop(tex, fxaaConsolePosPos.xw));
    FxaaFloat lumaNe = FxaaLuma(FxaaTexTop(tex, fxaaConsolePosPos.zy));
    FxaaFloat lumaSe = FxaaLuma(FxaaTexTop(tex, fxaaConsolePosPos.zw));

    FxaaFloat4 rgbyM = FxaaTexTop(tex, pos.xy);
    #if (FXAA_GREEN_AS_LUMA == 0)
        FxaaFloat lumaM = rgbyM.w;
    #else
        FxaaFloat lumaM = rgbyM.y;
    #endif

    FxaaFloat lumaMaxNwSw = max(lumaNw, lumaSw);
    lumaNe += 1.0/384.0;
    FxaaFloat lumaMinNwSw = min(lumaNw, lumaSw);

    FxaaFloat lumaMaxNeSe = max(lumaNe, lumaSe);
    FxaaFloat lumaMinNeSe = min(lumaNe, lumaSe);

    FxaaFloat lumaMax = max(lumaMaxNeSe, lumaMaxNwSw);
    FxaaFloat lumaMin = min(lumaMinNeSe, lumaMinNwSw);

    FxaaFloat lumaMaxScaled = lumaMax * fxaaConsoleEdgeThreshold;

    FxaaFloat lumaMinM = min(lumaMin, lumaM);
    FxaaFloat lumaMaxScaledClamped = max(fxaaConsoleEdgeThresholdMin, lumaMaxScaled);
    FxaaFloat lumaMaxM = max(lumaMax, lumaM);
    FxaaFloat dirSwMinusNe = lumaSw - lumaNe;
    FxaaFloat lumaMaxSubMinM = lumaMaxM - lumaMinM;
    FxaaFloat dirSeMinusNw = lumaSe - lumaNw;
    if(lumaMaxSubMinM < lumaMaxScaledClamped) return rgbyM;

    FxaaFloat2 dir;
    dir.x = dirSwMinusNe + dirSeMinusNw;
    dir.y = dirSwMinusNe - dirSeMinusNw;

    FxaaFloat2 dir1 = normalize(dir.xy);
    FxaaFloat4 rgbyN1 = FxaaTexTop(tex, pos.xy - dir1 * fxaaConsoleRcpFrameOpt.zw);
    FxaaFloat4 rgbyP1 = FxaaTexTop(tex, pos.xy + dir1 * fxaaConsoleRcpFrameOpt.zw);

    FxaaFloat dirAbsMinTimesC = min(abs(dir1.x), abs(dir1.y)) * fxaaConsoleEdgeSharpness;
    FxaaFloat2 dir2 = clamp(dir1.xy / dirAbsMinTimesC, -2.0, 2.0);

    FxaaFloat4 rgbyN2 = FxaaTexTop(tex, pos.xy - dir2 * fxaaConsoleRcpFrameOpt2.zw);
    FxaaFloat4 rgbyP2 = FxaaTexTop(tex, pos.xy + dir2 * fxaaConsoleRcpFrameOpt2.zw);

    FxaaFloat4 rgbyA = rgbyN1 + rgbyP1;
    FxaaFloat4 rgbyB = ((rgbyN2 + rgbyP2) * 0.25) + (rgbyA * 0.25);

    #if (FXAA_GREEN_AS_LUMA == 0)
        FxaaBool twoTap = (rgbyB.w < lumaMin) || (rgbyB.w > lumaMax);
    #else
        FxaaBool twoTap = (rgbyB.y < lumaMin) || (rgbyB.y > lumaMax);
    #endif
    if(twoTap) rgbyB.xyz = rgbyA.xyz * 0.5;
    return rgbyB; }

#endif




#if (FXAA_360 == 1)

[reduceTempRegUsage(4)]
float4 FxaaPixelShader(
    
    FxaaFloat2 pos,
    FxaaFloat4 fxaaConsolePosPos,
    FxaaTex tex,
    FxaaTex fxaaConsole360TexExpBiasNegOne,
    FxaaTex fxaaConsole360TexExpBiasNegTwo,
    FxaaFloat2 fxaaQualityRcpFrame,
    FxaaFloat4 fxaaConsoleRcpFrameOpt,
    FxaaFloat4 fxaaConsoleRcpFrameOpt2,
    FxaaFloat4 fxaaConsole360RcpFrameOpt2,
    FxaaFloat fxaaQualitySubpix,
    FxaaFloat fxaaQualityEdgeThreshold,
    FxaaFloat fxaaQualityEdgeThresholdMin,
    FxaaFloat fxaaConsoleEdgeSharpness,
    FxaaFloat fxaaConsoleEdgeThreshold,
    FxaaFloat fxaaConsoleEdgeThresholdMin,
    FxaaFloat4 fxaaConsole360ConstDir
) {

    float4 lumaNwNeSwSe;
    #if (FXAA_GREEN_AS_LUMA == 0)
        asm {
            tfetch2D lumaNwNeSwSe.w___, tex, pos.xy, OffsetX = -0.5, OffsetY = -0.5, UseComputedLOD=false
            tfetch2D lumaNwNeSwSe._w__, tex, pos.xy, OffsetX =  0.5, OffsetY = -0.5, UseComputedLOD=false
            tfetch2D lumaNwNeSwSe.__w_, tex, pos.xy, OffsetX = -0.5, OffsetY =  0.5, UseComputedLOD=false
            tfetch2D lumaNwNeSwSe.___w, tex, pos.xy, OffsetX =  0.5, OffsetY =  0.5, UseComputedLOD=false
        };
    #else
        asm {
            tfetch2D lumaNwNeSwSe.y___, tex, pos.xy, OffsetX = -0.5, OffsetY = -0.5, UseComputedLOD=false
            tfetch2D lumaNwNeSwSe._y__, tex, pos.xy, OffsetX =  0.5, OffsetY = -0.5, UseComputedLOD=false
            tfetch2D lumaNwNeSwSe.__y_, tex, pos.xy, OffsetX = -0.5, OffsetY =  0.5, UseComputedLOD=false
            tfetch2D lumaNwNeSwSe.___y, tex, pos.xy, OffsetX =  0.5, OffsetY =  0.5, UseComputedLOD=false
        };
    #endif

    lumaNwNeSwSe.y += 1.0/384.0;
    float2 lumaMinTemp = min(lumaNwNeSwSe.xy, lumaNwNeSwSe.zw);
    float2 lumaMaxTemp = max(lumaNwNeSwSe.xy, lumaNwNeSwSe.zw);
    float lumaMin = min(lumaMinTemp.x, lumaMinTemp.y);
    float lumaMax = max(lumaMaxTemp.x, lumaMaxTemp.y);

    float4 rgbyM = tex2Dlod(tex, float4(pos.xy, 0.0, 0.0));
    #if (FXAA_GREEN_AS_LUMA == 0)
        float lumaMinM = min(lumaMin, rgbyM.w);
        float lumaMaxM = max(lumaMax, rgbyM.w);
    #else
        float lumaMinM = min(lumaMin, rgbyM.y);
        float lumaMaxM = max(lumaMax, rgbyM.y);
    #endif
    if((lumaMaxM - lumaMinM) < max(fxaaConsoleEdgeThresholdMin, lumaMax * fxaaConsoleEdgeThreshold)) return rgbyM;

    float2 dir;
    dir.x = dot(lumaNwNeSwSe, fxaaConsole360ConstDir.yyxx);
    dir.y = dot(lumaNwNeSwSe, fxaaConsole360ConstDir.xyxy);
    dir = normalize(dir);

    float4 dir1 = dir.xyxy * fxaaConsoleRcpFrameOpt.xyzw;

    float4 dir2;
    float dirAbsMinTimesC = min(abs(dir.x), abs(dir.y)) * fxaaConsoleEdgeSharpness;
    dir2 = saturate(fxaaConsole360ConstDir.zzww * dir.xyxy / dirAbsMinTimesC + 0.5);
    dir2 = dir2 * fxaaConsole360RcpFrameOpt2.xyxy + fxaaConsole360RcpFrameOpt2.zwzw;

    float4 rgbyN1 = tex2Dlod(fxaaConsole360TexExpBiasNegOne, float4(pos.xy + dir1.xy, 0.0, 0.0));
    float4 rgbyP1 = tex2Dlod(fxaaConsole360TexExpBiasNegOne, float4(pos.xy + dir1.zw, 0.0, 0.0));
    float4 rgbyN2 = tex2Dlod(fxaaConsole360TexExpBiasNegTwo, float4(pos.xy + dir2.xy, 0.0, 0.0));
    float4 rgbyP2 = tex2Dlod(fxaaConsole360TexExpBiasNegTwo, float4(pos.xy + dir2.zw, 0.0, 0.0));

    float4 rgbyA = rgbyN1 + rgbyP1;
    float4 rgbyB = rgbyN2 + rgbyP2 + rgbyA * 0.5;

    float4 rgbyR = ((FxaaLuma(rgbyB) - lumaMax) > 0.0) ? rgbyA : rgbyB;
    rgbyR = ((FxaaLuma(rgbyB) - lumaMin) > 0.0) ? rgbyR : rgbyA;
    return rgbyR; }

#endif




#if (FXAA_PS3 == 1) && (FXAA_EARLY_EXIT == 0)

#pragma regcount 7
#pragma disablepc all
#pragma option O3
#pragma option OutColorPrec=fp16
#pragma texformat default RGBA8

half4 FxaaPixelShader(
    
    FxaaFloat2 pos,
    FxaaFloat4 fxaaConsolePosPos,
    FxaaTex tex,
    FxaaTex fxaaConsole360TexExpBiasNegOne,
    FxaaTex fxaaConsole360TexExpBiasNegTwo,
    FxaaFloat2 fxaaQualityRcpFrame,
    FxaaFloat4 fxaaConsoleRcpFrameOpt,
    FxaaFloat4 fxaaConsoleRcpFrameOpt2,
    FxaaFloat4 fxaaConsole360RcpFrameOpt2,
    FxaaFloat fxaaQualitySubpix,
    FxaaFloat fxaaQualityEdgeThreshold,
    FxaaFloat fxaaQualityEdgeThresholdMin,
    FxaaFloat fxaaConsoleEdgeSharpness,
    FxaaFloat fxaaConsoleEdgeThreshold,
    FxaaFloat fxaaConsoleEdgeThresholdMin,
    FxaaFloat4 fxaaConsole360ConstDir
) {


    half4 dir;
    half4 lumaNe = h4tex2Dlod(tex, half4(fxaaConsolePosPos.zy, 0, 0));
    #if (FXAA_GREEN_AS_LUMA == 0)
        lumaNe.w += half(1.0/512.0);
        dir.x = -lumaNe.w;
        dir.z = -lumaNe.w;
    #else
        lumaNe.y += half(1.0/512.0);
        dir.x = -lumaNe.y;
        dir.z = -lumaNe.y;
    #endif


    half4 lumaSw = h4tex2Dlod(tex, half4(fxaaConsolePosPos.xw, 0, 0));
    #if (FXAA_GREEN_AS_LUMA == 0)
        dir.x += lumaSw.w;
        dir.z += lumaSw.w;
    #else
        dir.x += lumaSw.y;
        dir.z += lumaSw.y;
    #endif


    half4 lumaNw = h4tex2Dlod(tex, half4(fxaaConsolePosPos.xy, 0, 0));
    #if (FXAA_GREEN_AS_LUMA == 0)
        dir.x -= lumaNw.w;
        dir.z += lumaNw.w;
    #else
        dir.x -= lumaNw.y;
        dir.z += lumaNw.y;
    #endif


    half4 lumaSe = h4tex2Dlod(tex, half4(fxaaConsolePosPos.zw, 0, 0));
    #if (FXAA_GREEN_AS_LUMA == 0)
        dir.x += lumaSe.w;
        dir.z -= lumaSe.w;
    #else
        dir.x += lumaSe.y;
        dir.z -= lumaSe.y;
    #endif


    half4 dir1_pos;
    dir1_pos.xy = normalize(dir.xyz).xz;
    half dirAbsMinTimesC = min(abs(dir1_pos.x), abs(dir1_pos.y)) * half(FXAA_CONSOLE_PS3_EDGE_SHARPNESS);


    half4 dir2_pos;
    dir2_pos.xy = clamp(dir1_pos.xy / dirAbsMinTimesC, half(-2.0), half(2.0));
    dir1_pos.zw = pos.xy;
    dir2_pos.zw = pos.xy;
    half4 temp1N;
    temp1N.xy = dir1_pos.zw - dir1_pos.xy * fxaaConsoleRcpFrameOpt.zw;


    temp1N = h4tex2Dlod(tex, half4(temp1N.xy, 0.0, 0.0));
    half4 rgby1;
    rgby1.xy = dir1_pos.zw + dir1_pos.xy * fxaaConsoleRcpFrameOpt.zw;


    rgby1 = h4tex2Dlod(tex, half4(rgby1.xy, 0.0, 0.0));
    rgby1 = (temp1N + rgby1) * 0.5;


    half4 temp2N;
    temp2N.xy = dir2_pos.zw - dir2_pos.xy * fxaaConsoleRcpFrameOpt2.zw;
    temp2N = h4tex2Dlod(tex, half4(temp2N.xy, 0.0, 0.0));


    half4 rgby2;
    rgby2.xy = dir2_pos.zw + dir2_pos.xy * fxaaConsoleRcpFrameOpt2.zw;
    rgby2 = h4tex2Dlod(tex, half4(rgby2.xy, 0.0, 0.0));
    rgby2 = (temp2N + rgby2) * 0.5;


    
    #if (FXAA_GREEN_AS_LUMA == 0)
        half lumaMin = min(min(lumaNw.w, lumaSw.w), min(lumaNe.w, lumaSe.w));
        half lumaMax = max(max(lumaNw.w, lumaSw.w), max(lumaNe.w, lumaSe.w));
    #else
        half lumaMin = min(min(lumaNw.y, lumaSw.y), min(lumaNe.y, lumaSe.y));
        half lumaMax = max(max(lumaNw.y, lumaSw.y), max(lumaNe.y, lumaSe.y));
    #endif
    rgby2 = (rgby2 + rgby1) * 0.5;


    #if (FXAA_GREEN_AS_LUMA == 0)
        bool twoTapLt = rgby2.w < lumaMin;
        bool twoTapGt = rgby2.w > lumaMax;
    #else
        bool twoTapLt = rgby2.y < lumaMin;
        bool twoTapGt = rgby2.y > lumaMax;
    #endif


    if(twoTapLt || twoTapGt) rgby2 = rgby1;

    return rgby2; }

#endif




#if (FXAA_PS3 == 1) && (FXAA_EARLY_EXIT == 1)

#pragma regcount 7
#pragma disablepc all
#pragma option O2
#pragma option OutColorPrec=fp16
#pragma texformat default RGBA8

half4 FxaaPixelShader(
    
    FxaaFloat2 pos,
    FxaaFloat4 fxaaConsolePosPos,
    FxaaTex tex,
    FxaaTex fxaaConsole360TexExpBiasNegOne,
    FxaaTex fxaaConsole360TexExpBiasNegTwo,
    FxaaFloat2 fxaaQualityRcpFrame,
    FxaaFloat4 fxaaConsoleRcpFrameOpt,
    FxaaFloat4 fxaaConsoleRcpFrameOpt2,
    FxaaFloat4 fxaaConsole360RcpFrameOpt2,
    FxaaFloat fxaaQualitySubpix,
    FxaaFloat fxaaQualityEdgeThreshold,
    FxaaFloat fxaaQualityEdgeThresholdMin,
    FxaaFloat fxaaConsoleEdgeSharpness,
    FxaaFloat fxaaConsoleEdgeThreshold,
    FxaaFloat fxaaConsoleEdgeThresholdMin,
    FxaaFloat4 fxaaConsole360ConstDir
) {


    half4 rgbyNe = h4tex2Dlod(tex, half4(fxaaConsolePosPos.zy, 0, 0));
    #if (FXAA_GREEN_AS_LUMA == 0)
        half lumaNe = rgbyNe.w + half(1.0/512.0);
    #else
        half lumaNe = rgbyNe.y + half(1.0/512.0);
    #endif


    half4 lumaSw = h4tex2Dlod(tex, half4(fxaaConsolePosPos.xw, 0, 0));
    #if (FXAA_GREEN_AS_LUMA == 0)
        half lumaSwNegNe = lumaSw.w - lumaNe;
    #else
        half lumaSwNegNe = lumaSw.y - lumaNe;
    #endif


    half4 lumaNw = h4tex2Dlod(tex, half4(fxaaConsolePosPos.xy, 0, 0));
    #if (FXAA_GREEN_AS_LUMA == 0)
        half lumaMaxNwSw = max(lumaNw.w, lumaSw.w);
        half lumaMinNwSw = min(lumaNw.w, lumaSw.w);
    #else
        half lumaMaxNwSw = max(lumaNw.y, lumaSw.y);
        half lumaMinNwSw = min(lumaNw.y, lumaSw.y);
    #endif


    half4 lumaSe = h4tex2Dlod(tex, half4(fxaaConsolePosPos.zw, 0, 0));
    #if (FXAA_GREEN_AS_LUMA == 0)
        half dirZ =  lumaNw.w + lumaSwNegNe;
        half dirX = -lumaNw.w + lumaSwNegNe;
    #else
        half dirZ =  lumaNw.y + lumaSwNegNe;
        half dirX = -lumaNw.y + lumaSwNegNe;
    #endif


    half3 dir;
    dir.y = 0.0;
    #if (FXAA_GREEN_AS_LUMA == 0)
        dir.x =  lumaSe.w + dirX;
        dir.z = -lumaSe.w + dirZ;
        half lumaMinNeSe = min(lumaNe, lumaSe.w);
    #else
        dir.x =  lumaSe.y + dirX;
        dir.z = -lumaSe.y + dirZ;
        half lumaMinNeSe = min(lumaNe, lumaSe.y);
    #endif


    half4 dir1_pos;
    dir1_pos.xy = normalize(dir).xz;
    half dirAbsMinTimes8 = min(abs(dir1_pos.x), abs(dir1_pos.y)) * half(FXAA_CONSOLE_PS3_EDGE_SHARPNESS);


    half4 dir2_pos;
    dir2_pos.xy = clamp(dir1_pos.xy / dirAbsMinTimes8, half(-2.0), half(2.0));
    dir1_pos.zw = pos.xy;
    dir2_pos.zw = pos.xy;
    #if (FXAA_GREEN_AS_LUMA == 0)
        half lumaMaxNeSe = max(lumaNe, lumaSe.w);
    #else
        half lumaMaxNeSe = max(lumaNe, lumaSe.y);
    #endif


    half4 temp1N;
    temp1N.xy = dir1_pos.zw - dir1_pos.xy * fxaaConsoleRcpFrameOpt.zw;
    temp1N = h4tex2Dlod(tex, half4(temp1N.xy, 0.0, 0.0));
    half lumaMax = max(lumaMaxNwSw, lumaMaxNeSe);
    half lumaMin = min(lumaMinNwSw, lumaMinNeSe);


    half4 rgby1;
    rgby1.xy = dir1_pos.zw + dir1_pos.xy * fxaaConsoleRcpFrameOpt.zw;
    rgby1 = h4tex2Dlod(tex, half4(rgby1.xy, 0.0, 0.0));
    rgby1 = (temp1N + rgby1) * 0.5;


    half4 rgbyM = h4tex2Dlod(tex, half4(pos.xy, 0.0, 0.0));
    #if (FXAA_GREEN_AS_LUMA == 0)
        half lumaMaxM = max(lumaMax, rgbyM.w);
        half lumaMinM = min(lumaMin, rgbyM.w);
    #else
        half lumaMaxM = max(lumaMax, rgbyM.y);
        half lumaMinM = min(lumaMin, rgbyM.y);
    #endif


    half4 temp2N;
    temp2N.xy = dir2_pos.zw - dir2_pos.xy * fxaaConsoleRcpFrameOpt2.zw;
    temp2N = h4tex2Dlod(tex, half4(temp2N.xy, 0.0, 0.0));
    half4 rgby2;
    rgby2.xy = dir2_pos.zw + dir2_pos.xy * fxaaConsoleRcpFrameOpt2.zw;
    half lumaRangeM = (lumaMaxM - lumaMinM) / FXAA_CONSOLE_PS3_EDGE_THRESHOLD;


    rgby2 = h4tex2Dlod(tex, half4(rgby2.xy, 0.0, 0.0));
    rgby2 = (temp2N + rgby2) * 0.5;


    rgby2 = (rgby2 + rgby1) * 0.5;


    #if (FXAA_GREEN_AS_LUMA == 0)
        bool twoTapLt = rgby2.w < lumaMin;
        bool twoTapGt = rgby2.w > lumaMax;
    #else
        bool twoTapLt = rgby2.y < lumaMin;
        bool twoTapGt = rgby2.y > lumaMax;
    #endif
    bool earlyExit = lumaRangeM < lumaMax;
    bool twoTap = twoTapLt || twoTapGt;


    if(twoTap) rgby2 = rgby1;
    if(earlyExit) rgby2 = rgbyM;

    return rgby2; }

#endif
)" },
{ "glsl/fxaa_fp.glsl", R"(







uniform sampler2D	u_ColorMap;

void	main()
{
	gl_FragColor = FxaaPixelShader(
		gl_FragCoord.xy * r_FBufScale, 
		vec4(0.0), 
		u_ColorMap, 
		u_ColorMap, 
		u_ColorMap, 
		r_FBufScale, 
		vec4(0.0), 
		vec4(0.0), 
		vec4(0.0), 
		0.75, 
		0.166, 
		0.0625, 
		0.0, 
		0.0, 
		0.0, 
		vec4(0.0) 
	);
}
)" },
{ "glsl/fxaa_vp.glsl", R"(




attribute vec3 		attr_Position;

void	main()
{
	
	gl_Position = vec4(attr_Position, 1.0);
}
)" },
{ "glsl/generic_fp.glsl", R"(




uniform sampler2D	u_ColorMap;
uniform float		u_AlphaThreshold;

varying vec2		var_Tex;
varying vec4		var_Color;

#if defined(USE_DEPTH_FADE) || defined(USE_VERTEX_SPRITE)
varying vec2            var_FadeDepth;
uniform sampler2D       u_DepthMap;
#endif

void	main()
{
	vec4 color = texture2D(u_ColorMap, var_Tex);

	if( abs(color.a + u_AlphaThreshold) <= 1.0 )
	{
		discard;
		return;
	}

#if defined(USE_DEPTH_FADE) || defined(USE_VERTEX_SPRITE)
	float depth = texture2D(u_DepthMap, gl_FragCoord.xy * r_FBufScale * r_NPOTScale).x;
	float fadeDepth = 0.5 * var_FadeDepth.x / var_FadeDepth.y + 0.5;
	color.a *= smoothstep(gl_FragCoord.z, fadeDepth, depth);
#endif

	color *= var_Color;
	gl_FragColor = color;
}
)" },
{ "glsl/generic_vp.glsl", R"(




uniform mat4		u_ColorTextureMatrix;
#if !defined(USE_VERTEX_SPRITE)
uniform vec3		u_ViewOrigin;
#endif

uniform float		u_Time;

uniform vec4		u_ColorModulate;
uniform vec4		u_Color;
#if defined(USE_TCGEN_ENVIRONMENT)
uniform mat4		u_ModelMatrix;
#endif
uniform mat4		u_ModelViewProjectionMatrix;

#if defined(USE_VERTEX_SPRITE)
varying vec2            var_FadeDepth;
uniform mat4		u_ProjectionMatrixTranspose;
#elif defined(USE_DEPTH_FADE)
uniform float           u_DepthScale;
varying vec2            var_FadeDepth;
#endif

varying vec2		var_Tex;
varying vec4		var_Color;

void DeformVertex( inout vec4 pos,
		   inout vec3 normal,
		   inout vec2 st,
		   inout vec4 color,
		   in    float time);

void	main()
{
	vec4 position;
	localBasis LB;
	vec4 color;
	vec2 texCoord, lmCoord;

	VertexFetch( position, LB, color, texCoord, lmCoord );
	color = color * u_ColorModulate + u_Color;

	DeformVertex( position,
		      LB.normal,
		      texCoord,
		      color,
		      u_Time);

	
	gl_Position = u_ModelViewProjectionMatrix * position;

	
#if defined(USE_TCGEN_ENVIRONMENT)
	{
		
		position.xyz = mat3(u_ModelMatrix) * position.xyz;

		vec3 viewer = normalize(u_ViewOrigin - position.xyz);

		float d = dot(LB.normal, viewer);

		vec3 reflected = LB.normal * 2.0 * d - viewer;

		var_Tex = 0.5 + vec2(0.5, -0.5) * reflected.yz;
	}
#elif defined(USE_TCGEN_LIGHTMAP)
	var_Tex = (u_ColorTextureMatrix * vec4(lmCoord, 0.0, 1.0)).xy;
#else
	var_Tex = (u_ColorTextureMatrix * vec4(texCoord, 0.0, 1.0)).xy;
#endif

#if defined(USE_DEPTH_FADE) || defined(USE_VERTEX_SPRITE)
	
	vec4 fadeDepth = u_ModelViewProjectionMatrix * (position - u_DepthScale * vec4(LB.normal, 0.0));
	var_FadeDepth = fadeDepth.zw;
#endif

	var_Color = color;
}
)" },
{ "glsl/heatHaze_fp.glsl", R"(




uniform sampler2D	u_NormalMap;
uniform sampler2D	u_CurrentMap;
uniform float		u_AlphaThreshold;

varying vec2		var_TexNormal;
varying float		var_Deform;

void	main()
{
	vec4 color0, color1;

	
	color0 = texture2D(u_NormalMap, var_TexNormal).rgba;
	vec3 N = 2.0 * (color0.rgb - 0.5);

	
	vec2 st = gl_FragCoord.st * r_FBufScale;

	
	st += N.xy * var_Deform;
	st = clamp(st, 0.0, 1.0);

	
	st *= r_NPOTScale;

	color0 = texture2D(u_CurrentMap, st);

	gl_FragColor = color0;
}
)" },
{ "glsl/heatHaze_vp.glsl", R"(




uniform float		u_Time;

uniform mat4		u_NormalTextureMatrix;
uniform mat4		u_ProjectionMatrixTranspose;
uniform mat4		u_ModelViewMatrixTranspose;
uniform mat4		u_ModelViewProjectionMatrix;

uniform float		u_DeformMagnitude;

varying vec2		var_TexNormal;
varying float		var_Deform;

void DeformVertex( inout vec4 pos,
		   inout vec3 normal,
		   inout vec2 st,
		   inout vec4 color,
		   in    float time);

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

	
	gl_Position = u_ModelViewProjectionMatrix * position;

	
	deformVec = vec4(1, 0, 0, 1);
	deformVec.z = dot(u_ModelViewMatrixTranspose[2], position);

	
	var_TexNormal = (u_NormalTextureMatrix * vec4(texCoord, 0.0, 1.0)).st;

	d1 = dot(u_ProjectionMatrixTranspose[0],  deformVec);
	d2 = dot(u_ProjectionMatrixTranspose[3],  deformVec);

	
	var_Deform = min(d1 * (1.0 / max(d2, 1.0)), 0.02) * u_DeformMagnitude;
}
)" },
{ "glsl/lightMapping_fp.glsl", R"(




uniform sampler2D	u_DiffuseMap;
uniform sampler2D	u_NormalMap;
uniform sampler2D	u_SpecularMap;
uniform sampler2D	u_GlowMap;
uniform sampler2D	u_LightMap;
uniform sampler2D	u_DeluxeMap;
uniform float		u_AlphaThreshold;
uniform vec3		u_ViewOrigin;
uniform float		u_DepthScale;
uniform vec2		u_SpecularExponent;

varying vec3		var_Position;
varying vec4		var_TexDiffuseGlow;
varying vec4		var_TexNormalSpecular;
varying vec2		var_TexLight;

varying vec3		var_Tangent;
varying vec3		var_Binormal;
varying vec3		var_Normal;

varying vec4		var_Color;


void	main()
{
#if defined(USE_NORMAL_MAPPING)

	vec2 texDiffuse = var_TexDiffuseGlow.st;
	vec2 texNormal = var_TexNormalSpecular.st;
	vec2 texSpecular = var_TexNormalSpecular.pq;

	mat3 tangentToWorldMatrix = mat3(var_Tangent.xyz, var_Binormal.xyz, var_Normal.xyz);

	
	vec3 I = normalize(u_ViewOrigin - var_Position);

#if defined(USE_PARALLAX_MAPPING)
	

	
	vec3 V = I * tangentToWorldMatrix;
	V = normalize(V);

	
	vec2 S = V.xy * -u_DepthScale / V.z;

#if 0
	vec2 texOffset = vec2(0.0);
	for(int i = 0; i < 4; i++) {
		vec4 Normal = texture2D(u_NormalMap, texNormal.st + texOffset);
		float height = Normal.a * 0.2 - 0.0125;
		texOffset += height * Normal.z * S;
	}
#else
	float depth = RayIntersectDisplaceMap(texNormal, S, u_NormalMap);

	
	vec2 texOffset = S * depth;
#endif

	texDiffuse.st += texOffset;
	texNormal.st += texOffset;
	texSpecular.st += texOffset;
#endif 

	
	vec4 diffuse = texture2D(u_DiffuseMap, texDiffuse);

	if( abs(diffuse.a + u_AlphaThreshold) <= 1.0 )
	{
		discard;
		return;
	}


	
	vec3 N = texture2D(u_NormalMap, texNormal.st).xyw;
	N.x *= N.z;
	N.xy = 2.0 * N.xy - 1.0;
	N.z = sqrt(1.0 - dot(N.xy, N.xy));
	N = normalize(tangentToWorldMatrix * N);

	
	vec3 L = (2.0 * (texture2D(u_DeluxeMap, var_TexLight).xyz - 0.5));
	L = normalize(L);

	
	vec3 H = normalize(L + I);

	
	vec3 lightColor = texture2D(u_LightMap, var_TexLight).rgb;

	
	vec4 specular = texture2D(u_SpecularMap, texSpecular).rgba;

	float NdotL = clamp(dot(N, L), 0.0, 1.0);

	float NdotLnobump = clamp(dot(normalize(var_Normal.xyz), L), 0.004, 1.0);
	

	
	vec3 lightColorNoNdotL = lightColor.rgb / NdotLnobump;

	
	vec4 color = diffuse;
	
	
	
	
	color.rgb *= clamp(lightColorNoNdotL.rgb * NdotL, lightColor.rgb * 0.3, lightColor.rgb);
	
	
	color.rgb += specular.rgb * lightColorNoNdotL * pow(clamp(dot(N, H), 0.0, 1.0), u_SpecularExponent.x * specular.a + u_SpecularExponent.y) * r_SpecularScale;
	color.a *= var_Color.a;	


#else 

	
	vec4 diffuse = texture2D(u_DiffuseMap, var_TexDiffuseGlow.st);

	if( abs(diffuse.a + u_AlphaThreshold) <= 1.0 )
	{
		discard;
		return;
	}

	vec3 N = normalize(var_Normal);

	vec3 specular = vec3(0.0, 0.0, 0.0);

	
	vec3 lightColor = texture2D(u_LightMap, var_TexLight).rgb;

	vec4 color = diffuse;
	color.rgb *= lightColor;
	color.a *= var_Color.a;	
#endif
#if defined(USE_GLOW_MAPPING)
	color.rgb += texture2D(u_GlowMap, var_TexDiffuseGlow.pq).rgb;
#endif
	
	N = N * 0.5 + 0.5;

#if defined(r_DeferredShading)
	gl_FragData[0] = color; 							
	gl_FragData[1] = vec4(diffuse.rgb, var_Color.a);	
	gl_FragData[2] = vec4(N, var_Color.a);
	gl_FragData[3] = vec4(specular, var_Color.a);
#else
	gl_FragColor = color;
#endif

#if defined(r_showLightMaps)
	gl_FragColor = texture2D(u_LightMap, var_TexLight);
#elif defined(r_showDeluxeMaps)
	gl_FragColor = texture2D(u_DeluxeMap, var_TexLight);
#endif


#if 0
#if defined(USE_PARALLAX_MAPPING)
	gl_FragColor = vec4(vec3(1.0, 0.0, 0.0), diffuse.a);
#elif defined(USE_NORMAL_MAPPING)
	gl_FragColor = vec4(vec3(0.0, 0.0, 1.0), diffuse.a);
#else
	gl_FragColor = vec4(vec3(0.0, 1.0, 0.0), diffuse.a);
#endif
#endif

}
)" },
{ "glsl/lightMapping_vp.glsl", R"(




uniform mat4		u_DiffuseTextureMatrix;
uniform mat4		u_NormalTextureMatrix;
uniform mat4		u_SpecularTextureMatrix;
uniform mat4		u_GlowTextureMatrix;
uniform mat4		u_ModelMatrix;
uniform mat4		u_ModelViewProjectionMatrix;

uniform float		u_Time;

uniform vec4		u_ColorModulate;
uniform vec4		u_Color;

varying vec3		var_Position;
varying vec4		var_TexDiffuseGlow;
varying vec4		var_TexNormalSpecular;
varying vec2		var_TexLight;
varying vec3		var_Tangent;
varying vec3		var_Binormal;
varying vec3		var_Normal;
varying vec4		var_Color;


void DeformVertex( inout vec4 pos,
		   inout vec3 normal,
		   inout vec2 st,
		   inout vec4 color,
		   in    float time);

void	main()
{
	vec4 position;
	localBasis LB;
	vec2 texCoord, lmCoord;
	vec4 color;

	VertexFetch( position, LB, color, texCoord, lmCoord );

	color = color * u_ColorModulate + u_Color;

	DeformVertex( position,
		      LB.normal,
		      texCoord.xy,
		      color,
		      u_Time);

	
	gl_Position = u_ModelViewProjectionMatrix * position;

	
	var_TexDiffuseGlow.st = (u_DiffuseTextureMatrix * vec4(texCoord, 0.0, 1.0)).st;
	var_TexLight = lmCoord;

#if defined(USE_NORMAL_MAPPING)
	
	var_TexNormalSpecular.st = (u_NormalTextureMatrix * vec4(texCoord, 0.0, 1.0)).st;

	
	var_TexNormalSpecular.pq = (u_SpecularTextureMatrix * vec4(texCoord, 0.0, 1.0)).st;
#endif

#if defined(USE_GLOW_MAPPING)
	var_TexDiffuseGlow.pq = (u_GlowTextureMatrix * vec4(texCoord, 0.0, 1.0)).st;
#endif

	var_Position = position.xyz;

	var_Normal = LB.normal;
#if defined(USE_NORMAL_MAPPING)
	var_Tangent = LB.tangent;
	var_Binormal = LB.binormal;
#endif

	var_Color = color;
}
)" },
{ "glsl/lightVolume_omni_fp.glsl", R"(





#ifdef TEXTURE_RG
#  define SWIZ1 r
#  define SWIZ2 rg
#else
#  define SWIZ1 a
#  define SWIZ2 ar
#endif

uniform sampler2D	u_DepthMap;
uniform sampler2D	u_AttenuationMapXY;
uniform sampler2D	u_AttenuationMapZ;
uniform samplerCube	u_ShadowMap;
uniform vec3		u_ViewOrigin;
uniform vec3		u_LightOrigin;
uniform vec3		u_LightColor;
uniform float		u_LightRadius;
uniform float       u_LightScale;
uniform mat4		u_LightAttenuationMatrix;
uniform mat4		u_UnprojectMatrix;

varying vec2		var_TexDiffuse;
varying vec3		var_TexAttenXYZ;

void	main()
{
	
	vec2 st = gl_FragCoord.st * r_FBufScale;

	
	st *= r_NPOTScale;

	
	float depth = texture2D(u_DepthMap, st).r;
	vec4 P = u_UnprojectMatrix * vec4(gl_FragCoord.xy, depth, 1.0);
	P.xyz /= P.w;

	
	vec3 R = normalize(P.xyz - u_ViewOrigin);
	

	
	

	float traceDistance = distance(P.xyz, u_ViewOrigin);
	traceDistance = clamp(traceDistance, 0.0, 2500.0);

	

	vec4 color = vec4(0.0, 0.0, 0.0, 1.0);

	
	

	int steps = int(min(traceDistance, 2000.0));	
	float stepSize = 64.0;

	for(float tracedDistance = 0.0; tracedDistance < traceDistance; tracedDistance += stepSize)
	{
		
		
		vec3 T = u_ViewOrigin + (R * tracedDistance);

		
		vec3 texAttenXYZ		= (u_LightAttenuationMatrix * vec4(T, 1.0)).xyz;
		vec3 attenuationXY		= texture2D(u_AttenuationMapXY, texAttenXYZ.xy).rgb;
		vec3 attenuationZ		= texture2D(u_AttenuationMapZ, vec2(texAttenXYZ.z, 0)).rgb;

		float shadow = 1.0;

		#if defined(VSM)
		#if defined(USE_SHADOWING)
		{
			
			vec3 I2 = T - u_LightOrigin;

			vec2 shadowMoments = textureCube(u_ShadowMap, I2).SWIZ2;

			#if defined(VSM_CLAMP)
			
			shadowMoments = 0.5 * (shadowMoments + 1.0);
			#endif

			float shadowDistance = shadowMoments.r;
			float shadowDistanceSquared = shadowMoments.g;

			const float	SHADOW_BIAS = 0.001;
			float vertexDistance = length(I2) / u_LightRadius - SHADOW_BIAS;

			
			shadow = vertexDistance <= shadowDistance ? 1.0 : 0.0;

			
			float E_x2 = shadowDistanceSquared;
			float Ex_2 = shadowDistance * shadowDistance;

			
			float variance = min(max(E_x2 - Ex_2, 0.0) + VSM_EPSILON, 1.0);

			float mD = shadowDistance - vertexDistance;
			float mD_2 = mD * mD;
			float p = variance / (variance + mD_2);

			color.rgb += attenuationXY * attenuationZ * max(shadow, p);
		}

		if(shadow <= 0.0)
		{
			continue;
		}
		else
		#endif
		#endif
		{
			color.rgb += attenuationXY * attenuationZ;
		}
	}

	color.rgb /= float(steps);
	color.rgb *= u_LightColor;
	

	gl_FragColor = color;
}
)" },
{ "glsl/lightVolume_omni_vp.glsl", R"(




attribute vec3 		attr_Position;

uniform mat4		u_ModelViewProjectionMatrix;

void	main()
{
	
	gl_Position = u_ModelViewProjectionMatrix * vec4(attr_Position, 1.0);
}
)" },
{ "glsl/liquid_fp.glsl", R"(




uniform sampler2D	u_CurrentMap;
uniform sampler2D	u_PortalMap;
uniform sampler2D	u_DepthMap;
uniform sampler2D	u_NormalMap;
uniform vec3		u_ViewOrigin;
uniform float		u_FogDensity;
uniform vec3		u_FogColor;
uniform float		u_RefractionIndex;
uniform float		u_FresnelPower;
uniform float		u_FresnelScale;
uniform float		u_FresnelBias;
uniform float		u_NormalScale;
uniform mat4		u_ModelMatrix;
uniform mat4		u_UnprojectMatrix;
uniform vec2		u_SpecularExponent;

uniform sampler3D       u_LightGrid1;
uniform sampler3D       u_LightGrid2;
uniform vec3            u_LightGridOrigin;
uniform vec3            u_LightGridScale;

varying vec3		var_Position;
varying vec2		var_TexNormal;
varying vec3		var_Tangent;
varying vec3		var_Binormal;
varying vec3		var_Normal;

#if defined(USE_PARALLAX_MAPPING)
float RayIntersectDisplaceMap(vec2 dp, vec2 ds)
{
	const int linearSearchSteps = 16;
	const int binarySearchSteps = 6;

	float depthStep = 1.0 / float(linearSearchSteps);

	
	float size = depthStep;

	
	float depth = 0.0;

	
	float bestDepth = 1.0;

	
	for(int i = 0; i < linearSearchSteps - 1; ++i)
	{
		depth += size;

		vec4 t = texture2D(u_NormalMap, dp + ds * depth);

		if(bestDepth > 0.996)		
			if(depth >= t.w)
				bestDepth = depth;	
	}

	depth = bestDepth;

	
	for(int i = 0; i < binarySearchSteps; ++i)
	{
		size *= 0.5;

		vec4 t = texture2D(u_NormalMap, dp + ds * depth);

		if(depth >= t.w)
		#ifdef RM_DOUBLEDEPTH
			if(depth <= t.z)
		#endif
			{
				bestDepth = depth;
				depth -= 2.0 * size;
			}

		depth += size;
	}

	return bestDepth;
}
#endif

void ReadLightGrid(in vec3 pos, out vec3 lgtDir,
		   out vec3 ambCol, out vec3 lgtCol ) {
	vec4 texel1 = texture3D(u_LightGrid1, pos);
	vec4 texel2 = texture3D(u_LightGrid2, pos);
	float ambLum, lgtLum;

	texel1.xyz = (texel1.xyz * 255.0 - 128.0) / 127.0;
	texel2.xyzw = texel2.xyzw - 0.5;

	lgtDir = normalize(texel1.xyz);

	lgtLum = 2.0 * length(texel1.xyz) * texel1.w;
	ambLum = 2.0 * texel1.w - lgtLum;

	
	ambCol.g = ambLum + texel2.x;
	ambLum   = ambLum - texel2.x;
	ambCol.r = ambLum + texel2.y;
	ambCol.b = ambLum - texel2.y;

	lgtCol.g = lgtLum + texel2.z;
	lgtLum   = lgtLum - texel2.z;
	lgtCol.r = lgtLum + texel2.w;
	lgtCol.b = lgtLum - texel2.w;
}


void	main()
{
	
	vec3 I = normalize(u_ViewOrigin - var_Position);

	mat3 tangentToWorldMatrix = mat3(var_Tangent.xyz, var_Binormal.xyz, var_Normal.xyz);
	if(gl_FrontFacing)
	{
		tangentToWorldMatrix = -tangentToWorldMatrix;
	}

	
	vec2 texScreen = gl_FragCoord.st * r_FBufScale * r_NPOTScale;
	vec2 texNormal = var_TexNormal.st;

#if defined(USE_PARALLAX_MAPPING)
	
	vec3 V = I * tangentToWorldMatrix;
	V = normalize(V);

	

	
	
	vec2 S = V.xy * -0.03 / V.z;

	float depth = RayIntersectDisplaceMap(texNormal, S);

	
	vec2 texOffset = S * depth;

	texScreen.st += texOffset;
	texNormal.st += texOffset;
#endif

	

	vec3 N = normalize(var_Normal);

	vec3 N2 = texture2D(u_NormalMap, texNormal.st).xyw);
	N2.x *= N2.z;
	N2.xy = 2.0 * N2.xy - 1.0;
	N2.z = sqrt(1.0 - dot(N2.xy, N2.xy));
	N2 = normalize(tangentToWorldMatrix * N2);

	
	float fresnel = clamp(u_FresnelBias + pow(1.0 - dot(I, N), u_FresnelPower) *
			u_FresnelScale, 0.0, 1.0);

	texScreen += u_NormalScale * N2.xy;

	vec3 refractColor = texture2D(u_CurrentMap, texScreen).rgb;
	vec3 reflectColor = texture2D(u_PortalMap, texScreen).rgb;

	vec4 color;

	color.rgb = mix(refractColor, reflectColor, fresnel);
	color.a = 1.0;

	if(u_FogDensity > 0.0)
	{
		
		float depth = texture2D(u_DepthMap, texScreen).r;
		vec4 P = u_UnprojectMatrix * vec4(gl_FragCoord.xy, depth, 1.0);
		P.xyz /= P.w;

		
		float fogDistance = distance(P.xyz, var_Position);

		
		float fogExponent = fogDistance * u_FogDensity;

		
		float fogFactor = exp2(-abs(fogExponent));

		color.rgb = mix(u_FogColor, color.rgb, fogFactor);
	}

	vec3 L;
	vec3 ambCol;
	vec3 lgtCol;

	ReadLightGrid( (var_Position - u_LightGridOrigin) * u_LightGridScale,
		       L, ambCol, lgtCol );

	
	vec3 H = normalize(L + I);

	
	vec3 light = lgtCol * clamp(dot(N2, L), 0.0, 1.0);

	
	vec3 specular = reflectColor * lgtCol * pow(clamp(dot(N2, H), 0.0, 1.0), u_SpecularExponent.x + u_SpecularExponent.y) * r_SpecularScale;
	color.rgb += specular;

	gl_FragColor = color;
}
)" },
{ "glsl/liquid_vp.glsl", R"(




attribute vec3 		attr_Position;
attribute vec2 		attr_TexCoord0;
attribute vec3		attr_Tangent;
attribute vec3		attr_Binormal;
attribute vec3		attr_Normal;

uniform mat4		u_NormalTextureMatrix;
uniform mat4		u_ModelMatrix;
uniform mat4		u_ModelViewProjectionMatrix;

varying vec3		var_Position;
varying vec2		var_TexNormal;
varying vec3		var_Tangent;
varying vec3		var_Binormal;
varying vec3		var_Normal;

void	main()
{
	
	gl_Position = u_ModelViewProjectionMatrix * vec4(attr_Position, 1.0);

	
	var_Position = (u_ModelMatrix * vec4(attr_Position, 1.0)).xyz;

	
	var_TexNormal = (u_NormalTextureMatrix * vec4(attr_TexCoord0, 0.0, 1.0)).st;

	var_Tangent.xyz = (u_ModelMatrix * vec4(attr_Tangent, 0.0)).xyz;
	var_Binormal.xyz = (u_ModelMatrix * vec4(attr_Binormal, 0.0)).xyz;

	
	var_Normal = (u_ModelMatrix * vec4(attr_Normal, 0.0)).xyz;
}

)" },
{ "glsl/motionblur_fp.glsl", R"(




uniform sampler2D	u_ColorMap;
uniform sampler2D	u_DepthMap;

uniform vec3            u_blurVec;

void	main()
{
	vec2 st = gl_FragCoord.st;
	vec4 color = vec4( 0.0 );

	
	st *= r_FBufScale;

	
	st *= r_NPOTScale;

	float depth = texture2D( u_DepthMap, st ).r;

	if( depth >= 1.0 ) {
		discard;
		return;
        }
	depth /= 1.0 - depth;

	vec3 start = vec3(st * 2.0 - 1.0, 1.0) * depth;
	vec3 end   = start + u_blurVec.xyz;

	float weight = 1.0;
	float total = 0.0;

	for( int i = 0; i < 6; i ++ ) {
		vec3 pos = mix( start, end, float(i) * 0.1 );
		pos /= pos.z;

		color += weight * texture2D( u_ColorMap, 0.5 * pos.xy + 0.5 );
		total += weight;
		weight *= 0.5;
        }

	gl_FragColor = color / total;
}
)" },
{ "glsl/motionblur_vp.glsl", R"(




attribute vec3 		attr_Position;

void	main()
{
	
	gl_Position = vec4(attr_Position, 1.0);
}
)" },
{ "glsl/portal_fp.glsl", R"(




uniform sampler2D	u_CurrentMap;
uniform float		u_PortalRange;

varying vec3		var_Position;
varying vec4		var_Color;

void	main()
{
	
	vec2 st = gl_FragCoord.st * r_FBufScale;

	
	st *= r_NPOTScale;

	vec4 color = texture2D(u_CurrentMap, st);
	color *= var_Color;

	float len = length(var_Position);

	len /= u_PortalRange;
	color.rgb *= 1.0 - clamp(len, 0.0, 1.0);

	gl_FragColor = color;
}
)" },
{ "glsl/portal_vp.glsl", R"(




attribute vec3 		attr_Position;
attribute vec4		attr_Color;

uniform mat4		u_ModelViewMatrix;
uniform mat4		u_ModelViewProjectionMatrix;

varying vec3		var_Position;
varying vec4		var_Color;

void	main()
{
	
	gl_Position = u_ModelViewProjectionMatrix * vec4(attr_Position, 1.0);

	
	var_Position = (u_ModelViewMatrix * vec4(attr_Position, 1.0)).xyz;

	
	var_Color = attr_Color;
}
)" },
{ "glsl/reflection_CB_fp.glsl", R"(




uniform samplerCube	u_ColorMap;
uniform sampler2D	u_NormalMap;
uniform vec3		u_ViewOrigin;
uniform mat4		u_ModelMatrix;

varying vec3		var_Position;
varying vec2		var_TexNormal;
varying vec4		var_Tangent;
varying vec4		var_Binormal;
varying vec4		var_Normal;

void	main()
{
	
	vec3 I = normalize(var_Position - u_ViewOrigin);


#if defined(USE_NORMAL_MAPPING)
	
	vec3 N = texture2D(u_NormalMap, var_TexNormal.st).xyw;
	N.x *= N.z;
	N.xy = 2.0 * N.xy - 1.0;
	N.z = sqrt(1.0 - dot(N.xy, N.xy));
	#if defined(r_NormalScale)
	N.z *= r_NormalScale;
	normalize(N);
	#endif

	mat3 tangentToWorldMatrix = mat3(var_Tangent.xyz, var_Binormal.xyz, var_Normal.xyz);

	
	N = normalize(tangentToWorldMatrix * N);

#else

	vec3 N = normalize(var_Normal.xyz);
#endif

	
	vec3 R = reflect(I, N);

	gl_FragColor = textureCube(u_ColorMap, R).rgba;
	
}
)" },
{ "glsl/reflection_CB_vp.glsl", R"(




uniform mat4		u_NormalTextureMatrix;
uniform mat4		u_ModelMatrix;
uniform mat4		u_ModelViewProjectionMatrix;

uniform float		u_Time;

varying vec3		var_Position;
varying vec2		var_TexNormal;
varying vec4		var_Tangent;
varying vec4		var_Binormal;
varying vec4		var_Normal;

void DeformVertex( inout vec4 pos,
		   inout vec3 normal,
		   inout vec2 st,
		   inout vec4 color,
		   in    float time);

void	main()
{
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

	
	gl_Position = u_ModelViewProjectionMatrix * position;

	
	var_Position = (u_ModelMatrix * position).xyz;

#if defined(USE_NORMAL_MAPPING)
	var_Tangent.xyz = (u_ModelMatrix * vec4(LB.tangent, 0.0)).xyz;
	var_Binormal.xyz = (u_ModelMatrix * vec4(LB.binormal, 0.0)).xyz;
#endif

	var_Normal.xyz = (u_ModelMatrix * vec4(LB.normal, 0.0)).xyz;

#if defined(USE_NORMAL_MAPPING)
	
	var_TexNormal = (u_NormalTextureMatrix * vec4(texCoord, 0.0, 1.0)).st;
#endif
}

)" },
{ "glsl/refraction_C_fp.glsl", R"(




uniform samplerCube	u_ColorMap;
uniform vec3		u_ViewOrigin;
uniform float		u_RefractionIndex;
uniform float		u_FresnelPower;
uniform float		u_FresnelScale;
uniform float		u_FresnelBias;

varying vec3		var_Position;
varying vec3		var_Normal;

void	main()
{
	
	vec3 I = normalize(var_Position - u_ViewOrigin);

	
	vec3 N = normalize(var_Normal);

	
	vec3 R = reflect(I, N);

	
	vec3 T = refract(I, N, u_RefractionIndex);

	
	float fresnel = u_FresnelBias + pow(1.0 - dot(I, N), u_FresnelPower) * u_FresnelScale;

	vec3 reflectColor = textureCube(u_ColorMap, R).rgb;
	vec3 refractColor = textureCube(u_ColorMap, T).rgb;

	
	vec4 color;
	color.r = (1.0 - fresnel) * refractColor.r + reflectColor.r * fresnel;
	color.g = (1.0 - fresnel) * refractColor.g + reflectColor.g * fresnel;
	color.b = (1.0 - fresnel) * refractColor.b + reflectColor.b * fresnel;
	color.a = 1.0;

	gl_FragColor = color;
}
)" },
{ "glsl/refraction_C_vp.glsl", R"(




attribute vec3 		attr_Position;
attribute vec3		attr_Normal;
#if defined(r_VertexSkinning)
attribute vec4		attr_BoneIndexes;
attribute vec4		attr_BoneWeights;
uniform int			u_VertexSkinning;
uniform mat4		u_BoneMatrix[MAX_GLSL_BONES];
#endif

uniform mat4		u_ModelMatrix;
uniform mat4		u_ModelViewProjectionMatrix;

varying vec3		var_Position;
varying vec3		var_Normal;

void	main()
{
#if defined(r_VertexSkinning)
	if(bool(u_VertexSkinning))
	{
		vec4 vertex = vec4(0.0);
		vec3 normal = vec3(0.0);

		for(int i = 0; i < 4; i++)
		{
			int boneIndex = int(attr_BoneIndexes[i]);
			float boneWeight = attr_BoneWeights[i];
			mat4  boneMatrix = u_BoneMatrix[boneIndex];

			vertex += (boneMatrix * vec4(attr_Position, 1.0)) * boneWeight;
			normal += (boneMatrix * vec4(attr_Normal, 0.0)).xyz * boneWeight;
		}

		
		gl_Position = u_ModelViewProjectionMatrix * vertex;

		
		var_Position = (u_ModelMatrix * vertex).xyz;

		
		var_Normal = (u_ModelMatrix * vec4(normal, 0.0)).xyz;
	}
	else
#endif
	{
		
		gl_Position = u_ModelViewProjectionMatrix * vec4(attr_Position, 1.0);

		
		var_Position = (u_ModelMatrix * vec4(attr_Position, 1.0)).xyz;

		
		var_Normal = (u_ModelMatrix * vec4(attr_Normal, 0.0)).xyz;
	}
}

)" },
{ "glsl/reliefMapping_fp.glsl", R"(



#if defined(USE_PARALLAX_MAPPING)
float RayIntersectDisplaceMap(vec2 dp, vec2 ds, sampler2D normalMap)
{
	const int linearSearchSteps = 16;
	const int binarySearchSteps = 6;

	float depthStep = 1.0 / float(linearSearchSteps);

	
	float size = depthStep;

	
	float depth = 0.0;

	
	float bestDepth = 1.0;

	
	for(int i = 0; i < linearSearchSteps - 1; ++i)
	{
		depth += size;

		vec4 t = texture2D(normalMap, dp + ds * depth);

		if(bestDepth > 0.996)		
			if(depth >= t.w)
				bestDepth = depth;	
	}

	depth = bestDepth;

	
	for(int i = 0; i < binarySearchSteps; ++i)
	{
		size *= 0.5;

		vec4 t = texture2D(normalMap, dp + ds * depth);

		if(depth >= t.w)
		#ifdef RM_DOUBLEDEPTH
			if(depth <= t.z)
		#endif
			{
				bestDepth = depth;
				depth -= 2.0 * size;
			}

		depth += size;
	}

	return bestDepth;
}
#endif



)" },
{ "glsl/screen_fp.glsl", R"(




uniform sampler2D	u_CurrentMap;

varying vec4		var_Color;

void	main()
{
	
	vec2 st = gl_FragCoord.st * r_FBufScale;

	
	st *= r_NPOTScale;

	vec4 color = texture2D(u_CurrentMap, st);
	color *= var_Color;

	gl_FragColor = color;
}
)" },
{ "glsl/screen_vp.glsl", R"(




attribute vec3 		attr_Position;
attribute vec4		attr_Color;

uniform mat4		u_ModelViewProjectionMatrix;

varying vec4		var_Color;

void	main()
{
	
	gl_Position = u_ModelViewProjectionMatrix * vec4(attr_Position, 1.0);

	
	var_Color = attr_Color;
}
)" },
{ "glsl/shadowFill_fp.glsl", R"(




uniform sampler2D	u_ColorMap;
uniform float		u_AlphaThreshold;
uniform vec3		u_LightOrigin;
uniform float       u_LightRadius;

varying vec3		var_Position;
varying vec2		var_Tex;
varying vec4		var_Color;

#ifdef TEXTURE_RG
#  define SWIZ1 r
#  define SWIZ2 rg
#else
#  define SWIZ1 a
#  define SWIZ2 ar
#endif

#if defined(EVSM)

vec2 WarpDepth(float depth)
{
    
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

#endif 

void	main()
{
	vec4 color = texture2D(u_ColorMap, var_Tex);

	if( abs(color.a + u_AlphaThreshold) <= 1.0 )
	{
		discard;
		return;
	}


#if defined(VSM)

	float distance;

#if defined(LIGHT_DIRECTIONAL)
	distance = gl_FragCoord.z;
#else
	distance = length(var_Position - u_LightOrigin) / u_LightRadius;
#endif

	float distanceSquared = distance * distance;

	

#if defined(VSM_CLAMP)
	
	gl_FragColor.SWIZ2 = vec2(distance, distanceSquared) * 0.5 + 0.5;
#else
	gl_FragColor.SWIZ2 = vec2(distance, distanceSquared);
#endif

#elif defined(EVSM) || defined(ESM)

	float distance;
#if defined(LIGHT_DIRECTIONAL)
	{
		distance = gl_FragCoord.z;
		
		
		
	}
#else
	{
		distance = (length(var_Position - u_LightOrigin) / u_LightRadius); 
	}
#endif

#if defined(EVSM)
#if !defined(r_EVSMPostProcess)
	gl_FragColor = ShadowDepthToEVSM(distance);
#else
	gl_FragColor.SWIZ1 = distance;
#endif
#else
	gl_FragColor.SWIZ1 = distance;
#endif 

#else
	gl_FragColor = vec4(0.0, 0.0, 0.0, 0.0);
#endif
}
)" },
{ "glsl/shadowFill_vp.glsl", R"(




uniform vec4		u_Color;

uniform mat4		u_ColorTextureMatrix;
uniform mat4		u_ModelMatrix;
uniform mat4		u_ModelViewProjectionMatrix;

uniform float		u_Time;

varying vec3		var_Position;
varying vec2		var_Tex;
varying vec4		var_Color;

void DeformVertex( inout vec4 pos,
		   inout vec3 normal,
		   inout vec2 st,
		   inout vec4 color,
		   in    float time);

void	main()
{
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

	
	gl_Position = u_ModelViewProjectionMatrix * position;
	
#if defined(LIGHT_DIRECTIONAL)
	var_Position = gl_Position.xyz / gl_Position.w;
#else
	
	var_Position = (u_ModelMatrix * position).xyz;
#endif

	
	var_Tex = (u_ColorTextureMatrix * vec4(texCoord, 0.0, 1.0)).st;

	
	var_Color = u_Color;
}
)" },
{ "glsl/skybox_fp.glsl", R"(




uniform samplerCube	u_ColorMap;
uniform vec3		u_ViewOrigin;

varying vec3		var_Position;

void	main()
{
	
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
)" },
{ "glsl/skybox_vp.glsl", R"(




attribute vec3 		attr_Position;

uniform mat4		u_ModelMatrix;
uniform mat4		u_ModelViewProjectionMatrix;

varying vec3		var_Position;

void	main()
{
	
	gl_Position = u_ModelViewProjectionMatrix * vec4(attr_Position, 1.0);

	
	var_Position = (u_ModelMatrix * vec4(attr_Position, 1.0)).xyz;
}

)" },
{ "glsl/vertexAnimation_vp.glsl", R"(



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
#if defined(USE_NORMAL_MAPPING)
	LB.tangent = normalize(mix(fromLB.tangent, toLB.tangent, u_VertexInterpolation));
	LB.binormal = normalize(mix(fromLB.binormal, toLB.binormal, u_VertexInterpolation));
#endif
	color    = attr_Color;
	texCoord = attr_TexCoord0;
	lmCoord  = attr_TexCoord0;
}
#endif
)" },
{ "glsl/vertexLighting_DBS_entity_fp.glsl", R"(




uniform sampler2D	u_DiffuseMap;
uniform sampler2D	u_NormalMap;
uniform sampler2D	u_SpecularMap;
uniform sampler2D	u_GlowMap;

uniform samplerCube	u_EnvironmentMap0;
uniform samplerCube	u_EnvironmentMap1;
uniform float		u_EnvironmentInterpolation;

uniform float		u_AlphaThreshold;
uniform vec3		u_ViewOrigin;
uniform float		u_DepthScale;
uniform vec2        u_SpecularExponent;

uniform sampler3D       u_LightGrid1;
uniform sampler3D       u_LightGrid2;
uniform vec3            u_LightGridOrigin;
uniform vec3            u_LightGridScale;

varying vec3		var_Position;
varying vec2		var_TexDiffuse;
varying vec4		var_Color;
#if defined(USE_NORMAL_MAPPING)
varying vec4		var_TexNormalSpecular;
varying vec3		var_Tangent;
varying vec3		var_Binormal;
#endif
#if defined(USE_GLOW_MAPPING)
varying vec2		var_TexGlow;
#endif
varying vec3		var_Normal;

void ReadLightGrid(in vec3 pos, out vec3 lgtDir,
		   out vec3 ambCol, out vec3 lgtCol ) {
	vec4 texel1 = texture3D(u_LightGrid1, pos);
	vec4 texel2 = texture3D(u_LightGrid2, pos);

	ambCol = texel1.xyz;
	lgtCol = texel2.xyz;

	lgtDir.x = (texel1.w * 255.0 - 128.0) / 127.0;
	lgtDir.y = (texel2.w * 255.0 - 128.0) / 127.0;
	lgtDir.z = 1.0 - abs( lgtDir.x ) - abs( lgtDir.y );

	vec2 signs = 2.0 * step( 0.0, lgtDir.xy ) - vec2( 1.0 );
	if( lgtDir.z < 0.0 ) {
		lgtDir.xy = signs * ( vec2( 1.0 ) - abs( lgtDir.yx ) );
	}

	lgtDir = normalize( lgtDir );
}

void	main()
{
	
	vec3 L;
	vec3 ambCol;
	vec3 lgtCol;

	ReadLightGrid( (var_Position - u_LightGridOrigin) * u_LightGridScale,
		       L, ambCol, lgtCol );

	
	vec3 V = normalize(u_ViewOrigin - var_Position);

	vec2 texDiffuse = var_TexDiffuse.st;

#if defined(USE_NORMAL_MAPPING)
	mat3 tangentToWorldMatrix = mat3(var_Tangent.xyz, var_Binormal.xyz, var_Normal.xyz);

	vec2 texNormal = var_TexNormalSpecular.xy;
	vec2 texSpecular = var_TexNormalSpecular.zw;

#if defined(USE_PARALLAX_MAPPING)

	

	
	vec3 Vts = (u_ViewOrigin - var_Position.xyz) * tangentToWorldMatrix;
	Vts = normalize(Vts);

	
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

	
	vec2 texOffset = S * depth;
#endif

	texDiffuse.st += texOffset;
	texNormal.st += texOffset;
	texSpecular.st += texOffset;
#endif 

	
	vec3 N = texture2D(u_NormalMap, texNormal.st).xyw;
	N.x *= N.z;
	N.xy = 2.0 * N.xy - 1.0;
	N.z = sqrt(1.0 - dot(N.xy, N.xy));

	#if defined(r_NormalScale)
	N.z *= r_NormalScale;
	#endif

	N = normalize(tangentToWorldMatrix * N);

	
	vec3 H = normalize(L + V);

	
#if defined(USE_REFLECTIVE_SPECULAR)

	vec4 specBase = texture2D(u_SpecularMap, texSpecular).rgba;

	vec4 envColor0 = textureCube(u_EnvironmentMap0, reflect(-V, N)).rgba;
	vec4 envColor1 = textureCube(u_EnvironmentMap1, reflect(-V, N)).rgba;

	specBase.rgb *= mix(envColor0, envColor1, u_EnvironmentInterpolation).rgb;

	
	float NH = clamp(dot(N, H), 0, 1);
	vec3 specMult = lgtCol * pow(NH, u_SpecularExponent.x * specBase.a + u_SpecularExponent.y) * r_SpecularScale;

#if 0
	gl_FragColor = vec4(specular, 1.0);
	
	
	return;
#endif

#else

	
	float NH = clamp(dot(N, H), 0, 1);
	vec4 specBase = texture2D(u_SpecularMap, texSpecular).rgba;
	vec3 specMult = lgtCol * pow(NH, u_SpecularExponent.x * specBase.a + u_SpecularExponent.y) * r_SpecularScale;

#endif 


#else 

	vec3 N = normalize(var_Normal);

	vec3 specBase = vec3(0.0);
	vec3 specMult = vec3(0.0);

#endif 

	
	vec4 diffuse = texture2D(u_DiffuseMap, texDiffuse) * var_Color;

	if( abs(diffuse.a + u_AlphaThreshold) <= 1.0 )
	{
		discard;
		return;
	}


#if defined(r_RimLighting)
	float rim = pow(1.0 - clamp(dot(N, V), 0.0, 1.0), r_RimExponent);
	specBase.rgb = mix(specBase.rgb, vec3(1.0), rim);
	vec3 emission = ambCol * rim * rim * 0.2;

	
	

#endif

	
#if defined(r_HalfLambertLighting)
	
	float NL = dot(N, L) * 0.5 + 0.5;
	NL *= NL;
#elif defined(r_WrapAroundLighting)
	float NL = clamp(dot(N, L) + r_WrapAroundLighting, 0.0, 1.0) / clamp(1.0 + r_WrapAroundLighting, 0.0, 1.0);
#else
	float NL = clamp(dot(N, L), 0.0, 1.0);
#endif

	vec3 light = ambCol + lgtCol * NL;
	light *= r_AmbientScale;

	
	vec4 color = diffuse;
	color.rgb *= light;
	color.rgb += specBase.rgb * specMult;
#if defined(r_RimLighting)
	color.rgb += 0.7 * emission;
#endif
#if defined(USE_GLOW_MAPPING)
	color.rgb += texture2D(u_GlowMap, var_TexGlow).rgb;
#endif
	
	N = N * 0.5 + 0.5;

#if defined(r_DeferredShading)
	gl_FragData[0] = color;
	gl_FragData[1] = vec4(diffuse.rgb, 0.0);
	gl_FragData[2] = vec4(N, 0.0);
	gl_FragData[3] = vec4(specBase.rgb, 0.0);
#else
	gl_FragColor = color;
#endif

	

#if 0
#if defined(USE_PARALLAX_MAPPING)
	gl_FragColor = vec4(vec3(1.0, 0.0, 0.0), diffuse.a);
#elif defined(USE_NORMAL_MAPPING)
	gl_FragColor = vec4(vec3(0.0, 0.0, 1.0), diffuse.a);
#else
	gl_FragColor = vec4(vec3(0.0, 1.0, 0.0), diffuse.a);
#endif
#endif


#if defined(r_showEntityNormals)
	gl_FragColor = vec4(N, 1.0);
#endif
}
)" },
{ "glsl/vertexLighting_DBS_entity_vp.glsl", R"(




uniform mat4		u_DiffuseTextureMatrix;
uniform mat4		u_NormalTextureMatrix;
uniform mat4		u_SpecularTextureMatrix;
uniform mat4		u_GlowTextureMatrix;
uniform mat4		u_ModelMatrix;
uniform mat4		u_ModelViewProjectionMatrix;

uniform float		u_Time;

varying vec3		var_Position;
varying vec2		var_TexDiffuse;
varying vec4		var_Color;
#if defined(USE_NORMAL_MAPPING)
varying vec4		var_TexNormalSpecular;
varying vec3		var_Tangent;
varying vec3		var_Binormal;
#endif
#if defined(USE_GLOW_MAPPING)
varying vec2		var_TexGlow;
#endif

varying vec3		var_Normal;

void DeformVertex( inout vec4 pos,
		   inout vec3 normal,
		   inout vec2 st,
		   inout vec4 color,
		   in    float time);

void	main()
{
	vec4 position;
	localBasis LB;
	vec2 texCoord, lmCoord;
	vec4 color;

	VertexFetch( position, LB, color, texCoord, lmCoord);

	DeformVertex( position,
		      LB.normal,
		      texCoord,
		      color,
		      u_Time);

	
	gl_Position = u_ModelViewProjectionMatrix * position;

	
	var_Position = (u_ModelMatrix * position).xyz;

#if defined(USE_NORMAL_MAPPING)
	var_Tangent.xyz = (u_ModelMatrix * vec4(LB.tangent, 0.0)).xyz;
	var_Binormal.xyz = (u_ModelMatrix * vec4(LB.binormal, 0.0)).xyz;
#endif

	var_Normal.xyz = (u_ModelMatrix * vec4(LB.normal, 0.0)).xyz;
	
	var_TexDiffuse = (u_DiffuseTextureMatrix * vec4(texCoord, 0.0, 1.0)).st;
	var_Color = color;

#if defined(USE_NORMAL_MAPPING)
	
	var_TexNormalSpecular.xy = (u_NormalTextureMatrix * vec4(texCoord, 0.0, 1.0)).st;

	
	var_TexNormalSpecular.zw = (u_SpecularTextureMatrix * vec4(texCoord, 0.0, 1.0)).st;
#endif
#if defined(USE_GLOW_MAPPING)
	var_TexGlow = (u_GlowTextureMatrix * vec4(texCoord, 0.0, 1.0)).st;
#endif
}
)" },
{ "glsl/vertexLighting_DBS_world_fp.glsl", R"(




uniform sampler2D	u_DiffuseMap;
uniform sampler2D	u_NormalMap;
uniform sampler2D	u_SpecularMap;
uniform sampler2D	u_GlowMap;

uniform float		u_AlphaThreshold;
uniform float		u_DepthScale;
uniform	float		u_LightWrapAround;
uniform vec2		u_SpecularExponent;

uniform sampler3D       u_LightGrid1;
uniform sampler3D       u_LightGrid2;
uniform vec3            u_LightGridOrigin;
uniform vec3            u_LightGridScale;

varying vec4		var_TexDiffuseGlow;
varying vec4		var_Color;

#if defined(USE_NORMAL_MAPPING)
varying vec4		var_TexNormalSpecular;
varying vec3		var_ViewDir; 
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
	vec3 L, ambCol, dirCol;
	ReadLightGrid( (var_Position - u_LightGridOrigin) * u_LightGridScale,
		       L, ambCol, dirCol);

	vec3 V = normalize(var_ViewDir);

	vec2 texNormal = var_TexNormalSpecular.st;
	vec2 texSpecular = var_TexNormalSpecular.pq;

#if defined(USE_PARALLAX_MAPPING)

	
	vec3 Vts = V;
	
	
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

	
	vec2 texOffset = S * depth;
#endif

	texDiffuse.st += texOffset;
	texNormal.st += texOffset;
	texSpecular.st += texOffset;
#endif 

	
	vec4 diffuse = texture2D(u_DiffuseMap, texDiffuse) * var_Color;

	if( abs(diffuse.a + u_AlphaThreshold) <= 1.0 )
	{
		discard;
		return;
	}

	
	vec3 N = texture2D(u_NormalMap, texNormal.st).xyw;
	N.x *= N.z;
	N.xy = 2.0 * N.xy - 1.0;
	N.z = sqrt(1.0 - dot(N.xy, N.xy));
	
	#if defined(r_NormalScale)
	N.z *= r_NormalScale;
	#endif
	
	N = normalize(N);

 	
	vec3 H = normalize(L + V);

	
#if defined(r_WrapAroundLighting)
	float NL = clamp(dot(N, L) + u_LightWrapAround, 0.0, 1.0) / clamp(1.0 + u_LightWrapAround, 0.0, 1.0);
#else
	float NL = clamp(dot(N, L), 0.0, 1.0);
#endif
	vec3 light = ambCol + dirCol * NL;
	light *= r_AmbientScale;

	
	vec4 spec = texture2D(u_SpecularMap, texSpecular).rgba;
	vec3 specular = spec.rgb * dirCol * pow(clamp(dot(N, H), 0.0, 1.0), u_SpecularExponent.x * spec.a + u_SpecularExponent.y) * r_SpecularScale;

	
	vec4 color = vec4(diffuse.rgb, 1.0);
	color.rgb *= light;
	color.rgb += specular;

	#if defined(USE_GLOW_MAPPING)
	color.rgb += texture2D(u_GlowMap, texGlow).rgb;
	#endif
#if defined(r_DeferredShading)
	gl_FragData[0] = color; 								
	gl_FragData[1] = vec4(diffuse.rgb, color.a);
	gl_FragData[2] = vec4(N, color.a);
	gl_FragData[3] = vec4(specular, color.a);
#else
	gl_FragColor = color;
#endif
#else 

	vec3 N = normalize(var_Normal);

	
	vec4 diffuse = texture2D(u_DiffuseMap, texDiffuse) * var_Color;

	if( abs(diffuse.a + u_AlphaThreshold) <= 1.0 )
	{
		discard;
		return;
	}

	vec4 color = diffuse;
#if defined(USE_GLOW_MAPPING)
	color.rgb += texture2D(u_GlowMap, texGlow).rgb;
#endif

#if 0 
	color = vec4(vec3(var_LightColor.a), 1.0);
#endif

#if defined(r_DeferredShading)
	gl_FragData[0] = color;
	gl_FragData[1] = vec4(diffuse.rgb, color.a);
	gl_FragData[2] = vec4(N, color.a);
	gl_FragData[3] = vec4(0.0, 0.0, 0.0, color.a);
#else
	gl_FragColor = color;
#endif

#endif
}
)" },
{ "glsl/vertexLighting_DBS_world_vp.glsl", R"(




uniform mat4		u_DiffuseTextureMatrix;
uniform mat4		u_NormalTextureMatrix;
uniform mat4		u_SpecularTextureMatrix;
uniform mat4		u_GlowTextureMatrix;
uniform mat4		u_ModelViewProjectionMatrix;

uniform float		u_Time;

uniform vec4		u_ColorModulate;
uniform vec4		u_Color;
uniform vec3		u_ViewOrigin;

varying vec4		var_TexDiffuseGlow;
varying vec4		var_Color;

#if defined(USE_NORMAL_MAPPING)
varying vec4		var_TexNormalSpecular;
varying vec3		var_ViewDir;
varying vec3            var_Position;
#else
varying vec3		var_Normal;
#endif

void DeformVertex( inout vec4 pos,
		   inout vec3 normal,
		   inout vec2 st,
		   inout vec4 color,
		   in    float time);

void	main()
{
	vec4 position;
	localBasis LB;
	vec2 texCoord, lmCoord;
	vec4 color;

	VertexFetch( position, LB, color, texCoord, lmCoord );

	color = color * u_ColorModulate + u_Color;

	DeformVertex( position,
		      LB.normal,
		      texCoord,
		      color,
		      u_Time);

	
	gl_Position = u_ModelViewProjectionMatrix * position;

	
	var_TexDiffuseGlow.st = (u_DiffuseTextureMatrix * vec4(texCoord, 0.0, 1.0)).st;

	
	var_Color = color;
	
#if defined(USE_NORMAL_MAPPING)
	
	var_TexNormalSpecular.st = (u_NormalTextureMatrix * vec4(texCoord, 0.0, 1.0)).st;

	
	var_TexNormalSpecular.pq = (u_SpecularTextureMatrix * vec4(texCoord, 0.0, 1.0)).st;

	
	mat3 objectToTangentMatrix = mat3( LB.tangent.x, LB.binormal.x, LB.normal.x,
					   LB.tangent.y, LB.binormal.y, LB.normal.y,
					   LB.tangent.z, LB.binormal.z, LB.normal.z );

	
	var_Position = position.xyz;
	
	
	var_ViewDir = objectToTangentMatrix * normalize( u_ViewOrigin - position.xyz );
#else
	var_Normal = LB.normal;
#endif

#if defined(USE_GLOW_MAPPING)
	var_TexDiffuseGlow.pq = ( u_GlowTextureMatrix * vec4(texCoord, 0.0, 1.0) ).st;
#endif
}
)" },
{ "glsl/vertexSimple_vp.glsl", R"(



struct localBasis {
	vec3 normal;
#if defined(USE_NORMAL_MAPPING)
	vec3 tangent, binormal;
#endif
};

vec3 QuatTransVec(in vec4 quat, in vec3 vec) {
	vec3 tmp = 2.0 * cross( quat.xyz, vec );
	return vec + quat.w * tmp + cross( quat.xyz, tmp );
}

void QTangentToLocalBasis( in vec4 qtangent, out localBasis LB ) {
	LB.normal = QuatTransVec( qtangent, vec3( 0.0, 0.0, 1.0 ) );
#if defined(USE_NORMAL_MAPPING)
	LB.tangent = QuatTransVec( qtangent, vec3( 1.0, 0.0, 0.0 ) );
	LB.tangent *= sign( qtangent.w );
	LB.binormal = QuatTransVec( qtangent, vec3( 0.0, 1.0, 0.0 ) );
#endif
}

#if !defined(USE_VERTEX_ANIMATION) && !defined(USE_VERTEX_SKINNING) && !defined(USE_VERTEX_SPRITE)

attribute vec3 attr_Position;
attribute vec4 attr_Color;
attribute vec4 attr_QTangent;
attribute vec4 attr_TexCoord0;

void VertexFetch(out vec4 position,
		 out localBasis normalBasis,
		 out vec4 color,
		 out vec2 texCoord,
		 out vec2 lmCoord)
{
	position = vec4( attr_Position, 1.0 );
	QTangentToLocalBasis( attr_QTangent, normalBasis );
	color    = attr_Color;
	texCoord = attr_TexCoord0.xy;
	lmCoord  = attr_TexCoord0.zw;
}
#endif
)" },
{ "glsl/vertexSkinning_vp.glsl", R"(



#if defined(USE_VERTEX_SKINNING)

attribute vec3 attr_Position;
attribute vec2 attr_TexCoord0;
attribute vec4 attr_Color;
attribute vec4 attr_QTangent;
attribute vec4 attr_BoneFactors;


uniform vec4 u_Bones[ 2 * MAX_GLSL_BONES ];

void VertexFetch(out vec4 position,
		 out localBasis LB,
		 out vec4 color,
		 out vec2 texCoord,
		 out vec2 lmCoord)
{
	const float scale = 1.0 / 256.0;
	localBasis inLB;

	ivec4 idx = 2 * ivec4( floor( attr_BoneFactors * scale ) );
	vec4  weights = fract( attr_BoneFactors * scale );
	weights.x = 1.0 - weights.x;

	vec4 quat = u_Bones[ idx.x ];
	vec4 trans = u_Bones[ idx.x + 1 ];

	QTangentToLocalBasis( attr_QTangent, inLB );

	position.xyz = weights.x * (QuatTransVec( quat, attr_Position ) * trans.w + trans.xyz);
	LB.normal = weights.x * (QuatTransVec( quat, inLB.normal ));
#if defined(USE_NORMAL_MAPPING)
	LB.tangent = weights.x * (QuatTransVec( quat, inLB.tangent ));
	LB.binormal = weights.x * (QuatTransVec( quat, inLB.binormal ));
#endif
	
	quat = u_Bones[ idx.y ];
	trans = u_Bones[ idx.y + 1 ];

	position.xyz += weights.y * (QuatTransVec( quat, attr_Position ) * trans.w + trans.xyz);
	LB.normal += weights.y * (QuatTransVec( quat, inLB.normal ));
#if defined(USE_NORMAL_MAPPING)
	LB.tangent += weights.y * (QuatTransVec( quat, inLB.tangent ));
	LB.binormal += weights.y * (QuatTransVec( quat, inLB.binormal ));
#endif

	quat = u_Bones[ idx.z ];
	trans = u_Bones[ idx.z + 1 ];

	position.xyz += weights.z * (QuatTransVec( quat, attr_Position ) * trans.w + trans.xyz);
	LB.normal += weights.z * (QuatTransVec( quat, inLB.normal ));
#if defined(USE_NORMAL_MAPPING)
	LB.tangent += weights.z * (QuatTransVec( quat, inLB.tangent ));
	LB.binormal += weights.z * (QuatTransVec( quat, inLB.binormal ));
#endif

	quat = u_Bones[ idx.w ];
	trans = u_Bones[ idx.w + 1 ];

	position.xyz += weights.w * (QuatTransVec( quat, attr_Position ) * trans.w + trans.xyz);
	LB.normal += weights.w * (QuatTransVec( quat, inLB.normal ));
#if defined(USE_NORMAL_MAPPING)
	LB.tangent += weights.w * (QuatTransVec( quat, inLB.tangent ));
	LB.binormal += weights.w * (QuatTransVec( quat, inLB.binormal ));
#endif

	position.w = 1.0;
	LB.normal   = normalize(LB.normal);
#if defined(USE_NORMAL_MAPPING)
	LB.tangent  = normalize(LB.tangent);
	LB.binormal = normalize(LB.binormal);
#endif
	color    = attr_Color;
	texCoord = attr_TexCoord0;
	lmCoord  = attr_TexCoord0;
}
#endif
)" },
{ "glsl/vertexSprite_vp.glsl", R"(



#if defined(USE_VERTEX_SPRITE)

attribute vec3 attr_Position;
attribute vec4 attr_Color;
attribute vec4 attr_TexCoord0;
attribute vec4 attr_Orientation;

uniform vec3 u_ViewOrigin;
uniform vec3 u_ViewUp;

float           u_DepthScale;

void VertexFetch(out vec4 position,
		 out localBasis normalBasis,
		 out vec4 color,
		 out vec2 texCoord,
		 out vec2 lmCoord)
{
	vec2 corner;
	float radius = attr_Orientation.w;
	vec3 normal = normalize( u_ViewOrigin - attr_Position ), up, left;
	float s, c; 

	corner = attr_TexCoord0.xy * 2.0 - 1.0;

	if( radius <= 0.0 ) {
		
		up = attr_Orientation.xyz;
		left = radius * normalize( cross( up, normal ) );
		position = vec4( attr_Position + corner.y * left, 1.0 );
	} else {
		
		left = normalize( cross( u_ViewUp, normal ) );
		up = cross( left, normal );

		s = radius * sin( radians( attr_Orientation.x ) );
		c = radius * cos( radians( attr_Orientation.x ) );

		
		vec3 leftOrig = left;
		left = c * left + s * up;
		up = c * up - s * leftOrig;

		left *= corner.x;
		up *= corner.y;

		position = vec4( attr_Position + left + up, 1.0 );
	}
	normalBasis.normal = normal;
#if defined(USE_NORMAL_MAPPING)
	normalBasis.tangent = normalize( up );
	normalBasis.binormal = normalize( left );
#endif
	texCoord = attr_TexCoord0.xy;
	lmCoord  = attr_TexCoord0.zw;
	color    = attr_Color;

	u_DepthScale = 2.0 * radius;
}
#endif
)" },
{ "glsl/volumetricFog_fp.glsl", R"(




uniform sampler2D	u_DepthMap;			
uniform sampler2D	u_DepthMapBack;		
uniform sampler2D	u_DepthMapFront;	
uniform vec3		u_ViewOrigin;
uniform float		u_FogDensity;
uniform vec3		u_FogColor;
uniform mat4		u_UnprojectMatrix;


float DecodeDepth(vec4 color)
{
	float depth;

	const vec4 bitShifts = vec4(1.0 / (256.0 * 256.0 * 256.0), 1.0 / (256.0 * 256.0), 1.0 / 256.0, 1.0); 	


	depth = dot(color, bitShifts);

	return depth;
}

void	main()
{
	
	vec2 st = gl_FragCoord.st * r_FBufScale;

	
	st *= r_NPOTScale;

	
	float fogDepth;

	float depthSolid = texture2D(u_DepthMap, st).r;






	float depthBack = DecodeDepth(texture2D(u_DepthMapBack, st));
	float depthFront = DecodeDepth(texture2D(u_DepthMapFront, st));


	if(depthSolid < depthFront)
	{
		discard;
		return;
	}

	if(depthSolid < depthBack)
	{
		depthBack = depthSolid;
	}

	fogDepth = depthSolid;


	
	vec4 posBack = u_UnprojectMatrix * vec4(gl_FragCoord.xy, depthBack, 1.0);
	posBack.xyz /= posBack.w;

	vec4 posFront = u_UnprojectMatrix * vec4(gl_FragCoord.xy, depthFront, 1.0);
	posFront.xyz /= posFront.w;

	if(posFront.w <= 0.0)
	{
		
		posFront = vec4(u_ViewOrigin, 1.0);
	}

	
	
	float fogDistance = distance(posBack, posFront);
	



	
	float fogExponent = fogDistance * u_FogDensity;

	
	float fogFactor = exp2(-abs(fogExponent));

	
	gl_FragColor = vec4(u_FogColor, fogFactor);
}
)" },
{ "glsl/volumetricFog_vp.glsl", R"(




attribute vec3 		attr_Position;

uniform mat4		u_ModelViewProjectionMatrix;

void	main()
{
	
	gl_Position = u_ModelViewProjectionMatrix * vec4(attr_Position, 1.0);
}
)" },
} );
