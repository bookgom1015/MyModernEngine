#ifndef __D3D12DRAWDEPTH_HLSL__
#define __D3D12DRAWDEPTH_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#include "./../../include/Renderer/D3D12/D3D12HlslCompaction.h"
#include "./../../include/LightData.h"

#include "./../../assets/Shader/D3D12/D3D12Shadow.hlsli"

ConstantBuffer<LightCB>     cbLight     : register(b0);
ConstantBuffer<ObjectCB>    cbObject    : register(b1);
ConstantBuffer<MaterialCB>  cbMaterial  : register(b2);

Shadow_DrawDepth_RootConstants(b3)

VERTEX_IN

struct VertexOut {
    float4 PosW : SV_POSITION;
    float2 TexC : TEXCOORD;
};

struct GeoOut {
    float4  PosH        : SV_POSITION;
    float2  TexC        : TEXCOORD;
    uint    ArrayIndex  : SV_RenderTargetArrayIndex;
};

VertexOut VS(in VertexIn vin) {
    VertexOut vout = (VertexOut)0;

    vout.PosW = mul(float4(vin.PosL, 1.f), cbObject.World);
    
    const float4 TexC = mul(float4(vin.TexC, 0.f, 1.f), cbObject.TexTransform);
    vout.TexC = mul(TexC, cbMaterial.MatTransform).xy;

    return vout;
}

[maxvertexcount(18)]
void GS(in triangle VertexOut gin[3], inout TriangleStream<GeoOut> triStream) {
    GeoOut gout = (GeoOut)0;
	
    LightData light = cbLight.Lights[gLightIndex];

	[loop]
    for (uint index = 0; index < gIndexStride; ++index) {
        gout.ArrayIndex = gBaseIndex + index;

        const float4x4 ViewProj = GetViewProjMatrix(light, index);

		[unroll]
        for (uint i = 0; i < 3; ++i) {
            gout.PosH = mul(gin[i].PosW, ViewProj);
            gout.TexC = gin[i].TexC;

            triStream.Append(gout);
        }

        triStream.RestartStrip();
    }
}

void PS(in GeoOut pin) {
    //float4 albedo = cbMaterial.Albedo;
    //
    //if (cbMaterial.AlbedoMapIndex != -1)
    //    albedo *= gi_Textures[cbMaterial.AlbedoMapIndex].SampleLevel(gsamAnisotropicClamp, pin.TexC, 0);

#ifdef ALPHA_TEST
	// Discard pixel if texture alpha < 0.1.  We do this test as soon 
	// as possible in the shader so that we can potentially exit the
	// shader early, thereby skipping the rest of the shader code.
	clip(albedo.a - 0.1f);
#endif
}

#endif // __D3D12DRAWDEPTH_HLSL__