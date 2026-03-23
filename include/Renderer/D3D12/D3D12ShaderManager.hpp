#pragma once

class D3D12ShaderManager {
public:
	struct D3D12ShaderInfo {
		LPCWSTR		FileName;
		LPCWSTR		EntryPoint;
		LPCWSTR		TargetProfile;
		DxcDefine*	Defines;
		UINT32		DefineCount;

		D3D12ShaderInfo();
		D3D12ShaderInfo(LPCWSTR fileName, LPCWSTR entryPoint, LPCWSTR profile);
		D3D12ShaderInfo(LPCWSTR fileName, LPCWSTR entryPoint, LPCWSTR profile, DxcDefine* defines, UINT32 defCount);
		D3D12ShaderInfo(const D3D12ShaderInfo& ref);
		~D3D12ShaderInfo();

		D3D12ShaderInfo& operator=(const D3D12ShaderInfo& ref);
	};

public:
	D3D12ShaderManager();
	virtual ~D3D12ShaderManager();

public:
	__forceinline IDxcBlob* GetShader(Hash hash);

public:
	bool Initialize();

public:
	bool AddShader(const D3D12ShaderInfo& shaderInfo, Hash& hash);
	bool CompileShaders(LPCWSTR baseDir);

private:
	bool CompileShader(Hash hash, LPCWSTR baseDir);
	bool CommitShaders();
	bool BuildPdb(IDxcResult* const result, LPCWSTR fileName);

private:
	bool mbCleanedUp{};

	Microsoft::WRL::ComPtr<IDxcUtils> mUtil;
	Microsoft::WRL::ComPtr<IDxcCompiler3> mCompiler;

	std::unordered_map<Hash, D3D12ShaderInfo> mShaderInfos;
	std::unordered_map<Hash, Microsoft::WRL::ComPtr<IDxcBlob>> mShaders;
	std::vector<std::pair<Hash, Microsoft::WRL::ComPtr<IDxcBlob>>> mStagingShaders;
};

#include "D3D12ShaderManager.inl"