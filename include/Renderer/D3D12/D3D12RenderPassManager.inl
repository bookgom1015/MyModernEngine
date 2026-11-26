#ifndef __D3D12RENDERPASSMANAGER_INL__
#define __D3D12RENDERPASSMANAGER_INL__

template <typename T>
	requires std::is_base_of_v<D3D12RenderPass, T>
void D3D12RenderPassManager::Add() {
	const auto iter = mRenderPasses.find(typeid(T));
	if (iter != mRenderPasses.end()) return;

	mRenderPasses[typeid(T)] = std::make_unique<T>();
}

template <typename T>
	requires std::is_base_of_v<D3D12RenderPass, T>
T* D3D12RenderPassManager::Get() {
	const auto iter = mRenderPasses.find(typeid(T));
	assert(iter != mRenderPasses.end() && "Render pass not found. Make sure to add it first.");

	return dynamic_cast<T*>(iter->second.get());
}

#endif // __D3D12RENDERPASSMANAGER_INL__