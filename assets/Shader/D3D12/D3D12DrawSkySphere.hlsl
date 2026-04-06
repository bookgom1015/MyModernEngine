#ifndef __D3D12DRAWSKYSPHERE_HLSL__
#define __D3D12DRAWSKYSPHERE_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#include "./../../include/Renderer/D3D12/D3D12HlslCompaction.h"

ConstantBuffer<PassCB>   cbPass          : register(b0);
ConstantBuffer<ObjectCB> cbObject        : register(b1);

StructuredBuffer<Vertex> gi_VertexBuffer : register(t0);
ByteAddressBuffer        gi_IndexBuffer  : register(t1);

TextureCube<EnvironmentManager::EnvironmentCubeMapFormat> gi_EnvCubeMap : register(t2);

EnvironmentManager_DrawSkySphere_RootConstants(b2);

VERTEX_IN

struct VertexOut {
    float4 PosH : SV_Position;
    float3 PosL : POSITION;
};

VertexOut VS(in VertexIn vin) {
    VertexOut vout = (VertexOut) 0;

    // Use local vertex position as cubemap lookup vector.
    vout.PosL = vin.PosL;
    
    float4 PosW = mul(float4(vout.PosL, 1.f), cbObject.World);
    // Always center sky about camera.
    PosW.xyz += cbPass.EyePosW;
    
    // Set z = w so that z/w = 1 (i.e., skydome always on far plane).
    vout.PosH = mul(PosW, cbPass.ViewProj).xyww;
    
    return vout;
}

[outputtopology("triangle")]
[numthreads(EnvironmentManager::ThreadGroup::MeshShader::ThreadsPerGroup, 1, 1)]
void MS(
        in uint GTid : SV_GroupThreadID,
        in uint Gid : SV_GroupID,
        out vertices VertexOut verts[MESH_SHADER_MAX_VERTICES],
        out indices uint3 prims[MESH_SHADER_MAX_PRIMITIVES]) {
    const uint TotalPrimCount = gIndexCount / 3;
    const uint GlobalPrimId = Gid * MESH_SHADER_MAX_PRIMITIVES + GTid;
    
    const uint Remaining = TotalPrimCount - Gid * MESH_SHADER_MAX_PRIMITIVES;
    const uint LocalPrimCount = min(Remaining, MESH_SHADER_MAX_PRIMITIVES);
    
    SetMeshOutputCounts(LocalPrimCount * 3, LocalPrimCount);
    
    if (GTid >= LocalPrimCount)
        return;
    
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
    
        Vertex vin = gi_VertexBuffer[Indices[i]];
    
        VertexOut vout = (VertexOut) 0;
        vout.PosL = vin.Position;
        
        float4 PosW = mul(float4(vout.PosL, 1.f), cbObject.World);
        // Always center sky about camera.
        PosW.xyz += cbPass.EyePosW;
        
        vout.PosH = mul(PosW, cbPass.ViewProj).xyww;
     
        verts[OutVert] = vout;
    }
}

HDR_FORMAT PS(in VertexOut pin) : SV_Target {
    //return (float4) 1;
    return gi_EnvCubeMap.SampleLevel(gsamAnisotropicClamp, normalize(pin.PosL), 0);
}

#endif // __D3D12DRAWSKYSPHERE_HLSL__