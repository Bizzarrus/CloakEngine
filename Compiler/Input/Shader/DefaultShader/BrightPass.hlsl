#include "ToneMapFilter.hlsli"
SamplerState Sampler : register(s0);
Texture2D<float4> tex : register(t0);
StructuredBuffer<float> Luminance : register(t1);

#define LUMINANCE float4(0.2126f, 0.7152f, 0.0722f, 0)
#define LUMINANCE_CLAMP float3(1024.0f, 1024.0f, 1024.0f)

struct BrightBuffer {
	float Threshold;
};

struct PS_INPUT {
	float4 pos : SV_POSITION;
	float2 tex : TEXCOORD0;
};

ConstantBuffer<BrightBuffer> Bloom : register(b0);

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
	float exposure = ToneMapping::Exposure(Luminance.Load(0));
	float4 linColor = exposure * tex.Sample(Sampler, input.tex);
	float L = max(0, dot(linColor, LUMINANCE));
	if (L > 0)
	{
		linColor /= L;
		L = max(0, L - Bloom.Threshold) / (1 - Bloom.Threshold);
	}
	float3 col = min(L * linColor.xyz, LUMINANCE_CLAMP);
	return float4(col, 1);
}