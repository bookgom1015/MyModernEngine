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
	static const FLOAT	InvalidDepthValue = 1.f;
	static const UINT	InvalidStencilValue = 0;

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

	static const UINT	InvalidNormalDepthValue = 0;

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

	bool IsValidNormalDepth(uint normalDepth) {
		return normalDepth != InvalidNormalDepthValue;
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
				E_AOEnabled = 0,
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
	static const UINT CubeMapSize = 256;

	static const UINT ProbeShape_Sphere = 0;
	static const UINT ProbeShape_Box = 1;

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

namespace Svgf {
#ifndef SVGF_TemporalSupersamplingReverseReproject_RCSTRUCT
#define SVGF_TemporalSupersamplingReverseReproject_RCSTRUCT {	\
		DirectX::XMFLOAT2 gTexDim;								\
		DirectX::XMFLOAT2 gInvTexDim;							\
	};
#endif

#ifndef SVGF_CalcDepthPartialDerivative_RCSTRUCT
#define SVGF_CalcDepthPartialDerivative_RCSTRUCT {	\
		DirectX::XMFLOAT2 gInvTexDim;				\
	};
#endif

#ifndef SVGF_AtrousWaveletTransformFilter_RCSTRUCT
#define SVGF_AtrousWaveletTransformFilter_RCSTRUCT {	\
		FLOAT gRayHitDistanceToKernelWidthScale;		\
		FLOAT gRayHitDistanceToKernelSizeScaleExponent;	\
	};
#endif

#ifndef SVGF_DisocclusionBlur_RCSTRUCT
#define SVGF_DisocclusionBlur_RCSTRUCT {	\
		DirectX::XMUINT2	gTextureDim;	\
		UINT				gStep;			\
		UINT				gMaxStep;		\
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

		namespace Atrous {
			enum {
				Width = 16,
				Height = 16,
				Depth = 1,
				Size = Width * Height * Depth
			};
		}
	}

#ifdef _HLSL
	typedef FLOAT		ValueMapFormat;
	typedef FLOAT		ValueSquaredMeanMapFormat;

	typedef uint4		TSPPSquaredMeanRayHitDistanceMapFormat;
	typedef float2		DepthPartialDerivativeMapFormat;
	typedef float2		LocalMeanVarianceMapFormat;
	typedef FLOAT		VarianceMapFormat;
	typedef FLOAT		RayHitDistanceMapFormat;
	typedef uint		TSPPMapFormat;
	typedef FLOAT		DisocclusionBlurStrengthMapFormat;

	static const FLOAT InvalidValue = -1.f;

#ifndef SVGF_TemporalSupersamplingReverseReproject_RootConstants
#define SVGF_TemporalSupersamplingReverseReproject_RootConstants(reg) cbuffer cbRootConstants : register(reg) SVGF_TemporalSupersamplingReverseReproject_RCSTRUCT
#endif

#ifndef SVGF_CalcDepthPartialDerivative_RootConstants
#define SVGF_CalcDepthPartialDerivative_RootConstants(reg) cbuffer cbRootConstants : register(reg) SVGF_CalcDepthPartialDerivative_RCSTRUCT
#endif

#ifndef SVGF_AtrousWaveletTransformFilter_RootConstants
#define SVGF_AtrousWaveletTransformFilter_RootConstants(reg) cbuffer cbRootConstants : register(reg) SVGF_AtrousWaveletTransformFilter_RCSTRUCT
#endif

#ifndef SVGF_DisocclusionBlur_RootConstants
#define SVGF_DisocclusionBlur_RootConstants(reg) cbuffer cbRootConstants : register(reg) SVGF_DisocclusionBlur_RCSTRUCT
#endif
#else
	const DXGI_FORMAT ValueMapFormat = DXGI_FORMAT_R16_FLOAT;
	const DXGI_FORMAT ValueSquaredMeanMapFormat = DXGI_FORMAT_R16_FLOAT;

	const DXGI_FORMAT TSPPSquaredMeanRayHitDistanceMapFormat = DXGI_FORMAT_R16G16B16A16_UINT;
	const DXGI_FORMAT DepthPartialDerivativeMapFormat = DXGI_FORMAT_R16G16_FLOAT;
	const DXGI_FORMAT LocalMeanVarianceMapFormat = DXGI_FORMAT_R32G32_FLOAT;
	const DXGI_FORMAT VarianceMapFormat = DXGI_FORMAT_R16_FLOAT;
	const DXGI_FORMAT RayHitDistanceMapFormat = DXGI_FORMAT_R16_FLOAT;
	const DXGI_FORMAT TSPPMapFormat = DXGI_FORMAT_R8_UINT;
	const DXGI_FORMAT DisocclusionBlurStrengthMapFormat = DXGI_FORMAT_R8_UNORM;
#endif

	namespace RootConstant {
		namespace TemporalSupersamplingReverseReproject {
			struct Struct SVGF_TemporalSupersamplingReverseReproject_RCSTRUCT;
			enum {
				E_TexDim_X = 0,
				E_TexDim_Y,
				E_InvTexDim_X,
				E_InvTexDim_Y,
				Count
			};
		}

		namespace CalcDepthPartialDerivative {
			struct Struct SVGF_CalcDepthPartialDerivative_RCSTRUCT;
			enum {
				E_InvTexDim_X = 0,
				E_InvTexDim_Y,
				Count
			};
		}

		namespace AtrousWaveletTransformFilter {
			struct Struct SVGF_AtrousWaveletTransformFilter_RCSTRUCT;
			enum {
				E_RayHitDistToKernelWidthScale = 0,
				E_RayHitDistToKernelSizeScaleExp,
				Count
			};
		}

		namespace DisocclusionBlur {
			struct Struct SVGF_DisocclusionBlur_RCSTRUCT;
			enum {
				E_TexDim_X = 0,
				E_TexDim_Y,
				E_Step,
				E_MaxStep,
				Count
			};
		}
	}
}

namespace VolumetricLight {
#ifndef VolumetricLight_CalculateScatteringAndDensity_RCSTRUCT
#define VolumetricLight_CalculateScatteringAndDensity_RCSTRUCT {\
		FLOAT gNearZ;											\
		FLOAT gFarZ;											\
		FLOAT gDepthExponent;									\
		FLOAT gUniformDensity;									\
		FLOAT gAnisotropicCoefficient;							\
		UINT  gFrameCount;										\
	};
#endif

#ifndef VolumetricLight_AccumulateScattering_RCSTRUCT 
#define VolumetricLight_AccumulateScattering_RCSTRUCT {	\
		FLOAT gNearZ;									\
		FLOAT gFarZ;									\
		FLOAT gDepthExponent;							\
		FLOAT gDensityScale;							\
	};
#endif

#ifndef VolumetricLight_ApplyFog_RCSTRUCT 
#define VolumetricLight_ApplyFog_RCSTRUCT {	\
		FLOAT gNearZ;						\
		FLOAT gFarZ;						\
		FLOAT gDepthExponent;				\
	};
#endif

	namespace ThreadGroup {
		namespace CalculateScatteringAndDensity {
			enum {
				Width = 8,
				Height = 8,
				Depth = 8,
				Size = Width * Height * Depth
			};
		}

		namespace AccumulateScattering {
			enum {
				Width = 8,
				Height = 8,
				Depth = 1,
				Size = Width * Height * Depth
			};
		}

		namespace BlendScattering {
			enum {
				Width = 8,
				Height = 8,
				Depth = 8,
				Size = Width * Height * Depth
			};
		}
	}

#ifdef _HLSL
#ifndef VolumetricLight_CalculateScatteringAndDensity_RootConstants
#define VolumetricLight_CalculateScatteringAndDensity_RootConstants(reg) cbuffer cbRootConstants : register (reg) VolumetricLight_CalculateScatteringAndDensity_RCSTRUCT
#endif

#ifndef VolumetricLight_AccumulateScattering_RootConstants
#define VolumetricLight_AccumulateScattering_RootConstants(reg) cbuffer cbRootConstants : register (reg) VolumetricLight_AccumulateScattering_RCSTRUCT
#endif

#ifndef VolumetricLight_ApplyFog_RootConstants
#define VolumetricLight_ApplyFog_RootConstants(reg) cbuffer cbRootConstants : register (reg) VolumetricLight_ApplyFog_RCSTRUCT
#endif

	typedef float4 FrustumVolumeMapFormat;
#else
	const DXGI_FORMAT FrustumVolumeMapFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
#endif

	namespace RootConstant {
		namespace CalculateScatteringAndDensity {
			struct Struct VolumetricLight_CalculateScatteringAndDensity_RCSTRUCT;
			enum {
				E_NearPlane = 0,
				E_FarPlane,
				E_DepthExponent,
				E_UniformDensity,
				E_AnisotropicCoefficient,
				E_FrameCount,
				Count
			};
		}

		namespace AccumulateScattering {
			struct Struct VolumetricLight_AccumulateScattering_RCSTRUCT;
			enum {
				E_NearPlane = 0,
				E_FarPlane,
				E_DepthExponent,
				E_DensityScale,
				Count
			};
		}

		namespace ApplyFog {
			struct Struct VolumetricLight_ApplyFog_RCSTRUCT;
			enum {
				E_NearPlane = 0,
				E_FarPlane,
				E_DepthExponent,
				Count
			};
		}
	}
}

namespace Rtao {
#ifdef _HLSL
	typedef AOMAP_FORMAT	AOCoefficientMapFormat;
	typedef uint			TSPPMapFormat;
	typedef FLOAT			AOCoefficientSquaredMeanMapFormat;
	typedef FLOAT			RayHitDistanceMapFormat;

	static const FLOAT RayHitDistanceOnMiss = 0.f;
	static const FLOAT InvalidAOCoefficientValue = Svgf::InvalidValue;

	bool HasAORayHitAnyGeometry(FLOAT tHit) {
		return tHit != RayHitDistanceOnMiss;
	}
#else
	const DXGI_FORMAT AOCoefficientMapFormat = AOMAP_FORMAT;
	const DXGI_FORMAT TSPPMapFormat = DXGI_FORMAT_R8_UINT;
	const DXGI_FORMAT AOCoefficientSquaredMeanMapFormat = DXGI_FORMAT_R16_FLOAT;
	const DXGI_FORMAT RayHitDistanceMapFormat = DXGI_FORMAT_R16_FLOAT;
#endif
}

namespace Ssao {
#ifndef SSAO_Default_RCSTRUCT
#define SSAO_Default_RCSTRUCT {			\
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
	typedef AOMAP_FORMAT	AOCoefficientMapFormat;
	typedef FLOAT			AOCoefficientSquaredMeanMapFormat;
	typedef float3			RandomVectorMapFormat;
	typedef float4			DebugMapFormat;

	static const float InvalidAOValue = -1.f;

	#ifndef SSAO_Default_RootConstants
	#define SSAO_Default_RootConstants(reg) cbuffer cbRootConstants : register(reg) SSAO_Default_RCSTRUCT
	#endif
#else
	const DXGI_FORMAT AOCoefficientMapFormat = AOMAP_FORMAT;
	const DXGI_FORMAT AOCoefficientSquaredMeanMapFormat = DXGI_FORMAT_R16_FLOAT;
	const DXGI_FORMAT RandomVectorMapFormat = DXGI_FORMAT_R8G8B8A8_SNORM;
	const DXGI_FORMAT DebugMapFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;

	const FLOAT AOMapClearValues[4] = { 1.f, 0.f, 0.f, 0.f };

	namespace RootConstant {
		namespace Default {
			struct Struct SSAO_Default_RCSTRUCT;
			enum {
				E_InvTexDimX = 0,
				E_InvTexDimY,
				Count
			};
		}
	}
#endif
}

namespace RaySorting {
	namespace ThreadGroup {
		enum {
			Width = 64,
			Height = 16,
			Depth = 1,
			Size = Width * Height * Depth
		};
	}

	namespace RayGroup {
		enum {
			NumElementPairsPerThread = 4,
			Width = ThreadGroup::Width,
			Height = NumElementPairsPerThread * 2 * ThreadGroup::Height,
			Depth = 1,
			Size = Width * Height * Depth
		};
	}

#ifdef _HLSL
	typedef uint2 RayIndexOffsetMapFormat;
#else
	static_assert(
		RayGroup::Width <= 64 
		&& RayGroup::Height <= 128 
		&& RayGroup::Size <= 8192
		, "Ray group dimensions are outside the supported limits set by the Counting Sort shader.");

	const DXGI_FORMAT RayIndexOffsetMapFormat = DXGI_FORMAT_R8G8_UINT;
#endif
}

namespace EyeAdaption {
#define MAX_BIN_COUNT 128

#ifndef EyeAdaption_LuminanceHistogram_RCSTRUCT
#define EyeAdaption_LuminanceHistogram_RCSTRUCT {	\
		DirectX::XMUINT2 gTexDim;					\
		FLOAT gMinLogLum;							\
		FLOAT gMaxLogLum;							\
		UINT gBinCount;								\
	};
#endif

#ifndef EyeAdaption_PercentileExtract_RCSTRUCT
#define EyeAdaption_PercentileExtract_RCSTRUCT {	\
		FLOAT gMinLogLum;							\
		FLOAT gMaxLogLum;							\
		FLOAT gLowPercent;							\
		FLOAT gHighPercent;							\
		UINT gBinCount;								\
	};
#endif

#ifndef EyeAdaption_TemporalSmoothing_RCSTRUCT
#define EyeAdaption_TemporalSmoothing_RCSTRUCT {	\
		FLOAT gUpSpeed;								\
		FLOAT gGlareUpSpeed;						\
		FLOAT gDownSpeed;							\
		FLOAT gDeltaTime;							\
	};
#endif

	struct HistogramBin {
		UINT Count;
	};

	struct Result {
		FLOAT AvgLogLum;
		FLOAT LowLogLum;
		FLOAT HighLogLum;
		UINT LowBin;
		UINT HighBin;
		UINT TotalCount;
	};

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
	#ifndef EyeAdaption_LuminanceHistogram_RootConstants
	#define EyeAdaption_LuminanceHistogram_RootConstants(reg) cbuffer cbRootConstants : register(reg) EyeAdaption_LuminanceHistogram_RCSTRUCT
	#endif
	
	#ifndef EyeAdaption_PercentileExtract_RootConstants
	#define EyeAdaption_PercentileExtract_RootConstants(reg) cbuffer cbRootConstants : register(reg) EyeAdaption_PercentileExtract_RCSTRUCT
	#endif
	
	#ifndef EyeAdaption_TemporalSmoothing_RootConstants
	#define EyeAdaption_TemporalSmoothing_RootConstants(reg) cbuffer cbRootConstants : register(reg) EyeAdaption_TemporalSmoothing_RCSTRUCT
	#endif
	#else		
#endif

	namespace RootConstant {
		namespace LuminanceHistogram {
			struct Struct EyeAdaption_LuminanceHistogram_RCSTRUCT;
			enum {
				E_TexDim_X = 0,
				E_TexDim_Y,
				E_MinLogLum,
				E_MaxLogLum,
				E_BinCount,
				Count
			};
		}

		namespace PercentileExtract {
			struct Struct EyeAdaption_PercentileExtract_RCSTRUCT;
			enum {
				E_MinLogLum,
				E_MaxLogLum,
				E_LowPercent,
				E_HighPercent,
				E_BinCount,
				Count
			};
		}

		namespace TemporalSmoothing {
			struct Struct EyeAdaption_TemporalSmoothing_RCSTRUCT;
			enum {
				E_UpSpeed,
				E_GlareUpSpeed,
				E_DownSpeed,
				E_DeltaTime,
				Count
			};
		}
	}
}

namespace MotionBlur {
#ifndef MotionBlur_Default_RCSTRUCT
#define MotionBlur_Default_RCSTRUCT {	\
		FLOAT gIntensity;				\
		FLOAT gLimit;					\
		FLOAT gDepthBias;				\
		UINT  gSampleCount;				\
	};
#endif

#ifdef _HLSL
	#ifndef MotionBlur_Default_RootConstants
	#define MotionBlur_Default_RootConstants(reg) cbuffer cbRootConstants : register(reg) MotionBlur_Default_RCSTRUCT
	#endif
#else
	namespace RootConstant {
		namespace Default {
			struct Struct MotionBlur_Default_RCSTRUCT;
			enum {
				E_Intensity = 0,
				E_Limit,
				E_DepthBias,
				E_SampleCount,
				Count
			};
		}
	}
#endif
}

#endif // __D3D12SHADERSHARED_H__