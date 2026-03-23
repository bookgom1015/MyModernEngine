#pragma once

#include "D3D12RenderPass.hpp"

class D3D12RenderPassManager : public Singleton<D3D12RenderPassManager>{
	SINGLETON(D3D12RenderPassManager);

public:
	bool CompileShaders(class D3D12ShaderManager* pShaderManager);
	bool BuildRootSignatures();
	bool BuildPipelineStates();
	bool AllocateDescriptors();
	bool BuildShaderTables(unsigned numRitems);

	bool OnResize(unsigned width, unsigned height);
	bool Update();

public:
	template <typename T>
		requires std::is_base_of_v<D3D12RenderPass, T>
	__forceinline void Add();

	template <typename T>
		requires std::is_base_of_v<D3D12RenderPass, T>
	__forceinline T* Get();

private:
	std::unordered_map<std::type_index, std::unique_ptr<D3D12RenderPass>> mRenderPasses;
};

#include "D3D12RenderPassManager.inl"

#ifndef RENDER_PASS_MANAGER
#define RENDER_PASS_MANAGER D3D12RenderPassManager::GetInstance()
#endif // RENDER_PASS_MANAGER

#define REGISTER_RENDER_PASS(__Type)                        \
    namespace {                                             \
        struct __Type##Register {                           \
            __Type##Register() {                            \
                RENDER_PASS_MANAGER->Add<__Type>();			\
            }                                               \
        };                                                  \
        static __Type##Register global_##__Type##Register;	\
    }