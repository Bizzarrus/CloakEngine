#include "GBufferEncoding.hlsli"
#include "ToneMapFilter.hlsli"

SamplerState PointSampler : register(s0);
SamplerState BloomSampler : register(s1);
Texture2D<float4> TexScene : register(t0);
StructuredBuffer<float> Luminance : register(t1);
Texture2D<float4> Bloom : register(t2);

struct ToneBuffer {
	float Gamma; //Default value is 0.45f (1/2.2f to be exact).
	uint DisplayCurve;
	float StandardNits; //Refrence brithness level of display
	float BloomBoost;
};

#define DISPLAY_CURVE_SRGB 0
#define DISPLAY_CURVE_HDR 1

#define ST2084_MAX_FACTOR (1 / 10000.0f)

#define RGB_TO_REC2020 float3x3(	0.627402f, 0.329292f, 0.043306f,	\
									0.069095f, 0.919544f, 0.011360f,	\
									0.016394f, 0.088028f, 0.895578f		)

//ST2084 factors
#define ST_A 0.1593017578125f
#define ST_B 78.84375f
#define ST_C 0.8359375f
#define ST_D 18.8515625f
#define ST_E 18.6875f

ConstantBuffer<ToneBuffer> Tone : register(b0);

struct PS_INPUT {
	float4 pos : SV_POSITION;
	float2 tex : TEXCOORD0;
};

float3 ApplyST2084(float3 x)
{
	const float3 a = pow(abs(x), ST_A);
	const float3 b = (a*ST_D) + ST_C;
	const float3 c = 1 / ((a*ST_E) + 1);
	return pow(b*c, ST_B);
}

/////////////////////////////////////////////////
// Vertex Shader
/////////////////////////////////////////////////

PS_INPUT mainVS(uint id : SV_VertexID)
{
	PS_INPUT output = (PS_INPUT)0;
	output.tex = float2((id << 1) & 2, id & 2);
	output.pos = float4((output.tex*float2(2, -2)) + float2(-1, 1), 0, 1);
	return output;
}

/////////////////////////////////////////////////
// Pixel Shader
/////////////////////////////////////////////////
float4 mainPS(PS_INPUT input) : SV_Target
{
	float4 scene = TexScene.Sample(PointSampler, input.tex);
	float4 bloom = Bloom.Sample(BloomSampler, input.tex);
	float3 linColor = scene.rgb;

	if (Tone.DisplayCurve == DISPLAY_CURVE_SRGB)
	{
		float exposure = ToneMapping::Exposure(Luminance.Load(0));
		float3 col = ToneMapping::Apply(lerp(exposure * linColor, bloom.xyz, Tone.BloomBoost));
		//Gamma correction:
		const float4 lCol = float4(col, 1);
		return GammaCorrection::ApplyGamma(lCol, Tone.Gamma);
	}
	else
	{
		//Gamma correction:
		const float4 lCol = float4(linColor, 1);
		const float3 corColor = GammaCorrection::ApplyGamma(lCol, Tone.Gamma).rgb;
		//Convert to ST2084 (HDR10):
		const float scale = Tone.StandardNits * ST2084_MAX_FACTOR;
		return float4(ApplyST2084(scale*mul(RGB_TO_REC2020, corColor)), 1);
	}
}