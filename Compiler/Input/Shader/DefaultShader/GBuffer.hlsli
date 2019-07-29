#include "GBufferEncoding.hlsli"

#ifndef GBUFFER_OFFSET
#define GBUFFER_OFFSET 0
#endif
#ifndef GBUFFER_SIZE
#define GBUFFER_SIZE 4
#endif

#ifndef NO_GBUFFER
	SamplerState Sampler : register(s0);
	#ifdef SHADER_MSAA
		Texture2DMS<float4, SHADER_MSAA> GBuffer[GBUFFER_SIZE - GBUFFER_OFFSET] : register(t0, space0);
		#define LoadTexSimple(buffer, texCoord, texPos) GBuffer[buffer - GBUFFER_OFFSET].Load(texPos.xy, 0)
		#define LoadTexComplex(buffer, texCoord, texPos, layer) GBuffer[buffer - GBUFFER_OFFSET].Load(texPos.xy, layer.x)
	#else
		Texture2D<float4> GBuffer[GBUFFER_SIZE - GBUFFER_OFFSET] : register(t0, space0);
		#define LoadTexSimple(buffer, texCoord, texPos) GBuffer[buffer - GBUFFER_OFFSET].Sample(Sampler, texCoord.xy)
		#define LoadTexComplex(buffer, texCoord, texPos, layer) LoadTexSimple(buffer, texCoord, texPos)
	#endif
#endif

#ifdef OPTION_COMPLEX
	#define LoadTex(buffer, texCoord, texPos, layer) LoadTexComplex(buffer, texCoord, texPos, layer)
	#ifdef SHADER_MSAA
		#define MSAAIteration(layerName) [unroll] for(uint layerName = 0; layerName < SHADER_MSAA; layerName++)
	#else
		#define MSAAIteration(layerName) uint layerName = 0;
	#endif
#else
	#define LoadTex(buffer, texCoord, texPos, layer) LoadTexSimple(buffer, texCoord, texPos)
	#define MSAAIteration(layerName) uint layerName = 0;
#endif