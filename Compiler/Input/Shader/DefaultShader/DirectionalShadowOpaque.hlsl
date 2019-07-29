#include "World.hlsli"

struct PS_INPUT {
	float4 pos : SV_POSITION;
};
struct DirLightBuffer {
	matrix WorldToLightProjection;
};
ConstantBuffer<DirLightBuffer> DirLight : register(b3);

#ifdef OPTION_TESSELLATION

Texture2D<float> displacement : register(t0);
SamplerState matSampler : register(s0);

struct DisplacementBuffer {
	uint ActivateFlag;
};

ConstantBuffer<DisplacementBuffer> Displacement : register(b4);

struct HS_INPUT {
	float4 pos : POSITION;
	float4 normal : NORMAL;
	float2 texCoord : TEXCOORD;
	float depth : DEPTH;
};

struct HS_INPUT_CONSTANT {
	float edges[3] : SV_TessFactor;
	float inside : SV_InsideTessFactor;
};

struct DS_INPUT {
	float4 normal : NORMAL;
	float4 pos : POSITION;
	float2 texCoord : TEXCOORD;
};

#endif

/////////////////////////////////////////////////
// Vertex Shader
/////////////////////////////////////////////////

#ifdef OPTION_TESSELLATION
HS_INPUT mainVS(VS_INPUT input)
{
	HS_INPUT output = (HS_INPUT)0;
	output.pos = mul(input.pos, Local.LocalToWorld);
	output.normal = mul(Local.WorldToLocal, float4(input.normal.xyz, 0));
	output.texCoord = input.texCoord;
	float4 pos = mul(output.pos, View.WorldToProjection);
	output.depth = pos.z / pos.w;
	return output;
}
#else
PS_INPUT mainVS(VS_INPUT_GEOMETRY input)
{
	PS_INPUT output = (PS_INPUT)0;
	output.pos = mul(mul(input.pos, Local.LocalToWorld), DirLight.WorldToLightProjection);
	output.pos /= output.pos.w;
	return output;
}
#endif

#ifdef OPTION_TESSELLATION

/////////////////////////////////////////////////
// Hull Shader
/////////////////////////////////////////////////

HS_INPUT_CONSTANT PatchFunction(InputPatch<HS_INPUT, 3> input, uint patchId : SV_PrimitiveID)
{
	HS_INPUT_CONSTANT output = (HS_INPUT_CONSTANT)0;
	output.edges[0] = 1;//TODO: Tessellation is currently deactivated (1 equals no tesselation, 0 generates no output at all)
	output.edges[1] = 1;
	output.edges[2] = 1;
	output.inside = 1;
	//TODO: If Displacement.ActivateFlag != 0 -> Activate tessellation
	return output;
}

[domain("tri")]
[partitioning("integer")] //Can be integer, fractional_even, fractional_odd, or pow2.
[outputtopology("triangle_cw")]
[outputcontrolpoints(3)]
[patchconstantfunc("PatchFunction")]
DS_INPUT mainHS(InputPatch<HS_INPUT, 3> input, uint pointId : SV_OutputControlPointID, uint patchId : SV_PrimitiveID)
{
	DS_INPUT output = (DS_INPUT)0;
	output.normal = input[pointId].normal;
	output.pos = input[pointId].pos;
	output.texCoord = input[pointId].texCoord;
	return output;
}

/////////////////////////////////////////////////
// Domain Shader
/////////////////////////////////////////////////

float WightField(float a, float b, float c, float3 uvw) { return (a*uvw.x) + (b*uvw.y) + (c*uvw.z); }
float2 WightField(float2 a, float2 b, float2 c, float3 uvw) { return (a*uvw.x) + (b*uvw.y) + (c*uvw.z); }
float3 WightField(float3 a, float3 b, float3 c, float3 uvw) { return (a*uvw.x) + (b*uvw.y) + (c*uvw.z); }
float4 WightField(float4 a, float4 b, float4 c, float3 uvw) { return (a*uvw.x) + (b*uvw.y) + (c*uvw.z); }
#define WEIGHT_PATCH(patch, location, field) WightField(patch[0].field, patch[1].field, patch[2].field, location)

[domain("tri")]
PS_INPUT mainDS(HS_INPUT_CONSTANT input, float3 uvw : SV_DomainLocation, const OutputPatch<DS_INPUT, 3> patch)
{
	PS_INPUT output = (PS_INPUT)0;
	float4 pos = WEIGHT_PATCH(patch, uvw, pos);
	float4 nor = WEIGHT_PATCH(patch, uvw, normal);
	float2 tex = WEIGHT_PATCH(patch, uvw, texCoord);

	//float4 cpos = mul(pos, View.WorldToProjection);
	output.pos = mul(pos, DirLight.WorldToLightProjection);
	output.pos /= output.pos.w;
	//TODO: If Displacement.ActivateFlag != 0 -> Sample displacement from materials[3] using tex (texCoord) and nor (surface normal)
	return output;
}

#undef WEIGHT_PATCH

#endif

/////////////////////////////////////////////////
// Pixel Shader
/////////////////////////////////////////////////

float4 mainPS(PS_INPUT input) : SV_Target
{
	return float4(0, 0, 0, 0);
}