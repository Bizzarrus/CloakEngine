#include "ImportanceSampleGGX.hlsli"
#include "../DefaultShader/PBR.hlsli"

SamplerState texSampler : register(s0);
TextureCube<float4> tex : register(t0);

struct Radiance {
	uint NumSamples;
};

ConstantBuffer<Radiance> Info : register(b0);

struct PS_INPUT {
	float4 pos : SV_POSITION;
	float2 tex : TEXPOSITION;
};

struct PS_OUTPUT {
	float4 Front : SV_Target0;
	float4 Back : SV_Target1;
	float4 Left : SV_Target2;
	float4 Right : SV_Target3;
	float4 Top : SV_Target4;
	float4 Bottom : SV_Target5;
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

float2 CalculateColor(float roughness, float NdV, uint numSamples)
{
	float3 V = float3(sqrt(max(0, 1 - (NdV*NdV))), 0, NdV);
	float2 C = 0;
	for (uint a = 0; a < numSamples; a++)
	{
		float3 H = ImportanceSampleGGX(a, numSamples, roughness, float3(0, 0, 1));
		float3 L = (2 * dot(V, H)*H) - V;

		float NdL = saturate(L.z);
		float NdH = saturate(H.z);
		float VdH = saturate(dot(V, H));
		if (NdL > 0)
		{
			float V = PBR::V_SchlickGGX(roughness*roughness, NdV, NdL);
			float GV = V * VdH * NdL / NdH;
			float2 F = PBR::F_Schlick(VdH);
			C += F.yx * GV;
		}
	}
	return C / numSamples;
}

float2 mainPS(PS_INPUT input) : SV_Target0
{
	return CalculateColor(input.tex.y, input.tex.x, Info.NumSamples);
}