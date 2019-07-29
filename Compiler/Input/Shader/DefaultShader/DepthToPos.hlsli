#include "GBuffer.hlsli"
#ifndef NO_DEPTHBUFFER
	#ifdef NO_GBUFFER
		SamplerState Sampler : register(s0);
	#endif
	#ifdef SHADER_MSAA
		Texture2DMS<float, SHADER_MSAA> Depth : register(t0, space1);
		#define LoadDepthSimple(texPos) Depth.Load(texPos,0)
		#define LoadDepthComplex(texPos,layer) Depth.Load(texPos,layer)
	#else
		Texture2D<float> Depth : register(t0, space1);
		#define LoadDepthSimple(texPos) Depth.Sample(Sampler,texPos)
		#define LoadDepthComplex(texPos,layer) LoadDepthSimple(texPos)
	#endif
#endif

#ifdef OPTION_COMPLEX
#define LoadDepth(texPos, layer) LoadDepthComplex(texPos, layer)
#else
#define LoadDepth(texPos, layer) LoadDepthSimple(texPos)
#endif

float4 DepthToPos(matrix ProjectionToWorld, float2 texPos, float depth)
{
	float x = (texPos.x * 2) - 1;
	float y = ((1 - texPos.y) * 2) - 1;
	float4 r = mul(float4(x, y, depth, 1), ProjectionToWorld);
	return r / r.w;
}

float4 GetWorldPosition(matrix ProjectionToWorld, float2 texPos)
{
	float d = LoadDepthSimple(texPos);
	return DepthToPos(ProjectionToWorld, texPos, d);
}
float4 GetWorldPosition(matrix ProjectionToWorld, float2 texPos, uint layer)
{
	float d = LoadDepthComplex(texPos, layer);
	return DepthToPos(ProjectionToWorld, texPos, d);
}
float4 GetWorldPositionAndDepth(matrix ProjectionToWorld, float2 texPos, out float depth)
{
	float d = LoadDepthSimple(texPos);
	depth = d;
	return DepthToPos(ProjectionToWorld, texPos, d);
}
float4 GetWorldPositionAndDepth(matrix ProjectionToWorld, float2 texPos, out float depth, uint layer)
{
	float d = LoadDepthComplex(texPos, layer);
	depth = d;
	return DepthToPos(ProjectionToWorld, texPos, d);
}