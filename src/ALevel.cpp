#include "pch.h"
#include "ALevel.hpp"

ALevel::ALevel() 
	: Asset(EAsset::E_Level)
	, mMatrix{}	
	, mbIsChanged{ true } {
	for (int i = 0; i < MAX_LAYER; ++i) 
		mLayers[i].mLayer = i;
}

ALevel::~ALevel() {}

bool ALevel::Begin() {
	for (size_t i = 0; i < MAX_LAYER; ++i) 
		CheckReturn(mLayers[i].Begin());

	return true;
}

bool ALevel::Update(float dt) {
	for (size_t i = 0; i < MAX_LAYER; ++i)
		CheckReturn(mLayers[i].Update(dt));

	return true;
}

bool ALevel::FixedUpdate(float dt) {
	for (size_t i = 0; i < MAX_LAYER; ++i)
		CheckReturn(mLayers[i].FixedUpdate(dt));

	return true;
}

bool ALevel::LateUpdate(float dt) {
	for (size_t i = 0; i < MAX_LAYER; ++i)
		CheckReturn(mLayers[i].LateUpdate(dt));

	return true;
};

bool ALevel::Final() {
	for (size_t i = 0; i < MAX_LAYER; ++i)
		CheckReturn(mLayers[i].Final());

	return true;
}

bool ALevel::OnLoaded() {
	for (size_t i = 0; i < MAX_LAYER; ++i)
		CheckReturn(mLayers[i].OnLoaded());

	return true;
}

bool ALevel::OnUnloaded() {
	for (size_t i = 0; i < MAX_LAYER; ++i)
		CheckReturn(mLayers[i].OnUnloaded());

	return true;
}

bool ALevel::AddGameObject(int layer, Ptr<GameObject> obj) {
	CheckReturn(mLayers[layer].AddObject(obj));

	return true;
}

bool ALevel::Deregister() {
	for (size_t i = 0; i < MAX_LAYER; ++i)
		mLayers[i].DeregisterAllObjects();

	return true;
}

void ALevel::CheckCollisionLayer(UINT layer1, UINT layer2) {
	UINT Row = layer1;
	UINT Col = layer2;

	// 더 작은 레이어 인덱스를 행 으로 사용한다.
	if (layer2 < layer1) {
		Row = layer2;
		Col = layer1;
	}

	mMatrix[Row] ^= (1 << Col);
}

void ALevel::CheckCollisionLayer(const std::wstring& layerName1, const std::wstring& layerName2) {

}

Ptr<GameObject> ALevel::FindObjectByName(const std::wstring& name) {
	for (size_t i = 0; i < MAX_LAYER; ++i) {
		const auto& parents = mLayers[i].GetParents();

		for (size_t i = 0, iend = parents.size(); i < iend; ++i) {
			std::list<Ptr<GameObject>> queue{};
			queue.push_back(parents[i]);

			while (!queue.empty()) {
				Ptr<GameObject> pObject = queue.front();
				queue.pop_front();

				if (pObject->GetName() == name)
					return pObject;

				const auto& vecChild = pObject->GetChildren();
				for (size_t j = 0, jend = vecChild.size(); j < jend; ++j)
					queue.push_back(vecChild[j]);
			}

		}
	}

	return nullptr;
}

void ALevel::Change() { mbIsChanged = true; }

ALevel* ALevel::Clone() { return NEW ALevel(*this); }

bool ALevel::Save(const std::wstring& filePath) {
	FILE* file{};
	GetFile(filePath, file);

	// 레벨 이름
	auto levelName = GetName();
	SaveWString(file, levelName);

	// 충돌 체크 정보
	fwrite(mMatrix.data(), sizeof(UINT), MAX_LAYER, file);

	// 레이어 정보
	for (UINT i = 0; i < MAX_LAYER; ++i) {
		// 레이어 이름 저장
		auto layerName = mLayers[i].GetName();
		SaveWString(file, layerName);

		// 레이어 내 최상위 개체를 계층 구조로 저장
		decltype(auto) parents = mLayers[i].GetParents();

		auto numParents = parents.size();
		fwrite(&numParents, sizeof(numParents), 1, file);

		for (const auto& object : parents)
			CheckReturn(object->SaveToLevelFile(file));
	}

	fclose(file);

	return true;
}

bool ALevel::Load(const std::wstring& filePath) {
	FILE* file{};
	_wfopen_s(&file, filePath.c_str(), L"rb");

	// 레벨 이름
	auto levelName = LoadWString(file);
	SetName(levelName);

	// 충돌 체크 정보
	fread(mMatrix.data(), sizeof(UINT), MAX_LAYER, file);

	// 레이어 정보
	for (UINT i = 0; i < MAX_LAYER; ++i) {
		// 레이어 이름 불러오기
		auto layerName = LoadWString(file);
		mLayers[i].SetName(layerName);

		size_t numParents{};
		fread(&numParents, sizeof(numParents), 1, file);

		for (size_t j = 0; j < numParents; ++j) {
			Ptr<GameObject> object = NEW GameObject;
			CheckReturn(object->LoadFromLevelFile(file));
			CheckReturn(AddGameObject(i, object));
		}
	}


	fclose(file);

	return true;
}