#include "SceneMatrices.hlsli"
#include "GBufferEncoding.hlsli"

SamplerState pointSampler : register(s0);
TextureCube<float4> tex : register(t0);

struct SkyLight {
	float LightIntensity;
};
ConstantBuffer<SkyLight> Light : register(b2);

struct PS_INPUT {
	float4 pos : SV_POSITION;
	float3 dir : DIRECTION;
};

/////////////////////////////////////////////////
// Vertex Shader
/////////////////////////////////////////////////

PS_INPUT mainVS(uint id : SV_VertexID)
{
	PS_INPUT output = (PS_INPUT)0;
	float2 tex = float2((id << 1) & 2, id & 2);
	output.pos = float4((tex*float2(2, -2)) + float2(-1, 1), 0, 1);
	float4 dir = mul(output.pos, View.ProjectionToWorld);
	output.dir = (dir.xyz / dir.w) - View.ViewPos;
	return output;
}

/////////////////////////////////////////////////
// Pixel Shader
/////////////////////////////////////////////////
float4 mainPS(PS_INPUT input) : SV_Target
{
	return Light.LightIntensity*GammaCorrection::ToLinearColor(tex.SampleLevel(pointSampler, normalize(input.dir),0));
}