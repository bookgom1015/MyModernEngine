#pragma once

#define MAX_LAYER 32

#if defined(_D3D12)
	#define RENDERER_HEADER "Renderer/D3D12/D3D12Renderer.hpp"
#elif defined(_D3D11)
	#define RENDERER_HEADER "Renderer/D3D11/D3D11Renderer.hpp"
#elif defined(_VK)
	#define RENDERER_HEADER "Renderer/VK/VKRenderer.hpp"
#endif

using ScriptFactory = std::function<class CScript*()>;