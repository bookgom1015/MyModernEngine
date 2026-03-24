#ifndef __D3D12SHADERSHARED_H__
#define __D3D12SHADERSHARED_H__

#ifdef _HLSL
	#ifndef HDR_FORMAT
	#define HDR_FORMAT float4
	#endif
	
	#ifndef SDR_FORMAT
	#define SDR_FORMAT float4
	#endif
	
	#ifndef AOMAP_FORMAT
	#define AOMAP_FORMAT FLOAT
	#endif
#else
	#ifndef HDR_FORMAT
	#define HDR_FORMAT DXGI_FORMAT_R16G16B16A16_FLOAT
	#endif
	
	#ifndef SDR_FORMAT
	#define SDR_FORMAT DXGI_FORMAT_R8G8B8A8_UNORM
	#endif
	
	#ifndef AOMAP_FORMAT
	#define AOMAP_FORMAT DXGI_FORMAT_R16_FLOAT
	#endif
#endif

namespace SwapChain {
#ifdef _HLSL
	typedef float4 BackBufferFormat;
#else
	const DXGI_FORMAT BackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
#endif
}

namespace DepthStencilBuffer {
	static const FLOAT InvalidDepthValue = 1.f;
	static const UINT InvalidStencilValue = 0;

#ifdef _HLSL
	typedef FLOAT DepthBufferFormat;
#else
	const DXGI_FORMAT DepthStencilBufferFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	const DXGI_FORMAT DepthBufferFormat = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
#endif
}

#ifndef MESH_SHADER_MAX_VERTICES
#define MESH_SHADER_MAX_VERTICES 192
#endif

#ifndef MESH_SHADER_MAX_PRIMITIVES
#define MESH_SHADER_MAX_PRIMITIVES 64
#endif

namespace GBuffer {
#ifndef GBuffer_Default_RCSTRUCT
#define GBuffer_Default_RCSTRUCT {	\
		DirectX::XMUINT2 gTexDim;	\
		UINT gVertexCount;			\
		UINT gIndexCount;			\
		FLOAT gDitheringMaxDist;	\
		FLOAT gDitheringMinDist;	\
	};
#endif

	namespace ThreadGroup {
		namespace MeshShader {
			enum {
				ThreadsPerGroup = MESH_SHADER_MAX_PRIMITIVES
			};
		}
	}

	namespace TextureSlot {
		enum {
			AlbedoMap = 0,
			NormalMap,
			AlphaMap,
			RoughnessMap,
			MetalnessMap,
			SpecularMap,
			Count
		};
	}

#ifdef _HLSL
	#ifndef GBuffer_Default_RootConstants
	#define GBuffer_Default_RootConstants(reg) cbuffer cbRootConstant \
		: register(reg) GBuffer_Default_RCSTRUCT
	#endif

	typedef float4	AlbedoMapFormat;
	typedef float4	NormalMapFormat;
	typedef uint	NormalDepthMapFormat;
	typedef float4	SpecularMapFormat;
	typedef float2	RoughnessMetalnessMapFormat;
	typedef float2	VelocityMapFormat;
	typedef float4	PositionMapFormat;

#else
	const DXGI_FORMAT AlbedoMapFormat				= DXGI_FORMAT_R8G8B8A8_UNORM;
	const DXGI_FORMAT NormalMapFormat				= DXGI_FORMAT_R16G16B16A16_FLOAT;
	const DXGI_FORMAT NormalDepthMapFormat			= DXGI_FORMAT_R32_UINT;
	const DXGI_FORMAT SpecularMapFormat				= DXGI_FORMAT_R8G8B8A8_UNORM;
	const DXGI_FORMAT RoughnessMetalnessMapFormat	= DXGI_FORMAT_R16G16_UNORM;
	const DXGI_FORMAT VelocityMapFormat				= DXGI_FORMAT_R16G16_FLOAT;
	const DXGI_FORMAT PositionMapFormat				= DXGI_FORMAT_R16G16B16A16_FLOAT;

	const FLOAT AlbedoMapClearValues[4]				= { 0.f,  0.f, 0.f,  0.f };
	const FLOAT NormalMapClearValues[4]				= { 0.f,  0.f, 0.f, -1.f };
	const FLOAT NormalDepthMapClearValues[4]		= { 0.f,  0.f, 0.f, 0.f };
	const FLOAT SpecularMapClearValues[4]			= { 0.08f, 0.08f, 0.08f, 0.f };
	const FLOAT RoughnessMetalnessMapClearValues[2]	= { 0.5f, 0.f };
	const FLOAT VelocityMapClearValues[2]			= { 1000.f, 1000.f };
	const FLOAT PositionMapClearValues[4]			= { 0.f, 0.f, 0.f, 0.f };

	namespace RootConstant {
		namespace Default {
			struct Struct GBuffer_Default_RCSTRUCT
				enum {
				E_TexDim_X = 0,
				E_TexDim_Y,
				E_VertexCount,
				E_IndexCount,
				E_DitheringMaxDist,
				E_DitheringMinDist,
				Count
			};
		}
	}
#endif
}

#endif // __D3D12SHADERSHARED_H__