#include "Engine/pch.h"
#include "Engine/GameObject.hpp"

#include "Engine/ScriptManager.hpp"
#include "Engine/LevelManager.hpp"
#include "Engine/TaskManager.hpp"
#include "Engine/EditorManager.hpp"

GameObject::GameObject() 
	: mpParent{}
	, mChildren{}
	, mComponents{}	
	, mRenderComponent{nullptr}
	, mColliderComponent{ nullptr }
	, mScripts{}
	, mLayer{ -1 }
	, mbIsDead{} {}

GameObject::GameObject(const GameObject& other) 
	: Entity{ other }
	, mpParent{}
	, mComponents{}
	, mLayer{ ELevelLayer::E_Default}
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

	// 객체를 소속 레이어에 등록합니다.
	CheckReturn(RegisterLayer());

	auto iter = mChildren.begin();
	for (; iter != mChildren.end();) {
		CheckReturn((*iter)->Final());

		if ((*iter)->IsDead())
			// 제거된 자식 객체를 목록에서 제외합니다.
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
			// 제거된 자식 객체를 목록에서 제외합니다.
			iter = mChildren.erase(iter);
		else
			++iter;
	}

	return true;
}

bool GameObject::Render() { return true; }

bool GameObject::OnLoaded() { 
	for (size_t i = 0, end = mScripts.size(); i < end; ++i)
		CheckReturn(mScripts[i]->OnLoaded());

	for (size_t i = 0; i < EComponent::Count; ++i)
		if (mComponents[i] != nullptr) CheckReturn(mComponents[i]->OnLoaded());

	for (size_t i = 0, end = mChildren.size(); i < end; ++i)
		CheckReturn(mChildren[i]->OnLoaded());

	return true; 
}

bool GameObject::OnUnloaded() { 
	for (size_t i = 0, end = mScripts.size(); i < end; ++i)
		CheckReturn(mScripts[i]->OnUnloaded());

	for (size_t i = 0; i < EComponent::Count; ++i)
		if (mComponents[i] != nullptr) CheckReturn(mComponents[i]->OnUnloaded());

	for (size_t i = 0, end = mChildren.size(); i < end; ++i)
		CheckReturn(mChildren[i]->OnUnloaded());

	return true;
}

bool GameObject::AddComponent(Ptr<Component> comp) {
	// 렌더링 컴포넌트는 하나만 가질 수 있습니다.
	if (dynamic_cast<CRenderComponent*>(comp.Get())) {
		if (mRenderComponent != nullptr) {
			LOG_WARNING("GameObject already has a render component. "
				"Only one render component is allowed per GameObject.");

			return false;
		}
	
		mRenderComponent = static_cast<CRenderComponent*>(comp.Get());
	}
	else if (dynamic_cast<CCollider*>(comp.Get())) {
		if (mColliderComponent != nullptr) {
			LOG_WARNING("GameObject already has a collider component. "
				"Only one collider component is allowed per GameObject.");
			return false;
		}

		mColliderComponent = static_cast<CCollider*>(comp.Get());
	}
	
	// 전달받은 컴포넌트가 스크립트라면 벡터에 보관합니다.
	if (comp->GetType() == EComponent::E_Script) {
		mScripts.push_back(static_cast<CScript*>(comp.Get()));
	}
	// 그 밖의 컴포넌트는 배열의 해당 위치에서 포인터로 참조합니다.
	else {
		// 같은 컴포넌트가 이미 등록되어 있으면 안 됩니다.
		if (mComponents[comp->GetType()] != nullptr) {
			LOG_WARNING_FORMAT("GameObject already has a component of type {}. "
				"Only one component of each type is allowed per GameObject.",
				EComponent::ComponentTypeToString(comp->GetType()));

			return false;
		}

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

		// 렌더링 컴포넌트를 제거하면 mRenderComponent를 nullptr로 초기화합니다.
		if (type == EComponent::E_MeshRender 
			|| type == EComponent::E_SkeletalMeshRender 
			|| type == EComponent::E_SkySphereRender)
			mRenderComponent = nullptr;
		// 충돌 컴포넌트를 제거하면 mColliderComponent를 nullptr로 초기화합니다.
		else if (type == EComponent::E_BoxCollider
			|| type == EComponent::E_SphereCollider
			|| type == EComponent::E_CapsuleCollider
			|| type == EComponent::E_MeshCollider)
			mColliderComponent = nullptr;

		comp = nullptr;
	}

	return true;
}

bool GameObject::AddChild(Ptr<GameObject> child) {
	// 부모 객체가 있는지 확인합니다.
	if (child->GetParent() != nullptr) {
		// 기존 부모 객체와의 관계를 해제합니다.
		CheckReturn(child->DisconnectWithParent());
	}
	// 기존에 최상위 부모 객체였던 경우입니다.
	else {
		// 레벨에 속해 있던 객체인 경우입니다.
		if (child->mLayer != -1) 
			// 레이어에서 최상위 부모 객체를 가리키던 포인터를 제거합니다.
			child->DeregisterAsParent();
	}

	mChildren.push_back(child);
	child->mpParent = this;

	if (child->mLayer == -1) {	
		child->mLayer = mLayer;

		if (mLayer != -1 && LEVEL_MANAGER->GetCurrentLevelState() == ELevelState::E_Playing)
			CheckReturn(child->Begin());
	}

	if (mLayer != -1) LEVEL_MANAGER->GetCurrentLevel()->Change();

	return true;
}

bool GameObject::DisconnectWithParent() {
	if (!mpParent) return true;

	if (mLayer != -1) LEVEL_MANAGER->GetCurrentLevel()->Change();

	auto iter = mpParent->mChildren.begin();
	for (; iter != mpParent->mChildren.end(); ++iter) {
		if (*iter == this) {
			mpParent->mChildren.erase(iter);
			mpParent = nullptr;

			return true;
		}
	}

	return true;
}

bool GameObject::RegisterAsParent() {
	if (mLayer == -1) return true;

	CheckReturn(LEVEL_MANAGER->GetCurrentLevel()->GetLayer(mLayer)->AddObject(this));

	return true;
}

bool GameObject::DeregisterAsParent() {
	auto currLevel = LEVEL_MANAGER->GetCurrentLevel();
	auto layer = currLevel->GetLayer(mLayer);
	CheckReturn(layer->DeregisterAsParent(this));

	return true;
}

void GameObject::Destroy() {
	if (mbIsDead) return;

	TaskInfo info{};

	info.Type = ETask::E_DestroyObject;
	info.Param_0 = reinterpret_cast<DWORD_PTR>(this);

	TASK_MANAGER->AddTask(info);

	mbIsDead = true;
}

bool GameObject::SaveToLevelFile(FILE* const pFile) {
	auto objectName = GetName();
	SaveWString(pFile, objectName);

	// 컴포넌트
	for (UINT i = 0; i < EComponent::Count; ++i) {
		decltype(auto) comp = mComponents[i];
		if (comp == nullptr) continue;

		// 컴포넌트 유형
		fwrite(&i, sizeof(i), 1, pFile);

		// 컴포넌트 데이터
		CheckReturn(comp->SaveToLevelFile(pFile));
	}

	UINT end = EComponent::Count;
	fwrite(&end, sizeof(end), 1, pFile);

	// 스크립트
	auto size = static_cast<std::uint32_t>(mScripts.size());
	fwrite(&size, sizeof(size), 1, pFile);

	for (const auto& script : mScripts) {
		auto scriptID = script->GetID();
		fwrite(&scriptID, sizeof(scriptID), 1, pFile);
	
		CheckReturn(script->SaveToLevelFile(pFile));
	}

	// 자식 객체
	auto numChildren = static_cast<std::uint32_t>(mChildren.size());
	fwrite(&numChildren, sizeof(numChildren), 1, pFile);

	for (const auto& child : mChildren)
		CheckReturn(child->SaveToLevelFile(pFile));

	return true;
}

bool GameObject::LoadFromLevelFile(FILE* const pFile) {
	auto objectName = LoadWString(pFile);
	SetName(objectName);

	// 컴포넌트
	while (true) {
		UINT comType{};
		fread(&comType, sizeof(comType), 1, pFile);
		if (comType == EComponent::Count) break;

		Component* component{};
		switch (comType) {
		case EComponent::E_Transform:
			component = NEW CTransform;
			break;
		case EComponent::E_Camera:
			component = NEW CCamera;
			break;
		case EComponent::E_Light:
			component = NEW CLight;
			break;
		case EComponent::E_MeshRender:
			component = NEW CMeshRender;
			break;
		case EComponent::E_SkeletalMeshRender:
			component = NEW CSkeletalMeshRender;
			break;		
		case EComponent::E_SkySphereRender:
			component = NEW CSkySphereRender;
			break;
		case EComponent::E_ReflectionProbe:
			component = NEW CReflectionProbe;
			break;
		case EComponent::E_BoxCollider:
			component = NEW CBoxCollider;
			break;
		case EComponent::E_SphereCollider:
			component = NEW CSphereCollider;
			break;
		case EComponent::E_CapsuleCollider:
			component = NEW CCapsuleCollider;
			break;
		case EComponent::E_MeshCollider:
			component = NEW CMeshCollider;
			break;
		case EComponent::E_Rigidbody:
			component = NEW CRigidbody;
			break;
		default:
			ReturnFalse("Undefined component type read from level file");
		}
		
		CheckReturn(AddComponent(component));
		component->LoadFromLevelFile(pFile);
	}

	// 스크립트
	std::uint32_t numScripts{};
	fread(&numScripts, sizeof(numScripts), 1, pFile);

	for (std::uint32_t i = 0; i < numScripts; ++i) {
		Hash scriptID{};
		fread(&scriptID, sizeof(scriptID), 1, pFile);

		Ptr<CScript> script = SCRIPT_MANAGER->CreateScript(scriptID);
		CheckReturn(AddComponent(script.Get()));
		CheckReturn(script->LoadFromLevelFile(pFile));
	}

	// 자식 객체
	std::uint32_t numChildren{};
	fread(&numChildren, sizeof(numChildren), 1, pFile);

	for (std::uint32_t i = 0; i < numChildren; ++i) {
		Ptr<GameObject> child = NEW GameObject;
		CheckReturn(AddChild(child));
		CheckReturn(child->LoadFromLevelFile(pFile));
	}

	return true;
}

bool GameObject::RegisterLayer() {
	auto currLevel = LEVEL_MANAGER->GetCurrentLevel();
	auto layer = currLevel->GetLayer(mLayer);
	CheckReturn(layer->RegisterObject(this));

	return true;
}