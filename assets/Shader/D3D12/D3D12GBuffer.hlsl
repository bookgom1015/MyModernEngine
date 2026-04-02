#ifndef __D3D12GBUFFER_HLSL__
#define __D3D12GBUFFER_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#include "./../../include/Renderer/D3D12/D3D12HlslCompaction.h"
#include "./../../assets/Shader/D3D12/D3D12GBuffer.hlsli"

ConstantBuffer<PassCB>     cbPass      : register(b0);
ConstantBuffer<ObjectCB>   cbObject    : register(b1);
ConstantBuffer<MaterialCB> cbMaterial  : register(b2);

GBuffer_Default_RootConstants(b3)

StructuredBuffer<Vertex>        gi_StaticVertexBuffer  : register(t0);
StructuredBuffer<SkinnedVertex> gi_SkinnedVertexBuffer : register(t1);
ByteAddressBuffer               gi_IndexBuffer         : register(t2);

StructuredBuffer<float4x4>      gi_BonePalette         : register(t3);

Texture2D<float4>        gi_AlbedoMap : register(t0, space1);
Texture2D<float4>        gi_NormalMap : register(t1, space1);

VERTEX_IN

SKINNED_VERTEX_IN

struct VertexOut {
    float4 PosH        : SV_Position;
    float4 CurrPosH    : POSITION0;
    float4 PrevPosH    : POSITION1;
    float3 PosW        : POSITION2;
    float3 PosL        : POSITION3;
    float3 NormalW     : NORMAL0;
    float4 TangentW    : TANGENT0;
    float3 PrevNormalW : NORMAL1;
    float2 TexC        : TEXCOORD;
};

struct PixelOut {
    GBuffer::AlbedoMapFormat             Color           : SV_TARGET0;
    GBuffer::NormalMapFormat             Normal          : SV_TARGET1;
    GBuffer::NormalDepthMapFormat        NormalDepth     : SV_TARGET2;
    GBuffer::NormalDepthMapFormat        PrevNormalDepth : SV_TARGET3;
    GBuffer::RMSMapFormat                RMS             : SV_TARGET4;
    GBuffer::VelocityMapFormat           Velocity        : SV_TARGET5;
    GBuffer::PositionMapFormat           Position        : SV_TARGET6;
};
                                                                                                                                                                                                                                                                            
VertexOut VS_Static(in VertexIn vin) {
    VertexOut vout = (VertexOut)0;
    
    vout.PosL = vin.PosL;
    
    const float4 PosW = mul(float4(vin.PosL, 1.f), cbObject.World);
    vout.PosW = PosW.xyz;
    
    const float4 PosH = mul(PosW, cbPass.ViewProj);
    vout.CurrPosH = PosH;
    vout.PosH = PosH + float4(cbPass.JitteredOffset * PosH.w, 0, 0);
    
    const float4 PrevPosW = mul(float4(vin.PosL, 1), cbObject.PrevWorld);
    vout.PrevPosH = mul(PrevPosW, cbPass.PrevViewProj);
    
    vout.NormalW = mul(vin.NormalL, (float3x3)cbObject.World);
    vout.PrevNormalW = mul(vin.NormalL, (float3x3)cbObject.PrevWorld);
    
    vout.TangentW = float4(mul(vin.TangentL.xyz, (float3x3)cbObject.World), vin.TangentL.w);
    
    float4 TexC = mul(float4(vin.TexC, 0.f, 1.f), cbObject.TexTransform);
    vout.TexC = mul(TexC, cbMaterial.MatTransform).xy;
    
    return vout;
}

float4x4 CalcSkinMatrix(uint4 joints, float4 weights) {
    float4x4 m0 = gi_BonePalette[cbObject.BonePaletteOffset + joints.x];
    float4x4 m1 = gi_BonePalette[cbObject.BonePaletteOffset + joints.y];
    float4x4 m2 = gi_BonePalette[cbObject.BonePaletteOffset + joints.z];
    float4x4 m3 = gi_BonePalette[cbObject.BonePaletteOffset + joints.w];

    return m0 * weights.x +
           m1 * weights.y +
           m2 * weights.z +
           m3 * weights.w;
}

VertexOut VS_Skinned(in SkinnedVertexIn vin) {
    VertexOut vout = (VertexOut)0;
    
    const float4x4 Skin = CalcSkinMatrix(vin.JointIndices, vin.JointWeights);
        
    vout.PosL = vin.PosL;
    
    const float4 PosS = mul(float4(vin.PosL, 1.f), Skin);    
    const float4 PosW = mul(PosS, cbObject.World);
    vout.PosW = PosW.xyz;
    
    const float4 PosH = mul(PosW, cbPass.ViewProj);
    vout.CurrPosH = PosH;
    vout.PosH = PosH + float4(cbPass.JitteredOffset * PosH.w, 0, 0);
    
    const float4 PrevPosW = mul(float4(vin.PosL, 1), cbObject.PrevWorld);
    vout.PrevPosH = mul(PrevPosW, cbPass.PrevViewProj);
    
    const float3 NormalS = mul(vin.NormalL, (float3x3)Skin);    
    vout.NormalW = mul(NormalS, (float3x3)cbObject.World);
    // 이런 썅 전 프레임 팔레트 들고 있어야지 이전 프레임의 노멀 계산이 가능
    // 우선 현재 프레임의 노말만 계산, 아직 전 프레임 노멀 쓰지 않으니 문제는 없음.
    // **수정해**수정해**수정해**수정해**수정해**수정해**수정해**수정해**수정해**수정해**
    vout.PrevNormalW = mul(vin.NormalL, (float3x3)cbObject.PrevWorld);
    
    const float3 TangentS = mul(vin.TangentL.xyz, (float3x3)Skin);
    vout.TangentW = float4(mul(TangentS, (float3x3)cbObject.World), vin.TangentL.w);
    
    float4 TexC = mul(float4(vin.TexC, 0.f, 1.f), cbObject.TexTransform);
    vout.TexC = mul(TexC, cbMaterial.MatTransform).xy;
    
    return vout;
}

[outputtopology("triangle")]
[numthreads(GBuffer::ThreadGroup::MeshShader::ThreadsPerGroup, 1, 1)]
void MS_Static(
        in uint GTid : SV_GroupThreadID,
        in uint Gid : SV_GroupID,
        out vertices VertexOut verts[MESH_SHADER_MAX_VERTICES],
        out indices uint3 prims[MESH_SHADER_MAX_PRIMITIVES]) {
    const uint TotalPrimCount = gIndexCount / 3;
    const uint PrimIdOffset = gStartIndex / 3;
    const uint GlobalPrimId = Gid * MESH_SHADER_MAX_PRIMITIVES + GTid + PrimIdOffset;
    
    const uint Remaining = TotalPrimCount - Gid * MESH_SHADER_MAX_PRIMITIVES;
    const uint LocalPrimCount = min(Remaining, MESH_SHADER_MAX_PRIMITIVES);
        
    SetMeshOutputCounts(LocalPrimCount * 3, LocalPrimCount);
    
    if (GTid >= LocalPrimCount) return;
    
    const uint LocalPrimId = GTid;
    const uint PrimIndex = Gid * MESH_SHADER_MAX_PRIMITIVES + LocalPrimId;
    
    const uint3 Indices = uint3(
        ShaderUtil::GetIndex32(gi_IndexBuffer, GlobalPrimId * 3 + 0),
        ShaderUtil::GetIndex32(gi_IndexBuffer, GlobalPrimId * 3 + 1),
        ShaderUtil::GetIndex32(gi_IndexBuffer, GlobalPrimId * 3 + 2));
        
    prims[LocalPrimId] = uint3(
        LocalPrimId * 3 + 0,
        LocalPrimId * 3 + 1,
        LocalPrimId * 3 + 2
    );
    
    [unroll]
    for (uint i = 0; i < 3; ++i) {
        const uint OutVert = LocalPrimId * 3 + i;
    
        Vertex vin = gi_StaticVertexBuffer[Indices[i]];
    
        VertexOut vout = (VertexOut) 0;
        vout.PosL = vin.Position;
        
        float4 PosW = mul(float4(vout.PosL, 1.f), cbObject.World);
        vout.PosW = PosW.xyz;
    
        const float4 PosH = mul(PosW, cbPass.ViewProj);
        vout.CurrPosH = PosH;
        vout.PosH = PosH + float4(cbPass.JitteredOffset * PosH.w, 0, 0);
    
        const float4 PrevPosW = mul(float4(vin.Position, 1), cbObject.PrevWorld);
        vout.PrevPosH = mul(PrevPosW, cbPass.PrevViewProj);
        
        vout.NormalW = mul(vin.Normal, (float3x3)cbObject.World);
        vout.PrevNormalW = mul(vin.Normal, (float3x3)cbObject.PrevWorld);
        
        vout.TangentW = float4(mul(vin.Tangent.xyz, (float3x3)cbObject.World), vin.Tangent.w);
    
        float4 TexC = mul(float4(vin.TexCoord, 0.f, 1.f), cbObject.TexTransform);
        vout.TexC = TexC.xy;
     
        verts[OutVert] = vout;
    }
}

[outputtopology("triangle")]
[numthreads(GBuffer::ThreadGroup::MeshShader::ThreadsPerGroup, 1, 1)]
void MS_Skinned(
        in uint GTid : SV_GroupThreadID,
        in uint Gid : SV_GroupID,
        out vertices VertexOut verts[MESH_SHADER_MAX_VERTICES],
        out indices uint3 prims[MESH_SHADER_MAX_PRIMITIVES]) {
    const uint TotalPrimCount = gIndexCount / 3;
    const uint PrimIdOffset = gStartIndex / 3;
    const uint GlobalPrimId = Gid * MESH_SHADER_MAX_PRIMITIVES + GTid + PrimIdOffset;
    
    const uint Remaining = TotalPrimCount - Gid * MESH_SHADER_MAX_PRIMITIVES;
    const uint LocalPrimCount = min(Remaining, MESH_SHADER_MAX_PRIMITIVES);
        
    SetMeshOutputCounts(LocalPrimCount * 3, LocalPrimCount);
    
    if (GTid >= LocalPrimCount) return;
    
    const uint LocalPrimId = GTid;
    const uint PrimIndex = Gid * MESH_SHADER_MAX_PRIMITIVES + LocalPrimId;
    
    const uint3 Indices = uint3(
        ShaderUtil::GetIndex32(gi_IndexBuffer, GlobalPrimId * 3 + 0),
        ShaderUtil::GetIndex32(gi_IndexBuffer, GlobalPrimId * 3 + 1),
        ShaderUtil::GetIndex32(gi_IndexBuffer, GlobalPrimId * 3 + 2));
        
    prims[LocalPrimId] = uint3(
        LocalPrimId * 3 + 0,
        LocalPrimId * 3 + 1,
        LocalPrimId * 3 + 2
    );
    
    [unroll]
    for (uint i = 0; i < 3; ++i) {
        const uint OutVert = LocalPrimId * 3 + i;
    
        SkinnedVertex vin = gi_SkinnedVertexBuffer[Indices[i]];
        
        const float4x4 Skin = CalcSkinMatrix(vin.JointIndices, vin.JointWeights);
    
        VertexOut vout = (VertexOut) 0;
        vout.PosL = vin.Position;
        
        const float4 PosS = mul(float4(vin.Position, 1.f), Skin);    
        const float4 PosW = mul(PosS, cbObject.World);
        vout.PosW = PosW.xyz;
    
        const float4 PosH = mul(PosW, cbPass.ViewProj);
        vout.CurrPosH = PosH;
        vout.PosH = PosH + float4(cbPass.JitteredOffset * PosH.w, 0, 0);
    
        const float4 PrevPosW = mul(float4(vin.Position, 1), cbObject.PrevWorld);
        vout.PrevPosH = mul(PrevPosW, cbPass.PrevViewProj);
        
        const float3 NormalS = mul(vin.Normal, (float3x3)Skin);    
        vout.NormalW = mul(NormalS, (float3x3)cbObject.World);
        // 이런 썅 전 프레임 팔레트 들고 있어야지 이전 프레임의 노멀 계산이 가능
        // 우선 현재 프레임의 노말만 계산, 아직 전 프레임 노멀 쓰지 않으니 문제는 없음.
        // **수정해**수정해**수정해**수정해**수정해**수정해**수정해**수정해**수정해**수정해**
        vout.PrevNormalW = mul(vin.Normal, (float3x3)cbObject.PrevWorld);        
        
        const float3 TangentS = mul(vin.Tangent.xyz, (float3x3)Skin);
        vout.TangentW = float4(mul(TangentS, (float3x3)cbObject.World), vin.Tangent.w);
    
        float4 TexC = mul(float4(vin.TexCoord, 0.f, 1.f), cbObject.TexTransform);
        vout.TexC = TexC.xy;
     
        verts[OutVert] = vout;
    }
}

PixelOut PS(in VertexOut pin) {
    pin.CurrPosH /= pin.CurrPosH.w;

    const float2 UV = pin.CurrPosH.xy * 0.5f + 0.5f;
    const uint2 ScreenPos = (uint2)floor(UV * gTexDim);
    const uint2 ScreenPos_xN = ScreenPos >> 1;
    const float Threshold = gThresholdMatrix8x8[ScreenPos_xN.y & 7][ScreenPos_xN.x & 7];
    
    float4 posV = mul(pin.CurrPosH, cbPass.InvProj);
	posV /= posV.w;

	const float Dist = (posV.z - gDitheringMinDist) / (gDitheringMaxDist - gDitheringMinDist);

	clip(Dist - Threshold);
    
    PixelOut pout = (PixelOut) 0;
    
    pin.PrevPosH /= pin.PrevPosH.w;
    const float2 Velocity = ShaderUtil::CalcVelocity(pin.CurrPosH, pin.PrevPosH);
    
    float4 albedo = float4(cbMaterial.Albedo, 1.f);
    if (gHasAlbedoMap) {
        float2 texc = pin.TexC;
        texc.y = 1.f - texc.y;
        albedo *= gi_AlbedoMap.SampleLevel(gsamLinearClamp, texc, 0);
    }
    
    float3 normal = normalize(pin.NormalW);
    if (gHasNormalMap) {
        float2 texc = pin.TexC;
        texc.y = 1.f - texc.y;
    
        normal = gi_NormalMap.SampleLevel(gsamLinearClamp, texc, 0).xyz;
        normal = normal * 2.f - 1.f;
        normal.y = -normal.y; // 중요
        
        float3 N = normalize(pin.NormalW);
        float3 T = normalize(pin.TangentW.xyz);
        T = normalize(T - N * dot(T, N));
        float3 B = cross(N, T) * pin.TangentW.w;
        
        float3x3 TBN = float3x3(T, B, N);
        
        normal = mul(normal, TBN);
    }
    
    pout.Color = albedo;
    pout.Normal = float4(normal, 1.f);
    pout.NormalDepth = ValuePackaging::EncodeNormalDepth(pin.NormalW, pin.CurrPosH.z);
    pout.PrevNormalDepth = ValuePackaging::EncodeNormalDepth(pin.PrevNormalW, pin.PrevPosH.z);
    pout.RMS = float3(cbMaterial.Roughness, cbMaterial.Metalness, cbMaterial.Specular);
    pout.Velocity = Velocity;
    pout.Position = float4(pin.PosW, 1.f);
    
    return pout;
}


#endif // __D3D12GBUFFER_HLSL__