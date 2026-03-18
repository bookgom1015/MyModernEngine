#pragma once

#include "Entity.hpp"
#include "GameObject.hpp"

class Layer : public Entity {
	friend class ALevel;

public:
	Layer();
	Layer(const Layer& other);
	virtual ~Layer();

public:
	bool Begin();

	bool Update(float dt);
	bool FixedUpdate(float dt);
	bool LateUpdate(float dt);

	bool Final();

	bool Render();

public:
	bool AddObject(Ptr<GameObject> obj);

	bool RegisterObject(Ptr<GameObject> obj);
	bool DeregisterAllObjects();

	bool DeregisterAsParent(Ptr<GameObject> obj);

public:
	__forceinline const std::vector<Ptr<GameObject>>& GetParents() const noexcept;
	__forceinline const std::vector<Ptr<GameObject>>& GetAllObjects() const noexcept;

private:
	std::vector<Ptr<GameObject>> mParents;
	std::vector<Ptr<GameObject>> mAllObjects;

	int mLayer;
};

#include "Layer.inl"