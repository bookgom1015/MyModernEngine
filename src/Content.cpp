#include "pch.h"
#include "Content.hpp"

#include "AssetManager.hpp"

Content::Content() : EditorUI("Content") {
	mTree = NEW TreeUI;
	mTree->AddDynamicSelect(this, (DELEGATE_1)&Content::SelectAsset);

	AddChildUI(mTree.Get());

	// Asset 내용을 트리에 반영
	Renew();
}

Content::~Content() {}

void Content::DrawUI() {
	if (ASSET_MANAGER->IsChanged()) 
		Renew();
}

void Content::Renew() {
	// 트리 비우기
	mTree->Clear();

	// 에셋 종류별로 Tree 에 추가하기
	for (UINT i = 0; i < EAsset::Count; ++i) {
		// 에셋의 이름에 해당하는 노드를 추가 (enum 타입을 문자열로 바꿔서 추가)
		Ptr<TreeNode> pNode = mTree->AddItem(nullptr, AssetTypeToString((EAsset::Type)i));
		pNode->SetFramed(true);

		// 해당 에셋 모든 이름을 받아와서 하위 자식으로 추가
		std::vector<std::wstring> vecNames;
		ASSET_MANAGER->GetAssetNames((EAsset::Type)i, vecNames);

		for (const auto& str : vecNames) {
			Ptr<Asset> pAsset = ASSET_MANAGER->FindAsset((EAsset::Type)i, str);
			mTree->AddItem(pNode, WStrToStr(str), (DWORD_PTR)pAsset.Get());
		}
	}
}

void Content::SelectAsset(DWORD_PTR asset) {
	if (asset == 0) return;

	// 클릭한 노드가 들고있는 Asset 주소값을 입력받는다.
	Ptr<Asset> pAsset = (Asset*)asset;

	//// Inspector에 ContentUI 에서 클릭된 Asset 의 주소를 알려준다.
	//Ptr<Inspector> pInspector = (Inspector*)EditorMgr::GetInst()->FindUI("Inspector").Get();
	//assert(pInspector.Get());
	//
	//pInspector->SetTargetAsset(pAsset);
}