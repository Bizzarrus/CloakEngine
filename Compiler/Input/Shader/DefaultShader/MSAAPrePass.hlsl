#ifdef SHADER_MSAA
Texture2DMS<uint, SHADER_MSAA> texCoverage : register(t0);
#else
Texture2D<uint> texCoverage : register(t0);
#endif

struct PS_INPUT {
	float4 pos : SV_POSITION;
};

/////////////////////////////////////////////////
// Vertex Shader
/////////////////////////////////////////////////

PS_INPUT mainVS(uint id : SV_VertexID)
{
	PS_INPUT output = (PS_INPUT)0;
	float2 tex = float2((id << 1) & 2, id & 2);
	output.pos = float4((tex*float2(2, -2)) + float2(-1, 1), 0, 1);
	return output;
}

/////////////////////////////////////////////////
// Pixel Shader
/////////////////////////////////////////////////

//Seperating pixels in stencil buffer: 0 = simple pixel (all values the same), not 0 = complex pixel
//Notice that texCoverage is filled with the coverage-mask, which is geometry based, not image based, therefore it overestimates (marks pixels as complex that aren't)
uint mainPS(PS_INPUT input) : SV_StencilRef
{
#ifdef SHADER_MSAA
	uint res = 0;
	[unroll]
	for (uint a = 0; a < SHADER_MSAA; a++)
	{
		uint c = texCoverage.Load(input.pos.xy, a);
		res |= MAX_COVERAGE & (~c);
	}
	return res;
#else
	return 0;
#endif
}