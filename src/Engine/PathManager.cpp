#include "Engine/pch.h"
#include "Engine/PathManager.hpp"

PathManager::PathManager() {}

PathManager::~PathManager() {}

bool PathManager::Initialize() {
	// ContentPath를 찾습니다.
	// 실행 파일이 있는 Bin 폴더의 경로를 찾습니다.
	// 디버깅 모드에서도 동일하게 동작하도록
	// 프로젝트 속성의 디버깅 탭에서 작업 디렉터리를 실행 파일 경로로 설정합니다.
	GetCurrentDirectory(255, mContentPath);
	//SetWindowText(Engine::GetInst()->GetMainWndHwnd(), m_ContentPath);

	//auto path = std::wstring(mContentPath);

	auto length = static_cast<int>(wcslen(mContentPath));

	bool trig{};
	for (int i = length - 1; 0 <= i; --i) {
		if (mContentPath[i] == '\\') {
			if (!trig) {
				trig = true;
				continue;
			}

			mContentPath[i] = '\0';
			break;
		}
	}

	wcscat_s(mContentPath, L"\\assets\\");

	return true;
}