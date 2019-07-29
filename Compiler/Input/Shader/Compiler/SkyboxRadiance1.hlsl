#include "ImportanceSampleGGX.hlsli"
#include "../DefaultShader/PBR.hlsli"
#include "../DefaultShader/GBufferEncoding.hlsli"

SamplerState texSampler : register(s0);
TextureCube<float4> tex : register(t0);

struct Radiance {
	float Roughness;
	uint NumSamples;
	float SampleRate; //4 * PI / (6 * tex[0].W * tex[0].W)
};

ConstantBuffer<Radiance> Info : register(b0);

struct PS_INPUT {
	float4 pos : SV_POSITION;
	float2 tex : TEXPOSITION;
};

struct PS_OUTPUT {
	float4 Right : SV_Target0;
	float4 Left : SV_Target1;
	float4 Top : SV_Target2;
	float4 Bottom : SV_Target3;
	float4 Front : SV_Target4;
	float4 Back : SV_Target5;
};

/////////////////////////////////////////////////
// Vertex Shader
/////////////////////////////////////////////////

PS_INPUT mainVS(uint id : SV_VertexID)
{
	PS_INPUT output = (PS_INPUT)0;
	float2 tex = float2((id << 1) & 2, id & 2);
	output.pos = float4((tex*float2(2, -2)) + float2(-1, 1), 0, 1);
	output.tex = tex;
	return output;
}

/////////////////////////////////////////////////
// Pixel Shader
/////////////////////////////////////////////////

float4 CalculateColor(float3 R, uint numSamples, float roughness, float sampleRate)
{
	float weight = 0;
	float4 color = 0;
	for (uint a = 0; a < numSamples; a++)
	{
		float3 H = ImportanceSampleGGX(a, numSamples, roughness, R); //R = N = V
		float3 L = normalize((2 * dot(R, H) * H) - R);
		float NdL = dot(L, R);
		float NdH = saturate(dot(R, H));
		if (NdL > 0)
		{
			float D = PBR::D_GGX(roughness*roughness, NdH);
			float pdf = D * 0.25f;
			float sas = rcp(numSamples * pdf);
			float mipLevel = roughness == 0 ? 0 : (0.5f * log2(sas / sampleRate));

			color += GammaCorrection::ToLinearColor(max(0,tex.SampleLevel(texSampler, L, mipLevel))) * NdL;
			weight += NdL;
		}
	}
	return weight == 0 ? 0 : color / weight;
}

PS_OUTPUT mainPS(PS_INPUT input)
{
	PS_OUTPUT output = (PS_OUTPUT)0;
	float2 sp = float2((input.tex.x * 2) - 1, 1 - (2 * input.tex.y));
	float3 frontDir = normalize(float3(sp, 1));
	float3 backDir = normalize(float3(-sp.x, sp.y, -1));
	float3 rightDir = normalize(float3(1, sp.y, -sp.x));
	float3 leftDir = normalize(float3(-1, sp.yx));
	float3 topDir = normalize(float3(sp.x, 1, -sp.y));
	float3 bottomDir = normalize(float3(sp.x, -1, sp.y));
	
	output.Front = CalculateColor(frontDir, Info.NumSamples, Info.Roughness, Info.SampleRate);
	output.Back = CalculateColor(backDir, Info.NumSamples, Info.Roughness, Info.SampleRate);
	output.Left = CalculateColor(leftDir, Info.NumSamples, Info.Roughness, Info.SampleRate);
	output.Right = CalculateColor(rightDir, Info.NumSamples, Info.Roughness, Info.SampleRate);
	output.Top = CalculateColor(topDir, Info.NumSamples, Info.Roughness, Info.SampleRate);
	output.Bottom = CalculateColor(bottomDir, Info.NumSamples, Info.Roughness, Info.SampleRate);
	return output;
}