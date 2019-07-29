#define TONEMAPPING_ACES_SIMPLE

//Tone mapping factors
namespace ToneMapping {

#define MIN_LUMINANCE 0.03f
#define MAX_LUMINANCE 5.9f

#ifdef TONEMAPPING_ACES_SIMPLE

#define TM_A 2.51f
#define TM_B 0.03f
#define TM_C 2.43f
#define TM_D 0.59f
#define TM_E 0.14f

	float3 Apply(float3 x)
	{
		//We use simplified ACES from https://knarkowicz.wordpress.com/2016\01\06/aces-filmic-tone-mapping-curve/
		const float3 a = x*((TM_A*x) + TM_B);
		const float3 b = 1 / ((x*((TM_C*x) + TM_D)) + TM_E);
		return saturate(a*b);
	}

#elif defined(TONEMAPPING_ACES)

#define TM_SRGB_TO_RRT float3x3(	 0.59719f,  0.35458f,  0.04823f, \
								 0.07600f,  0.90834f,  0.01566f, \
								 0.02840f,  0.13383f,  0.83777f)

#define TM_ODT_TO_SRGB float3x3(	 1.60475f, -0.53108f, -0.07367f, \
								-0.10208f,  1.10813f, -0.00605f, \
								-0.00327f, -0.07276f, 1.07602f)

#define TM_A 0.0245786f
#define TM_B -0.000090537f
#define TM_C 0.983729f
#define TM_D 0.4329510f
#define TM_E 0.238081f

	float3 Apply(float3 x)
	{
		//Use ACES from https://github.com/TheRealMJP/BakingLab/blob/master/BakingLab/ACES.hlsl
		x = mul(TM_SRGB_TO_RRT, x);
		float3 a = (x*(x + TM_A)) + TM_B;
		float3 b = (x*((x*TM_C) + TM_D)) + TM_E;
		x = a / b;
		return mul(TM_ODT_TO_SRGB, x);
	}

#elif defined(TONEMAPPING_UNCHARTED)

	/*
#define TM_A 0.15f
#define TM_B 0.50f
#define TM_C 0.10f
#define TM_D 0.20f
#define TM_E 0.02f
#define TM_F 0.30f
//#define TM_W 11.2f
#define TM_W 7.0f
*/

#define TM_A 0.63f
#define TM_B 0.01f
#define TM_C 0.63f
#define TM_D 0.15f
#define TM_E 0.02f
#define TM_F 1.16f
#define TM_W 3.62f

	float3 Apply(float3 x)
	{
		//Use Uncharted 2 Tone mapping operator
		float4 y = float4(x, TM_W);
		float4 a = y * TM_A;
		float4 b = (y * (a + (TM_B * TM_C))) + (TM_D * TM_E);
		float4 c = (y * (a + TM_B)) + (TM_D * TM_F);
		float4 d = (b / c) - (TM_E / TM_F);
		return r.xyz / r.w;
	}

#endif

	float Exposure(float avLuminance)
	{
		float lum = clamp(exp(avLuminance), MIN_LUMINANCE, MAX_LUMINANCE);
		float keyVal = 1.03f - (2.0f / (2.0f + log10(lum + 1)));
		return keyVal / lum;
	}

	float3 Apply(float3 color, float avLuminance)
	{
		return Apply(Exposure(avLuminance)*color);
	}
}