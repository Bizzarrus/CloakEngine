#include "Blur.hlsli"

[numthreads(THREAD_COUNT, 1, 1)]
void mainCS(uint3 groupID : SV_GroupId, uint groupIndex : SV_GroupIndex)
{
	//Chunks need to overlap, since a chunk can't read the shared data of another chunk. 
	const int2 pos = int2(groupID.x, (groupID.y*CHUNK_SIZE) + groupIndex);
	//Moving chunks by KERNEL_HALF to the left and clamping it to the texture size ensures to blur the edges correctly
	int2 tex = clamp(pos - int2(0, KERNEL_HALF), 0, Info.TextureSize - 1);

	SharedBuffer[groupIndex] = Source.Load(int3(tex + Info.OffsetSrc, 0));

	GroupMemoryBarrierWithGroupSync();

	if (groupIndex < CHUNK_SIZE && pos.y < Info.TextureSize.y)
	{
		float4 res = 0;
		res += SharedBuffer[groupIndex +  0] * KERNEL(0);
		res += SharedBuffer[groupIndex +  1] * KERNEL(1);
		res += SharedBuffer[groupIndex +  2] * KERNEL(2);
		res += SharedBuffer[groupIndex +  3] * KERNEL(3);
		res += SharedBuffer[groupIndex +  4] * KERNEL(4);
		res += SharedBuffer[groupIndex +  5] * KERNEL(5);
		res += SharedBuffer[groupIndex +  6] * KERNEL(6);
		res += SharedBuffer[groupIndex +  7] * KERNEL(7);
		res += SharedBuffer[groupIndex +  8] * KERNEL(6);
		res += SharedBuffer[groupIndex +  9] * KERNEL(5);
		res += SharedBuffer[groupIndex + 10] * KERNEL(4);
		res += SharedBuffer[groupIndex + 11] * KERNEL(3);
		res += SharedBuffer[groupIndex + 12] * KERNEL(2);
		res += SharedBuffer[groupIndex + 13] * KERNEL(1);
		res += SharedBuffer[groupIndex + 14] * KERNEL(0);
		Destination[pos + Info.OffsetDst] = res;
	}
}