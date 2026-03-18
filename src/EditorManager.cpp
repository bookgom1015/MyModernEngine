#include "pch.h"
#include "EditorManager.hpp"

#include "Engine.hpp"

#include "Asset.hpp"

#include "Menu.hpp"
#include "SceneUI.hpp"
#include "Outliner.hpp"
#include "Inspector.hpp"
#include "Content.hpp"
#include "LogUI.hpp"

#if defined(_D3D12)
	#include "Renderer/D3D12/D3D12Renderer.hpp"
#endif

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

	return true;
}

void EditorManager::Draw() {
	BeginFrame();

	for (auto& [name, ui] : mUIs) {
		if (ui->IsActive()) 
			ui->Draw();
	}

	EndFrame();
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

	mLogUI = new LogUI;
	AddUI(mLogUI->GetUIName(), mLogUI.Get());
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