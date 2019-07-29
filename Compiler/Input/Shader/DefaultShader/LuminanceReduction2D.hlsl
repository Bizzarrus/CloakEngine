#define BLOCK_SIZE_X 8
#define BLOCK_SIZE_Y 8
#define BLOCK_SIZE (BLOCK_SIZE_X * BLOCK_SIZE_Y)

struct ReductionData {
	uint2 TextureSize;
	float2 BlendFactors;
	uint DispatchWidth;
};

Texture2D<float4> Source : register(t0);
StructuredBuffer<float> LastLuminance : register(t1);
RWStructuredBuffer<float> Destination : register(u0);
ConstantBuffer<ReductionData> Data : register(b0);

groupshared float TotalSum[BLOCK_SIZE];
groupshared uint TotalDiv[BLOCK_SIZE];

#define LUMINANCE float4(0.2126f, 0.7152f, 0.0722f, 0)
#define EPSILON 2.3e-5f

[numthreads(BLOCK_SIZE_X,BLOCK_SIZE_Y,1)]
void mainCS(uint3 groupID : SV_GroupID, uint3 dispatchID : SV_DispatchThreadID, uint groupIndex : SV_GroupIndex)
{
	float llum = LastLuminance.Load(0);
	uint3 loc[4];
	uint2 locPos = dispatchID.xy * 2;
	loc[0] = uint3(locPos, 0);
	loc[1] = uint3(locPos + uint2(1, 0), 0);
	loc[2] = uint3(locPos + uint2(0, 1), 0);
	loc[3] = uint3(locPos + uint2(1, 1), 0);

	float s = 0;
	float4 scd = 0;
	float lum = 0;
	uint d = 0;
	[unroll]
	for (uint a = 0; a < 4; a++)
	{
		scd = Source.Load(loc[a]);
		if (loc[a].x < Data.TextureSize.x && loc[a].y < Data.TextureSize.y)
		{
			lum = log(dot(scd, LUMINANCE) + EPSILON);
			s += (llum*Data.BlendFactors.x)+(lum*Data.BlendFactors.y);
			d++;
		}
	}

	TotalSum[groupIndex] = s;
	TotalDiv[groupIndex] = d;

	GroupMemoryBarrierWithGroupSync();

	uint k = BLOCK_SIZE >> 1;
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
		if (groupIndex == 0) { Destination[(groupID.y*Data.DispatchWidth) + groupID.x] = TotalSum[0] / TotalDiv[0]; }
	}
}