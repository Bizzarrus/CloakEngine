namespace Quaternion {
	typedef float4 Quaternion;
	struct DualQuaternion {
		Quaternion Rotation;
		Quaternion Translation;
	};

	float4 Rotation(in Quaternion quaternion, in float4 possition)
	{
		float3 a = cross(quaternion.xyz, possition.xyz) + (quaternion.w*possition.xyz);
		float3 v = (cross(a, -quaternion.xyz) + (dot(quaternion.xyz, possition.xyz)*quaternion.xyz) + (quaternion.w*a));
		return float4(v, possition.w);
	}
	float4 Transform(in DualQuaternion q, in float4 pos)
	{
		return float4(Rotation(q.Rotation, pos).xyz + (2*q.Translation.xyz*pos.w), pos.w);
	}
}