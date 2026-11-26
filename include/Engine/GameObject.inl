#ifndef __GAMEOBJECT_INL__
#define __GAMEOBJECT_INL__

Ptr<Component> GameObject::GetComponent(EComponent::Type type) const noexcept {
	return mComponents[type];
}

Ptr<CRenderComponent> GameObject::GetRenderComponent() const noexcept {
	return mRenderComponent;
}

Ptr<CCollider> GameObject::GetColliderComponent() const noexcept {
	return mColliderComponent;
}

Ptr<GameObject> GameObject::GetChild(size_t index) const noexcept { return mChildren[index]; }

Ptr<GameObject> GameObject::GetParent() const noexcept { return mpParent; }

const std::vector<Ptr<GameObject>>& GameObject::GetChildren() const noexcept { return mChildren; }

bool GameObject::IsDead() const noexcept { return mbIsDead; }

int GameObject::GetLayer() const noexcept { return mLayer; }

#endif // __GAMEOBJECT_INL__