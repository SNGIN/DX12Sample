cbuffer cbTansMatrix : register(b0) {
	matrix mW;
	matrix mWVP;
	float4 vLightDir;
	float4 vEye;
	float4 vAmbient = float4(0, 0, 0, 0);
	float4 vDiffuse = float4(1, 0, 0, 0);
	float4 vSpecular = float4(1, 1, 1, 1);
};

Texture2D<float4> tex0 : register(t0);
SamplerState samp0 : register(s0);

struct VS_INPUT {
	float3 Position : POSITION;
	float3 Normal	: NORMAL;
	float2 Tex		: TEXCOORD;
};

struct PS_INPUT {//(VS_OUTPUT)
	float4 Pos : SV_POSITION;
	float4 Color : COLOR0;
	float3 Light : TEXCOORD0;
	float3 Normal : TEXCOORD1;
	float3 EyeVector : TEXCOORD2;
	float2 Tex : TEXCOORD3;
};

PS_INPUT VSMain(VS_INPUT input) {
	PS_INPUT output;

	float4 Pos = float4(input.Position, 1.0f);
	float4 Nrm = float4(input.Normal, 1.0f);

	output.Pos = mul(Pos, mWVP);

	output.Light = vLightDir;

	float3 PosWorld = mul(input.Position, mW);
	output.EyeVector = vEye - PosWorld;

	output.Normal = mul(Nrm, mWVP);

	float3 Normal = normalize(output.Normal);
	float3 LightDir = normalize(output.Light);
	float3 ViewDir = normalize(output.EyeVector);
	float4 NL = saturate(dot(Normal, LightDir));
	float3 Reflect = normalize(2 * NL * Normal - LightDir);
	float4 specular = pow(saturate(dot(Reflect, ViewDir)), 4);
	output.Color = vDiffuse * NL + specular * vSpecular;

	output.Tex = input.Tex;
	
	return output;
}

float4  PSMain(PS_INPUT input) : SV_TARGET{
	float4 color = tex0.Sample(samp0, input.Tex);
	color += input.Color / 2;

	return input.Color;
}