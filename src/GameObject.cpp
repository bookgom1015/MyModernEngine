#include "pch.h"
#include "GameObject.hpp"

GameObject::GameObject() {}

GameObject::GameObject(const GameObject& other) {}

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

	for (size_t i = 0, end = mChildren.size(); i < end; ++i)
		CheckReturn(mChildren[i]->Update(dt));

	return true;
}

bool GameObject::FixedUpdate(float dt) {
	for (size_t i = 0, end = mScripts.size(); i < end; ++i)
		CheckReturn(mScripts[i]->FixedUpdate(dt));

	for (size_t i = 0, end = mChildren.size(); i < end; ++i)
		CheckReturn(mChildren[i]->FixedUpdate(dt));

	return true;
}

bool GameObject::LateUpdate(float dt) {
	for (size_t i = 0, end = mScripts.size(); i < end; ++i)
		CheckReturn(mScripts[i]->LateUpdate(dt));

	for (size_t i = 0, end = mChildren.size(); i < end; ++i)
		CheckReturn(mChildren[i]->LateUpdate(dt));

	return true;
}

bool GameObject::Final() {
	for (size_t i = 0; i < EComponent::Count; ++i)
		if (mComponents[i] != nullptr) CheckReturn(mComponents[i]->Final());

	// 자신이 소속된 Layer 에 자기자신을 알림(등록)
	RegisterLayer();

	auto iter = mChildren.begin();
	auto end = mChildren.end();
	for (; iter != end;) {
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
	auto end = mChildren.end();
	for (; iter != end;) {
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
	//// 렌더링 기능 컴포넌트는 하나만 가질 수 있음
	//if (dynamic_cast<CRenderComponent*>(_Com.Get())) {
	//	assert(!m_RenderCom.Get());
	//
	//	m_RenderCom = (CRenderComponent*)_Com.Get();
	//}
	//
	//// 입력으로 들어온 컴포넌트가 스크립트면, vector 로 관리
	//if (_Com->GetType() == EComponent::E_Script) {
	//	m_vecScripts.push_back((CScript*)_Com.Get());
	//}
	//// 입력으로 들어온 컴포넌트가 스크립트가 아니면, 알맞은 배열 포인터로 가리킴
	//else {
	//	// 해당 컴포넌트를 이미 가지고 있지 않아야 한다.
	//	assert(nullptr == m_Com[(UINT)_Com->GetType()]);
	//	m_Com[_Com->GetType()] = _Com;
	//}
	//
	//_Com->m_Owner = this;
	//_Com->Init();

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
		child->DisconnectWithParent();
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
	return true;
}

bool GameObject::LoadFromLevelFile(FILE* const pFile) {
	// TODO: implement loading logic
	(void)pFile;
	return true;
}

bool GameObject::RegisterLayer() {


	return true;
}