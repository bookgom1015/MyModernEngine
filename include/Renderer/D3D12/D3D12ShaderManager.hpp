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
	void CleanUp();

public:
	bool AddShader(const D3D12ShaderInfo& shaderInfo, Hash& hash);
	bool CompileShaders(LPCWSTR baseDir);

private:
	bool CompileShader(Hash hash, LPCWSTR baseDir);
	bool CommitShaders();
	bool BuildPdb(IDxcResult* const result, LPCWSTR fileName);

private:
	bool mbCleanedUp;

	unsigned mThreadCount;

	// 컴파일 슬롯별 DXC 유틸리티. 소스 Blob, include 핸들러 및 컴파일 인자를 생성합니다.
	std::vector<Microsoft::WRL::ComPtr<IDxcUtils>> mUtils;
	// 컴파일 슬롯별 DXC 컴파일러. mUtils와 같은 인덱스의 인스턴스를 함께 사용합니다.
	std::vector<Microsoft::WRL::ComPtr<IDxcCompiler3>> mCompilers;
	// 슬롯별 뮤텍스. 해당 슬롯의 DXC 인스턴스와 스테이징 버퍼에 대한 동시 접근을 막습니다.
	std::vector<std::unique_ptr<std::mutex>> mCompileMutexes;

	// AddShader로 등록한 셰이더의 컴파일 정보(파일, 진입점, 프로파일, 매크로)를 해시별로 보관합니다.
	std::unordered_map<Hash, D3D12ShaderInfo> mShaderInfos;
	// CommitShaders로 반영한 컴파일 결과. GetShader에서 해시로 조회합니다.
	std::unordered_map<Hash, Microsoft::WRL::ComPtr<IDxcBlob>> mShaders;
	// 슬롯별 임시 컴파일 결과. 모든 작업 완료 후 CommitShaders에서 mShaders로 반영하고 비웁니다.
	std::vector<std::vector<std::pair<Hash, Microsoft::WRL::ComPtr<IDxcBlob>>>> mStagingShaders;
};

#include "D3D12ShaderManager.inl"