namespace GammaCorrection {
	float3 ApplyGamma(float3 linearColor, float gamma) { return pow(max(0, linearColor.rgb), gamma); }
	float4 ApplyGamma(float4 linearColor, float gamma) { return float4(pow(max(0,linearColor.rgb), gamma), linearColor.a); }
	float4 ToLinearColor(float4 color) { return ApplyGamma(color, 2.2f); }
	float4 InlineGamma(float4 color, float gamma) { return float4(pow(max(0, color.rgb), 2.2f * gamma), color.a); }
}
namespace NormalEncoding {
#ifdef ENCODE_NORMAL_COMPRESSED
	float2 Encode(float3 normal) 
	{
		normal = normalize(normal);
		float dn = rcp(1 - normal.Z);
		return ((normal.xy * dn) + 1)*0.5f;
	}
	float3 Decode(float2 input)
	{
		float2 i = (input.xy * 2) - 1;
		float dn = 2 / (1 + (i.x*i.x) + (i.y * i.y));
		return float3(input.xy * dn, 1 - dn);
	}
#else
	float3 Encode(float3 normal) { return (normalize(normal) + 1)*0.5f; }
	float3 Decode(float3 input) { return (input * 2) - 1; }
#endif
}
namespace PBREncoding {
	float Roughness(float r) { return r*r; }
}