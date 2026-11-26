#ifndef __D3D12SHADERUITL_HLSLI__
#define __D3D12SHADERUITL_HLSLI__

#ifndef FitToScreenVertexOut
#define FitToScreenVertexOut    \
struct VertexOut {              \
    float4 PosH : SV_Position;  \
    float2 TexC : TexCoord;     \
};
#endif // FitToScreenVertexOut

#ifndef FitToScreenVertexShader
//-----------------------------------------------------------------------------------------------------------
//
// UV로 화면 전체를 덮는 정점 셰이더를 정의한다.
//
//-----------------------------------------------------------------------------------------------------------
#define FitToScreenVertexShader                          \
VertexOut VS(                                            \
        in uint vid : SV_VertexID,                       \
        in uint instanceID : SV_InstanceID) {            \
    VertexOut vout = (VertexOut)0;                       \
    vout.TexC = gVertices[ vid];                         \
    vout.PosH = ShaderUtil::TexCoordToScreen(vout.TexC); \
    return vout;                                         \
}
#endif // FitToScreenVertexShader

#ifndef FitToCubeVertexShader
//-----------------------------------------------------------------------------------------------------------
//
// 큐브의 로컬 정점 위치를 넘기는 정점 셰이더를 정의한다.
//
//-----------------------------------------------------------------------------------------------------------
#define FitToCubeVertexShader             \
VertexOut VS(in uint vid : SV_VertexID) { \
    VertexOut vout = (VertexOut) 0;       \
    vout.PosL = gVertices[vid];           \
    return vout;                          \
}
#endif // FitToCubeVertexShader

#ifndef FitToCubeGeometryShader
//-----------------------------------------------------------------------------------------------------------
//
// 입력 삼각형을 큐브맵의 여섯 면에 투영하는 지오메트리 셰이더를 정의한다.
//
//-----------------------------------------------------------------------------------------------------------
#define FitToCubeGeometryShader(__cbProjectToCube)                              \
[maxvertexcount(18)]                                                            \
void GS(in triangle VertexOut gin[3], inout TriangleStream<GeoOut> triStream) { \
    GeoOut gout = (GeoOut) 0;                                                   \
	[unroll]                                                                    \
    for (uint face = 0; face < 6; ++face) {                                     \
        const float4x4 view = __cbProjectToCube.Views[face];                    \
		[unroll]                                                                \
        for (uint i = 0; i < 3; ++i) {                                          \
            const float3 posL = gin[i].PosL;                                    \
            const float3 posV = mul(posL, (float3x3)view);                      \
            const float4 posH = mul(float4(posV, 1.f), __cbProjectToCube.Proj); \
            gout.PosL = posL;                                                   \
            gout.PosH = posH.xyww;                                              \
            gout.ArrayIndex = face;                                             \
            triStream.Append(gout);                                             \
        }                                                                       \
        triStream.RestartStrip();                                               \
    }                                                                           \
}
#endif // FitToCubeGeometryShader

#ifndef FitToScreenMeshShader
//-----------------------------------------------------------------------------------------------------------
//
// 화면 전체를 덮는 삼각형 두 개를 생성하는 메시 셰이더를 정의한다.
//
//-----------------------------------------------------------------------------------------------------------
#define FitToScreenMeshShader                                \
[outputtopology("triangle")]                                 \
[numthreads(2, 1, 1)]                                        \
void MS(                                                     \
        in uint GTid : SV_GroupThreadID,                     \
        in uint Gid : SV_GroupID,                            \
        out vertices VertexOut verts[6],                     \
        out indices uint3 prims[2]) {                        \
    SetMeshOutputCounts(6, 2);                               \
    const uint VertexBase = GTid * 3;                        \
    const uint3 Index = uint3(                               \
        VertexBase + 0,                                      \
        VertexBase + 1,                                      \
        VertexBase + 2);                                     \
    prims[GTid] = Index;                                     \
    [unroll]                                                 \
    for (uint i = 0;i < 3; ++i) {                            \
        VertexOut vout = (VertexOut)0;                       \
        vout.TexC = gVertices[Index[i]];                     \
        vout.PosH = ShaderUtil::TexCoordToScreen(vout.TexC); \
        verts[VertexBase + i] = vout;                        \
    }                                                        \
}
#endif // FitToScreenMeshShader

#ifndef FitToScreenViewPosVertexOut
#define FitToScreenViewPosVertexOut     \
struct VertexOut {                      \
    float4 PosH : SV_Position;          \
    float3 PosV : ViewPosition;         \
    float2 TexC : TexCoord;             \
};
#endif // FitToScreenViewPosVertexOut

#ifndef FitToScreenViewPosVertexShader
//-----------------------------------------------------------------------------------------------------------
//
// 화면 정점과 뷰 공간 위치를 출력하는 정점 셰이더를 정의한다.
//
// @param InvProj : 역투영 행렬
//
//-----------------------------------------------------------------------------------------------------------
#define FitToScreenViewPosVertexShader(InvProj)          \
VertexOut VS(                                            \
        in uint vid : SV_VertexID,                       \
        in uint instanceID : SV_InstanceID) {            \
    VertexOut vout = (VertexOut)0;                       \
    vout.TexC = gVertices[ vid];                         \
    vout.PosH = ShaderUtil::TexCoordToScreen(vout.TexC); \
    const float4 ph = mul(vout.PosH, InvProj);           \
	vout.PosV = ph.xyz / ph.w;                           \
    return vout;                                         \
}
#endif // FitToScreenViewPosVertexShader

#ifndef FitToScreenViewPosMeshShader
//-----------------------------------------------------------------------------------------------------------
//
// 화면 정점과 뷰 공간 위치를 출력하는 메시 셰이더를 정의한다.
//
// @param InvProj : 역투영 행렬
//
//-----------------------------------------------------------------------------------------------------------
#define FitToScreenViewPosMeshShader(InvProj)                \
[outputtopology("triangle")]                                 \
[numthreads(2, 1, 1)]                                        \
void MS(                                                     \
        in uint GTid : SV_GroupThreadID,                     \
        in uint Gid : SV_GroupID,                            \
        out vertices VertexOut verts[6],                     \
        out indices uint3 prims[2]) {                        \
    SetMeshOutputCounts(6, 2);                               \
    const uint VertexBase = GTid * 3;                        \
    const uint3 Index = uint3(                               \
        VertexBase + 0,                                      \
        VertexBase + 1,                                      \
        VertexBase + 2);                                     \
    prims[GTid] = Index;                                     \
    [unroll]                                                 \
    for (uint i = 0;i < 3; ++i) {                            \
        VertexOut vout = (VertexOut)0;                       \
        vout.TexC = gVertices[Index[i]];                     \
        vout.PosH = ShaderUtil::TexCoordToScreen(vout.TexC); \
        const float4 ph = mul(vout.PosH, InvProj);           \
	    vout.PosV = ph.xyz / ph.w;                           \
        verts[VertexBase + i] = vout;                        \
    }                                                        \
}
#endif // FitToScreenViewPosMeshShader

namespace ShaderUtil {
    //-----------------------------------------------------------------------------------------------------------
    //
    // @param texc : 텍스처 UV
    //
    // @return 화면에 대응하는 동차 클립 좌표
    //
    //-----------------------------------------------------------------------------------------------------------
    float4 TexCoordToScreen(in float2 texc) {
        return float4(2.f * texc.x - 1.f, 1.f - 2.f * texc.y, 0.f, 1.f);
    }
    
    //-----------------------------------------------------------------------------------------------------------
    //
    // 32비트 워드에서 인덱스의 홀짝에 맞는 16비트를 추출한다.
    //
    // @param buf : 16비트 인덱스 버퍼
    // @param index : 인덱스 원소 번호
    //
    // @return 읽어 온 인덱스
    //
    //-----------------------------------------------------------------------------------------------------------
    uint GetIndex16(in ByteAddressBuffer buf, in uint index) {
        const uint BaseIndex = index / 2;
        const bool IsOdd = index % 2 != 0;
        
        uint value = 0;
        if (IsOdd) {
            value = buf.Load(BaseIndex * 4);
            value = value >> 16;
        }
        else {
            value = buf.Load(BaseIndex * 4) & 0x0000FFFF;
        }
        
        return value;
    }
    
    //-----------------------------------------------------------------------------------------------------------
    //
    // @param buf : 32비트 인덱스 버퍼
    // @param idx : 인덱스 원소 번호
    //
    // @return 읽어 온 인덱스
    //
    //-----------------------------------------------------------------------------------------------------------
    uint GetIndex32(in ByteAddressBuffer buf, in uint idx) {
        return buf.Load(idx * 4);
    }
    
    //-----------------------------------------------------------------------------------------------------------
    //
    // 연속된 32비트 값 네 개를 읽어 float로 재해석한다.
    //
    // @param buf : 원시 데이터 버퍼
    // @param offset : 시작 바이트 오프셋
    //
    // @return 복원한 float4
    //
    //-----------------------------------------------------------------------------------------------------------
    float4 GetFloat4(in ByteAddressBuffer buf, in uint offset) {
        uint4 raw = buf.Load4(offset);
        return asfloat(raw);
    }
    
    //-----------------------------------------------------------------------------------------------------------
    //
    // @param direction : 큐브맵 샘플 방향
    //
    // @return 절댓값이 가장 큰 축과 부호로 정한 면 인덱스
    //
    //-----------------------------------------------------------------------------------------------------------
    uint GetCubeFaceIndex(in float3 direction) {
        const float3 AbsDir = abs(direction);
        if (AbsDir.x >= AbsDir.y && AbsDir.x >= AbsDir.z)
            return (direction.x > 0.f) ? 0 : 1; // +X : -X
        else if (AbsDir.y >= AbsDir.x && AbsDir.y >= AbsDir.z)
            return (direction.y > 0.f) ? 2 : 3; // +Y : -Y
        else
            return (direction.z > 0.f) ? 4 : 5; // +Z : -Z    
    }
    
    //-----------------------------------------------------------------------------------------------------------
    //
    // @param dir : 큐브맵 샘플 방향
    //
    // @return 해당 큐브맵 면의 [0, 1] UV
    //
    //-----------------------------------------------------------------------------------------------------------
    float2 ConvertDirectionToUV(in float3 dir) {
        const float3 AbsDir = abs(dir);
        float u = 0.f;
        float v = 0.f;

        if (AbsDir.x >= AbsDir.y && AbsDir.x >= AbsDir.z) {
        // X축이 지배적인 면
            if (dir.x > 0.f) {
            // +X 면 (-Z, -Y)
                u = -dir.z / AbsDir.x;
                v = -dir.y / AbsDir.x;
            }
            else {
            // -X 면 (Z, -Y)
                u =  dir.z / AbsDir.x;
                v = -dir.y / AbsDir.x;
            }
        }
        else if (AbsDir.y >= AbsDir.x && AbsDir.y >= AbsDir.z) {
        // Y축이 지배적인 면
            if (dir.y > 0.f) {
            // +Y 면 (X, Z)
                u = dir.x / AbsDir.y;
                v = dir.z / AbsDir.y;
            }
            else {
            // -Y 면 (X, -Z)
                u =  dir.x / AbsDir.y;
                v = -dir.z / AbsDir.y;
            }
        }
        else {
        // Z축이 지배적인 면
            if (dir.z > 0.f) {
            // +Z 면 (X, -Y)
                u =  dir.x / AbsDir.z;
                v = -dir.y / AbsDir.z;
            }
            else {
            // -Z 면 (-X, -Y)
                u = -dir.x / AbsDir.z;
                v = -dir.y / AbsDir.z;
            }
        }

        return float2(u, v) * 0.5f + 0.5f; // [-1, 1] 범위를 [0, 1]로 변환한다.
    }
    
    //-----------------------------------------------------------------------------------------------------------
    //
    // @param curr_pos : 현재 프레임 NDC 위치
    // @param prev_pos : 이전 프레임 NDC 위치
    //
    // @return 현재 UV에서 이전 UV를 뺀 이동량
    //
    //-----------------------------------------------------------------------------------------------------------
    float2 CalcVelocity(in float4 curr_pos, in float4 prev_pos) {
        curr_pos.xy = (curr_pos.xy + (float2) 1.f) * 0.5f;
        curr_pos.y = 1.f - curr_pos.y;

        prev_pos.xy = (prev_pos.xy + (float2) 1.f) * 0.5f;
        prev_pos.y = 1.f - prev_pos.y;

        return (curr_pos - prev_pos).xy;
    }
    
    //-----------------------------------------------------------------------------------------------------------
    //
    // @param z_ndc : NDC 깊이
    // @param proj : 투영 행렬
    //
    // @return 뷰 공간 깊이
    //
    //-----------------------------------------------------------------------------------------------------------
    float NdcDepthToViewDepth(in float z_ndc, in float4x4 proj) {
	    // z_ndc = A + B/viewZ이며 A = proj[2][2], B = proj[3][2]이다.
        const float viewZ = proj[3][2] / (z_ndc - proj[2][2]);
        
        return viewZ;
    }

    //-----------------------------------------------------------------------------------------------------------
    //
    // @param z_ndc : 정규화된 깊이 좌표
    // @param z_exp : 깊이 분포 지수
    // @param near : 근평면 거리
    // @param far : 원평면 거리
    //
    // @return 거듭제곱 분포를 적용한 뷰 공간 깊이
    //
    //-----------------------------------------------------------------------------------------------------------
    float NdcDepthToExpViewDepth(in float z_ndc, in float z_exp, in float near, in float far) {
        const float viewZ = pow(z_ndc, z_exp) * (far - near) + near;
        
        return viewZ;
    }
    
    //-----------------------------------------------------------------------------------------------------------
    //
    // @param depthV : 뷰 공간 깊이
    // @param z_exp : 깊이 분포 지수
    // @param near : 근평면 거리
    // @param far : 원평면 거리
    //
    // @return [0, 1] 범위의 역매핑 깊이 좌표
    //
    //-----------------------------------------------------------------------------------------------------------
    float ViewDepthToNdcDepth(in float depthV, in float z_exp, in float near, in float far) {
        return pow(saturate((depthV - near) / (far - near) ), 1.f / z_exp);
    }
    
    /***************************************************************/
    // 3D 값 노이즈용 해시
    // 참고 자료 https://www.shadertoy.com/view/XsXfRH
    // The MIT License
    // Copyright � 2017 Inigo Quilez
    // Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions: The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software. THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
    //-----------------------------------------------------------------------------------------------------------
    //
    // @param p : 해시 입력 3D 좌표
    //
    // @return [0, 1) 범위의 스칼라 해시
    //
    //-----------------------------------------------------------------------------------------------------------
    float Hash(in float3 p) {
        p = frac(p * 0.3183099f + 0.1f);
        p *= 17.f;
        return frac(p.x * p.y * p.z * (p.x + p.y + p.z));
    }
    
    
    //-----------------------------------------------------------------------------------------------------------
    //
    // @param DTid : 3D 디스패치 스레드 좌표
    // @param dims : 디스패치 대상 격자의 크기
    //
    // @return 격자 셀 중심의 NDC 좌표
    //
    //-----------------------------------------------------------------------------------------------------------
    float3 ThreadIdToNdc(in float3 DTid, in uint3 dims) {
    	float3 ndc = DTid;
    	ndc += 0.5f;
    	ndc *= float3(2.f / dims.x, -2.f / dims.y, 1.f / dims.z);
    	ndc += float3(-1.f, 1.f, 0.f);
    	return ndc;
    }
    
    //-----------------------------------------------------------------------------------------------------------
    //
    // @param ndc : NDC 좌표
    // @param depthV : 뷰 공간 깊이
    // @param invView : 역뷰 행렬
    // @param invProj : 역투영 행렬
    //
    // @return 재구성한 월드 공간 위치
    //
    //-----------------------------------------------------------------------------------------------------------
    float3 NdcToWorldPosition(in float3 ndc, in float depthV, in float4x4 invView, in float4x4 invProj) {
    	float4 rayV = mul(float4(ndc, 1.f), invProj);
    	rayV /= rayV.w;
    	rayV /= rayV.z; // 광선의 뷰 공간 Z를 1로 맞춘다.
    
    	const float4 PosW = mul(float4(rayV.xyz * depthV, 1.f), invView);
    	return PosW.xyz;
    }
    
    //-----------------------------------------------------------------------------------------------------------
    //
    // @param DTid : 3D 디스패치 스레드 좌표
    // @param dims : 디스패치 대상 격자의 크기
    // @param z_exp : 깊이 분포 지수
    // @param near : 근평면 거리
    // @param far : 원평면 거리
    // @param invView : 역뷰 행렬
    // @param invProj : 역투영 행렬
    //
    // @return 격자 셀 중심의 월드 공간 위치
    //
    //-----------------------------------------------------------------------------------------------------------
    float3 ThreadIdToWorldPosition(
            in float3 DTid, 
            in uint3 dims, 
            in float z_exp, 
            in float near, 
            in float far, 
            in float4x4 invView, 
            in float4x4 invProj) {
    	const float3 Ndc = ThreadIdToNdc(DTid, dims);
    	const float DepthVS = NdcDepthToExpViewDepth(Ndc.z, z_exp, near, far);
    	return NdcToWorldPosition(Ndc, DepthVS, invView, invProj);
    }
    
    //-----------------------------------------------------------------------------------------------------------
    //
    // @param index : 검사할 2D 인덱스
    // @param dimensions : 대상 영역의 크기
    //
    // @return 각 성분이 0 이상이고 해당 크기보다 작은지 여부
    //
    //-----------------------------------------------------------------------------------------------------------
    bool IsWithinBounds(in int2 index, in int2 dimensions) {
        return index.x >= 0 && index.y >= 0 && index.x < dimensions.x && index.y < dimensions.y;
    }
    
    //-----------------------------------------------------------------------------------------------------------
    //
    // @param val : 검사할 값
    // @param min : 하한
    // @param max : 상한
    //
    // @return 양 끝을 포함한 범위 안에 있는지 여부
    //
    //-----------------------------------------------------------------------------------------------------------
    bool IsInRange(in float val, in float min, in float max) {
        return (val >= min && val <= max);
    }
    
    //-----------------------------------------------------------------------------------------------------------
    //
    // @param m : 변환 행렬
    // @param p : 변환할 위치
    //
    // @return 이동 성분을 적용한 위치
    //
    //-----------------------------------------------------------------------------------------------------------
    float3 TransformPosition(float4x4 m, float3 p) {
        return mul(float4(p, 1.0f), m).xyz;
    }
    
    //-----------------------------------------------------------------------------------------------------------
    //
    // @param m : 변환 행렬
    // @param d : 변환할 방향
    //
    // @return 이동 성분을 제외하고 변환한 방향
    //
    //-----------------------------------------------------------------------------------------------------------
    float3 TransformDirection(float4x4 m, float3 d) {
        return mul(float4(d, 0.0f), m).xyz;
    }
    
    //-----------------------------------------------------------------------------------------------------------
    //
    // @param v : 정규화할 벡터
    //
    // @return 정규화된 벡터 또는 길이 제곱이 1e-8 이하일 때 +Z 방향
    //
    //-----------------------------------------------------------------------------------------------------------
    float3 SafeNormalize(float3 v) {
        float lenSq = dot(v, v);
        if (lenSq <= 1e-8f)
            return float3(0.f, 0.f, 1.f);
        return v * rsqrt(lenSq);
    }
}

#endif // __D3D12SHADERUITL_HLSLI__