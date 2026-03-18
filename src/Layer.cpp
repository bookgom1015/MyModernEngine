#include "pch.h"
#include "Layer.hpp"

Layer::Layer() : mLayer{ -1 } {}

Layer::Layer(const Layer& other) 
	: Entity{ other }
	, mLayer{ other.mLayer } {
	for (const auto& object : other.mParents)
		AddObject(object->Clone());
}

Layer::~Layer() {}

bool Layer::Begin() {
	for (size_t i = 0, end = mParents.size(); i < end; ++i)
		CheckReturn(mParents[i]->Begin());

	return true;
}

bool Layer::Update(float dt) {
	for (size_t i = 0, end = mParents.size(); i < end; ++i)
		CheckReturn(mParents[i]->Update(dt));

	return true;
}

bool Layer::FixedUpdate(float dt) {
	for (size_t i = 0, end = mParents.size(); i < end; ++i)
		CheckReturn(mParents[i]->FixedUpdate(dt));

	return true;
}

bool Layer::LateUpdate(float dt) {
	for (size_t i = 0, end = mParents.size(); i < end; ++i)
		CheckReturn(mParents[i]->LateUpdate(dt));

	return true;
}

bool Layer::Final() {
	auto iter = mParents.begin();
	auto end = mParents.end();

	for (; iter != end;) {
		CheckReturn((*iter)->Final());

		if ((*iter)->IsDead()) iter = mParents.erase(iter);
		else ++iter;
	}

	return true;
}

bool Layer::Render() {
	for (size_t i = 0, end = mParents.size(); i < end; ++i)
		CheckReturn(mParents[i]->Render());

	return true;
}

bool Layer::AddObject(Ptr<GameObject> obj) {
	mParents.push_back(obj);

	std::list<GameObject*> queue{};
	queue.push_back(obj.Get());

	while (!queue.empty()) {
		auto pObject = queue.front();
		queue.pop_back();

		pObject->mLayer = mLayer; 

		for (size_t i = 0, end = pObject->mChildren.size(); i < end; ++i)	
			queue.push_back(pObject->mChildren[i].Get());
	}

	return true;
}

bool Layer::RegisterObject(Ptr<GameObject> obj) {
	mAllObjects.push_back(obj);

	return true;
}

bool Layer::DeregisterAllObjects() {
	mAllObjects.clear();

	return true;
}

bool Layer::DeregisterAsParent(Ptr<GameObject> obj) {
	auto iter = mParents.begin();
	auto end = mParents.end();

	for (; iter != end; ++iter) {
		if (*iter == obj) {
			mParents.erase(iter);
			return true;
		}
	}

	ReturnFalse("The object is not registered as a parent in this layer");
}