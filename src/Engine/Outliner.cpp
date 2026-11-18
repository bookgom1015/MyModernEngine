#include "Engine/pch.h"
#include "Engine/Outliner.hpp"

#include "Engine/EditorManager.hpp"
#include "Engine/LevelManager.hpp"
#include "Engine/TaskManager.hpp"

#include "Engine/Inspector.hpp"

Outliner::Outliner() : EditorUI("Outliner") {
	mTree = NEW TreeUI;
	mTree->AddDynamicSelect(this, (DELEGATE_1)&Outliner::SelectGameObject);

	mTree->SetDropKey("Outliner"); // 자체 드래그 앤 드롭 처리를 사용합니다.
	mTree->AddDynamicDragDrop(this, (DELEGATE_2)&Outliner::AddChild);

	AddChildUI(mTree.Get());
}

Outliner::~Outliner() {}

void Outliner::DrawUI() {
	RestoreSelection();
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