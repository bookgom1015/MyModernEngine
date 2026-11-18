#include "Engine/pch.h"
#include "Engine/EditorManager.hpp"

#include "Engine/Engine.hpp"

#include RENDERER_HEADER
#include "Engine/ScriptManager.hpp"
#include "Engine/LevelManager.hpp"
#include "Engine/TimeManager.hpp"
#include "Engine/AssetManager.hpp"

#include "Engine/Asset.hpp"

#include "Engine/Menu.hpp"
#include "Engine/SceneUI.hpp"
#include "Engine/Outliner.hpp"
#include "Engine/Inspector.hpp"
#include "Engine/Content.hpp"
#include "Engine/Profiler.hpp"
#include "Engine/FrameViewer.hpp"
#include "Engine/LogUI.hpp"
#include "Engine/ListUI.hpp"
#include "Engine/Preference.hpp"

#include "Engine/CCamera.hpp"

#include "Engine/Script/CEditorCameraMoveScript.hpp"

EditorManager::EditorManager()
	: mbCleanedUp{}
	, mUIs{} {}

EditorManager::~EditorManager() {
	if (!mbCleanedUp) CleanUp();
}

bool EditorManager::Initialize() {    
	CheckReturn(InitializeImGui());

	CreateEditorUI();
	CreateEditorObjects();

	return true;
}

void EditorManager::CleanUp() {
#if defined(_D3D12)
	ImGui_ImplDX12_Shutdown();
#endif

	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
		mbCleanedUp = true;
}

bool EditorManager::Update() {
	if (LEVEL_MANAGER->GetCurrentLevelState() != ELevelState::E_Playing) {
		for (const auto& Object : mEditorObjects)
			Object->Update(E_DT);

		for (const auto& Object : mEditorObjects)
			Object->FinalEditor();
	}

	return true;
}

bool EditorManager::Draw() {
	BeginFrame();

	for (auto& [name, ui] : mUIs) {
		if (ui->IsActive()) 
			ui->Draw();
	}

	EndFrame();

	return true;
}

void EditorManager::AddUI(const std::string& name, Ptr<EditorUI> ui) {
	Ptr<EditorUI> pUI = FindUI(name);
	assert(pUI == nullptr);

	mUIs.insert(std::make_pair(name, ui));
}

Ptr<EditorUI> EditorManager::FindUI(const std::string& name) {
	auto it = mUIs.find(name);
	if (it != mUIs.end()) return it->second;

	return nullptr;
}

void EditorManager::AddLog(const LogEntry& entry) {
	mLogUI->AddLog(entry);
}

void EditorManager::AddInfoLog(const std::string& msg) {
	auto entry = LogEntry{
		.Level = LogLevel::E_Info,
		.Message = msg };
	mLogUI->AddLog(entry);
}

void EditorManager::AddWarningLog(const std::string& msg) {
	auto entry = LogEntry{
		.Level = LogLevel::E_Warning,
		.Message = msg };
	mLogUI->AddLog(entry);
}

void EditorManager::AddErrorLog(const std::string& msg) {
	auto entry = LogEntry{
		.Level = LogLevel::E_Error,
		.Message = msg };
	mLogUI->AddLog(entry);
}

void EditorManager::RegisterFocusedUI(Ptr<EditorUI> ui) { 
	mFocusedUI = ui; 
}

void EditorManager::AddDisplayTexture(const std::string& name, ImTextureID id) {
	mFrameViewer->AddDisplayTexture(name, id);
}

bool EditorManager::SetSystemSounds() {
	mWarningSound = LOAD(ASound, L"Sound\\Windows Background.wav");
	if (!mWarningSound) ReturnFalse("Failed to load warning sound.");

	return true;
}

float EditorManager::CalcItemSize(std::string_view text) {
	float spacing = ImGui::GetStyle().ItemSpacing.x;
	float padding = ImGui::GetStyle().FramePadding.x;
	return ImGui::CalcTextSize(text.data()).x + padding * 2.f + spacing;
}

void EditorManager::RightAlignNextItem(const std::initializer_list<std::string_view>& text) {
	float spacing = ImGui::GetStyle().ItemSpacing.x;
	float padding = ImGui::GetStyle().FramePadding.x;

	float totalWidth{};
	for (const auto& txt : text) {
		totalWidth += ImGui::CalcTextSize(txt.data()).x + padding * 2.0f;
		totalWidth += spacing;
	}

	totalWidth -= spacing;

	// 커서를 오른쪽으로 이동합니다.
	ImGui::SetCursorPosX(ImGui::GetContentRegionAvail().x - totalWidth + ImGui::GetCursorPosX());
}

void EditorManager::AcceptAssetDragDrop(
	std::string_view sender
	, EAsset::Type type
	, const std::function<void(Ptr<Asset>)>& func) {
	decltype(auto) PayLoad = ImGui::AcceptDragDropPayload(sender.data());
	if (PayLoad) {
		auto data = *(static_cast<DWORD_PTR*>(PayLoad->Data));
		Ptr<Asset> asset = reinterpret_cast<Asset*>(data);
		if (asset->GetType() == type) func(asset);
	}
}

bool EditorManager::InitializeImGui() {
	ImGui_ImplWin32_EnableDpiAwareness();

	float main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(
		::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigWindowsMoveFromTitleBarOnly = true;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
	io.ConfigDpiScaleFonts = true;
	io.ConfigDpiScaleViewports = true;

	io.Fonts->AddFontFromFileTTF(
		"C:\\Windows\\Fonts\\malgun.ttf",
		18.f,
		nullptr,
		io.Fonts->GetGlyphRangesKorean());

	SetDarkTheme();

	ImGuiStyle& style = ImGui::GetStyle();
	style.WindowMenuButtonPosition = ImGuiDir_None;
	style.ScaleAllSizes(main_scale);
	style.FontScaleDpi = main_scale;

	CheckReturn(ImGui_ImplWin32_Init(Engine::GetInstance()->GetMainWndHandle()));

#if defined(_D3D12)
	ImGui_ImplDX12_InitInfo initInfo{};
	RENDERER->BuildDX12InitInfo(initInfo);
	CheckReturn(ImGui_ImplDX12_Init(&initInfo));
#elif defined(_VK)

#endif

	return true;
}

void EditorManager::CreateEditorUI() {
	Ptr<EditorUI> pUI{};

	pUI = new Menu;
	AddUI(pUI->GetUIName(), pUI);

	pUI = new SceneUI;
	AddUI(pUI->GetUIName(), pUI);

	pUI = new Outliner;
	AddUI(pUI->GetUIName(), pUI);

	pUI = new Inspector;
	AddUI(pUI->GetUIName(), pUI);

	pUI = new Content;
	AddUI(pUI->GetUIName(), pUI);

	pUI = new Profiler;
	AddUI(pUI->GetUIName(), pUI);

	mFrameViewer = new FrameViewer;
	AddUI(mFrameViewer->GetUIName(), mFrameViewer.Get());

	mLogUI = new LogUI;
	AddUI(mLogUI->GetUIName(), mLogUI.Get());

	pUI = NEW ListUI;
	pUI->SetModal(true);
	pUI->SetActive(false);
	AddUI(pUI->GetUIName(), pUI);

	pUI = NEW Preference;
	pUI->SetActive(false);
	AddUI(pUI->GetUIName(), pUI);
}

void EditorManager::CreateEditorObjects() {
	// 에디터용 카메라 객체를 생성합니다.
	Ptr<GameObject> object = NEW GameObject;
	object->SetName(L"EditorCamera");

	object->AddComponent(NEW CTransform);
	object->AddComponent(NEW CCamera);
	object->AddComponent(GET_SCRIPT(CEditorCameraMoveScript));

	object->Camera()->LayerCheckAll();

	object->Camera()->SetProjectionType(EProjection::E_Perspective);
	object->Camera()->SetFar(10000.f);
	object->Camera()->SetFovY(PITwo);
	object->Camera()->SetOrthoScale(1.f);

	auto resolution = ENGINE->GetResolution();
	object->Camera()->SetAspectRatio(
		static_cast<FLOAT>(resolution.x) / static_cast<FLOAT>(resolution.y));
	object->Camera()->SetWidth(static_cast<FLOAT>(resolution.x));

	mEditorObjects.push_back(object);

	// 렌더 매니저에 에디터용 카메라로 등록합니다.
	mpEditorCamera = object->Camera().Get();
}

void EditorManager::BeginFrame() {
#if defined(_D3D12)
	ImGui_ImplDX12_NewFrame();
#endif

	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	ImGui::DockSpaceOverViewport(
		0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
}

void EditorManager::EndFrame() {
	ImGui::EndFrame();
	ImGui::Render();

#if defined(_D3D12)
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), RENDERER->GetCommandList());
#endif

	// 추가 플랫폼 창을 갱신하고 렌더링합니다.
	if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}
}

void EditorManager::SetDarkTheme() {
	ImGui::StyleColorsDark();

	ImGuiStyle& style = ImGui::GetStyle();
	ImVec4* colors = style.Colors;

	// =========================
	// 모서리 라운딩 및 형태
	// =========================
	style.WindowRounding = 6.0f;
	style.ChildRounding = 4.0f;
	style.FrameRounding = 5.0f;
	style.TabRounding = 5.0f;
	style.PopupRounding = 5.0f;
	style.ScrollbarRounding = 10.0f;
	style.GrabRounding = 5.0f;

	style.WindowBorderSize = 1.0f;
	style.ChildBorderSize = 1.0f;
	style.PopupBorderSize = 1.0f;
	style.FrameBorderSize = 0.0f;
	style.TabBorderSize = 0.0f;

	// 여백이 좁게 느껴지면 값을 8~12 정도로 조정할 수 있습니다.
	style.WindowPadding = ImVec2(10.0f, 10.0f);
	style.FramePadding = ImVec2(10.0f, 6.0f);
	style.CellPadding = ImVec2(8.0f, 6.0f);
	style.ItemSpacing = ImVec2(8.0f, 8.0f);
	style.ItemInnerSpacing = ImVec2(6.0f, 6.0f);
	style.IndentSpacing = 20.0f;
	style.ScrollbarSize = 14.0f;
	style.GrabMinSize = 10.0f;

	// 도킹 에디터에 사용하는 설정입니다.
	style.WindowMenuButtonPosition = ImGuiDir_None;

	// =========================
	// 색상 팔레트
	// =========================
	const ImVec4 bg0 = ImVec4(0.08f, 0.08f, 0.09f, 1.00f); // 창
	const ImVec4 bg1 = ImVec4(0.11f, 0.11f, 0.12f, 1.00f); // 패널과 탭
	const ImVec4 bg2 = ImVec4(0.15f, 0.15f, 0.17f, 1.00f); // 프레임과 버튼
	const ImVec4 bg3 = ImVec4(0.20f, 0.20f, 0.22f, 1.00f); // 마우스 오버
	const ImVec4 bg4 = ImVec4(0.26f, 0.26f, 0.29f, 1.00f); // 활성 상태

	const ImVec4 border = ImVec4(0.22f, 0.22f, 0.25f, 1.00f);
	const ImVec4 text = ImVec4(0.88f, 0.88f, 0.90f, 1.00f);
	const ImVec4 textDim = ImVec4(0.55f, 0.55f, 0.58f, 1.00f);

	// 파란색을 섞지 않은 중성 강조색입니다.
	const ImVec4 accent = ImVec4(0.48f, 0.48f, 0.52f, 1.00f);
	const ImVec4 accentHi = ImVec4(0.62f, 0.62f, 0.66f, 1.00f);

	// =========================
	// 텍스트와 배경
	// =========================
	colors[ImGuiCol_Text] = text;
	colors[ImGuiCol_TextDisabled] = textDim;

	colors[ImGuiCol_WindowBg] = bg0;
	colors[ImGuiCol_ChildBg] = bg0;
	colors[ImGuiCol_PopupBg] = bg1;

	colors[ImGuiCol_Border] = border;
	colors[ImGuiCol_BorderShadow] = ImVec4(0, 0, 0, 0);

	// =========================
	// 제목 표시줄, 메뉴, 스크롤
	// =========================
	colors[ImGuiCol_TitleBg] = bg1;
	colors[ImGuiCol_TitleBgActive] = bg1;
	colors[ImGuiCol_TitleBgCollapsed] = bg1;
	colors[ImGuiCol_MenuBarBg] = bg1;

	colors[ImGuiCol_ScrollbarBg] = bg1;
	colors[ImGuiCol_ScrollbarGrab] = bg2;
	colors[ImGuiCol_ScrollbarGrabHovered] = bg3;
	colors[ImGuiCol_ScrollbarGrabActive] = bg4;

	// =========================
	// 입력 위젯과 버튼
	// =========================
	colors[ImGuiCol_FrameBg] = bg2;
	colors[ImGuiCol_FrameBgHovered] = bg3;
	colors[ImGuiCol_FrameBgActive] = bg4;

	colors[ImGuiCol_Button] = bg2;
	colors[ImGuiCol_ButtonHovered] = bg3;
	colors[ImGuiCol_ButtonActive] = bg4;

	colors[ImGuiCol_Header] = bg2;
	colors[ImGuiCol_HeaderHovered] = bg3;
	colors[ImGuiCol_HeaderActive] = bg4;

	// =========================
	// 구버전과 신버전의 탭 색상을 모두 설정합니다.
	// =========================
	colors[ImGuiCol_Tab] = bg1;
	colors[ImGuiCol_TabHovered] = bg3;

	colors[ImGuiCol_TabSelected] = bg2;
	colors[ImGuiCol_TabDimmed] = bg1;
	colors[ImGuiCol_TabDimmedSelected] = bg2;
	colors[ImGuiCol_TabSelectedOverline] = bg4;
	colors[ImGuiCol_TabDimmedSelectedOverline] = bg3;

	// =========================
	// 구분선, 조절 핸들, 체크 표시
	// =========================
	colors[ImGuiCol_Separator] = border;
	colors[ImGuiCol_SeparatorHovered] = accent;
	colors[ImGuiCol_SeparatorActive] = accentHi;

	colors[ImGuiCol_ResizeGrip] = ImVec4(accent.x, accent.y, accent.z, 0.20f);
	colors[ImGuiCol_ResizeGripHovered] = ImVec4(accent.x, accent.y, accent.z, 0.45f);
	colors[ImGuiCol_ResizeGripActive] = ImVec4(accentHi.x, accentHi.y, accentHi.z, 0.70f);

	colors[ImGuiCol_CheckMark] = accentHi;
	colors[ImGuiCol_SliderGrab] = accent;
	colors[ImGuiCol_SliderGrabActive] = accentHi;

	// =========================
	// 도킹 및 기타 항목
	// =========================
	colors[ImGuiCol_DockingPreview] = ImVec4(accentHi.x, accentHi.y, accentHi.z, 0.25f);
	colors[ImGuiCol_DockingEmptyBg] = bg0;

	colors[ImGuiCol_TextSelectedBg] = ImVec4(accentHi.x, accentHi.y, accentHi.z, 0.25f);
	colors[ImGuiCol_DragDropTarget] = accentHi;
	colors[ImGuiCol_NavHighlight] = ImVec4(accentHi.x, accentHi.y, accentHi.z, 0.25f);
	colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.35f);

	// =========================
	// 테이블
	// =========================
	colors[ImGuiCol_TableHeaderBg] = bg1;
	colors[ImGuiCol_TableBorderStrong] = border;
	colors[ImGuiCol_TableBorderLight] = ImVec4(0.18f, 0.18f, 0.20f, 1.00f);
	colors[ImGuiCol_TableRowBg] = ImVec4(0, 0, 0, 0);
	colors[ImGuiCol_TableRowBgAlt] = ImVec4(1, 1, 1, 0.02f);
}