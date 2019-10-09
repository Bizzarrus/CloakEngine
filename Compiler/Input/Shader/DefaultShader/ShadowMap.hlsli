//This file implements Non-Linear Quantisized Moment Shadow Mapping by Christoph Peters
//	See http://cg.cs.uni-bonn.de/aigaion2root/attachments/NonLinearMSM.pdf


#define SHADOW_MAP_RES float4
#define SHADOW_MIN_MOMENT_OFFSET 1.2e-7f
#define SHADOW_MAX_MOMENT_OFFSET 0.25f
#define SHADOW_MOMENT_OFFSET_SINGULARITY 1.0e-7f

#if (defined(SHADOW_DECODE) && !defined(SHADOW_BLUR))

#define SHADOW_MAP_TYPE Texture2D<SHADOW_MAP_RES>
#define SHADOW_CLAMP_OFFSET 1.0e-6f

float Shadow(float3 WorldPos, matrix WorldToLight, SHADOW_MAP_TYPE shadowMap, float2 noise)
{
	float4 cs = mul(float4(WorldPos, 1), WorldToLight);
	cs /= cs.w;
	cs.z = 1 - cs.z;

	uint2 size;
	shadowMap.GetDimensions(size.x, size.y);
	uint2 texPos = uint2(floor((float2(size.xy) * cs.xy) + clamp(noise, 0, 1)));
	float4 moments = shadowMap.Load(uint3(clamp(texPos, uint2(0, 0), size - 1), 0));
	//Unpack moment offset
	const float warpSummand = log(log(SHADOW_MIN_MOMENT_OFFSET / SHADOW_MOMENT_OFFSET_SINGULARITY));
	const float warpFactor = log(log(SHADOW_MAX_MOMENT_OFFSET / SHADOW_MOMENT_OFFSET_SINGULARITY)) - warpSummand;
	moments.w = exp(exp(mad(moments.w, warpFactor, warpSummand))) * SHADOW_MOMENT_OFFSET_SINGULARITY;
	//calculate shadow factor
	moments.z = clamp(moments.z, SHADOW_CLAMP_OFFSET, 1.0f - SHADOW_CLAMP_OFFSET);
	moments.y = max(moments.x + SHADOW_CLAMP_OFFSET, moments.y);
	[branch]
	if (cs.z <= moments.x) { return 1.0f; }
	float scaling = rcp(moments.y - moments.x);
	float nie = (cs.z - moments.x) * scaling;
	float ssq = scaling * scaling;
	moments.w *= ssq * ssq;
	float iw0 = rcp(1.0f - moments.z);
	float iw1 = rcp(moments.z);
	float imo = rcp(moments.w);
	
	float root = nie;
	float fr = 1.0f - root;
	float2 ot = float2(-1.0f, 1.0f);
	[branch]
	if (nie < 1.0f)
	{
		float q = -moments.w * iw0 / nie;
		float ph = mad(-0.5f * q, mad(nie, -iw1, 1.0f) / fr, -0.5f);
		root = -ph - sqrt(mad(ph, ph, -q));
		fr = 1.0f - root;
		ot = float2(1.0f, 0.0f);
	}
	float rs = root * root;
	float frs = fr * fr;
	float rw = rcp(dot(float3(iw0, iw1, imo), float3(frs, rs, rs * frs)));
	float sf = 1.0f - mad(rw, ot.x, ot.y);
	return clamp(sf, 0.0f, 1.0f);
}

#elif (!defined(SHADOW_DECODE) && defined(SHADOW_BLUR))

#ifdef SHADER_MSAA
#define SHADOW_MAP_TYPE Texture2DMS<float, SHADER_MSAA>
#else
#define SHADOW_MAP_TYPE Texture2D<float>
#endif
#define SHADOW_MAP_OUTPUT RWTexture2D<unorm SHADOW_MAP_RES>

#define THREAD_COUNT_X 32
#define THREAD_COUNT_Y 8
#define THREAD_COUNT_Z 1
#define THREAD_COUNT (THREAD_COUNT_X * THREAD_COUNT_Y * THREAD_COUNT_Z)

#define SHADOW_OUTPUT_BLOCK_WIDTH 32
#define SHADOW_OUTPUT_BLOCK_HEIGHT 24
#define SHADOW_GUARD_BAND 8
#define SHADOW_INPUT_BLOCK_WIDTH (SHADOW_OUTPUT_BLOCK_WIDTH + SHADOW_GUARD_BAND)
#define SHADOW_INPUT_BLOCK_HEIGHT (SHADOW_OUTPUT_BLOCK_HEIGHT + SHADOW_GUARD_BAND)
#define SHADOW_SPACER_STRIDE 4
#define SHADOW_HALF_INPUT_BLOCK_WIDTH (SHADOW_INPUT_BLOCK_WIDTH / 2)
#define SHADOW_MOMENT 4
#define SHADOW_THREAD_COUNT_HALF (THREAD_COUNT / 2)
#define SHADOW_INPUT_PER_THREAD ((SHADOW_INPUT_BLOCK_WIDTH * SHADOW_INPUT_BLOCK_HEIGHT) / THREAD_COUNT)
#define SHADOW_OUTPUT_PER_THREAD ((SHADOW_OUTPUT_BLOCK_WIDTH * SHADOW_OUTPUT_BLOCK_HEIGHT) / THREAD_COUNT)
#define SHADOW_MOMENT_PITCH ((SHADOW_INPUT_BLOCK_WIDTH * SHADOW_INPUT_BLOCK_HEIGHT) + (SHADOW_INPUT_BLOCK_HEIGHT / SHADOW_SPACER_STRIDE) - 1)

//9-tap gaussian blur (centered at index 4) with standard deviation 2.4
#define SHADOW_GAUSSIAN_0 0.0440444341987f
#define SHADOW_GAUSSIAN_1 0.0808695919535f
#define SHADOW_GAUSSIAN_2 0.124819121228f
#define SHADOW_GAUSSIAN_3 0.161949138463f
#define SHADOW_GAUSSIAN_4 0.176635428315f
#define SHADOW_GAUSSIAN_5 0.161949138463f
#define SHADOW_GAUSSIAN_6 0.124819121228f
#define SHADOW_GAUSSIAN_7 0.0808695919535f
#define SHADOW_GAUSSIAN_8 0.0440444341987f
#define SHADOW_GAUSSIAN(X) SHADOW_GAUSSIAN_##X

groupshared float ShadowMoment[SHADOW_MOMENT_PITCH * SHADOW_MOMENT];

void ShadowBlur(const uint3 groupThreadID, const bool rightHalf)
{
	uint threadMoment = groupThreadID.y % SHADOW_MOMENT;
	uint momentOffset = threadMoment * SHADOW_MOMENT_PITCH;

	//Load moments for horizontal blur:
	float momentRow[(SHADOW_OUTPUT_BLOCK_WIDTH / 2) + SHADOW_GUARD_BAND];
	uint row = groupThreadID.x;
	uint rowOffset = (row * SHADOW_INPUT_BLOCK_WIDTH) + (row / SHADOW_SPACER_STRIDE) + momentOffset;
	uint memPos = rightHalf ? (SHADOW_OUTPUT_BLOCK_WIDTH + rowOffset) : rowOffset;
	[unroll]
	for (uint a = 0; a < SHADOW_GUARD_BAND / 2; a++) { momentRow[a] = ShadowMoment[memPos + (2 * a)]; }
	memPos = rightHalf ? (memPos + 1 - (2 * (SHADOW_OUTPUT_BLOCK_WIDTH / 2))) : (memPos + (2 * (SHADOW_GUARD_BAND / 2)));
	[unroll]
	for (uint a = SHADOW_GUARD_BAND / 2; a < SHADOW_INPUT_BLOCK_WIDTH / 2; a++) { momentRow[a] = ShadowMoment[memPos + (2 * (a - (SHADOW_GUARD_BAND / 2)))]; }
	memPos = rightHalf ? (memPos + (2 * (SHADOW_HALF_INPUT_BLOCK_WIDTH - (SHADOW_GUARD_BAND / 2)))) : (memPos + 1 - SHADOW_GUARD_BAND);
	[unroll]
	for (uint a = SHADOW_HALF_INPUT_BLOCK_WIDTH; a < (SHADOW_OUTPUT_BLOCK_WIDTH / 2) + SHADOW_GUARD_BAND; a++) { momentRow[a] = ShadowMoment[memPos + (2 * (a - SHADOW_HALF_INPUT_BLOCK_WIDTH))]; }
	//Apply horizontal blur:
	float blurredRow[SHADOW_OUTPUT_BLOCK_WIDTH / 2];
	[unroll]
	for (uint a = 0; a < SHADOW_OUTPUT_BLOCK_WIDTH / 2; a++)
	{
		blurredRow[a] = 0;
		blurredRow[a] += SHADOW_GAUSSIAN(0) * momentRow[a + 0];
		blurredRow[a] += SHADOW_GAUSSIAN(1) * momentRow[a + 1];
		blurredRow[a] += SHADOW_GAUSSIAN(2) * momentRow[a + 2];
		blurredRow[a] += SHADOW_GAUSSIAN(3) * momentRow[a + 3];
		blurredRow[a] += SHADOW_GAUSSIAN(4) * momentRow[a + 4];
		blurredRow[a] += SHADOW_GAUSSIAN(5) * momentRow[a + 5];
		blurredRow[a] += SHADOW_GAUSSIAN(6) * momentRow[a + 6];
		blurredRow[a] += SHADOW_GAUSSIAN(7) * momentRow[a + 7];
		blurredRow[a] += SHADOW_GAUSSIAN(8) * momentRow[a + 8];
	}
	//Write blurred values back:
	memPos = rightHalf ? (SHADOW_GUARD_BAND + 1 + rowOffset) : rowOffset;
	for (uint a = 0; a < SHADOW_OUTPUT_BLOCK_WIDTH / 2; a++) { ShadowMoment[memPos + (2 * a)] = blurredRow[a]; }
#if LANE_COUNT < 32
	GroupMemoryBarrierWithGroupSync();
#endif

	//Load moments for vertical blur:
	float momentColumn[(SHADOW_OUTPUT_BLOCK_HEIGHT / 2) + SHADOW_GUARD_BAND];
	uint column = groupThreadID.x % (SHADOW_OUTPUT_BLOCK_HEIGHT / 2);
	column = rightHalf ? (column + (SHADOW_OUTPUT_BLOCK_WIDTH / 2) + SHADOW_GUARD_BAND) : column;
	bool lowerHalf = (groupThreadID.x >= SHADOW_OUTPUT_BLOCK_WIDTH / 2);
	uint firstRow = lowerHalf ? (SHADOW_OUTPUT_BLOCK_HEIGHT / 2) : 0;
	float memPosFirstRow = (column * 2) + (firstRow * SHADOW_INPUT_BLOCK_WIDTH) + (firstRow / SHADOW_SPACER_STRIDE) + momentOffset;
	memPosFirstRow = rightHalf ? (memPosFirstRow + 1 - (SHADOW_HALF_INPUT_BLOCK_WIDTH * 2)) : memPosFirstRow;
	[unroll]
	for (uint a = 0; a < (SHADOW_OUTPUT_BLOCK_HEIGHT / 2) + SHADOW_GUARD_BAND; a++)
	{
		uint memOffset = (a * SHADOW_INPUT_BLOCK_WIDTH) + (a / SHADOW_SPACER_STRIDE);
		momentColumn[a] = ShadowMoment[memPosFirstRow + memOffset];
	}
	//Apply vertical blur:
	float blurredColumn[SHADOW_OUTPUT_BLOCK_HEIGHT / 2];
	[unroll]
	for (uint a = 0; a < SHADOW_OUTPUT_BLOCK_HEIGHT / 2; a++)
	{
		blurredColumn[a] = 0;
		blurredColumn[a] += SHADOW_GAUSSIAN(0) * momentColumn[a + 0];
		blurredColumn[a] += SHADOW_GAUSSIAN(1) * momentColumn[a + 1];
		blurredColumn[a] += SHADOW_GAUSSIAN(2) * momentColumn[a + 2];
		blurredColumn[a] += SHADOW_GAUSSIAN(3) * momentColumn[a + 3];
		blurredColumn[a] += SHADOW_GAUSSIAN(4) * momentColumn[a + 4];
		blurredColumn[a] += SHADOW_GAUSSIAN(5) * momentColumn[a + 5];
		blurredColumn[a] += SHADOW_GAUSSIAN(6) * momentColumn[a + 6];
		blurredColumn[a] += SHADOW_GAUSSIAN(7) * momentColumn[a + 7];
		blurredColumn[a] += SHADOW_GAUSSIAN(8) * momentColumn[a + 8];
	}
#if LANE_COUNT < 32
	GroupMemoryBarrierWithGroupSync();
#endif
	//Write blurred values back:
	[unroll]
	for (uint a = 0; a < SHADOW_OUTPUT_BLOCK_HEIGHT / 2; a++)
	{
		uint memOffset = (a * SHADOW_INPUT_BLOCK_WIDTH) + (a / SHADOW_SPACER_STRIDE);
		ShadowMoment[memPosFirstRow + memOffset] = blurredColumn[a];
	}
}

SHADOW_MAP_RES PackMoments(const float4 moments)
{
	float4 b = lerp(moments, float4(0.0f, 0.375f, 0.0f, 0.375f), 3.0e-7f);
	//Apply Vandermonde decomposition:
	float3 polynom;
	float variance = mad(-b.x, b.x, b.y);
	polynom.z = variance;
	polynom.y = mad(b.x, b.y, -b.z);
	polynom.x = dot(-b.xy, polynom.yz);
	float iv = rcp(polynom.z);
	float p = polynom.y * iv;
	float q = polynom.x * iv;
	float D = (p * p * 0.25f) - q;
	float r = sqrt(D);
	float2 depth = float2((-0.5f * p) - r, (-0.5f * p) + r);
	float weight = (b.x - depth.x) / (depth.y - depth.x);
	//Compute small part of Choleky factorization for moment offset:
	float L21D11 = mad(-b.x, b.y, b.z);
	float sdv = mad(-b.y, b.y, b.w);
	float momentOffset = mad(-L21D11, L21D11 * iv, sdv);
	//Warp moment offset to create uniform distribution in 0..1 range:
	const float warpFactor = rcp(log(log(SHADOW_MAX_MOMENT_OFFSET / SHADOW_MOMENT_OFFSET_SINGULARITY)) - log(log(SHADOW_MIN_MOMENT_OFFSET / SHADOW_MOMENT_OFFSET_SINGULARITY)));
	const float warpSummand = -log(log(SHADOW_MIN_MOMENT_OFFSET / SHADOW_MOMENT_OFFSET_SINGULARITY)) * warpFactor;
	return float4(mad(depth, 0.5f, 0.5f), weight, saturate(mad(log(log(max(SHADOW_MIN_MOMENT_OFFSET, momentOffset) / SHADOW_MOMENT_OFFSET_SINGULARITY)), warpFactor, warpSummand)));
}

void Shadow(inout SHADOW_MAP_OUTPUT outMap, SHADOW_MAP_TYPE inMap, const uint3 groupID, const uint3 groupThreadID)
{
	uint2 outSize;
	outMap.GetDimensions(outSize.x, outSize.y);
	uint2 maxPos = outSize - 1;

	bool rightHalf = (groupThreadID.y >= SHADOW_THREAD_COUNT_HALF / SHADOW_INPUT_BLOCK_HEIGHT);
	uint2 inputPos = groupThreadID.yx;
	inputPos.x = rightHalf ? (inputPos.x + SHADOW_HALF_INPUT_BLOCK_WIDTH - (SHADOW_THREAD_COUNT_HALF / SHADOW_INPUT_BLOCK_HEIGHT)) : inputPos.x;
	uint2 srcTex = uint2((groupID.x * SHADOW_OUTPUT_BLOCK_WIDTH) + inputPos.x - (SHADOW_GUARD_BAND / 2), clamp((groupID.y * SHADOW_OUTPUT_BLOCK_HEIGHT) + inputPos.y -(SHADOW_GUARD_BAND / 2), 0, maxPos.y));
	uint memPos = (inputPos.x * 2) + (inputPos.y * SHADOW_INPUT_BLOCK_WIDTH) + (inputPos.y / SHADOW_SPACER_STRIDE);
	memPos = rightHalf ? (memPos + 1 - (2 * SHADOW_HALF_INPUT_BLOCK_WIDTH)) : memPos;

	//Load depths and calculate moment values
	[unroll]
	for (uint a = 0; a < SHADOW_INPUT_PER_THREAD; a++, memPos += 2 * (SHADOW_THREAD_COUNT_HALF / SHADOW_INPUT_BLOCK_HEIGHT), srcTex.x += SHADOW_THREAD_COUNT_HALF / SHADOW_INPUT_BLOCK_HEIGHT)
	{
		float4 moments = float4(0, 0, 0, 0);
		uint clX = clamp(srcTex.x, 0, maxPos.x);
#ifdef SHADER_MSAA
		[unroll]
		for (uint b = 0; b < SHADER_MSAA; b++)
		{
			float depth = inMap.Load(uint2(clX, srcTex.y), b);
#else
		{
			float depth = inMap.Load(uint3(clX, srcTex.y, 0));
#endif
			depth = mad(1 - depth, 2.0f, -1.0f);
			float sd = depth * depth;
			moments += float4(depth, sd, depth * sd, sd * sd);
		}
#ifdef SHADER_MSAA
		moments /= SHADER_MSAA;
#endif
		ShadowMoment[(0 * SHADOW_MOMENT_PITCH) + memPos] = moments.x;
		ShadowMoment[(1 * SHADOW_MOMENT_PITCH) + memPos] = moments.y;
		ShadowMoment[(2 * SHADOW_MOMENT_PITCH) + memPos] = moments.z;
		ShadowMoment[(3 * SHADOW_MOMENT_PITCH) + memPos] = moments.w;
	}

	//Perform gaussian blur in groupshared memory
	GroupMemoryBarrierWithGroupSync();
	ShadowBlur(groupThreadID, rightHalf);
	GroupMemoryBarrierWithGroupSync();

	//Each thread packs three moment vectors:
	uint2 outPos = groupThreadID.xy;
	uint2 outTex = uint2((groupID.x * SHADOW_OUTPUT_BLOCK_WIDTH) + outPos.x, (groupID.y * SHADOW_OUTPUT_BLOCK_HEIGHT) + outPos.y);
	uint2 srcPos = outPos;
	rightHalf = (srcPos.x >= SHADOW_OUTPUT_BLOCK_WIDTH / 2);
	srcPos.x = rightHalf ? (srcPos.x + SHADOW_GUARD_BAND) : srcPos.x;
	memPos = (srcPos.x * 2) + (srcPos.y * SHADOW_INPUT_BLOCK_WIDTH) + (srcPos.y / SHADOW_SPACER_STRIDE);
	memPos = rightHalf ? (memPos + 1 - (2 * SHADOW_HALF_INPUT_BLOCK_WIDTH)) : memPos;
	const uint memOffset = ((THREAD_COUNT / SHADOW_OUTPUT_BLOCK_WIDTH) * SHADOW_INPUT_BLOCK_WIDTH) + (THREAD_COUNT / (SHADOW_OUTPUT_BLOCK_WIDTH * SHADOW_SPACER_STRIDE));
	float4 momentRes[SHADOW_OUTPUT_PER_THREAD];
	[unroll]
	for (uint a = 0; a < SHADOW_OUTPUT_PER_THREAD; a++, memPos += memOffset)
	{
		momentRes[a].x = ShadowMoment[(0 * SHADOW_MOMENT_PITCH) + memPos];
		momentRes[a].y = ShadowMoment[(1 * SHADOW_MOMENT_PITCH) + memPos];
		momentRes[a].z = ShadowMoment[(2 * SHADOW_MOMENT_PITCH) + memPos];
		momentRes[a].w = ShadowMoment[(3 * SHADOW_MOMENT_PITCH) + memPos];
	}
	SHADOW_MAP_RES packedRes[SHADOW_OUTPUT_PER_THREAD];
	for (uint a = 0; a < SHADOW_OUTPUT_PER_THREAD; a++) { packedRes[a] = PackMoments(momentRes[a]); }
	//Output packed moments:
	for (uint a = 0; a < SHADOW_OUTPUT_PER_THREAD; a++, outTex.y += THREAD_COUNT / SHADOW_OUTPUT_BLOCK_WIDTH)
	{
		if (all(outTex <= maxPos)) { outMap[outTex] = packedRes[a]; }
	}
}

#elif (defined(SHADOW_BLUR) && defined(SHADOW_DECODE))
#error SHADOW_BLUR and SHADOW_DECODE are defined, which is not allowed!
#else
#error Either SHADOW_DECODE or SHADOW_BLUR must be defined to use Shadow Maps!
#endif