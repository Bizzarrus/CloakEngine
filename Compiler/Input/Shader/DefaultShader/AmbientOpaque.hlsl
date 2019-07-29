#include "GBuffer.hlsli"
#include "PBR.hlsli"

struct LightBuffer {
	float4 Color;
};

struct PS_INPUT {
	float4 pos : SV_POSITION;
	float2 tex : TEXCOORD0;
};

ConstantBuffer<LightBuffer> Light : register(b0);

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
	float4 Albedo;
	float4 Normal;
	float4 Specul;
	float4 Wetnes;
	float4 res = 0;

	MSAAIteration(layer)
	{
		Albedo = LoadTex(0, input.tex, input.pos, layer);
		Normal = LoadTex(1, input.tex, input.pos, layer);
		Specul = LoadTex(2, input.tex, input.pos, layer);
		Wetnes = LoadTex(3, input.tex, input.pos, layer);
		res += PBR::Ambient(Albedo, Specul.xyz, Light.Color, Specul.w, Normal.w, Wetnes.x, Wetnes.y, Wetnes.z);
	}
	return res;
}