#define THREAD_COUNT 128

#include "Clustering.hlsli"

RWByteAddressBuffer IndexBuffer : register(u0);					//Current index to write to in index buffer

groupshared uint IndexCount[THREAD_COUNT];						//Number of indices each thread wants to register
groupshared uint StartIndex;									//Already registered indices (by other groups)
#if ((THREAD_COUNT / LANE_COUNT) > LANE_COUNT)
groupshared uint WaveSize[(THREAD_COUNT / LANE_COUNT) * 2];		//Number of indices each wave wants to register
#else
groupshared uint WaveSize[THREAD_COUNT / LANE_COUNT];
#endif

Texture3D<uint> Clusters : register(t0);						//Table of used clusteres
RWTexture1D<uint> ClusterIDs : register(u1);					//Resulting list of cluster indices

[numthreads(THREAD_COUNT, 1, 1)]
void mainCS(uint3 groupID : SV_DispatchThreadID, uint groupIndex : SV_GroupIndex)
{
	uint3 keys = ClusterKeys(groupID.x);
	uint r = 0;
	uint a;
	uint iTmp;
	[flatten]
	if (keys.z < CLUSTER_COUNT_Z) { r = (Clusters[keys] != 0 ? 1 : 0); }
	IndexCount[groupIndex] = r;

	//Integrate over index count:
	uint waveID = groupIndex / LANE_COUNT; //ID of wave this thread runs on
	uint waveOffset = waveID * LANE_COUNT; //Number of threads before this wave
	uint waveIndex = groupIndex - waveOffset; //ID of thread within the current wave

#if ((TARGET & TARGET_TYPE_DX) != 0 && TARGET >= TARGET_DX_6_0)
	[branch]
	if (WaveAnyTrue(r > 0))
	{
#endif
		[unroll]
		for (a = 1; a < LANE_COUNT; a <<= 1)
		{
			iTmp = 0;
			[flatten]
			if (waveIndex >= a)
			{
				iTmp += IndexCount[groupIndex - a];
			}
			IndexCount[groupIndex] += iTmp;
		}
#if ((TARGET & TARGET_TYPE_DX) != 0 && TARGET >= TARGET_DX_6_0)
	}
#endif
	if (waveIndex == 0)
	{
		WaveSize[waveID] = IndexCount[waveOffset + LANE_COUNT - 1];
	}
	GroupMemoryBarrierWithGroupSync();

	uint rof = 0;
#if ((THREAD_COUNT / LANE_COUNT) > LANE_COUNT)
	uint wof = THREAD_COUNT / LANE_COUNT;
	uint tof;
	if (groupIndex < THREAD_COUNT / LANE_COUNT)
	{
		iTmp = WaveSize[groupIndex];
		[flatten]
		if (groupIndex >= 1)
		{
			iTmp += WaveSize[groupIndex - 1];
		}
		WaveSize[groupIndex + wof] = iTmp;
	}
	tof = rof;
	rof = wof;
	wof = tof;

	uint nix;
	[unroll]
	for (a = 2; a < THREAD_COUNT / LANE_COUNT; a <<= 1)
	{
		GroupMemoryBarrierWithGroupSync();
		nix = groupIndex + rof;
		if (groupIndex < THREAD_COUNT / LANE_COUNT)
		{
			iTmp = WaveSize[nix];
			[flatten]
			if (groupIndex >= a)
			{
				iTmp += WaveSize[nix - a];
			}
			WaveSize[groupIndex + wof] = iTmp;
		}
		tof = rof;
		rof = wof;
		wof = tof;
	}
#else
	[branch]
	if (waveID == 0) //Only true for the first wave in the group
	{
		[unroll]
		for (a = 1; a < THREAD_COUNT / LANE_COUNT; a <<= 1)
		{
			iTmp = 0;
			[flatten]
			if (groupIndex >= a && groupIndex < THREAD_COUNT / LANE_COUNT)
			{
				iTmp += WaveSize[groupIndex - a];
			}
			WaveSize[groupIndex] += iTmp;
		}
	}
#endif
	GroupMemoryBarrierWithGroupSync();

	uint numIndexTotal = WaveSize[(THREAD_COUNT / LANE_COUNT) + rof - 1];

	[branch]
	if (numIndexTotal > 0)
	{
		uint off = 0;
		if (waveID > 0) { off = WaveSize[waveID + rof - 1]; }

		//Load first index:
		if (groupIndex == 0)
		{
			uint start = 0;
			IndexBuffer.InterlockedAdd(0, numIndexTotal, start);
			StartIndex = start;
		}
		GroupMemoryBarrierWithGroupSync();

		//Write cluster index:
		uint wIndex = IndexCount[groupIndex] + off + StartIndex;
		if (r > 0) { ClusterIDs[wIndex] = groupID.x; }
	}
}