#ifndef __D3D12SHADERMANAGER_INL__
#define __D3D12SHADERMANAGER_INL__

namespace std {
	template<>
	struct hash<D3D12ShaderManager::D3D12ShaderInfo> {
		Hash operator()(const D3D12ShaderManager::D3D12ShaderInfo& info) const {
			Hash hash = 0;
			hash = HashCombine(hash, std::hash<LPCWSTR>()(info.FileName));
			hash = HashCombine(hash, std::hash<LPCWSTR>()(info.EntryPoint));
			hash = HashCombine(hash, std::hash<LPCWSTR>()(info.TargetProfile));
			for (UINT i = 0, end = static_cast<UINT>(info.DefineCount); i < end; ++i) {
				hash = HashCombine(hash, std::hash<LPCWSTR>()(info.Defines[i].Name));
				hash = HashCombine(hash, std::hash<LPCWSTR>()(info.Defines[i].Value));
			}
			hash = HashCombine(hash, static_cast<UINT>(info.DefineCount));
			return hash;
		}
	};
}

IDxcBlob* D3D12ShaderManager::GetShader(Hash hash) { return mShaders[hash].Get(); }

#endif // __D3D12SHADERMANAGER_INL__