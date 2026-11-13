#ifndef __D3D12_SSR_HLSL__
#define __D3D12_SSR_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#ifndef _FIT_TO_SCREEN_COORD
#define _FIT_TO_SCREEN_COORD
#endif

#include "./../../include/Renderer/D3D12/D3D12HlslCompaction.h"

ConstantBuffer<PassCB>                  cbPass  : register(b0);
ConstantBuffer<ScreenSpaceReflectionCB> cbSsr   : register(b1);

Texture2D<HDR_FORMAT>                               gi_BackBuffer   : register(t0);
Texture2D<GBuffer::PositionMapFormat>               gi_Position     : register(t1);
Texture2D<GBuffer::NormalMapFormat>                 gi_Normal       : register(t2);
Texture2D<DepthStencilBuffer::DepthBufferFormat>    gi_Depth        : register(t3);
Texture2D<GBuffer::RMSMapFormat>                    gi_RMS          : register(t4);

FitToScreenViewPosVertexOut

FitToScreenViewPosVertexShader(cbPass.InvProj)

FitToScreenViewPosMeshShader(cbPass.InvProj)

HDR_FORMAT PS_View(in VertexOut pin) : SV_Target {
    float depth = gi_Depth.SampleLevel(gsamDepthMap, pin.TexC, 0);
    if (depth == DepthStencilBuffer::InvalidDepthValue)
        return 0;

    depth = ShaderUtil::NdcDepthToViewDepth(depth, cbPass.Proj);

    const float3 posV = (depth / pin.PosV.z) * pin.PosV;
    if (posV.z > cbSsr.MaxDistance)
        return 0;

    const float3 normalW = normalize(gi_Normal.Sample(gsamLinearClamp, pin.TexC).xyz);
    const float3 normalV = normalize(mul(normalW, (float3x3)cbPass.View));

    const float3 toEyeV = normalize(-posV);
    const float3 toLightV = reflect(-toEyeV, normalV);
    const float3 lightPosV = posV + toLightV * cbSsr.RayLength;

    const float3 roughnessMetalicSpecular = gi_RMS.Sample(gsamLinearClamp, pin.TexC).rgb;
    const float shiness = 1.f - roughnessMetalicSpecular.r;

	[loop]
    for (uint i = 0; i < cbSsr.StepCount; ++i) {
        float3 dpv = posV + toLightV * (i + 1.f);
        float4 ph = mul(float4(dpv, 1.f), cbPass.Proj);
        
        ph /= ph.w;
        
        if (abs(ph.y) > 1.f)
            return 0;

        float2 tex = float2(ph.x, -ph.y) * 0.5f + (float2) 0.5;
        float d = gi_Depth.SampleLevel(gsamDepthMap, tex, 0);
        float dv = ShaderUtil::NdcDepthToViewDepth(d, cbPass.Proj);

        const float noise = Random::Rand_1_05(tex) * cbSsr.NoiseIntensity;

        if (ph.z > d) {
            float3 half_r = toLightV;

			[loop]
            for (uint j = 0; j < cbSsr.BackStepCount; ++j) {
                half_r *= 0.5f;
                dpv -= half_r;
                
                ph = mul(float4(dpv, 1.f), cbPass.Proj);
                ph /= ph.w;

                tex = float2(ph.x, -ph.y) * 0.5f + (float2) 0.5f;
                
                d = gi_Depth.SampleLevel(gsamDepthMap, tex, 0);
                dv = ShaderUtil::NdcDepthToViewDepth(d, cbPass.Proj);
                
                if (abs(dpv.z - dv) > cbSsr.DepthThreshold)
                    return (float4) 0;

                if (ph.z < d)
                    dpv += half_r;
            }

            ph = mul(float4(dpv, 1.f), cbPass.Proj);
            ph /= ph.w;

            tex = float2(ph.x, -ph.y) * 0.5f + (float2) 0.5f;

            const float3 color = gi_BackBuffer.Sample(gsamLinearClamp, tex).rgb;
            
            return float4(color, shiness);
        }
    }

    return 0;
}

HDR_FORMAT PS_Screen(VertexOut pin) : SV_Target
{
    uint2 size;
    gi_BackBuffer.GetDimensions(size.x, size.y);
	
    const float4 posW = gi_Position.Sample(gsamLinearClamp, pin.TexC);
    if (!GBuffer::IsValidPosition(posW))
        return 0;
	
    const float3 posV = mul(posW, cbPass.View).xyz;
	
    const float3 normalW = normalize(gi_Normal.Sample(gsamLinearClamp, pin.TexC).xyz);
    const float3 normalV = normalize(mul(normalW, (float3x3)cbPass.View));
	
    const float3 fromEyeV = normalize(posV);
    const float3 toLightV = reflect(fromEyeV, normalV);
	
    float3 startV = posV;
    float3 endV = startV + toLightV * cbSsr.MaxDistance;
    if (endV.z < 0.f)
        return 0;
	
    float4 endH = mul(float4(endV, 1.f), cbPass.Proj);
    endH /= endH.w;
	
    float2 startFrag = pin.TexC;
    startFrag *= size;
	
    float2 endFrag = endH.xy * 0.5f + 0.5f;
    endFrag.y = 1.f - endFrag.y;
    endFrag *= size;
	
    float2 _delta = endFrag - startFrag;
    float useX = abs(_delta.x) >= abs(_delta.y) ? 1.f : 0.f;
    float delta = lerp(abs(_delta.y), abs(_delta.x), useX) * clamp(cbSsr.Resolution, 0.f, 1.f);
    float2 increment = _delta / max(delta, 0.001f);
	
    float search_prev = 0.f;
    float search_curr = 0.f;
	
    uint hit_p0 = 0;
    uint hit_p1 = 0;
	
    float viewDist = startV.z;
    float depth = cbSsr.Thickness;
	
    float2 frag = startFrag;
    float2 texc = frag / size;
    float3 currV = startV;
	
    const uint maxStep = min(delta, 64);
	
    [loop]
    for (uint i = 0; i < maxStep; ++i) {
        frag += increment;
        texc = frag / size;
		
        if (texc.x < 0.f || texc.x > 1.f || texc.y < 0.f || texc.y > 1.f)
            continue;
            
        const float4 _posW = gi_Position.Sample(gsamLinearClamp, texc);
        if (!GBuffer::IsValidPosition(_posW))
            continue;
	
        currV = mul(_posW, cbPass.View).xyz;
	
        search_curr = lerp((frag.y - startFrag.y) / _delta.y, (frag.x - startFrag.x) / _delta.x, useX);
        search_curr = clamp(search_curr, 0.f, 1.f);
	
        viewDist = (startV.z * endV.z) / lerp(endV.z, startV.z, search_curr);
        depth = viewDist - currV.z;
	
        if (depth > 0.f && depth < cbSsr.Thickness) {
            hit_p0 = 1;
            break;
        }
        else {
            search_prev = search_curr;
        }
    }
	
    search_curr = search_prev + ((search_curr - search_prev) * 0.5f);
    if (hit_p0 == 0)
        return 0;
	
    [loop]
    for (uint i = 0; i < 8; ++i) {
        frag = lerp(startFrag, endFrag, search_curr);
        texc = frag / size;
			{
            const float4 _posW = gi_Position.Sample(gsamLinearClamp, texc);
            if (!GBuffer::IsValidPosition(_posW))
                continue;
	
            currV = mul(_posW, cbPass.View).xyz;
        }
	
        viewDist = (startV.z * endV.z) / lerp(endV.z, startV.z, search_curr);
        depth = viewDist - currV.z;
	
        if (depth > 0 && depth < cbSsr.Thickness) {
            hit_p1 = 1;
            search_curr = search_prev + ((search_curr - search_prev) * 0.5f);
        }
        else {
            const float temp = search_curr;
            search_curr = search_curr + ((search_curr - search_prev) * 0.5f);
            search_prev = temp;
        }
    }
	
    return gi_BackBuffer.Sample(gsamLinearClamp, texc);
}

#endif // __D3D12_SSR_HLSL__