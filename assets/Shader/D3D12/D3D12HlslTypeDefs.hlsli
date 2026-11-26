#ifndef __D3D12HLSLTYPEDEFS_HLSL__
#define __D3D12HLSLTYPEDEFS_HLSL__

typedef float FLOAT;
typedef uint UINT;
typedef int INT;
typedef int BOOL;

namespace DirectX {
    typedef float2 XMFLOAT2;
    typedef float3 XMFLOAT3;
    typedef float4 XMFLOAT4;
    typedef float4 XMVECTOR;
    typedef float4x4 XMFLOAT4X4;
    typedef uint2 XMUINT2;
    typedef uint3 XMUINT3;
    typedef uint4 XMUINT4;
    typedef int2 XMINT2;
    typedef int3 XMINT3;
    typedef int4 XMINT4;
    
    namespace SimpleMath {
        typedef float2 Vector2;
        typedef float3 Vector3;
        typedef float4 Vector4;
    }
}

typedef float2 Vec2;
typedef float3 Vec3;
typedef float4 Vec4;
typedef uint2 Uint2;
typedef uint3 Uint3;
typedef uint4 Uint4;
typedef float4x4 Mat4;

#endif // __D3D12HLSLTYPEDEFS_HLSL__