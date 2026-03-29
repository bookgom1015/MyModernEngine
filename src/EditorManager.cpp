#include "pch.h"
#include "EditorManager.hpp"

#include "Engine.hpp"

#include RENDERER_HEADER
#include "ScriptManager.hpp"
#include "LevelManager.hpp"
#include "TimeManager.hpp"

#include "Asset.hpp"

#include "Menu.hpp"
#include "SceneUI.hpp"
#include "Outliner.hpp"
#include "Inspector.hpp"
#include "Content.hpp"
#include "Profiler.hpp"
#include "FrameViewer.hpp"
#include "LogUI.hpp"
#include "ListUI.hpp"

#include "CCamera.hpp"

#include "Script/CEditorCameraMoveScript.hpp"

EditorManager::EditorManager()
	: mUIs{} {}

EditorManager::~EditorManager() {
#if defined(_D3D12)
	ImGui_ImplDX12_Shutdown();
#endif

	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

bool EditorManager::Initialize() {    
	InitializeImGui();

	CreateEditorUI();
	CreateEditorObjects();

	return true;
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

	// 오른쪽으로 커서 이동
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

void EditorManager::InitializeImGui() {
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

	ImGui::StyleColorsDark();

	ImGuiStyle& style = ImGui::GetStyle();
	style.WindowMenuButtonPosition = ImGuiDir_None;
	style.WindowRounding = 0.f;
	style.ScaleAllSizes(main_scale);
	style.FontScaleDpi = main_scale;

	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		style.WindowRounding = 0.f;
		style.Colors[ImGuiCol_WindowBg].w = 1.f;
	}

	ImGui_ImplWin32_Init(Engine::GetInstance()->GetMainWndHandle());

#if defined(_D3D12)
	ImGui_ImplDX12_InitInfo initInfo{};
	RENDERER->BuildDX12InitInfo(initInfo);
	ImGui_ImplDX12_Init(&initInfo);
#endif
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
}

void EditorManager::CreateEditorObjects() {
	// Editor Camera Object 생성
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

	// Editor 용 카메라로서 RenderMgr 에 등록
	RENDERER->SetEditorCamera(object->Camera().Get());
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

	// Update and Render additional Platform Windows
	if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}
}