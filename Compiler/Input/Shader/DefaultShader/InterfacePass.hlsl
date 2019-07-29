#include "GBufferEncoding.hlsli"

struct GammaBufferData {
	float Gamma;
};

SamplerState texSampler : register(s0);
Texture2D<float4> tex : register(t0);
ConstantBuffer<GammaBufferData> gammaBuffer : register(b0);

struct VS_INPUT {
	float2 pos : POSITION;
	float2 size : SIZE;
	float2 texTL : TEXTL;
	float2 texBR : TEXBR;
	float4 colorR : COLORR;
	float4 colorG : COLORG;
	float4 colorB : COLORB;
	float4 scissor : SCISSOR;
	float2 rotation : ROTATE;
	uint Flags : FLAGS;
};

struct PS_INPUT {
	float4 pos : SV_POSITION;	
	float2 tex : TEXCOORD0;
	float2 tpos : TEXPOS;
	float4 scissor : SCISSOR;
	float4 color0 : COLOR0;
	float4 color1 : COLOR1;
	float4 color2 : COLOR2;
};

#define GRAYSCALE float3(0.2126f, 0.7152f, 0.0722f)

/////////////////////////////////////////////////
// Vertex Shader
/////////////////////////////////////////////////

float2 RotateCoord(float2 cen, float2 rot, float2 dir)
{
	return cen + float2(dot(dir.xy, float2(rot.y, -rot.x)), dot(rot, dir));
}

PS_INPUT mainVS(VS_INPUT input, uint id : SV_VertexID)
{
	PS_INPUT output = (PS_INPUT)0;

	float2 hs = 0.5f*input.size;
	float2 cen = input.pos + hs;
	float2 pos = float2(0, 0);
	float2 tex = float2(0, 0);
	if ((id & 0x1) == 0) //Left
	{
		pos.x = -hs.x;
		tex.x = input.texTL.x;
	}
	else //Right
	{
		pos.x = hs.x;
		tex.x = input.texBR.x;
	}
	if ((id & 0x2) == 0) //Top
	{
		pos.y = -hs.y;
		tex.y = input.texTL.y;
	}
	else //Bottom
	{
		pos.y = hs.y;
		tex.y = input.texBR.y;
	}

	output.tpos = RotateCoord(cen, input.rotation, pos);
	output.pos = float4(mad(output.tpos, float2(2, -2), float2(-1, 1)), 0, 1);
	output.tex = tex;
	output.scissor = input.scissor;

	//Color:
	if ((input.Flags & 0x1) != 0)
	{
		float colW = max(input.colorR.w, max(input.colorG.w, input.colorB.w));
		output.color0 = float4(input.colorR.xyz * input.colorR.w / colW, colW);
		output.color1 = float4(input.colorG.xyz * input.colorG.w / colW, colW);
		output.color2 = float4(input.colorB.xyz * input.colorB.w / colW, colW);
	}
	else
	{
		output.color0 = float4(input.colorR.x, 0, 0, input.colorR.w);
		output.color1 = float4(0, input.colorR.y, 0, input.colorR.w);
		output.color2 = float4(0, 0, input.colorR.z, input.colorR.w);
	}

	//Grayscale
	if ((input.Flags & 0x2) != 0)
	{
		output.color0.xyz = dot(output.color0.xyz, GRAYSCALE);
		output.color1.xyz = dot(output.color1.xyz, GRAYSCALE);
		output.color2.xyz = dot(output.color2.xyz, GRAYSCALE);
	}

	return output;
}

/////////////////////////////////////////////////
// Pixel Shader
/////////////////////////////////////////////////

float4 mainPS(PS_INPUT input) : SV_Target
{
	float4 col = tex.Sample(texSampler,input.tex);
	float2 tp = input.tpos - input.scissor.xy;
	if (tp.x < 0 || tp.y < 0 || tp.x > input.scissor.z || tp.y > input.scissor.w) { col.w = 0; }
	float4 res = float4((col.x * input.color0.xyz) + (col.y * input.color1.xyz) + (col.z * input.color2.xyz), col.w * input.color0.w);
	return GammaCorrection::InlineGamma(res, gammaBuffer.Gamma);
}