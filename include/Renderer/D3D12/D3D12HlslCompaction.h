#ifndef __D3D12HLSLCOMPACTION_H__
#define __D3D12HLSLCOMPACTION_H__

#ifdef _HLSL
	#include "./../../assets/Shader/D3D12/D3D12HlslTypeDefs.hlsli" 
	#include "./../../assets/Shader/ConstantValues.hlsli"
	#include "./../../include/Renderer/D3D12/D3D12ConstantBuffers.h"
	#include "./../../assets/Shader/ShaderStructures.hlsli"
	#include "./../../include/Vertex.h"
	#include "./../../include/Renderer/D3D12/D3D12MaterialData.h"
	#include "./../../include/Renderer/D3D12/D3D12ShaderShared.h"
	#include "./../../assets/Shader/D3D12/D3D12ShaderUtil.hlsli" 
	#include "./../../assets/Shader/D3D12/D3D12ValuePackaging.hlsli" 
	#include "./../../assets/Shader/D3D12/D3D12Samplers.hlsli"
	#include "./../../assets/Shader/HardCodedCoordinates.hlsli"
	#include "./../../assets/Shader/Random.hlsli"
	#include "./../../assets/Shader/FloatPrecision.hlsli"
#else
	#include <DirectXMath.h>
	#include <dxgiformat.h>

	#include "D3D12ShaderShared.h"
	#include "D3D12ConstantBuffers.h"
#endif

#endif // __D3D12HLSLCOMPACTION_H__