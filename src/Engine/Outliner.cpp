#include "Engine/pch.h"
#include "Engine/Outliner.hpp"

#include "Engine/EditorManager.hpp"
#include "Engine/LevelManager.hpp"
#include "Engine/TaskManager.hpp"

#include "Engine/Inspector.hpp"

Outliner::Outliner() : EditorUI("Outliner") {
	mTree = NEW TreeUI{[this](TreeNode* const pNode) { DrawObjectMenu(pNode); }};
	mTree->AddDynamicSelect(this, (DELEGATE_1)&Outliner::SelectGameObject);

	mTree->SetDropKey("Outliner"); // 자체 드래그 앤 드롭 처리를 사용합니다.
	mTree->AddDynamicDragDrop(this, (DELEGATE_2)&Outliner::AddChild);

	AddChildUI(mTree.Get());
}

Outliner::~Outliner() {}

void Outliner::DrawUI() {
	RestoreSelection();
}

void Outliner::DrawChildren() {
	const float height = ImGui::GetContentRegionAvail().y - 2.0f * ImGui::GetStyle().CellPadding.y;
	if (height <= 0.0f) return;

	constexpr ImGuiTableFlags flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV;
	if (ImGui::BeginTable("##OutlinerLayout", 1, flags)) {
		ImGui::TableSetupColumn("Game Objects", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableNextRow();

		if (ImGui::TableSetColumnIndex(0) && mTree->IsActive()) {
			if (ImGui::BeginChild("##GameObjects", ImVec2(0.0f, height))) {
				mTree->Draw();
				DrawBackgroundMenu();
			}

			ImGui::EndChild();
		}

		ImGui::EndTable();
	}

	DrawBackgroundMenu();
}

void Outliner::Renew() {
	// 트리에 표시된 객체 정보를 모두 지웁니다.
	mTree->Clear();

	// 현재 레벨을 가져옵니다.
	Ptr<ALevel> currLevel = LEVEL_MANAGER->GetCurrentLevel();
	if (currLevel == nullptr) return;

	// 32개 레이어를 순회합니다.
	for (UINT i = 0; i < MAX_LAYER; ++i) {
		// 레이어마다 최상위 부모 객체를 가져옵니다.
		const auto& parents = currLevel->GetLayer(i)->GetParents();

		// 최상위 부모 객체를 트리에 추가합니다.
		for (const auto& parent  : parents)
			AddGameObject(nullptr, parent);
	}
}

void Outliner::RestoreSelection() {
	Ptr<ALevel> currLevel = LEVEL_MANAGER->GetCurrentLevel();
	if (currLevel == nullptr || !currLevel->IsChanged()) return;

	const auto selectedNode = mTree->GetSelected();
	Ptr<GameObject> selectedObject;
	if (selectedNode != nullptr)
		selectedObject = reinterpret_cast<GameObject*>(selectedNode->Data);

	Renew();

	if (selectedObject != nullptr) {
		TaskInfo info{};
		info.Type = ETask::E_DeferredProcessing;
		info.Param_0 = DWORD_PTR_DEFERRED_TASK({
			auto node = mTree->FindNodeByData(reinterpret_cast<DWORD_PTR>(selectedObject.Get()));
			if (node != nullptr) {
				mTree->RegisterSelected(node);
			}
			else {
				mTree->RegisterSelected(nullptr);

				auto ui = EDITOR_MANAGER->FindUI("Inspector");
				auto inspector = static_cast<Inspector*>(ui.Get());
				inspector->SetTargetObject(nullptr);
			}
		}, &, selectedObject);

		TASK_MANAGER->AddTask(info);
	}
}

void Outliner::AddGameObject(Ptr<TreeNode> parent, Ptr<GameObject> obj) {
	std::string objName = WStrToStr(obj->GetName());
	if (objName.empty()) objName = WStrToStr(MakeUniqueName(L"Blank"));

	Ptr<TreeNode> newNode = mTree->AddItem(parent, objName.c_str(), reinterpret_cast<DWORD_PTR>(obj.Get()));

	for (size_t i = 0, end = obj->GetChildren().size(); i < end; ++i)
		AddGameObject(newNode, obj->GetChild(i));
}

void Outliner::SelectGameObject(DWORD_PTR obj) {
	Ptr<GameObject> selected = reinterpret_cast<GameObject*>(obj);

	Ptr<Inspector> inspector = reinterpret_cast<Inspector*>(EDITOR_MANAGER->FindUI("Inspector").Get());
	assert(inspector != nullptr);

	inspector->SetTargetObject(selected);
}

void Outliner::AddChild(DWORD_PTR src, DWORD_PTR dst) {
	Ptr<TreeNode> dragNode = reinterpret_cast<TreeNode*>(src);
	Ptr<TreeNode> dropNode = reinterpret_cast<TreeNode*>(dst);

	Ptr<GameObject> srcObj = reinterpret_cast<GameObject*>(dragNode->Data);
	Ptr<GameObject> destObj{};
	if (dropNode != nullptr) destObj = reinterpret_cast<GameObject*>(dropNode->Data);

	// 드롭 대상이 없고
	if (destObj == nullptr) {
		// 드래그한 객체가 자식 객체인 경우,
		if (srcObj->GetParent() != nullptr) {
			// 해당 객체를 최상위로 옮깁니다.
			srcObj->DisconnectWithParent();
			srcObj->RegisterAsParent();
		}
	}
	else {
		// SrcObj가 DestObj의 조상이면 안 됩니다.
		destObj->AddChild(srcObj);
	}
}

void Outliner::DrawBackgroundMenu() {
	if (ImGui::BeginPopupContextWindow(
		"OutlinerBackgroundMenu",
		ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems)) {
		if (ImGui::MenuItem("Create Object")) CreateObject(nullptr);
		if (ImGui::BeginMenu("Create Light")) {
			if (ImGui::MenuItem("Create Point Light")) {
				YOU_MUST_HAVE_CURR_LEVEL({
					auto obj = NEW GameObject;
					obj->AddComponent(NEW CTransform);

					auto light = NEW CLight;
					light->SetLightType(ELight::E_Point);
					light->SetLightColor(Vec3(1.f));
					light->SetRadius(1.f);
					light->SetAttenuationRadius(10.f);
					light->SetIntensity(1.f);
					obj->AddComponent(light);

					unsigned i = 1;
					std::wstring name{};
					while (true) {
						name = std::format(L"Point Light {}", i++);

						auto found = LEVEL_MANAGER->FindObjectByName(name);
						if (found == nullptr) break;
					}

					obj->SetName(name);

					CreateGameObject(obj, ELevelLayer::E_Light);
					});
			}
			if (ImGui::MenuItem("Create Spot Light")) {
				YOU_MUST_HAVE_CURR_LEVEL({
					auto obj = NEW GameObject;
					obj->AddComponent(NEW CTransform);

					auto light = NEW CLight;
					light->SetLightType(ELight::E_Spot);
					light->SetLightColor(Vec3(1.f));
					light->SetAttenuationRadius(10.f);
					light->SetIntensity(1.f);
					light->SetLightDirection(Vec3(0.f, -1.f, 0.f));
					light->SetOuterAngle(90.f);
					light->SetInnerAngle(30.f);
					obj->AddComponent(light);

					unsigned i = 1;
					std::wstring name{};
					while (true) {
						name = std::format(L"Spot Light {}", i++);

						auto found = LEVEL_MANAGER->FindObjectByName(name);
						if (found == nullptr) break;
					}

					obj->SetName(name);

					CreateGameObject(obj, ELevelLayer::E_Light);
					});
			}
			if (ImGui::MenuItem("Create Directional Light")) {
				YOU_MUST_HAVE_CURR_LEVEL({
					auto obj = NEW GameObject;
					obj->AddComponent(NEW CTransform);

					auto light = NEW CLight;
					light->SetLightType(ELight::E_Directional);
					light->SetLightColor(Vec3(1.f));
					light->SetIntensity(1.f);
					light->SetLightDirection(Vec3(0.f, -1.f, 0.f));
					light->SetAmbient(Vec3(0.05f));
					obj->AddComponent(light);

					unsigned i = 1;
					std::wstring name{};
					while (true) {
						name = std::format(L"Directional Light {}", i++);

						auto found = LEVEL_MANAGER->FindObjectByName(name);
						if (found == nullptr) break;
					}

					obj->SetName(name);

					CreateGameObject(obj, ELevelLayer::E_Light);
					});
			}

			ImGui::EndMenu();
		}

		ImGui::EndPopup();
	}
}

void Outliner::DrawObjectMenu(TreeNode* const pNode) {
	if (ImGui::BeginPopupContextItem(nullptr, ImGuiPopupFlags_MouseButtonRight)) {

		ImGui::TextUnformatted(pNode->DisplayName.c_str());

		ImGui::Dummy(ImVec2(0.f, 2.f));
		ImGui::Separator();
		ImGui::Dummy(ImVec2(0.f, 2.f));

		if (reinterpret_cast<GameObject*>(pNode->Data)) {
			if (ImGui::MenuItem("Create Object")) CreateObject(pNode);
			if (ImGui::MenuItem("Delete Object")) DeleteObject(pNode);
			if (ImGui::MenuItem("Rename")) RenameObject(pNode);
		}

		ImGui::EndPopup();
	}
}

void Outliner::CreateObject(TreeNode* const pNode) {
	YOU_MUST_HAVE_CURR_LEVEL({
		auto obj = NEW GameObject;

		obj->AddComponent(NEW CTransform);

		unsigned i = 1;
		std::wstring name{};
		while (true) {
			name = std::format(L"New GameObject {}", i++);

			auto found = LEVEL_MANAGER->FindObjectByName(name);
			if (found == nullptr) break;
		}

		obj->SetName(name);

		if (pNode == nullptr) 
			CreateGameObject(obj, ELevelLayer::E_Default);
		else 
			reinterpret_cast<GameObject*>(pNode->Data)->AddChild(obj);
	});
}

void Outliner::DeleteObject(TreeNode* const pNode) {
	if (auto gameObject = reinterpret_cast<GameObject*>(pNode->Data)) 
		DestroyGameObject(gameObject);
}

void Outliner::RenameObject(TreeNode* const pNode) {
	pNode->BeginRename([gameObject = Ptr<GameObject>{reinterpret_cast<GameObject*>(pNode->Data)}](
		const std::string& name) {
		gameObject->SetName(StrToWStr(name));
	});
}
