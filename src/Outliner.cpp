#include "pch.h"
#include "Outliner.hpp"

#include "EditorManager.hpp"
#include "LevelManager.hpp"
#include "TaskManager.hpp"

#include "Inspector.hpp"

Outliner::Outliner() : EditorUI("Outliner") {
	mTree = NEW TreeUI;
	mTree->AddDynamicSelect(this, (DELEGATE_1)&Outliner::SelectGameObject);

	mTree->SetDropKey("Outliner"); // Self DragDrop 사용
	mTree->AddDynamicDragDrop(this, (DELEGATE_2)&Outliner::AddChild);

	AddChildUI(mTree.Get());
}

Outliner::~Outliner() {}

void Outliner::DrawUI() {
	RestoreSelection();
}

void Outliner::Renew() {
	// 트리에 표기된 오브젝트 정보를 전부 삭제
	mTree->Clear();

	// 현재 레벨을 가져옴
	Ptr<ALevel> currLevel = LEVEL_MANAGER->GetCurrentLevel();
	if (currLevel == nullptr) return;

	// 32개의 레이어를 순회
	for (UINT i = 0; i < MAX_LAYER; ++i) {
		// 각 레이어에 등록된 최상위 부모 오브젝트를 가져옴
		const auto& parents = currLevel->GetLayer(i)->GetParents();

		// 최상위 부모 오브젝트들을 트리에 추가한다.
		for (const auto& parent  : parents)
			AddGameObject(nullptr, parent);
	}
}

void Outliner::RestoreSelection() {
	Ptr<ALevel> currLevel = LEVEL_MANAGER->GetCurrentLevel();
	if (currLevel == nullptr || !currLevel->IsChanged()) return;

	auto name = mTree->GetSelectedNodeName();

	Renew();

	if (!name.empty()) {
		TaskInfo info{};
		info.Type = ETask::E_DeferredProcessing;
		info.Param_0 = DWORD_PTR_DEFERRED_TASK({
			auto node = mTree->FindNodeByName(name);
			if (node != nullptr) {
				mTree->RegisterSelected(node);
			}
			else {
				mTree->RegisterSelected(nullptr);

				auto ui = EDITOR_MANAGER->FindUI("Inspector");
				auto inspector = static_cast<Inspector*>(ui.Get());
				inspector->SetTargetObject(nullptr);
			}
		}, &, name);

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

	// 목적지가 없고, 
	if (destObj == nullptr) {
		// 자식타입 오브젝트인 경우
		if (srcObj->GetParent() != nullptr) {
			// 자식오브젝트를 최상위 부모 타입으로 뺀다
			srcObj->DisconnectWithParent();
			srcObj->RegisterAsParent();
		}
	}
	else {
		// SrcObj 가 DestObj 의 Ancetor 이면 안된다.
		destObj->AddChild(srcObj);
	}
}