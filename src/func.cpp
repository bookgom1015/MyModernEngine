#include "pch.h"

#include <mutex>
#include <vector>
#include <iostream>

#include "TaskManager.hpp"

#include "Asset.hpp"
#include "ALevel.hpp"

#include "Components.hpp"

namespace {
    const char* const STR_FAIL_GET_PROC_INFO = "Failed to retrieve processor information.";
    const char* const STR_FAIL_GET_BUFF_SIZE = "Failed to get required buffer size.";
    const char* const STR_FAIL_GET_MEM_STAT = "Failed to get memory status";

    wchar_t LevelNameBuffer[255]{};

    bool GetProcessorName(Processor* const pInfo) {
        INT CPUInfo[4]{ -1 };
        CHAR CPUBrandString[0x40]{ 0 };

        // Processor's name
        __cpuid(CPUInfo, 0x80000002);
        memcpy(CPUBrandString, CPUInfo, sizeof(CPUInfo));

        __cpuid(CPUInfo, 0x80000003);
        memcpy(CPUBrandString + 16, CPUInfo, sizeof(CPUInfo));

        __cpuid(CPUInfo, 0x80000004);
        memcpy(CPUBrandString + 32, CPUInfo, sizeof(CPUInfo));

        pInfo->Name = StrToWStr(CPUBrandString);

        return true;
    }

    bool GetInstructionSupport(Processor* const pInfo) {
        INT CPUInfo[4]{ 0 };

        // Call default CPUID (EAX=1)
        __cpuid(CPUInfo, 1);

        // Check if SSE, SSE2 are supported in EDX register
        pInfo->SupportMMX = (CPUInfo[3] & (1 << 23)) != 0; // EDX 23th bit (MMX)
        pInfo->SupportSSE = (CPUInfo[3] & (1 << 25)) != 0; // EDX 25th bit (SSE)
        pInfo->SupportSSE2 = (CPUInfo[3] & (1 << 26)) != 0; // EDX 26th bit (SSE2)

        // Check if SSE3, AVX, etc. are supported in ECX register
        pInfo->SupportSSE3 = (CPUInfo[2] & (1 << 0)) != 0; // ECX 0th  bit (SSE3)
        pInfo->SupportSSSE3 = (CPUInfo[2] & (1 << 9)) != 0; // ECX 9th  bit (SSSE3)
        pInfo->SupportSSE4_1 = (CPUInfo[2] & (1 << 19)) != 0; // ECX 19th bit (SSE4.1)
        pInfo->SupportSSE4_2 = (CPUInfo[2] & (1 << 20)) != 0; // ECX 20th bit (SSE4.2)
        pInfo->SupportAVX = (CPUInfo[2] & (1 << 28)) != 0; // ECX 28th bit (AVX)

        // Call CPUID (EAX=7, ECX=0)
        __cpuidex(CPUInfo, 7, 0);

        pInfo->SupportAVX2 = (CPUInfo[1] & (1 << 5)) != 0; // EBX 5th  bit (AVX2)
        pInfo->SupportAVX512F = (CPUInfo[1] & (1 << 16)) != 0; // EBX 16th bit (AVX512F)
        pInfo->SupportAVX512DQ = (CPUInfo[1] & (1 << 17)) != 0; // EBX 17th bit (AVX512DQ)
        pInfo->SupportAVX512BW = (CPUInfo[1] & (1 << 30)) != 0; // EBX 30th bit (AVX512BW)

        return true;
    }

    bool GetCoreInfo(Processor* const pInfo) {
        DWORD length = 0;
        GetLogicalProcessorInformationEx(RelationProcessorCore, nullptr, &length);

        if (length == 0)
            ReturnFalse(STR_FAIL_GET_BUFF_SIZE);

        std::vector<uint8_t> buffer(length);
        if (!GetLogicalProcessorInformationEx(
            RelationProcessorCore
            , reinterpret_cast<SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*>(buffer.data())
            , &length))
            ReturnFalse(STR_FAIL_GET_PROC_INFO);

        pInfo->Physical = 0;
        pInfo->Logical = 0;

        for (size_t offset = 0; offset < length;) {
            auto* const infoEX = reinterpret_cast<SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*>(buffer.data() + offset);

            if (infoEX->Relationship == RelationProcessorCore) {
                ++pInfo->Physical;
                pInfo->Logical += __popcnt64(infoEX->Processor.GroupMask->Mask);
            }

            offset += infoEX->Size;
        }

        return true;
    }

    bool GetSystemMemoryInfo(Processor* const pInfo) {
        MEMORYSTATUSEX memInfo{};
        memInfo.dwLength = sizeof(MEMORYSTATUSEX);

        if (!GlobalMemoryStatusEx(&memInfo))
            ReturnFalse(STR_FAIL_GET_MEM_STAT);

        UINT64 denom = 1024 * 1024;
        pInfo->TotalPhysicalMemory = memInfo.ullTotalPhys / denom;
        pInfo->AvailablePhysicalMemory = memInfo.ullAvailPhys / denom;
        pInfo->TotalVirtualMemory = memInfo.ullTotalVirtual / denom;
        pInfo->AvailableVirtualMemory = memInfo.ullAvailVirtual / denom;

        return true;
    }
}

LogFile::LogFile() : Handle{ INVALID_HANDLE_VALUE }, Mutex{} {}

LogFile::~LogFile() {
    if (Handle != INVALID_HANDLE_VALUE && Handle != NULL) {
        CloseHandle(Handle);
        Handle = INVALID_HANDLE_VALUE;
    }
}

LogFile Logger::mLogFile{};

BOOL Logger::Initialize(LPCWSTR filePath) {
	mLogFile.Handle = CreateFile(
		filePath,
		GENERIC_WRITE,
		FILE_SHARE_WRITE,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	DWORD writtenBytes = 0;
	WORD bom = 0xFEFF;

	return WriteFile(mLogFile.Handle, &bom, 2, &writtenBytes, NULL);
}

void Logger::LogFn(const std::string& msg) {
	std::wstring wstr = StrToWStr(msg);

	DWORD writtenBytes = 0;
	{
		std::lock_guard<std::mutex> lock(mLogFile.Mutex);

		WriteFile(
			mLogFile.Handle,
			wstr.c_str(),
			static_cast<DWORD>(wstr.length() * sizeof(WCHAR)),
			&writtenBytes,
			NULL
		);
	}
#ifdef _DEBUG
    std::cout << msg;
#endif
}

void Logger::LogFn(const std::wstring& msg) {
	DWORD writtenBytes = 0;
	{
		std::lock_guard<std::mutex> lock(mLogFile.Mutex);

		WriteFile(
            mLogFile.Handle,
			msg.c_str(),
			static_cast<DWORD>(msg.length() * sizeof(WCHAR)),
			&writtenBytes,
			NULL
		);
	}
#ifdef _DEBUG
    std::cout << WStrToStr(msg);
#endif
}

BOOL Logger::SetTextToWnd(HWND hWnd, LPCWSTR text) {
	return SetWindowTextW(hWnd, text);
}

BOOL Logger::AppendTextToWnd(HWND hWnd, LPCWSTR text) {
	const INT length = GetWindowTextLengthW(hWnd) + lstrlenW(text) + 1;

	std::vector<WCHAR> buffer(length);

	GetWindowTextW(hWnd, buffer.data(), length);
	wcscat_s(buffer.data(), length, text);
	SetWindowTextW(hWnd, buffer.data());

	return TRUE;
}

bool GetProcessorInfo(Processor* const pInfo) {
    CheckReturn(GetProcessorName(pInfo));
    CheckReturn(GetInstructionSupport(pInfo));
    CheckReturn(GetCoreInfo(pInfo));
    CheckReturn(GetSystemMemoryInfo(pInfo));

    return true;
}

void SaveWString(FILE* pFile, const std::wstring& string) {
    int Len = static_cast<int>(string.length());
    fwrite(&Len, sizeof(int), 1, pFile);
    fwrite(string.data(), sizeof(wchar_t), Len, pFile);
}

std::wstring LoadWString(FILE* pFile) {
    int Len{};
    fread(&Len, sizeof(int), 1, pFile);

    wchar_t buff[255]{};
    fread(buff, sizeof(wchar_t), Len, pFile);

    return buff;
}

void SaveAssetRef(FILE* pFile, Asset* pAsset) {
    // Asset 이 Null 인지 아닌지 저장
    bool IsNull = pAsset;
    fwrite(&IsNull, sizeof(bool), 1, pFile);

    // Asset 의 Key, RelativePath 저장
    if (nullptr != pAsset) {
        SaveWString(pFile, pAsset->GetKey());
        SaveWString(pFile, pAsset->GetRelativePath());
    }
}

std::string EAsset::AssetTypeToString(EAsset::Type type) {
    switch (type) {
    case EAsset::E_Mesh: return "Mesh";
    case EAsset::E_Material: return "Material";
    case EAsset::E_Texture: return "Texture";
    case EAsset::E_Sound: return "Sound";
    case EAsset::E_GraphicShader: return "GraphicShader";
    case EAsset::E_ComputeShader: return "ComputeShader";
    case EAsset::E_Level: return "Level";
    case EAsset::E_Sprite: return "Sprite";
    case EAsset::E_Flipbook: return "Flipbook";
    case EAsset::E_TileMap: return "TileMap";
    case EAsset::E_Prefab: return "Prefab";
    default: return "Unknown";
    }
}

namespace EComponent {
    std::string ComponentTypeToString(Type type) {
        switch (type) {
        case E_Transform: return "Transform";
        case E_Camera: return "Camera";
        case E_Collider2D: return "Collider2D";
        case E_Collider3D: return "Collider3D";
        case E_Light2D: return "Light2D";
        case E_Light3D: return "Light3D";
        case E_MeshRender: return "MeshRender";
        case E_BillboardRender: return "BillboardRender";
        case E_SpriteRender: return "SpriteRender";
        case E_FlipbookRender: return "FlipbookRender";
        case E_ParticleRender: return "ParticleRender";
        case E_TileRender: return "TileRender";
        case E_Rigidbody: return "Rigidbody";
        default: assert(false && "Unknown component type");
			return "Unknown";
        }
    }

    Type ComponentStringToType(std::string name) {
        switch (HashString(name)) {
        case HashString("Transform"): return E_Transform;
		case HashString("Camera"): return E_Camera;		
        case HashString("Collider2D"): return E_Collider2D;
		case HashString("Collider3D"): return E_Collider3D;
		case HashString("Light2D"): return E_Light2D;
		case HashString("Light3D"): return E_Light3D;
		case HashString("MeshRender"): return E_MeshRender;
		case HashString("BillboardRender"): return E_BillboardRender;
		case HashString("SpriteRender"): return E_SpriteRender;
		case HashString("FlipbookRender"): return E_FlipbookRender;
		case HashString("ParticleRender"): return E_ParticleRender;
		case HashString("TileRender"): return E_TileRender;
		case HashString("Rigidbody"): return E_Rigidbody;
		default: assert(false && "Unknown component type string");
            return static_cast<Type>(-1);
        }
    }

    Component* GetComponent(Type type) {
        switch (type) {
        case E_Transform: return NEW CTransform;
            //case EComponent::E_Camera: return new Camera();
            //case EComponent::E_Collider2D: return new Collider2D();
            //case EComponent::E_Collider3D: return new Collider3D();
            //case EComponent::E_Light2D: return new Light2D();
            //case EComponent::E_Light3D: return new Light3D();
            case EComponent::E_MeshRender: return new CMeshRender();
            //case EComponent::E_BillboardRender: return new CBillboardRender();
            //case EComponent::E_SpriteRender: return new CSpriteRender();
            //case EComponent::E_FlipbookRender: return new CFlipbookRender();
            //case EComponent::E_ParticleRender: return new CParticleRender();
            //case EComponent::E_TileRender: return new CTileRender();
            //case EComponent::E_Rigidbody: return new Rigidbody();
        default: return nullptr;
        }
    }
}

void CreateGameObject(GameObject* obj, int layer) {
    TaskInfo info{};

    info.Type = ETask::E_CreateObject;
    info.Param_0 = reinterpret_cast<DWORD_PTR>(obj);
    info.Param_1 = layer;

    TASK_MANAGER->AddTask(info);
}

void ChangeLevel(const std::wstring& name) {
    TaskInfo info{};
    
    wcscpy_s(LevelNameBuffer, 255, name.c_str());

    info.Type = ETask::E_ChangeLevel;
    info.Param_0 = reinterpret_cast<DWORD_PTR>(LevelNameBuffer);

    TASK_MANAGER->AddTask(info);
}

void ChangeNewLevel(ALevel* pNewLevel) {
    TaskInfo info{};

    info.Type = ETask::E_ChangeNewLevel;
    info.Param_0 = reinterpret_cast<DWORD_PTR>(pNewLevel);

    TASK_MANAGER->AddTask(info);
}

void ChangeLevelState(ELevelState::Type nextState) {
    TaskInfo info{};

    info.Type = ETask::E_ChangeLevelState;
    info.Param_0 = static_cast<DWORD_PTR>(nextState);

    TASK_MANAGER->AddTask(info);
}

decltype(auto) GetTimeStamp() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

std::wstring MakeUniqueName(const std::wstring& name) {
    return std::format(L"{}##{}", name, GetTimeStamp());
}

bool GetFile(const std::wstring& filePath, FILE*& pFile) {
    std::filesystem::path path(filePath);
    std::filesystem::path dir = path.parent_path();

    if (!dir.empty() && !std::filesystem::exists(dir))
        if (!std::filesystem::create_directories(dir))
			ReturnFalse(std::format("Failed to create directory: {}", dir.string()));
    
    if (_wfopen_s(&pFile, filePath.c_str(), L"wb") != 0)
        ReturnFalse("Failed to open file for writing");

    return true;
}