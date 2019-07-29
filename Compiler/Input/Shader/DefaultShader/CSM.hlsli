//Cascade shadow map algorithm
#define SHADOW_DECODE
#include "ShadowMap.hlsli"

#define CSM_CASCADE_COUNT 4
#define CSM_CASCADE_COUNT_QUATER (CSM_CASCADE_COUNT >> 2)

struct CSMData {
	float4 NCS[CSM_CASCADE_COUNT_QUATER];
	float4 LCS[CSM_CASCADE_COUNT_QUATER];
	matrix WorldToLight[CSM_CASCADE_COUNT];
};

#if (defined(CSM_DEBUG_OUTPUT) || defined(SHADOW_MAP_DEBUG_OUTPUT))
float4 CSM(float3 WorldPos, float depth, CSMData data, SHADOW_MAP_TYPE shadowMap, float2 noise)
#else
float CSM(float3 WorldPos, float depth, CSMData data, SHADOW_MAP_TYPE shadowMap, float2 noise)
#endif
{
	depth = 1 - depth;
	float lcs = 0;
	float ncs = 0;
	uint cascade = CSM_CASCADE_COUNT;
	[unroll]
	for (uint a = CSM_CASCADE_COUNT_QUATER; a > 0; a--)
	{
		uint b = a - 1;
		uint c = b << 2;
		if (depth <= data.NCS[b].x)
		{
			cascade = c + 0;
			ncs = data.NCS[b].x;
			lcs = data.LCS[b].x;
		}
		else if (depth <= data.NCS[b].y)
		{
			cascade = c + 1;
			ncs = data.NCS[b].y;
			lcs = data.LCS[b].y;
		}
		else if (depth <= data.NCS[b].z)
		{
			cascade = c + 2;
			ncs = data.NCS[b].z;
			lcs = data.LCS[b].z;
		}
		else if (depth <= data.NCS[b].w)
		{
			cascade = c + 3;
			ncs = data.NCS[b].w;
			lcs = data.LCS[b].w;
		}
	}
#if (defined(CSM_DEBUG_OUTPUT) || defined(SHADOW_MAP_DEBUG_OUTPUT))
	float4 res = float4(0, 0, 0, 0);
#else
	float res = 1;
#endif
	if (cascade < CSM_CASCADE_COUNT)
	{
		if (cascade + 1 < CSM_CASCADE_COUNT && depth > lcs)
		{
			float blend = (depth - lcs) / (ncs - lcs);
#if defined(CSM_DEBUG_OUTPUT)
			float4 l = float4(cascade & 0x1, cascade & 0x2, cascade == 0, 1);
			float4 r = float4((cascade + 1) & 0x1, (cascade + 1) & 0x2, (cascade + 1) == 0, 1);
#elif defined(SHADOW_MAP_DEBUG_OUTPUT)
			float4 l = Shadow(WorldPos, data.WorldToLight[cascade], shadowMap, noise);
			float4 r = Shadow(WorldPos, data.WorldToLight[cascade + 1], shadowMap, noise);
#else
			float l = Shadow(WorldPos, data.WorldToLight[cascade], shadowMap, noise);
			float r = Shadow(WorldPos, data.WorldToLight[cascade + 1], shadowMap, noise);
#endif
			res = lerp(l, r, blend);
		}
		else
		{
#if defined(CSM_DEBUG_OUTPUT)
			res = float4(cascade & 0x1, cascade & 0x2, cascade == 0, 1);
#else
			res = Shadow(WorldPos, data.WorldToLight[cascade], shadowMap, noise);
#endif
		}
	}
	return res;
}