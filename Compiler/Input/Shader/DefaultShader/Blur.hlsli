#define THREAD_COUNT 128

#define KERNEL_SIZE 15
#define KERNEL_HALF (KERNEL_SIZE >> 1)
#define CHUNK_SIZE (THREAD_COUNT - (KERNEL_HALF << 1))

#define KERNEL_0 0.000010f
#define KERNEL_1 0.000152f
#define KERNEL_2 0.001473f
#define KERNEL_3 0.009446f
#define KERNEL_4 0.040050f
#define KERNEL_5 0.112315f
#define KERNEL_6 0.208464f
#define KERNEL_7 0.256179f

#define KERNEL(x) KERNEL_##x

struct BlurInfo {
	int2 TextureSize;
	uint2 OffsetSrc;
	uint2 OffsetDst;	
};

Texture2D<float4> Source : register(t0);
RWTexture2D<float4> Destination : register(u0);
ConstantBuffer<BlurInfo> Info : register(b0);

groupshared float4 SharedBuffer[THREAD_COUNT];