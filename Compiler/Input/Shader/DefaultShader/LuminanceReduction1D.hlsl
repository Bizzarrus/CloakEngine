#define THREAD_COUNT 128
#define WAVE_COUNT ((THREAD_COUNT+LANE_COUNT-1)/LANE_COUNT)

#define NO_BRANCHING

struct ReductionInfo {
	uint NumberToReduce;
};

StructuredBuffer<float> Source : register(t0);
RWStructuredBuffer<float> Destination : register(u0);
ConstantBuffer<ReductionInfo> Data : register(b0);

#if ((TARGET & TARGET_TYPE_DX) != 0 && TARGET >= TARGET_DX_6_0)

groupshared float TotalSum[WAVE_COUNT];
groupshared uint TotalDiv[WAVE_COUNT];

#else

groupshared float TotalSum[THREAD_COUNT];
groupshared uint TotalDiv[THREAD_COUNT];

#endif

[numthreads(THREAD_COUNT,1,1)]
void mainCS(uint3 groupID : SV_GroupID, uint3 dispatchID : SV_DispatchThreadID, uint groupIndex : SV_GroupIndex)
{
	float l = Source.Load(dispatchID.x);
#if ((TARGET & TARGET_TYPE_DX) != 0 && TARGET >= TARGET_DX_6_0)
	float d = 1;
	if (dispatchID.x >= Data.NumberToReduce) { l = 0; d = 0; }
	float tl = WaveActiveSum(l);
	float td = WaveActiveSum(d);
	if (WaveIsFirstLane())
	{
		TotalSum[groupIndex / LANE_COUNT] = tl;
		TotalDiv[groupIndex / LANE_COUNT] = td;
	}
	uint k = WAVE_COUNT >> 1;
#else
	if (dispatchID.x < Data.NumberToReduce)
	{
		TotalSum[groupIndex] = l;
		TotalDiv[groupIndex] = 1;
	}
	else
	{
		TotalSum[groupIndex] = 0;
		TotalDiv[groupIndex] = 0;
	}
	uint k = THREAD_COUNT >> 1;
#endif
	GroupMemoryBarrierWithGroupSync();

	[unroll]
	for (; k > LANE_COUNT; k >>= 1)
	{
		if (groupIndex < k)
		{
			TotalSum[groupIndex] += TotalSum[groupIndex + k];
			TotalDiv[groupIndex] += TotalDiv[groupIndex + k];
		}
		GroupMemoryBarrierWithGroupSync();
	}
	if (groupIndex < LANE_COUNT)
	{
		for (; k > 0; k >>= 1)
		{
			TotalSum[groupIndex] += TotalSum[groupIndex + k];
			TotalDiv[groupIndex] += TotalDiv[groupIndex + k];
		}
		if (groupIndex == 0) { Destination[groupID.x] = TotalSum[0] / TotalDiv[0]; }
	}
}