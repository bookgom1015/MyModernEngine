#pragma once

#include "Entity.hpp"
#include "Components.hpp"

#define GET_COMPONENT(COM_NAME, COM_TYPE) Ptr<C##COM_NAME> COM_NAME() {			\
    return static_cast<C##COM_NAME*>(mComponents[EComponent::COM_TYPE].Get());	\
}

class GameObject : public Entity {
	friend class Layer;
	friend class TaskManager;

public:
	GameObject();
	GameObject(const GameObject& other);
	virtual ~GameObject();

public:
	bool Begin();

	bool Update(float dt);
	bool FixedUpdate(float dt);
	bool LateUpdate(float dt);

	bool Final();
	bool FinalEditor();

	bool Render();

public:
	bool AddComponent(Ptr<Component> comp);
	bool RemoveComponent(EComponent::Type type);

	bool AddChild(Ptr<GameObject> child);
	bool DisconnectWithParent();

	bool RegisterAsParent();
	bool DeregisterAsParent();

	void Destroy();

public:
	CLONE(GameObject);

	bool SaveToLevelFile(FILE* const pFile);
	bool LoadFromLevelFile(FILE* const pFile);

public:
	GET_COMPONENT(Transform, E_Transform);

	GET_COMPONENT(Camera, E_Camera);
	GET_COMPONENT(MeshRender, E_MeshRender);

public:
	__forceinline Ptr<Component> GetComponent(EComponent::Type type) const noexcept;
	__forceinline Ptr<CRenderComponent> GetRenderComponent() const noexcept;

	__forceinline Ptr<GameObject> GetParent() const noexcept;
	__forceinline Ptr<GameObject> GetChild(size_t index) const noexcept;
	__forceinline const std::vector<Ptr<GameObject>>& GetChildren() const noexcept;

	__forceinline bool IsDead() const noexcept;

	__forceinline int GetLayer() const noexcept;

private:
	bool RegisterLayer();

private:
	GameObject* mpParent;
	std::vector<Ptr<GameObject>> mChildren;

	std::array<Ptr<Component>, EComponent::Count> mComponents;
	Ptr<CRenderComponent> mRenderComponent;

	std::vector<Ptr<CScript>> mScripts;

	int mLayer;
	bool mbIsDead;
};

#include "GameObject.inl"