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
	typedef float4 HdrMapFormat;
#else
	const DXGI_FORMAT BackBufferFormat	= SDR_FORMAT;
	const DXGI_FORMAT HdrMapFormat		= HDR_FORMAT;
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
		UINT	gIndexCount;		\
		UINT	gStartIndex;		\
		UINT	gBaseVertex;		\
		FLOAT	gDitheringMaxDist;	\
		FLOAT	gDitheringMinDist;	\
		BOOL	gHasAlbedoMap;		\
		BOOL	gHasNormalMap;		\
	};
#endif

	namespace ThreadGroup {
		namespace MeshShader {
			enum {
				ThreadsPerGroup = MESH_SHADER_MAX_PRIMITIVES
			};
		}
	}

#ifdef _HLSL
	#ifndef GBuffer_Default_RootConstants
	#define GBuffer_Default_RootConstants(reg) cbuffer cbRootConstant \
		: register(reg) GBuffer_Default_RCSTRUCT
	#endif

	typedef float4	AlbedoMapFormat;
	typedef float4	NormalMapFormat;
	typedef uint	NormalDepthMapFormat;
	typedef float3	RMSMapFormat;
	typedef float2	VelocityMapFormat;
	typedef float4	PositionMapFormat;

	bool IsValidPosition(float4 position) {
		return position.w > 0.f;
	}

	bool IsValidVelocity(float2 velocity) {
		return all(velocity != float2(1000.f, 1000.f));
	}

#else
	const DXGI_FORMAT AlbedoMapFormat		= DXGI_FORMAT_R8G8B8A8_UNORM;
	const DXGI_FORMAT NormalMapFormat		= DXGI_FORMAT_R16G16B16A16_FLOAT;
	const DXGI_FORMAT NormalDepthMapFormat	= DXGI_FORMAT_R32_UINT;
	const DXGI_FORMAT RMSMapFormat			= DXGI_FORMAT_R11G11B10_FLOAT;
	const DXGI_FORMAT VelocityMapFormat		= DXGI_FORMAT_R16G16_FLOAT;
	const DXGI_FORMAT PositionMapFormat		= DXGI_FORMAT_R16G16B16A16_FLOAT;

	const FLOAT AlbedoMapClearValues[4]			= { 0.f,  0.f, 0.f,  0.f };
	const FLOAT NormalMapClearValues[4]			= { 0.f,  0.f, 0.f, -1.f };
	const FLOAT NormalDepthMapClearValues[4]	= { 0.f,  0.f, 0.f, 0.f };
	const FLOAT RMSMapClearValues[3]			= { 0.5f, 0.f, 1.f };
	const FLOAT VelocityMapClearValues[2]		= { 1000.f, 1000.f };
	const FLOAT PositionMapClearValues[4]		= { 0.f, 0.f, 0.f, -1.f };

	namespace RootConstant {
		namespace Default {
			struct Struct GBuffer_Default_RCSTRUCT;
			enum {
				E_TexDim_X = 0,
				E_TexDim_Y,
				E_IndexCount,
				E_StartInde,
				E_BaseVertex,
				E_DitheringMaxDist,
				E_DitheringMinDist,
				E_HasAlbedoMap,
				E_HasNormalMap,
				Count
			};
		}
	}
#endif
}

namespace BRDF {
#ifndef BRDF_ComputeBRDF_RCSTRUCT
#define BRDF_ComputeBRDF_RCSTRUCT {	\
		Vec2 gInvTexDim;			\
		BOOL gShadowEnabled;		\
	};
#endif

#ifndef BRDF_IntegrateIrradiance_RCSTRUCT
#define BRDF_IntegrateIrradiance_RCSTRUCT {	\
		BOOL gAoEnabled;					\
	};
#endif

#ifdef _HLSL
	#ifndef BRDF_ComputeBRDF_RootConstants
	#define BRDF_ComputeBRDF_RootConstants(reg) cbuffer cbRootConstant : register(reg) BRDF_ComputeBRDF_RCSTRUCT
	#endif
	
	#ifndef BRDF_IntegrateIrradiance_RootConstants
	#define BRDF_IntegrateIrradiance_RootConstants(reg) cbuffer cbRootConstant : register(reg) BRDF_IntegrateIrradiance_RCSTRUCT
	#endif
#else
	namespace RootConstant {
		namespace ComputeBRDF {
			struct Struct BRDF_ComputeBRDF_RCSTRUCT;
			enum {
				E_InvTexDim_X = 0,
				E_InvTexDim_Y,
				E_ShadowEnabled,
				Count
			};
		}

		namespace IntegrateIrradiance {
			struct Struct BRDF_IntegrateIrradiance_RCSTRUCT;
			enum {
				E_SsaoEnabled = 0,
				Count
			};
		}
	}
#endif

	namespace MipmapGenerator {
		static const UINT MaxMipLevel = 5;
	}
}

namespace ToneMapping {
#ifndef ToneMapping_Default_RCSTRUCT
#define ToneMapping_Default_RCSTRUCT {	\
		FLOAT gExposure;				\
		FLOAT gMiddleGrayKey;			\
		UINT gTonemapperType;			\
	};
#endif

	enum Type {
		E_None = 0,
		E_ACES,
		E_Exponential,
		E_Reinhard,
		E_ReinhardExt,
		E_Uncharted2,
		E_Log,
		Count
	};

#ifdef _HLSL

#ifndef ToneMapping_Default_RootConstants
	#define ToneMapping_Default_RootConstants(reg) cbuffer gRootConstants : register(reg) ToneMapping_Default_RCSTRUCT
#endif
#else
	namespace RootConstant {
		namespace Default {
			struct Struct ToneMapping_Default_RCSTRUCT;
			enum {
				E_Exposure = 0,
				E_MiddleGray,
				E_TonemapperType,
				Count
			};
		}
	}
#endif
}

namespace GammaCorrection {
#ifndef GammaCorrection_Default_RCSTRUCT
#define GammaCorrection_Default_RCSTRUCT {	\
		FLOAT gGamma;						\
	};
#endif

#ifdef _HLSL
	#ifndef GammaCorrection_Default_RootConstants
	#define GammaCorrection_Default_RootConstants(reg) cbuffer gRootConstants : register(reg) GammaCorrection_Default_RCSTRUCT
	#endif
#else
	namespace RootConstant {
		namespace Default {
			struct Struct GammaCorrection_Default_RCSTRUCT;
			enum {
				E_Gamma = 0,
				Count
			};
		}
	}
#endif
}

namespace Shadow {	
#ifndef Shadow_DrawDepth_RCSTRUCT
#define Shadow_DrawDepth_RCSTRUCT {	\
		UINT gLightIndex;			\
		UINT gBaseIndex;			\
		UINT gIndexStride;			\
	};
#endif

#ifdef _HLSL
	#ifndef Shadow_DrawDepth_RootConstants
	#define Shadow_DrawDepth_RootConstants(reg) cbuffer cbRootConstants : register(reg) Shadow_DrawDepth_RCSTRUCT
	#endif

	typedef float	DepthMapFormat;
#else 
	const DXGI_FORMAT DepthMapFormat = DXGI_FORMAT_D32_FLOAT;
#endif 
	namespace RootConstant {
		namespace DrawDepth {
			struct Struct Shadow_DrawDepth_RCSTRUCT;
			enum {
				E_LightIndex = 0,
				E_BaseIndex,
				E_IndexStride,
				Count
			};
		}
	}
}

namespace Taa {
#ifndef TAA_Default_RCSTRUCT
#define TAA_Default_RCSTRUCT {					\
		FLOAT			  gModulationFactor;	\
		DirectX::XMFLOAT2 gInvTexDim;			\
	};
#endif

#ifdef _HLSL
	#ifndef TAA_Default_RootConstants
	#define TAA_Default_RootConstants(reg) cbuffer cbRootConstants : register(reg) TAA_Default_RCSTRUCT
	#endif
#else
	namespace RootConstant {
		namespace Default {
			struct Struct TAA_Default_RCSTRUCT;
			enum {
				E_ModulationFactor = 0,
				E_InvTexDimX,
				E_InvTexDimY,
				Count
			};
		}
	}
#endif
}

namespace TextureScaler {
#ifndef TextureScaler_DownSample2Nx2N_RCSTRUCT
#define TextureScaler_DownSample2Nx2N_RCSTRUCT {	\
		DirectX::XMUINT2 gSrcTexDim;				\
		DirectX::XMUINT2 gDstTexDim;				\
	};
#endif

	namespace ThreadGroup {
		namespace DownSample2Nx2N {
			enum {
				Width = 8,
				Height = 8,
				Depth = 1,
				Size = Width * Height * Depth
			};
		}
	}

#ifdef _HLSL
	#ifndef TextureScaler_DownSample2Nx2N_RootConstants
	#define TextureScaler_DownSample2Nx2N_RootConstants(reg) cbuffer cbRootConstants : register(reg) TextureScaler_DownSample2Nx2N_RCSTRUCT
	#endif
#else		
#endif

	namespace RootConstant {
		namespace DownSample2Nx2N {
			struct Struct TextureScaler_DownSample2Nx2N_RCSTRUCT;
			enum {
				E_SrcTexDim_X = 0,
				E_SrcTexDim_Y,
				E_DstTexDim_X,
				E_DstTexDim_Y,
				Count
			};
		}
	}
}

namespace Bloom {
#ifndef Bloom_BlendBloomWithDownSampled_RCSTRUCT
#define Bloom_BlendBloomWithDownSampled_RCSTRUCT {	\
		DirectX::XMFLOAT2 gInvTexDim;				\
	};
#endif

#ifndef Bloom_ApplyBloom_RCSTRUCT
#define Bloom_ApplyBloom_RCSTRUCT {		\
		FLOAT gSharpness;				\
	};
#endif

	namespace ThreadGroup {
		namespace Default {
			enum {
				Width = 8,
				Height = 8,
				Depth = 1,
				Size = Width * Height * Depth
			};
		}
	}

#ifdef _HLSL
	#ifndef Bloom_BlendBloomWithDownSampled_RootConstants
	#define Bloom_BlendBloomWithDownSampled_RootConstants(reg) cbuffer cbRootConstants : register(reg) Bloom_BlendBloomWithDownSampled_RCSTRUCT
	#endif

	#ifndef Bloom_ApplyBloom_RootConstants
	#define Bloom_ApplyBloom_RootConstants(reg) cbuffer cbRootConstants : register(reg) Bloom_ApplyBloom_RCSTRUCT
	#endif

	typedef HDR_FORMAT HighlightMapFormat;
#else
	const DXGI_FORMAT HighlightMapFormat = HDR_FORMAT;
#endif

	namespace RootConstant {		
		namespace BlendBloomWithDownSampled {
			struct Struct Bloom_BlendBloomWithDownSampled_RCSTRUCT;
			enum {
				E_InvTexDimX = 0,
				E_InvTexDimY,
				Count
			};
		}

		namespace ApplyBloom {
			struct Struct Bloom_ApplyBloom_RCSTRUCT;
			enum {
				E_Sharpness = 0,
				Count
			};
		}
	}
}

namespace BlurFilter {
#ifndef BlurFilter_Default_RCSTRUCT
#define BlurFilter_Default_RCSTRUCT {	\
		DirectX::XMFLOAT2 gTexDim;		\
		DirectX::XMFLOAT2 gInvTexDim;	\
	};
#endif

	namespace ThreadGroup {
		namespace Default {
			enum {
				Width = 8,
				Height = 8,
				Depth = 1,
				Size = Width * Height * Depth
			};
		}
	}

#ifdef _HLSL
	#ifndef BlurFilter_Default_RootConstants
	#define BlurFilter_Default_RootConstants(reg) cbuffer cbRootConstants : register(reg) BlurFilter_Default_RCSTRUCT
	#endif
#else
	namespace RootConstant {
		namespace Default {
			struct Struct BlurFilter_Default_RCSTRUCT;
			enum {
				E_TexDimX = 0,
				E_TexDimY,
				E_InvTexDimX,
				E_InvTexDimY,
				Count
			};
		}
	}
#endif
}

namespace Vignette {
#ifndef Vignette_Default_RCSTRUCT
#define Vignette_Default_RCSTRUCT {	\
		float gStrength;			\
	};
#endif

#ifdef _HLSL
	#ifndef Vignette_Default_RootConstants
	#define Vignette_Default_RootConstants(reg) cbuffer cbRootConstants : register(reg) Vignette_Default_RCSTRUCT
	#endif
#else
	namespace RootConstant {
		namespace Default {
			struct Struct Vignette_Default_RCSTRUCT;
			enum {
				E_Strength = 0,
				Count
			};
		}
	}
#endif
}

namespace EnvironmentManager {
#ifndef EnvironmentManager_DrawSkySphere_RCSTRUCT
#define EnvironmentManager_DrawSkySphere_RCSTRUCT {	\
		UINT gIndexCount;							\
	};
#endif

#ifndef EnvironmentManager_CaptureEnvironment_RCSTRUCT
#define EnvironmentManager_CaptureEnvironment_RCSTRUCT {	\
		Vec2 gInvTexDim;									\
		BOOL gHasAlbedoMap;									\
		BOOL gHasNormalMap;									\
	};
#endif

#ifndef EnvironmentManager_ConvoluteSpecularIrradiance_RCSTRUCT
#define EnvironmentManager_ConvoluteSpecularIrradiance_RCSTRUCT {	\
		UINT	gMipLevel;											\
		FLOAT	gRoughness;											\
		FLOAT	gResolution;										\
	};
#endif

	namespace ThreadGroup {
		namespace MeshShader {
			enum {
				ThreadsPerGroup = MESH_SHADER_MAX_PRIMITIVES
			};
		}
	}

	static const UINT BrdfLutMapSize = 1024;
	static const UINT CubeMapSize = 1024;

#ifdef _HLSL
	#ifndef EnvironmentManager_DrawSkySphere_RootConstants
	#define EnvironmentManager_DrawSkySphere_RootConstants(reg) cbuffer cbRootConstants : register(reg) EnvironmentManager_DrawSkySphere_RCSTRUCT
	#endif

	#ifndef EnvironmentManager_CaptureEnvironment_RootConstants
	#define EnvironmentManager_CaptureEnvironment_RootConstants(reg) cbuffer cbRootConstants : register(reg) EnvironmentManager_CaptureEnvironment_RCSTRUCT
	#endif

	#ifndef EnvironmentManager_ConvoluteSpecularIrradiance_RootConstants
	#define EnvironmentManager_ConvoluteSpecularIrradiance_RootConstants(reg) cbuffer cbRootConstants : register(reg) EnvironmentManager_ConvoluteSpecularIrradiance_RCSTRUCT
	#endif

	typedef float2 BrdfLutMapFormat;
	typedef HDR_FORMAT EnvironmentCubeMapFormat;
	typedef HDR_FORMAT DiffuseIrradianceCubeMapFormat;
	typedef HDR_FORMAT SpecularIrradianceCubeMapFormat;
	typedef float DepthBufferArrayFormat;
#else	
	const DXGI_FORMAT BrdfLutMapFormat = DXGI_FORMAT_R16G16_FLOAT;
	const DXGI_FORMAT EnvironmentCubeMapFormat = HDR_FORMAT;
	const DXGI_FORMAT DiffuseIrradianceCubeMapFormat = HDR_FORMAT;
	const DXGI_FORMAT SpecularIrradianceCubeMapFormat = HDR_FORMAT;
	const DXGI_FORMAT DepthBufferArrayFormat = DXGI_FORMAT_D32_FLOAT;

	const FLOAT EnvironmentCubeMapClearValues[4] = { 0.f,  0.f, 0.f, 0.f };
#endif

	namespace RootConstant {
		namespace DrawSkySphere {
			struct Struct EnvironmentManager_DrawSkySphere_RCSTRUCT;
			enum {
				E_IndexCount = 0,
				Count
			};
		}

		namespace CaptureEnvironment {
			struct Struct EnvironmentManager_CaptureEnvironment_RCSTRUCT;
			enum {
				E_InvTexDimX = 0,
				E_InvTexDimY,
				E_HasAlbedoMap,
				E_HasNormalMap,
				Count
			};
		}

		namespace ConvoluteSpecularIrradiance {
			struct Struct EnvironmentManager_ConvoluteSpecularIrradiance_RCSTRUCT;
			enum {
				E_MipLevel = 0,
				E_Roughness,
				E_Resolution,
				Count
			};
		}
	}
}

#endif // __D3D12SHADERSHARED_H__