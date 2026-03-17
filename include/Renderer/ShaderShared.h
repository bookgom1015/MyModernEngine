#ifndef __SHADERSHARED_H__
#define __SHADERSHARED_H__

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
	static const float InvalidDepthValue = 1.f;
	static const unsigned InvalidStencilValue = 0;

#ifdef _HLSL
	typedef float DepthBufferFormat;
#else
	const DXGI_FORMAT DepthStencilBufferFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	const DXGI_FORMAT DepthBufferFormat = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
#endif
}

#endif // __SHADERSHARED_H__