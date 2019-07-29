#include "DepthToPos.hlsli"
#include "PBR.hlsli"
#include "SceneMatrices.hlsli"

//#define CSM_DEBUG_OUTPUT
//#define SHADOW_MAP_DEBUG_OUTPUT
//#define SHADOW_DEBUG

#include "CSM.hlsli"

SHADOW_MAP_TYPE ShadowMap : register(t1, space1);

struct LightBuffer {
	float4 Direction;
	float4 Color;
	CSMData CSM;
};

struct PS_INPUT {
	float4 pos : SV_POSITION;
	float2 tex : TEXCOORD0;
};

struct PS_OUTPUT {
	float4 Diffuse : SV_Target0;
	float4 Specular : SV_Target1;
};

ConstantBuffer<LightBuffer> Light : register(b2);

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
	float3 normal;
	float Depth = 0;
	float4 Pos;
	float3 viewDir;
	float3 lightDir;
	float csm;
	float shadow;
	
	float2 noise = float2(0, 0); //TODO: Add Blue Noise

	MSAAIteration(layer)
	{
		Albedo = LoadTex(0, input.tex, input.pos, layer);
		Normal = LoadTex(1, input.tex, input.pos, layer);
		Specul = LoadTex(2, input.tex, input.pos, layer);
		Wetnes = LoadTex(3, input.tex, input.pos, layer);
		normal = NormalEncoding::Decode(Normal.xyz);

		Pos = GetWorldPositionAndDepth(View.ProjectionToWorld, input.tex, Depth);
		viewDir = normalize(View.ViewPos - Pos.xyz);
		lightDir = normalize(-Light.Direction.xyz);
#if (defined(CSM_DEBUG_OUTPUT) || defined(SHADOW_MAP_DEBUG_OUTPUT))
		output.Diffuse = CSM(Pos.xyz, Depth, Light.CSM, ShadowMap, ShadowSampler);
#elif defined(SHADOW_DEBUG)
		csm = CSM(Pos.xyz, Depth, Light.CSM, ShadowMap, ShadowSampler);
		output.Diffuse = float4(csm, csm, csm, 1);
#else
		shadow = CSM(Pos.xyz, Depth, Light.CSM, ShadowMap, noise);
		PBR::LightResult light = PBR::Light(viewDir, lightDir, normal, Albedo.xyz, Specul.xyz, Light.Color, Specul.w, Normal.w, Wetnes.x, Wetnes.y, Wetnes.z, Wetnes.w);
		output.Diffuse += shadow * light.Diffuse;
		output.Specular += shadow * light.Specular;
#endif
	}
	return output;
}