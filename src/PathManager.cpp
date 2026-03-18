#include "pch.h"
#include "PathManager.hpp"

PathManager::PathManager() {}

PathManager::~PathManager() {}

bool PathManager::Initialize() {
	// ContentPath 경로 찾기
	// 실행파일이 있는 Bin 폴더 경로를 찾아낸다.
	// 디버깅 모드에서도 똑같이 동작하게 하기 위해서, 
	// 프로젝트 구성설정, 디버깅 탭에 작업 디렉터리를 실행파일 경로로 설정해준다.
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