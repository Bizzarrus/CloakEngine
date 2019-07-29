#define PI 3.14159265359f

float2 Hammersley(uint index, uint numSamples)
{
	return float2(float(index) / numSamples, rcp(4294967296.0f) * reversebits(index));
}

//Reference: http://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf
float3 ImportanceSampleGGX(uint index, uint numSamples, float roughness, float3 direction)
{
	float2 U = Hammersley(index, numSamples);
	float a = roughness * roughness;
	float p = 2 * PI * U.x;
	float c = saturate(sqrt(max(0, (1 - U.y) / (1 + (U.y*((a*a) - 1))))));
	float s = sqrt(max(0, 1 - (c*c)));
	float3 H = float3(s*cos(p), s*sin(p), c);
	float3 up = abs(direction.z) < 0.9f ? float3(0, 0, 1) : float3(1, 0, 0);
	float3 tx = normalize(cross(up, direction));
	float3 ty = cross(direction, tx);
	return normalize((tx*H.x) + (ty*H.y) + (direction*H.z));
}