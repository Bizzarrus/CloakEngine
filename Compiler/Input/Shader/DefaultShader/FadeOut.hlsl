struct ResizeBuffer {
	float4 Color;
};

ConstantBuffer<ResizeBuffer> Fade : register(b0);

struct PS_INPUT {
	float4 pos : SV_POSITION;
	float2 tex : TEXCOORD0;
};

/////////////////////////////////////////////////
// Vertex Shader
/////////////////////////////////////////////////

PS_INPUT mainVS(uint id : SV_VertexID)
{
	PS_INPUT output = (PS_INPUT)0;
	output.tex = float2((id << 1) & 2, id & 2);
	output.pos = float4((output.tex*float2(2, -2)) + float2(-1, 1), 0, 1);
	return output;
}

/////////////////////////////////////////////////
// Pixel Shader
/////////////////////////////////////////////////
float4 mainPS(PS_INPUT input) : SV_Target
{
	return Fade.Color;
}