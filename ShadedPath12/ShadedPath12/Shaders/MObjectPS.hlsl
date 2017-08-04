#include "lights_basic.hlsi"
#include "MObject.hlsli"

[RootSignature(ObjectRS)]

float4 main(PSInput input) : SV_TARGET
{
/*	Material m;
	m.ambient = float4(1,1,1,1);
	m.specExp = 0;
	m.specIntensity = 0;
	m.isLightSource = 0;
	m.fill2 = 0;
*/	float2 thisTexCoord = input.Tex;
	float3 texColor;
	texColor = screenTex.Sample(s, thisTexCoord).xyz;
	float3 finalColor = float4(1,1,1,1);
	  //return float4(texColor,1);
	finalColor = applyLighting(texColor, input.Pos.xyz, input.PosW, input.Normal, cbv[0].cameraPos, cbv[0].material);

	float4 alphaColor;
	alphaColor.rgb = finalColor;
	alphaColor.a = cbv[0].alpha;
	//return float4(0.003125, 0.003125, 0.003125, 1);
	return clamp(alphaColor, 0.0021973, 1.0);  // prevent strange smearing effect for total black pixels (only in HMD)
}

