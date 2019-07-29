#include "SceneMatrices.hlsli"

struct VS_INPUT {
	float4 pos : POSITION;
	float4 normal : NORMAL;
	float4 binormal : BINORMAL;
	float4 tangent : TANGENT;
	float4 boneW : BONEWEIGHT;
	uint4 boneID : BONEID;
	float2 texCoord : TEXCOORD;
};
struct VS_INPUT_GEOMETRY {
	float4 pos : POSITION;
};
struct ObjectBuffer {
	matrix LocalToWorld;
	matrix WorldToLocal;
	float Wetness;
};
ConstantBuffer<ObjectBuffer> Local : register(b2);