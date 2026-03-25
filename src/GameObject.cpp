#include "pch.h"
#include "GameObject.hpp"

#include "ScriptManager.hpp"
#include "LevelManager.hpp"

GameObject::GameObject() 
	: mpParent{}
	, mChildren{}
	, mComponents{}	
	, mRenderComponent{nullptr}
	, mScripts{}
	, mLayer{ -1 }
	, mbIsDead{} {}

GameObject::GameObject(const GameObject& other) 
	: Entity{ other }
	, mpParent{}
	, mComponents{}
	, mLayer{}
	, mbIsDead{} {
	for (UINT i = 0; i < EComponent::Count; ++i) {
		if (other.mComponents[i] == nullptr) continue;
		AddComponent(other.mComponents[i]->Clone());
	}
	for (const auto& script : other.mScripts) 
		AddComponent(script->Clone());
	for (const auto& child : other.mChildren) 
		AddChild(child->Clone());
}

GameObject::~GameObject() {}

bool GameObject::Begin() {
	for (size_t i = 0, end = mScripts.size(); i < end; ++i) 
		CheckReturn(mScripts[i]->Begin());

	for (size_t i = 0; i < EComponent::Count; ++i) 
		if (mComponents[i] != nullptr) CheckReturn(mComponents[i]->Begin());

	for (size_t i = 0, end = mChildren.size(); i < end; ++i)
		CheckReturn(mChildren[i]->Begin());

	return true;
}

bool GameObject::Update(float dt) {
	for (size_t i = 0, end = mScripts.size(); i < end; ++i) 
		CheckReturn(mScripts[i]->Update(dt));

	for (size_t i = 0; i < EComponent::Count; ++i)
		if (mComponents[i] != nullptr) CheckReturn(mComponents[i]->Update(dt));

	for (size_t i = 0, end = mChildren.size(); i < end; ++i)
		CheckReturn(mChildren[i]->Update(dt));

	return true;
}

bool GameObject::FixedUpdate(float dt) {
	for (size_t i = 0, end = mScripts.size(); i < end; ++i)
		CheckReturn(mScripts[i]->FixedUpdate(dt));

	for (size_t i = 0; i < EComponent::Count; ++i)
		if (mComponents[i] != nullptr) CheckReturn(mComponents[i]->FixedUpdate(dt));

	for (size_t i = 0, end = mChildren.size(); i < end; ++i)
		CheckReturn(mChildren[i]->FixedUpdate(dt));

	return true;
}

bool GameObject::LateUpdate(float dt) {
	for (size_t i = 0, end = mScripts.size(); i < end; ++i)
		CheckReturn(mScripts[i]->LateUpdate(dt));

	for (size_t i = 0; i < EComponent::Count; ++i)
		if (mComponents[i] != nullptr) CheckReturn(mComponents[i]->LateUpdate(dt));

	for (size_t i = 0, end = mChildren.size(); i < end; ++i)
		CheckReturn(mChildren[i]->LateUpdate(dt));

	return true;
}

bool GameObject::Final() {
	for (size_t i = 0; i < EComponent::Count; ++i)
		if (mComponents[i] != nullptr) CheckReturn(mComponents[i]->Final());

	// 자신이 소속된 Layer 에 자기자신을 알림(등록)
	CheckReturn(RegisterLayer());

	auto iter = mChildren.begin();
	for (; iter != mChildren.end();) {
		CheckReturn((*iter)->Final());

		if ((*iter)->IsDead())
			// 자식이 죽었다면 제거
			iter = mChildren.erase(iter);
		else 
			++iter;
	}

	return true;
}

bool GameObject::FinalEditor() {
	for (size_t i = 0; i < EComponent::Count; ++i)
		if (mComponents[i] != nullptr) CheckReturn(mComponents[i]->Final());

	auto iter = mChildren.begin();
	for (; iter != mChildren.end();) {
		CheckReturn((*iter)->Final());

		if ((*iter)->IsDead())
			// 자식이 죽었다면 제거
			iter = mChildren.erase(iter);
		else
			++iter;
	}

	return true;
}

bool GameObject::Render() {


	return true;
}

bool GameObject::AddComponent(Ptr<Component> comp) {
	// 렌더링 기능 컴포넌트는 하나만 가질 수 있음
	if (dynamic_cast<CRenderComponent*>(comp.Get())) {
		assert(mRenderComponent == nullptr);
	
		mRenderComponent = static_cast<CRenderComponent*>(comp.Get());
	}
	
	// 입력으로 들어온 컴포넌트가 스크립트면, vector 로 관리
	if (comp->GetType() == EComponent::E_Script) {
		mScripts.push_back(static_cast<CScript*>(comp.Get()));
	}
	// 입력으로 들어온 컴포넌트가 스크립트가 아니면, 알맞은 배열 포인터로 가리킴
	else {
		// 해당 컴포넌트를 이미 가지고 있지 않아야 한다.
		assert(mComponents[(UINT)comp->GetType()] == nullptr);

		mComponents[comp->GetType()] = comp;
	}
	
	comp->mpOwner = this;
	comp->Initialize();

	return true;
}

bool GameObject::RemoveComponent(EComponent::Type type) {
	if (type == EComponent::E_Script) {
		//const auto begin = mScripts.begin();
		//const auto end = mScripts.end();
		//
		//const auto iter = std::find_if(begin, end, [&, _ScriptType](Ptr<CScript>& s) {
		//	return s->GetScriptType() == _ScriptType;
		//});
		//if (iter == end) ReturnFalse("Script of the specified type does not exist");)
		//
		//iter->Get()->CleanUp();
		//
		//std::iter_swap(iter, end - 1);
		//mScripts.pop_back();
	}
	else {
		auto& comp = mComponents[type];
		if (comp == nullptr) ReturnFalse("Component of the specified type does not exist");

		comp = nullptr;
	}

	return true;
}

bool GameObject::AddChild(Ptr<GameObject> child) {
	// 부모 오브젝트가 있는지 확인
	if (child->GetParent() != nullptr) {
		// 기존 부모 오브젝트와 관계를 해제한다.
		CheckReturn(child->DisconnectWithParent());
	}
	// 최상위 부모 오브젝트 였다면
	else {
		// 레벨 내부에 있던 오브젝트 라면
		if (child->mLayer != -1) 
			// Layer 에서 최상위 부모로 가리키던 포인터를 제거
			child->DeregisterAsParent();
	}

	mChildren.push_back(child);
	child->mpParent = this;

	if (child->mLayer == -1) {	
		child->mLayer = mLayer;

		//if (mLayer != -1 && )
	}

	//if (mLayer != -1)

	return true;
}

bool GameObject::DisconnectWithParent() {
	return true;
}

bool GameObject::RegisterAsParent() {
	return true;
}

bool GameObject::DeregisterAsParent() {
	return true;
}

void GameObject::Destroy() {
	// Mark this object as dead. Full destruction/cleanup handled elsewhere.
	mbIsDead = true;
}

bool GameObject::SaveToLevelFile(FILE* const pFile) {
	auto objectName = GetName();
	SaveWString(pFile, objectName);

	// 컴포넌트 
	for (UINT i = 0; i < EComponent::Count; ++i) {
		decltype(auto) comp = mComponents[i];
		if (comp == nullptr) continue;

		// 컴포넌트 타입
		fwrite(&i, sizeof(i), 1, pFile);

		// 컴포넌트 내용
		CheckReturn(comp->SaveToLevelFile(pFile));
	}

	UINT end = EComponent::Count;
	fwrite(&end, sizeof(end), 1, pFile);

	// 스크립트
	auto size = mScripts.size();
	fwrite(&size, sizeof(size), 1, pFile);

	for (const auto& script : mScripts) {
		auto scriptID = script->GetID();
		fwrite(&scriptID, sizeof(scriptID), 1, pFile);
	
		CheckReturn(script->SaveToLevelFile(pFile));
	}

	// 자식 객체
	size_t numChildren = mChildren.size();
	fwrite(&numChildren, sizeof(numChildren), 1, pFile);

	for (const auto& child : mChildren)
		CheckReturn(child->SaveToLevelFile(pFile));

	return true;
}

bool GameObject::LoadFromLevelFile(FILE* const pFile) {
	auto objectName = LoadWString(pFile);
	SetName(objectName);

	// 컴포너트
	while (true) {
		UINT comType{};
		fread(&comType, sizeof(comType), 1, pFile);
		if (comType == EComponent::Count) break;

		Component* component{};
		switch (comType) {
		case EComponent::E_Transform:
			component = NEW CTransform;
			break;
		//case EComponent::E_Camera:
		//	component = NEW CCamera;
		//	break;
		//case EComponent::E_Collider2D:
		//	component = NEW CCollider2D;
		//	break;
		//case EComponent::E_Collider3D:
		//	break;
		//case EComponent::E_Light2D:
		//	component = NEW CLight2D;
		//	break;
		case EComponent::E_Light:
			component = NEW CLight;
			break;
		case EComponent::E_MeshRender:
			component = NEW CMeshRender;
			break;
		//case EComponent::E_BillboardRender:
		//	component = NEW CBillboardRender;
		//	break;
		//case EComponent::E_SpriteRender:
		//	component = NEW CSpriteRender;
		//	break;
		//case EComponent::E_FlipbookRender:
		//	component = NEW CFlipbookRender;
		//	break;
		//case EComponent::E_ParticleRender:
		//	break;
		//case EComponent::E_TileRender:
		//	component = NEW CTileRender;
		//	break;
		//case EComponent::E_Rigidbody:
		//	component = NEW CRigidBody;
		//	break;
		}
		
		CheckReturn(AddComponent(component));
		component->LoadFromLevelFile(pFile);
	}

	// 스크립트
	size_t numScripts{};
	fread(&numScripts, sizeof(numScripts), 1, pFile);

	for (size_t i = 0; i < numScripts; ++i) {
		Hash scriptID{};
		fread(&scriptID, sizeof(scriptID), 1, pFile);

		Ptr<CScript> script = SCRIPT_MANAGER->CreateScript(scriptID);
		CheckReturn(AddComponent(script.Get()));
	
		CheckReturn(AddComponent(script.Get()));
	}

	// 자식 객체
	size_t numChildren{};
	fread(&numChildren, sizeof(numChildren), 1, pFile);

	for (size_t i = 0; i < numChildren; ++i) {
		Ptr<GameObject> child = NEW GameObject;
		CheckReturn(AddChild(child));
		CheckReturn(child->LoadFromLevelFile(pFile));
	}

	return true;
}

bool GameObject::RegisterLayer() {
	Ptr<ALevel> currLevel = LEVEL_MANAGER->GetCurrentLevel();
	if (currLevel == nullptr) ReturnFalse("Current level is null");

	Layer* layer = currLevel->GetLayer(mLayer);
	CheckReturn(layer->RegisterObject(this));

	return true;
}