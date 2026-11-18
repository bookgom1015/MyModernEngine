#pragma once

#include "Engine/EditorUI.hpp"

#include "Engine/GameObject.hpp"

class ComponentUI : public EditorUI {
public:
	ComponentUI(EComponent::Type type, const std::string& name);
	virtual ~ComponentUI();

protected:
	virtual void OutputTitle(const std::string& title);

	virtual void TargetChanged();

public:
	virtual void SetTarget(Ptr<GameObject> obj);
	__forceinline Ptr<GameObject> GetTarget() const;

private:
	void RemoveComponent();

private:
	Ptr<GameObject> mTarget;
	const EComponent::Type mType;
};

#include "Engine/ComponentUI.inl"