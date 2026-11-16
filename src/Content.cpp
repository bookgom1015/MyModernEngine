#include "pch.h"
#include "Content.hpp"

#include "AssetManager.hpp"

Content::Content() : EditorUI("Content") {
	mTree = NEW TreeUI;
	mTree->AddDynamicSelect(this, (DELEGATE_1)&Content::SelectAsset);

	AddChildUI(mTree.Get());

	// 에셋 정보를 트리에 반영합니다.
	Renew();
}

Content::~Content() {}

void Content::DrawUI() {
	if (ASSET_MANAGER->IsChanged()) 
		Renew();
}

void Content::Renew() {
	// 트리를 비웁니다.
	mTree->Clear();

	// 에셋을 종류별로 트리에 추가합니다.
	for (UINT i = 0; i < EAsset::Count; ++i) {
		// 열거형 값을 문자열로 변환하여 에셋 종류에 해당하는 노드를 추가합니다.
		Ptr<TreeNode> pNode = mTree->AddItem(nullptr, AssetTypeToString((EAsset::Type)i));
		pNode->SetFramed(true);

		// 해당 종류의 모든 에셋 이름을 가져와 하위 노드로 추가합니다.
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

	// 클릭한 노드에 저장된 에셋 주소를 받습니다.
	Ptr<Asset> pAsset = (Asset*)asset;

	//// ContentUI에서 클릭한 에셋의 주소를 Inspector에 전달합니다.
	//Ptr<Inspector> pInspector = (Inspector*)EditorMgr::GetInst()->FindUI("Inspector").Get();
	//assert(pInspector.Get());
	//
	//pInspector->SetTargetAsset(pAsset);
}