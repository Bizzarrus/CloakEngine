#include "World.hlsli"
#include "GBufferEncoding.hlsli"

#define NORMAL_MAPPING
/*
Material Texture Layout :
[R	G	B	A ]	Albedo
[Nx	Ny	P	AO]	Normal Map X and Y + Porosity + Ambient Occlusion shadowing factor
[Sr	Sg	Sb	Rg]	Specular Color + Roughness 
*/
Texture2D<float4> material[3] : register(t0);
SamplerState matSampler : register(s0);

struct WetnessBuffer {
	float WorldWetness;
	float WaterSpecular;		//F0 (Water - Air)
	float FluidRefractionFactor;//IOR-Air / IOR-Water
};

ConstantBuffer<WetnessBuffer> Fluid : register(b3);

#ifdef OPTION_TESSELLATION
Texture2D<float> displacement : register(t3);

struct DisplacementBuffer {
	uint ActivateFlag;
};
ConstantBuffer<DisplacementBuffer> Displacement : register(b4);

struct HS_INPUT {
	float4 pos : POSITION;
	float4 normal : NORMAL;
	float4 binormal : BINORMAL;
	float4 tangent : TANGENT;
	float2 texCoord : TEXCOORD;
	float depth : DEPTH;
	float wetness : WETNESS;
};

struct HS_INPUT_CONSTANT {
	float edges[3] : SV_TessFactor;
	float inside : SV_InsideTessFactor;
	uint enabled : TESENABLED;
};

struct DS_INPUT {
	float4 normal : NORMAL;
	float4 binormal : BINORMAL;
	float4 tangent : TANGENT;
	float4 pos : POSITION;
	float2 texCoord : TEXCOORD;
	float wetness : WETNESS;
};

#endif

struct PS_INPUT {
	float4 pos : SV_POSITION;
	float4 normal : NORMAL;
	float4 binormal : BINORMAL;
	float4 tangent : TANGENT;
	float2 texCoord : TEXCOORD;
	float wetness : WETNESS;
};

struct PS_OUTPUT {
	float4 Albedo : SV_Target0;
	float4 Normal : SV_Target1;
	float4 Specular : SV_Target2;
	float4 Wetness : SV_Target3;
	uint Coverage : SV_Target4;
};



/////////////////////////////////////////////////
// Vertex Shader
/////////////////////////////////////////////////
#ifdef OPTION_TESSELLATION
HS_INPUT mainVS(VS_INPUT input)
{
	HS_INPUT output = (HS_INPUT)0;
#else
PS_INPUT mainVS(VS_INPUT input)
{
	PS_INPUT output = (PS_INPUT)0;
#endif
	//TODO: Bone transformation
	float4 worldPos = mul(input.pos, Local.LocalToWorld);
	float4 screenPos = mul(worldPos, View.WorldToProjection);
	output.binormal = mul(float4(input.binormal.xyz, 0), Local.LocalToWorld);
	output.tangent = mul(float4(input.tangent.xyz, 0), Local.LocalToWorld);
	output.normal = mul(Local.WorldToLocal, float4(input.normal.xyz, 0));
	output.texCoord = input.texCoord;
	output.wetness = Local.Wetness;
#ifdef OPTION_TESSELLATION
	output.pos = worldPos;
	output.depth = screenPos.z / screenPos.w;
#else
	output.pos = screenPos;
#endif
	return output;
}

#ifdef OPTION_TESSELLATION

/////////////////////////////////////////////////
// Hull Shader
/////////////////////////////////////////////////
HS_INPUT_CONSTANT HullPatch(InputPatch<HS_INPUT, 3> input, uint patchId : SV_PrimitiveID)
{
	HS_INPUT_CONSTANT output = (HS_INPUT_CONSTANT)0;
	output.edges[0] = 1;
	output.edges[1] = 1;
	output.edges[2] = 1;
	output.inside = 1;
	output.enabled = 0;
	if (Displacement.ActivateFlag != 0)
	{
		//TODO: Increase factors based on depth
		//output.enabled = 1;
	}
	return output;
}

[domain("tri")]
[partitioning("integer")] //Can be integer, fractional_even, fractional_odd, or pow2.
[outputtopology("triangle_cw")]
[outputcontrolpoints(3)]
[patchconstantfunc("HullPatch")]
DS_INPUT mainHS(InputPatch<HS_INPUT, 3> input, uint pointId : SV_OutputControlPointID, uint patchId : SV_PrimitiveID)
{
	DS_INPUT output = (DS_INPUT)0;
	output.pos = input[pointId].pos;
	output.normal = input[pointId].normal;
	output.binormal = input[pointId].binormal;
	output.tangent = input[pointId].tangent;
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
	float4 normal = WEIGHT_PATCH(patch, uvw, normal);
	float4 binormal = WEIGHT_PATCH(patch, uvw, binormal);
	float4 tangent = WEIGHT_PATCH(patch, uvw, tangent);
	float2 texCoord = WEIGHT_PATCH(patch, uvw, texCoord);

	if (input.enabled != 0)
	{
		//float offset = (2 * displacement.Sample(matSampler, texCoord)) - 1;
		//pos += normal * offset;
	}

	output.pos = mul(pos, View.WorldToProjection);
	output.normal = normal;
	output.binormal = binormal;
	output.tangent = tangent;
	output.texCoord = texCoord;
	return output;
}

#undef WEIGHT_PATCH

#endif

/////////////////////////////////////////////////
// Pixel Shader
/////////////////////////////////////////////////
PS_OUTPUT mainPS(PS_INPUT input, uint coverage : SV_Coverage)
{
	PS_OUTPUT output = (PS_OUTPUT)0;
	float4 col = material[0].Sample(matSampler, input.texCoord);
	float4 norm = material[1].Sample(matSampler, input.texCoord);
	float4 spec = material[2].Sample(matSampler, input.texCoord);

	float wetLevel = clamp(input.wetness + Fluid.WorldWetness, 0, 1);

	output.Albedo = GammaCorrection::ToLinearColor(col);
#ifdef NORMAL_MAPPING
	float3 normMap = float3((2 * norm.xy) - 1, 0);
	normMap.z = sqrt(1 - ((normMap.x*normMap.x) + (normMap.y*normMap.y)));

	float3 normal = normalize(input.normal.xyz);
	float3 binormal = normalize(input.binormal.xyz);
	float3 tangent = normalize(input.tangent.xyz);
	output.Normal = float4(NormalEncoding::Encode((normMap.x*tangent) + (normMap.y*binormal) + (normMap.z*normal)), norm.w);
#else
	output.Normal = float4(NormalEncoding::Encode(input.normal.xyz), norm.w);
#endif
	output.Specular = float4(spec.xyz, PBREncoding::Roughness(spec.w));
	output.Wetness = float4(Fluid.WaterSpecular, norm.z, wetLevel, Fluid.FluidRefractionFactor);
	output.Coverage = coverage;
	return output;
}