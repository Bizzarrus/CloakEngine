#define CLUSTER_COUNT_X 16
#define CLUSTER_COUNT_Y 8
#define CLUSTER_COUNT_Z 128

//Careful: requires view space depth, not screen space depth!
uint3 ClusterKeys(float2 uv, float viewSpaceDepth, float nearPlane, float farPlane)
{
	float2 d = float2(viewSpaceDepth, farPlane);
	//Manually tweaked slice function. Original function proposted by "Clustered Deferred and Forward Shading" from Ola Olsson, Markus Billeter, and Ulf Assarsson would be:
	// dk = log(float2(viewSpaceDepth / nearPlane, 1 + ((2 * tan(fov)) / CLUSTER_COUNT_Y))); k.z = dk.x/dk.y;
	float2 dk = log((4 * (d / nearPlane)) + 0.75f);
	float3 k = float3(uv*float2(CLUSTER_COUNT_X, CLUSTER_COUNT_Y), (dk.x / dk.y)*CLUSTER_COUNT_Z);
	k = clamp(k, float3(0, 0, 0), float3(CLUSTER_COUNT_X - 1, CLUSTER_COUNT_Y - 1, CLUSTER_COUNT_Z - 1));
	return (uint3)floor(k);
}

uint3 ClusterKeys(uint index)
{
	uint3 r = 0;
	r.x = index % CLUSTER_COUNT_X;
	index /= CLUSTER_COUNT_X;
	r.y = index % CLUSTER_COUNT_Y;
	index /= CLUSTER_COUNT_Y;
	r.z = index;
	return r;
}

uint ClusterIndex(uint3 key)
{
	key = clamp(key, uint3(0, 0, 0), uint3(CLUSTER_COUNT_X - 1, CLUSTER_COUNT_Y - 1, CLUSTER_COUNT_Z - 1));
	return (((key.z*CLUSTER_COUNT_Y) + key.y)*CLUSTER_COUNT_X) + key.x;
}