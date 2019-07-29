/*
Weighted order independend blending
Algorithm by Morgan McGuire and Louis Bavoil
*/

namespace WOIBlending {
	/*
	Calculate reveal and accum by color (after light calculation) and z value:

	float alpha=CalculateAlpha(color);
	float weight=CalculateWeight(z, alpha); //Use "CalculateWeightByViewSpace" or "CalculateWeightByScreenSpace"
	float4 accum;
	float reveal;
	CalculateOutput(color, weight, accum, reveal);

	Save accum in RGBA16_FLOAT texture with Blend-Source and Blend-Destination set to ONE
	Save reveal in R16_FLOAT texture with Blend-Source set to ZERO and Blend-Destination set to ONE_MINUS_SRC_ALPHA

	accum-texture should be cleared to float4(0)
	reveal-texture should be cleared to float(1)

	After all transparent objects were rendered, use a full-screen-pass and call "CalculateAccumulatedColor"
	Save color to final render target with Blend-Source set to ONE_MINUS_SRC_ALPHA and Blend-Destination set to SRC_ALPHA
	*/

	float CalculateAlpha(float4 color)
	{
		return max(min(1.0, max(max(color.r, color.g), color.b)*color.a), color.a);
	}
	void CalculateOutput(float4 color, float weight, out float4 accum, out float reveal)
	{
		accum = float4(color.rgb*color.a, color.a)*weight;
		reveal = color.a;
	}
	float CalculateWeightByViewSpace(float viewSpaceZ, float alpha)
	{
		float z = abs(viewSpaceZ) / 200;
		z = z*z;
		z = z*z;
		//z = pow(viewSpaceZ/200,4.0);
		return alpha*clamp(0.03 / (1e-5 + z), 1e-2, 3e3);
		/*
		Alternative computations:

		// Alternative 1:
		float2 z = float2(abs(viewSpaceZ) / 5, abs(viewSpaceZ) / 200);
		z.x = z.x*z.x;
		z.y = z.y*z.y;
		z.y = z.y*z.y*z.y;
		//z.x = pow(viewSpaceZ/5,2.0);
		//z.y = pow(viewSpaceZ/200,6.0);
		return alpha*clamp(10 / (1e-5 + z.x + z.y), 1e-2, 3e3);

		// Alternative 2:
		float2 z = float2(abs(viewSpaceZ) / 10, abs(viewSpaceZ) / 200);
		z.x = z.x*z.x*z.x;
		z.y = z.y*z.y;
		z.y = z.y*z.y*z.y;
		//z.x = pow(viewSpaceZ/10,3.0);
		//z.y = pow(viewSpaceZ/200,6.0);
		return alpha*clamp(10 / (1e-5 + z.x + z.y), 1e-2, 3e3);
		*/
	}
	float CalculateWeightByScreenSpace(float screenSpaceZ, float alpha)
	{
		float z = 1 - screenSpaceZ;
		z = z*z*z;
		//z = pow(1-screenSpaceZ,3.0);
		return alpha*max(1e-2, 3e3*z);
	}

	float4 CalculateAccumulatedColor(float4 accum, float reveal)
	{
		return float4(accum.rgb / max(accum.a, 1e-5), reveal);
	}
}