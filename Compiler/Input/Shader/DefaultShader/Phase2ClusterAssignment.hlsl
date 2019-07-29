/////////////////////////////////////////////////
// Cluster Assignment:
//	Reads the depth buffer generated during 
//	rendering and flags the clusteres that
//	have geometry inside them (acording to
//	the depth buffer)
/////////////////////////////////////////////////

#define NO_GBUFFER
#include "DepthToPos.hlsli"
#include "SceneMatrices.hlsli"
#include "Clustering.hlsli"

struct ClusterGenerationBuffer {
	float NearPlane;
	float FarPlane;
};

ConstantBuffer<ClusterGenerationBuffer> ClusterGeneration : register(b0, space1);
RWTexture3D<uint> Clusters : register(u0);

struct PS_INPUT {
	float4 pos : SV_POSITION;
	float2 tex : TEXCOORD0;
};

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
void mainPS(PS_INPUT input)
{
	float d = 0;
	float4 p = 0;
	uint3 k = 0;
	MSAAIteration(layer)
	{
		d = LoadDepth(input.tex, layer);
		p = DepthToPos(Projection.ProjectionToView, input.tex, d);
		k = ClusterKeys(input.tex, p.z, ClusterGeneration.NearPlane, ClusterGeneration.FarPlane);
		InterlockedOr(Clusters[k], 0xFFFFFFFF);
	}
}