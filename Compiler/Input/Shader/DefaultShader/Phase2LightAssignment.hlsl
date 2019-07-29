#define THREAD_COUNT 128

#include "Clustering.hlsli"

RWByteAddressBuffer IndexBuffer : register(u0); //Current index to write to in index buffer
RWByteAddressBuffer MaxIndexBuffer : register(u1); //Highest number of used indices (used to resize index buffer if requiered)

Texture1D<uint> ClusterIDs : register(t0);						//List of cluster ids
ByteAddressBuffer ClusterIDCount : register(t1);				//Number of cluster ids
RWTexture1D<uint> LightIndex : register(t2);					//Final Index buffer
Texture3D<uint> Clusters : register(t3);						//Cluster Buffer

groupshared uint IndexCount[THREAD_COUNT];						//Number of indices each thread wants to register
groupshared uint StartIndex;									//Already registered indices (by other groups)
#if ((THREAD_COUNT / LANE_COUNT) > LANE_COUNT)
groupshared uint WaveSize[(THREAD_COUNT / LANE_COUNT) * 2];		//Number of indices each wave wants to register
#else
groupshared uint WaveSize[THREAD_COUNT / LANE_COUNT];
#endif

uint CompressLightIndex(uint start, uint pointLight, uint spotLight)
{
	return (start << 19) | (pointLight << 9) | spotLight;
}

[numthreads(THREAD_COUNT, 1, 1)]
void mainCS(uint3 groupID : SV_DispatchThreadID, uint groupIndex : SV_GroupIndex)
{
	//Find all lights within cluster:
	uint numSpotLights = 0;		//Number of spot lights intersecting with current cluster, up to 512
	uint numPointLights = 0;	//Number of point lights intersecting with current cluster, up to 1024
	uint lightIds[1024];
	uint activeClusterCount = ClusterIDCount.Load(0);
	uint3 keys = uint3(CLUSTER_COUNT_X, CLUSTER_COUNT_Y, CLUSTER_COUNT_Z);

#if ((TARGET & TARGET_TYPE_DX) != 0 && TARGET >= TARGET_DX_6_0)
	[branch]
	if (WaveAnyTrue(groupID.x < activeClusterCount))
	{
#endif
		[flatten]
		if (groupID.x < activeClusterCount)
		{
			keys = ClusterKeys(ClusterIDs[groupID.x]);
			[flatten]
			if (keys.z < CLUSTER_COUNT_Z)
			{
				//TODO: iterate over all lights, cull lights and fill local lightIds list
				//TODO: May be extended to include multiple light types and/or IBL sources
			}
		}
#if ((TARGET & TARGET_TYPE_DX) != 0 && TARGET >= TARGET_DX_6_0)
	}
#endif
	
	uint numLightIds = max(numSpotLights, numPointLights); //Number of used light indices

	//Integrate index counts in parallel:
	IndexCount[groupIndex] = numLightIds;
	uint waveID = groupIndex / LANE_COUNT; //ID of wave this thread runs on
	uint waveOffset = waveID * LANE_COUNT; //Number of threads before this wave
	uint waveIndex = groupIndex - waveOffset; //ID of thread within the current wave

#if ((TARGET & TARGET_TYPE_DX) != 0 && TARGET >= TARGET_DX_6_0)
	[branch]
	if (WaveAnyTrue(numLightIds > 0))
	{
#endif
		[unroll]
		for (uint a = 1; a < LANE_COUNT; a <<= 1)
		{
			[flatten]
			if (waveIndex >= a)
			{
				IndexCount[groupIndex] += IndexCount[groupIndex - a];
			}
		}
#if ((TARGET & TARGET_TYPE_DX) != 0 && TARGET >= TARGET_DX_6_0)
	}
#endif
	[flatten]
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
		[flatten]
		if (groupIndex >= 1)
		{
			WaveSize[groupIndex + wof] = WaveSize[groupIndex] + WaveSize[groupIndex - 1];
		}
		else
		{
			WaveSize[groupIndex + wof] = WaveSize[groupIndex];
		}
	}
	tof = rof;
	rof = wof;
	wof = tof;

	uint nix;
	[unroll]
	for (uint a = 2; a < THREAD_COUNT / LANE_COUNT; a <<= 1)
	{
		GroupMemoryBarrierWithGroupSync();
		nix = groupIndex + rof;
		if (groupIndex < THREAD_COUNT / LANE_COUNT)
		{
			[flatten]
			if (groupIndex >= a)
			{
				WaveSize[groupIndex + wof] = WaveSize[nix] + WaveSize[nix - a];
			}
			else
			{
				WaveSize[groupIndex + wof] = WaveSize[nix];
			}
		}
		tof = rof;
		rof = wof;
		wof = tof;
	}
#elif (THREAD_COUNT / LANE_COUNT > 1)
	[branch]
	if (waveID == 0) //Only true for the first wave in the group
	{
#if ((TARGET & TARGET_TYPE_DX) != 0 && TARGET >= TARGET_DX_6_0)
		[branch]
		if (WaveAnyTrue(groupIndex < THREAD_COUNT / LANE_COUNT && WaveSize[groupIndex] > 0))
		{
#endif
			[unroll]
			for (uint a = 1; a < THREAD_COUNT / LANE_COUNT; a <<= 1)
			{
				[flatten]
				if (groupIndex >= a && groupIndex < THREAD_COUNT / LANE_COUNT)
				{
					WaveSize[groupIndex] += WaveSize[groupIndex - a];
				}
			}
#if ((TARGET & TARGET_TYPE_DX) != 0 && TARGET >= TARGET_DX_6_0)
		}
#endif
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
			IndexBuffer.InterlockedAdd(0, numIndexTotal, out start);
			MaxIndexBuffer.InterlockedMax(0, start + numIndexTotal);
			StartIndex = start;
		}
		GroupMemoryBarrierWithGroupSync();

		//Write indices into buffer:
		uint startIndex = IndexCount[groupIndex] + off + StartIndex;
		[flatten]
		if (keys.z < CLUSTER_COUNT_Z)
		{
			Clusters[keys] = CompressLightIndex(startIndex, numPointLights, numSpotLights);
			[unroll(1024)]
			for (uint a = 0; a < numLightIds; a++)
			{
				LightIndex[startIndex + a] = lightIds[a];
			}
		}
	}
}