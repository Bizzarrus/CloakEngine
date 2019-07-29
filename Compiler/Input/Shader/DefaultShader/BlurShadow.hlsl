
#define SHADOW_BLUR
#include "ShadowMap.hlsli"

SHADOW_MAP_TYPE Source : register(t0);
SHADOW_MAP_OUTPUT Destination : register(u0);

[numthreads(THREAD_COUNT_X, THREAD_COUNT_Y, THREAD_COUNT_Z)]
void mainCS(uint3 groupID : SV_GroupID, uint3 groupThreadID : SV_GroupThreadID)
{
	Shadow(Destination, Source, groupID, groupThreadID);
}