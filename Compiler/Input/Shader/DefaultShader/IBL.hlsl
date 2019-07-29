#include "DepthToPos.hlsli"
#include "SceneMatrices.hlsli"
#include "PBR.hlsli"

struct IrradianceBuffer {
	matrix Color[3];
	uint FirstSumMipMaps;
};
struct SettingsBuffer {
	float RadianceScale;
	float IrradianceScale;
};

struct PS_INPUT {
	float4 pos : SV_POSITION;
	float2 tex : TEXCOORD0;
};

struct PS_OUTPUT {
	float4 Diffuse : SV_Target0;
	float4 Specular : SV_Target1;
};

SamplerState texSampler : register(s1);
ConstantBuffer<IrradianceBuffer> Irradiance : register(b0, space1);
ConstantBuffer<SettingsBuffer> Settings : register(b1, space1);
Texture2D<float2> LUT : register(t0, space2);
TextureCube<float4> FirstSum : register(t1, space2);

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
PS_OUTPUT mainPS(PS_INPUT input)
{
	PS_OUTPUT output = (PS_OUTPUT)0;

	float4 Albedo;
	float4 Normal;
	float4 Specul;
	float4 Wetnes;
	float Depth;
	float4 Pos;
	float3 V;
	float4 N;
	float NdV;
	float Roughness;
	float3 irr;
	float3 firstSum;
	float2 lut;
	PBR::LightResult light;

	MSAAIteration(layer)
	{
		Albedo = LoadTex(0, input.tex, input.pos, layer);
		Normal = LoadTex(1, input.tex, input.pos, layer);
		Specul = LoadTex(2, input.tex, input.pos, layer);
		Wetnes = LoadTex(3, input.tex, input.pos, layer);

		Depth = 0;
		Pos = GetWorldPositionAndDepth(View.ProjectionToWorld, input.tex, Depth);
		V = normalize(View.ViewPos - Pos.xyz);
		N = float4(NormalEncoding::Decode(Normal.xyz), 1);
		NdV = dot(N.xyz, V);
		Roughness = Specul.w;

		irr = 0;
		irr.x = dot(mul(N, Irradiance.Color[0]), N);
		irr.y = dot(mul(N, Irradiance.Color[1]), N);
		irr.z = dot(mul(N, Irradiance.Color[2]), N);

		firstSum = FirstSum.SampleLevel(texSampler, (2 * NdV * N.xyz) - V, Roughness * (Irradiance.FirstSumMipMaps - 1)).rgb;
		lut = LUT.Sample(texSampler, float2(NdV, Roughness));

		if (dot(Normal.xyz, Normal.xyz) > 0)
		{
			light = PBR::IBL(Albedo.xyz, max(0, irr), lut, firstSum, Specul.xyz, Normal.w, max(0, NdV), Wetnes.y, Wetnes.z, Wetnes.x, Roughness);
			output.Diffuse += Settings.IrradianceScale * light.Diffuse;
			output.Specular += Settings.RadianceScale * light.Specular;
		}
	}

	return output;
}